#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <unordered_set>
#include <vulkyrie.h>

using namespace Vulkyrie;

// ===========================================================================================
// Helpers
// ===========================================================================================

static TransformComponent makeTransform(float x, float y = 0.0f, float z = 0.0f) {
    TransformComponent t{};
    t.Position = glm::vec3(x, y, z);
    t.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return t;
}

static Material makeMaterial(float friction = 0.5f, float restitution = 0.3f, float density = 1.0f) {
    return Material(friction, restitution, density);
}

// Constructs a minimal ColliderComponent. The store stores Collider* and CollisionShape*
// as raw pointers and never dereferences them internally, so nullptr is safe for store tests.
static ColliderComponent makeComponent(Entity bodyEntity,
                                       const TransformComponent &localToBody = makeTransform(0.0f),
                                       const TransformComponent &localToWorld = makeTransform(0.0f),
                                       u16 categoryBits = 0x0001,
                                       u16 maskBits = 0xFFFF) {
    return ColliderComponent(bodyEntity,
                             nullptr, // Collider* — not dereferenced by the store
                             localToBody,
                             nullptr, // CollisionShape* — not dereferenced by the store
                             categoryBits,
                             maskBits,
                             localToWorld,
                             makeMaterial());
}

// Verifies the dense packing invariant: active components occupy [0, activeCount)
// and the span views are aligned with per-entity getters.
static void requireDensePacking(ColliderComponentStore &store, const std::vector<Entity> &expectedActive, const std::vector<Entity> &expectedInactive) {
    REQUIRE(store.GetActiveComponentCount() == expectedActive.size());
    REQUIRE(store.GetTotalComponentCount() == expectedActive.size() + expectedInactive.size());

    auto activeEntities = store.GetActiveEntities();
    auto activeTransforms = store.GetActiveLocalToWorldTransforms();

    REQUIRE(activeEntities.size() == expectedActive.size());
    REQUIRE(activeTransforms.size() == expectedActive.size());

    std::unordered_set<Entity> activeSet(activeEntities.begin(), activeEntities.end());
    for (const auto &e : expectedActive) {
        REQUIRE(activeSet.contains(e));
    }
    for (const auto &e : expectedInactive) {
        REQUIRE_FALSE(activeSet.contains(e));
        REQUIRE(store.IsDisabled(e));
    }

    // Verify each active entity's local-to-world transform aligns with the span.
    for (size_t i = 0; i < activeEntities.size(); ++i) {
        REQUIRE(store.GetLocalToWorldTransform(activeEntities[i]).Position == activeTransforms[i].Position);
    }
}

// ===========================================================================================
// AddComponent
// ===========================================================================================

TEST_CASE("ColliderComponentStore - Add single active component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetBodyEntity(e).GetID() == body.GetID());
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 1.0f);
    REQUIRE(store.GetLocalToWorldTransform(e).Position.x == 10.0f);
}

TEST_CASE("ColliderComponentStore - Add single inactive component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 2.0f);
    REQUIRE(store.GetLocalToWorldTransform(e).Position.x == 20.0f);
}

TEST_CASE("ColliderComponentStore - Add multiple active components", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), true);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f), makeTransform(30.0f)), true);

    requireDensePacking(store, { e1, e2, e3 }, {});
    REQUIRE(store.GetLocalToBodyTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(e3).Position.x == 3.0f);
}

TEST_CASE("ColliderComponentStore - Add multiple inactive components", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), false);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), false);

    requireDensePacking(store, {}, { e1, e2 });
}

TEST_CASE("ColliderComponentStore - Add active after inactive preserves partition", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity inactive = em.CreateEntity();
    Entity active = em.CreateEntity();

    store.AddComponent(inactive, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), false);
    store.AddComponent(active, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), true);

    requireDensePacking(store, { active }, { inactive });
    REQUIRE(store.GetLocalToBodyTransform(inactive).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(active).Position.x == 2.0f);
}

TEST_CASE("ColliderComponentStore - Interleaved active and inactive additions", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity e4 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), false);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f)), true);
    store.AddComponent(e4, makeComponent(body, makeTransform(4.0f)), false);

    requireDensePacking(store, { e1, e3 }, { e2, e4 });
    REQUIRE(store.GetLocalToBodyTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(e3).Position.x == 3.0f);
    REQUIRE(store.GetLocalToBodyTransform(e4).Position.x == 4.0f);
}

