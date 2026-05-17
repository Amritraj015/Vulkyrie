#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>
#include "physics/body/body.h"

#include <unordered_set>

using namespace Vulkyrie;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Creates a BodyComponent wrapping the given (potentially null) Body pointer.
// Use nullptr for tests that never call GetBody; use a real Body* for pointer-
// identity tests (see the "GetBody" section below).
static BodyComponent makeBodyComp(Body *body) {
    return BodyComponent{ body };
}

// Verifies the dense-packing invariant: active components occupy [0, activeCount)
// and inactive components occupy [activeCount, totalCount).
static void requireDensePacking(BodyComponentStore &store, const std::vector<Entity> &expectedActive, const std::vector<Entity> &expectedInactive) {
    REQUIRE(store.GetActiveComponentCount() == expectedActive.size());
    REQUIRE(store.GetTotalComponentCount() == expectedActive.size() + expectedInactive.size());

    auto activeEntities = store.GetActiveEntities();
    REQUIRE(activeEntities.size() == expectedActive.size());

    std::unordered_set<Entity> activeSet(activeEntities.begin(), activeEntities.end());
    for (const auto &e : expectedActive) {
        REQUIRE(activeSet.contains(e));
    }
    for (const auto &e : expectedInactive) {
        REQUIRE_FALSE(activeSet.contains(e));
    }
}

// ===========================================================================================
// AddComponent
// ===========================================================================================

TEST_CASE("BodyComponentStore - Add single active component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.HasComponent(e));
}

TEST_CASE("BodyComponentStore - Add single inactive component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.HasComponent(e));
}

TEST_CASE("BodyComponentStore - Add multiple active components", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), true);
    store.AddComponent(e2, makeBodyComp(nullptr), true);
    store.AddComponent(e3, makeBodyComp(nullptr), true);

    requireDensePacking(store, { e1, e2, e3 }, {});
}

TEST_CASE("BodyComponentStore - Add multiple inactive components", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), false);
    store.AddComponent(e2, makeBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e1, e2 });
}

TEST_CASE("BodyComponentStore - Add active after inactive components triggers swap into active zone", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity inactive1 = em.CreateEntity();
    Entity inactive2 = em.CreateEntity();
    Entity active1 = em.CreateEntity();

    store.AddComponent(inactive1, makeBodyComp(nullptr), false);
    store.AddComponent(inactive2, makeBodyComp(nullptr), false);
    store.AddComponent(active1, makeBodyComp(nullptr), true);

    requireDensePacking(store, { active1 }, { inactive1, inactive2 });
}

TEST_CASE("BodyComponentStore - Interleaved active and inactive additions", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity e4 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), true);
    store.AddComponent(e2, makeBodyComp(nullptr), false);
    store.AddComponent(e3, makeBodyComp(nullptr), true);
    store.AddComponent(e4, makeBodyComp(nullptr), false);

    requireDensePacking(store, { e1, e3 }, { e2, e4 });
}

// ===========================================================================================
// GetBody - pointer identity
// Uses a real PhysicsWorld and Body objects to avoid undefined behaviour.
// ===========================================================================================

TEST_CASE("BodyComponentStore - GetBody returns the correct body pointer", "[ecs][body]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<Body> b1(new Body(bodyEm.CreateEntity(), world));
    std::unique_ptr<Body> b2(new Body(bodyEm.CreateEntity(), world));

    EntityManager em;
    BodyComponentStore store;
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(b1.get()), true);
    store.AddComponent(e2, makeBodyComp(b2.get()), true);

    REQUIRE(&store.GetBody(e1) == b1.get());
    REQUIRE(&store.GetBody(e2) == b2.get());
}

