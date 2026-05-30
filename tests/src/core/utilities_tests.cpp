#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;

// ===========================================================================================
// AreParallelVectors
// ===========================================================================================

TEST_CASE("AreParallelVectors - identical vectors are parallel", "[core][utilities]") {
    REQUIRE(AreParallelVectors(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST_CASE("AreParallelVectors - opposite direction vectors are parallel", "[core][utilities]") {
    REQUIRE(AreParallelVectors(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
}

TEST_CASE("AreParallelVectors - parallel vectors with different magnitudes are parallel", "[core][utilities]") {
    REQUIRE(AreParallelVectors(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(5.0f, 0.0f, 0.0f)));
}

TEST_CASE("AreParallelVectors - diagonal vectors in same direction are parallel", "[core][utilities]") {
    REQUIRE(AreParallelVectors(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(3.0f, 3.0f, 0.0f)));
}

TEST_CASE("AreParallelVectors - perpendicular vectors are not parallel", "[core][utilities]") {
    REQUIRE_FALSE(AreParallelVectors(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
}

TEST_CASE("AreParallelVectors - arbitrary non-parallel vectors are not parallel", "[core][utilities]") {
    REQUIRE_FALSE(AreParallelVectors(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    REQUIRE_FALSE(AreParallelVectors(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(3.0f, 2.0f, 1.0f)));
}

TEST_CASE("AreParallelVectors - zero vector is considered parallel to any vector", "[core][utilities]") {
    // cross(zero, v) = zero, length2(zero) = 0 < epsilon^2 -> true
    REQUIRE(AreParallelVectors(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 2.0f, 3.0f)));
    REQUIRE(AreParallelVectors(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
}

// ===========================================================================================
// ComputeClosestPointOnLineSegment
// ===========================================================================================

TEST_CASE("ComputeClosestPointOnLineSegment - point projecting onto interior of segment returns foot of perpendicular",
          "[core][utilities]") {
    // Segment along X from (0,0,0) to (4,0,0). Point (2,3,0) projects to (2,0,0).
    glm::vec3 result = ComputeClosestPointOnLineSegment({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {2.0f, 3.0f, 0.0f});
    REQUIRE(result.x == Catch::Approx(2.0f));
    REQUIRE(result.y == Catch::Approx(0.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("ComputeClosestPointOnLineSegment - point projecting before segment start is clamped to start",
          "[core][utilities]") {
    // Point (-5,1,0) projects to x=-5, which is before start (0,0,0). Clamped to (0,0,0).
    glm::vec3 result = ComputeClosestPointOnLineSegment({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {-5.0f, 1.0f, 0.0f});
    REQUIRE(result.x == Catch::Approx(0.0f));
    REQUIRE(result.y == Catch::Approx(0.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("ComputeClosestPointOnLineSegment - point projecting past segment end is clamped to end",
          "[core][utilities]") {
    // Point (10,2,0) projects to x=10, past end (4,0,0). Clamped to (4,0,0).
    glm::vec3 result = ComputeClosestPointOnLineSegment({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {10.0f, 2.0f, 0.0f});
    REQUIRE(result.x == Catch::Approx(4.0f));
    REQUIRE(result.y == Catch::Approx(0.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("ComputeClosestPointOnLineSegment - point on the segment itself is returned unchanged",
          "[core][utilities]") {
    // Point exactly on segment at (2,0,0).
    glm::vec3 result = ComputeClosestPointOnLineSegment({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f});
    REQUIRE(result.x == Catch::Approx(2.0f));
    REQUIRE(result.y == Catch::Approx(0.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("ComputeClosestPointOnLineSegment - degenerate segment (start == end) returns that point",
          "[core][utilities]") {
    glm::vec3 result = ComputeClosestPointOnLineSegment({3.0f, 1.0f, 2.0f}, {3.0f, 1.0f, 2.0f}, {10.0f, 5.0f, 7.0f});
    REQUIRE(result.x == Catch::Approx(3.0f));
    REQUIRE(result.y == Catch::Approx(1.0f));
    REQUIRE(result.z == Catch::Approx(2.0f));
}

TEST_CASE("ComputeClosestPointOnLineSegment - works correctly with a diagonal 3D segment",
          "[core][utilities]") {
    // Segment from (0,0,0) to (2,2,2). Point (1,0,0): projection t = dot((1,0,0),(2,2,2)) / 12 = 2/12 = 1/6.
    // Closest = (2/6, 2/6, 2/6) = (1/3, 1/3, 1/3).
    const f32 expected = 1.0f / 3.0f;
    glm::vec3 result = ComputeClosestPointOnLineSegment({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}, {1.0f, 0.0f, 0.0f});
    REQUIRE(result.x == Catch::Approx(expected).margin(1e-5f));
    REQUIRE(result.y == Catch::Approx(expected).margin(1e-5f));
    REQUIRE(result.z == Catch::Approx(expected).margin(1e-5f));
}

// ===========================================================================================
// ComputeDistanceSquaredPointToLine
// ===========================================================================================

TEST_CASE("ComputeDistanceSquaredPointToLine - point on the line has zero distance", "[core][utilities]") {
    // Point (2,0,0) lies on the line through (0,0,0) and (4,0,0).
    f32 result = ComputeDistanceSquaredPointToLine({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f});
    REQUIRE(result == Catch::Approx(0.0f).margin(1e-10f));
}

TEST_CASE("ComputeDistanceSquaredPointToLine - point perpendicular to line midpoint gives correct squared distance",
          "[core][utilities]") {
    // Line along X. Point (2,3,0): perpendicular distance = 3, squared = 9.
    f32 result = ComputeDistanceSquaredPointToLine({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {2.0f, 3.0f, 0.0f});
    REQUIRE(result == Catch::Approx(9.0f).margin(1e-5f));
}

TEST_CASE("ComputeDistanceSquaredPointToLine - distance is independent of projection along the line",
          "[core][utilities]") {
    // Both points are 3 units away from the line (X axis), just at different X positions.
    f32 r1 = ComputeDistanceSquaredPointToLine({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f});
    f32 r2 = ComputeDistanceSquaredPointToLine({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {100.0f, 3.0f, 0.0f});
    REQUIRE(r1 == Catch::Approx(9.0f).margin(1e-5f));
    REQUIRE(r2 == Catch::Approx(9.0f).margin(1e-4f));
}

TEST_CASE("ComputeDistanceSquaredPointToLine - line through arbitrary 3D points gives correct distance",
          "[core][utilities]") {
    // Line along Y axis (through origin and (0,1,0)). Point (3,5,4): perpendicular dist = sqrt(9+16)=5, distSq=25.
    f32 result = ComputeDistanceSquaredPointToLine({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {3.0f, 5.0f, 4.0f});
    REQUIRE(result == Catch::Approx(25.0f).margin(1e-5f));
}

TEST_CASE("ComputeDistanceSquaredPointToLine - degenerate line (start == end) returns squared distance to the point",
          "[core][utilities]") {
    // Degenerate line: both endpoints at (1,0,0). Point at (4,0,0): distance = 3, distSq = 9.
    f32 result = ComputeDistanceSquaredPointToLine({1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f});
    REQUIRE(result == Catch::Approx(9.0f).margin(1e-5f));
}

// ===========================================================================================
// ComputePlaneSegmentIntersection
// ===========================================================================================

TEST_CASE("ComputePlaneSegmentIntersection - segment crossing plane perpendicularly at midpoint returns t=0.5",
          "[core][utilities]") {
    // Segment (0,-1,0) to (0,1,0). Plane Y=0 (normal=(0,1,0), d=0).
    // t = (0 - dot((0,1,0),(0,-1,0))) / dot((0,1,0),(0,2,0)) = 1/2 = 0.5.
    f32 t = ComputePlaneSegmentIntersection({0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(0.5f));
}

TEST_CASE("ComputePlaneSegmentIntersection - segment parallel to plane returns -1",
          "[core][utilities]") {
    // Segment (0,0,0) to (1,0,0) is parallel to plane Y=1 (normal=(0,1,0), d=1).
    f32 t = ComputePlaneSegmentIntersection({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(-1.0f));
}

TEST_CASE("ComputePlaneSegmentIntersection - segment start exactly on plane returns t=0",
          "[core][utilities]") {
    // Segment (0,0,0) to (0,2,0). Plane Y=0 (d=0). t = (0-0)/2 = 0.
    f32 t = ComputePlaneSegmentIntersection({0.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 0.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(0.0f));
}

TEST_CASE("ComputePlaneSegmentIntersection - segment end exactly on plane returns t=1",
          "[core][utilities]") {
    // Segment (0,0,0) to (0,1,0). Plane Y=1. t = (1-0)/1 = 1.
    f32 t = ComputePlaneSegmentIntersection({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 1.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(1.0f));
}

TEST_CASE("ComputePlaneSegmentIntersection - intersection beyond segment end returns t greater than 1",
          "[core][utilities]") {
    // Segment (0,0,0) to (0,0.5,0). Plane Y=1. t = (1-0)/0.5 = 2.0.
    f32 t = ComputePlaneSegmentIntersection({0.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}, 1.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(2.0f));
}

TEST_CASE("ComputePlaneSegmentIntersection - intersection before segment start returns negative t",
          "[core][utilities]") {
    // Segment (0,1,0) to (0,2,0). Plane Y=0 (below start). t = (0-1)/1 = -1.
    f32 t = ComputePlaneSegmentIntersection({0.0f, 1.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 0.0f, {0.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(-1.0f));
}

TEST_CASE("ComputePlaneSegmentIntersection - works with non-axis-aligned plane",
          "[core][utilities]") {
    // Segment (0,0,0) to (2,2,0). Plane: normal=(1,1,0)/sqrt(2), d = dot(normal,(1,1,0)) = 2/sqrt(2) = sqrt(2).
    // Use unnormalized normal (1,1,0) with d=2: dot(n,A)=0, dot(n,B-A)=4. t=(2-0)/4=0.5.
    f32 t = ComputePlaneSegmentIntersection({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, 2.0f, {1.0f, 1.0f, 0.0f});
    REQUIRE(t == Catch::Approx(0.5f));
}

// ===========================================================================================
// ComputeClosestPointBetweenTwoSegments
// ===========================================================================================

TEST_CASE("ComputeClosestPointBetweenTwoSegments - intersecting segments have coincident closest points",
          "[core][utilities]") {
    // Seg1 along X: (0,0,0)-(2,0,0). Seg2 along Y: (1,-1,0)-(1,1,0). They intersect at (1,0,0).
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
                                          {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.z == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - skew segments produce correct closest points",
          "[core][utilities]") {
    // Seg1 along X: (0,0,0)-(2,0,0). Seg2 parallel to Y but offset in Z: (1,-1,1)-(1,1,1).
    // Closest on seg1: (1,0,0). Closest on seg2: (1,0,1). Distance = 1.
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
                                          {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.z == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.z == Catch::Approx(1.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - both degenerate segments return the two endpoints",
          "[core][utilities]") {
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f},
                                          {4.0f, 5.0f, 6.0f}, {4.0f, 5.0f, 6.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(1.0f));
    REQUIRE(p1.y == Catch::Approx(2.0f));
    REQUIRE(p1.z == Catch::Approx(3.0f));
    REQUIRE(p2.x == Catch::Approx(4.0f));
    REQUIRE(p2.y == Catch::Approx(5.0f));
    REQUIRE(p2.z == Catch::Approx(6.0f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - seg1 degenerate (a point) returns closest point on seg2",
          "[core][utilities]") {
    // Point (0,0,0). Seg2: (0,1,0) to (2,1,0). Closest on seg2 to (0,0,0) is (0,1,0).
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                                          {0.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 0.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.z == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p2.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - seg2 degenerate (a point) returns closest point on seg1",
          "[core][utilities]") {
    // Seg1: (0,0,0) to (4,0,0). Point: (5,1,0) — beyond end, clamped to (4,0,0).
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f},
                                          {5.0f, 1.0f, 0.0f}, {5.0f, 1.0f, 0.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(4.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.z == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(5.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p2.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - parallel non-overlapping segments: closest points are endpoints",
          "[core][utilities]") {
    // Seg1: (0,0,0)-(1,0,0). Seg2: (3,0,0)-(4,0,0). Closest: (1,0,0) and (3,0,0).
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
                                          {3.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p1.z == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(3.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - parallel overlapping segments: closest points coincide within the overlap",
          "[core][utilities]") {
    // Seg1: (0,0,0)-(2,0,0). Seg2: (1,0,0)-(3,0,0). Overlap is [1,2].
    // The algorithm returns some point in the overlap; for both, the distance should be 0.
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
                                          {1.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f},
                                          p1, p2);
    REQUIRE(glm::distance(p1, p2) == Catch::Approx(0.0f).margin(1e-5f));
    // Both points lie in the overlap region [1,2].
    REQUIRE(p1.x >= Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(p1.x <= Catch::Approx(2.0f).margin(1e-5f));
}

TEST_CASE("ComputeClosestPointBetweenTwoSegments - closest points clamped at seg1 start when seg2 is beyond seg1 start",
          "[core][utilities]") {
    // Seg1: (2,0,0)-(4,0,0). Seg2: (0,0,0)-(0,2,0) (perpendicular, to the left of seg1 start).
    // Closest on seg1: (2,0,0). Closest on seg2: (0,0,0).
    glm::vec3 p1, p2;
    ComputeClosestPointBetweenTwoSegments({2.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f},
                                          {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f},
                                          p1, p2);
    REQUIRE(p1.x == Catch::Approx(2.0f).margin(1e-5f));
    REQUIRE(p1.y == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.x == Catch::Approx(0.0f).margin(1e-5f));
    REQUIRE(p2.y == Catch::Approx(0.0f).margin(1e-5f));
}
