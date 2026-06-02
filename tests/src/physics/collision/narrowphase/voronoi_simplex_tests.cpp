#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;
using Catch::Approx;

// ===========================================================================================
// Helper Functions for Testing
// ===========================================================================================

namespace {
    // Helper to check if two vectors are approximately equal
    bool Vec3Approx(const glm::vec3 &a, const glm::vec3 &b, f32 epsilon = 0.0001f) {
        return std::abs(a.x - b.x) < epsilon && std::abs(a.y - b.y) < epsilon && std::abs(a.z - b.z) < epsilon;
    }

    // Helper to check if a value is approximately zero
    bool IsApproxZero(f32 value, f32 epsilon = 0.0001f) { return std::abs(value) < epsilon; }
} // namespace

// ===========================================================================================
// Empty Simplex Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - newly created simplex is empty", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    REQUIRE(simplex.IsEmpty());
    REQUIRE_FALSE(simplex.IsFull());
}

TEST_CASE("VoronoiSimplex - empty simplex cannot compute valid closest point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 closestPoint;

    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE_FALSE(isValid);
}

TEST_CASE("VoronoiSimplex - empty simplex returns zero vertices via GetSimplex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 suppA[4], suppB[4], vertices[4];

    size_t count = simplex.GetSimplex(suppA, suppB, vertices);

    REQUIRE(count == 0);
}

// ===========================================================================================
// Single Point (1 vertex) Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - single point simplex is not empty and not full", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 2.0f, 3.0f);
    glm::vec3 suppA(1.5f, 2.5f, 3.5f);
    glm::vec3 suppB(0.5f, 0.5f, 0.5f);

    simplex.AddPoint(point, suppA, suppB);

    REQUIRE_FALSE(simplex.IsEmpty());
    REQUIRE_FALSE(simplex.IsFull());
}

TEST_CASE("VoronoiSimplex - single point is its own closest point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 2.0f, 3.0f);
    glm::vec3 suppA(1.5f, 2.5f, 3.5f);
    glm::vec3 suppB(0.5f, 0.5f, 0.5f);

    simplex.AddPoint(point, suppA, suppB);

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, point));
}

TEST_CASE("VoronoiSimplex - single point simplex returns correct support points", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 2.0f, 3.0f);
    glm::vec3 suppA(1.5f, 2.5f, 3.5f);
    glm::vec3 suppB(0.5f, 0.5f, 0.5f);

    simplex.AddPoint(point, suppA, suppB);

    glm::vec3 pA, pB;
    REQUIRE(simplex.ComputeClosestPoint(point)); // Must compute first
    simplex.ComputeClosestPointsOfAandB(pA, pB);

    REQUIRE(Vec3Approx(pA, suppA));
    REQUIRE(Vec3Approx(pB, suppB));
}

TEST_CASE("VoronoiSimplex - point at origin has zero distance", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(0.0f, 0.0f, 0.0f);
    glm::vec3 suppA(1.0f, 0.0f, 0.0f);
    glm::vec3 suppB(1.0f, 0.0f, 0.0f);

    simplex.AddPoint(point, suppA, suppB);

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f)));
}

TEST_CASE("VoronoiSimplex - GetMaxLengthSquareOfAPoint for single point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(3.0f, 4.0f, 0.0f); // Distance = 5, squared = 25

    simplex.AddPoint(point, glm::vec3(0.0f), glm::vec3(0.0f));

    f32 maxLengthSq = simplex.GetMaxLengthSquareOfAPoint();

    REQUIRE(maxLengthSq == Approx(25.0f).epsilon(0.0001f));
}

// ===========================================================================================
// Two Points (Line Segment) Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - segment with origin closest to first vertex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Segment from (1,0,0) to (5,0,0), origin is closest to first point
    glm::vec3 a(1.0f, 0.0f, 0.0f);
    glm::vec3 b(5.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, a));

    // After reduction, only vertex A should remain
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 1);
}

TEST_CASE("VoronoiSimplex - segment with origin closest to second vertex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Segment from (-5,0,0) to (-1,0,0), origin is closest to second point
    glm::vec3 a(-5.0f, 0.0f, 0.0f);
    glm::vec3 b(-1.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, b));

    // After reduction, only vertex B should remain
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 1);
}

