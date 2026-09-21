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
        constexpr uint64_t OffsetBasis = 14695981039346656037ULL;
        constexpr uint64_t Prime = 1099511628211ULL;

        size_t effectiveSize = m_mask.size();
        while (effectiveSize > 0 && m_mask[effectiveSize - 1] == 0)
        {
            --effectiveSize;
        }

        uint64_t hash = OffsetBasis;
        hash ^= static_cast<uint64_t>(effectiveSize);
        hash *= Prime;

        for (size_t i = 0; i < effectiveSize; ++i)
        {
            hash ^= m_mask[i];
            hash *= Prime;
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
                const auto bit = static_cast<size_t>(std::countr_zero(mask));
                callback(static_cast<ComponentId>(i * 64 + bit));
                mask &= mask - 1;
            }
        }
    }
}
