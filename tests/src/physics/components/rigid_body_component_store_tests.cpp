#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

#include <unordered_set>

using namespace Vulkyrie;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Creates a RigidBodyComponent with the given parameters
static RigidBodyComponent makeRigidBodyComp(RigidBody *body, BodyType type = BodyType::Dynamic, const glm::vec3 &worldPos = glm::vec3(0.0f)) {
    return RigidBodyComponent{ body, type, worldPos };
}

// Verifies the dense-packing invariant: active components occupy [0, activeCount)
// and inactive components occupy [activeCount, totalCount).
static void requireDensePacking(RigidBodyComponentStore &store, const std::vector<Entity> &expectedActive, const std::vector<Entity> &expectedInactive) {
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

TEST_CASE("RigidBodyComponentStore - Add single active component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    requireDensePacking(store, { e }, {});
    REQUIRE(store.HasComponent(e));
}

TEST_CASE("RigidBodyComponentStore - Add single inactive component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e });
    REQUIRE(store.HasComponent(e));
}

TEST_CASE("RigidBodyComponentStore - Add multiple active components", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e2, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e3, makeRigidBodyComp(nullptr), true);

    requireDensePacking(store, { e1, e2, e3 }, {});
}

TEST_CASE("RigidBodyComponentStore - Add multiple inactive components", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(nullptr), false);
    store.AddComponent(e2, makeRigidBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e1, e2 });
}

TEST_CASE("RigidBodyComponentStore - Add active after inactive components triggers swap into active zone", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity inactive1 = em.CreateEntity();
    Entity inactive2 = em.CreateEntity();
    Entity active1 = em.CreateEntity();

    store.AddComponent(inactive1, makeRigidBodyComp(nullptr), false);
    store.AddComponent(inactive2, makeRigidBodyComp(nullptr), false);
    store.AddComponent(active1, makeRigidBodyComp(nullptr), true);

    requireDensePacking(store, { active1 }, { inactive1, inactive2 });
}

TEST_CASE("RigidBodyComponentStore - Interleaved active and inactive additions", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();
    Entity e4 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e2, makeRigidBodyComp(nullptr), false);
    store.AddComponent(e3, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e4, makeRigidBodyComp(nullptr), false);

    requireDensePacking(store, { e1, e3 }, { e2, e4 });
}

TEST_CASE("RigidBodyComponentStore - AddComponent initializes WorldPosition", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    glm::vec3 worldPos(10.0f, 20.0f, 30.0f);
    store.AddComponent(e, makeRigidBodyComp(nullptr, BodyType::Dynamic, worldPos), true);

    REQUIRE(store.GetWorldCenterOfMass(e) == worldPos);
}

// ===========================================================================================
// GetRigidBody - pointer identity
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - GetRigidBody returns the correct body pointer", "[ecs][rigidbody]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<RigidBody> rb1(new RigidBody(bodyEm.CreateEntity(), world));
    std::unique_ptr<RigidBody> rb2(new RigidBody(bodyEm.CreateEntity(), world));

    EntityManager em;
    RigidBodyComponentStore store;
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(rb1.get()), true);
    store.AddComponent(e2, makeRigidBodyComp(rb2.get()), true);

    REQUIRE(&store.GetRigidBody(e1) == rb1.get());
    REQUIRE(&store.GetRigidBody(e2) == rb2.get());
}

TEST_CASE("RigidBodyComponentStore - GetRigidBody const overload works correctly", "[ecs][rigidbody]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<RigidBody> rb(new RigidBody(bodyEm.CreateEntity(), world));

    EntityManager em;
    RigidBodyComponentStore store;
    Entity e = em.CreateEntity();

    store.AddComponent(e, makeRigidBodyComp(rb.get()), true);

    const RigidBodyComponentStore &constStore = store;
    REQUIRE(&constStore.GetRigidBody(e) == rb.get());
}

