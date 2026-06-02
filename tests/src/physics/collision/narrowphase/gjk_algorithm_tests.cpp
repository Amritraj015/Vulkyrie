#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;
using Catch::Approx;

// Helper function to create a transform
TransformComponent MakeTransform(glm::vec3 position, glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
    TransformComponent t;
    t.Position = position;
    t.Rotation = rotation;
    return t;
}


// ============================================================================
// Basic Separation Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Two separated spheres", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    // Create two spheres separated by distance
    SphereShape sphere1(1.0f); // radius 1
    SphereShape sphere2(1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}), // Far apart
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Two separated capsules", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f); // radius 0.5, height 2.0
    CapsuleShape capsule2(0.5f, 2.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}), // Separated
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

// ============================================================================
// Collision Tests (Shallow Penetration / Margin Collision)
// ============================================================================

TEST_CASE("GJKAlgorithm - Spheres touching in margins", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Spheres very close - distance 1.98, radii sum to 2.0
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.98f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Capsules overlapping", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Capsules slightly overlapping
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.95f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Deep Penetration Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Spheres deeply interpenetrating", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Significant overlap - distance 0.5, radii sum to 2.0
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.5f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    // Deep overlap, but GJK can still compute closest points
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Capsules deeply overlapping", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Deep penetration
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.3f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    // Deep overlap, but GJK can still compute closest points
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Rotation Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Rotated capsules separated", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // 45-degree rotation around Z axis
    glm::quat rot45 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}, rot45),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Rotated spheres colliding", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Rotation doesn't matter for spheres, but test the code path
    glm::quat rot1 = glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat rot2 = glm::angleAxis(glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}, rot1),
                             MakeTransform({1.5f, 0.0f, 0.0f}, rot2),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Mixed Shape Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Sphere and capsule separated", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere(1.0f);
    CapsuleShape capsule(0.5f, 2.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, capsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Sphere and capsule colliding", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere(1.0f);
    CapsuleShape capsule(0.5f, 2.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere, capsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.3f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("GJKAlgorithm - Spheres at same position", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Exact same position - maximum penetration
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Interpenetrate);
}

TEST_CASE("GJKAlgorithm - Very small spheres", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(0.01f);  // Very small radius
    SphereShape sphere2(0.01f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.5f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Large distance separation", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Very far apart
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1000.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

// ============================================================================
// Batch Processing Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Multiple collision pairs in batch", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame1, lastFrame2, lastFrame3;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);
    SphereShape sphere3(1.0f);

    // Pair 1: Separated
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}),
                             false, lastFrame1);

    // Pair 2: Colliding
    batch.AddNarrowPhaseData(1, Entity(2, 0), Entity(3, 0), sphere1, sphere3,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.5f, 0.0f, 0.0f}),
                             false, lastFrame2);

    // Pair 3: Separated
    batch.AddNarrowPhaseData(2, Entity(4, 0), Entity(5, 0), sphere2, sphere3,
                             MakeTransform({10.0f, 0.0f, 0.0f}),
                             MakeTransform({15.0f, 0.0f, 0.0f}),
                             false, lastFrame3);

    gjk.PerformCollisionCheck(batch, 0, 3, results);

    REQUIRE(results.size() == 3);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
    REQUIRE((results[1] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[1] == GJKAlgorithm::GJKResult::Interpenetrate));
    REQUIRE(results[2] == GJKAlgorithm::GJKResult::Separated);
}

// ============================================================================
// Frame Coherence Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Frame coherence with cached separating axis", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // First frame - no cached data
    lastFrame.IsValid = false;
    lastFrame.WasUsingGJKAlgorithm = false;

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);

    // Now lastFrame should have cached separating axis
    // Second check should use frame coherence
    batch.Data.clear();
    results.clear();

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({5.1f, 0.0f, 0.0f}),  // Slightly different position
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

// ============================================================================
// ReportContacts Flag Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - ReportContacts flag false", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // ReportContacts = false
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.98f, 0.0f, 0.0f}),
                             false, lastFrame);  // Report contacts = false

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    // Should still detect collision but not generate contact points
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Different Axes Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Y-axis collision spheres", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 1.5f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Z-axis collision spheres", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 0.0f, 1.5f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Diagonal (1,1,0) collision", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.0f, 1.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Diagonal (1,1,1) collision", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.8f, 0.8f, 0.8f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Capsule Orientation Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Perpendicular capsules colliding", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Rotate second capsule 90 degrees around Z to make it horizontal
    glm::quat rot90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.8f, 0.0f, 0.0f}, rot90),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - End-to-end capsules separated", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Position capsules end-to-end along Y axis
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 5.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - End-to-end capsules touching", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Position capsules almost touching end-to-end (height/2 + radius = 1.5 each)
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.0f, 2.8f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Side-to-end capsule collision", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape capsule1(0.5f, 2.0f);
    CapsuleShape capsule2(0.5f, 2.0f);

    // Rotate second capsule horizontal
    glm::quat rot90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), capsule1, capsule2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.8f, 1.0f, 0.0f}, rot90),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Exact Boundary Cases
// ============================================================================

TEST_CASE("GJKAlgorithm - Spheres exactly at margin boundary", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Distance exactly equals sum of radii (2.0)
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({2.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    // At exact boundary - could be separated or touching in margin
}

TEST_CASE("GJKAlgorithm - Spheres just barely separated", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Distance just slightly more than sum of radii
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({2.0001f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Spheres just barely touching", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape sphere1(1.0f);
    SphereShape sphere2(1.0f);

    // Distance just slightly less than sum of radii
    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), sphere1, sphere2,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({1.9999f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

// ============================================================================
// Mismatched Size Tests
// ============================================================================

TEST_CASE("GJKAlgorithm - Large sphere vs tiny sphere separated", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape largeSphere(10.0f);
    SphereShape tinySphere(0.1f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), largeSphere, tinySphere,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({15.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == GJKAlgorithm::GJKResult::Separated);
}

TEST_CASE("GJKAlgorithm - Large sphere vs tiny sphere colliding", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    SphereShape largeSphere(10.0f);
    SphereShape tinySphere(0.1f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), largeSphere, tinySphere,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({9.0f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}

TEST_CASE("GJKAlgorithm - Tall capsule vs short capsule", "[physics][narrowphase][gjk][gjk_algorithm]") {
    GJKAlgorithm gjk;
    NarrowPhaseDataBatch batch;
    std::vector<GJKAlgorithm::GJKResult> results;
    LastFrameCollisionData lastFrame;

    CapsuleShape tallCapsule(0.5f, 10.0f);
    CapsuleShape shortCapsule(0.5f, 1.0f);

    batch.AddNarrowPhaseData(0, Entity(0, 0), Entity(1, 0), tallCapsule, shortCapsule,
                             MakeTransform({0.0f, 0.0f, 0.0f}),
                             MakeTransform({0.8f, 0.0f, 0.0f}),
                             false, lastFrame);

    gjk.PerformCollisionCheck(batch, 0, 1, results);

    REQUIRE(results.size() == 1);
    REQUIRE((results[0] == GJKAlgorithm::GJKResult::CollideInMargin ||
             results[0] == GJKAlgorithm::GJKResult::Interpenetrate));
}