TEST_CASE("BodyComponentStore - GetBody returns correct pointer after active-zone swap on add", "[ecs][body]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<Body> b1(new Body(bodyEm.CreateEntity(), world));
    std::unique_ptr<Body> b2(new Body(bodyEm.CreateEntity(), world));

    EntityManager em;
    BodyComponentStore store;
    Entity inactive = em.CreateEntity();
    Entity active = em.CreateEntity();

    // Inactive goes in first; adding an active component afterwards triggers a swap.
    store.AddComponent(inactive, makeBodyComp(b1.get()), false);
    store.AddComponent(active, makeBodyComp(b2.get()), true);

    REQUIRE(&store.GetBody(inactive) == b1.get());
    REQUIRE(&store.GetBody(active) == b2.get());
}

TEST_CASE("BodyComponentStore - GetBody returns correct pointer after SetActiveStatus swap", "[ecs][body]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<Body> b1(new Body(bodyEm.CreateEntity(), world));
    std::unique_ptr<Body> b2(new Body(bodyEm.CreateEntity(), world));

    EntityManager em;
    BodyComponentStore store;
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(b1.get()), false);
    store.AddComponent(e2, makeBodyComp(b2.get()), false);

    // Activating e1 swaps it into the active zone, moving e2's data.
    store.SetActiveStatus(e1, true);

    REQUIRE(&store.GetBody(e1) == b1.get());
    REQUIRE(&store.GetBody(e2) == b2.get());
}

// ===========================================================================================
// Default flag values
// ===========================================================================================

TEST_CASE("BodyComponentStore - _bodyActiveFlags is true by default for active ECS component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    REQUIRE(store.IsBodyActive(e));
}

TEST_CASE("BodyComponentStore - _bodyActiveFlags is false when added as inactive ECS component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);

    // Both the ECS active status and the physics-simulation body-active flag mirror the active param.
    REQUIRE_FALSE(store.IsBodyActive(e));
    REQUIRE(store.IsDisabled(e));
}

TEST_CASE("BodyComponentStore - _simulationColliderFlags is false by default", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    REQUIRE_FALSE(store.HasSimulationColliders(e));
}

// ===========================================================================================
// IsBodyActive / SetBodyActive
// ===========================================================================================

TEST_CASE("BodyComponentStore - SetBodyActive to false", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    store.SetBodyActive(e, false);

    REQUIRE_FALSE(store.IsBodyActive(e));
}

TEST_CASE("BodyComponentStore - SetBodyActive toggle", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    store.SetBodyActive(e, false);
    REQUIRE_FALSE(store.IsBodyActive(e));

    store.SetBodyActive(e, true);
    REQUIRE(store.IsBodyActive(e));
}

TEST_CASE("BodyComponentStore - SetBodyActive is independent of ECS active status", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false); // ECS inactive

    store.SetBodyActive(e, false);
    REQUIRE_FALSE(store.IsBodyActive(e));
    REQUIRE(store.IsDisabled(e)); // ECS still inactive

    store.SetBodyActive(e, true);
    REQUIRE(store.IsBodyActive(e));
    REQUIRE(store.IsDisabled(e)); // ECS still inactive
}

// ===========================================================================================
// HasSimulationColliders / SetHasSimulationColliders
// ===========================================================================================

TEST_CASE("BodyComponentStore - SetHasSimulationColliders to true", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    store.SetHasSimulationColliders(e, true);

    REQUIRE(store.HasSimulationColliders(e));
}

TEST_CASE("BodyComponentStore - SetHasSimulationColliders toggle", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    store.SetHasSimulationColliders(e, true);
    REQUIRE(store.HasSimulationColliders(e));

    store.SetHasSimulationColliders(e, false);
    REQUIRE_FALSE(store.HasSimulationColliders(e));
}

// ===========================================================================================
// AddColliderToBody / GetColliders
// ===========================================================================================

TEST_CASE("BodyComponentStore - GetColliders is empty after AddComponent", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    store.AddComponent(body, makeBodyComp(nullptr), true);

    REQUIRE(store.GetColliders(body).empty());
}

