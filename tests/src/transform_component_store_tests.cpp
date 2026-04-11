#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

#include <unordered_set>

using namespace Vulkyrie;

// Helper to create a TransformComponent with a distinct position for easy identification.
static TransformComponent makeTransform(float x, float y = 0.0f, float z = 0.0f) {
    TransformComponent t{};
    t.Position = glm::vec3(x, y, z);
    t.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    t.Scale = glm::vec3(1.0f);
    return t;
}

// Verifies the dense packing invariant: active components occupy [0, activeCount)
// and inactive components occupy [activeCount, totalCount).
static void requireDensePacking(TransformComponentStore &store, const std::vector<Entity> &expectedActive, const std::vector<Entity> &expectedInactive) {
    REQUIRE(store.GetActiveComponentCount() == expectedActive.size());
    REQUIRE(store.GetTotalComponentCount() == expectedActive.size() + expectedInactive.size());

    auto activeEntities = store.GetActiveEntities();
    auto activeTransforms = store.GetActiveTransforms();
    REQUIRE(activeEntities.size() == expectedActive.size());
    REQUIRE(activeTransforms.size() == expectedActive.size());

    std::unordered_set<Entity> activeSet(activeEntities.begin(), activeEntities.end());
    for (const auto &e : expectedActive) {
        REQUIRE(activeSet.contains(e));
    }
    for (const auto &e : expectedInactive) {
        REQUIRE_FALSE(activeSet.contains(e));
    }

    // Verify active transforms are contiguous and addressable through the span.
    for (size_t i = 0; i < activeEntities.size(); ++i) {
        REQUIRE(&store.GetTransform(activeEntities[i]) == &activeTransforms[i]);
    }
}

// ===========================================================================================
// AddComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add single active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetTransform(e).Position.x == 1.0f);
}

TEST_CASE("TransformComponentStore - Add single inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(2.0f), false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetTransform(e).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add multiple active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), true);

    requireDensePacking(store, { e1, e2, e3 }, {});
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Add multiple inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), false);
    store.AddComponent(e2, makeTransform(2.0f), false);

    requireDensePacking(store, {}, { e1, e2 });
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add active after inactive preserves partition", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity inactive1 = em.CreateEntity();
    Entity inactive2 = em.CreateEntity();
    Entity active1 = em.CreateEntity();

    store.AddComponent(inactive1, makeTransform(10.0f), false);
    store.AddComponent(inactive2, makeTransform(20.0f), false);
    store.AddComponent(active1, makeTransform(30.0f), true);

    requireDensePacking(store, { active1 }, { inactive1, inactive2 });
    REQUIRE(store.GetTransform(inactive1).Position.x == 10.0f);
    REQUIRE(store.GetTransform(inactive2).Position.x == 20.0f);
    REQUIRE(store.GetTransform(active1).Position.x == 30.0f);
}

TEST_CASE("TransformComponentStore - Interleaved active and inactive additions", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity e4 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), false);
    store.AddComponent(e3, makeTransform(3.0f), true);
    store.AddComponent(e4, makeTransform(4.0f), false);

    requireDensePacking(store, { e1, e3 }, { e2, e4 });
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
    REQUIRE(store.GetTransform(e4).Position.x == 4.0f);
}

// ===========================================================================================
// SetComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - SetComponent updates values", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    store.SetTransform(e, makeTransform(99.0f, 88.0f, 77.0f));
    auto &t = store.GetTransform(e);

    REQUIRE(t.Position.x == 99.0f);
    REQUIRE(t.Position.y == 88.0f);
    REQUIRE(t.Position.z == 77.0f);
    requireDensePacking(store, { e }, {});
}

TEST_CASE("TransformComponentStore - SetComponent on inactive entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);

    store.SetTransform(e, makeTransform(42.0f));
    REQUIRE(store.GetTransform(e).Position.x == 42.0f);
    requireDensePacking(store, {}, { e });
}

// ===========================================================================================
// RemoveComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - Remove only active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);
    store.RemoveComponent(e);

    requireDensePacking(store, {}, {});
}

TEST_CASE("TransformComponentStore - Remove only inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);
    store.RemoveComponent(e);

    requireDensePacking(store, {}, {});
}

