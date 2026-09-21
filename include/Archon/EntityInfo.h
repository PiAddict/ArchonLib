#ifndef ARCHON_ENTITYINFO_H
#define ARCHON_ENTITYINFO_H

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include <Archon/Archetype.h>
#include "Archon/EntityId.h"

namespace Archon
{
    struct EntityInfo
    {
        VersionType version = 0;
        EntityLocation location{};
        IndexType nextFreeIndex = 0;

        [[nodiscard]] bool IsValidFor(EntityId entity) const noexcept
        {
            return location.IsValid() && version == entity.GetVersion();
        }
    };

    class EntityInfoTable
    {
        static constexpr size_t EntityPageSize = 4096;

        struct Page
        {
            std::array<EntityInfo, EntityPageSize> entries{};
        };

        std::vector<std::unique_ptr<Page>> m_pages;

    public:
        EntityInfo& Get(IndexType index)
        {
            assert(index >= EntityId::FirstIndex);

            const size_t pageIndex = index / EntityPageSize;
            if (pageIndex >= m_pages.size())
            {
                m_pages.resize(pageIndex + 1);
            }

            if (!m_pages[pageIndex])
            {
                m_pages[pageIndex] = std::make_unique<Page>();
            }

            return m_pages[pageIndex]->entries[index % EntityPageSize];
        }

        [[nodiscard]] EntityInfo* TryGet(IndexType index)
        {
            if (index < EntityId::FirstIndex)
            {
                return nullptr;
            }

            const size_t pageIndex = index / EntityPageSize;
            if (pageIndex >= m_pages.size() || !m_pages[pageIndex])
            {
                return nullptr;
            }

            return &m_pages[pageIndex]->entries[index % EntityPageSize];
        }

        [[nodiscard]] const EntityInfo* TryGet(IndexType index) const
        {
            if (index < EntityId::FirstIndex)
            {
                return nullptr;
            }

            const size_t pageIndex = index / EntityPageSize;
            if (pageIndex >= m_pages.size() || !m_pages[pageIndex])
            {
                return nullptr;
            }

            return &m_pages[pageIndex]->entries[index % EntityPageSize];
        }
    };
}

#endif // ARCHON_ENTITYINFO_H
