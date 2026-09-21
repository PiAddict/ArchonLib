#include <Archon/EntityManager.h>

#include <cassert>

namespace Archon
{
    ArchetypeStorage* EntityManager::InitializeArchetypeStorage(const ArchetypeId id)
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

    ArchetypeStorage* EntityManager::FindArchetypeStorage(const ArchetypeId id) const
    {
        if (ArchetypeRegistry::TryGetInfo(id) == nullptr)
        {
            return nullptr;
        }

        const size_t pageIndex = id / ArchetypePageSize;
        if (pageIndex >= m_archetypePages.size())
        {
            return nullptr;
        }

        if (!m_archetypePages[pageIndex])
        {
            return nullptr;
        }

        const auto& storage = m_archetypePages[pageIndex]->entries[id % ArchetypePageSize];
        if (!storage)
        {
            return nullptr;
        }

        return storage.get();
    }

    EntityInfo* EntityManager::TryGetEntityInfo(const EntityId entity)
    {
        EntityInfo* info = m_entityInfo.TryGet(entity.GetIndex());
        return info != nullptr && info->IsValidFor(entity) ? info : nullptr;
    }

    const EntityInfo* EntityManager::TryGetEntityInfo(const EntityId entity) const
    {
        const EntityInfo* info = m_entityInfo.TryGet(entity.GetIndex());
        return info != nullptr && info->IsValidFor(entity) ? info : nullptr;
    }

    ArchetypeStorage& EntityManager::GetArchetypeStorage(const ArchetypeId id)
    {
        ArchetypeStorage* storage = InitializeArchetypeStorage(id);
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

    const ArchetypeStorage& EntityManager::GetArchetypeStorage(const ArchetypeId id) const
    {
        ArchetypeStorage* storage = FindArchetypeStorage(id);
        assert(storage != nullptr);
        return *storage;
    }

    const ArchetypeStorage* EntityManager::TryGetArchetype(const ArchetypeId id) const
    {
        return FindArchetypeStorage(id);
    }

    ArchetypeId EntityManager::CreateArchetype(const ComponentMask& componentMask)
    {
        const ArchetypeId id = ArchetypeRegistry::GetId(componentMask);
        if (id != InvalidArchetypeId)
        {
            InitializeArchetypeStorage(id);
        }

        return id;
    }

    EntityId EntityManager::CreateEntity(const ArchetypeId archetypeId)
    {
        if (archetypeId == InvalidArchetypeId || ArchetypeRegistry::TryGetInfo(archetypeId) == nullptr)
        {
            return EntityId::Null;
        }

        ArchetypeStorage* archetype = InitializeArchetypeStorage(archetypeId);
        if (archetype == nullptr)
        {
            return EntityId::Null;
        }

        IndexType index;
        EntityInfo* info = nullptr;

        if (m_freeListHead != 0)
        {
            index = m_freeListHead;
            info = &m_entityInfo.Get(index);

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
        info = &m_entityInfo.Get(index);
        info->version = EntityId::FirstVersion;

        const EntityId entity(index, info->version);
        const EntityLocation location = archetype->AddEntity(entity);

        info->location = location;
        ++m_nextEntityIndex;
        return entity;
    }

    std::byte* EntityManager::TryGetComponent(EntityId entity, ComponentId componentId)
    {
        EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr)
        {
            return nullptr;
        }

        ArchetypeStorage* const storage = TryGetArchetype(entityInfo->location.archetypeId);
        if (storage == nullptr)
        {
            return nullptr;
        }

        return storage->TryGetComponentData(componentId, entityInfo->location.chunkIndex, entityInfo->location.columnIndex);
    }

    const std::byte* EntityManager::TryGetComponent(EntityId entity, ComponentId componentId) const
    {
        const EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr)
        {
            return nullptr;
        }

        const ArchetypeStorage* const storage = TryGetArchetype(entityInfo->location.archetypeId);
        if (!storage)
        {
            return nullptr;
        }

        return storage->TryGetComponentData(componentId, entityInfo->location.chunkIndex, entityInfo->location.columnIndex);
    }

    const ComponentMask* EntityManager::TryGetComponentMask(const EntityId entity) const
    {
        const EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr)
        {
            return nullptr;
        }

