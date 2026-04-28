#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;

// ===========================================================================================
// Constructor
// ===========================================================================================

TEST_CASE("AABB - Construct with valid min and max", "[physics][aabb]") {
    AABB box(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f));

    REQUIRE(box.GetMin() == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(box.GetMax() == glm::vec3(4.0f, 5.0f, 6.0f));
}

TEST_CASE("AABB - Construct with min equal to max (zero-volume)", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(box.GetMin() == glm::vec3(5.0f));
    REQUIRE(box.GetMax() == glm::vec3(5.0f));
    REQUIRE(box.GetVolume() == 0.0f);
}

TEST_CASE("AABB - Construct with negative coordinates", "[physics][aabb]") {
    AABB box(glm::vec3(-10.0f, -20.0f, -30.0f), glm::vec3(-1.0f, -2.0f, -3.0f));

    REQUIRE(box.GetMin() == glm::vec3(-10.0f, -20.0f, -30.0f));
    REQUIRE(box.GetMax() == glm::vec3(-1.0f, -2.0f, -3.0f));
}

TEST_CASE("AABB - Construct with min spanning negative to positive", "[physics][aabb]") {
    AABB box(glm::vec3(-5.0f, -5.0f, -5.0f), glm::vec3(5.0f, 5.0f, 5.0f));

    REQUIRE(box.GetMin() == glm::vec3(-5.0f));
    REQUIRE(box.GetMax() == glm::vec3(5.0f));
}

// ===========================================================================================
// GetExtents
// ===========================================================================================

TEST_CASE("AABB - GetExtents of unit box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(box.GetExtents() == glm::vec3(1.0f));
}

TEST_CASE("AABB - GetExtents with different extents per axis", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(box.GetExtents() == glm::vec3(2.0f, 3.0f, 4.0f));
}

TEST_CASE("AABB - GetExtents of zero-volume box", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(box.GetExtents() == glm::vec3(0.0f));
}

TEST_CASE("AABB - GetExtents with negative coordinates", "[physics][aabb]") {
    AABB box(glm::vec3(-10.0f, -20.0f, -30.0f), glm::vec3(-5.0f, -10.0f, -15.0f));

    REQUIRE(box.GetExtents() == glm::vec3(5.0f, 10.0f, 15.0f));
}

TEST_CASE("AABB - GetExtents spanning negative to positive", "[physics][aabb]") {
    AABB box(glm::vec3(-5.0f, -3.0f, -2.0f), glm::vec3(5.0f, 7.0f, 8.0f));

    REQUIRE(box.GetExtents() == glm::vec3(10.0f, 10.0f, 10.0f));
}

TEST_CASE("AABB - GetExtents after inflation", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Inflate(glm::vec3(2.0f));

    REQUIRE(box.GetExtents() == glm::vec3(14.0f));
}

TEST_CASE("AABB - GetExtents after scale", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(5.0f));
    box.Scale(glm::vec3(2.0f));

    REQUIRE(box.GetExtents() == glm::vec3(10.0f));
}

// ===========================================================================================
// GetCenter
// ===========================================================================================

TEST_CASE("AABB - GetCenter of symmetric box", "[physics][aabb]") {
    AABB box(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f));

    REQUIRE(box.GetCenter() == glm::vec3(0.0f));
}

TEST_CASE("AABB - GetCenter of asymmetric box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(4.0f, 6.0f, 8.0f));

    REQUIRE(box.GetCenter() == glm::vec3(2.0f, 3.0f, 4.0f));
}

TEST_CASE("AABB - GetCenter of zero-volume box", "[physics][aabb]") {
    AABB box(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(3.0f, 3.0f, 3.0f));

    REQUIRE(box.GetCenter() == glm::vec3(3.0f));
}

// ===========================================================================================
// Encapsulate
// ===========================================================================================

