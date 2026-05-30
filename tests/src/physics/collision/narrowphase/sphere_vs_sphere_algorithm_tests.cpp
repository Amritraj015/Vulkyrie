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

    void AddPair(NarrowPhaseDataBatch &batch, LastFrameCollisionData &lastFrame,
                 SphereShape &s1, const TransformComponent &t1,
                 SphereShape &s2, const TransformComponent &t2,
                 bool reportContacts = true) {
        batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), s1, s2, t1, t2, reportContacts, lastFrame);
    }

} // namespace

// ===========================================================================================
// No collision
// ===========================================================================================

TEST_CASE("SphereVsSphere - separated spheres do not collide", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Centers are 5 units apart, radii sum to 2 — clearly separated.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({5.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

TEST_CASE("SphereVsSphere - spheres touching exactly do not collide", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Distance between centers equals exact sum of radii: strict < fails, no collision.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({2.0f, 0.0f, 0.0f}));

    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding);
}

// ===========================================================================================
// Collision detection
// ===========================================================================================

TEST_CASE("SphereVsSphere - overlapping spheres are detected as colliding", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Distance = 1.5, radius sum = 2.0 → overlap of 0.5.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({1.5f, 0.0f, 0.0f}));

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 1);
}

TEST_CASE("SphereVsSphere - penetration depth is correct", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Distance = 1.5, radius sum = 2.0 → penetrationDepth = 0.5.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({1.5f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.5f));
}

TEST_CASE("SphereVsSphere - contact normal points from shape one toward shape two", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Shape one at origin, shape two along +X.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({1.5f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.x == Catch::Approx(1.0f));
    REQUIRE(normal.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(normal.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("SphereVsSphere - asymmetric radii, penetration depth is correct", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(0.5f), s2(2.0f);

    // radius sum = 2.5, distance = 2.0 → penetrationDepth = 0.5.
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({2.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPoints[0].PenetrationDepth == Catch::Approx(0.5f));
}

// ===========================================================================================
// ReportContacts flag
// ===========================================================================================

TEST_CASE("SphereVsSphere - ReportContacts false: collision detected but no contact points", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({1.5f, 0.0f, 0.0f}),
                              /*reportContacts=*/false);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 1));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE(batch.Data[0].ContactPointCount == 0);
}

// ===========================================================================================
// Degenerate case
// ===========================================================================================

TEST_CASE("SphereVsSphere - coincident centers use world-up as contact normal", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lastFrame;
    SphereShape s1(1.0f), s2(1.0f);

    // Both spheres at the same position — degenerate, fall back to (0, 1, 0).
    AddPair(batch, lastFrame, s1, MakeTransform({0.0f, 0.0f, 0.0f}),
                              s2, MakeTransform({0.0f, 0.0f, 0.0f}));

    algo.PerformCollisionCheck(batch, 0, 1);

    REQUIRE(batch.Data[0].IsColliding);
    const glm::vec3 &normal = batch.Data[0].ContactPoints[0].WorldSpaceContactNormal;
    REQUIRE(normal.x == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(normal.y == Catch::Approx(1.0f));
    REQUIRE(normal.z == Catch::Approx(0.0f).margin(1e-5f));
}

// ===========================================================================================
// Batch processing
// ===========================================================================================

TEST_CASE("SphereVsSphere - batch with mixed colliding and non-colliding pairs", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1, lf2;
    SphereShape sA(1.0f), sB(1.0f), sC(1.0f), sD(1.0f), sE(1.0f), sF(1.0f);

    // Pair 0: overlapping (distance = 1.5 < 2.0)
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sA, sB,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.5f, 0.0f, 0.0f}),
                             true, lf0);

    // Pair 1: separated (distance = 10.0 > 2.0)
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sC, sD,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({10.0f, 0.0f, 0.0f}),
                             true, lf1);

    // Pair 2: overlapping (distance = 1.0 < 2.0)
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), sE, sF,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 1.0f, 0.0f}),
                             true, lf2);

    REQUIRE(algo.PerformCollisionCheck(batch, 0, 3));
    REQUIRE(batch.Data[0].IsColliding);
    REQUIRE_FALSE(batch.Data[1].IsColliding);
    REQUIRE(batch.Data[2].IsColliding);
}

TEST_CASE("SphereVsSphere - processing a sub-range of the batch", "[physics][narrowphase][sphere_vs_sphere]") {
    SphereVsSphereAlgorithm algo;
    NarrowPhaseDataBatch batch;
    LastFrameCollisionData lf0, lf1;
    SphereShape sA(1.0f), sB(1.0f), sC(1.0f), sD(1.0f);

    // Pair 0: overlapping — but outside the processed range.
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sA, sB,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.5f, 0.0f, 0.0f}),
                             true, lf0);

    // Pair 1: separated — the only pair in the processed range.
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sC, sD,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({10.0f, 0.0f, 0.0f}),
                             true, lf1);

    // Process only pair 1 — should return false (no collision in that range).
    REQUIRE_FALSE(algo.PerformCollisionCheck(batch, 1, 1));
    REQUIRE_FALSE(batch.Data[0].IsColliding); // untouched
    REQUIRE_FALSE(batch.Data[1].IsColliding);
}
