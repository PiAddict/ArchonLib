#ifndef ARCHON_CHUNK_H
#define ARCHON_CHUNK_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <Archon/ComponentRegistry.h>

namespace Archon
{
    struct ComponentPoolInfo
    {
        ComponentId id;
        size_t offset;
        size_t stride;
        size_t alignment;
    };

    struct ChunkInfo
    {
        size_t capacity;
        size_t allocatedSize;
        size_t allocationAlignment;

        std::vector<ComponentPoolInfo> componentPools;
    };

    struct Chunk
    {
        uint16_t m_columnCount{};
        std::byte* data{};
    };
}

#endif // ARCHON_CHUNK_H
