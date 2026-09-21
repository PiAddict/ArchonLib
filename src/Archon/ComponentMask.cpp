#include <Archon/ComponentMask.h>

#include <algorithm>
#include <bit>

namespace Archon
{
    void ComponentMask::Set(const ComponentId id)
    {
        if (id == InvalidComponentId)
        {
            return;
        }

        const size_t index = id / 64;
        if (index >= m_mask.size())
        {
            m_mask.resize(index + 1);
        }

        m_mask[index] |= 1ULL << (id % 64);
    }

    bool ComponentMask::Test(const ComponentId id) const
    {
        if (id == InvalidComponentId)
        {
            return false;
        }

        const size_t index = id / 64;
        if (index >= m_mask.size())
        {
            return false;
        }

        return m_mask[index] & (1ULL << (id % 64));
    }

    void ComponentMask::Clear(const ComponentId id)
    {
        if (id == InvalidComponentId)
        {
            return;
        }

        const size_t index = id / 64;
        if (index >= m_mask.size())
        {
            return;
        }

        m_mask[index] &= ~(1ULL << (id % 64));
    }

    uint64_t ComponentMask::GetHash() const
    {
        uint64_t hash = 0;
        for (const uint64_t mask : m_mask)
        {
            hash ^= mask;
        }
        return hash;
    }

    void ComponentMask::Clear()
    {
        m_mask.clear();
    }

    bool ComponentMask::operator==(const ComponentMask& other) const
    {
        const size_t minSize = std::min(m_mask.size(), other.m_mask.size());

        for (size_t i = 0; i < minSize; ++i)
        {
            if (m_mask[i] != other.m_mask[i])
            {
                return false;
            }
        }

        for (size_t i = minSize; i < m_mask.size(); ++i)
        {
            if (m_mask[i] != 0)
            {
                return false;
            }
        }

        for (size_t i = minSize; i < other.m_mask.size(); ++i)
        {
            if (other.m_mask[i] != 0)
            {
                return false;
            }
        }

        return true;
    }

    void ComponentMask::ForEachComponentId(const std::function<void(ComponentId)>& callback) const
    {
        for (size_t i = 0; i < m_mask.size(); ++i)
        {
            uint64_t mask = m_mask[i];
            while (mask != 0)
            {
                const int bit = std::countr_zero(mask);
                callback(static_cast<ComponentId>(i * 64 + bit));
                mask &= mask - 1;
            }
        }
    }
}