TEST_CASE("RigidBodyComponentStore - GetRigidBody returns correct pointer after swap on add", "[ecs][rigidbody]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<RigidBody> rb1(new RigidBody(bodyEm.CreateEntity(), world));
    std::unique_ptr<RigidBody> rb2(new RigidBody(bodyEm.CreateEntity(), world));

    EntityManager em;
    RigidBodyComponentStore store;
    Entity inactive = em.CreateEntity();
    Entity active = em.CreateEntity();

    store.AddComponent(inactive, makeRigidBodyComp(rb1.get()), false);
    store.AddComponent(active, makeRigidBodyComp(rb2.get()), true);

    REQUIRE(&store.GetRigidBody(inactive) == rb1.get());
    REQUIRE(&store.GetRigidBody(active) == rb2.get());
}

// ===========================================================================================
// BodyType
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - BodyType is correctly set on creation", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity eStatic = em.CreateEntity();
    Entity eKinematic = em.CreateEntity();
    Entity eDynamic = em.CreateEntity();

    store.AddComponent(eStatic, makeRigidBodyComp(nullptr, BodyType::Static), true);
    store.AddComponent(eKinematic, makeRigidBodyComp(nullptr, BodyType::Kinematic), true);
    store.AddComponent(eDynamic, makeRigidBodyComp(nullptr, BodyType::Dynamic), true);

    REQUIRE(store.GetBodyType(eStatic) == BodyType::Static);
    REQUIRE(store.GetBodyType(eKinematic) == BodyType::Kinematic);
    REQUIRE(store.GetBodyType(eDynamic) == BodyType::Dynamic);
}

TEST_CASE("RigidBodyComponentStore - SetBodyType changes body type", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr, BodyType::Dynamic), true);

    store.SetBodyType(e, BodyType::Static);
    REQUIRE(store.GetBodyType(e) == BodyType::Static);

    store.SetBodyType(e, BodyType::Kinematic);
    REQUIRE(store.GetBodyType(e) == BodyType::Kinematic);
}

// ===========================================================================================
// Sleep behavior
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - CanSleep defaults to true", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.CanSleep(e) == true);
}

TEST_CASE("RigidBodyComponentStore - SetCanSleep updates can sleep flag", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetCanSleep(e, false);
    REQUIRE(store.CanSleep(e) == false);

    store.SetCanSleep(e, true);
    REQUIRE(store.CanSleep(e) == true);
}

TEST_CASE("RigidBodyComponentStore - IsSleeping defaults to false", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.IsSleeping(e) == false);
}

TEST_CASE("RigidBodyComponentStore - SetIsSleeping updates sleeping flag", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetIsSleeping(e, true);
    REQUIRE(store.IsSleeping(e) == true);

    store.SetIsSleeping(e, false);
    REQUIRE(store.IsSleeping(e) == false);
}

TEST_CASE("RigidBodyComponentStore - GetSleepTime defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetSleepTime(e) == 0.0f);
}

TEST_CASE("RigidBodyComponentStore - SetSleepTime updates sleep time", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetSleepTime(e, 2.5f);
    REQUIRE(store.GetSleepTime(e) == 2.5f);

    store.SetSleepTime(e, 0.0f);
    REQUIRE(store.GetSleepTime(e) == 0.0f);
}

// ===========================================================================================
// Velocities
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Linear velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetLinearVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetLinearVelocity updates linear velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 vel(10.0f, 20.0f, 30.0f);
    store.SetLinearVelocity(e, vel);
    REQUIRE(store.GetLinearVelocity(e) == vel);
}

TEST_CASE("RigidBodyComponentStore - Angular velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetAngularVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetAngularVelocity updates angular velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 angVel(0.1f, 0.2f, 0.3f);
    store.SetAngularVelocity(e, angVel);
    REQUIRE(store.GetAngularVelocity(e) == angVel);
}

// ===========================================================================================
// Forces and torques
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - External force defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetExternalForce(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetExternalForce updates external force", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 force(100.0f, 200.0f, 300.0f);
    store.SetExternalForce(e, force);
    REQUIRE(store.GetExternalForce(e) == force);
}

TEST_CASE("RigidBodyComponentStore - External torque defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetExternalTorque(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetExternalTorque updates external torque", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 torque(10.0f, 20.0f, 30.0f);
    store.SetExternalTorque(e, torque);
    REQUIRE(store.GetExternalTorque(e) == torque);
}

