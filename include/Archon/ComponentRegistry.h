#ifndef ARCHON_COMPONENTREGISTRY_H
#define ARCHON_COMPONENTREGISTRY_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <Archon/Component.h>

namespace Archon
{
    using ComponentId = uint16_t;
    constexpr ComponentId InvalidComponentId = std::numeric_limits<ComponentId>::max();

    struct ComponentInfo
    {
        ComponentId m_id = InvalidComponentId;
        size_t size = 0;
        size_t alignment = 0;
        void (*construct)(void*) = nullptr;
    };

    class ComponentRegistry
    {
        static std::unordered_map<std::type_index, ComponentId> m_componentIdMap;
        static std::vector<ComponentInfo> m_componentInfo;

        template <Component ComponentType>
        static void ConstructComponent(void* address)
        {
            std::construct_at(static_cast<ComponentType*>(address), ComponentType{});
        }

    public:
        template<Component ComponentType>
        static ComponentId Register();

        template<ComponentAccess ComponentType>
        [[nodiscard]] static ComponentId GetId();

        [[nodiscard]] static const ComponentInfo* TryGetComponentInfo(ComponentId componentId);
        [[nodiscard]] static const ComponentInfo& GetComponentInfo(ComponentId componentId);

        template<ComponentAccess ComponentType>
        [[nodiscard]] static const ComponentInfo& GetComponentInfo();
    };

    template <Component ComponentType>
    ComponentId ComponentRegistry::Register()
    {
        const std::type_index index = typeid(ComponentType);
        if (const auto found = m_componentIdMap.find(index); found != m_componentIdMap.end())
        {
            return found->second;
        }

        if (m_componentInfo.size() >= InvalidComponentId)
        {
            return InvalidComponentId;
        }

        const auto id = static_cast<ComponentId>(m_componentInfo.size());
        m_componentIdMap.emplace(index, id);
        m_componentInfo.emplace_back(
            id,
            sizeof(ComponentType),
            alignof(ComponentType),
            &ConstructComponent<ComponentType>);
        return id;
    }

    template <ComponentAccess ComponentType>
    ComponentId ComponentRegistry::GetId()
    {
        using BaseType = std::remove_const_t<ComponentType>;
        static auto id = Register<BaseType>();
        return id;
    }

    template <ComponentAccess ComponentType>
    const ComponentInfo& ComponentRegistry::GetComponentInfo()
    {
        return GetComponentInfo(GetId<ComponentType>());
    }
}

#endif // ARCHON_COMPONENTREGISTRY_H
