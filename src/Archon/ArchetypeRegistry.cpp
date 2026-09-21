#include <Archon/ArchetypeRegistry.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

#include "Math.h"
#include "Archon/EntityId.h"

namespace Archon
{
    std::vector<std::unique_ptr<ArchetypeInfo>> ArchetypeRegistry::m_archetypeInfo;
    std::unordered_map<ComponentMask, ArchetypeId> ArchetypeRegistry::m_archetypeIds;

    namespace
    {
        size_t GetAllocationAlignment(const ComponentMask& componentMask)
        {
            size_t alignment = alignof(std::max_align_t);

            componentMask.ForEachComponentId([&alignment](ComponentId componentId)
            {
                alignment = std::max(alignment, ComponentRegistry::GetComponentInfo(componentId).alignment);
            });

            return alignment;
        }

        size_t GetRequiredAllocationSize(const ComponentMask& componentMask, size_t columnCount, size_t allocationAlignment)
        {
            size_t offset = sizeof(EntityId) * columnCount;

            componentMask.ForEachComponentId([&offset, columnCount](ComponentId componentId)
            {
                const ComponentInfo& info = ComponentRegistry::GetComponentInfo(componentId);
                offset = Core::AlignUp(offset, info.alignment);
                offset += info.size * columnCount;
            });

            return Core::AlignUp(offset, allocationAlignment);
        }

        ChunkInfo CreateChunkInfo(const ComponentMask& componentMask)
        {
            constexpr size_t kChunkSize = 1024;
            const size_t allocationAlignment = GetAllocationAlignment(componentMask);

            size_t sizePerColumn = sizeof(EntityId);
            componentMask.ForEachComponentId([&sizePerColumn](ComponentId componentId)
            {
                sizePerColumn += ComponentRegistry::GetComponentInfo(componentId).size;
            });

            size_t columnCount = kChunkSize / sizePerColumn;

            while (columnCount > 0 && GetRequiredAllocationSize(componentMask, columnCount, allocationAlignment) > kChunkSize)
            {
                --columnCount;
            }

            while (GetRequiredAllocationSize(componentMask, columnCount + 1, allocationAlignment) <= kChunkSize)
            {
                ++columnCount;
            }

            assert(columnCount > 0);

            ChunkInfo chunkInfo{};
            chunkInfo.capacity = columnCount;
            chunkInfo.allocationAlignment = allocationAlignment;

            size_t offset = sizeof(EntityId) * columnCount;
            componentMask.ForEachComponentId([&chunkInfo, &offset, columnCount](ComponentId componentId)
            {
                const ComponentInfo& info = ComponentRegistry::GetComponentInfo(componentId);
                offset = Core::AlignUp(offset, info.alignment);
                chunkInfo.componentPools.emplace_back(componentId, offset, info.size, info.alignment);
                offset += info.size * columnCount;
            });

            chunkInfo.allocatedSize = Core::AlignUp(offset, chunkInfo.allocationAlignment);
            return chunkInfo;
        }
    }

    ArchetypeId ArchetypeRegistry::Register(const ComponentMask& componentMask)
    {
        if (const auto found = m_archetypeIds.find(componentMask); found != m_archetypeIds.end())
        {
            return found->second;
        }

        if (m_archetypeInfo.size() >= InvalidArchetypeId)
        {
            return InvalidArchetypeId;
        }

        const auto id = static_cast<ArchetypeId>(m_archetypeInfo.size());
        auto info = std::make_unique<ArchetypeInfo>();
        info->id = id;
        info->componentMask = componentMask;
        info->chunkInfo = CreateChunkInfo(componentMask);

        m_archetypeInfo.emplace_back(std::move(info));
        m_archetypeIds.emplace(componentMask, id);
        return id;
    }

    const ArchetypeInfo& ArchetypeRegistry::GetInfo(ArchetypeId id)
    {
        const ArchetypeInfo* info = TryGetInfo(id);
        assert(info != nullptr);
        return *info;
    }

    const ArchetypeInfo* ArchetypeRegistry::TryGetInfo(ArchetypeId id)
    {
        if (id >= m_archetypeInfo.size())
        {
            return nullptr;
        }

        return m_archetypeInfo[id].get();
    }
}