// ===========================================================================================
// HasComponent
// ===========================================================================================

TEST_CASE("ColliderComponentStore - HasComponent returns false for unknown entity", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity e = em.CreateEntity();
    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("ColliderComponentStore - HasComponent returns true after add", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    REQUIRE(store.HasComponent(e));
}

TEST_CASE("ColliderComponentStore - HasComponent returns false after remove", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.RemoveComponent(e);
    REQUIRE_FALSE(store.HasComponent(e));
}

// ===========================================================================================
// BroadPhaseID
// ===========================================================================================

TEST_CASE("ColliderComponentStore - BroadPhaseID initializes to AABB_TREE_NULL_NODE", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    REQUIRE(store.GetBroadPhaseID(e) == AABB_TREE_NULL_NODE);
}

TEST_CASE("ColliderComponentStore - SetBroadPhaseID updates value", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.SetBroadPhaseID(e, 42);

    REQUIRE(store.GetBroadPhaseID(e) == 42);
}

TEST_CASE("ColliderComponentStore - BroadPhaseID persists across SetActiveStatus", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.SetBroadPhaseID(e, 7);

    store.SetActiveStatus(e, false);
    REQUIRE(store.GetBroadPhaseID(e) == 7);

    store.SetActiveStatus(e, true);
    REQUIRE(store.GetBroadPhaseID(e) == 7);
}

// ===========================================================================================
// CollisionCategoryBits / CollidesWithMaskBits
// ===========================================================================================

TEST_CASE("ColliderComponentStore - CollisionCategoryBits round-trips", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(0.0f), makeTransform(0.0f), 0x0002, 0x0004), true);

    REQUIRE(store.GetCollisionCategoryBits(e) == 0x0002);
    REQUIRE(store.GetCollidesWithMaskBits(e) == 0x0004);
}

TEST_CASE("ColliderComponentStore - SetCollisionCategoryBits updates value", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.SetCollisionCategoryBits(e, 0x00FF);

    REQUIRE(store.GetCollisionCategoryBits(e) == 0x00FF);
}

TEST_CASE("ColliderComponentStore - SetCollidesWithMaskBits updates value", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.SetCollidesWithMaskBits(e, 0xFF00);

    REQUIRE(store.GetCollidesWithMaskBits(e) == 0xFF00);
}

// ===========================================================================================
// Transforms
// ===========================================================================================

TEST_CASE("ColliderComponentStore - SetLocalToBodyTransform updates value", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(1.0f)), true);
    store.SetLocalToBodyTransform(e, makeTransform(99.0f));

    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 99.0f);
}

TEST_CASE("ColliderComponentStore - SetLocalToWorldTransform updates value", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(0.0f), makeTransform(5.0f)), true);
    store.SetLocalToWorldTransform(e, makeTransform(88.0f));

    REQUIRE(store.GetLocalToWorldTransform(e).Position.x == 88.0f);
}

// ===========================================================================================
// Material
// ===========================================================================================

TEST_CASE("ColliderComponentStore - Material values stored on add", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();

    ColliderComponent comp(body, nullptr, makeTransform(0.0f), nullptr, 0x0001, 0xFFFF, makeTransform(0.0f), makeMaterial(0.8f, 0.2f, 2.0f));
    store.AddComponent(e, comp, true);

    REQUIRE(store.GetMaterial(e).GetFrictionCoefficient() == Catch::Approx(0.8f).epsilon(0.001f));
    REQUIRE(store.GetMaterial(e).GetRestitutionCoefficient() == Catch::Approx(0.2f).epsilon(0.001f));
}

TEST_CASE("ColliderComponentStore - SetMaterial updates values", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;

    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.SetMaterial(e, makeMaterial(0.1f, 0.9f, 3.0f));

    REQUIRE(store.GetMaterial(e).GetFrictionCoefficient() == Catch::Approx(0.1f).epsilon(0.001f));
    REQUIRE(store.GetMaterial(e).GetRestitutionCoefficient() == Catch::Approx(0.9f).epsilon(0.001f));
}

