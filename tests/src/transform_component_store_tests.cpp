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
static void requireDensePacking(TransformComponentStore &mgr, const std::vector<Entity> &expectedActive, const std::vector<Entity> &expectedInactive) {
    REQUIRE(mgr.GetActiveComponentCount() == expectedActive.size());
    REQUIRE(mgr.GetTotalComponentCount() == expectedActive.size() + expectedInactive.size());

    auto activeEntities = mgr.GetActiveEntities();
    auto activeTransforms = mgr.GetActiveTransforms();
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
        REQUIRE(&mgr.GetTransform(activeEntities[i]) == &activeTransforms[i]);
    }
}

// ===========================================================================================
// AddComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add single active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);

    requireDensePacking(mgr, { e }, {});
    REQUIRE(mgr.GetTransform(e).Position.x == 1.0f);
}

TEST_CASE("TransformComponentStore - Add single inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(2.0f), false);

    requireDensePacking(mgr, {}, { e });
    REQUIRE(mgr.GetTransform(e).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add multiple active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), true);

    requireDensePacking(mgr, { e1, e2, e3 }, {});
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Add multiple inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), false);
    mgr.AddComponent(e2, makeTransform(2.0f), false);

    requireDensePacking(mgr, {}, { e1, e2 });
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add active after inactive preserves partition", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity inactive1 = em.CreateEntity();
    Entity inactive2 = em.CreateEntity();
    Entity active1 = em.CreateEntity();

    mgr.AddComponent(inactive1, makeTransform(10.0f), false);
    mgr.AddComponent(inactive2, makeTransform(20.0f), false);
    mgr.AddComponent(active1, makeTransform(30.0f), true);

    requireDensePacking(mgr, { active1 }, { inactive1, inactive2 });
    REQUIRE(mgr.GetTransform(inactive1).Position.x == 10.0f);
    REQUIRE(mgr.GetTransform(inactive2).Position.x == 20.0f);
    REQUIRE(mgr.GetTransform(active1).Position.x == 30.0f);
}

TEST_CASE("TransformComponentStore - Interleaved active and inactive additions", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity e4 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), false);
    mgr.AddComponent(e3, makeTransform(3.0f), true);
    mgr.AddComponent(e4, makeTransform(4.0f), false);

    requireDensePacking(mgr, { e1, e3 }, { e2, e4 });
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
    REQUIRE(mgr.GetTransform(e4).Position.x == 4.0f);
}

// ===========================================================================================
// SetComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - SetComponent updates values", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);

    mgr.SetTransform(e, makeTransform(99.0f, 88.0f, 77.0f));
    auto &t = mgr.GetTransform(e);

    REQUIRE(t.Position.x == 99.0f);
    REQUIRE(t.Position.y == 88.0f);
    REQUIRE(t.Position.z == 77.0f);
    requireDensePacking(mgr, { e }, {});
}

TEST_CASE("TransformComponentStore - SetComponent on inactive entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), false);

    mgr.SetTransform(e, makeTransform(42.0f));
    REQUIRE(mgr.GetTransform(e).Position.x == 42.0f);
    requireDensePacking(mgr, {}, { e });
}

// ===========================================================================================
// RemoveComponent
// ===========================================================================================

TEST_CASE("TransformComponentStore - Remove only active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);
    mgr.RemoveComponent(e);

    requireDensePacking(mgr, {}, {});
}

TEST_CASE("TransformComponentStore - Remove only inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), false);
    mgr.RemoveComponent(e);

    requireDensePacking(mgr, {}, {});
}

