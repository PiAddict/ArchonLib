#ifndef ARCHON_ENTITYMANAGER_H
#define ARCHON_ENTITYMANAGER_H

#include <array>
#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>

#include <Archon/ArchetypeRegistry.h>
#include <Archon/ArchetypeStorage.h>
#include <Archon/EntityInfo.h>
#include <Archon/EntityView.h>

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

        friend struct Core::EntityViewState;

        ArchetypeStorage* InitializeArchetypeStorage(ArchetypeId id);
        [[nodiscard]] ArchetypeStorage* FindArchetypeStorage(ArchetypeId id) const;

        [[nodiscard]] EntityInfo* TryGetEntityInfo(EntityId entity);
        [[nodiscard]] const EntityInfo* TryGetEntityInfo(EntityId entity) const;

        bool TrySetComponent(EntityId entity, ComponentId componentId, const std::byte* component);
        bool TryAddComponent(EntityId entity, ComponentId componentId, const std::byte* component);
        bool TryRemoveComponent(EntityId entity, ComponentId componentId);
        bool MigrateEntity(EntityId entity, EntityInfo& entityInfo, const ComponentMask& newComponentMask,
            ComponentId addedComponentId = InvalidComponentId, const std::byte* addedComponent = nullptr);

    public:
        ArchetypeStorage& GetArchetypeStorage(ArchetypeId id);
        ArchetypeStorage* TryGetArchetype(ArchetypeId id);

        [[nodiscard]] const ArchetypeStorage& GetArchetypeStorage(ArchetypeId id) const;
        [[nodiscard]] const ArchetypeStorage* TryGetArchetype(ArchetypeId id) const;

        ArchetypeId CreateArchetype(const ComponentMask& componentMask);

        template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
        ArchetypeId CreateArchetype();

        EntityId CreateEntity(ArchetypeId archetypeId);

        template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
        EntityView<ComponentTypes...> CreateEntity();

        template <ComponentAccess... ComponentTypes> requires UniqueTypes<ComponentTypes...>
        [[nodiscard]] std::optional<EntityView<ComponentTypes...>> TryView(EntityId entity);

        template <ComponentAccess ComponentType>
        ComponentType* TryGetComponent(EntityId entity);

        template <ComponentAccess ComponentType>
        const ComponentType* TryGetComponent(EntityId entity) const;

        [[nodiscard]] std::byte* TryGetComponent(EntityId entity, ComponentId componentId);
        [[nodiscard]] const std::byte* TryGetComponent(EntityId entity, ComponentId componentId) const;

        template <Component ComponentType>
        void SetComponent(EntityId entity, const ComponentType& component);

        template <Component ComponentType>
        [[nodiscard]] bool TrySetComponent(EntityId entity, const ComponentType& component);

        template <Component ComponentType>
        void AddComponent(EntityId entity, const ComponentType& component);

        template <Component ComponentType>
        [[nodiscard]] bool TryAddComponent(EntityId entity, const ComponentType& component);

        template <Component ComponentType>
        void RemoveComponent(EntityId entity);

        template <Component ComponentType>
        [[nodiscard]] bool TryRemoveComponent(EntityId entity);

        template <ComponentAccess ComponentType>
        [[nodiscard]] bool HasComponent(EntityId entity) const;

        template <ComponentAccess... ComponentTypes> requires UniqueTypes<ComponentTypes...>
        [[nodiscard]] bool HasComponents(EntityId entity) const;

        [[nodiscard]] const ComponentMask* TryGetComponentMask(EntityId entity) const;

        void DestroyEntity(EntityId entity);
        [[nodiscard]] bool IsValid(EntityId entity) const;
    };

    template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    ArchetypeId EntityManager::CreateArchetype()
    {
        return CreateArchetype(ComponentMask::Create<ComponentTypes...>());
    }

    template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    EntityView<ComponentTypes...> EntityManager::CreateEntity()
    {
        const EntityId entity = CreateEntity(ArchetypeRegistry::GetId<ComponentTypes...>());
        assert(entity != EntityId::Null && "Failed to create entity");
        return EntityView<ComponentTypes...>(*this, entity);
    }

    template <ComponentAccess... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    std::optional<EntityView<ComponentTypes...>> EntityManager::TryView(const EntityId entity)
    {
        if (!HasComponents<ComponentTypes...>(entity))
        {
            return std::nullopt;
        }

        return std::optional<EntityView<ComponentTypes...>>{std::in_place, *this, entity};
    }

    template <ComponentAccess ComponentType>
    ComponentType* EntityManager::TryGetComponent(const EntityId entity)
    {
        return reinterpret_cast<ComponentType*>(TryGetComponent(entity, ComponentRegistry::GetId<ComponentType>()));
    }

    template <ComponentAccess ComponentType>
    const ComponentType* EntityManager::TryGetComponent(const EntityId entity) const
    {
        return reinterpret_cast<const ComponentType*>(TryGetComponent(entity, ComponentRegistry::GetId<ComponentType>()));
    }

    template <Component ComponentType>
    void EntityManager::SetComponent(const EntityId entity, const ComponentType& component)
    {
        if (!IsValid(entity))
        {
            assert(false && "Cannot set a component on an invalid entity");
            return;
        }

        if (!HasComponent<ComponentType>(entity))
        {
            assert(false && "SetComponent requires the component to already exist");
            return;
        }

        const bool succeeded = TrySetComponent(entity, component);
        assert(succeeded && "Failed to set an existing component");
        (void)succeeded;
    }

    template <Component ComponentType>
    bool EntityManager::TrySetComponent(const EntityId entity, const ComponentType& component)
    {
        return TrySetComponent(entity, ComponentRegistry::GetId<ComponentType>(), reinterpret_cast<const std::byte*>(&component));
    }

    template <Component ComponentType>
    void EntityManager::AddComponent(const EntityId entity, const ComponentType& component)
    {
        if (!IsValid(entity))
        {
            assert(false && "Cannot add a component to an invalid entity");
            return;
        }

        if (HasComponent<ComponentType>(entity))
        {
            assert(false && "AddComponent requires the component to be absent");
            return;
        }

        const bool succeeded = TryAddComponent(entity, component);
        assert(succeeded && "Failed to add component during archetype migration");
        (void)succeeded;
    }

    template <Component ComponentType>
    bool EntityManager::TryAddComponent(const EntityId entity, const ComponentType& component)
    {
        return TryAddComponent(entity, ComponentRegistry::GetId<ComponentType>(), reinterpret_cast<const std::byte*>(&component));
    }

    template <Component ComponentType>
    void EntityManager::RemoveComponent(const EntityId entity)
    {
        if (!IsValid(entity))
        {
            assert(false && "Cannot remove a component from an invalid entity");
            return;
        }

        if (!HasComponent<ComponentType>(entity))
        {
            assert(false && "RemoveComponent requires the component to exist");
            return;
        }

        const bool succeeded = TryRemoveComponent<ComponentType>(entity);
        assert(succeeded && "Failed to remove component during archetype migration");
        (void)succeeded;
    }

    template <Component ComponentType>
    bool EntityManager::TryRemoveComponent(const EntityId entity)
    {
        return TryRemoveComponent(entity, ComponentRegistry::GetId<ComponentType>());
    }

    template <ComponentAccess ComponentType>
    bool EntityManager::HasComponent(const EntityId entity) const
    {
        const ComponentMask* componentMask = TryGetComponentMask(entity);
        return componentMask != nullptr && componentMask->Test<ComponentType>();
    }

    template <ComponentAccess... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    bool EntityManager::HasComponents(const EntityId entity) const
    {
        const ComponentMask* componentMask = TryGetComponentMask(entity);
        return componentMask != nullptr && (componentMask->Test<ComponentTypes>() && ...);
    }
}

#endif // ARCHON_ENTITYMANAGER_H