TEST_CASE("AABB - Encapsulate point already inside", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Encapsulate(glm::vec3(5.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - Encapsulate point outside expands min", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Encapsulate(glm::vec3(-5.0f, -3.0f, -1.0f));

    REQUIRE(box.GetMin() == glm::vec3(-5.0f, -3.0f, -1.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - Encapsulate point outside expands max", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Encapsulate(glm::vec3(15.0f, 20.0f, 25.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(15.0f, 20.0f, 25.0f));
}

TEST_CASE("AABB - Encapsulate point on boundary", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Encapsulate(glm::vec3(0.0f, 10.0f, 5.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - Encapsulate expands both min and max", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Encapsulate(glm::vec3(-1.0f, 5.0f, 15.0f));

    REQUIRE(box.GetMin() == glm::vec3(-1.0f, 0.0f, 0.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f, 10.0f, 15.0f));
}

TEST_CASE("AABB - Encapsulate multiple points", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(0.0f));
    box.Encapsulate(glm::vec3(1.0f, 0.0f, 0.0f));
    box.Encapsulate(glm::vec3(0.0f, 2.0f, 0.0f));
    box.Encapsulate(glm::vec3(0.0f, 0.0f, 3.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(1.0f, 2.0f, 3.0f));
}

// ===========================================================================================
// Inflate
// ===========================================================================================

TEST_CASE("AABB - Inflate uniformly", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Inflate(glm::vec3(1.0f));

    REQUIRE(box.GetMin() == glm::vec3(-1.0f));
    REQUIRE(box.GetMax() == glm::vec3(11.0f));
}

TEST_CASE("AABB - Inflate with different values per axis", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Inflate(glm::vec3(1.0f, 2.0f, 3.0f));

    REQUIRE(box.GetMin() == glm::vec3(-1.0f, -2.0f, -3.0f));
    REQUIRE(box.GetMax() == glm::vec3(11.0f, 12.0f, 13.0f));
}

TEST_CASE("AABB - Inflate with zero does nothing", "[physics][aabb]") {
    AABB box(glm::vec3(1.0f), glm::vec3(5.0f));
    box.Inflate(glm::vec3(0.0f));

    REQUIRE(box.GetMin() == glm::vec3(1.0f));
    REQUIRE(box.GetMax() == glm::vec3(5.0f));
}

TEST_CASE("AABB - Inflate with negative shrinks the box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.Inflate(glm::vec3(-2.0f));

    REQUIRE(box.GetMin() == glm::vec3(2.0f));
    REQUIRE(box.GetMax() == glm::vec3(8.0f));
}

TEST_CASE("AABB - Inflate to exact zero volume", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(4.0f));
    box.Inflate(glm::vec3(-2.0f));

    REQUIRE(box.GetMin() == glm::vec3(2.0f));
    REQUIRE(box.GetMax() == glm::vec3(2.0f));
    REQUIRE(box.GetVolume() == 0.0f);
}

// ===========================================================================================
// Scale
// ===========================================================================================

TEST_CASE("AABB - Scale uniformly", "[physics][aabb]") {
    AABB box(glm::vec3(1.0f), glm::vec3(3.0f));
    box.Scale(glm::vec3(2.0f));

    REQUIRE(box.GetMin() == glm::vec3(2.0f));
    REQUIRE(box.GetMax() == glm::vec3(6.0f));
}

TEST_CASE("AABB - Scale with different factors per axis", "[physics][aabb]") {
    AABB box(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(2.0f, 4.0f, 6.0f));
    box.Scale(glm::vec3(2.0f, 0.5f, 3.0f));

    REQUIRE(box.GetMin() == glm::vec3(2.0f, 1.0f, 9.0f));
    REQUIRE(box.GetMax() == glm::vec3(4.0f, 2.0f, 18.0f));
}

TEST_CASE("AABB - Scale by 1 does nothing", "[physics][aabb]") {
    AABB box(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f));
    box.Scale(glm::vec3(1.0f));

    REQUIRE(box.GetMin() == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(box.GetMax() == glm::vec3(4.0f, 5.0f, 6.0f));
}

TEST_CASE("AABB - Scale box at origin stays at origin", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(5.0f));
    box.Scale(glm::vec3(3.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(15.0f));
}

TEST_CASE("AABB - Scale with very small factor", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(100.0f));
    box.Scale(glm::vec3(0.01f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax().x == Catch::Approx(1.0f));
    REQUIRE(box.GetMax().y == Catch::Approx(1.0f));
    REQUIRE(box.GetMax().z == Catch::Approx(1.0f));
}

// ===========================================================================================
// CollidesWith
// ===========================================================================================

TEST_CASE("AABB - CollidesWith overlapping boxes", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(3.0f), glm::vec3(8.0f));

    REQUIRE(a.CollidesWith(b));
    REQUIRE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith non-overlapping on X axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(5.0f, 2.0f, 2.0f));

    REQUIRE_FALSE(a.CollidesWith(b));
    REQUIRE_FALSE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith non-overlapping on Y axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(2.0f, 5.0f, 2.0f));

    REQUIRE_FALSE(a.CollidesWith(b));
    REQUIRE_FALSE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith non-overlapping on Z axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(2.0f, 2.0f, 5.0f));

    REQUIRE_FALSE(a.CollidesWith(b));
    REQUIRE_FALSE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith touching edges (should collide)", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(4.0f, 2.0f, 2.0f));

    REQUIRE(a.CollidesWith(b));
    REQUIRE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith touching at a single corner point", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(1.0f));
    AABB b(glm::vec3(1.0f), glm::vec3(2.0f));

    REQUIRE(a.CollidesWith(b));
    REQUIRE(b.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith one box fully inside another", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB inner(glm::vec3(2.0f), glm::vec3(4.0f));

    REQUIRE(outer.CollidesWith(inner));
    REQUIRE(inner.CollidesWith(outer));
}

TEST_CASE("AABB - CollidesWith identical boxes", "[physics][aabb]") {
    AABB a(glm::vec3(1.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(1.0f), glm::vec3(5.0f));

    REQUIRE(a.CollidesWith(b));
}

TEST_CASE("AABB - CollidesWith itself", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(a.CollidesWith(a));
}

TEST_CASE("AABB - CollidesWith zero-volume boxes at same point", "[physics][aabb]") {
    AABB a(glm::vec3(5.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(a.CollidesWith(b));
}

TEST_CASE("AABB - CollidesWith zero-volume boxes at different points", "[physics][aabb]") {
    AABB a(glm::vec3(5.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(6.0f), glm::vec3(6.0f));

    REQUIRE_FALSE(a.CollidesWith(b));
}

// ===========================================================================================
// GetVolume
// ===========================================================================================

TEST_CASE("AABB - GetVolume of unit box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(box.GetVolume() == 1.0f);
}

TEST_CASE("AABB - GetVolume with different extents per axis", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(box.GetVolume() == 24.0f);
}

TEST_CASE("AABB - GetVolume of zero-volume box", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(box.GetVolume() == 0.0f);
}

TEST_CASE("AABB - GetVolume of flat box (zero on one axis)", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 5.0f, 0.0f));

    REQUIRE(box.GetVolume() == 0.0f);
}

// ===========================================================================================
// Contains (point)
// ===========================================================================================

TEST_CASE("AABB - Contains point inside", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(5.0f)));
}

TEST_CASE("AABB - Contains point outside", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE_FALSE(box.Contains(glm::vec3(15.0f)));
}

TEST_CASE("AABB - Contains point on min corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(0.0f)));
}

TEST_CASE("AABB - Contains point on max corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(10.0f)));
}

TEST_CASE("AABB - Contains point on face", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(0.0f, 5.0f, 5.0f)));
    REQUIRE(box.Contains(glm::vec3(5.0f, 10.0f, 5.0f)));
    REQUIRE(box.Contains(glm::vec3(5.0f, 5.0f, 0.0f)));
}