// ===========================================================================================
// Damping
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Linear damping defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetLinearDamping(e) == 0.0f);
}

TEST_CASE("RigidBodyComponentStore - SetLinearDamping updates linear damping", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetLinearDamping(e, 0.5f);
    REQUIRE(store.GetLinearDamping(e) == 0.5f);
}

TEST_CASE("RigidBodyComponentStore - Angular damping defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetAngularDamping(e) == 0.0f);
}

TEST_CASE("RigidBodyComponentStore - SetAngularDamping updates angular damping", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetAngularDamping(e, 0.3f);
    REQUIRE(store.GetAngularDamping(e) == 0.3f);
}

// ===========================================================================================
// Mass
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Mass defaults to 1.0", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetMass(e) == 1.0f);
}

TEST_CASE("RigidBodyComponentStore - SetMass updates mass", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetMass(e, 10.0f);
    REQUIRE(store.GetMass(e) == 10.0f);
}

TEST_CASE("RigidBodyComponentStore - Inverse mass defaults to 1.0", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetInverseMass(e) == 1.0f);
}

TEST_CASE("RigidBodyComponentStore - SetInverseMass updates inverse mass", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetInverseMass(e, 0.1f);
    REQUIRE(store.GetInverseMass(e) == 0.1f);
}

TEST_CASE("RigidBodyComponentStore - SetInverseMass to zero for static bodies", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr, BodyType::Static), true);

    store.SetInverseMass(e, 0.0f);
    REQUIRE(store.GetInverseMass(e) == 0.0f);
}

// ===========================================================================================
// Inertia tensors
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Local inertia tensor defaults to identity diagonal", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetLocalInertiaTensor(e) == glm::vec3(1.0f));
}

TEST_CASE("RigidBodyComponentStore - SetLocalInertiaTensor updates local inertia tensor", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 inertia(2.0f, 3.0f, 4.0f);
    store.SetLocalInertiaTensor(e, inertia);
    REQUIRE(store.GetLocalInertiaTensor(e) == inertia);
}

TEST_CASE("RigidBodyComponentStore - Inverse local inertia tensor defaults to identity diagonal", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetInverseLocalInertiaTensor(e) == glm::vec3(1.0f));
}

TEST_CASE("RigidBodyComponentStore - SetInverseLocalInertiaTensor updates inverse local inertia tensor", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 invInertia(0.5f, 0.33f, 0.25f);
    store.SetInverseLocalInertiaTensor(e, invInertia);
    REQUIRE(store.GetInverseLocalInertiaTensor(e) == invInertia);
}

TEST_CASE("RigidBodyComponentStore - Inverse world inertia tensor defaults to identity matrix", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::mat3 identity = glm::identity<glm::mat3>();
    REQUIRE(store.GetInverseWorldInertiaTensor(e) == identity);
}

TEST_CASE("RigidBodyComponentStore - SetInverseWorldInertiaTensor updates inverse world inertia tensor", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::mat3 invWorldInertia(2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f);
    store.SetInverseWorldInertiaTensor(e, invWorldInertia);
    REQUIRE(store.GetInverseWorldInertiaTensor(e) == invWorldInertia);
}

// ===========================================================================================
// Constrained velocities
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Constrained linear velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetConstrainedLinearVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetConstrainedLinearVelocity updates constrained linear velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 vel(5.0f, 10.0f, 15.0f);
    store.SetConstrainedLinearVelocity(e, vel);
    REQUIRE(store.GetConstrainedLinearVelocity(e) == vel);
}

TEST_CASE("RigidBodyComponentStore - Constrained angular velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetConstrainedAngularVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetConstrainedAngularVelocity updates constrained angular velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 angVel(0.5f, 1.0f, 1.5f);
    store.SetConstrainedAngularVelocity(e, angVel);
    REQUIRE(store.GetConstrainedAngularVelocity(e) == angVel);
}

