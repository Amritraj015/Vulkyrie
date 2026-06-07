#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>
#include "physics/collision/shapes/box_shape.h"

using namespace Vulkyrie;
using Catch::Approx;

// Note: reportContacts must be false for non-triangle shape pairs in debug builds.
// ComputeSmoothTriangleMeshContact asserts that at least one shape is a triangle.

namespace {

    TransformComponent MakeTransform(glm::vec3 position, glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
        TransformComponent t;
        t.Position = position;
        t.Rotation = rotation;
        return t;
    }

} // namespace

// ============================================================================
// Separation tests
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - upright capsule clearly above box", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f); // radius 0.5, half-height 1
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule at (0,5,0): bottom of capsule at y=3, box top at y=1 → gap = 2-0.5 = 1.5.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - upright capsule separated laterally", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({5.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - upright capsule separated on all axes", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({5.0f, 5.0f, 5.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Collision tests (GJK margin and SAT deep penetration paths)
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - capsule just touching box top (GJK margin path)", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 1.98f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - upright capsule deeply inside box (SAT path)", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.3f, 0.5f);
    BoxShape box(glm::vec3(2.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - capsule near box centre (deeply inside)", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(3.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Slight offset from box centre to avoid the degenerate GJK coincident-origin case.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 0.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - capsule colliding on lateral face of box", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule approaching from the X+ side: centre at (1.3,0,0), radius 0.5 → penetration 0.2.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({1.3f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Horizontal capsule (tests edge-edge axis candidates)
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - horizontal capsule above box, separated", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule lying along X, centre at (0,3,0) — clearly above box.
    glm::quat rot90z = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}, rot90z),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - horizontal capsule penetrating box top face", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule lying along X, centre at (0,1.3,0): bottom surface at y=0.8, inside top face (y=1).
    glm::quat rot90z = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 1.3f, 0.0f}, rot90z),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

// ============================================================================
// Shape ordering (reversed)
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - box as shapeOne, capsule as shapeTwo, colliding", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, capsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 2.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - box as shapeOne, capsule as shapeTwo, separated", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, capsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Batch processing
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - batch with mixed colliding and separated pairs", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lf0, lf1, lf2;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf0);
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), capsule, box,
                             MakeTransform({0.0f, 1.3f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf1);
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), capsule, box,
                             MakeTransform({-5.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf2);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    const bool anyCollision = algo.PerformCollisionCheck(batch, 0, 3, false);

    REQUIRE(anyCollision);
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[1].IsColliding);
    REQUIRE_FALSE(batch.Data[2].IsColliding);
}

// ============================================================================
// Rotation tests
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - capsule vs rotated box, separated", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - capsule vs rotated box, colliding", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 1.3f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Size extremes
// ============================================================================

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - thin capsule inside large box", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.05f, 0.2f);
    BoxShape box(glm::vec3(5.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 2.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("CapsuleVsConvexPolyhedronAlgorithm - fat capsule engulfing small box", "[physics][narrowphase][capsule_poly]") {
    PhysicsContext ctx;
    CapsuleShape capsule(2.0f, 0.5f);
    BoxShape box(glm::vec3(0.3f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 0.2f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    CapsuleVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}