TEST_CASE("AABB - Contains point on edge", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(0.0f, 0.0f, 5.0f)));
    REQUIRE(box.Contains(glm::vec3(10.0f, 10.0f, 5.0f)));
}

TEST_CASE("AABB - Contains point just outside each axis", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE_FALSE(box.Contains(glm::vec3(-0.001f, 5.0f, 5.0f)));
    REQUIRE_FALSE(box.Contains(glm::vec3(5.0f, 10.001f, 5.0f)));
    REQUIRE_FALSE(box.Contains(glm::vec3(5.0f, 5.0f, -0.001f)));
}

TEST_CASE("AABB - Contains point in zero-volume box at same point", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(box.Contains(glm::vec3(5.0f)));
}

TEST_CASE("AABB - Contains point in zero-volume box at different point", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE_FALSE(box.Contains(glm::vec3(5.001f)));
}

TEST_CASE("AABB - Contains point with custom epsilon allows near-boundary points", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    f32 epsilon = 0.1f;

    // Points just outside the boundary but within epsilon should be contained
    REQUIRE(box.Contains(glm::vec3(-0.05f, 5.0f, 5.0f), epsilon));
    REQUIRE(box.Contains(glm::vec3(10.05f, 5.0f, 5.0f), epsilon));
    REQUIRE(box.Contains(glm::vec3(5.0f, -0.1f, 5.0f), epsilon));
    REQUIRE(box.Contains(glm::vec3(5.0f, 10.1f, 5.0f), epsilon));
}

