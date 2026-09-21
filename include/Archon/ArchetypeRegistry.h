#ifndef ARCHON_ARCHETYPEREGISTRY_H
#define ARCHON_ARCHETYPEREGISTRY_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <Archon/Archetype.h>

namespace Archon
{
    class ArchetypeRegistry
    {
        static std::vector<std::unique_ptr<ArchetypeInfo>> m_archetypeInfo;
        static std::unordered_map<ComponentMask, ArchetypeId> m_archetypeIds;

        static ArchetypeId Register(const ComponentMask& componentMask);

    public:
        [[nodiscard]] static const ArchetypeInfo& GetInfo(ArchetypeId id);
        [[nodiscard]] static const ArchetypeInfo* TryGetInfo(ArchetypeId id);

        template<typename... ComponentTypes>
        [[nodiscard]] static ArchetypeId GetId();

        [[nodiscard]] static ArchetypeId GetId(const ComponentMask& mask);
    };

    template<typename... ComponentTypes>
    ArchetypeId ArchetypeRegistry::GetId()
    {
        return Register(ComponentMask::Create<ComponentTypes...>());
    }
}

#endif // ARCHON_ARCHETYPEREGISTRY_H