TEST_CASE("TransformComponentStore - Remove active component preserves other active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), true);

    store.RemoveComponent(e1);

    requireDensePacking(store, { e2, e3 }, {});
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove active component with inactive present", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(a2, makeTransform(2.0f), true);
    store.AddComponent(i1, makeTransform(3.0f), false);

    store.RemoveComponent(a1);

    requireDensePacking(store, { a2 }, { i1 });
    REQUIRE(store.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove inactive component with active present", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(i1, makeTransform(2.0f), false);
    store.AddComponent(i2, makeTransform(3.0f), false);

    store.RemoveComponent(i1);

    requireDensePacking(store, { a1 }, { i2 });
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(i2).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove first active among many active and inactive", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(a2, makeTransform(2.0f), true);
    store.AddComponent(a3, makeTransform(3.0f), true);
    store.AddComponent(i1, makeTransform(4.0f), false);
    store.AddComponent(i2, makeTransform(5.0f), false);

    store.RemoveComponent(a1);

    requireDensePacking(store, { a2, a3 }, { i1, i2 });
    REQUIRE(store.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(store.GetTransform(i2).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Remove all components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), false);

    store.RemoveComponent(e2);
    requireDensePacking(store, { e1 }, { e3 });
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);

    store.RemoveComponent(e1);
    requireDensePacking(store, {}, { e3 });
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);

    store.RemoveComponent(e3);
    requireDensePacking(store, {}, {});
}

// ===========================================================================================
// SetActiveStatus
// ===========================================================================================

TEST_CASE("TransformComponentStore - SetActiveStatus true on inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(5.0f), false);

    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus true on already active component is no-op", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(5.0f), true);

    store.SetActiveStatus(e, true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus true on one of several inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), false);
    store.AddComponent(e2, makeTransform(2.0f), false);
    store.AddComponent(e3, makeTransform(3.0f), false);

    store.SetActiveStatus(e2, true);

    requireDensePacking(store, { e2 }, { e1, e3 });
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus true with existing active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(i1, makeTransform(2.0f), false);

    store.SetActiveStatus(i1, true);

    requireDensePacking(store, { a1, i1 }, {});
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 2.0f);
}

// ===========================================================================================
// SetActiveStatus (deactivate)
// ===========================================================================================

TEST_CASE("TransformComponentStore - SetActiveStatus false on active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(5.0f), true);

    requireDensePacking(store, { e }, {});

    store.SetActiveStatus(e, false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus false on already inactive component is no-op", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(5.0f), false);

    store.SetActiveStatus(e, false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus false on one of several active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), true);

    store.SetActiveStatus(e2, false);

    requireDensePacking(store, { e1, e3 }, { e2 });
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus false preserves inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(a2, makeTransform(2.0f), true);
    store.AddComponent(i1, makeTransform(3.0f), false);

    store.SetActiveStatus(a1, false);

    requireDensePacking(store, { a2 }, { a1, i1 });
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 3.0f);
}

// ===========================================================================================
// SetActiveStatus round-trips
// ===========================================================================================

TEST_CASE("TransformComponentStore - SetActiveStatus true then false returns to original state", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(i1, makeTransform(2.0f), false);

    store.SetActiveStatus(i1, true);
    requireDensePacking(store, { a1, i1 }, {});

    store.SetActiveStatus(i1, false);
    requireDensePacking(store, { a1 }, { i1 });
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus false then true returns to original state", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(7.0f), true);

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetTransform(e).Position.x == 7.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus true on all inactive components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), false);
    store.AddComponent(e2, makeTransform(2.0f), false);
    store.AddComponent(e3, makeTransform(3.0f), false);

    store.SetActiveStatus(e3, true);
    requireDensePacking(store, { e3 }, { e1, e2 });

    store.SetActiveStatus(e1, true);
    requireDensePacking(store, { e3, e1 }, { e2 });

    store.SetActiveStatus(e2, true);
    requireDensePacking(store, { e3, e1, e2 }, {});

    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - SetActiveStatus false on all active components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), true);

    store.SetActiveStatus(e2, false);
    requireDensePacking(store, { e1, e3 }, { e2 });

    store.SetActiveStatus(e1, false);
    requireDensePacking(store, { e3 }, { e1, e2 });

    store.SetActiveStatus(e3, false);
    requireDensePacking(store, {}, { e1, e2, e3 });

    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
}

