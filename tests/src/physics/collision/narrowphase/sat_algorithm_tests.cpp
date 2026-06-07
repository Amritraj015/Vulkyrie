#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>
#include "physics/collision/shapes/box_shape.h"

using namespace Vulkyrie;
using Catch::Approx;

// Note: reportContacts must be false for non-triangle shape pairs in debug builds.
// ComputeSmoothTriangleMeshContact asserts that at least one shape is a triangle,
// so contact point generation is only valid in tests that include triangle shapes.

namespace {

    TransformComponent MakeTransform(glm::vec3 position, glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
        TransformComponent t;
        t.Position = position;
        t.Rotation = rotation;
        return t;
    }

} // namespace

// ============================================================================
// SAT - Sphere vs Convex Polyhedron (direct SAT, bypassing GJK)
// ============================================================================

TEST_CASE("SATAlgorithm - sphere separated from box above top face", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Sphere at (0,3,0): bottom of sphere at y=2, top face of box at y=1 — gap = 1.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    const bool colliding = sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1);

    REQUIRE_FALSE(colliding);
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - sphere interpenetrating box through top face", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Sphere at (0,1.5,0): centre 0.5 above top face (y=1), radius 1 → 0.5 penetration.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    const bool colliding = sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1);

    REQUIRE(colliding);
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - sphere deeply inside box", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(0.3f);
    BoxShape box(glm::vec3(2.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 0.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - sphere separated on negative Y axis", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, -3.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
}

TEST_CASE("SATAlgorithm - sphere separated diagonally from box corner", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(0.5f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({2.0f, 2.0f, 2.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
}

TEST_CASE("SATAlgorithm - sphere vs box with reversed shape order (polyhedron as shapeOne)", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Box as shapeOne, sphere as shapeTwo — algorithm handles either ordering.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, sphere,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - sphere vs rotated box (45 deg Y), separated", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45y = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45y),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
}

TEST_CASE("SATAlgorithm - sphere vs rotated box (45 deg Y), colliding", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot45y = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot45y),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 1));
}

TEST_CASE("SATAlgorithm - sphere vs box, batch with multiple pairs", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    SphereShape sphere(1.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lf0, lf1, lf2;

    NarrowPhaseDataBatch batch;
    // Pair 0: separated.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf0);
    // Pair 1: colliding.
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sphere, box,
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf1);
    // Pair 2: separated on X.
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), sphere, box,
                             MakeTransform({3.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lf2);

    SATAlgorithm sat(false);
    const bool anyCollision = sat.PerformSphereVsConvexPolyhedronCollisionCheck(batch, 0, 3);

    REQUIRE(anyCollision);
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[1].IsColliding);
    REQUIRE_FALSE(batch.Data[2].IsColliding);
}

// ============================================================================
// SAT - Capsule vs Convex Polyhedron (direct SAT, bypassing GJK)
// ============================================================================

TEST_CASE("SATAlgorithm - upright capsule separated from box above top face", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f); // radius 0.5, half-height 1
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule at (0,3,0): segment from (0,2,0) to (0,4,0).
    // Closest segment point to top face (y=1): (0,2,0). Gap = 2-1-0.5 = 0.5 → separated.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - upright capsule interpenetrating box through top face", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule at (0,2,0): segment from (0,1,0) to (0,3,0).
    // Closest segment point to y=1 face: (0,1,0), on the face → penetration = radius 0.5.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 2.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - upright capsule deeply inside box", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.3f, 0.5f);
    BoxShape box(glm::vec3(2.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 0.5f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - horizontal capsule interpenetrating box top face (edge-edge axis)", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    // Capsule rotated 90° around Z (segment along X), centre at (0,1.3,0).
    // Bottom surface at y = 1.3 - 0.5 = 0.8, inside box top face at y=1 → collision.
    glm::quat rot90z = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 1.3f, 0.0f}, rot90z),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - horizontal capsule separated from box", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    glm::quat rot90z = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({0.0f, 3.0f, 0.0f}, rot90z),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
}