TEST_CASE("BodyComponentStore - AddColliderToBody adds a collider", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity collider = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, collider);

    const auto &colliders = store.GetColliders(body);
    REQUIRE(colliders.size() == 1);
    REQUIRE(colliders[0] == collider);
}

TEST_CASE("BodyComponentStore - AddColliderToBody adds multiple colliders", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();
    Entity c3 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.AddColliderToBody(body, c2);
    store.AddColliderToBody(body, c3);

    const auto &colliders = store.GetColliders(body);
    REQUIRE(colliders.size() == 3);

    std::unordered_set<Entity> colliderSet(colliders.begin(), colliders.end());
    REQUIRE(colliderSet.contains(c1));
    REQUIRE(colliderSet.contains(c2));
    REQUIRE(colliderSet.contains(c3));
}

TEST_CASE("BodyComponentStore - Collider lists are independent per body", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body1 = em.CreateEntity();
    Entity body2 = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();

    store.AddComponent(body1, makeBodyComp(nullptr), true);
    store.AddComponent(body2, makeBodyComp(nullptr), true);

    store.AddColliderToBody(body1, c1);
    store.AddColliderToBody(body2, c2);

    REQUIRE(store.GetColliders(body1).size() == 1);
    REQUIRE(store.GetColliders(body2).size() == 1);
    REQUIRE(store.GetColliders(body1)[0] == c1);
    REQUIRE(store.GetColliders(body2)[0] == c2);
}

// ===========================================================================================
// RemoveColliderFromBody
// ===========================================================================================

TEST_CASE("BodyComponentStore - RemoveColliderFromBody removes the only collider", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.RemoveColliderFromBody(body, c1);

    REQUIRE(store.GetColliders(body).empty());
}

TEST_CASE("BodyComponentStore - RemoveColliderFromBody removes middle collider (swap-erase)", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();
    Entity c3 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.AddColliderToBody(body, c2);
    store.AddColliderToBody(body, c3);

    store.RemoveColliderFromBody(body, c2);

    const auto &colliders = store.GetColliders(body);
    REQUIRE(colliders.size() == 2);

    std::unordered_set<Entity> colliderSet(colliders.begin(), colliders.end());
    REQUIRE(colliderSet.contains(c1));
    REQUIRE_FALSE(colliderSet.contains(c2));
    REQUIRE(colliderSet.contains(c3));
}

TEST_CASE("BodyComponentStore - RemoveColliderFromBody removes last collider in list (no swap needed)", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.AddColliderToBody(body, c2);
    store.RemoveColliderFromBody(body, c2);

    const auto &colliders = store.GetColliders(body);
    REQUIRE(colliders.size() == 1);
    REQUIRE(colliders[0] == c1);
}

TEST_CASE("BodyComponentStore - RemoveColliderFromBody removes first collider (swap-erase with last)", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.AddColliderToBody(body, c2);
    store.RemoveColliderFromBody(body, c1); // c2 swaps into index 0

    const auto &colliders = store.GetColliders(body);
    REQUIRE(colliders.size() == 1);
    REQUIRE(colliders[0] == c2);
}

// ===========================================================================================
// RemoveComponent
// ===========================================================================================

TEST_CASE("BodyComponentStore - Remove only active component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);
    store.RemoveComponent(e);

    requireDensePacking(store, {}, {});
    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("BodyComponentStore - Remove only inactive component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);
    store.RemoveComponent(e);

    requireDensePacking(store, {}, {});
    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("BodyComponentStore - Remove active component preserves other active components", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), true);
    store.AddComponent(e2, makeBodyComp(nullptr), true);
    store.AddComponent(e3, makeBodyComp(nullptr), true);

    store.RemoveComponent(e1);

    requireDensePacking(store, { e2, e3 }, {});
    REQUIRE_FALSE(store.HasComponent(e1));
    REQUIRE(store.HasComponent(e2));
    REQUIRE(store.HasComponent(e3));
}

