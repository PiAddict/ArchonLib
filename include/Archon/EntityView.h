#ifndef ARCHON_ENTITYVIEW_H
#define ARCHON_ENTITYVIEW_H

#include <type_traits>

#include <Archon/EntityId.h>

namespace Archon
{
    class EntityManager;

    template <typename... ComponentTypes>
    class EntityView : protected EntityId
    {
    private:
        EntityManager& m_manager;

    public:
        EntityView(const EntityView& other)
            : EntityId(other), m_manager(other.m_manager)
        {
        }

        explicit EntityView(EntityManager& manager) : m_manager(manager)
        {
        }

        template <typename ComponentType>
            requires(std::is_same_v<ComponentType, ComponentTypes> || ...)
        ComponentType& Get() const;

        template <typename ComponentType>
        ComponentType* TryGet() const;
    };

    template <typename... ComponentTypes>
    template <typename ComponentType>
        requires (std::is_same_v<ComponentType, ComponentTypes> || ...)
    ComponentType& EntityView<ComponentTypes...>::Get() const
    {
        // TODO: Hook up to EntityManager.
        return nullptr;
    }

    template <typename ... ComponentTypes>
    template <typename ComponentType>
    ComponentType* EntityView<ComponentTypes...>::TryGet() const
    {
        // TODO: Hook up to EntityManager.
        return nullptr;
    }
}

#endif // ARCHON_ENTITYVIEW_H
