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

    struct Frozen
    {
    };

    struct Ground
    {
        int value;
        bool operator==(const Ground&) const = default;
    };

    TEST(SystemContract, CachesQueryMasksAndMatchesArchetypeComposition)
    {
        using Contract = Archon::SystemContract<
            Position,
            const Velocity,
            Archon::Optional<Health>,
            Archon::Without<Frozen>>;

        const Archon::ComponentMask& required = Contract::GetRequiredMask();
        const Archon::ComponentMask& excluded = Contract::GetExcludedMask();

        EXPECT_EQ(&required, &Contract::GetRequiredMask());
        EXPECT_EQ(&excluded, &Contract::GetExcludedMask());
        EXPECT_TRUE(required.Test<Position>());
        EXPECT_TRUE(required.Test<Velocity>());
        EXPECT_FALSE(required.Test<Health>());
        EXPECT_TRUE(excluded.Test<Frozen>());

        const Archon::ComponentMask matching = Archon::ComponentMask::Create<Position, Velocity>();
        const Archon::ComponentMask excludedMatch = Archon::ComponentMask::Create<Position, Velocity, Frozen>();
        const Archon::ComponentMask incomplete = Archon::ComponentMask::Create<Position>();

        EXPECT_TRUE(matching.ContainsAll(required));
        EXPECT_FALSE(matching.Intersects(excluded));
        EXPECT_FALSE(incomplete.ContainsAll(required));
        EXPECT_TRUE(excludedMatch.ContainsAll(required));
        EXPECT_TRUE(excludedMatch.Intersects(excluded));
    }

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

    TEST(SystemQuery, MatchesOnlyThisManagersInitializedStoragesAndExposesChunkSpans)
    {
        using Contract = Archon::SystemContract<Position, const Velocity, Archon::Optional<const Ground>, Archon::Without<Frozen>>;
        Archon::EntityManager firstManager;
        Archon::EntityManager secondManager;

        // Register and initialize a matching storage only in the first world.
        const Archon::EntityId firstOnly = firstManager.CreateEntity<Position, Velocity>();
        firstManager.SetComponent(firstOnly, Position{1, 2});
        firstManager.SetComponent(firstOnly, Velocity{3, 4});

        const Archon::EntityId matching = secondManager.CreateEntity<Position, Velocity, Ground>();
        const Archon::EntityId optionalAbsent = secondManager.CreateEntity<Position, Velocity>();
        const Archon::EntityId excluded = secondManager.CreateEntity<Position, Velocity, Frozen>();
        secondManager.SetComponent(matching, Position{10, 11});
        secondManager.SetComponent(matching, Velocity{12, 13});
        secondManager.SetComponent(matching, Ground{14});
        secondManager.SetComponent(optionalAbsent, Position{20, 21});
        secondManager.SetComponent(optionalAbsent, Velocity{22, 23});
        secondManager.SetComponent(excluded, Position{30, 31});
        secondManager.SetComponent(excluded, Velocity{32, 33});

        Archon::SystemQuery<Contract> query(secondManager);
        size_t chunkCount = 0;
        size_t entityCount = 0;
        query.ForEachChunk([&](const Archon::QueryChunk<Contract>& chunk)
        {
            ++chunkCount;
            const auto positions = chunk.Get<Position>();
            const auto velocities = chunk.Get<const Velocity>();
            const auto ground = chunk.TryGet<const Ground>();
            EXPECT_EQ(positions.size(), chunk.Entities().size());
            EXPECT_EQ(velocities.size(), chunk.Entities().size());
            EXPECT_EQ(ground.has_value(), chunk.Entities()[0] == matching);
            entityCount += chunk.Entities().size();
        });

        EXPECT_EQ(chunkCount, 2U);
        EXPECT_EQ(entityCount, 2U);
    }

    TEST(SystemQuery, EmptyChunksExposeEmptySpansForPresentComponents)
    {
        Archon::EntityManager manager;
        const Archon::EntityId entity = manager.CreateEntity<Position>();
        manager.DestroyEntity(entity);

        Archon::ArchetypeStorage& storage = manager.GetArchetypeStorage(
            Archon::ArchetypeRegistry::GetId<Position>());
        ASSERT_EQ(storage.GetNumChunks(), 1U);
        EXPECT_TRUE(storage.GetEntities(0).empty());

        const auto positions = storage.TryGetComponentSpan<Position>(0);
        ASSERT_TRUE(positions.has_value());
        EXPECT_TRUE(positions->empty());
        EXPECT_TRUE(storage.GetComponentSpan<Position>(0).empty());
        EXPECT_FALSE(storage.TryGetComponentSpan<Velocity>(0).has_value());
        EXPECT_FALSE(storage.TryGetComponentSpan<Position>(1).has_value());
    }

    TEST(SystemQuery, ForEachBindsContractAwareViewsAndDirectComponentParameters)
    {
        using Contract = Archon::SystemContract<Position, const Velocity, Archon::Optional<const Ground>, Archon::Without<Frozen>>;
        Archon::EntityManager manager;
        const Archon::EntityId withGround = manager.CreateEntity<Position, Velocity, Ground>();
        const Archon::EntityId withoutGround = manager.CreateEntity<Position, Velocity>();
        manager.SetComponent(withGround, Position{1, 2});
        manager.SetComponent(withGround, Velocity{3, 4});
        manager.SetComponent(withGround, Ground{5});
        manager.SetComponent(withoutGround, Position{6, 7});
        manager.SetComponent(withoutGround, Velocity{8, 9});

        Archon::SystemQuery<Contract> query(manager);
        Position* withGroundPosition = nullptr;
        Position* withoutGroundPosition = nullptr;
        query.ForEachChunk([&](const Archon::QueryChunk<Contract>& chunk)
        {
            const auto entities = chunk.Entities();
            const auto positions = chunk.Get<Position>();
            for (size_t index = 0; index < entities.size(); ++index)
            {
                Position*& destination = entities[index] == withGround ? withGroundPosition : withoutGroundPosition;
                destination = &positions[index];
            }
        });

        size_t viewVisits = 0;
        query.ForEach([&](auto entity)
        {
            ++viewVisits;
            EXPECT_NE(entity.template TryGet<Position>(), nullptr);
            EXPECT_NE(entity.template TryGet<const Velocity>(), nullptr);
            if (entity.GetEntityId() == withGround)
            {
                EXPECT_EQ(entity.template TryGet<Position>(), withGroundPosition);
                EXPECT_EQ(entity.template TryGet<const Ground>()->value, 5);
            }
            else
            {
                EXPECT_EQ(entity.template TryGet<Position>(), withoutGroundPosition);
                EXPECT_EQ(entity.template TryGet<const Ground>(), nullptr);
            }
        });
        EXPECT_EQ(viewVisits, 2U);

        size_t directVisits = 0;
        query.ForEach([&](auto entity, Position& position, const Velocity& velocity, const Ground* ground)
        {
            ++directVisits;
            EXPECT_EQ(&position, entity.template TryGet<Position>());
            EXPECT_EQ(&velocity, entity.template TryGet<const Velocity>());
            EXPECT_EQ(ground, entity.template TryGet<const Ground>());
            position.x += velocity.x;
        });

        EXPECT_EQ(directVisits, 2U);
        EXPECT_EQ(manager.TryGetComponent<Position>(withGround)->x, 4);
        EXPECT_EQ(manager.TryGetComponent<Position>(withoutGround)->x, 14);
    }
}