        const ArchetypeInfo* archetypeInfo = ArchetypeRegistry::TryGetInfo(entityInfo->location.archetypeId);
        return archetypeInfo != nullptr ? &archetypeInfo->componentMask : nullptr;
    }

    bool EntityManager::TrySetComponent(const EntityId entity, const ComponentId componentId, const std::byte* component)
    {
        EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr || component == nullptr)
        {
            return false;
        }

        const ComponentMask* componentMask = TryGetComponentMask(entity);
        if (componentMask == nullptr || !componentMask->Test(componentId))
        {
            return false;
        }

        ArchetypeStorage* storage = TryGetArchetype(entityInfo->location.archetypeId);
        if (storage == nullptr)
        {
            return false;
        }

        storage->SetComponentData(componentId, entityInfo->location.chunkIndex, entityInfo->location.columnIndex, component);
        return true;
    }

    bool EntityManager::TryAddComponent(const EntityId entity, const ComponentId componentId, const std::byte* component)
    {
        EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr || component == nullptr)
        {
            return false;
        }

        const ComponentMask* currentComponentMask = TryGetComponentMask(entity);
        if (currentComponentMask == nullptr || currentComponentMask->Test(componentId))
        {
            return false;
        }

        ComponentMask newComponentMask = *currentComponentMask;
        newComponentMask.Set(componentId);
        return MigrateEntity(entity, *entityInfo, newComponentMask, componentId, component);
    }

    bool EntityManager::TryRemoveComponent(const EntityId entity, const ComponentId componentId)
    {
        EntityInfo* entityInfo = TryGetEntityInfo(entity);
        if (entityInfo == nullptr)
        {
            return false;
        }

        const ComponentMask* currentComponentMask = TryGetComponentMask(entity);
        if (currentComponentMask == nullptr || !currentComponentMask->Test(componentId))
        {
            return false;
        }

        ComponentMask newComponentMask = *currentComponentMask;
        newComponentMask.Clear(componentId);
        return MigrateEntity(entity, *entityInfo, newComponentMask);
    }

    bool EntityManager::MigrateEntity(const EntityId entity, EntityInfo& entityInfo, const ComponentMask& newComponentMask,
        const ComponentId addedComponentId, const std::byte* addedComponent)
    {
        if (addedComponentId != InvalidComponentId && addedComponent == nullptr)
        {
            assert(false && "Added component data must be provided during migration");
            return false;
        }

        const EntityLocation oldLocation = entityInfo.location;
        ArchetypeStorage* oldStorage = TryGetArchetype(oldLocation.archetypeId);
        const ArchetypeInfo* oldArchetypeInfo = ArchetypeRegistry::TryGetInfo(oldLocation.archetypeId);
        if (oldStorage == nullptr || oldArchetypeInfo == nullptr)
        {
            return false;
        }

        const ArchetypeId newArchetypeId = CreateArchetype(newComponentMask);
        ArchetypeStorage* newStorage = TryGetArchetype(newArchetypeId);
        if (newStorage == nullptr)
        {
            return false;
        }

        const EntityLocation newLocation = newStorage->AddEntity(entity);
        oldArchetypeInfo->componentMask.ForEachComponentId(
            [oldStorage, newStorage, &newComponentMask, oldLocation, newLocation](const ComponentId componentId)
            {
                if (!newComponentMask.Test(componentId))
                {
                    return;
                }

                const std::byte* componentData = oldStorage->TryGetComponentData(componentId, oldLocation.chunkIndex, oldLocation.columnIndex);
                assert(componentData != nullptr && "Source archetype is missing a declared component");
                if (componentData != nullptr)
                {
                    newStorage->SetComponentData(componentId, newLocation.chunkIndex, newLocation.columnIndex, componentData);
                }
            });

        if (addedComponentId != InvalidComponentId)
        {
            newStorage->SetComponentData(addedComponentId, newLocation.chunkIndex, newLocation.columnIndex, addedComponent);
        }

        if (const std::optional<EntityId> movedEntity = oldStorage->RemoveEntity(oldLocation))
        {
            EntityInfo* movedEntityInfo = TryGetEntityInfo(*movedEntity);
            assert(movedEntityInfo != nullptr && "Swap-moved entity must remain valid");
            if (movedEntityInfo != nullptr)
            {
                movedEntityInfo->location = oldLocation;
            }
        }

        entityInfo.location = newLocation;
        return true;
    }

    void EntityManager::DestroyEntity(const EntityId entity)
    {
        EntityInfo* info = TryGetEntityInfo(entity);
        if (info == nullptr)
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
            EntityInfo* movedEntityInfo = TryGetEntityInfo(*movedEntity);
            assert(movedEntityInfo != nullptr && "Swap-moved entity must remain valid");
            if (movedEntityInfo != nullptr)
            {
                movedEntityInfo->location = location;
            }
        }

        info->location = {};
        info->version = EntityId::NextVersion(info->version);
        info->nextFreeIndex = m_freeListHead;
        m_freeListHead = entity.GetIndex();
    }

    bool EntityManager::IsValid(const EntityId entity) const
    {
        return TryGetEntityInfo(entity) != nullptr;
    }
}
