#include <Archon/Archon.h>

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace
{
    struct Position
    {
        int x;
        int y;
    };

    struct Velocity
    {
        int x;
        int y;
    };

    struct Health
    {
        int value;
    };

    struct Target
    {
        int id;
    };

    struct Frozen
    {
    };

    struct Mana
    {
        int value;
    };

    struct NonTrivial
    {
        ~NonTrivial() {}
    };

    template <typename... ComponentTypes>
    concept CanCreateEntity = requires(Archon::EntityManager& manager)
    {
        manager.template CreateEntity<ComponentTypes...>();
    };

    template <typename... Terms>
    concept CanCreateSystemContract = requires
    {
        typename Archon::SystemContract<Terms...>;
    };

    static_assert(Archon::Component<Position>);
    static_assert(!Archon::Component<const Position>);
    static_assert(Archon::ComponentAccess<Position>);
    static_assert(Archon::ComponentAccess<const Position>);
    static_assert(!Archon::Component<NonTrivial>);
    static_assert(Archon::UniqueTypes<Position, Velocity, Health>);
    static_assert(!Archon::UniqueTypes<Position, Velocity, Position>);
    static_assert(!Archon::UniqueTypes<Position, const Position>);

    static_assert(!Archon::Core::IsOptionalV<Position>);
    static_assert(Archon::Core::IsOptionalV<Archon::Optional<Position>>);
    static_assert(Archon::Core::IsOptionalV<Archon::Optional<const Position>>);
    static_assert(!Archon::Core::IsWithoutV<Position>);
    static_assert(Archon::Core::IsWithoutV<Archon::Without<Position>>);
    static_assert(Archon::Core::IsRequiredTermV<Position>);
    static_assert(!Archon::Core::IsRequiredTermV<Archon::Optional<Position>>);
    static_assert(!Archon::Core::IsRequiredTermV<Archon::Without<Position>>);
    static_assert(std::same_as<Archon::Core::UnderlyingComponentT<Position>, Position>);
    static_assert(std::same_as<Archon::Core::UnderlyingComponentT<Archon::Optional<const Position>>, const Position>);
    static_assert(std::same_as<Archon::Core::UnderlyingComponentT<Archon::Without<Position>>, Position>);
    static_assert(std::same_as<Archon::Core::NormalizedComponentT<Archon::Optional<const Position>>, Position>);
    static_assert(Archon::Core::AreUniqueUnderlyingComponentsV<Position, const Velocity, Archon::Optional<Health>>);
    static_assert(!Archon::Core::AreUniqueUnderlyingComponentsV<Position, const Position>);
    static_assert(!Archon::Core::AreUniqueUnderlyingComponentsV<Position, Archon::Optional<Position>>);
    static_assert(!Archon::Core::AreUniqueUnderlyingComponentsV<Position, Archon::Without<Position>>);
    static_assert(Archon::Core::ContainsV<Position, Archon::Core::TypeList<Velocity, Position>>);
    static_assert(!Archon::Core::ContainsV<Health, Archon::Core::TypeList<Velocity, Position>>);
    static_assert(Archon::Core::SizeV<Archon::Core::TypeList<Position, Velocity, Health>> == 3);

    using MovementContract = Archon::SystemContract<
        Position,
        const Velocity,
        Archon::Optional<Health>,
        Archon::Optional<const Target>,
        Archon::Without<Frozen>>;

    static_assert(CanCreateSystemContract<Position, const Velocity>);
    static_assert(!CanCreateSystemContract<Position, Position>);
    static_assert(!CanCreateSystemContract<Position, const Position>);
    static_assert(!CanCreateSystemContract<Position, Archon::Optional<Position>>);
    static_assert(!CanCreateSystemContract<Position, Archon::Without<Position>>);

    static_assert(std::same_as<MovementContract::RequiredTerms, Archon::Core::TypeList<Position, const Velocity>>);
    static_assert(std::same_as<MovementContract::OptionalTerms, Archon::Core::TypeList<Archon::Optional<Health>, Archon::Optional<const Target>>>);
    static_assert(std::same_as<MovementContract::ExcludedTerms, Archon::Core::TypeList<Archon::Without<Frozen>>>);

    static_assert(MovementContract::HasReadAccess<Position>);
    static_assert(MovementContract::HasReadAccess<const Position>);
    static_assert(MovementContract::HasWriteAccess<Position>);
    static_assert(!MovementContract::HasWriteAccess<const Position>);
    static_assert(MovementContract::Requires<Position>);
    static_assert(!MovementContract::MayHave<Position>);

    static_assert(MovementContract::HasReadAccess<Velocity>);
    static_assert(MovementContract::HasReadAccess<const Velocity>);
    static_assert(!MovementContract::HasWriteAccess<Velocity>);
    static_assert(!MovementContract::HasWriteAccess<const Velocity>);
    static_assert(MovementContract::Requires<Velocity>);

    static_assert(MovementContract::HasReadAccess<Health>);
    static_assert(MovementContract::HasWriteAccess<Health>);
    static_assert(!MovementContract::Requires<Health>);
    static_assert(MovementContract::MayHave<Health>);

    static_assert(MovementContract::HasReadAccess<Target>);
    static_assert(!MovementContract::HasWriteAccess<Target>);
    static_assert(MovementContract::MayHave<Target>);

    static_assert(MovementContract::Excludes<Frozen>);
    static_assert(!MovementContract::HasReadAccess<Frozen>);
    static_assert(!MovementContract::HasWriteAccess<Frozen>);
    static_assert(!MovementContract::Requires<Frozen>);
    static_assert(!MovementContract::MayHave<Frozen>);

    static_assert(!MovementContract::HasReadAccess<Mana>);
    static_assert(!MovementContract::HasWriteAccess<Mana>);
    static_assert(!MovementContract::Requires<Mana>);
    static_assert(!MovementContract::MayHave<Mana>);
    static_assert(!MovementContract::Excludes<Mana>);

    static_assert(CanCreateEntity<Position>);
    static_assert(CanCreateEntity<Position, Velocity, Health>);
    static_assert(!CanCreateEntity<Position, Position>);
    static_assert(std::is_constructible_v<Archon::EntityView<Position>, const Archon::EntityView<Position, Velocity>&>);
    static_assert(std::is_constructible_v<Archon::EntityView<const Position>, const Archon::EntityView<Position>&>);
    static_assert(std::is_constructible_v<Archon::EntityView<const Position>, const Archon::EntityView<const Position, Velocity>&>);
    static_assert(!std::is_constructible_v<Archon::EntityView<Position>, const Archon::EntityView<const Position>&>);
    static_assert(std::same_as<decltype(std::declval<Archon::EntityView<Position>>().Get<Position>()), Position&>);
    static_assert(std::same_as<decltype(std::declval<Archon::EntityView<const Position>>().Get<const Position>()), const Position&>);
    static_assert(std::same_as<decltype(std::declval<Archon::EntityView<Position>>().TryGet<Position>()), Position*>);
    static_assert(std::same_as<decltype(std::declval<Archon::EntityView<const Position>>().TryGet<const Position>()), const Position*>);

    [[maybe_unused]] void InstantiateTypedPublicApi(Archon::EntityManager& manager)
    {
        [[maybe_unused]] const Archon::ArchetypeId oneComponent = manager.CreateArchetype<Position>();
        [[maybe_unused]] const Archon::ArchetypeId twoComponents = manager.CreateArchetype<Position, Velocity>();
        [[maybe_unused]] const Archon::ArchetypeId threeComponents = manager.CreateArchetype<Position, Velocity, Health>();

        auto entity = manager.CreateEntity<Position, Velocity, Health>();
        [[maybe_unused]] Position* position = manager.TryGetComponent<Position>(entity);
        [[maybe_unused]] const Position* constPosition = std::as_const(manager).TryGetComponent<Position>(entity);

        manager.SetComponent(entity, Position{1, 2});
        [[maybe_unused]] const bool set = manager.TrySetComponent(entity, Velocity{3, 4});
        [[maybe_unused]] const bool hasPosition = manager.HasComponent<Position>(entity);
        [[maybe_unused]] const bool hasAll = manager.HasComponents<Position, Velocity, Health>(entity);

        std::optional<Archon::EntityView<Position, Velocity, Health>> view = manager.TryView<Position, Velocity, Health>(entity);
        if (view)
        {
            [[maybe_unused]] Position& positionReference = view->Get<Position>();
            [[maybe_unused]] Velocity* velocity = view->TryGet<Velocity>();
            Archon::EntityView<Position> positionView(*view);
            [[maybe_unused]] const Archon::EntityId id = positionView;
        }

        std::optional<Archon::EntityView<const Position>> constView = manager.TryView<const Position>(entity);
        if (constView)
        {
            [[maybe_unused]] const Position& positionReference = constView->Get<const Position>();
            [[maybe_unused]] const Position* positionPointer = constView->TryGet<const Position>();
        }

        manager.RemoveComponent<Health>(entity);
        [[maybe_unused]] const bool readded = manager.TryAddComponent(entity, Health{100});
    }
}

int main()
{
    return 0;
}
