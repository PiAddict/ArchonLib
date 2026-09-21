#ifndef ARCHON_INTERNAL_MATH_H
#define ARCHON_INTERNAL_MATH_H

#include <cstddef>

namespace Archon::Core
{
    constexpr std::size_t AlignUp( std::size_t value, std::size_t alignment) noexcept
    {
        const std::size_t remainder = value % alignment;
        return remainder == 0 ? value : value + (alignment - remainder);
    }
}

#endif // ARCHON_INTERNAL_MATH_H
