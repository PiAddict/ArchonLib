#ifndef ARCHON_ARCHETYPE_H
#define ARCHON_ARCHETYPE_H

#include <cstdint>
#include <limits>

#include <Archon/Chunk.h>
#include <Archon/ComponentMask.h>

namespace Archon
{
    using ArchetypeId = uint32_t;
    constexpr ArchetypeId InvalidArchetypeId = std::numeric_limits<ArchetypeId>::max();

    using ChunkIndex = uint32_t;
    using ColumnIndex = uint16_t;

    struct ArchetypeInfo
    {
        ArchetypeId id = InvalidArchetypeId;
        ComponentMask componentMask;
        ChunkInfo chunkInfo{};
    };

    struct EntityLocation
    {
        ArchetypeId archetypeId = InvalidArchetypeId;
        ChunkIndex chunkIndex = 0;
        ColumnIndex columnIndex = 0;

        [[nodiscard]] bool IsValid() const
        {
            return archetypeId != InvalidArchetypeId;
        }
    };
}

#endif // ARCHON_ARCHETYPE_H
