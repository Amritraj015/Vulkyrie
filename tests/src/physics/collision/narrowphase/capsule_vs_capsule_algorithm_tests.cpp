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

    // Rotation that aligns the capsule's Y axis (its symmetry axis) with the world X axis.
    // i.e., 90 degrees around the world Z axis.
    glm::quat RotZ90() {
        return glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void AddCapsulePair(NarrowPhaseDataBatch &batch, LastFrameCollisionData &lastFrame,
                        CapsuleShape &c1, const TransformComponent &t1,
                        CapsuleShape &c2, const TransformComponent &t2,
                        bool reportContacts = true) {
        batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), c1, c2, t1, t2, reportContacts, lastFrame);
    }

} // namespace

// ===========================================================================================
// No collision
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - parallel capsules with perpendicular distance greater than radius sum do not collide",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Both capsules along Y, 2 units apart on X. radiusSum = 1.0 < 2.0.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({2.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

TEST_CASE("CapsuleVsCapsule - parallel capsules touching exactly at radius sum do not collide",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Perpendicular distance = 1.0 = radiusSum exactly. Strict < fails, so no collision.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({1.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsCapsule - non-parallel capsules with segments far apart do not collide",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1 along Y at origin. c2 along X (rotated 90 deg around Z) centered at (3,0,0).
    // c2 inner segment in world: (2,0,0) to (4,0,0). Closest approach to c1 segment: ~2 units.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({3.0f, 0.0f, 0.0f}, RotZ90()));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

TEST_CASE("CapsuleVsCapsule - parallel capsules with no longitudinal overlap and endpoints too far do not collide",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1: segment Y[-1, 1]. c2 at (0.8, 2.9, 0): segment Y[1.9, 3.9].
    // The parallel path falls through (no longitudinal overlap), so closest-point path runs.
    // Closest point on c1 is its top (0,1,0), on c2 is its bottom (0.8,1.9,0).
    // Distance = sqrt(0.64 + 0.81) ≈ 1.205 > radiusSum = 1.0.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 2.9f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

// ===========================================================================================
// Collision — parallel segments overlapping
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - parallel capsules with full segment overlap are detected as colliding",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Perpendicular distance = 0.8, radiusSum = 1.0 → overlap.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 0.0f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsCapsule - parallel fully-overlapping capsules produce two contact points",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    REQUIRE(batch.Data[0].ContactPointCount == 2);
}

TEST_CASE("CapsuleVsCapsule - parallel fully-overlapping capsules penetration depth is correct",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // distance = 0.8, radiusSum = 1.0 → penetration = 0.2
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.2f).margin(1e-5f));
    REQUIRE(batch.Data[0].ContactPoints[1].PenetrationDepth == Catch::Approx(0.2f).margin(1e-5f));
}

TEST_CASE("CapsuleVsCapsule - parallel fully-overlapping capsules contact normal points from shape one to shape two",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1 at origin, c2 at (0.8, 0, 0). Normal should point from c1 to c2: (+1, 0, 0).
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    const glm::vec3 &n = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(n.x == Catch::Approx(1.0f));
    REQUIRE(n.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(n.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("CapsuleVsCapsule - parallel capsules with partial segment overlap produce two contact points",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1: segment Y[-1, 1]. c2 at (0.8, 1.5, 0): segment Y[0.5, 2.5].
    // Overlap region Y[0.5, 1.0]. Perpendicular distance = 0.8, radiusSum = 1.0.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 1.5f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 2);

    // Same perpendicular separation, same penetration depth.
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.2f).margin(1e-5f));
}

// ===========================================================================================
// Collision — parallel, degenerate (coaxial segments)
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - coaxial capsules (same axis) are detected as colliding",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Both capsules along Y at X=0, Z=0, staggered by 0.5 in Y so segments overlap.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.5f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 2);
}

TEST_CASE("CapsuleVsCapsule - coaxial capsules contact normal is perpendicular to capsule axis",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Perpendicular distance = 0: degenerate parallel path. The normal must be perpendicular
    // to the Y axis (the capsule axis) and must be a unit vector.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.5f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    const glm::vec3 &n = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(n.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(glm::length(n) == Catch::Approx(1.0f).margin(1e-5f));
}

TEST_CASE("CapsuleVsCapsule - coaxial capsules penetration depth equals radius sum",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // depth = radiusSum - sqrt(0) = 1.0
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.5f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(1.0f).margin(1e-5f));
}