TEST_CASE("AABB - Contains point with custom epsilon rejects far-outside points", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    f32 epsilon = 0.1f;

    // Points beyond epsilon should not be contained
    REQUIRE_FALSE(box.Contains(glm::vec3(-0.2f, 5.0f, 5.0f), epsilon));
    REQUIRE_FALSE(box.Contains(glm::vec3(10.2f, 5.0f, 5.0f), epsilon));
}

TEST_CASE("AABB - Contains point with zero epsilon is exact", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.Contains(glm::vec3(10.0f), 0.0f));
    REQUIRE_FALSE(box.Contains(glm::vec3(10.00001f), 0.0f));
}

TEST_CASE("AABB - Contains point with large epsilon expands containment significantly", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    f32 epsilon = 5.0f;

    REQUIRE(box.Contains(glm::vec3(-4.0f, 5.0f, 5.0f), epsilon));
    REQUIRE(box.Contains(glm::vec3(14.0f, 5.0f, 5.0f), epsilon));
    REQUIRE_FALSE(box.Contains(glm::vec3(-6.0f, 5.0f, 5.0f), epsilon));
}

// ===========================================================================================
// Contains (AABB)
// ===========================================================================================

TEST_CASE("AABB - Contains AABB fully inside", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB inner(glm::vec3(2.0f), glm::vec3(8.0f));

    REQUIRE(outer.Contains(inner));
}

TEST_CASE("AABB - Contains AABB not contained", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(3.0f), glm::vec3(8.0f));

    REQUIRE_FALSE(a.Contains(b));
}

