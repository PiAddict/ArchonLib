#ifndef ARCHON_ARCHETYPESTORAGE_H
#define ARCHON_ARCHETYPESTORAGE_H

#include <cassert>
#include <cstddef>
#include <optional>
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

        EntityLocation AddEntity(EntityId entity);
        std::optional<EntityId> RemoveEntity(const EntityLocation& location);

        template <typename ComponentType>
        void SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentType& component);

        template <typename... ComponentTypes>
        void SetComponent(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components);

        template <typename ComponentType>
        ComponentType& GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex);

        template <typename ComponentType>
        const ComponentType& GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        template <typename ComponentType>
        ComponentType* TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex);

        template <typename ComponentType>
        const ComponentType* TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        [[nodiscard]] std::byte* TryGetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex);
        [[nodiscard]] const std::byte* TryGetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex) const;
        [[nodiscard]] std::byte& GetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex);
        [[nodiscard]] const std::byte& GetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex) const;

        void SetComponentData(ComponentId componentId, ChunkIndex chunkIndex, ColumnIndex columnIndex, const std::byte* component) const;
    };

    template <typename ComponentType>
    void ArchetypeStorage::SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentType& component)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();
        SetComponentData(componentId, chunkIndex, columnIndex, reinterpret_cast<const std::byte*>(&component));
    }

    template <typename... ComponentTypes>
    void ArchetypeStorage::SetComponent(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components)
    {
        (SetComponentData(chunkIndex, columnIndex, components), ...);
    }

    template <typename ComponentType>
    const ComponentType& ArchetypeStorage::GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<const ComponentType&>(GetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <typename ComponentType>
    ComponentType* ArchetypeStorage::TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<ComponentType*>(TryGetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <typename ComponentType>
    const ComponentType* ArchetypeStorage::TryGetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex) const
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<const ComponentType*>(TryGetComponentData(componentId, chunkIndex, columnIndex));
    }

    template <typename ComponentType>
    ComponentType& ArchetypeStorage::GetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex)
    {
        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        return reinterpret_cast<ComponentType&>(GetComponentData(componentId, chunkIndex, columnIndex));
    }
}

#endif // ARCHON_ARCHETYPESTORAGE_H
