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

    struct NonTrivial
    {
        ~NonTrivial() {}
    };

    template <typename... ComponentTypes>
    concept CanCreateEntity = requires(Archon::EntityManager& manager)
    {
        manager.template CreateEntity<ComponentTypes...>();
    };

    static_assert(Archon::Component<Position>);
    static_assert(!Archon::Component<const Position>);
    static_assert(Archon::ComponentAccess<Position>);
    static_assert(Archon::ComponentAccess<const Position>);
    static_assert(!Archon::Component<NonTrivial>);
    static_assert(Archon::UniqueTypes<Position, Velocity, Health>);
    static_assert(!Archon::UniqueTypes<Position, Velocity, Position>);
    static_assert(!Archon::UniqueTypes<Position, const Position>);
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
