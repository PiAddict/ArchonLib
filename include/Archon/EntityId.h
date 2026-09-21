#ifndef ARCHON_ENTITYID_H
#define ARCHON_ENTITYID_H

#include <cstdint>

namespace Archon
{
    using IndexType = uint32_t;
    using VersionType = uint32_t;
    using EntityType = uint32_t;

    class EntityId
    {
    public:
        static constexpr IndexType IndexBitCount = 24;
        static constexpr IndexType MaxIndex = (IndexType{1} << IndexBitCount) - 1;
        static constexpr IndexType FirstIndex = 1;
        static constexpr VersionType VersionBitCount = 8;
        static constexpr VersionType MaxVersion = (VersionType{1} << VersionBitCount) - 1;
        static constexpr VersionType FirstVersion = 1;
        static EntityId Null;

    protected:
        static constexpr IndexType InvalidIndex = 0;
        static constexpr VersionType InvalidVersion = 0;

        static_assert(sizeof(EntityType) <= (IndexBitCount + VersionBitCount) / 8);

        union
        {
            EntityType m_id{};

            struct
            {
                IndexType m_index : IndexBitCount;
                VersionType m_version : VersionBitCount;
            };
        };

    public:
        EntityId() : m_index(InvalidIndex), m_version(InvalidVersion)
        {
        }

        EntityId(IndexType index) : m_index(index), m_version(FirstVersion)
        {
        }

        EntityId(IndexType index, VersionType version) : m_index(index), m_version(version)
        {
        }

        bool operator==(const EntityId&) const;
        bool operator!=(const EntityId&) const;

        [[nodiscard]] static VersionType NextVersion(VersionType version);
        void IncrementVersion();
        [[nodiscard]] IndexType GetIndex() const;
        [[nodiscard]] VersionType GetVersion() const;
        [[nodiscard]] EntityType GetUnderlyingId() const;
    };
}

#endif // ARCHON_ENTITYID_H