TEST_CASE("TransformComponentStore - Remove active component preserves other active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), true);

    mgr.RemoveComponent(e1);

    requireDensePacking(mgr, { e2, e3 }, {});
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove active component with inactive present", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(a2, makeTransform(2.0f), true);
    mgr.AddComponent(i1, makeTransform(3.0f), false);

    mgr.RemoveComponent(a1);

    requireDensePacking(mgr, { a2 }, { i1 });
    REQUIRE(mgr.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove inactive component with active present", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(i1, makeTransform(2.0f), false);
    mgr.AddComponent(i2, makeTransform(3.0f), false);

    mgr.RemoveComponent(i1);

    requireDensePacking(mgr, { a1 }, { i2 });
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(i2).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Remove first active among many active and inactive", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(a2, makeTransform(2.0f), true);
    mgr.AddComponent(a3, makeTransform(3.0f), true);
    mgr.AddComponent(i1, makeTransform(4.0f), false);
    mgr.AddComponent(i2, makeTransform(5.0f), false);

    mgr.RemoveComponent(a1);

    requireDensePacking(mgr, { a2, a3 }, { i1, i2 });
    REQUIRE(mgr.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(mgr.GetTransform(i2).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Remove all components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), false);

    mgr.RemoveComponent(e2);
    requireDensePacking(mgr, { e1 }, { e3 });
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);

    mgr.RemoveComponent(e1);
    requireDensePacking(mgr, {}, { e3 });
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);

    mgr.RemoveComponent(e3);
    requireDensePacking(mgr, {}, {});
}

// ===========================================================================================
// Activate
// ===========================================================================================

TEST_CASE("TransformComponentStore - Activate inactive component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(5.0f), false);

    requireDensePacking(mgr, {}, { e });

    mgr.Activate(e);

    requireDensePacking(mgr, { e }, {});
    REQUIRE(mgr.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Activate already active component is no-op", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(5.0f), true);

    mgr.Activate(e);

    requireDensePacking(mgr, { e }, {});
    REQUIRE(mgr.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Activate one of several inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), false);
    mgr.AddComponent(e2, makeTransform(2.0f), false);
    mgr.AddComponent(e3, makeTransform(3.0f), false);

    mgr.Activate(e2);

    requireDensePacking(mgr, { e2 }, { e1, e3 });
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Activate with existing active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(i1, makeTransform(2.0f), false);

    mgr.Activate(i1);

    requireDensePacking(mgr, { a1, i1 }, {});
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 2.0f);
}

// ===========================================================================================
// Deactivate
// ===========================================================================================

TEST_CASE("TransformComponentStore - Deactivate active component", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(5.0f), true);

    requireDensePacking(mgr, { e }, {});

    mgr.Deactivate(e);

    requireDensePacking(mgr, {}, { e });
    REQUIRE(mgr.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Deactivate already inactive component is no-op", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(5.0f), false);

    mgr.Deactivate(e);

    requireDensePacking(mgr, {}, { e });
    REQUIRE(mgr.GetTransform(e).Position.x == 5.0f);
}

TEST_CASE("TransformComponentStore - Deactivate one of several active components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), true);

    mgr.Deactivate(e2);

    requireDensePacking(mgr, { e1, e3 }, { e2 });
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Deactivate preserves inactive components", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(a2, makeTransform(2.0f), true);
    mgr.AddComponent(i1, makeTransform(3.0f), false);

    mgr.Deactivate(a1);

    requireDensePacking(mgr, { a2 }, { a1, i1 });
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 3.0f);
}

// ===========================================================================================
// Activate / Deactivate round-trips
// ===========================================================================================

TEST_CASE("TransformComponentStore - Activate then deactivate returns to original state", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(i1, makeTransform(2.0f), false);

    mgr.Activate(i1);
    requireDensePacking(mgr, { a1, i1 }, {});

    mgr.Deactivate(i1);
    requireDensePacking(mgr, { a1 }, { i1 });
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Deactivate then activate returns to original state", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(7.0f), true);

    mgr.Deactivate(e);
    requireDensePacking(mgr, {}, { e });

    mgr.Activate(e);
    requireDensePacking(mgr, { e }, {});
    REQUIRE(mgr.GetTransform(e).Position.x == 7.0f);
}

