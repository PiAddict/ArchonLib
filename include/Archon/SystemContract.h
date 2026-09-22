#ifndef ARCHON_SYSTEMCONTRACT_H
#define ARCHON_SYSTEMCONTRACT_H

#include <type_traits>

#include <Archon/Component.h>
#include <Archon/TemplateUtilities.h>

#include <Archon/ComponentMask.h>

namespace Archon
{
    template <typename... Terms>
        requires Core::AreUniqueUnderlyingComponentsV<Terms...>
    class SystemContract
    {
        template <typename Requested, typename Term>
        static constexpr bool IsSameUnderlyingComponent = std::is_same_v<
            std::remove_const_t<Requested>,
            Core::NormalizedComponentT<Term>>;

        template <typename Requested, typename Term>
        struct TermGrantsReadAccess : std::bool_constant<
                !Core::IsWithoutV<Term> && IsSameUnderlyingComponent<Requested, Term>>
        {
        };

        template <typename Requested, typename Term>
        struct TermGrantsWriteAccess : std::bool_constant<
                !Core::IsWithoutV<Term> &&
                !std::is_const_v<Requested> &&
                !std::is_const_v<Core::UnderlyingComponentT<Term>> &&
                IsSameUnderlyingComponent<Requested, Term>>
        {
        };

        template <typename Requested, typename Term>
        struct TermProvidesRequiredComponent : std::bool_constant<
                Core::IsRequiredTermV<Term> &&
                IsSameUnderlyingComponent<Requested, Term>>
        {
        };

        template <typename Requested, typename Term>
        struct TermProvidesOptionalComponent : std::bool_constant<
                Core::IsOptionalV<Term> && IsSameUnderlyingComponent<Requested, Term>>
        {
        };

        template <typename Requested, typename Term>
        struct TermExcludesComponent : std::bool_constant<
                Core::IsWithoutV<Term> && IsSameUnderlyingComponent<Requested, Term>>
        {
        };

        static_assert(
            (Component<Core::NormalizedComponentT<Terms>> && ...),
            "SystemContract terms must refer to valid Archon components");

        template <typename Term>
        static void AddRequiredComponent(ComponentMask& mask)
        {
            if constexpr (Core::IsRequiredTermV<Term>)
            {
                using ComponentType = Core::NormalizedComponentT<Term>;
                mask.Set<ComponentType>();
            }
        }

        template <typename Term>
        static void AddExcludedComponent(ComponentMask& mask)
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                using ComponentType = Core::NormalizedComponentT<Term>;
                mask.Set<ComponentType>();
            }
        }

        static ComponentMask CreateRequiredMask()
        {
            ComponentMask mask;
            (AddRequiredComponent<Terms>(mask), ...);
            return mask;
        }

        static ComponentMask CreateExcludedMask()
        {
            ComponentMask mask;
            (AddExcludedComponent<Terms>(mask), ...);
            return mask;
        }

    public:
        using DeclaredTerms = Core::TypeList<Terms...>;
        using RequiredTerms = Core::FilterT<DeclaredTerms, Core::IsRequiredTerm>;
        using OptionalTerms = Core::FilterT<DeclaredTerms, Core::IsOptional>;
        using ExcludedTerms = Core::FilterT<DeclaredTerms, Core::IsWithout>;

        template <ComponentAccess Requested>
        static constexpr bool HasReadAccess = (TermGrantsReadAccess<Requested, Terms>::value || ...);

        template <ComponentAccess Requested>
        static constexpr bool HasWriteAccess = (TermGrantsWriteAccess<Requested, Terms>::value || ...);

        template <ComponentAccess Requested>
        static constexpr bool Requires = (TermProvidesRequiredComponent<Requested, Terms>::value || ...);

        template <ComponentAccess Requested>
        static constexpr bool MayHave = (TermProvidesOptionalComponent<Requested, Terms>::value || ...);

        template <ComponentAccess Requested>
        static constexpr bool Excludes = (TermExcludesComponent<Requested, Terms>::value || ...);

        [[nodiscard]] static const ComponentMask& GetRequiredMask()
        {
            static const ComponentMask mask = CreateRequiredMask();
            return mask;
        }

        [[nodiscard]] static const ComponentMask& GetExcludedMask()
        {
            static const ComponentMask mask = CreateExcludedMask();
            return mask;
        }
    };

    namespace Core
    {
        template <typename Type>
        struct IsSystemContract : std::false_type
        {
        };

        template <typename... Terms>
        struct IsSystemContract<SystemContract<Terms...>> : std::true_type
        {
        };

        template <typename Type>
        inline constexpr bool IsSystemContractV = IsSystemContract<Type>::value;
    }

    template <typename Type>
    concept SystemContractConcept = Core::IsSystemContractV<Type>;
}

#endif // ARCHON_SYSTEMCONTRACT_H