TEST_CASE("AABB - Contains AABB identical boxes contains each other", "[physics][aabb]") {
    AABB a(glm::vec3(1.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(1.0f), glm::vec3(5.0f));

    REQUIRE(a.Contains(b));
    REQUIRE(b.Contains(a));
}

TEST_CASE("AABB - Contains AABB contains itself", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(a.Contains(a));
}

TEST_CASE("AABB - Contains AABB touching boundary is contained", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB boundary(glm::vec3(0.0f), glm::vec3(5.0f));

    REQUIRE(outer.Contains(boundary));
}

TEST_CASE("AABB - Contains AABB exceeding on one axis is not contained", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB b(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(8.0f, 8.0f, 11.0f));

    REQUIRE_FALSE(a.Contains(b));
}

TEST_CASE("AABB - Contains AABB exceeding on min side is not contained", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB b(glm::vec3(-1.0f, 2.0f, 2.0f), glm::vec3(8.0f, 8.0f, 8.0f));

    REQUIRE_FALSE(a.Contains(b));
}

TEST_CASE("AABB - Contains AABB zero-volume inside", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB point(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(outer.Contains(point));
}

TEST_CASE("AABB - Contains AABB zero-volume on boundary", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB point(glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(outer.Contains(point));
}

TEST_CASE("AABB - Contains AABB zero-volume outside", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB point(glm::vec3(15.0f), glm::vec3(15.0f));

    REQUIRE_FALSE(outer.Contains(point));
}

TEST_CASE("AABB - Contains AABB larger box does not contain smaller parent", "[physics][aabb]") {
    AABB small(glm::vec3(2.0f), glm::vec3(4.0f));
    AABB large(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE_FALSE(small.Contains(large));
}

TEST_CASE("AABB - Contains AABB with negative coordinates", "[physics][aabb]") {
    AABB outer(glm::vec3(-10.0f), glm::vec3(-1.0f));
    AABB inner(glm::vec3(-8.0f), glm::vec3(-3.0f));

    REQUIRE(outer.Contains(inner));
}

TEST_CASE("AABB - Contains AABB partially overlapping is not contained", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(2.0f), glm::vec3(7.0f));

    REQUIRE_FALSE(a.Contains(b));
    REQUIRE_FALSE(b.Contains(a));
}

// ===========================================================================================
// SetMin / SetMax
// ===========================================================================================

TEST_CASE("AABB - SetMin updates minimum corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.SetMin(glm::vec3(-5.0f));

    REQUIRE(box.GetMin() == glm::vec3(-5.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - SetMax updates maximum corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.SetMax(glm::vec3(20.0f));

    REQUIRE(box.GetMax() == glm::vec3(20.0f));
    REQUIRE(box.GetMin() == glm::vec3(0.0f));
}

TEST_CASE("AABB - SetMin equal to max produces zero-volume box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.SetMin(glm::vec3(10.0f));

    REQUIRE(box.GetMin() == glm::vec3(10.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
    REQUIRE(box.GetVolume() == 0.0f);
}

TEST_CASE("AABB - SetMax equal to min produces zero-volume box", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    box.SetMax(glm::vec3(0.0f));

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(0.0f));
    REQUIRE(box.GetVolume() == 0.0f);
}

// ===========================================================================================
// Combined operations
// ===========================================================================================

TEST_CASE("AABB - Inflate then check collision", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(4.0f), glm::vec3(6.0f));

    REQUIRE_FALSE(a.CollidesWith(b));

    a.Inflate(glm::vec3(2.0f));

    REQUIRE(a.CollidesWith(b));
}

TEST_CASE("AABB - Scale then check containment", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(5.0f));

    REQUIRE_FALSE(box.Contains(glm::vec3(8.0f)));

    box.Scale(glm::vec3(2.0f));

    REQUIRE(box.Contains(glm::vec3(8.0f)));
}

TEST_CASE("AABB - Encapsulate then check volume grows", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(box.GetVolume() == 1.0f);

    box.Encapsulate(glm::vec3(2.0f, 1.0f, 1.0f));

    REQUIRE(box.GetVolume() == 2.0f);
}

// ===========================================================================================
// SetMinMax
// ===========================================================================================

TEST_CASE("AABB - SetMinMax sets min and max correctly", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));
    box.SetMinMax(glm::vec3(2.0f, 3.0f, 4.0f), glm::vec3(5.0f, 6.0f, 7.0f));

    REQUIRE(box.GetMin() == glm::vec3(2.0f, 3.0f, 4.0f));
    REQUIRE(box.GetMax() == glm::vec3(5.0f, 6.0f, 7.0f));
}

TEST_CASE("AABB - SetMinMax where new min exceeds old max does not assert", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));

    // New min (10) is far beyond current max (1) — would trigger SetMin's assertion but not SetMinMax
    box.SetMinMax(glm::vec3(10.0f), glm::vec3(20.0f));

    REQUIRE(box.GetMin() == glm::vec3(10.0f));
    REQUIRE(box.GetMax() == glm::vec3(20.0f));
}