TEST_CASE("TransformComponentStore - Activate all inactive components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), false);
    mgr.AddComponent(e2, makeTransform(2.0f), false);
    mgr.AddComponent(e3, makeTransform(3.0f), false);

    mgr.Activate(e3);
    requireDensePacking(mgr, { e3 }, { e1, e2 });

    mgr.Activate(e1);
    requireDensePacking(mgr, { e3, e1 }, { e2 });

    mgr.Activate(e2);
    requireDensePacking(mgr, { e3, e1, e2 }, {});

    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

TEST_CASE("TransformComponentStore - Deactivate all active components one by one", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), true);

    mgr.Deactivate(e2);
    requireDensePacking(mgr, { e1, e3 }, { e2 });

    mgr.Deactivate(e1);
    requireDensePacking(mgr, { e3 }, { e1, e2 });

    mgr.Deactivate(e3);
    requireDensePacking(mgr, {}, { e1, e2, e3 });

    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
}

// ===========================================================================================
// Combined operations (add, remove, activate, deactivate, set)
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add, deactivate, remove active", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);
    mgr.AddComponent(e3, makeTransform(3.0f), true);

    mgr.Deactivate(e2);
    requireDensePacking(mgr, { e1, e3 }, { e2 });

    mgr.RemoveComponent(e1);
    requireDensePacking(mgr, { e3 }, { e2 });
    REQUIRE(mgr.GetTransform(e3).Position.x == 3.0f);
    REQUIRE(mgr.GetTransform(e2).Position.x == 2.0f);
}

TEST_CASE("TransformComponentStore - Add inactive, activate, set, deactivate", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(0.0f), false);
    requireDensePacking(mgr, {}, { e });

    mgr.Activate(e);
    requireDensePacking(mgr, { e }, {});

    mgr.SetTransform(e, makeTransform(50.0f));
    REQUIRE(mgr.GetTransform(e).Position.x == 50.0f);
    requireDensePacking(mgr, { e }, {});

    mgr.Deactivate(e);
    requireDensePacking(mgr, {}, { e });
    REQUIRE(mgr.GetTransform(e).Position.x == 50.0f);
}

TEST_CASE("TransformComponentStore - Remove then re-add same entity", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);
    mgr.RemoveComponent(e);
    requireDensePacking(mgr, {}, {});

    mgr.AddComponent(e, makeTransform(99.0f), true);
    requireDensePacking(mgr, { e }, {});
    REQUIRE(mgr.GetTransform(e).Position.x == 99.0f);
}

TEST_CASE("TransformComponentStore - Stress: many adds, removes, activates, deactivates", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    constexpr int N = 100;
    std::vector<Entity> entities;
    entities.reserve(N);

    // Add N entities: even indices active, odd indices inactive.
    for (int i = 0; i < N; i++) {
        Entity e = em.CreateEntity();
        entities.push_back(e);
        mgr.AddComponent(e, makeTransform(static_cast<float>(i)), i % 2 == 0);
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
        requireDensePacking(mgr, active, inactive);
    }

    // Verify all values are accessible.
    for (int i = 0; i < N; i++) {
        REQUIRE(mgr.GetTransform(entities[i]).Position.x == static_cast<float>(i));
    }

    // Activate all odd-indexed (inactive) entities.
    for (int i = 1; i < N; i += 2) {
        mgr.Activate(entities[i]);
    }
    requireDensePacking(mgr, entities, {});

    // Deactivate the first half.
    for (int i = 0; i < N / 2; i++) {
        mgr.Deactivate(entities[i]);
    }
    {
        std::vector<Entity> active(entities.begin() + N / 2, entities.end());
        std::vector<Entity> inactive(entities.begin(), entities.begin() + N / 2);
        requireDensePacking(mgr, active, inactive);
    }

    // Remove all deactivated (first half).
    for (int i = 0; i < N / 2; i++) {
        mgr.RemoveComponent(entities[i]);
    }
    {
        std::vector<Entity> remaining(entities.begin() + N / 2, entities.end());
        requireDensePacking(mgr, remaining, {});
    }

    // The remaining entities should still have correct values.
    for (int i = N / 2; i < N; i++) {
        REQUIRE(mgr.GetTransform(entities[i]).Position.x == static_cast<float>(i));
    }
}