// ===========================================================================================
// Boolean flags
// ===========================================================================================

TEST_CASE("ColliderComponentStore - IsTrigger initializes to false", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    REQUIRE_FALSE(store.IsTrigger(e));
}

TEST_CASE("ColliderComponentStore - SetTrigger true/false round-trips", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.SetTrigger(e, true);
    REQUIRE(store.IsTrigger(e));
    store.SetTrigger(e, false);
    REQUIRE_FALSE(store.IsTrigger(e));
}

TEST_CASE("ColliderComponentStore - IsSimulationCollider initializes to false", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    REQUIRE_FALSE(store.IsSimulationCollider(e));
}

TEST_CASE("ColliderComponentStore - SetSimulationCollider true/false round-trips", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.SetSimulationCollider(e, true);
    REQUIRE(store.IsSimulationCollider(e));
    store.SetSimulationCollider(e, false);
    REQUIRE_FALSE(store.IsSimulationCollider(e));
}

TEST_CASE("ColliderComponentStore - IsQueryCollider initializes to false", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    REQUIRE_FALSE(store.IsQueryCollider(e));
}

TEST_CASE("ColliderComponentStore - SetQueryCollider true/false round-trips", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.SetQueryCollider(e, true);
    REQUIRE(store.IsQueryCollider(e));
    store.SetQueryCollider(e, false);
    REQUIRE_FALSE(store.IsQueryCollider(e));
}

TEST_CASE("ColliderComponentStore - HasCollisionShapeChangedSize initializes to false", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    REQUIRE_FALSE(store.HasCollisionShapeChangedSize(e));
}

TEST_CASE("ColliderComponentStore - SetCollisionShapeChangedSize true/false round-trips", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.SetCollisionShapeChangedSize(e, true);
    REQUIRE(store.HasCollisionShapeChangedSize(e));
    store.SetCollisionShapeChangedSize(e, false);
    REQUIRE_FALSE(store.HasCollisionShapeChangedSize(e));
}

TEST_CASE("ColliderComponentStore - Flags are independent per entity", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    store.AddComponent(e1, makeComponent(body), true);
    store.AddComponent(e2, makeComponent(body), true);

    store.SetTrigger(e1, true);
    store.SetSimulationCollider(e2, true);

    REQUIRE(store.IsTrigger(e1));
    REQUIRE_FALSE(store.IsTrigger(e2));
    REQUIRE_FALSE(store.IsSimulationCollider(e1));
    REQUIRE(store.IsSimulationCollider(e2));
}

// ===========================================================================================
// CollisionPairs
// ===========================================================================================

TEST_CASE("ColliderComponentStore - CollisionPairs empty on add", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    REQUIRE(store.GetOverlappingPairs(e).empty());
}

TEST_CASE("ColliderComponentStore - CollisionPairs can add and retrieve pairs", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.GetOverlappingPairs(e).push_back(10);
    store.GetOverlappingPairs(e).push_back(20);
    store.GetOverlappingPairs(e).push_back(30);

    REQUIRE(store.GetOverlappingPairs(e).size() == 3);
    REQUIRE(store.GetOverlappingPairs(e)[0] == 10);
    REQUIRE(store.GetOverlappingPairs(e)[1] == 20);
    REQUIRE(store.GetOverlappingPairs(e)[2] == 30);
}

TEST_CASE("ColliderComponentStore - CollisionPairs swap-erase removes correct pair", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    auto &pairs = store.GetOverlappingPairs(e);
    pairs.push_back(10);
    pairs.push_back(20);
    pairs.push_back(30);

    // Swap-erase the middle element (20).
    auto it = std::find(pairs.begin(), pairs.end(), 20);
    REQUIRE(it != pairs.end());
    *it = pairs.back();
    pairs.pop_back();

    REQUIRE(pairs.size() == 2);
    REQUIRE(std::find(pairs.begin(), pairs.end(), 20) == pairs.end());
    REQUIRE(std::find(pairs.begin(), pairs.end(), 10) != pairs.end());
    REQUIRE(std::find(pairs.begin(), pairs.end(), 30) != pairs.end());
}