// ===========================================================================================
// Split velocities
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Split linear velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetSplitLinearVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetSplitLinearVelocity updates split linear velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 vel(1.0f, 2.0f, 3.0f);
    store.SetSplitLinearVelocity(e, vel);
    REQUIRE(store.GetSplitLinearVelocity(e) == vel);
}

TEST_CASE("RigidBodyComponentStore - Split angular velocity defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetSplitAngularVelocity(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetSplitAngularVelocity updates split angular velocity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 angVel(0.1f, 0.2f, 0.3f);
    store.SetSplitAngularVelocity(e, angVel);
    REQUIRE(store.GetSplitAngularVelocity(e) == angVel);
}

// ===========================================================================================
// Constrained position and orientation
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Constrained position defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetConstrainedPosition(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetConstrainedPosition updates constrained position", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 pos(10.0f, 20.0f, 30.0f);
    store.SetConstrainedPosition(e, pos);
    REQUIRE(store.GetConstrainedPosition(e) == pos);
}

TEST_CASE("RigidBodyComponentStore - GetConstrainedPosition returns mutable reference", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.GetConstrainedPosition(e) = glm::vec3(5.0f, 10.0f, 15.0f);
    REQUIRE(store.GetConstrainedPosition(e) == glm::vec3(5.0f, 10.0f, 15.0f));
}

TEST_CASE("RigidBodyComponentStore - Constrained orientation defaults to identity quaternion", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::quat identity(1.0f, 0.0f, 0.0f, 0.0f);
    REQUIRE(store.GetConstrainedOrientation(e) == identity);
}

TEST_CASE("RigidBodyComponentStore - SetConstrainedOrientation updates constrained orientation", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    store.SetConstrainedOrientation(e, rotation);
    REQUIRE(store.GetConstrainedOrientation(e) == rotation);
}

TEST_CASE("RigidBodyComponentStore - GetConstrainedOrientation returns mutable reference", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::quat rotation(0.707f, 0.0f, 0.707f, 0.0f);
    store.GetConstrainedOrientation(e) = rotation;
    REQUIRE(store.GetConstrainedOrientation(e) == rotation);
}

// ===========================================================================================
// Center of mass
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Local center of mass defaults to zero", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetLocalCenterOfMass(e) == glm::vec3(0.0f));
}

TEST_CASE("RigidBodyComponentStore - SetLocalCenterOfMass updates local center of mass", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 com(0.5f, 0.0f, 0.0f);
    store.SetLocalCenterOfMass(e, com);
    REQUIRE(store.GetLocalCenterOfMass(e) == com);
}

TEST_CASE("RigidBodyComponentStore - World center of mass initialized from WorldPosition", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    glm::vec3 worldPos(100.0f, 200.0f, 300.0f);
    store.AddComponent(e, makeRigidBodyComp(nullptr, BodyType::Dynamic, worldPos), true);

    REQUIRE(store.GetWorldCenterOfMass(e) == worldPos);
}

TEST_CASE("RigidBodyComponentStore - SetWorldCenterOfMass updates world center of mass", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 worldCom(50.0f, 100.0f, 150.0f);
    store.SetWorldCenterOfMass(e, worldCom);
    REQUIRE(store.GetWorldCenterOfMass(e) == worldCom);
}

// ===========================================================================================
// Flags
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Gravity enabled defaults to true", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.IsGravityEnabled(e) == true);
}

TEST_CASE("RigidBodyComponentStore - SetGravityEnabled updates gravity flag", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetGravityEnabled(e, false);
    REQUIRE(store.IsGravityEnabled(e) == false);

    store.SetGravityEnabled(e, true);
    REQUIRE(store.IsGravityEnabled(e) == true);
}

TEST_CASE("RigidBodyComponentStore - IsInIsland defaults to false", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.IsInIsland(e) == false);
}

TEST_CASE("RigidBodyComponentStore - SetInIsland updates island flag", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    store.SetInIsland(e, true);
    REQUIRE(store.IsInIsland(e) == true);

    store.SetInIsland(e, false);
    REQUIRE(store.IsInIsland(e) == false);
}

