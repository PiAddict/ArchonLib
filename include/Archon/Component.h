#ifndef ARCHON_COMPONENT_H
#define ARCHON_COMPONENT_H

#include <concepts>
#include <type_traits>

namespace Archon
{
    template <typename Type>
    concept Component = std::same_as<Type, std::remove_cvref_t<Type>> && std::is_standard_layout_v<Type> && std::is_trivial_v<Type>;

    template <typename Type>
    concept ComponentAccess = Component<std::remove_const_t<Type>>;

    template <typename Type, typename... Types>
    concept ContainsType = (std::same_as<Type, Types> || ...);

    namespace Core
    {
        template <typename... Types>
        struct AreUniqueTypes : std::true_type
        {
        };

        template <typename First, typename... Rest>
        struct AreUniqueTypes<First, Rest...> : std::bool_constant<
            (!std::same_as<std::remove_cvref_t<First>, std::remove_cvref_t<Rest>> && ...) &&
            AreUniqueTypes<Rest...>::value>
        {
        };
    }

    template <typename... Types>
    concept UniqueTypes = Core::AreUniqueTypes<Types...>::value;
}

#endif // ARCHON_COMPONENT_H
