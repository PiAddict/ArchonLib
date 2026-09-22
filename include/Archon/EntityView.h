#ifndef ARCHON_ENTITYVIEW_H
#define ARCHON_ENTITYVIEW_H

#include <cassert>
#include <memory>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>

#include <Archon/ArchetypeStorage.h>
#include <Archon/Component.h>
#include <Archon/EntityId.h>
#include <Archon/SystemContract.h>

namespace Archon
{
    class EntityManager;

    namespace Core
    {
        struct SystemQueryAccess;

        struct EntityViewState
        {
            EntityManager* manager = nullptr;
            EntityId entity;
            ArchetypeStorage* storage = nullptr;
            EntityLocation location{};

            EntityViewState(EntityManager& manager, EntityId entity);
            EntityViewState(EntityManager& manager, EntityId entity, ArchetypeStorage& storage, EntityLocation location);
            [[nodiscard]] bool IsValid() const;

            template <ComponentAccess ComponentType>
            [[nodiscard]] ComponentType* ResolveComponent() const
            {
                return storage == nullptr ? nullptr : storage->TryGetComponentData<ComponentType>(location.chunkIndex, location.columnIndex);
            }
        };

        template<typename Term>
        using EntityViewPointerT = UnderlyingComponentT<Term>*;

        template<typename Term>
        using EntityViewStorageT = std::conditional_t<IsWithoutV<Term>, std::monostate, EntityViewPointerT<Term>>;

        template<typename Term>
        using EntityViewChunkStorageT = std::conditional_t<IsWithoutV<Term>, std::monostate,
            std::optional<std::span<UnderlyingComponentT<Term>>>>;
    }

    // Entity views are transient and must not be retained across structural changes.
    // Optional terms cache a nullable pointer; excluded terms intentionally store no pointer.
    template <typename... Terms>
        requires Core::AreUniqueUnderlyingComponentsV<Terms...> && (Component<Core::NormalizedComponentT<Terms>> && ...)
    class EntityView
    {
        friend struct Core::SystemQueryAccess;

        template <typename... OtherTerms>
            requires Core::AreUniqueUnderlyingComponentsV<OtherTerms...> && (Component<Core::NormalizedComponentT<OtherTerms>> && ...)
        friend class EntityView;

        using Contract = SystemContract<Terms...>;
        Core::EntityViewState m_state;
        std::tuple<Core::EntityViewStorageT<Terms>...> m_components;

        using ChunkBindings = std::tuple<Core::EntityViewChunkStorageT<Terms>...>;