TEST_CASE("BodyComponentStore - Remove active component with inactive present", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeBodyComp(nullptr), true);
    store.AddComponent(a2, makeBodyComp(nullptr), true);
    store.AddComponent(i1, makeBodyComp(nullptr), false);

    store.RemoveComponent(a1);

    requireDensePacking(store, { a2 }, { i1 });
}

TEST_CASE("BodyComponentStore - Remove inactive component with active present", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    store.AddComponent(a1, makeBodyComp(nullptr), true);
    store.AddComponent(i1, makeBodyComp(nullptr), false);
    store.AddComponent(i2, makeBodyComp(nullptr), false);

    store.RemoveComponent(i1);

    requireDensePacking(store, { a1 }, { i2 });
}

TEST_CASE("BodyComponentStore - Collider list is removed with its component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), true);
    store.AddColliderToBody(body, c1);
    store.AddColliderToBody(body, c2);

    store.RemoveComponent(body);

    requireDensePacking(store, {}, {});
    REQUIRE_FALSE(store.HasComponent(body));
}

TEST_CASE("BodyComponentStore - Remove all components one by one", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), true);
    store.AddComponent(e2, makeBodyComp(nullptr), true);
    store.AddComponent(e3, makeBodyComp(nullptr), false);

    store.RemoveComponent(e2);
    requireDensePacking(store, { e1 }, { e3 });

    store.RemoveComponent(e1);
    requireDensePacking(store, {}, { e3 });

    store.RemoveComponent(e3);
    requireDensePacking(store, {}, {});
}

// ===========================================================================================
// SetActiveStatus
// ===========================================================================================

TEST_CASE("BodyComponentStore - SetActiveStatus true on inactive component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);

    requireDensePacking(store, { e }, {});
}

TEST_CASE("BodyComponentStore - SetActiveStatus true on already active is a no-op", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    store.SetActiveStatus(e, true);

    requireDensePacking(store, { e }, {});
}

TEST_CASE("BodyComponentStore - SetActiveStatus false on active component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    requireDensePacking(store, { e }, {});

    store.SetActiveStatus(e, false);

    requireDensePacking(store, {}, { e });
}

TEST_CASE("BodyComponentStore - SetActiveStatus false on already inactive is a no-op", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);

    store.SetActiveStatus(e, false);

    requireDensePacking(store, {}, { e });
}

TEST_CASE("BodyComponentStore - SetActiveStatus true on one of several inactive components", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), false);
    store.AddComponent(e2, makeBodyComp(nullptr), false);
    store.AddComponent(e3, makeBodyComp(nullptr), false);

    store.SetActiveStatus(e2, true);

    requireDensePacking(store, { e2 }, { e1, e3 });
}

TEST_CASE("BodyComponentStore - SetActiveStatus preserves body flags and colliders through swap", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity c1 = em.CreateEntity();

    store.AddComponent(body, makeBodyComp(nullptr), false);
    store.AddColliderToBody(body, c1);
    store.SetBodyActive(body, false);
    store.SetHasSimulationColliders(body, true);

    store.SetActiveStatus(body, true);

    requireDensePacking(store, { body }, {});
    REQUIRE(store.GetColliders(body).size() == 1);
    REQUIRE(store.GetColliders(body)[0] == c1);
    REQUIRE_FALSE(store.IsBodyActive(body));
    REQUIRE(store.HasSimulationColliders(body));
}

// ===========================================================================================
// IsDisabled
// ===========================================================================================

TEST_CASE("BodyComponentStore - IsDisabled returns false for active component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    REQUIRE_FALSE(store.IsDisabled(e));
}

TEST_CASE("BodyComponentStore - IsDisabled returns true for inactive component", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), false);

    REQUIRE(store.IsDisabled(e));
}

TEST_CASE("BodyComponentStore - IsDisabled reflects ECS active status after SetActiveStatus", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeBodyComp(nullptr), true);

    REQUIRE_FALSE(store.IsDisabled(e));

    store.SetActiveStatus(e, false);
    REQUIRE(store.IsDisabled(e));

    store.SetActiveStatus(e, true);
    REQUIRE_FALSE(store.IsDisabled(e));
}

