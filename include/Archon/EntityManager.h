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
        IndexType m_nextEntityIndex = EntityId::FirstIndex;
        IndexType m_freeListHead = 0;

        ArchetypeStorage* GetOrCreateArchetypeStorage(ArchetypeId id);

    public:
        ArchetypeStorage& GetArchetype(ArchetypeId id);
        ArchetypeStorage* TryGetArchetype(ArchetypeId id);
        ArchetypeId CreateArchetype(const ComponentMask& componentMask);

        template<typename... ComponentTypes>
        ArchetypeId CreateArchetype();

        EntityId CreateEntity(ArchetypeId archetypeId);

        template<typename... ComponentTypes>
        EntityId CreateEntity();

        void DestroyEntity(EntityId entity);
        [[nodiscard]] bool IsValid(EntityId entity) const;
    };

    template<typename... ComponentTypes>
    ArchetypeId EntityManager::CreateArchetype()
    {
        return CreateArchetype(ComponentMask::Create<ComponentTypes...>());
    }

    template<typename... ComponentTypes>
    EntityId EntityManager::CreateEntity()
    {
        return CreateEntity(ArchetypeRegistry::GetId<ComponentTypes...>());
    }
}

#endif // ARCHON_ENTITYMANAGER_H