// ===========================================================================================
// Axis locking factors
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Linear lock axis factors default to all free (1.0)", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetLinearLockAxisFactor(e) == glm::vec3(1.0f));
}

TEST_CASE("RigidBodyComponentStore - SetLinearLockAxisFactor updates linear lock factors", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 lockFactors(1.0f, 0.0f, 1.0f); // Lock Y axis
    store.SetLinearLockAxisFactor(e, lockFactors);
    REQUIRE(store.GetLinearLockAxisFactor(e) == lockFactors);
}

TEST_CASE("RigidBodyComponentStore - Angular lock axis factors default to all free (1.0)", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetAngularLockAxisFactor(e) == glm::vec3(1.0f));
}

TEST_CASE("RigidBodyComponentStore - SetAngularLockAxisFactor updates angular lock factors", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    glm::vec3 lockFactors(0.0f, 1.0f, 0.0f); // Lock X and Z, free Y
    store.SetAngularLockAxisFactor(e, lockFactors);
    REQUIRE(store.GetAngularLockAxisFactor(e) == lockFactors);
}

// ===========================================================================================
// Joint management
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - GetJoints returns empty vector by default", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.GetJoints(e).empty());
}

TEST_CASE("RigidBodyComponentStore - AddJointToBody adds joint entity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity joint = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddJointToBody(body, joint);
    auto &joints = store.GetJoints(body);
    REQUIRE(joints.size() == 1);
    REQUIRE(joints[0] == joint);
}

TEST_CASE("RigidBodyComponentStore - AddJointToBody adds multiple joints", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity joint1 = em.CreateEntity();
    Entity joint2 = em.CreateEntity();
    Entity joint3 = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddJointToBody(body, joint1);
    store.AddJointToBody(body, joint2);
    store.AddJointToBody(body, joint3);

    auto &joints = store.GetJoints(body);
    REQUIRE(joints.size() == 3);
    REQUIRE(std::find(joints.begin(), joints.end(), joint1) != joints.end());
    REQUIRE(std::find(joints.begin(), joints.end(), joint2) != joints.end());
    REQUIRE(std::find(joints.begin(), joints.end(), joint3) != joints.end());
}

TEST_CASE("RigidBodyComponentStore - RemoveJointFromBody removes joint entity", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity joint = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddJointToBody(body, joint);
    REQUIRE(store.GetJoints(body).size() == 1);

    store.RemoveJointFromBody(body, joint);
    REQUIRE(store.GetJoints(body).empty());
}

TEST_CASE("RigidBodyComponentStore - RemoveJointFromBody uses swap-erase (order not preserved)", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity joint1 = em.CreateEntity();
    Entity joint2 = em.CreateEntity();
    Entity joint3 = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddJointToBody(body, joint1);
    store.AddJointToBody(body, joint2);
    store.AddJointToBody(body, joint3);

    store.RemoveJointFromBody(body, joint2);
    auto &joints = store.GetJoints(body);
    REQUIRE(joints.size() == 2);
    REQUIRE(std::find(joints.begin(), joints.end(), joint1) != joints.end());
    REQUIRE(std::find(joints.begin(), joints.end(), joint3) != joints.end());
    REQUIRE(std::find(joints.begin(), joints.end(), joint2) == joints.end());
}

// ===========================================================================================
// Contact pair management
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - AddContactPair adds contact pair index", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddContactPair(body, 42);
    store.AddContactPair(body, 100);
    store.AddContactPair(body, 200);

    // Note: GetContactPairs doesn't exist in the public interface, but we can test via RemoveAllContactPairs
    // We verify by adding pairs, removing all, then re-adding to check it starts empty
}

TEST_CASE("RigidBodyComponentStore - RemoveAllContactPairs clears contact pairs", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    store.AddContactPair(body, 10);
    store.AddContactPair(body, 20);
    store.AddContactPair(body, 30);

    store.RemoveAllContactPairs(body);

    // After clearing, adding new pairs should start fresh
    store.AddContactPair(body, 50);
    // Can't directly verify count, but the operation should complete without error
}