TEST_CASE("VoronoiSimplex - segment with origin projecting onto interior", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Segment from (-2,1,0) to (2,1,0), origin projects to (0,1,0)
    glm::vec3 a(-2.0f, 1.0f, 0.0f);
    glm::vec3 b(2.0f, 1.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Both vertices should remain after reduction
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 2);
}

TEST_CASE("VoronoiSimplex - segment passing through origin", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Segment from (-1,0,0) to (1,0,0), passes through origin
    glm::vec3 a(-1.0f, 0.0f, 0.0f);
    glm::vec3 b(1.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f)));
}

// ===========================================================================================
// Three Points (Triangle) Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - triangle with origin in vertex A Voronoi region", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Triangle far from origin, closest to vertex A
    glm::vec3 a(1.0f, 0.0f, 0.0f);
    glm::vec3 b(5.0f, 0.0f, 0.0f);
    glm::vec3 c(3.0f, 4.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, a));

    // Only vertex A should remain
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 1);
}

TEST_CASE("VoronoiSimplex - triangle with origin in edge AB Voronoi region", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Triangle with origin projecting onto edge AB
    glm::vec3 a(-1.0f, 1.0f, 0.0f);
    glm::vec3 b(1.0f, 1.0f, 0.0f);
    glm::vec3 c(0.0f, 5.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Vertices A and B should remain
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 2);
}

TEST_CASE("VoronoiSimplex - triangle with origin projecting onto face", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Equilateral triangle parallel to XY plane at z=1, origin projects to center
    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 b(-0.5f, 0.866f, 1.0f);
    glm::vec3 c(-0.5f, -0.866f, 1.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    // Closest point should have z=1 and be near origin in XY
    REQUIRE(closestPoint.z == Approx(1.0f).epsilon(0.01f));
    REQUIRE(IsApproxZero(closestPoint.x, 0.01f));
    REQUIRE(IsApproxZero(closestPoint.y, 0.01f));

    // All three vertices should remain
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 3);
}

TEST_CASE("VoronoiSimplex - triangle containing origin in its plane", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Triangle in XY plane containing origin
    glm::vec3 a(1.0f, 0.0f, 0.0f);
    glm::vec3 b(0.0f, 1.0f, 0.0f);
    glm::vec3 c(-1.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f)));
}

// ===========================================================================================
// Four Points (Tetrahedron) Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - tetrahedron is full when it has 4 vertices", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsFull());
    REQUIRE_FALSE(simplex.IsEmpty());
}

TEST_CASE("VoronoiSimplex - tetrahedron containing origin returns origin as closest", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Regular tetrahedron containing origin
    simplex.AddPoint(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(0.0f)));

    // All four vertices should remain (origin is inside)
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 4);
}

TEST_CASE("VoronoiSimplex - tetrahedron with origin outside finds closest face", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Tetrahedron shifted away from origin, closest face should be found
    simplex.AddPoint(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(3.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(3.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    // Closest point should be on the surface, not at origin
    f32 distance = glm::length(closestPoint);
    REQUIRE(distance > 0.1f); // Should be far from origin

    // Simplex should be reduced to the closest feature (likely a triangle or less)
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count <= 3); // Should reduce to triangle, edge, or vertex
}

TEST_CASE("VoronoiSimplex - tetrahedron with origin closest to a vertex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Tetrahedron with one vertex much closer to origin than others
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(10.0f, 0.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(1.0f, 0.0f, 0.0f)));

    // Should reduce to single vertex
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 1);
}

// ===========================================================================================
// Point In Simplex Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - IsPointInSimplex returns true for existing point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 2.0f, 3.0f);

    simplex.AddPoint(point, glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsPointInSimplex(point));
}

TEST_CASE("VoronoiSimplex - IsPointInSimplex returns false for non-existing point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point1(1.0f, 2.0f, 3.0f);
    glm::vec3 point2(5.0f, 6.0f, 7.0f);

    simplex.AddPoint(point1, glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE_FALSE(simplex.IsPointInSimplex(point2));
}

TEST_CASE("VoronoiSimplex - IsPointInSimplex returns true for nearly identical point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point1(1.0f, 2.0f, 3.0f);
    glm::vec3 point2(1.00001f, 2.00001f, 3.00001f); // Very close

    simplex.AddPoint(point1, glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsPointInSimplex(point2));
}

// ===========================================================================================
// Affinely Dependent Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - single point is not affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE_FALSE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - two distinct points are not affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE_FALSE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - two very close points are affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(1.000001f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - three non-collinear points are not affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE_FALSE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - three collinear points are affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - four non-coplanar points are not affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE_FALSE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - four coplanar points are affinely dependent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // All points in XY plane (z=0)
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(simplex.IsAffinelyDependent());
}

