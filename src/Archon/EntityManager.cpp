#include <Archon/EntityManager.h>

#include <cassert>

namespace Archon
{
    ArchetypeStorage* EntityManager::GetOrCreateArchetypeStorage(const ArchetypeId id)
    {
        if (ArchetypeRegistry::TryGetInfo(id) == nullptr)
        {
            return nullptr;
        }

        const size_t pageIndex = id / ArchetypePageSize;
        if (pageIndex >= m_archetypePages.size())
        {
            m_archetypePages.resize(pageIndex + 1);
        }

        if (!m_archetypePages[pageIndex])
        {
            m_archetypePages[pageIndex] = std::make_unique<ArchetypePage>();
        }

        auto& storage = m_archetypePages[pageIndex]->entries[id % ArchetypePageSize];
        if (!storage)
        {
            storage = std::unique_ptr<ArchetypeStorage>(new ArchetypeStorage(id));
        }

        return storage.get();
    }

    ArchetypeStorage& EntityManager::GetArchetype(const ArchetypeId id)
    {
        ArchetypeStorage* storage = GetOrCreateArchetypeStorage(id);
        assert(storage != nullptr);
        return *storage;
    }

    ArchetypeStorage* EntityManager::TryGetArchetype(const ArchetypeId id)
    {
        if (id == InvalidArchetypeId)
        {
            return nullptr;
        }

        const size_t pageIndex = id / ArchetypePageSize;
        if (pageIndex >= m_archetypePages.size() || !m_archetypePages[pageIndex])
        {
            return nullptr;
        }

        return m_archetypePages[pageIndex]->entries[id % ArchetypePageSize].get();
    }

    ArchetypeId EntityManager::CreateArchetype(const ComponentMask& componentMask)
    {
        const ArchetypeId id = ArchetypeRegistry::Register(componentMask);
        if (id != InvalidArchetypeId)
        {
            GetOrCreateArchetypeStorage(id);
        }

        return id;
    }

    EntityId EntityManager::CreateEntity(const ArchetypeId archetypeId)
    {
        if (archetypeId == InvalidArchetypeId || ArchetypeRegistry::TryGetInfo(archetypeId) == nullptr)
        {
            return EntityId::Null;
        }

        ArchetypeStorage* archetype = GetOrCreateArchetypeStorage(archetypeId);
        if (archetype == nullptr)
        {
            return EntityId::Null;
        }

        IndexType index;
        EntityInfo* info = nullptr;

        if (m_freeListHead != 0)
        {
            index = m_freeListHead;
            info = &m_entityInfo.GetOrCreate(index);

            const EntityId entity(index, info->version);
            const EntityLocation location = archetype->AddEntity(entity);

            m_freeListHead = info->nextFreeIndex;
            info->nextFreeIndex = 0;
            info->location = location;
            return entity;
        }

        if (m_nextEntityIndex > EntityId::MaxIndex)
        {
            return EntityId::Null;
        }

        index = static_cast<IndexType>(m_nextEntityIndex);
        info = &m_entityInfo.GetOrCreate(index);
        info->version = EntityId::FirstVersion;

        const EntityId entity(index, info->version);
        const EntityLocation location = archetype->AddEntity(entity);

        info->location = location;
        ++m_nextEntityIndex;
        return entity;
    }

    void EntityManager::DestroyEntity(const EntityId entity)
    {
        EntityInfo* info = m_entityInfo.TryGet(entity.GetIndex());
        if (info == nullptr || !info->location.IsValid() || info->version != entity.GetVersion())
        {
            return;
        }

        const EntityLocation location = info->location;
        ArchetypeStorage* archetype = TryGetArchetype(location.archetypeId);
        if (archetype == nullptr)
        {
            return;
        }

        if (const std::optional<EntityId> movedEntity = archetype->RemoveEntity(location))
        {
            m_entityInfo.GetOrCreate(movedEntity->GetIndex()).location = location;
        }

        info->location = {};
        info->version = EntityId::NextVersion(info->version);
        info->nextFreeIndex = m_freeListHead;
        m_freeListHead = entity.GetIndex();
    }

    bool EntityManager::IsValid(const EntityId entity) const
    {
        const EntityInfo* info = m_entityInfo.TryGet(entity.GetIndex());
        return info != nullptr && info->location.IsValid() && info->version == entity.GetVersion();
    }
}