// ===========================================================================================
// Parallel-array swap integrity
// Tests that all parallel arrays (_bodies, _colliders, _bodyActiveFlags,
// _simulationColliderFlags) stay in sync with _entities through swapComponents.
// ===========================================================================================

TEST_CASE("BodyComponentStore - All parallel arrays stay in sync after add-triggered swap", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity inactive = em.CreateEntity();
    Entity active = em.CreateEntity();
    Entity c1 = em.CreateEntity();

    // Populate the inactive entity with distinct flag values and a collider.
    store.AddComponent(inactive, makeBodyComp(nullptr), false);
    store.AddColliderToBody(inactive, c1);
    store.SetBodyActive(inactive, false);
    store.SetHasSimulationColliders(inactive, true);

    // Adding an active component after an inactive one triggers swapComponents.
    store.AddComponent(active, makeBodyComp(nullptr), true);

    requireDensePacking(store, { active }, { inactive });

    // Inactive entity's data should be unchanged despite the swap.
    REQUIRE(store.GetColliders(inactive).size() == 1);
    REQUIRE(store.GetColliders(inactive)[0] == c1);
    REQUIRE_FALSE(store.IsBodyActive(inactive));
    REQUIRE(store.HasSimulationColliders(inactive));

    // Active entity's data should have default values.
    REQUIRE(store.GetColliders(active).empty());
    REQUIRE(store.IsBodyActive(active));
    REQUIRE_FALSE(store.HasSimulationColliders(active));
}

TEST_CASE("BodyComponentStore - All parallel arrays stay in sync after SetActiveStatus swap", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity c1 = em.CreateEntity();
    Entity c2 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), false);
    store.AddComponent(e2, makeBodyComp(nullptr), false);

    store.AddColliderToBody(e1, c1);
    store.SetBodyActive(e1, false);
    store.SetHasSimulationColliders(e1, true);

    store.AddColliderToBody(e2, c2);
    store.SetBodyActive(e2, true); // default is true anyway, just being explicit
    store.SetHasSimulationColliders(e2, false);

    store.SetActiveStatus(e1, true);

    requireDensePacking(store, { e1 }, { e2 });

    // e1 data after being swapped into active zone.
    REQUIRE(store.GetColliders(e1).size() == 1);
    REQUIRE(store.GetColliders(e1)[0] == c1);
    REQUIRE_FALSE(store.IsBodyActive(e1));
    REQUIRE(store.HasSimulationColliders(e1));

    // e2 data after being displaced.
    REQUIRE(store.GetColliders(e2).size() == 1);
    REQUIRE(store.GetColliders(e2)[0] == c2);
    REQUIRE(store.IsBodyActive(e2));
    REQUIRE_FALSE(store.HasSimulationColliders(e2));
}

TEST_CASE("BodyComponentStore - All parallel arrays stay in sync after RemoveComponent swap", "[ecs][body]") {
    EntityManager em;
    BodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity c1 = em.CreateEntity();

    store.AddComponent(e1, makeBodyComp(nullptr), true);
    store.AddComponent(e2, makeBodyComp(nullptr), true);
    store.AddComponent(e3, makeBodyComp(nullptr), true);

    store.AddColliderToBody(e3, c1);
    store.SetBodyActive(e3, false);
    store.SetHasSimulationColliders(e3, true);

    // Removing e1 (active) will trigger swaps with e2 and e3 to close the gap.
    store.RemoveComponent(e1);

    requireDensePacking(store, { e2, e3 }, {});

    // e3's data should remain intact after being shuffled around.
    REQUIRE(store.GetColliders(e3).size() == 1);
    REQUIRE(store.GetColliders(e3)[0] == c1);
    REQUIRE_FALSE(store.IsBodyActive(e3));
    REQUIRE(store.HasSimulationColliders(e3));
}
