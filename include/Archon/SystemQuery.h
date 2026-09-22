#ifndef ARCHON_SYSTEMQUERY_H
#define ARCHON_SYSTEMQUERY_H

#include <cassert>
#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include <Archon/EntityManager.h>
#include <Archon/SystemContract.h>

namespace Archon
{
    namespace Core
    {
        struct SystemQueryAccess
        {
            template <typename Callback>
            static void ForEachInitializedArchetype(EntityManager& manager, Callback&& callback)
            {
                manager.ForEachInitializedArchetype(std::forward<Callback>(callback));
            }

            template <typename View>
            [[nodiscard]] static auto BindChunk(ArchetypeStorage& storage, const ChunkIndex chunkIndex)
            {
                return View::BindChunk(storage, chunkIndex);
            }

            template <typename View, typename Bindings>
            [[nodiscard]] static View MakeView(EntityManager& manager, ArchetypeStorage& storage,
                                               const ChunkIndex chunkIndex, const ColumnIndex columnIndex, const EntityId entity,
                                               const Bindings& bindings)
            {
                return View(manager, entity, storage, EntityLocation{storage.GetId(), chunkIndex, columnIndex}, bindings);
            }

            template <ComponentAccess ComponentType, typename View>
            [[nodiscard]] static ComponentType* GetCached(View& view)
            {
                return view.template GetCached<ComponentType>();
            }
        };

        template <typename List>
        struct EntityViewFromTerms;

        template <typename... Terms>
        struct EntityViewFromTerms<TypeList<Terms...>>
        {
            using Type = EntityView<Terms...>;
        };

        template <typename T>
        struct CallableTraits;

        template <typename Class, typename Return, typename... Arguments>
        struct CallableTraits<Return (Class::*)(Arguments...) const>
        {
            using ArgumentsList = TypeList<Arguments...>;
        };

        template <typename Class, typename Return, typename... Arguments>
        struct CallableTraits<Return (Class::*)(Arguments...)>
        {
            using ArgumentsList = TypeList<Arguments...>;
        };

        template <typename Class, typename Return, typename... Arguments>
        struct CallableTraits<Return (Class::*)(Arguments...) const noexcept> : CallableTraits<Return (Class::*)(Arguments...) const>
        {
        };

        template <typename Class, typename Return, typename... Arguments>
        struct CallableTraits<Return (Class::*)(Arguments...) noexcept> : CallableTraits<Return (Class::*)(Arguments...)>
        {
        };

        template <typename Callback, typename View>
        auto SelectCallbackOperator(int) -> std::type_identity<decltype(&Callback::operator())>;

        template <typename Callback, typename View>
        auto SelectCallbackOperator(long) -> std::type_identity<decltype(&Callback::template operator()<View>)>;

        template <typename Callback, typename View>
        using CallbackArgumentsT = typename CallableTraits<typename decltype(SelectCallbackOperator<Callback, View>(0))::type>::ArgumentsList;
    }

    template <SystemContractConcept Contract>
    class QueryChunk
    {
        ArchetypeStorage& m_storage;
        ChunkIndex m_chunkIndex;

    public:
        QueryChunk(ArchetypeStorage& storage, const ChunkIndex chunkIndex)
            : m_storage(storage), m_chunkIndex(chunkIndex)
        {
        }

        [[nodiscard]] std::span<const EntityId> Entities() const { return m_storage.GetEntities(m_chunkIndex); }

        template <ComponentAccess ComponentType>
            requires Contract::template Requires<ComponentType> && Contract::template HasReadAccess<ComponentType> &&
            (std::is_const_v<ComponentType> || Contract::template HasWriteAccess<ComponentType>)
        [[nodiscard]] std::span<ComponentType> Get() const
        {
            return m_storage.GetComponentSpan<ComponentType>(m_chunkIndex);
        }

        template <ComponentAccess ComponentType>
            requires Contract::template MayHave<ComponentType> && Contract::template HasReadAccess<ComponentType> &&
            (std::is_const_v<ComponentType> || Contract::template HasWriteAccess<ComponentType>)
        [[nodiscard]] std::optional<std::span<ComponentType>> TryGet() const
        {
            return m_storage.TryGetComponentSpan<ComponentType>(m_chunkIndex);
        }
    };

    template <SystemContractConcept Contract>
    class SystemQuery
    {
    public:
        using View = typename Core::EntityViewFromTerms<typename Contract::DeclaredTerms>::Type;
        using Chunk = QueryChunk<Contract>;

    private:
        EntityManager& m_entityManager;
        const ComponentMask& m_requiredComponents;
        const ComponentMask& m_excludedComponents;

        [[nodiscard]] bool Matches(const ArchetypeStorage& storage) const
        {
            const ComponentMask& archetypeMask = storage.GetInfo().componentMask;
            return archetypeMask.ContainsAll(m_requiredComponents) && !archetypeMask.Intersects(m_excludedComponents);
        }