TEST_CASE("ColliderComponentStore - CollisionPairs are independent per entity", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    store.AddComponent(e1, makeComponent(body), true);
    store.AddComponent(e2, makeComponent(body), true);

    store.GetOverlappingPairs(e1).push_back(100);
    store.GetOverlappingPairs(e2).push_back(200);
    store.GetOverlappingPairs(e2).push_back(300);

    REQUIRE(store.GetOverlappingPairs(e1).size() == 1);
    REQUIRE(store.GetOverlappingPairs(e1)[0] == 100);
    REQUIRE(store.GetOverlappingPairs(e2).size() == 2);
}

TEST_CASE("ColliderComponentStore - CollisionPairs persist across SetActiveStatus", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);

    store.GetOverlappingPairs(e).push_back(5);
    store.GetOverlappingPairs(e).push_back(6);

    store.SetActiveStatus(e, false);
    REQUIRE(store.GetOverlappingPairs(e).size() == 2);

    store.SetActiveStatus(e, true);
    REQUIRE(store.GetOverlappingPairs(e).size() == 2);
}

// ===========================================================================================
// RemoveComponent
// ===========================================================================================

TEST_CASE("ColliderComponentStore - Remove only active component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), true);
    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});
}

TEST_CASE("ColliderComponentStore - Remove only inactive component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body), false);
    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});
}

TEST_CASE("ColliderComponentStore - Remove active component preserves others", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), true);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f), makeTransform(30.0f)), true);

    store.RemoveComponent(e1);
    requireDensePacking(store, { e2, e3 }, {});
    REQUIRE(store.GetLocalToBodyTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(e3).Position.x == 3.0f);
}

TEST_CASE("ColliderComponentStore - Remove active component with inactive present", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), true);
    store.AddComponent(a2, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), true);
    store.AddComponent(i1, makeComponent(body, makeTransform(3.0f), makeTransform(30.0f)), false);

    store.RemoveComponent(a1);
    requireDensePacking(store, { a2 }, { i1 });
    REQUIRE(store.GetLocalToBodyTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(i1).Position.x == 3.0f);
}

TEST_CASE("ColliderComponentStore - Remove inactive component with active present", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();
    Entity i2 = em.CreateEntity();

    store.AddComponent(a1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(i1, makeComponent(body, makeTransform(2.0f)), false);
    store.AddComponent(i2, makeComponent(body, makeTransform(3.0f)), false);

    store.RemoveComponent(i1);
    requireDensePacking(store, { a1 }, { i2 });
    REQUIRE(store.GetLocalToBodyTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(i2).Position.x == 3.0f);
}

TEST_CASE("ColliderComponentStore - Remove all components one by one", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), true);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f)), false);

    store.RemoveComponent(e2);
    requireDensePacking(store, { e1 }, { e3 });

    store.RemoveComponent(e1);
    requireDensePacking(store, {}, { e3 });

    store.RemoveComponent(e3);
    requireDensePacking(store, {}, {});
}

TEST_CASE("ColliderComponentStore - Remove then re-add same entity", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();

    store.AddComponent(e, makeComponent(body, makeTransform(1.0f)), true);
    store.RemoveComponent(e);
    requireDensePacking(store, {}, {});

    store.AddComponent(e, makeComponent(body, makeTransform(99.0f)), true);
    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 99.0f);
}

// ===========================================================================================
// SetActiveStatus (activate)
// ===========================================================================================

TEST_CASE("ColliderComponentStore - SetActiveStatus true on inactive component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(5.0f), makeTransform(50.0f)), false);

    store.SetActiveStatus(e, true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 5.0f);
    REQUIRE(store.GetLocalToWorldTransform(e).Position.x == 50.0f);
}

TEST_CASE("ColliderComponentStore - SetActiveStatus true on already active is no-op", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(5.0f)), true);

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});
}

TEST_CASE("ColliderComponentStore - SetActiveStatus true on one of several inactive", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), false);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), false);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f)), false);

    store.SetActiveStatus(e2, true);

    requireDensePacking(store, { e2 }, { e1, e3 });
    REQUIRE(store.GetLocalToBodyTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(e3).Position.x == 3.0f);
}

// ===========================================================================================
// SetActiveStatus (deactivate)
// ===========================================================================================