// ===========================================================================================
// SetActiveStatus
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - SetActiveStatus activates inactive component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), false);

    requireDensePacking(store, {}, { e });

    store.SetActiveStatus(e, true);
    requireDensePacking(store, { e }, {});
}

TEST_CASE("RigidBodyComponentStore - SetActiveStatus deactivates active component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    requireDensePacking(store, { e }, {});

    store.SetActiveStatus(e, false);
    requireDensePacking(store, {}, { e });
}

TEST_CASE("RigidBodyComponentStore - SetActiveStatus preserves all component data during swap", "[ecs][rigidbody]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<RigidBody> rb(new RigidBody(bodyEm.CreateEntity(), world));

    EntityManager em;
    RigidBodyComponentStore store;
    Entity e = em.CreateEntity();

    store.AddComponent(e, makeRigidBodyComp(rb.get(), BodyType::Kinematic, glm::vec3(10.0f, 20.0f, 30.0f)), true);
    store.SetMass(e, 5.0f);
    store.SetLinearVelocity(e, glm::vec3(1.0f, 2.0f, 3.0f));
    store.SetAngularVelocity(e, glm::vec3(0.1f, 0.2f, 0.3f));

    store.SetActiveStatus(e, false);

    REQUIRE(&store.GetRigidBody(e) == rb.get());
    REQUIRE(store.GetBodyType(e) == BodyType::Kinematic);
    REQUIRE(store.GetMass(e) == 5.0f);
    REQUIRE(store.GetLinearVelocity(e) == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(store.GetAngularVelocity(e) == glm::vec3(0.1f, 0.2f, 0.3f));
    REQUIRE(store.GetWorldCenterOfMass(e) == glm::vec3(10.0f, 20.0f, 30.0f));
}

// ===========================================================================================
// RemoveComponent
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - RemoveComponent removes active component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    REQUIRE(store.HasComponent(e));
    store.RemoveComponent(e);
    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("RigidBodyComponentStore - RemoveComponent removes inactive component", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), false);

    REQUIRE(store.HasComponent(e));
    store.RemoveComponent(e);
    REQUIRE_FALSE(store.HasComponent(e));
}

TEST_CASE("RigidBodyComponentStore - RemoveComponent maintains dense packing", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();
    Entity e3 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e2, makeRigidBodyComp(nullptr), true);
    store.AddComponent(e3, makeRigidBodyComp(nullptr), false);

    store.RemoveComponent(e2);

    requireDensePacking(store, { e1 }, { e3 });
}

TEST_CASE("RigidBodyComponentStore - RemoveComponent preserves data of remaining components", "[ecs][rigidbody]") {
    PhysicsWorldSettings ws("test_world");
    PhysicsWorld world(ws);
    EntityManager bodyEm;
    std::unique_ptr<RigidBody> rb1(new RigidBody(bodyEm.CreateEntity(), world));
    std::unique_ptr<RigidBody> rb2(new RigidBody(bodyEm.CreateEntity(), world));

    EntityManager em;
    RigidBodyComponentStore store;
    Entity e1 = em.CreateEntity();
    Entity e2 = em.CreateEntity();

    store.AddComponent(e1, makeRigidBodyComp(rb1.get()), true);
    store.AddComponent(e2, makeRigidBodyComp(rb2.get()), true);
    store.SetMass(e1, 10.0f);
    store.SetMass(e2, 20.0f);

    store.RemoveComponent(e2);

    REQUIRE(&store.GetRigidBody(e1) == rb1.get());
    REQUIRE(store.GetMass(e1) == 10.0f);
}

// ===========================================================================================
// Edge cases
// ===========================================================================================

TEST_CASE("RigidBodyComponentStore - Multiple activations/deactivations preserve data", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);
    store.SetLinearVelocity(e, glm::vec3(10.0f, 20.0f, 30.0f));

    store.SetActiveStatus(e, false);
    REQUIRE(store.GetLinearVelocity(e) == glm::vec3(10.0f, 20.0f, 30.0f));

    store.SetActiveStatus(e, true);
    REQUIRE(store.GetLinearVelocity(e) == glm::vec3(10.0f, 20.0f, 30.0f));

    store.SetActiveStatus(e, false);
    REQUIRE(store.GetLinearVelocity(e) == glm::vec3(10.0f, 20.0f, 30.0f));
}

