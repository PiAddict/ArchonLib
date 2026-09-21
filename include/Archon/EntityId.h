#ifndef ARCHON_ENTITYID_H
#define ARCHON_ENTITYID_H

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Archon
{
    using IndexType = uint32_t;
    using VersionType = uint32_t;
    using EntityType = uint64_t;

    class EntityId
    {
    public:
        static constexpr uint32_t IndexBitCount = 32;
        static constexpr IndexType MaxIndex = std::numeric_limits<IndexType>::max();
        static constexpr IndexType FirstIndex = 1;
        static constexpr uint32_t VersionBitCount = 32;
        static constexpr VersionType MaxVersion = std::numeric_limits<VersionType>::max();
        static constexpr VersionType FirstVersion = 1;
        static const EntityId Null;

    protected:
        static constexpr IndexType InvalidIndex = 0;
        static constexpr VersionType InvalidVersion = 0;
        static constexpr EntityType IndexMask = MaxIndex;
        static constexpr uint32_t VersionShift = IndexBitCount;

        EntityType m_id{};

        [[nodiscard]] static constexpr EntityType Encode(const IndexType index, const VersionType version) noexcept
        {
            return (EntityType{version} << VersionShift) | EntityType{index};
        }

    public:
        constexpr EntityId() noexcept = default;

        constexpr EntityId(const IndexType index) noexcept
            : EntityId(index, FirstVersion)
        {
        }

        constexpr EntityId(const IndexType index, const VersionType version) noexcept
            : m_id(Encode(index, version))
        {
        }

        bool operator==(const EntityId&) const noexcept;
        bool operator!=(const EntityId&) const noexcept;

        [[nodiscard]] static VersionType NextVersion(VersionType version) noexcept;
        void IncrementVersion() noexcept;
        [[nodiscard]] IndexType GetIndex() const noexcept;
        [[nodiscard]] VersionType GetVersion() const noexcept;
        [[nodiscard]] EntityType GetUnderlyingId() const noexcept;
    };

    static_assert(sizeof(EntityId) == sizeof(EntityType));
    static_assert(std::is_standard_layout_v<EntityId>);
    static_assert(std::is_trivially_copyable_v<EntityId>);
}

#endif // ARCHON_ENTITYID_H