TEST_CASE("ColliderComponentStore - SetActiveStatus false on active component", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(5.0f), makeTransform(50.0f)), true);

    store.SetActiveStatus(e, false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 5.0f);
    REQUIRE(store.GetLocalToWorldTransform(e).Position.x == 50.0f);
}

TEST_CASE("ColliderComponentStore - SetActiveStatus false on already inactive is no-op", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(5.0f)), false);

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });
}

TEST_CASE("ColliderComponentStore - SetActiveStatus false on one of several active", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), true);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f)), true);

    store.SetActiveStatus(e2, false);

    requireDensePacking(store, { e1, e3 }, { e2 });
    REQUIRE(store.GetLocalToBodyTransform(e1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(e2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(e3).Position.x == 3.0f);
}

TEST_CASE("ColliderComponentStore - SetActiveStatus false preserves inactive components", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity a1 = em.CreateEntity();
    Entity a2 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(a2, makeComponent(body, makeTransform(2.0f)), true);
    store.AddComponent(i1, makeComponent(body, makeTransform(3.0f)), false);

    store.SetActiveStatus(a1, false);

    requireDensePacking(store, { a2 }, { a1, i1 });
    REQUIRE(store.GetLocalToBodyTransform(a1).Position.x == 1.0f);
    REQUIRE(store.GetLocalToBodyTransform(a2).Position.x == 2.0f);
    REQUIRE(store.GetLocalToBodyTransform(i1).Position.x == 3.0f);
}

// ===========================================================================================
// SetActiveStatus round-trips
// ===========================================================================================

TEST_CASE("ColliderComponentStore - SetActiveStatus true then false returns to inactive", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity a1 = em.CreateEntity();
    Entity i1 = em.CreateEntity();

    store.AddComponent(a1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(i1, makeComponent(body, makeTransform(2.0f)), false);

    store.SetActiveStatus(i1, true);
    requireDensePacking(store, { a1, i1 }, {});

    store.SetActiveStatus(i1, false);
    requireDensePacking(store, { a1 }, { i1 });
    REQUIRE(store.GetLocalToBodyTransform(i1).Position.x == 2.0f);
}

TEST_CASE("ColliderComponentStore - SetActiveStatus false then true returns to active", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();
    store.AddComponent(e, makeComponent(body, makeTransform(7.0f)), true);

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});
    REQUIRE(store.GetLocalToBodyTransform(e).Position.x == 7.0f);
}

// ===========================================================================================
// Span accessors correctness after swaps
// ===========================================================================================

TEST_CASE("ColliderComponentStore - Span accessors align with per-entity getters after deactivation", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f), makeTransform(10.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f), makeTransform(20.0f)), true);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f), makeTransform(30.0f)), true);

    store.SetActiveStatus(e1, false);
    requireDensePacking(store, { e2, e3 }, { e1 });

    auto broadPhaseIDs = store.GetActiveBroadPhaseIDs();
    REQUIRE(broadPhaseIDs.size() == 2);
    // All initialized to -1.
    REQUIRE(broadPhaseIDs[0] == AABB_TREE_NULL_NODE);
    REQUIRE(broadPhaseIDs[1] == AABB_TREE_NULL_NODE);
}

TEST_CASE("ColliderComponentStore - GetActiveBroadPhaseIDs reflects SetBroadPhaseID", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body), true);
    store.AddComponent(e2, makeComponent(body), true);
    store.SetBroadPhaseID(e1, 11);
    store.SetBroadPhaseID(e2, 22);

    auto ids = store.GetActiveBroadPhaseIDs();
    auto entities = store.GetActiveEntities();
    REQUIRE(ids.size() == 2);

    for (size_t i = 0; i < entities.size(); ++i) {
        REQUIRE(ids[i] == store.GetBroadPhaseID(entities[i]));
    }
}

TEST_CASE("ColliderComponentStore - GetActiveLocalToBodyTransforms align with per-entity getter", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body, makeTransform(1.0f)), true);
    store.AddComponent(e2, makeComponent(body, makeTransform(2.0f)), false);
    store.AddComponent(e3, makeComponent(body, makeTransform(3.0f)), true);

    auto transforms = store.GetActiveLocalToBodyTransforms();
    auto entities = store.GetActiveEntities();
    REQUIRE(transforms.size() == 2);

    for (size_t i = 0; i < entities.size(); ++i) {
        REQUIRE(transforms[i].Position.x == store.GetLocalToBodyTransform(entities[i]).Position.x);
    }
}

