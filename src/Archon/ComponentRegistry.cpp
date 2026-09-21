#include <Archon/ComponentRegistry.h>

#include <cassert>

namespace Archon
{
    std::unordered_map<std::type_index, ComponentId> ComponentRegistry::m_componentIdMap;
    std::vector<ComponentInfo> ComponentRegistry::m_componentInfo;

    const ComponentInfo* ComponentRegistry::TryGetComponentInfo(ComponentId componentId)
    {
        return componentId < m_componentInfo.size() ? &m_componentInfo[componentId] : nullptr;
    }

    const ComponentInfo& ComponentRegistry::GetComponentInfo(ComponentId componentId)
    {
        const ComponentInfo* info = TryGetComponentInfo(componentId);
        assert(info != nullptr);
        return *info;
    }
}