TEST_CASE("AABB - SetMinMax where new max is below old min does not assert", "[physics][aabb]") {
    AABB box(glm::vec3(10.0f), glm::vec3(20.0f));

    // New max (1) is far below current min (10) — would trigger SetMax's assertion but not SetMinMax
    box.SetMinMax(glm::vec3(-5.0f), glm::vec3(1.0f));

    REQUIRE(box.GetMin() == glm::vec3(-5.0f));
    REQUIRE(box.GetMax() == glm::vec3(1.0f));
}

TEST_CASE("AABB - SetMinMax with equal min and max produces zero-volume AABB", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));
    box.SetMinMax(glm::vec3(3.0f), glm::vec3(3.0f));

    REQUIRE(box.GetMin() == glm::vec3(3.0f));
    REQUIRE(box.GetMax() == glm::vec3(3.0f));
    REQUIRE(box.GetVolume() == 0.0f);
}

TEST_CASE("AABB - SetMinMax with negative coordinates", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));
    box.SetMinMax(glm::vec3(-10.0f, -5.0f, -3.0f), glm::vec3(-1.0f, -2.0f, -1.0f));

    REQUIRE(box.GetMin() == glm::vec3(-10.0f, -5.0f, -3.0f));
    REQUIRE(box.GetMax() == glm::vec3(-1.0f, -2.0f, -1.0f));
}

TEST_CASE("AABB - SetMinMax updates GetCenter", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));
    box.SetMinMax(glm::vec3(0.0f), glm::vec3(4.0f, 6.0f, 8.0f));

    REQUIRE(box.GetCenter() == glm::vec3(2.0f, 3.0f, 4.0f));
}

// ===========================================================================================
// MergeWithAABB
// ===========================================================================================

TEST_CASE("AABB - MergeWithAABB two non-overlapping boxes on X axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(7.0f, 2.0f, 2.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(0.0f));
    REQUIRE(a.GetMax() == glm::vec3(7.0f, 2.0f, 2.0f));
}

TEST_CASE("AABB - MergeWithAABB two non-overlapping boxes on Y axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(2.0f, 7.0f, 2.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(0.0f));
    REQUIRE(a.GetMax() == glm::vec3(2.0f, 7.0f, 2.0f));
}

TEST_CASE("AABB - MergeWithAABB two non-overlapping boxes on Z axis", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(2.0f));
    AABB b(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(2.0f, 2.0f, 7.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(0.0f));
    REQUIRE(a.GetMax() == glm::vec3(2.0f, 2.0f, 7.0f));
}

TEST_CASE("AABB - MergeWithAABB two overlapping boxes", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(5.0f));
    AABB b(glm::vec3(3.0f), glm::vec3(8.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(0.0f));
    REQUIRE(a.GetMax() == glm::vec3(8.0f));
}