        template<typename Term>
        [[nodiscard]] static Core::EntityViewChunkStorageT<Term> BindChunkTerm(
            ArchetypeStorage& storage, const ChunkIndex chunkIndex)
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                return {};
            }
            else
            {
                return storage.TryGetComponentSpan<Core::UnderlyingComponentT<Term>>(chunkIndex);
            }
        }

        [[nodiscard]] static ChunkBindings BindChunk(ArchetypeStorage& storage, const ChunkIndex chunkIndex)
        {
            return ChunkBindings{BindChunkTerm<Terms>(storage, chunkIndex)...};
        }

        template<typename Term>
        [[nodiscard]] static Core::EntityViewStorageT<Term> ResolveBoundTerm(
            const ChunkBindings& bindings, const size_t entityIndex)
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                return {};
            }
            else
            {
                const auto& values = std::get<Core::EntityViewChunkStorageT<Term>>(bindings);
                return values ? std::addressof((*values)[entityIndex]) : nullptr;
            }
        }

        EntityView(EntityManager& manager, const EntityId entity, ArchetypeStorage& storage,
            const EntityLocation location, const ChunkBindings& bindings)
            : m_state(manager, entity, storage, location),
              m_components(ResolveBoundTerm<Terms>(bindings, location.columnIndex)...)
        {
        }

        template<typename Term>
        [[nodiscard]] Core::EntityViewStorageT<Term> ResolveTerm() const
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                return {};
            }
            else
            {
                return m_state.ResolveComponent<Core::UnderlyingComponentT<Term>>();
            }
        }

        template<typename Term>
        [[nodiscard]] bool HasRequiredTerm() const
        {
            if constexpr (Core::IsRequiredTermV<Term>)
            {
                return std::get<Core::EntityViewPointerT<Term>>(m_components) != nullptr;
            }
            return true;
        }

        template<typename Term>
        [[nodiscard]] bool ExclusionIsSatisfied() const
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                return !m_state.storage->GetInfo().componentMask.Test<Core::NormalizedComponentT<Term>>();
            }
            return true;
        }

        template<ComponentAccess Requested>
        [[nodiscard]] Requested* GetCached() const
        {
            return std::get<Requested*>(m_components);
        }

        template<typename TargetTerm, typename SourceContract>
        static constexpr bool CanNarrowTerm = []
        {
            using Requested = Core::UnderlyingComponentT<TargetTerm>;
            if constexpr (Core::IsWithoutV<TargetTerm>)
            {
                return SourceContract::template Excludes<Requested>;
            }
            else
            {
                constexpr bool sourceMentions = Core::IsRequiredTermV<TargetTerm>
                    ? SourceContract::template Requires<Requested>
                    : (SourceContract::template Requires<Requested> || SourceContract::template MayHave<Requested>);
                constexpr bool sourceCanProvideAccess = std::is_const_v<Requested>
                    ? SourceContract::template HasReadAccess<Requested>
                    : SourceContract::template HasWriteAccess<Requested>;
                return sourceMentions && sourceCanProvideAccess;
            }
        }();

        template<typename Term, typename... SourceTerms>
        [[nodiscard]] static Core::EntityViewStorageT<Term> GetSourceTerm(const EntityView<SourceTerms...>& source)
        {
            if constexpr (Core::IsWithoutV<Term>)
            {
                return {};
            }
            else
            {
                using Requested = Core::UnderlyingComponentT<Term>;
                using SourceContract = SystemContract<SourceTerms...>;
                if constexpr (SourceContract::template HasWriteAccess<std::remove_const_t<Requested>>)
                {
                    return source.template GetCached<std::remove_const_t<Requested>>();
                }
                else
                {
                    return source.template GetCached<const std::remove_const_t<Requested>>();
                }
            }
        }

    public:
        EntityView(EntityManager& manager, EntityId entity)
            : m_state(manager, entity), m_components(ResolveTerm<Terms>()...)
        {
        }

        EntityView(const EntityView&) = default;
        EntityView& operator=(const EntityView&) = default;

        template<typename... SourceTerms>
            requires Core::AreUniqueUnderlyingComponentsV<SourceTerms...> &&
                (Component<Core::NormalizedComponentT<SourceTerms>> && ...) &&
                (CanNarrowTerm<Terms, SystemContract<SourceTerms...>> && ...)
        EntityView(const EntityView<SourceTerms...>& source)
            : m_state(source.m_state), m_components(GetSourceTerm<Terms>(source)...)
        {
        }

        [[nodiscard]] EntityId GetEntityId() const noexcept { return m_state.entity; }
        operator EntityId() const noexcept { return m_state.entity; }

        [[nodiscard]] bool IsValid() const
        {
            return m_state.IsValid() && (HasRequiredTerm<Terms>() && ...) && (ExclusionIsSatisfied<Terms>() && ...);
        }

        explicit operator bool() const { return IsValid(); }

        template<ComponentAccess ComponentType>
            requires Contract::template Requires<ComponentType> && Contract::template HasReadAccess<ComponentType> &&
                (std::is_const_v<ComponentType> || Contract::template HasWriteAccess<ComponentType>)
        [[nodiscard]] ComponentType& Get() const
        {
            assert(IsValid() && "Cannot access an invalid or stale EntityView");
            ComponentType* component = GetCached<ComponentType>();
            assert(component != nullptr && "EntityView component cache is invalid");
            return *component;
        }

        template<ComponentAccess ComponentType>
            requires (Contract::template Requires<ComponentType> || Contract::template MayHave<ComponentType>) &&
                Contract::template HasReadAccess<ComponentType> &&
                (std::is_const_v<ComponentType> || Contract::template HasWriteAccess<ComponentType>)
        [[nodiscard]] ComponentType* TryGet() const
        {
            return IsValid() ? GetCached<ComponentType>() : nullptr;
        }
    };

    EntityView(EntityManager&, EntityId) -> EntityView<>;
}

#endif // ARCHON_ENTITYVIEW_H
