#ifndef ARCHON_ARCHETYPESTORAGE_H
#define ARCHON_ARCHETYPESTORAGE_H

#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <Archon/Archetype.h>

namespace Archon
{
    class EntityId;
    class EntityManager;

    class ArchetypeStorage
    {
    private:
        ArchetypeId m_id = InvalidArchetypeId;
        const ArchetypeInfo* m_info = nullptr;
        std::vector<Chunk> m_chunks;
        std::vector<ChunkIndex> m_nonFullChunks;

        void AllocateChunk();
        explicit ArchetypeStorage(ArchetypeId id);

        friend class EntityManager;

    public:
        ~ArchetypeStorage();

        ArchetypeStorage(const ArchetypeStorage&) = delete;
        ArchetypeStorage& operator=(const ArchetypeStorage&) = delete;
        ArchetypeStorage(ArchetypeStorage&&) = delete;
        ArchetypeStorage& operator=(ArchetypeStorage&&) = delete;

        [[nodiscard]] ArchetypeId GetId() const;
        [[nodiscard]] const ArchetypeInfo& GetInfo() const;
        [[nodiscard]] size_t GetNumChunks() const;
        [[nodiscard]] size_t GetChunkEntityCount(ChunkIndex chunkIndex) const;
        [[nodiscard]] std::span<const EntityId> GetEntities(ChunkIndex chunkIndex) const;

        EntityLocation AddEntity(EntityId entity);
        std::optional<EntityId> RemoveEntity(const EntityLocation& location);

        template <Component ComponentType>
        void SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentType& component);

        template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
        void SetComponent(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components);

        template <Component ComponentType>
        ComponentType& GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex);

        template <ComponentAccess ComponentType>
        const ComponentType& GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        template <Component ComponentType>
        ComponentType* TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex);

        template <ComponentAccess ComponentType>
        const ComponentType* TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        template <ComponentAccess ComponentType>
        [[nodiscard]] std::span<ComponentType> GetComponentSpan(ChunkIndex chunkIndex);

        template <ComponentAccess ComponentType>
        [[nodiscard]] std::optional<std::span<ComponentType>> TryGetComponentSpan(ChunkIndex chunkIndex);

        [[nodiscard]] std::byte* TryGetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex);
        [[nodiscard]] const std::byte* TryGetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex) const;
        [[nodiscard]] std::byte& GetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex);
        [[nodiscard]] const std::byte& GetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        void SetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex, const std::byte* component) const;
    };

    template <Component ComponentType>
    void ArchetypeStorage::SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentType& component)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();
        SetComponentData(componentId, chunkIndex, columnIndex, reinterpret_cast<const std::byte*>(&component));
    }

    template <Component... ComponentTypes> requires UniqueTypes<ComponentTypes...>
    void ArchetypeStorage::SetComponent(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components)
    {
        (SetComponentData(chunkIndex, columnIndex, components), ...);
    }

    template <ComponentAccess ComponentType>
    const ComponentType& ArchetypeStorage::GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<const ComponentType&>(GetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <Component ComponentType>
    ComponentType* ArchetypeStorage::TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<ComponentType*>(TryGetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <ComponentAccess ComponentType>
    const ComponentType* ArchetypeStorage::TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<const ComponentType*>(TryGetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <Component ComponentType>
    ComponentType& ArchetypeStorage::GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<ComponentType&>(GetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <ComponentAccess ComponentType>
    std::span<ComponentType> ArchetypeStorage::GetComponentSpan(const ChunkIndex chunkIndex)
    {
        const auto result = TryGetComponentSpan<ComponentType>(chunkIndex);
        assert(result.has_value() && "Component is not present in this archetype or the chunk is invalid");
        return *result;
    }

    template <ComponentAccess ComponentType>
    std::optional<std::span<ComponentType>> ArchetypeStorage::TryGetComponentSpan(const ChunkIndex chunkIndex)
    {
        if (chunkIndex >= m_chunks.size())
        {
            return std::nullopt;
        }

        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();
        const ComponentPoolInfo* pool = nullptr;
        for (const ComponentPoolInfo& candidate : m_info->chunkInfo.componentPools)
        {
            if (candidate.id == componentId)
            {
                pool = &candidate;
                break;
            }
        }

        if (pool == nullptr)
        {
            return std::nullopt;
        }

        ComponentType* first = reinterpret_cast<ComponentType*>(m_chunks[chunkIndex].data + pool->offset);
        return std::span<ComponentType>(first, GetChunkEntityCount(chunkIndex));
    }
}

#endif // ARCHON_ARCHETYPESTORAGE_H
