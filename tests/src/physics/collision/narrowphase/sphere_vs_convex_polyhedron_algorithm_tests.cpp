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

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere clearly above box, separated", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere separated laterally from box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({5.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere separated diagonally from box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(0.5f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({3.0f, 3.0f, 3.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere very far from box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({100.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Collision tests (GJK margin and SAT deep penetration paths)
// ============================================================================

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere just touching box top face (GJK margin path)", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 1.98f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere deeply interpenetrating box (SAT path)", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Sphere centre at (0,0.5,0): only 0.5 above top face, deep penetration triggers SAT.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 0.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere centre near box centre", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(0.5f);
    BoxShape box(glm::vec3(2.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Sphere offset slightly from the box centre to avoid the degenerate exactly-coincident case.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 0.3f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere colliding on lateral face", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({1.5f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Shape ordering (reversed)
// ============================================================================

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - box as shapeOne, sphere as shapeTwo, colliding", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, sphere,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - box as shapeOne, sphere as shapeTwo, separated", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, sphere,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Batch processing
// ============================================================================

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - batch with mixed colliding and separated pairs", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lf0, lf1, lf2;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf0);
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sphere, box,
                             MakeTransform({0.0f, 0.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf1);
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), sphere, box,
                             MakeTransform({-5.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf2);

    SphereVsConvexPolyhedronAlgorithm algo;
    const bool anyCollision = algo.PerformCollisionCheck(batch, 0, 3, false);

    REQUIRE(anyCollision);
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[1].IsColliding);
    REQUIRE_FALSE(batch.Data[2].IsColliding);
}

// ============================================================================
// Rotation tests
// ============================================================================

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere vs 45-degree rotated box, separated", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - sphere vs 45-degree rotated box, colliding", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

// ============================================================================
// Size extremes
// ============================================================================

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - tiny sphere inside large box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(0.01f);
    BoxShape box(glm::vec3(10.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Sphere offset from box centre to avoid degenerate GJK simplex when both shapes share the exact same origin.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - large sphere engulfing small box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(5.0f);
    BoxShape box(glm::vec3(0.5f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1, false));
}

TEST_CASE("SphereVsConvexPolyhedronAlgorithm - large sphere separated from small box", "[physics][narrowphase][sphere_poly]") {
    PhysicsContext ctx;
    SphereShape sphere(5.0f);
    BoxShape box(glm::vec3(0.5f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({20.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SphereVsConvexPolyhedronAlgorithm algo;
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1, false));
}
