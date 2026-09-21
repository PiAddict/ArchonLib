#ifndef ARCHON_ENTITYVIEW_H
#define ARCHON_ENTITYVIEW_H

#include <cassert>
#include <tuple>

#include <Archon/ArchetypeStorage.h>
#include <Archon/Component.h>
#include <Archon/EntityId.h>

namespace Archon
{
    class EntityManager;

    namespace Core
    {
        struct EntityViewState
        {
            EntityManager* manager = nullptr;
            EntityId entity;
            ArchetypeStorage* storage = nullptr;
            EntityLocation location{};

            EntityViewState(EntityManager& manager, EntityId entity);

            [[nodiscard]] bool IsValid() const;

            template <ComponentAccess ComponentType>
            [[nodiscard]] ComponentType* ResolveComponent() const
            {
                if (storage == nullptr)
                {
                    return nullptr;
                }

                return storage->TryGetComponentData<ComponentType>(location.chunkIndex, location.columnIndex);
            }
        };
    }

    // Entity views are transient and must not be retained across calls that may perform structural changes.
    // IsValid detects destruction, relocation, swap-removal, and missing requested components at the captured location.
    template <ComponentAccess... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    class EntityView
    {
        template <ComponentAccess... OtherComponentTypes> requires UniqueTypes<OtherComponentTypes...>
        friend class EntityView;

        Core::EntityViewState m_state;
        std::tuple<ComponentTypes*...> m_components;

        template <ComponentAccess ComponentType, ComponentAccess... SourceComponentTypes>
        static ComponentType* GetSourceComponent(const EntityView<SourceComponentTypes...>& source)
        {
            if constexpr (ContainsType<ComponentType, SourceComponentTypes...>)
            {
                return std::get<ComponentType*>(source.m_components);
            }
            else
            {
                static_assert(std::is_const_v<ComponentType>);
                return std::get<std::remove_const_t<ComponentType>*>(source.m_components);
            }
        }

    public:
        EntityView(EntityManager& manager, EntityId entity) : m_state(manager, entity), m_components(m_state.ResolveComponent<ComponentTypes>()...)
        {
            if (m_state.storage == nullptr)
            {
                return;
            }

            const ComponentMask& componentMask = m_state.storage->GetInfo().componentMask;
            const bool hasAllComponents = (componentMask.Test<ComponentTypes>() && ...);
            if (!hasAllComponents)
            {
                return;
            }

            assert(((std::get<ComponentTypes*>(m_components) != nullptr) && ...) && "Failed to prefetch a component declared by the EntityView");
        }

        EntityView(const EntityView&) = default;
        EntityView& operator=(const EntityView&) = default;

        template <ComponentAccess... SourceComponentTypes>
            requires UniqueTypes<SourceComponentTypes...> &&
                ((ContainsType<ComponentTypes, SourceComponentTypes...> ||
                    (std::is_const_v<ComponentTypes> && ContainsType<std::remove_const_t<ComponentTypes>, SourceComponentTypes...>)) && ...)
        EntityView(const EntityView<SourceComponentTypes...>& source)
            : m_state(source.m_state), m_components(GetSourceComponent<ComponentTypes>(source)...)
        {
        }

        [[nodiscard]] EntityId GetEntityId() const noexcept
        {
            return m_state.entity;
        }

        operator EntityId() const noexcept
        {
            return m_state.entity;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_state.IsValid() && ((std::get<ComponentTypes*>(m_components) != nullptr) && ...);
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        template <ComponentAccess ComponentType> requires ContainsType<ComponentType, ComponentTypes...>
        [[nodiscard]] ComponentType& Get() const
        {
            assert(IsValid() && "Cannot access an invalid or stale EntityView");
            ComponentType* component = std::get<ComponentType*>(m_components);
            assert(component != nullptr && "EntityView component cache is invalid");
            return *component;
        }

        template <ComponentAccess ComponentType> requires ContainsType<ComponentType, ComponentTypes...>
        [[nodiscard]] ComponentType* TryGet() const
        {
            return IsValid() ? std::get<ComponentType*>(m_components) : nullptr;
        }
    };

    EntityView(EntityManager&, EntityId) -> EntityView<>;
}

#endif // ARCHON_ENTITYVIEW_H