        template <typename Parameter>
        static constexpr bool IsValidComponentParameter = []
        {
            using Bare = std::remove_cvref_t<Parameter>;
            if constexpr (std::is_pointer_v<Bare>)
            {
                using Requested = std::remove_pointer_t<Bare>;
                return !std::is_reference_v<Parameter> && ComponentAccess<Requested> &&
                    Contract::template MayHave<Requested> && Contract::template HasReadAccess<Requested> &&
                    (std::is_const_v<Requested> || Contract::template HasWriteAccess<Requested>);
            }
            else
            {
                using Requested = std::remove_reference_t<Parameter>;
                return std::is_lvalue_reference_v<Parameter> && ComponentAccess<Requested> &&
                    Contract::template Requires<Requested> && Contract::template HasReadAccess<Requested> &&
                    (std::is_const_v<Requested> || Contract::template HasWriteAccess<Requested>);
            }
        }();

        template <typename Parameter>
        static decltype(auto) BindComponent(View& view)
        {
            using Bare = std::remove_cvref_t<Parameter>;
            if constexpr (std::is_pointer_v<Bare>)
            {
                using Requested = std::remove_pointer_t<Bare>;
                return Core::SystemQueryAccess::GetCached<Requested>(view);
            }
            else
            {
                using Requested = std::remove_reference_t<Parameter>;
                Requested* component = Core::SystemQueryAccess::GetCached<Requested>(view);
                assert(component != nullptr && "Required query component binding is missing");
                return *component;
            }
        }

        template <typename Callback, typename First, typename... Parameters>
        static void InvokeDirect(Callback& callback, View& view, Core::TypeList<First, Parameters...>)
        {
            static_assert(std::same_as<std::remove_cvref_t<First>, View>,
                "The first SystemQuery callback parameter must be the query EntityView (or auto)");
            static_assert((IsValidComponentParameter<Parameters> && ...),
                          "Query callback component parameters must be required T&/const T& or optional T*/const T* permitted by the contract");
            callback(view, BindComponent<Parameters>(view)...);
        }

        template <typename Callback>
        void ForEachView(Callback& callback)
        {
            Core::SystemQueryAccess::ForEachInitializedArchetype(m_entityManager, [&](ArchetypeStorage& storage)
            {
                if (!Matches(storage))
                {
                    return;
                }

                for (ChunkIndex chunkIndex = 0; chunkIndex < storage.GetNumChunks(); ++chunkIndex)
                {
                    Chunk chunk(storage, chunkIndex);
                    const std::span<const EntityId> entities = chunk.Entities();
                    if (entities.empty())
                    {
                        continue;
                    }
                    const auto bindings = Core::SystemQueryAccess::BindChunk<View>(storage, chunkIndex);
                    for (ColumnIndex columnIndex = 0; columnIndex < entities.size(); ++columnIndex)
                    {
                        View view = Core::SystemQueryAccess::MakeView<View>(m_entityManager, storage,
                                                                            chunkIndex, columnIndex, entities[columnIndex], bindings);
                        callback(view);
                    }
                }
            });
        }

        template <typename Callback, typename... Arguments>
        void ForEachDirect(Callback& callback, Core::TypeList<Arguments...> arguments)
        {
            static_assert(sizeof...(Arguments) > 0, "SystemQuery callbacks require an EntityView first parameter");
            Core::SystemQueryAccess::ForEachInitializedArchetype(m_entityManager, [&](ArchetypeStorage& storage)
            {
                if (!Matches(storage))
                {
                    return;
                }

                for (ChunkIndex chunkIndex = 0; chunkIndex < storage.GetNumChunks(); ++chunkIndex)
                {
                    Chunk chunk(storage, chunkIndex);
                    const std::span<const EntityId> entities = chunk.Entities();
                    if (entities.empty())
                    {
                        continue;
                    }
                    const auto bindings = Core::SystemQueryAccess::BindChunk<View>(storage, chunkIndex);
                    for (ColumnIndex columnIndex = 0; columnIndex < entities.size(); ++columnIndex)
                    {
                        View view = Core::SystemQueryAccess::MakeView<View>(m_entityManager, storage,
                                                                            chunkIndex, columnIndex, entities[columnIndex], bindings);
                        InvokeDirect(callback, view, arguments);
                    }
                }
            });
        }

    public:
        explicit SystemQuery(EntityManager& entityManager)
            : m_entityManager(entityManager),
              m_requiredComponents(Contract::GetRequiredMask()),
              m_excludedComponents(Contract::GetExcludedMask())
        {
        }

        template <typename Callback>
        void ForEachChunk(Callback&& callback)
        {
            Core::SystemQueryAccess::ForEachInitializedArchetype(m_entityManager, [&](ArchetypeStorage& storage)
            {
                if (!Matches(storage))
                {
                    return;
                }

                for (ChunkIndex chunkIndex = 0; chunkIndex < storage.GetNumChunks(); ++chunkIndex)
                {
                    Chunk chunk(storage, chunkIndex);
                    if (!chunk.Entities().empty())
                    {
                        callback(chunk);
                    }
                }
            });
        }

        template <typename Callback>
        void ForEach(Callback&& callback)
        {
            using CallbackType = std::remove_reference_t<Callback>;
            if constexpr (std::invocable<CallbackType&, View&>)
            {
                ForEachView(callback);
            }
            else
            {
                using Arguments = Core::CallbackArgumentsT<CallbackType, View>;
                ForEachDirect(callback, Arguments{});
            }
        }
    };
}

#endif // ARCHON_SYSTEMQUERY_H
