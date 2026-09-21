#include <Archon/EntityId.h>

namespace Archon
{
    EntityId EntityId::Null;

    bool EntityId::operator==(const EntityId& other) const
    {
        return m_id == other.m_id;
    }

    bool EntityId::operator!=(const EntityId& other) const
    {
        return !(*this == other);
    }

    VersionType EntityId::NextVersion(VersionType version)
    {
        return version == MaxVersion ? FirstVersion : version + 1;
    }

    void EntityId::IncrementVersion()
    {
        m_version = NextVersion(m_version);
    }

    IndexType EntityId::GetIndex() const
    {
        return m_index;
    }

    VersionType EntityId::GetVersion() const
    {
        return m_version;
    }

    EntityType EntityId::GetUnderlyingId() const
    {
        return m_id;
    }
}