TEST_CASE("SATAlgorithm - capsule vs box with reversed shape order (polyhedron as shapeOne)", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), box, capsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 2.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
    REQUIRE(batch.Data[0].IsColliding);
}

TEST_CASE("SATAlgorithm - capsule separated on all three axes from box", "[physics][narrowphase][sat]") {
    PhysicsContext ctx;
    CapsuleShape capsule(0.5f, 2.0f);
    BoxShape box(glm::vec3(1.0f), ctx);
    LastFrameCollisionData lastFrame;

    NarrowPhaseDataBatch batch;
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule, box,
                             MakeTransform({5.0f, 5.0f, 5.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    SATAlgorithm sat(false);
    REQUIRE_FALSE(sat.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, 0));
}

// ============================================================================
// SAT - IsMinkowskiFaceCapsuleVsEdge
// ============================================================================

TEST_CASE("SATAlgorithm - IsMinkowskiFaceCapsuleVsEdge returns true when arcs intersect", "[physics][narrowphase][sat]") {
    SATAlgorithm sat(false);

    // Capsule segment along Y. Two adjacent face normals straddle the equatorial plane
    // of the capsule's Gauss map (one positive Y component, one negative) → arcs cross.
    glm::vec3 capsuleSegment(0.0f, 1.0f, 0.0f);
    glm::vec3 faceNormal1(1.0f, 0.5f, 0.0f);  // dot with segment > 0
    glm::vec3 faceNormal2(1.0f, -0.5f, 0.0f); // dot with segment < 0

    REQUIRE(sat.IsMinkowskiFaceCapsuleVsEdge(capsuleSegment, faceNormal1, faceNormal2));
}

TEST_CASE("SATAlgorithm - IsMinkowskiFaceCapsuleVsEdge returns false when both normals on same hemisphere", "[physics][narrowphase][sat]") {
    SATAlgorithm sat(false);

    // Both normals have positive Y component → same side of equatorial plane → arcs do not cross.
    glm::vec3 capsuleSegment(0.0f, 1.0f, 0.0f);
    glm::vec3 faceNormal1(1.0f, 0.5f, 0.0f);
    glm::vec3 faceNormal2(1.0f, 0.3f, 0.0f);

    REQUIRE_FALSE(sat.IsMinkowskiFaceCapsuleVsEdge(capsuleSegment, faceNormal1, faceNormal2));
}

TEST_CASE("SATAlgorithm - IsMinkowskiFaceCapsuleVsEdge returns false when one normal is orthogonal to segment", "[physics][narrowphase][sat]") {
    SATAlgorithm sat(false);

    // One normal exactly orthogonal → dot = 0 → product = 0, not strictly < 0 → false.
    glm::vec3 capsuleSegment(0.0f, 1.0f, 0.0f);
    glm::vec3 faceNormal1(1.0f, 0.0f, 0.0f); // dot = 0
    glm::vec3 faceNormal2(1.0f, -0.5f, 0.0f);

    REQUIRE_FALSE(sat.IsMinkowskiFaceCapsuleVsEdge(capsuleSegment, faceNormal1, faceNormal2));
}

TEST_CASE("SATAlgorithm - IsMinkowskiFaceCapsuleVsEdge with segment along X axis", "[physics][narrowphase][sat]") {
    SATAlgorithm sat(false);

    // Segment along X. Normals straddle the X=0 plane.
    glm::vec3 capsuleSegment(1.0f, 0.0f, 0.0f);
    glm::vec3 faceNormal1(0.6f, 0.8f, 0.0f);  // dot > 0
    glm::vec3 faceNormal2(-0.6f, 0.8f, 0.0f); // dot < 0

    REQUIRE(sat.IsMinkowskiFaceCapsuleVsEdge(capsuleSegment, faceNormal1, faceNormal2));
}