TEST_CASE("TransformComponentStore - Remove middle active with many inactive", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();
    Entity i3 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(a2, makeTransform(2.0f), true);
    mgr.AddComponent(a3, makeTransform(3.0f), true);
    mgr.AddComponent(i1, makeTransform(4.0f), false);
    mgr.AddComponent(i2, makeTransform(5.0f), false);
    mgr.AddComponent(i3, makeTransform(6.0f), false);

    // Remove the middle active element.
    mgr.RemoveComponent(a2);

    requireDensePacking(mgr, { a1, a3 }, { i1, i2, i3 });
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(mgr.GetTransform(i2).Position.x == 5.0f);
    REQUIRE(mgr.GetTransform(i3).Position.x == 6.0f);
}

TEST_CASE("TransformComponentStore - Remove middle inactive with many active", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity a3 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();
    Entity i3 = em.CreateEntity();

    mgr.AddComponent(a1, makeTransform(1.0f), true);
    mgr.AddComponent(a2, makeTransform(2.0f), true);
    mgr.AddComponent(a3, makeTransform(3.0f), true);
    mgr.AddComponent(i1, makeTransform(4.0f), false);
    mgr.AddComponent(i2, makeTransform(5.0f), false);
    mgr.AddComponent(i3, makeTransform(6.0f), false);

    // Remove the middle inactive element.
    mgr.RemoveComponent(i2);

    requireDensePacking(mgr, { a1, a2, a3 }, { i1, i3 });
    REQUIRE(mgr.GetTransform(a1).Position.x == 1.0f);
    REQUIRE(mgr.GetTransform(a2).Position.x == 2.0f);
    REQUIRE(mgr.GetTransform(a3).Position.x == 3.0f);
    REQUIRE(mgr.GetTransform(i1).Position.x == 4.0f);
    REQUIRE(mgr.GetTransform(i3).Position.x == 6.0f);
}

// ===========================================================================================
// GetTransform mutability
// ===========================================================================================

TEST_CASE("TransformComponentStore - GetTransform returns mutable reference", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(0.0f), true);

    mgr.GetTransform(e).Position = glm::vec3(111.0f, 222.0f, 333.0f);

    auto &t = mgr.GetTransform(e);
    REQUIRE(t.Position.x == 111.0f);
    REQUIRE(t.Position.y == 222.0f);
    REQUIRE(t.Position.z == 333.0f);
    requireDensePacking(mgr, { e }, {});
}

// ===========================================================================================
// Edge: single element scenarios
// ===========================================================================================

TEST_CASE("TransformComponentStore - Add and remove single active leaves empty", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);
    mgr.RemoveComponent(e);

    requireDensePacking(mgr, {}, {});
}

TEST_CASE("TransformComponentStore - Deactivate single active then remove", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), true);

    mgr.Deactivate(e);
    requireDensePacking(mgr, {}, { e });

    mgr.RemoveComponent(e);
    requireDensePacking(mgr, {}, {});
}

TEST_CASE("TransformComponentStore - Activate single inactive then remove", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e = em.CreateEntity();
    mgr.AddComponent(e, makeTransform(1.0f), false);

    mgr.Activate(e);
    requireDensePacking(mgr, { e }, {});

    mgr.RemoveComponent(e);
    requireDensePacking(mgr, {}, {});
}

// ===========================================================================================
// Edge: remove last active when it is also the last element overall
// ===========================================================================================

TEST_CASE("TransformComponentStore - Remove last active that is also last element", "[ecs][transform]") {
    EntityManager em;
    TransformComponentStore mgr;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    mgr.AddComponent(e1, makeTransform(1.0f), true);
    mgr.AddComponent(e2, makeTransform(2.0f), true);

    // Remove e2 - it is the last active AND the last overall element.
    mgr.RemoveComponent(e2);

    requireDensePacking(mgr, { e1 }, {});
    REQUIRE(mgr.GetTransform(e1).Position.x == 1.0f);
}