TEST_CASE("AABB - MergeWithAABB one box fully inside the other", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB inner(glm::vec3(2.0f), glm::vec3(4.0f));
    outer.MergeWithAABB(inner);

    REQUIRE(outer.GetMin() == glm::vec3(0.0f));
    REQUIRE(outer.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - MergeWithAABB inner absorbs outer expands to outer bounds", "[physics][aabb]") {
    AABB outer(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB inner(glm::vec3(2.0f), glm::vec3(4.0f));
    inner.MergeWithAABB(outer);

    REQUIRE(inner.GetMin() == glm::vec3(0.0f));
    REQUIRE(inner.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - MergeWithAABB identical boxes produces same box", "[physics][aabb]") {
    AABB a(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f));
    AABB b(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(a.GetMax() == glm::vec3(4.0f, 5.0f, 6.0f));
}

TEST_CASE("AABB - MergeWithAABB with itself does not change bounds", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(5.0f));
    a.MergeWithAABB(a);

    REQUIRE(a.GetMin() == glm::vec3(0.0f));
    REQUIRE(a.GetMax() == glm::vec3(5.0f));
}

TEST_CASE("AABB - MergeWithAABB with zero-volume AABB inside expands nothing", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));
    AABB point(glm::vec3(5.0f), glm::vec3(5.0f));
    box.MergeWithAABB(point);

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - MergeWithAABB with zero-volume AABB outside expands to include it", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(5.0f));
    AABB point(glm::vec3(10.0f), glm::vec3(10.0f));
    box.MergeWithAABB(point);

    REQUIRE(box.GetMin() == glm::vec3(0.0f));
    REQUIRE(box.GetMax() == glm::vec3(10.0f));
}

TEST_CASE("AABB - MergeWithAABB of two zero-volume AABBs at different points", "[physics][aabb]") {
    AABB a(glm::vec3(1.0f), glm::vec3(1.0f));
    AABB b(glm::vec3(5.0f), glm::vec3(5.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(1.0f));
    REQUIRE(a.GetMax() == glm::vec3(5.0f));
}

TEST_CASE("AABB - MergeWithAABB result contains both original mins and maxes", "[physics][aabb]") {
    AABB a(glm::vec3(-3.0f, 0.0f, 1.0f), glm::vec3(2.0f, 4.0f, 5.0f));
    AABB b(glm::vec3(0.0f, -2.0f, 3.0f), glm::vec3(6.0f, 1.0f, 9.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(-3.0f, -2.0f, 1.0f));
    REQUIRE(a.GetMax() == glm::vec3(6.0f, 4.0f, 9.0f));
}

TEST_CASE("AABB - MergeWithAABB result contains all original corners", "[physics][aabb]") {
    AABB a(glm::vec3(0.0f), glm::vec3(3.0f));
    AABB b(glm::vec3(5.0f, 1.0f, 2.0f), glm::vec3(8.0f, 6.0f, 7.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.Contains(glm::vec3(0.0f)));
    REQUIRE(a.Contains(glm::vec3(3.0f)));
    REQUIRE(a.Contains(glm::vec3(5.0f, 1.0f, 2.0f)));
    REQUIRE(a.Contains(glm::vec3(8.0f, 6.0f, 7.0f)));
}

TEST_CASE("AABB - MergeWithAABB is commutative in result bounds", "[physics][aabb]") {
    AABB a1(glm::vec3(-1.0f, 0.0f, 2.0f), glm::vec3(3.0f, 4.0f, 5.0f));
    AABB a2(glm::vec3(-1.0f, 0.0f, 2.0f), glm::vec3(3.0f, 4.0f, 5.0f));
    AABB b(glm::vec3(1.0f, -2.0f, 0.0f), glm::vec3(6.0f, 3.0f, 8.0f));

    a1.MergeWithAABB(b);
    b.MergeWithAABB(a2);

    REQUIRE(a1.GetMin() == b.GetMin());
    REQUIRE(a1.GetMax() == b.GetMax());
}

TEST_CASE("AABB - MergeWithAABB with negative-coordinate boxes", "[physics][aabb]") {
    AABB a(glm::vec3(-10.0f, -8.0f, -6.0f), glm::vec3(-5.0f, -3.0f, -1.0f));
    AABB b(glm::vec3(-20.0f, -1.0f, -4.0f), glm::vec3(-7.0f, 2.0f, 0.0f));
    a.MergeWithAABB(b);

    REQUIRE(a.GetMin() == glm::vec3(-20.0f, -8.0f, -6.0f));
    REQUIRE(a.GetMax() == glm::vec3(-5.0f, 2.0f, 0.0f));
}