// ===========================================================================================
// Collision — non-parallel segments, general case
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - non-parallel capsules with inner segments close enough are detected as colliding",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1 along Y at origin. c2 along X (rotated 90 deg around Z) at (0.3, 0, 0.1).
    // Closest points on the inner segments are 0.1 apart in Z → distance = 0.1 < radiusSum = 1.0.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 0.0f, 0.1f}, RotZ90()));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("CapsuleVsCapsule - non-parallel capsules general case penetration depth is correct",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // Closest segment distance = 0.1 (Z separation only). depth = radiusSum - dist = 1.0 - 0.1 = 0.9.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 0.0f, 0.1f}, RotZ90()));

    algo.PerformCollisionCheck(batch, 0, 1);
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.9f).margin(1e-4f));
}

TEST_CASE("CapsuleVsCapsule - non-parallel capsules general case contact normal is a unit vector",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 0.0f, 0.1f}, RotZ90()));

    algo.PerformCollisionCheck(batch, 0, 1);
    const glm::vec3 &n = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(glm::length(n) == Catch::Approx(1.0f).margin(1e-5f));
}

// ===========================================================================================
// Collision — non-parallel segments, degenerate (crossing at a point)
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - perpendicular capsules crossing at their axes detect collision",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1 along Y at origin, c2 along X at origin. Inner segments cross at (0,0,0).
    // closestPointsDistanceSquare = 0 → degenerate non-parallel path.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.0f, 0.0f}, RotZ90()));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("CapsuleVsCapsule - perpendicular capsules crossing at axes: contact normal is unit vector perpendicular to both axes",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // seg1 = Y direction, seg2 = X direction. cross(Y,X) = -Z.
    // normal in cap2 space = normalize(cross((0,2,0),(2,0,0))) = normalize((0,0,-4)) = (0,0,-1).
    // normalWorld = Rot_Z(90°) * (0,0,-1) = (0,0,-1) (Z unchanged).
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.0f, 0.0f}, RotZ90()));

    algo.PerformCollisionCheck(batch, 0, 1);
    const glm::vec3 &n = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(glm::length(n) == Catch::Approx(1.0f).margin(1e-5f));

    // Must be perpendicular to both capsule axes (Y and X in world space).
    // dot(n, Y) = n.y must be ~0, dot(n, X) = n.x must be ~0.
    REQUIRE(std::abs(n.x) == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(std::abs(n.y) == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("CapsuleVsCapsule - perpendicular capsules crossing at axes: penetration depth equals radius sum",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.0f, 0.0f, 0.0f}, RotZ90()));

    algo.PerformCollisionCheck(batch, 0, 1);
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(1.0f).margin(1e-5f));
}

// ===========================================================================================
// Collision — closest-point path (parallel, no longitudinal overlap, but endpoints close)
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - parallel capsules with no longitudinal overlap but close endpoints collide",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // c1: segment Y[-1, 1]. c2 at (0.3, 2.5, 0): segment Y[1.5, 3.5].
    // The parallel path finds no longitudinal overlap (t2 < 0) and falls to closest-point path.
    // Closest: c1 top (0,1,0) and c2 bottom (0.3,1.5,0). Distance = sqrt(0.09+0.25) ≈ 0.583 < 1.0.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 2.5f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("CapsuleVsCapsule - parallel capsules endpoint-to-endpoint: contact normal is a unit vector",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 2.5f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    const glm::vec3 &n = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(glm::length(n) == Catch::Approx(1.0f).margin(1e-5f));
}

