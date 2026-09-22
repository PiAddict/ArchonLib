#ifndef ARCHON_TEMPLATEUTILITIES_H
#define ARCHON_TEMPLATEUTILITIES_H

#include <cstddef>
#include <type_traits>

namespace Archon
{
    template <typename Type>
    struct Optional
    {
        using TypeValue = Type;
    };

    template <typename Type>
    struct Without
    {
        using TypeValue = Type;
    };

    namespace Core
    {
        template <typename... Types>
        struct TypeList
        {
        };

        template <typename Type>
        struct IsOptional : std::false_type
        {
        };

        template <typename Type>
        struct IsOptional<Optional<Type>> : std::true_type
        {
        };

        template <typename Type>
        inline constexpr bool IsOptionalV = IsOptional<Type>::value;

        template <typename Type>
        struct IsWithout : std::false_type
        {
        };

        template <typename Type>
        struct IsWithout<Without<Type>> : std::true_type
        {
        };

        template <typename Type>
        inline constexpr bool IsWithoutV = IsWithout<Type>::value;

        template <typename Type>
        struct UnderlyingComponent
        {
            using TypeValue = Type;
        };

        template <typename Type>
        struct UnderlyingComponent<Optional<Type>>
        {
            using TypeValue = Type;
        };

        template <typename Type>
        struct UnderlyingComponent<Without<Type>>
        {
            using TypeValue = Type;
        };

        template <typename Type>
        using UnderlyingComponentT = typename UnderlyingComponent<Type>::TypeValue;

        template <typename Term>
        using NormalizedComponentT = std::remove_const_t<UnderlyingComponentT<Term>>;

        template <typename Term>
        struct IsRequiredTerm : std::bool_constant<!IsOptionalV<Term> && !IsWithoutV<Term>>
        {
        };

        template <typename Term>
        inline constexpr bool IsRequiredTermV = IsRequiredTerm<Term>::value;

        template <typename... Terms>
        struct AreUniqueUnderlyingComponents;

        template <>
        struct AreUniqueUnderlyingComponents<> : std::true_type
        {
        };

        template <typename First, typename... Rest>
        struct AreUniqueUnderlyingComponents<First, Rest...> : std::bool_constant<
            (!std::is_same_v<NormalizedComponentT<First>, NormalizedComponentT<Rest>> && ...) &&
            AreUniqueUnderlyingComponents<Rest...>::value>
        {
        };

        template <typename... Terms>
        inline constexpr bool AreUniqueUnderlyingComponentsV =
            AreUniqueUnderlyingComponents<Terms...>::value;

        template <typename Target, typename List>
        struct Contains;

        template <typename Target, typename... Types>
        struct Contains<Target, TypeList<Types...>> : std::bool_constant<(std::is_same_v<Target, Types> || ...)>
        {
        };

        template <typename Target, typename List>
        inline constexpr bool ContainsV = Contains<Target, List>::value;

        template <typename List>
        struct Size;

        template <typename... Types>
        struct Size<TypeList<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)>
        {
        };

        template <typename List>
        inline constexpr std::size_t SizeV = Size<List>::value;

        template<typename Type, typename List>
        struct Prepend;

        template<typename Type, typename... Types>
        struct Prepend<Type, TypeList<Types...>>
        {
            using TypeValue = TypeList<Type, Types...>;
        };

        template<typename Type, typename List>
        using PrependT = typename Prepend<Type, List>::TypeValue;

        template<typename Target, template<typename> typename Predicate>
        struct Filter;

        template<template<typename> typename Predicate>
        struct Filter<TypeList<>, Predicate>
        {
            using TypeValue = TypeList<>;
        };

        template<typename First, typename... Rest, template<typename> typename Predicate>
        struct Filter<TypeList<First, Rest...>, Predicate>
        {
        private:
            using FilteredRest = typename Filter<TypeList<Rest...>, Predicate>::TypeValue;

        public:
            using TypeValue = std::conditional_t<
                Predicate<First>::value,
                PrependT<First, FilteredRest>,
                FilteredRest>;
        };

        template<typename List, template<typename> typename Predicate>
        using FilterT = typename Filter<List, Predicate>::TypeValue;

    }
}

#endif // ARCHON_TEMPLATEUTILITIES_H
