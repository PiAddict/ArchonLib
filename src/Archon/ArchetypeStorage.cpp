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
        const size_t alignment = m_info->chunkInfo.allocationAlignment;
        auto* allocation = static_cast<std::byte*>(::operator new(
            m_info->chunkInfo.allocatedSize,
            std::align_val_t{alignment}));

        const auto chunkIndex = static_cast<ChunkIndex>(m_chunks.size());
        m_chunks.emplace_back(0, allocation);
        m_nonFullChunks.push_back(chunkIndex);
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
        if (m_nonFullChunks.empty())
        {
            AllocateChunk();
        }

        const ChunkIndex chunkIndex = m_nonFullChunks.back();
        Chunk& chunk = m_chunks[chunkIndex];
        assert(chunk.m_columnCount < m_info->chunkInfo.capacity);

        const ColumnIndex columnIndex = chunk.m_columnCount;
        auto* entities = reinterpret_cast<EntityId*>(chunk.data);
        std::construct_at(entities + columnIndex, entity);

        for (const ComponentPoolInfo& componentPool : m_info->chunkInfo.componentPools)
        {
            std::byte* address = chunk.data + componentPool.offset + columnIndex * componentPool.stride;
            const ComponentInfo& componentInfo = ComponentRegistry::GetComponentInfo(componentPool.id);
            assert(componentInfo.construct != nullptr);
            componentInfo.construct(address);
        }

        ++chunk.m_columnCount;

        if (chunk.m_columnCount == m_info->chunkInfo.capacity)
        {
            m_nonFullChunks.pop_back();
        }

        return {m_id, chunkIndex, columnIndex};
    }

    std::optional<EntityId> ArchetypeStorage::RemoveEntity(const EntityLocation& location)
    {
        assert(location.archetypeId == m_id);
        assert(location.chunkIndex < m_chunks.size());

        Chunk& chunk = m_chunks[location.chunkIndex];
        assert(location.columnIndex < chunk.m_columnCount);

        const bool wasFull = chunk.m_columnCount == m_info->chunkInfo.capacity;
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

        if (wasFull)
        {
            m_nonFullChunks.push_back(location.chunkIndex);
        }

        return movedEntity;
    }

    const std::byte* ArchetypeStorage::TryGetComponentData(
        ComponentId componentId,
        ChunkIndex chunkIndex,
        ColumnIndex columnIndex) const
    {
        if (m_chunks.size() <= chunkIndex)
        {
            return nullptr;
        }

        if (m_chunks[chunkIndex].m_columnCount <= columnIndex)
        {
            return nullptr;
        }

        for (const ComponentPoolInfo& componentPool : m_info->chunkInfo.componentPools)
        {
            if (componentPool.id == componentId)
            {
                return m_chunks[chunkIndex].data + componentPool.offset + columnIndex * componentPool.stride;
            }
        }

        return nullptr;
    }

    std::byte* ArchetypeStorage::TryGetComponentData(
        ComponentId componentId,
        ChunkIndex chunkIndex,
        ColumnIndex columnIndex)
    {
        return const_cast<std::byte*>(
            static_cast<const ArchetypeStorage&>(*this).TryGetComponentData(
                componentId,
                chunkIndex,
                columnIndex));
    }

    const std::byte& ArchetypeStorage::GetComponentData(
        ComponentId componentId,
        ChunkIndex chunkIndex,
        ColumnIndex columnIndex) const
    {
        const std::byte* component = TryGetComponentData(componentId, chunkIndex, columnIndex);
        assert(component != nullptr && "Component is not present in this archetype or the location is invalid");
        return *component;
    }

    std::byte& ArchetypeStorage::GetComponentData(
        ComponentId componentId,
        ChunkIndex chunkIndex,
        ColumnIndex columnIndex)
    {
        return const_cast<std::byte&>(
            static_cast<const ArchetypeStorage&>(*this).GetComponentData(
                componentId,
                chunkIndex,
                columnIndex));
    }

    void ArchetypeStorage::SetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex, const std::byte* component) const
    {
        assert(m_chunks.size() > chunkIndex);
        assert(m_chunks[chunkIndex].m_columnCount > columnIndex);
        assert(component != nullptr);

        for (const ComponentPoolInfo& componentPool : m_info->chunkInfo.componentPools)
        {
            if (componentPool.id == componentId)
            {
                std::byte* address = m_chunks[chunkIndex].data + componentPool.offset + columnIndex * componentPool.stride;
                const size_t componentSize = ComponentRegistry::GetComponentInfo(componentId).size;
                std::memmove(address, component, componentSize);
                return;
            }
        }

        assert(false && "Component is not present in this archetype");
    }
}
