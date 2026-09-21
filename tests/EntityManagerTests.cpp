#include <Archon/Archon.h>

#include <gtest/gtest.h>

namespace
{
    struct Position
    {
        int x;
        int y;

        bool operator==(const Position&) const = default;
    };

    struct Velocity
    {
        int x;
        int y;

        bool operator==(const Velocity&) const = default;
    };

    struct Health
    {
        int value;

        bool operator==(const Health&) const = default;
    };

    TEST(EntityManager, DestroyedIdsAreInvalidAndTheirIndexIsReusedWithANewVersion)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();

        ASSERT_TRUE(manager.IsValid(entity));
        ASSERT_NE(manager.TryGetComponent<Position>(entity), nullptr);

        manager.DestroyEntity(entity);

        EXPECT_FALSE(manager.IsValid(entity));
        EXPECT_EQ(manager.TryGetComponent<Position>(entity), nullptr);
        EXPECT_FALSE(manager.TryView<Position>(entity).has_value());
        EXPECT_FALSE(manager.TrySetComponent(entity, Position{1, 2}));
        EXPECT_FALSE(manager.TryAddComponent(entity, Velocity{3, 4}));
        EXPECT_FALSE(manager.TryRemoveComponent<Position>(entity));

        const Archon::EntityId replacement = manager.CreateEntity<Position>();
        EXPECT_EQ(replacement.GetIndex(), entity.GetIndex());
        EXPECT_NE(replacement.GetVersion(), entity.GetVersion());
        EXPECT_TRUE(manager.IsValid(replacement));
    }

    TEST(EntityManager, StructuralChangesPreserveRetainedComponentsAndSupportEmptyArchetypes)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();
        manager.SetComponent(entity, Position{10, 20});

        ASSERT_TRUE(manager.TryAddComponent(entity, Velocity{3, 4}));
        EXPECT_TRUE((manager.HasComponents<Position, Velocity>(entity)));
        EXPECT_EQ(*manager.TryGetComponent<Position>(entity), (Position{10, 20}));
        EXPECT_EQ(*manager.TryGetComponent<Velocity>(entity), (Velocity{3, 4}));

        ASSERT_TRUE(manager.TryRemoveComponent<Velocity>(entity));
        EXPECT_TRUE(manager.HasComponent<Position>(entity));
        EXPECT_FALSE(manager.HasComponent<Velocity>(entity));
        EXPECT_EQ(*manager.TryGetComponent<Position>(entity), (Position{10, 20}));

        ASSERT_TRUE(manager.TryRemoveComponent<Position>(entity));
        EXPECT_FALSE((manager.HasComponents<Position, Velocity>(entity)));
        EXPECT_EQ(manager.TryGetComponent<Position>(entity), nullptr);

        ASSERT_TRUE(manager.TryAddComponent(entity, Health{100}));
        EXPECT_TRUE(manager.HasComponent<Health>(entity));
        EXPECT_EQ(*manager.TryGetComponent<Health>(entity), (Health{100}));
    }

    TEST(EntityManager, SwapRemovalKeepsTheOtherEntityLocationCurrent)
    {
        Archon::EntityManager manager;
        const Archon::EntityId migrated = manager.CreateEntity<Position, Velocity>();
        const Archon::EntityId remaining = manager.CreateEntity<Position, Velocity>();
        manager.SetComponent(migrated, Position{1, 2});
        manager.SetComponent(remaining, Position{3, 4});
        manager.SetComponent(remaining, Velocity{5, 6});

        ASSERT_TRUE(manager.TryAddComponent(migrated, Health{7}));

        ASSERT_TRUE(manager.IsValid(remaining));
        ASSERT_NE(manager.TryGetComponent<Position>(remaining), nullptr);
        ASSERT_NE(manager.TryGetComponent<Velocity>(remaining), nullptr);
        EXPECT_EQ(*manager.TryGetComponent<Position>(remaining), (Position{3, 4}));
        EXPECT_EQ(*manager.TryGetComponent<Velocity>(remaining), (Velocity{5, 6}));
    }

    TEST(EntityView, ProvidesTypedAccessNarrowingAndEntityIdConversion)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position, Velocity>();
        manager.SetComponent(entity, Position{1, 2});
        manager.SetComponent(entity, Velocity{3, 4});

        auto view = manager.TryView<Position, Velocity>(entity);
        ASSERT_TRUE(view.has_value());
        EXPECT_TRUE(view->IsValid());
        EXPECT_EQ(view->GetEntityId(), entity);
        EXPECT_EQ(static_cast<Archon::EntityId>(*view), entity);
        EXPECT_EQ(view->Get<Position>(), (Position{1, 2}));
        EXPECT_EQ(view->TryGet<Velocity>(), manager.TryGetComponent<Velocity>(entity));

        Archon::EntityView<Position> positionView(*view);
        EXPECT_EQ(positionView.Get<Position>(), (Position{1, 2}));
        EXPECT_EQ(positionView.TryGet<Position>(), manager.TryGetComponent<Position>(entity));
        EXPECT_FALSE(manager.TryView<Health>(entity).has_value());
    }

    TEST(EntityView, PublicConstructionReportsInvalidEntitiesAndMissingComponents)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();

        Archon::EntityView<Health> missingComponent(manager, entity);
        EXPECT_FALSE(missingComponent.IsValid());
        EXPECT_EQ(missingComponent.TryGet<Health>(), nullptr);

        manager.DestroyEntity(entity);
        Archon::EntityView<Position> destroyedEntity(manager, entity);
        EXPECT_FALSE(destroyedEntity.IsValid());
        EXPECT_EQ(destroyedEntity.TryGet<Position>(), nullptr);
    }

    TEST(EntityView, StructuralRelocationInvalidatesViewsButComponentWritesDoNot)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();
        Archon::EntityView<Position> view(manager, entity);

        ASSERT_TRUE(view.IsValid());
        manager.SetComponent(entity, Position{1, 2});
        EXPECT_TRUE(view.IsValid());
        EXPECT_EQ(view.Get<Position>(), (Position{1, 2}));

        ASSERT_TRUE(manager.TryAddComponent(entity, Velocity{3, 4}));
        EXPECT_FALSE(view.IsValid());
        EXPECT_EQ(view.TryGet<Position>(), nullptr);

        Archon::EntityView<Position, Velocity> freshView(manager, entity);
        EXPECT_TRUE(freshView.IsValid());
        EXPECT_EQ(freshView.Get<Position>(), (Position{1, 2}));
        EXPECT_EQ(freshView.Get<Velocity>(), (Velocity{3, 4}));
    }

    TEST(EntityView, SwapRemovalInvalidatesTheMovedEntitiesView)
    {
        Archon::EntityManager manager;
        const Archon::EntityId first = manager.CreateEntity<Position>();
        const Archon::EntityId moved = manager.CreateEntity<Position>();
        manager.SetComponent(moved, Position{5, 6});
        Archon::EntityView<Position> movedView(manager, moved);

        manager.DestroyEntity(first);

        EXPECT_FALSE(movedView.IsValid());
        EXPECT_EQ(movedView.TryGet<Position>(), nullptr);
        auto freshView = manager.TryView<Position>(moved);
        ASSERT_TRUE(freshView.has_value());
        EXPECT_EQ(freshView->Get<Position>(), (Position{5, 6}));
    }

    TEST(EntityView, ConstViewsProvideReadOnlyAccessAndCanBeCreatedFromMutableViews)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();
        manager.SetComponent(entity, Position{7, 8});

        Archon::EntityView<Position> mutableView(manager, entity);
        Archon::EntityView<const Position> constView(mutableView);

        ASSERT_TRUE(constView.IsValid());
        EXPECT_EQ(constView.Get<const Position>(), (Position{7, 8}));
        EXPECT_EQ(constView.TryGet<const Position>(), manager.TryGetComponent<Position>(entity));

        auto directConstView = manager.TryView<const Position>(entity);
        ASSERT_TRUE(directConstView.has_value());
        EXPECT_EQ(directConstView->Get<const Position>(), (Position{7, 8}));
    }
}
