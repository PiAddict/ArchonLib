#include <Archon/ArchetypeStorage.h>

#include <cstring>
#include <memory>
#include <new>

#include <Archon/ArchetypeRegistry.h>

#include "Archon/EntityId.h"

namespace Archon
{
    ArchetypeStorage::ArchetypeStorage(ArchetypeId id)
        : m_id(id),
          m_info(&ArchetypeRegistry::GetInfo(id))
    {
    }

    void ArchetypeStorage::AllocateChunk()
    {
        auto* allocation = static_cast<std::byte*>(
            ::operator new(
                m_info->chunkInfo.allocatedSize,
                std::align_val_t{m_info->chunkInfo.allocationAlignment}));

        m_chunks.emplace_back(0, allocation);
    }

    ArchetypeStorage::~ArchetypeStorage()
    {
        for (const Chunk& chunk : m_chunks)
        {
            ::operator delete(
                chunk.data,
                std::align_val_t{m_info->chunkInfo.allocationAlignment});
        }
    }

    ArchetypeId ArchetypeStorage::GetId() const
    {
        return m_id;
    }

    const ArchetypeInfo& ArchetypeStorage::GetInfo() const
    {
        return *m_info;
    }

    size_t ArchetypeStorage::GetNumChunks() const
    {
        return m_chunks.size();
    }

    EntityLocation ArchetypeStorage::AddEntity(EntityId entity)
    {
        for (ChunkIndex chunkIndex = 0; chunkIndex < m_chunks.size(); ++chunkIndex)
        {
            Chunk& chunk = m_chunks[chunkIndex];
            if (chunk.m_columnCount < m_info->chunkInfo.capacity)
            {
                const ColumnIndex columnIndex = chunk.m_columnCount;
                auto* entities = reinterpret_cast<EntityId*>(chunk.data);
                std::construct_at(entities + columnIndex, entity);
                ++chunk.m_columnCount;
                return {m_id, chunkIndex, columnIndex};
            }
        }

        AllocateChunk();
        auto* entities = reinterpret_cast<EntityId*>(m_chunks.back().data);
        std::construct_at(entities, entity);
        m_chunks.back().m_columnCount = 1;
        return {m_id, static_cast<ChunkIndex>(m_chunks.size() - 1), 0};
    }

    std::optional<EntityId> ArchetypeStorage::RemoveEntity(const EntityLocation& location)
    {
        assert(location.archetypeId == m_id);
        assert(location.chunkIndex < m_chunks.size());

        Chunk& chunk = m_chunks[location.chunkIndex];
        assert(location.columnIndex < chunk.m_columnCount);

        const ColumnIndex lastColumn = chunk.m_columnCount - 1;
        auto* entities = reinterpret_cast<EntityId*>(chunk.data);
        std::optional<EntityId> movedEntity;

        if (location.columnIndex != lastColumn)
        {
            movedEntity = entities[lastColumn];
            entities[location.columnIndex] = *movedEntity;

            for (const ComponentPoolInfo& componentPool : m_info->chunkInfo.componentPools)
            {
                std::byte* pool = chunk.data + componentPool.offset;
                std::memmove(
                    pool + location.columnIndex * componentPool.stride,
                    pool + lastColumn * componentPool.stride,
                    componentPool.stride);
            }
        }

        std::destroy_at(entities + lastColumn);
        --chunk.m_columnCount;
        return movedEntity;
    }
}
