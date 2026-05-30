#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;

namespace {

    TransformComponent MakeTransform(glm::vec3 position, glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
        TransformComponent t;
        t.Position = position;
        t.Rotation = rotation;
        return t;
    }

    // Adds a sphere (ShapeOne) vs capsule (ShapeTwo) pair.
    void AddSphereCapsulePair(NarrowPhaseDataBatch &batch, LastFrameCollisionData &lastFrame,
                              SphereShape &sphere, const TransformComponent &sphereTransform,
                              CapsuleShape &capsule, const TransformComponent &capsuleTransform,
                              bool reportContacts = true) {
        batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0),
                                 sphere, capsule,
                                 sphereTransform, capsuleTransform,
                                 reportContacts, lastFrame);
    }

    // Adds a capsule (ShapeOne) vs sphere (ShapeTwo) pair — tests the reversed-ordering path.
    void AddCapsuleSpherePair(NarrowPhaseDataBatch &batch, LastFrameCollisionData &lastFrame,
                              CapsuleShape &capsule, const TransformComponent &capsuleTransform,
                              SphereShape &sphere, const TransformComponent &sphereTransform,
                              bool reportContacts = true) {
        batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0),
                                 capsule, sphere,
                                 capsuleTransform, sphereTransform,
                                 reportContacts, lastFrame);
    }

} // namespace

// ===========================================================================================
// No collision
// ===========================================================================================