TEST_CASE("CapsuleVsCapsule - parallel capsules endpoint-to-endpoint: penetration depth is positive",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    // distance ≈ 0.583, radiusSum = 1.0 → depth ≈ 0.417.
    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 2.5f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);
    const f32 depth = batch.Data[0].ContactPoints[0].PenetrationDepth;
    REQUIRE(depth > 0.0f);
    REQUIRE(depth == Catch::Approx(1.0f - std::sqrt(0.09f + 0.25f)).margin(1e-4f));
}

// ===========================================================================================
// ReportContacts flag
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - ReportContacts false: collision detected but no contact points generated",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.8f, 0.0f, 0.0f}),
                                    /*reportContacts=*/false);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

TEST_CASE("CapsuleVsCapsule - ReportContacts false works for non-parallel collision",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;

    CapsuleShape c1(0.5f, 2.0f), c2(0.5f, 2.0f);
    AddCapsulePair(batch, lastFrame, c1, MakeTransform({0.0f, 0.0f, 0.0f}),
                                    c2, MakeTransform({0.3f, 0.0f, 0.1f}, RotZ90()),
                                    /*reportContacts=*/false);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

// ===========================================================================================
// Batch processing
// ===========================================================================================

TEST_CASE("CapsuleVsCapsule - batch with all colliding pairs returns true",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1, lf2;

    CapsuleShape c(0.5f, 2.0f);

    // Pair 0: parallel overlap
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.8f, 0.0f, 0.0f}),
                             true, lf0);
    // Pair 1: non-parallel close
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.3f, 0.0f, 0.1f}, RotZ90()),
                             true, lf1);
    // Pair 2: coaxial
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.0f, 0.5f, 0.0f}),
                             true, lf2);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 3));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[1].IsColliding);
    REQUIRE(batch.Data[2].IsColliding);
}

TEST_CASE("CapsuleVsCapsule - batch with mixed colliding and non-colliding pairs",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1, lf2;

    CapsuleShape c(0.5f, 2.0f);

    // Pair 0: colliding
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.8f, 0.0f, 0.0f}),
                             true, lf0);
    // Pair 1: not colliding
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({2.0f, 0.0f, 0.0f}),
                             true, lf1);
    // Pair 2: colliding
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.0f, 0.5f, 0.0f}),
                             true, lf2);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 3));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE_FALSE(batch.Data[1].IsColliding);
    REQUIRE(batch.Data[2].IsColliding);
}

TEST_CASE("CapsuleVsCapsule - batch with all non-colliding pairs returns false",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1;

    CapsuleShape c(0.5f, 2.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({2.0f, 0.0f, 0.0f}),
                             true, lf0);
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({3.0f, 0.0f, 0.0f}, RotZ90()),
                             true, lf1);

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 2));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE_FALSE(batch.Data[1].IsColliding);
}

TEST_CASE("CapsuleVsCapsule - batch subset: batchStartIndex and batchItemsCount are respected",
          "[physics][narrowphase][capsule_vs_capsule]") {
    CapsuleVsCapsuleAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1, lf2;

    CapsuleShape c(0.5f, 2.0f);

    // Pair 0: colliding — should be skipped by batchStartIndex=1
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.8f, 0.0f, 0.0f}),
                             true, lf0);
    // Pair 1: not colliding — the only pair processed
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({2.0f, 0.0f, 0.0f}),
                             true, lf1);
    // Pair 2: colliding — should be skipped by batchItemsCount=1
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), c, c,
                             MakeTransform({0.0f, 0.0f, 0.0f}), MakeTransform({0.0f, 0.5f, 0.0f}),
                             true, lf2);

    // Only process pair 1 (the non-colliding one).
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 1, 1));
    REQUIRE_FALSE(batch.Data[1].IsColliding);
    // Pairs 0 and 2 must remain untouched.
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE_FALSE(batch.Data[2].IsColliding);
}
