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
        void SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components);
    };

    template <typename ComponentType>
    void ArchetypeStorage::SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex,
                                            const ComponentType& component)
    {
        assert(m_chunks.size() > chunkIndex);
        assert(m_chunks[chunkIndex].m_columnCount > columnIndex);

        const ComponentId componentId = ComponentRegistry::GetId<ComponentType>();

        for (const ComponentPoolInfo& componentPool : m_info->chunkInfo.componentPools)
        {
            if (componentPool.id == componentId)
            {
                std::byte* address = m_chunks[chunkIndex].data + componentPool.offset + columnIndex * componentPool.stride;
                auto* destination = reinterpret_cast<ComponentType*>(address);
                *destination = component;
                return;
            }
        }

        assert(false && "Component is not present in this archetype");
    }

    template <typename... ComponentTypes>
    void ArchetypeStorage::SetComponentData(ChunkIndex chunkIndex, ColumnIndex columnIndex, const ComponentTypes&... components)
    {
        (SetComponentData(chunkIndex, columnIndex, components), ...);
    }
}

#endif // ARCHON_ARCHETYPESTORAGE_H
