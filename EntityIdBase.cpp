//
// Created by Mika Goetze on 20/9/2026.
//

#include "EntityIdBase.h"

bool EntityIdBase::operator==(const EntityIdBase& other) const
{
    return m_id == other.m_id;
}

bool EntityIdBase::operator!=(const EntityIdBase& other) const
{
    return !(*this == other);
}

void EntityIdBase::IncrementVersion()
{
    m_version = (m_version % 15) + 1;
}

EntityType EntityIdBase::GetUnderlyingId() const
{
    return m_id;
}