// ===========================================================================================
// Combined operations (add, remove, SetActiveStatus, set)
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add, SetActiveStatus false, remove active", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), true);

    store.SetActiveStatus(e2, false);
    requireDensePacking(store, { e1, e3 }, { e2 });

    store.RemoveComponent(e1);
    requireDensePacking(store, { e3 }, { e2 });
    REQUIRE(store.GetTransform(e3).Position.x == 3.0f);
    REQUIRE(store.GetTransform(e2).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add inactive, SetActiveStatus true, set, SetActiveStatus false", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(0.0f), false);
    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});

    store.SetTransform(e, makeTransform(50.0f));
    REQUIRE(store.GetTransform(e).Position.x == 50.0f);
    requireDensePacking(store, { e }, {});

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetTransform(e).Position.x == 50.0f);
}

TEST_CASE("TransformComponentStore - Remove then re-add same entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);
    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});

    store.AddComponent(e, makeTransform(99.0f), true);
    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetTransform(e).Position.x == 99.0f);
}

TEST_CASE("TransformComponentStore - Stress: many adds, removes, SetActiveStatus calls", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    constexpr int N = 100;
    std::vector<Entity> entities;
    entities.reserve(N);

    // Add N entities: even indices active, odd indices inactive.
    for (int i = 0; i < N; i++) {
        Entity e = em.CreateEntity();
        entities.push_back(e);
        store.AddComponent(e, makeTransform(static_cast<float>(i)), i % 2 == 0);
    }

    // Verify dense packing after interleaved active/inactive adds.
    {
        std::vector<Entity> active, inactive;
        for (int i = 0; i < N; i++) {
            if (i % 2 == 0)
                active.push_back(entities[i]);
            else
                inactive.push_back(entities[i]);
        }
        requireDensePacking(store, active, inactive);
    }

    // Verify all values are accessible.
    for (int i = 0; i < N; i++) {
        REQUIRE(store.GetTransform(entities[i]).Position.x == static_cast<float>(i));
    }

    // SetActiveStatus true for all odd-indexed (inactive) entities.
    for (int i = 1; i < N; i += 2) {
        store.SetActiveStatus(entities[i], true);
    }
    requireDensePacking(store, entities, {});

    // SetActiveStatus false for the first half.
    for (int i = 0; i < N / 2; i++) {
        store.SetActiveStatus(entities[i], false);
    }
    {
        std::vector<Entity> active(entities.begin() + N / 2, entities.end());
        std::vector<Entity> inactive(entities.begin(), entities.begin() + N / 2);
        requireDensePacking(store, active, inactive);
    }

    // Remove all deactivated (first half).
    for (int i = 0; i < N / 2; i++) {
        store.RemoveComponent(entities[i]);
    }
    {
        std::vector<Entity> remaining(entities.begin() + N / 2, entities.end());
        requireDensePacking(store, remaining, {});
    }

    // The remaining entities should still have correct values.
    for (int i = N / 2; i < N; i++) {
        REQUIRE(store.GetTransform(entities[i]).Position.x == static_cast<float>(i));
    }
}

TEST_CASE("TransformComponentStore - Remove middle active with many inactive", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();
    Entity i3 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(a2, makeTransform(2.0f), true);
    store.AddComponent(a3, makeTransform(3.0f), true);
    store.AddComponent(i1, makeTransform(4.0f), false);
    store.AddComponent(i2, makeTransform(5.0f), false);
    store.AddComponent(i3, makeTransform(6.0f), false);

    // Remove the middle active element.
    store.RemoveComponent(a2);

    requireDensePacking(store, { a1, a3 }, { i1, i2, i3 });
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(store.GetTransform(i2).Position.x == 5.0f);
    REQUIRE(store.GetTransform(i3).Position.x == 6.0f);
}

