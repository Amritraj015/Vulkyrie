#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie::Physics;

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
// ContainsPoint
// ===========================================================================================

TEST_CASE("AABB - ContainsPoint inside", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(5.0f)));
}

TEST_CASE("AABB - ContainsPoint outside", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(15.0f)));
}

TEST_CASE("AABB - ContainsPoint on min corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(0.0f)));
}

TEST_CASE("AABB - ContainsPoint on max corner", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(10.0f)));
}

TEST_CASE("AABB - ContainsPoint on face", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(0.0f, 5.0f, 5.0f)));
    REQUIRE(box.ContainsPoint(glm::vec3(5.0f, 10.0f, 5.0f)));
    REQUIRE(box.ContainsPoint(glm::vec3(5.0f, 5.0f, 0.0f)));
}

TEST_CASE("AABB - ContainsPoint on edge", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(0.0f, 0.0f, 5.0f)));
    REQUIRE(box.ContainsPoint(glm::vec3(10.0f, 10.0f, 5.0f)));
}

TEST_CASE("AABB - ContainsPoint just outside each axis", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(10.0f));

    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(-0.001f, 5.0f, 5.0f)));
    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(5.0f, 10.001f, 5.0f)));
    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(5.0f, 5.0f, -0.001f)));
}

TEST_CASE("AABB - ContainsPoint in zero-volume box at same point", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(5.0f)));
}

TEST_CASE("AABB - ContainsPoint in zero-volume box at different point", "[physics][aabb]") {
    AABB box(glm::vec3(5.0f), glm::vec3(5.0f));

    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(5.001f)));
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

    REQUIRE_FALSE(box.ContainsPoint(glm::vec3(8.0f)));

    box.Scale(glm::vec3(2.0f));

    REQUIRE(box.ContainsPoint(glm::vec3(8.0f)));
}

TEST_CASE("AABB - Encapsulate then check volume grows", "[physics][aabb]") {
    AABB box(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(box.GetVolume() == 1.0f);

    box.Encapsulate(glm::vec3(2.0f, 1.0f, 1.0f));

    REQUIRE(box.GetVolume() == 2.0f);
}
