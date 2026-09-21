#ifndef ARCHON_ENTITYMANAGER_H
#define ARCHON_ENTITYMANAGER_H

#include <array>
#include <cstddef>
#include <deque>
#include <memory>

#include <Archon/ArchetypeRegistry.h>
#include <Archon/ArchetypeStorage.h>
#include <Archon/EntityInfo.h>

namespace Archon
{
    class EntityManager
    {
        static constexpr size_t ArchetypePageSize = 256;

        struct ArchetypePage
        {
            std::array<std::unique_ptr<ArchetypeStorage>, ArchetypePageSize> entries{};
        };

        EntityInfoTable m_entityInfo;
        std::deque<std::unique_ptr<ArchetypePage>> m_archetypePages;
        EntityType m_nextEntityIndex = EntityId::FirstIndex;
        IndexType m_freeListHead = 0;

        ArchetypeStorage* InitializeArchetypeStorage(ArchetypeId id);
        ArchetypeStorage* FindArchetypeStorage(ArchetypeId id) const;

    public:
        ArchetypeStorage& GetArchetypeStorage(ArchetypeId id);
        ArchetypeStorage* TryGetArchetype(ArchetypeId id);

        const ArchetypeStorage& GetArchetypeStorage(ArchetypeId id) const;
        const ArchetypeStorage* TryGetArchetype(ArchetypeId id) const;

        ArchetypeId CreateArchetype(const ComponentMask& componentMask);

        template <typename... ComponentTypes>
        ArchetypeId CreateArchetype();

        EntityId CreateEntity(ArchetypeId archetypeId);

        template <typename... ComponentTypes>
        EntityId CreateEntity();

        template <typename ComponentType>
        ComponentType* TryGetComponent(EntityId entity);

        template <typename ComponentType>
        const ComponentType* TryGetComponent(EntityId entity) const;

        [[nodiscard]] std::byte* TryGetComponent(EntityId entity, ComponentId componentId);
        [[nodiscard]] const std::byte* TryGetComponent(EntityId entity, ComponentId componentId) const;

        template <typename ComponentType>
        void SetComponent(EntityId entity, const ComponentType& component);

        void DestroyEntity(EntityId entity);
        [[nodiscard]] bool IsValid(EntityId entity) const;
    };

    template <typename... ComponentTypes>
    ArchetypeId EntityManager::CreateArchetype()
    {
        return CreateArchetype(ComponentMask::Create<ComponentTypes...>());
    }

    template <typename... ComponentTypes>
    EntityId EntityManager::CreateEntity()
    {
        return CreateEntity(ArchetypeRegistry::GetId<ComponentTypes...>());
    }

    template <typename ComponentType>
    ComponentType* EntityManager::TryGetComponent(EntityId entity)
    {
        return reinterpret_cast<ComponentType*>(TryGetComponent(entity, ComponentRegistry::GetId<ComponentType>()));
    }

    template <typename ComponentType>
    const ComponentType* EntityManager::TryGetComponent(EntityId entity) const
    {
        return reinterpret_cast<const ComponentType*>(TryGetComponent(entity, ComponentRegistry::GetId<ComponentType>()));
    }

    template <typename ComponentType>
    void EntityManager::SetComponent(EntityId entity, const ComponentType& component)
    {
        if (!IsValid(entity))
        {
            return;
        }

        const EntityInfo* entityInfo = m_entityInfo.TryGet(entity.GetIndex());
        assert(entityInfo);

        ArchetypeStorage* storage = TryGetArchetype(entityInfo->location.archetypeId);
        assert(storage != nullptr);
        if (storage == nullptr)
        {
            return;
        }

        const ComponentMask& currentArchetypeMask = ArchetypeRegistry::GetInfo(entityInfo->location.archetypeId).componentMask;
        if (!currentArchetypeMask.Test<ComponentType>())
        {
            // Migrate entity to new archetype
            // TODO: This should go into a deferred command buffer at some stage so we're not making structural changes randomly.

            ComponentMask newArchetypeMask = currentArchetypeMask;
            newArchetypeMask.Set<ComponentType>();

            const EntityLocation oldLocation = entityInfo->location;
            const ArchetypeId id = CreateArchetype(newArchetypeMask);
            ArchetypeStorage& newStorage = GetArchetypeStorage(id);

            const EntityLocation location = newStorage.AddEntity(entity);

            currentArchetypeMask.ForEachComponentId([this, entity, location, &newStorage](const ComponentId componentId)
            {
                std::byte* componentData = TryGetComponent(entity, componentId);
                assert(componentData);
                newStorage.SetComponentData(componentId, location.chunkIndex, location.columnIndex, componentData);
            });

            newStorage.SetComponentData<ComponentType>(location.chunkIndex, location.columnIndex, component);

            if (const std::optional<EntityId> movedEntity = storage->RemoveEntity(oldLocation))
            {
                m_entityInfo.Get(movedEntity->GetIndex()).location = oldLocation;
            }

            // Finally, update entity location to point to new archetype
            m_entityInfo.Get(entity.GetIndex()).location = location;
        }
        else
        {
            storage->SetComponentData<ComponentType>(entityInfo->location.chunkIndex, entityInfo->location.columnIndex, component);
        }
    }
}

#endif // ARCHON_ENTITYMANAGER_H