TEST_CASE("TransformComponentStore - Remove middle inactive with many active", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();
    Entity i3 = em.CreateEntity();

    store.AddComponent(a1, makeTransform(1.0f), true);
    store.AddComponent(a2, makeTransform(2.0f), true);
    store.AddComponent(a3, makeTransform(3.0f), true);
    store.AddComponent(i1, makeTransform(4.0f), false);
    store.AddComponent(i2, makeTransform(5.0f), false);
    store.AddComponent(i3, makeTransform(6.0f), false);

    // Remove the middle inactive element.
    store.RemoveComponent(i2);

    requireDensePacking(store, { a1, a2, a3 }, { i1, i3 });
    REQUIRE(store.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(store.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(store.GetTransform(i3).Position.x == 6.0f);
}

// ===========================================================================================
// GetTransform mutability
// ===========================================================================================

TEST_CASE("TransformComponentStore - GetTransform returns mutable reference", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(0.0f), true);

    store.GetTransform(e).Position = glm::vec3(111.0f, 222.0f, 333.0f);

    auto &t = store.GetTransform(e);
    REQUIRE(t.Position.x == 111.0f);
    REQUIRE(t.Position.y == 222.0f);
    REQUIRE(t.Position.z == 333.0f);
    requireDensePacking(store, { e }, {});
}

// ===========================================================================================
// Edge: single element scenarios
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add and remove single active leaves empty", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);
    store.RemoveComponent(e);

    requireDensePacking(store, {}, {});
}

TEST_CASE("TransformComponentStore - SetActiveStatus false on single active then remove", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });

    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});
}

TEST_CASE("TransformComponentStore - SetActiveStatus true on single inactive then remove", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});

    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});
}

// ===========================================================================================
// Edge: remove last active when it is also the last element overall
// ===========================================================================================

TEST_CASE("TransformComponentStore - Remove last active that is also last element", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);

    // Remove e2 - it is the last active AND the last overall element.
    store.RemoveComponent(e2);

    requireDensePacking(store, { e1 }, {});
    REQUIRE(store.GetTransform(e1).Position.x == 1.0f);
}

// ===========================================================================================
// HasComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - HasComponent returns false for unknown entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();

    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("TransformComponentStore - HasComponent returns true for active entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    REQUIRE(store.HasComponent(e));
}

TEST_CASE("TransformComponentStore - HasComponent returns true for inactive entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);

    REQUIRE(store.HasComponent(e));
}

TEST_CASE("TransformComponentStore - HasComponent returns false after removal", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);
    store.RemoveComponent(e);

    REQUIRE_FALSE(store.HasComponent(e));
}

// ===========================================================================================
// IsDisabled
// ===========================================================================================

TEST_CASE("TransformComponentStore - IsDisabled returns false for active entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    REQUIRE_FALSE(store.IsDisabled(e));
}

TEST_CASE("TransformComponentStore - IsDisabled returns true for inactive entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);

    REQUIRE(store.IsDisabled(e));
}

TEST_CASE("TransformComponentStore - IsDisabled returns true after SetActiveStatus false", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    REQUIRE_FALSE(store.IsDisabled(e));

    store.SetActiveStatus(e, false);

    REQUIRE(store.IsDisabled(e));
}

TEST_CASE("TransformComponentStore - IsDisabled returns false after SetActiveStatus true", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), false);

    REQUIRE(store.IsDisabled(e));

    store.SetActiveStatus(e, true);

    REQUIRE_FALSE(store.IsDisabled(e));
}

// ===========================================================================================
// GetEntityIndex
// ===========================================================================================

TEST_CASE("TransformComponentStore - GetEntityIndex returns valid index for active entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeTransform(1.0f), true);

    size_t idx = store.GetEntityIndex(e);
    REQUIRE(idx < store.GetActiveComponentCount());
}

TEST_CASE("TransformComponentStore - GetEntityIndex returns index in inactive zone for inactive entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a = em.CreateEntity();
    Entity i = em.CreateEntity();
    store.AddComponent(a, makeTransform(1.0f), true);
    store.AddComponent(i, makeTransform(2.0f), false);

    size_t idx = store.GetEntityIndex(i);
    REQUIRE(idx >= store.GetActiveComponentCount());
    REQUIRE(idx < store.GetTotalComponentCount());
}

