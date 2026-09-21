#include <Archon/EntityId.h>

namespace Archon
{
    const EntityId EntityId::Null;

    bool EntityId::operator==(const EntityId& other) const noexcept
    {
        return m_id == other.m_id;
    }

    bool EntityId::operator!=(const EntityId& other) const noexcept
    {
        return !(*this == other);
    }

    VersionType EntityId::NextVersion(const VersionType version) noexcept
    {
        return version == MaxVersion ? FirstVersion : version + 1;
    }

    void EntityId::IncrementVersion() noexcept
    {
        m_id = Encode(GetIndex(), NextVersion(GetVersion()));
    }

    IndexType EntityId::GetIndex() const noexcept
    {
        return static_cast<IndexType>(m_id & IndexMask);
    }

    VersionType EntityId::GetVersion() const noexcept
    {
        return static_cast<VersionType>(m_id >> VersionShift);
    }

    EntityType EntityId::GetUnderlyingId() const noexcept
    {
        return m_id;
    }
}
