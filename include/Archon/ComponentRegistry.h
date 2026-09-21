#ifndef ARCHON_COMPONENTREGISTRY_H
#define ARCHON_COMPONENTREGISTRY_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Archon
{
    using ComponentId = uint16_t;
    constexpr ComponentId InvalidComponentId = std::numeric_limits<ComponentId>::max();

    struct ComponentInfo
    {
        ComponentId m_id = InvalidComponentId;
        size_t size = 0;
        size_t alignment = 0;
    };

    class ComponentRegistry
    {
        static std::unordered_map<std::type_index, ComponentId> m_componentIdMap;
        static std::vector<ComponentInfo> m_componentInfo;

    public:
        template<typename ComponentType>
        static ComponentId Register();

        template<typename ComponentType>
        [[nodiscard]] static ComponentId GetId();

        [[nodiscard]] static const ComponentInfo* TryGetComponentInfo(ComponentId componentId);
        [[nodiscard]] static const ComponentInfo& GetComponentInfo(ComponentId componentId);

        template<typename ComponentType>
        [[nodiscard]] static const ComponentInfo& GetComponentInfo();
    };

    template <typename ComponentType>
    ComponentId ComponentRegistry::Register()
    {
        static_assert(std::is_standard_layout_v<ComponentType>, "ComponentType must be standard layout");
        static_assert(std::is_trivial_v<ComponentType>, "ComponentType must be trivial");

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
        m_componentInfo.emplace_back(id, sizeof(ComponentType), alignof(ComponentType));
        return id;
    }

    template <typename ComponentType>
    ComponentId ComponentRegistry::GetId()
    {
        using BaseType = std::remove_cvref_t<ComponentType>;
        static auto id = Register<BaseType>();
        return id;
    }

    template <typename ComponentType>
    const ComponentInfo& ComponentRegistry::GetComponentInfo()
    {
        return GetComponentInfo(GetId<ComponentType>());
    }
}

#endif // ARCHON_COMPONENTREGISTRY_H