// ===========================================================================================
// BodyEntity tracking
// ===========================================================================================

TEST_CASE("ColliderComponentStore - GetBodyEntity returns correct body", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body1 = em.CreateEntity();
    Entity body2 = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body1), true);
    store.AddComponent(e2, makeComponent(body2), true);

    REQUIRE(store.GetBodyEntity(e1).GetID() == body1.GetID());
    REQUIRE(store.GetBodyEntity(e2).GetID() == body2.GetID());
}

TEST_CASE("ColliderComponentStore - GetBodyEntity preserved after SetActiveStatus swap", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body1 = em.CreateEntity();
    Entity body2 = em.CreateEntity();
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeComponent(body1), true);
    store.AddComponent(e2, makeComponent(body2), true);

    store.SetActiveStatus(e1, false);

    REQUIRE(store.GetBodyEntity(e1).GetID() == body1.GetID());
    REQUIRE(store.GetBodyEntity(e2).GetID() == body2.GetID());
}

// ===========================================================================================
// Pointer passthrough
// ===========================================================================================

TEST_CASE("ColliderComponentStore - GetCollider returns stored pointer", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();

    // The store stores and returns the pointer as-is; use a non-null sentinel to verify.
    auto *sentinel = reinterpret_cast<Collider *>(static_cast<uintptr_t>(0xDEADBEEF));
    ColliderComponent comp(body, sentinel, makeTransform(0.0f), nullptr, 0x0001, 0xFFFF, makeTransform(0.0f), makeMaterial());
    store.AddComponent(e, comp, true);

    REQUIRE(&store.GetCollider(e) == sentinel);
}

TEST_CASE("ColliderComponentStore - GetCollisionShape returns stored pointer", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();
    Entity e = em.CreateEntity();

    auto *sentinel = reinterpret_cast<CollisionShape *>(static_cast<uintptr_t>(0xCAFEBABE));
    ColliderComponent comp(body, nullptr, makeTransform(0.0f), sentinel, 0x0001, 0xFFFF, makeTransform(0.0f), makeMaterial());
    store.AddComponent(e, comp, true);

    REQUIRE(&store.GetCollisionShape(e) == sentinel);
}

// ===========================================================================================
// Stress test
// ===========================================================================================

TEST_CASE("ColliderComponentStore - Stress: many adds, removes, and SetActiveStatus calls", "[ecs][collider]") {
    EntityManager em;
    ColliderComponentStore store;
    Entity body = em.CreateEntity();

    constexpr int N = 100;
    std::vector<Entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i) {
        Entity e = em.CreateEntity();
        entities.push_back(e);
        bool active = (i % 3 != 0);
        store.AddComponent(e, makeComponent(body, makeTransform(static_cast<float>(i))), active);
    }

    REQUIRE(store.GetTotalComponentCount() == N);

    // Deactivate all active components.
    for (const auto &e : entities) {
        if (!store.IsDisabled(e)) {
            store.SetActiveStatus(e, false);
        }
    }
    REQUIRE(store.GetActiveComponentCount() == 0);

    // Re-activate all components.
    for (const auto &e : entities) {
        store.SetActiveStatus(e, true);
    }
    REQUIRE(store.GetActiveComponentCount() == N);

    // Remove every other component.
    for (int i = 0; i < N; i += 2) {
        store.RemoveComponent(entities[static_cast<size_t>(i)]);
    }
    REQUIRE(store.GetTotalComponentCount() == N / 2);

    // Verify all remaining components have intact data.
    for (int i = 1; i < N; i += 2) {
        Entity e = entities[static_cast<size_t>(i)];
        REQUIRE(store.HasComponent(e));
        REQUIRE(store.GetLocalToBodyTransform(e).Position.x == Catch::Approx(static_cast<float>(i)));
    }

    // Verify removed entities are gone.
    for (int i = 0; i < N; i += 2) {
        REQUIRE_FALSE(store.HasComponent(entities[static_cast<size_t>(i)]));
    }
}