// ===========================================================================================
// GetMaxLengthSquareOfAPoint Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - GetMaxLengthSquareOfAPoint returns max distance", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f)); // length^2 = 1
    simplex.AddPoint(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f)); // length^2 = 4
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f)); // length^2 = 9

    f32 maxLengthSq = simplex.GetMaxLengthSquareOfAPoint();

    REQUIRE(maxLengthSq == Approx(9.0f).epsilon(0.0001f));
}

TEST_CASE("VoronoiSimplex - GetMaxLengthSquareOfAPoint returns zero for empty simplex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    f32 maxLengthSq = simplex.GetMaxLengthSquareOfAPoint();

    REQUIRE(maxLengthSq == Approx(0.0f).epsilon(0.0001f));
}

// ===========================================================================================
// Backup Closest Point Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - BackupClosestPointInSimplex stores current closest point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 2.0f, 3.0f);
    simplex.AddPoint(point, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 computed, backup;
    REQUIRE(simplex.ComputeClosestPoint(computed));
    simplex.BackupClosestPointInSimplex(backup);

    REQUIRE(Vec3Approx(backup, computed));
    REQUIRE(Vec3Approx(backup, point));
}

// ===========================================================================================
// Edge Cases and Special Scenarios
// ===========================================================================================

TEST_CASE("VoronoiSimplex - segment perpendicular to origin", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Vertical segment at x=1
    glm::vec3 a(1.0f, -5.0f, 0.0f);
    glm::vec3 b(1.0f, 5.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST_CASE("VoronoiSimplex - right triangle with origin at right angle", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Right triangle with right angle at origin-adjacent area
    glm::vec3 a(1.0f, 0.0f, 0.0f);
    glm::vec3 b(1.0f, 1.0f, 0.0f);
    glm::vec3 c(1.0f, 0.0f, 1.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    // Should reduce to single closest vertex
    f32 distance = glm::length(closestPoint);
    REQUIRE(distance >= 1.0f - 0.01f); // At least 1 unit away
}

TEST_CASE("VoronoiSimplex - multiple calls to ComputeClosestPoint are idempotent", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 result1, result2, result3;
    REQUIRE(simplex.ComputeClosestPoint(result1));
    REQUIRE(simplex.ComputeClosestPoint(result2));
    REQUIRE(simplex.ComputeClosestPoint(result3));

    REQUIRE(Vec3Approx(result1, result2));
    REQUIRE(Vec3Approx(result2, result3));
}

TEST_CASE("VoronoiSimplex - adding points incrementally maintains consistency", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 cp;

    // Add first point
    simplex.AddPoint(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    REQUIRE(simplex.ComputeClosestPoint(cp));

    // Add second point
    simplex.AddPoint(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    REQUIRE(simplex.ComputeClosestPoint(cp));

    // Add third point
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    REQUIRE(simplex.ComputeClosestPoint(cp));

    // Each addition should produce a valid closest point
    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count <= 3); // Should be reduced to minimal set
}

TEST_CASE("VoronoiSimplex - large coordinate values maintain precision", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Use large coordinates
    f32 large = 1000.0f;
    simplex.AddPoint(glm::vec3(large, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, large, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    // Should find a point on the segment
    f32 distance = glm::length(closestPoint);
    REQUIRE(distance > 0.0f);
    REQUIRE(distance < large * 2.0f); // Sanity check
}

TEST_CASE("VoronoiSimplex - very small coordinate values", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Use very small but non-zero coordinates
    f32 small = 0.001f;
    simplex.AddPoint(glm::vec3(small, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 closestPoint;
    bool isValid = simplex.ComputeClosestPoint(closestPoint);

    REQUIRE(isValid);
    REQUIRE(Vec3Approx(closestPoint, glm::vec3(small, 0.0f, 0.0f), 0.0001f));
}

// ===========================================================================================
// Support Point Interpolation Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - support points are correctly interpolated for segment", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    // Segment from (-1,1,0) to (1,1,0), closest point at (0,1,0)
    glm::vec3 a(-1.0f, 1.0f, 0.0f);
    glm::vec3 b(1.0f, 1.0f, 0.0f);

    glm::vec3 suppA1(0.0f, 2.0f, 0.0f);
    glm::vec3 suppB1(1.0f, 1.0f, 0.0f);
    glm::vec3 suppA2(2.0f, 2.0f, 0.0f);
    glm::vec3 suppB2(1.0f, 1.0f, 0.0f);

    simplex.AddPoint(a, suppA1, suppB1);
    simplex.AddPoint(b, suppA2, suppB2);

    glm::vec3 closestPoint;
    REQUIRE(simplex.ComputeClosestPoint(closestPoint));

    glm::vec3 pA, pB;
    simplex.ComputeClosestPointsOfAandB(pA, pB);

    // Support points should be interpolated at t=0.5
    REQUIRE(Vec3Approx(pA, glm::vec3(1.0f, 2.0f, 0.0f), 0.01f));
    REQUIRE(Vec3Approx(pB, glm::vec3(1.0f, 1.0f, 0.0f), 0.01f));
}

TEST_CASE("VoronoiSimplex - support points for single vertex simplex", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 point(1.0f, 0.0f, 0.0f);
    glm::vec3 suppA(5.0f, 0.0f, 0.0f);
    glm::vec3 suppB(4.0f, 0.0f, 0.0f);

    simplex.AddPoint(point, suppA, suppB);

    glm::vec3 closestPoint;
    REQUIRE(simplex.ComputeClosestPoint(closestPoint));

    glm::vec3 pA, pB;
    simplex.ComputeClosestPointsOfAandB(pA, pB);

    REQUIRE(Vec3Approx(pA, suppA));
    REQUIRE(Vec3Approx(pB, suppB));
    REQUIRE(Vec3Approx(pA - pB, point)); // Verify Minkowski difference
}

// ===========================================================================================
// Stress Tests
// ===========================================================================================

TEST_CASE("VoronoiSimplex - alternating additions and reductions", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;
    glm::vec3 cp;

    // Add 3 points
    simplex.AddPoint(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    // Compute (may reduce)
    REQUIRE(simplex.ComputeClosestPoint(cp));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);

    // Should have valid result regardless of reduction
    REQUIRE(count >= 1);
    REQUIRE(count <= 3);
}

// ===========================================================================================
// Additional Edge Case Tests (added to improve coverage)
// ===========================================================================================

TEST_CASE("VoronoiSimplex - degenerate tetrahedron (coplanar) returns invalid closest point", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Four coplanar points (all z = 1.0) -> degenerate tetrahedron
    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 b(0.0f, 1.0f, 1.0f);
    glm::vec3 c(-1.0f, 0.0f, 1.0f);
    glm::vec3 d(0.5f, 0.2f, 1.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(d, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE_FALSE(simplex.ComputeClosestPoint(cp));
    REQUIRE(simplex.IsAffinelyDependent());
}

TEST_CASE("VoronoiSimplex - duplicate points (zero-length segment) handled", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    glm::vec3 p(1.234f, 2.345f, 3.456f);
    glm::vec3 suppA1(2.0f, 0.0f, 0.0f);
    glm::vec3 suppB1(1.0f, 0.0f, 0.0f);
    glm::vec3 suppA2(2.5f, 0.0f, 0.0f);
    glm::vec3 suppB2(1.5f, 0.0f, 0.0f);

    simplex.AddPoint(p, suppA1, suppB1);
    simplex.AddPoint(p, suppA2, suppB2);

    // Should be detected as affinely dependent (degenerate segment)
    REQUIRE(simplex.IsAffinelyDependent());

    glm::vec3 cp;
    if (simplex.ComputeClosestPoint(cp)) {
        // If it does compute a point, it must be the duplicated point
        REQUIRE(Vec3Approx(cp, p));
    } else {
        // It's acceptable for an implementation to declare the case degenerate
        SUCCEED("ComputeClosestPoint returned false for degenerate duplicate points");
    }
}

TEST_CASE("VoronoiSimplex - triangle with origin in edge AC Voronoi region", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Triangle arranged so origin projects onto edge AC
    glm::vec3 a(-2.0f, 0.0f, 0.0f);
    glm::vec3 b(0.0f, 5.0f, 0.0f);
    glm::vec3 c(2.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(Vec3Approx(cp, glm::vec3(0.0f, 0.0f, 0.0f)));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 2); // Should reduce to edge AC
}

TEST_CASE("VoronoiSimplex - triangle with origin in edge BC Voronoi region", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Reordered vertices so origin projects onto BC
    glm::vec3 a(0.0f, 5.0f, 0.0f);
    glm::vec3 b(-2.0f, 0.0f, 0.0f);
    glm::vec3 c(2.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(Vec3Approx(cp, glm::vec3(0.0f, 0.0f, 0.0f)));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 2); // Should reduce to edge BC
}

TEST_CASE("VoronoiSimplex - tetrahedron closest face ABC", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Place triangle ABC on z=1 and D far on +z so origin is outside and closest to ABC
    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 b(-1.0f, 0.0f, 1.0f);
    glm::vec3 c(0.0f, 1.0f, 1.0f);
    glm::vec3 d(0.0f, 0.0f, 10.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(d, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(cp.z == Approx(1.0f).epsilon(0.001f));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 3);
}

TEST_CASE("VoronoiSimplex - tetrahedron closest face ACD", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Face ACD on z=1, vertex B placed far to make ACD the closest face
    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 c(0.0f, 1.0f, 1.0f);
    glm::vec3 d(-1.0f, 0.0f, 1.0f);
    glm::vec3 b(0.0f, 0.0f, 10.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(d, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(cp.z == Approx(1.0f).epsilon(0.001f));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 3);
}

TEST_CASE("VoronoiSimplex - tetrahedron closest face ADB", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 d(0.0f, 1.0f, 1.0f);
    glm::vec3 b(-1.0f, 0.0f, 1.0f);
    glm::vec3 c(0.0f, 0.0f, 10.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(d, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(cp.z == Approx(1.0f).epsilon(0.001f));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 3);
}

TEST_CASE("VoronoiSimplex - tetrahedron closest face BDC", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    glm::vec3 b(1.0f, 0.0f, 1.0f);
    glm::vec3 d(0.0f, 1.0f, 1.0f);
    glm::vec3 c(-1.0f, 0.0f, 1.0f);
    glm::vec3 a(0.0f, 0.0f, 10.0f);

    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(d, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));
    REQUIRE(cp.z == Approx(1.0f).epsilon(0.001f));

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);
    REQUIRE(count == 3);
}

TEST_CASE("VoronoiSimplex - RemovePoint swaps last vertex into removed index", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    glm::vec3 a(1.0f, 0.0f, 0.0f);
    glm::vec3 b(2.0f, 0.0f, 0.0f);
    glm::vec3 c(3.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(b, glm::vec3(0.0f), glm::vec3(0.0f));
    simplex.AddPoint(c, glm::vec3(0.0f), glm::vec3(0.0f));

    // Remove the middle element; last element should be swapped into its place
    simplex.RemovePoint(1);

    glm::vec3 suppA[4], suppB[4], vertices[4];
    size_t count = simplex.GetSimplex(suppA, suppB, vertices);

    REQUIRE(count == 2);
    REQUIRE(Vec3Approx(vertices[0], a));
    REQUIRE(Vec3Approx(vertices[1], c));
}

TEST_CASE("VoronoiSimplex - triangle barycentric interpolation produces consistent support points", "[physics][narrowphase][gjk][voronoi_simplex]") {
    VoronoiSimplex simplex;

    // Equilateral triangle parallel to z=1, origin projects to centroid
    glm::vec3 a(1.0f, 0.0f, 1.0f);
    glm::vec3 b(-0.5f, 0.866f, 1.0f);
    glm::vec3 c(-0.5f, -0.866f, 1.0f);

    glm::vec3 suppA1(1.0f, 0.0f, 1.0f);
    glm::vec3 suppB1(0.0f, 0.0f, 0.0f);
    glm::vec3 suppA2(-0.5f, 0.866f, 1.0f);
    glm::vec3 suppB2(0.0f, 0.0f, 0.0f);
    glm::vec3 suppA3(-0.5f, -0.866f, 1.0f);
    glm::vec3 suppB3(0.0f, 0.0f, 0.0f);

    simplex.AddPoint(a, suppA1, suppB1);
    simplex.AddPoint(b, suppA2, suppB2);
    simplex.AddPoint(c, suppA3, suppB3);

    glm::vec3 cp;
    REQUIRE(simplex.ComputeClosestPoint(cp));

    glm::vec3 pA, pB;
    simplex.ComputeClosestPointsOfAandB(pA, pB);

    // Interpolated support points should reconstruct the Minkowski closest point
    REQUIRE(Vec3Approx(pA - pB, cp, 0.001f));
}