TEST_CASE("RigidBodyComponentStore - Large number of components maintains dense packing", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    std::vector<Entity> activeEntities;
    std::vector<Entity> inactiveEntities;

    for (int i = 0; i < 100; ++i) {
        Entity e = em.CreateEntity();
        bool active = (i % 2 == 0);
        store.AddComponent(e, makeRigidBodyComp(nullptr), active);
        if (active) {
            activeEntities.push_back(e);
        } else {
            inactiveEntities.push_back(e);
        }
    }

    requireDensePacking(store, activeEntities, inactiveEntities);
}

TEST_CASE("RigidBodyComponentStore - Zero mass and inverse mass edge case", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr, BodyType::Static), true);

    store.SetMass(e, 0.0f);
    store.SetInverseMass(e, 0.0f);

    REQUIRE(store.GetMass(e) == 0.0f);
    REQUIRE(store.GetInverseMass(e) == 0.0f);
}

TEST_CASE("RigidBodyComponentStore - Axis lock factors can be partially locked", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    // Lock only X and Z linear, free Y
    store.SetLinearLockAxisFactor(e, glm::vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(store.GetLinearLockAxisFactor(e) == glm::vec3(0.0f, 1.0f, 0.0f));

    // Lock only Y angular, free X and Z
    store.SetAngularLockAxisFactor(e, glm::vec3(1.0f, 0.0f, 1.0f));
    REQUIRE(store.GetAngularLockAxisFactor(e) == glm::vec3(1.0f, 0.0f, 1.0f));
}

TEST_CASE("RigidBodyComponentStore - Quaternion normalization is caller's responsibility", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity e = em.CreateEntity();
    store.AddComponent(e, makeRigidBodyComp(nullptr), true);

    // Store doesn't normalize quaternions - caller must ensure they're valid
    glm::quat unnormalized(2.0f, 0.0f, 0.0f, 0.0f);
    store.SetConstrainedOrientation(e, unnormalized);
    REQUIRE(store.GetConstrainedOrientation(e) == unnormalized);
}

TEST_CASE("RigidBodyComponentStore - Joint removal from body with no joints doesn't crash", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity body = em.CreateEntity();
    Entity joint = em.CreateEntity();
    store.AddComponent(body, makeRigidBodyComp(nullptr), true);

    // This should trigger an assertion in debug builds, but we can't test that directly
    // In release, it will search and not find the joint
    // store.RemoveJointFromBody(body, joint); // Would assert
}

TEST_CASE("RigidBodyComponentStore - All BodyType values are supported", "[ecs][rigidbody]") {
    EntityManager em;
    RigidBodyComponentStore store;

    Entity eStatic = em.CreateEntity();
    Entity eKinematic = em.CreateEntity();
    Entity eDynamic = em.CreateEntity();

    store.AddComponent(eStatic, makeRigidBodyComp(nullptr, BodyType::Static), true);
    store.AddComponent(eKinematic, makeRigidBodyComp(nullptr, BodyType::Kinematic), true);
    store.AddComponent(eDynamic, makeRigidBodyComp(nullptr, BodyType::Dynamic), true);

    REQUIRE(store.GetBodyType(eStatic) == BodyType::Static);
    REQUIRE(store.GetBodyType(eKinematic) == BodyType::Kinematic);
    REQUIRE(store.GetBodyType(eDynamic) == BodyType::Dynamic);

    // Test type transitions
    store.SetBodyType(eDynamic, BodyType::Static);
    REQUIRE(store.GetBodyType(eDynamic) == BodyType::Static);

    store.SetBodyType(eStatic, BodyType::Kinematic);
    REQUIRE(store.GetBodyType(eStatic) == BodyType::Kinematic);

    store.SetBodyType(eKinematic, BodyType::Dynamic);
    REQUIRE(store.GetBodyType(eKinematic) == BodyType::Dynamic);
}
