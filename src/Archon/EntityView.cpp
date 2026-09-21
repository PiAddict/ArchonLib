#include <Archon/EntityView.h>

#include <cassert>

#include <Archon/EntityManager.h>

namespace Archon::Core
{
    EntityViewState::EntityViewState(EntityManager& manager, const EntityId entity) : manager(&manager), entity(entity)
    {
        const EntityInfo* info = manager.TryGetEntityInfo(entity);
        if (info == nullptr)
        {
            return;
        }

        location = info->location;
        storage = manager.TryGetArchetype(location.archetypeId);
        assert(storage != nullptr && "Entity references missing archetype storage");
    }

    bool EntityViewState::IsValid() const
    {
        if (manager == nullptr)
        {
            return false;
        }

        const EntityInfo* info = manager->TryGetEntityInfo(entity);
        return info != nullptr && storage != nullptr &&
            info->location.archetypeId == location.archetypeId &&
            info->location.chunkIndex == location.chunkIndex &&
            info->location.columnIndex == location.columnIndex &&
            manager->TryGetArchetype(location.archetypeId) == storage;
    }
}