TEST_CASE("SphereVsCapsule - separated sphere and capsule do not collide", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Capsule at origin: height=2, radius=0.5 → extends ±1.5 on Y, ±0.5 on X/Z.
    // Sphere at (5, 0, 0) with radius 0.5: distance to capsule axis = 5 > radiusSum (1.0).
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({5.0f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

TEST_CASE("SphereVsCapsule - sphere touching capsule body exactly does not collide", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Sphere center is exactly at radiusSum (1.0) from the capsule axis — strict < fails.
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({1.0f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsCapsule - sphere touching capsule end cap exactly does not collide", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Capsule height=2 → half-height=1, segment end at (0,1,0).
    // Sphere at (0, 2, 0) with radius 0.5: distance from sphere center to segment end = 1.0 = radiusSum.
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 2.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

// ===========================================================================================
// Collision — sphere vs capsule body
// ===========================================================================================

TEST_CASE("SphereVsCapsule - sphere overlapping capsule body is detected", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Sphere center at (0.8, 0, 0), radius=0.5. Capsule axis along Y.
    // Closest point on segment to sphere = (0,0,0). Distance = 0.8 < radiusSum (1.0).
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.8f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("SphereVsCapsule - penetration depth for body overlap is correct", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Distance = 0.8, radiusSum = 1.0 → penetrationDepth = 0.2.
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.8f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.2f));
}

TEST_CASE("SphereVsCapsule - contact normal for body overlap points from sphere toward capsule axis", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Sphere at (+X), capsule at origin. vectorBetweenSphereAndCapsule points in -X → contactNormal = (-1, 0, 0).
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.8f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.x == Catch::Approx(-1.0f));
    REQUIRE(normal.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(normal.z == Catch::Approx(0.0f).margin(1e-5f));
}

// ===========================================================================================
// Collision — sphere vs capsule end cap
// ===========================================================================================

TEST_CASE("SphereVsCapsule - sphere overlapping capsule top end cap is detected", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Capsule height=2, half-height=1. Segment top end at (0,1,0).
    // Sphere at (0, 1.8, 0): distance from sphere center to segment end = 0.8 < radiusSum (1.0).
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 1.8f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("SphereVsCapsule - penetration depth for end cap overlap is correct", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Distance from sphere center (0,1.8,0) to segment end (0,1,0) = 0.8, radiusSum = 1.0 → depth = 0.2.
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 1.8f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.2f));
}

TEST_CASE("SphereVsCapsule - contact normal for end cap overlap points downward (toward capsule)", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Sphere above capsule top cap. vectorBetweenSphereAndCapsule = (0,1,0)-(0,1.8,0) = (0,-0.8,0) → normal = (0,-1,0).
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 1.8f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.x == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(normal.y == Catch::Approx(-1.0f));
    REQUIRE(normal.z == Catch::Approx(0.0f).margin(1e-5f));
}

// ===========================================================================================
// ReportContacts flag
// ===========================================================================================

TEST_CASE("SphereVsCapsule - ReportContacts false: collision detected but no contact points", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.8f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}),
                                          /*reportContacts=*/false);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

// ===========================================================================================
// Reversed shape ordering (capsule is ShapeOne, sphere is ShapeTwo)
// ===========================================================================================

TEST_CASE("SphereVsCapsule - capsule as ShapeOne detects collision", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddCapsuleSpherePair(batch, lastFrame, capsule, MakeTransform({0.0f, 0.0f, 0.0f}),
                                          sphere,  MakeTransform({0.8f, 0.0f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("SphereVsCapsule - capsule as ShapeOne: contact normal points from capsule toward sphere", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    // Capsule = ShapeOne at origin. Sphere = ShapeTwo at (+X, 0.8).
    // Normal should point from ShapeOne (capsule) toward ShapeTwo (sphere): +X direction.
    AddCapsuleSpherePair(batch, lastFrame, capsule, MakeTransform({0.0f, 0.0f, 0.0f}),
                                          sphere,  MakeTransform({0.8f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.x == Catch::Approx(1.0f));
    REQUIRE(normal.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(normal.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("SphereVsCapsule - capsule as ShapeOne: penetration depth matches sphere-as-ShapeOne", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;

    SphereShape sphere(0.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    // Sphere first
    NarrowPhaseDataBatch batchA;
    LastFrameCollisionData lfA;
    AddSphereCapsulePair(batchA, lfA, sphere, MakeTransform({0.8f, 0.0f, 0.0f}),
                                     capsule, MakeTransform({0.0f, 0.0f, 0.0f}));
    algo.PerformCollisionCheck(batchA, 0, 1);

    // Capsule first
    NarrowPhaseDataBatch batchB;
    LastFrameCollisionData lfB;
    AddCapsuleSpherePair(batchB, lfB, capsule, MakeTransform({0.0f, 0.0f, 0.0f}),
                                     sphere,  MakeTransform({0.8f, 0.0f, 0.0f}));
    algo.PerformCollisionCheck(batchB, 0, 1);

    REQUIRE(batchA.Data[0].ContactPoints[0].PenetrationDepth ==
            Catch::Approx(batchB.Data[0].ContactPoints[0].PenetrationDepth));
}

// ===========================================================================================
// Degenerate case — sphere center on capsule inner segment
// ===========================================================================================

TEST_CASE("SphereVsCapsule - sphere center on capsule axis uses perpendicular contact normal", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Sphere center at (0,0,0) — exactly on the capsule's Y axis.
    // distanceSquared = 0 ≤ VE_MACHINE_EPSILON → degenerate path.
    // Capsule segment direction = (0,1,0). cosA1 = |dot((1,0,0),(0,1,0))| = 0,
    // cosA2 = |dot((0,1,0),(0,1,0))| = 1 → pick vec1 → cross((0,1,0),(1,0,0)) = (0,0,-1) → normal perpendicular to Y.
    SphereShape sphere(1.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);

    // The contact normal must be perpendicular to the capsule's Y axis (no Y component).
    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.y == Catch::Approx(0.0f).margin(1e-5f));

    // Must be a unit vector.
    REQUIRE(glm::length(normal) == Catch::Approx(1.0f));
}

TEST_CASE("SphereVsCapsule - degenerate case penetration depth equals full radius sum", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // penetrationDepth = radiusSum in degenerate path.
    SphereShape sphere(1.5f);
    CapsuleShape capsule(0.5f, 2.0f);

    AddSphereCapsulePair(batch, lastFrame, sphere, MakeTransform({0.0f, 0.0f, 0.0f}),
                                          capsule, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(2.0f)); // 1.5 + 0.5
}

// ===========================================================================================
// Batch processing
// ===========================================================================================

TEST_CASE("SphereVsCapsule - batch with mixed colliding and non-colliding pairs", "[physics][narrowphase][sphere_vs_capsule]") {
    SphereVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1, lf2;

    SphereShape sA(0.5f), sB(0.5f), sC(0.5f);
    CapsuleShape cA(0.5f, 2.0f), cB(0.5f, 2.0f), cC(0.5f, 2.0f);

    // Pair 0: overlapping (sphere body overlap)
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sA, cA,
                             MakeTransform({0.8f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             true, lf0);

    // Pair 1: separated
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sB, cB,
                             MakeTransform({10.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             true, lf1);

    // Pair 2: overlapping (end cap overlap)
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), sC, cC,
                             MakeTransform({0.0f, 1.8f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             true, lf2);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 3));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE_FALSE(batch.Data[1].IsColliding);
    REQUIRE(batch.Data[2].IsColliding);
}