TEST_CASE("TransformComponentStore - GetEntityIndex updates after SetActiveStatus true", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a = em.CreateEntity();
    Entity i = em.CreateEntity();
    store.AddComponent(a, makeTransform(1.0f), true);
    store.AddComponent(i, makeTransform(2.0f), false);

    REQUIRE(store.GetEntityIndex(i) >= store.GetActiveComponentCount());

    store.SetActiveStatus(i, true);

    REQUIRE(store.GetEntityIndex(i) < store.GetActiveComponentCount());
}

TEST_CASE("TransformComponentStore - GetEntityIndex updates after SetActiveStatus false", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);

    REQUIRE(store.GetEntityIndex(e1) < store.GetActiveComponentCount());

    store.SetActiveStatus(e1, false);

    REQUIRE(store.GetEntityIndex(e1) >= store.GetActiveComponentCount());
}

// ===========================================================================================
// Empty store queries
// ===========================================================================================

TEST_CASE("TransformComponentStore - Empty store returns empty spans", "[ecs][transform]") {
    TransformComponentStore store;

    REQUIRE(store.GetActiveTransforms().empty());
    REQUIRE(store.GetActiveEntities().empty());
    REQUIRE(store.GetActiveComponentCount() == 0);
    REQUIRE(store.GetTotalComponentCount() == 0);
}

TEST_CASE("TransformComponentStore - All removed returns empty spans", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), false);

    store.RemoveComponent(e1);
    store.RemoveComponent(e2);

    REQUIRE(store.GetActiveTransforms().empty());
    REQUIRE(store.GetActiveEntities().empty());
    REQUIRE(store.GetActiveComponentCount() == 0);
    REQUIRE(store.GetTotalComponentCount() == 0);
}

// ===========================================================================================
// Additional edge cases
// ===========================================================================================

TEST_CASE("TransformComponentStore - Remove last inactive that is also last element", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a = em.CreateEntity();
    Entity i = em.CreateEntity();

    store.AddComponent(a, makeTransform(1.0f), true);
    store.AddComponent(i, makeTransform(2.0f), false);

    // Remove i - it is the last inactive AND the last overall element.
    store.RemoveComponent(i);

    requireDensePacking(store, { a }, {});
    REQUIRE(store.GetTransform(a).Position.x == 1.0f);
}

TEST_CASE("TransformComponentStore - Multiple SetActiveStatus cycles on same entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a = em.CreateEntity();
    Entity e = em.CreateEntity();

    store.AddComponent(a, makeTransform(1.0f), true);
    store.AddComponent(e, makeTransform(2.0f), false);

    for (int i = 0; i < 10; i++) {
        store.SetActiveStatus(e, true);
        requireDensePacking(store, { a, e }, {});
        REQUIRE(store.GetTransform(e).Position.x == 2.0f);

        store.SetActiveStatus(e, false);
        requireDensePacking(store, { a }, { e });
        REQUIRE(store.GetTransform(e).Position.x == 2.0f);
    }
}

TEST_CASE("TransformComponentStore - Re-add after remove into non-empty store", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeTransform(1.0f), true);
    store.AddComponent(e2, makeTransform(2.0f), true);
    store.AddComponent(e3, makeTransform(3.0f), false);

    store.RemoveComponent(e2);
    requireDensePacking(store, { e1 }, { e3 });

    // Re-add e2 as inactive this time.
    store.AddComponent(e2, makeTransform(20.0f), false);
    requireDensePacking(store, { e1 }, { e3, e2 });
    REQUIRE(store.GetTransform(e2).Position.x == 20.0f);
}

TEST_CASE("TransformComponentStore - Remove active when it is first active with inactive after", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore store;

    Entity a = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    store.AddComponent(a, makeTransform(1.0f), true);
    store.AddComponent(i1, makeTransform(2.0f), false);
    store.AddComponent(i2, makeTransform(3.0f), false);

    // Remove the only active entity; inactive entities should remain intact.
    store.RemoveComponent(a);

    requireDensePacking(store, {}, { i1, i2 });
    REQUIRE(store.GetTransform(i1).Position.x == 2.0f);
    REQUIRE(store.GetTransform(i2).Position.x == 3.0f);
}
