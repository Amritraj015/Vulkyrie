#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <vulkyrie.h>

#include "physics/collision/shapes/sphere_shape.h"

using namespace Vulkyrie;

namespace {

    TransformComponent MakeTransform(glm::vec3 position, glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
        TransformComponent t;
        t.Position = position;
        t.Rotation = rotation;
        return t;
    }

} // namespace

// ===========================================================================================
// Constructor / Accessors
// ===========================================================================================

TEST_CASE("SphereShape - GetRadius returns constructed radius", "[physics][sphere]") {
    SphereShape shape(3.0f);

    REQUIRE(shape.GetRadius() == 3.0f);
}

TEST_CASE("SphereShape - GetMargin returns zero when not specified", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.GetMargin() == 0.0f);
}

TEST_CASE("SphereShape - GetMargin returns constructed margin", "[physics][sphere]") {
    SphereShape shape(1.0f, 0.05f);

    REQUIRE(shape.GetMargin() == 0.05f);
}

TEST_CASE("SphereShape - GetType returns Sphere", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.GetType() == CollisionShapeType::Sphere);
}

TEST_CASE("SphereShape - GetName returns Sphere", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.GetName() == CollisionShapeName::Sphere);
}

// ===========================================================================================
// Shape classification
// ===========================================================================================

TEST_CASE("SphereShape - IsConvex returns true", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.IsConvex() == true);
}

TEST_CASE("SphereShape - IsPolyhedral returns false", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.IsPolyhedral() == false);
}

// ===========================================================================================
// SetRadius
// ===========================================================================================

TEST_CASE("SphereShape - SetRadius updates GetRadius", "[physics][sphere]") {
    SphereShape shape(1.0f);
    shape.SetRadius(5.0f);

    REQUIRE(shape.GetRadius() == 5.0f);
}

TEST_CASE("SphereShape - SetRadius updates GetLocalAABB", "[physics][sphere]") {
    SphereShape shape(1.0f);
    shape.SetRadius(4.0f);
    const AABB aabb = shape.GetLocalAABB();

    REQUIRE(aabb.GetMin() == glm::vec3(-4.0f));
    REQUIRE(aabb.GetMax() == glm::vec3(4.0f));
}

// ===========================================================================================
// GetLocalAABB
// ===========================================================================================

TEST_CASE("SphereShape - GetLocalAABB min is negative radius", "[physics][sphere]") {
    SphereShape shape(2.5f);

    REQUIRE(shape.GetLocalAABB().GetMin() == glm::vec3(-2.5f));
}

TEST_CASE("SphereShape - GetLocalAABB max is positive radius", "[physics][sphere]") {
    SphereShape shape(2.5f);

    REQUIRE(shape.GetLocalAABB().GetMax() == glm::vec3(2.5f));
}

TEST_CASE("SphereShape - GetLocalAABB is centered at origin", "[physics][sphere]") {
    SphereShape shape(3.0f);

    REQUIRE(shape.GetLocalAABB().GetCenter() == glm::vec3(0.0f));
}

TEST_CASE("SphereShape - GetLocalAABB is a cube (all half-extents equal)", "[physics][sphere]") {
    SphereShape shape(7.0f);
    const AABB aabb = shape.GetLocalAABB();
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == halfExtents.y);
    REQUIRE(halfExtents.y == halfExtents.z);
}

// ===========================================================================================
// GetVolume
// ===========================================================================================

TEST_CASE("SphereShape - GetVolume matches (4/3)*pi*r^3 for r=1", "[physics][sphere]") {
    SphereShape shape(1.0f);
    const float expected = (4.0f / 3.0f) * std::numbers::pi_v<float>;

    REQUIRE(shape.GetVolume() == Catch::Approx(expected));
}

TEST_CASE("SphereShape - GetVolume scales as cube of radius", "[physics][sphere]") {
    SphereShape s1(1.0f);
    SphereShape s2(2.0f);

    REQUIRE(s2.GetVolume() == Catch::Approx(s1.GetVolume() * 8.0f));
}

// ===========================================================================================
// GetLocalInertiaTensor
// ===========================================================================================

TEST_CASE("SphereShape - GetLocalInertiaTensor is uniform (all components equal)", "[physics][sphere]") {
    SphereShape shape(3.0f);
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(1.0f);

    REQUIRE(inertia.x == Catch::Approx(inertia.y));
    REQUIRE(inertia.y == Catch::Approx(inertia.z));
}

TEST_CASE("SphereShape - GetLocalInertiaTensor matches (2/5)*mass*r^2", "[physics][sphere]") {
    SphereShape shape(3.0f);
    const float mass = 5.0f;
    const float expected = 0.4f * mass * 3.0f * 3.0f;
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(mass);

    REQUIRE(inertia.x == Catch::Approx(expected));
    REQUIRE(inertia.y == Catch::Approx(expected));
    REQUIRE(inertia.z == Catch::Approx(expected));
}

TEST_CASE("SphereShape - GetLocalInertiaTensor scales linearly with mass", "[physics][sphere]") {
    SphereShape shape(2.0f);
    const glm::vec3 i1 = shape.GetLocalInertiaTensor(1.0f);
    const glm::vec3 i2 = shape.GetLocalInertiaTensor(3.0f);

    REQUIRE(i2.x == Catch::Approx(i1.x * 3.0f));
}

TEST_CASE("SphereShape - GetLocalInertiaTensor is zero for zero mass", "[physics][sphere]") {
    SphereShape shape(1.0f);
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(0.0f);

    REQUIRE(inertia.x == Catch::Approx(0.0f));
    REQUIRE(inertia.y == Catch::Approx(0.0f));
    REQUIRE(inertia.z == Catch::Approx(0.0f));
}

// ===========================================================================================
// ContainsPoint
// ===========================================================================================

TEST_CASE("SphereShape - ContainsPoint returns true for origin", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f)) == true);
}

TEST_CASE("SphereShape - ContainsPoint returns true for point strictly inside", "[physics][sphere]") {
    SphereShape shape(2.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(1.0f, 0.0f, 0.0f)) == true);
}

TEST_CASE("SphereShape - ContainsPoint returns false for point on surface", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(1.0f, 0.0f, 0.0f)) == false);
}

TEST_CASE("SphereShape - ContainsPoint returns false for point outside", "[physics][sphere]") {
    SphereShape shape(1.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(2.0f, 0.0f, 0.0f)) == false);
}

TEST_CASE("SphereShape - ContainsPoint works with negative coordinates", "[physics][sphere]") {
    SphereShape shape(3.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(-1.0f, -1.0f, -1.0f)) == true);
    REQUIRE(shape.ContainsPoint(glm::vec3(-3.0f, 0.0f, 0.0f)) == false);
}

TEST_CASE("SphereShape - ContainsPoint boundary is exclusive along each axis", "[physics][sphere]") {
    SphereShape shape(5.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(5.0f, 0.0f, 0.0f)) == false);
    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 5.0f, 0.0f)) == false);
    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 0.0f, 5.0f)) == false);
}

// ===========================================================================================
// ComputeTransformedAABB
// ===========================================================================================

TEST_CASE("SphereShape - ComputeTransformedAABB at origin with identity rotation matches local AABB", "[physics][sphere]") {
    SphereShape shape(2.0f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));

    REQUIRE(aabb.GetMin() == glm::vec3(-2.0f));
    REQUIRE(aabb.GetMax() == glm::vec3(2.0f));
}

TEST_CASE("SphereShape - ComputeTransformedAABB translation moves center", "[physics][sphere]") {
    SphereShape shape(1.0f);
    const glm::vec3 pos(3.0f, -2.0f, 5.0f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(pos));

    REQUIRE(aabb.GetMin().x == Catch::Approx(pos.x - 1.0f));
    REQUIRE(aabb.GetMin().y == Catch::Approx(pos.y - 1.0f));
    REQUIRE(aabb.GetMin().z == Catch::Approx(pos.z - 1.0f));
    REQUIRE(aabb.GetMax().x == Catch::Approx(pos.x + 1.0f));
    REQUIRE(aabb.GetMax().y == Catch::Approx(pos.y + 1.0f));
    REQUIRE(aabb.GetMax().z == Catch::Approx(pos.z + 1.0f));
}

TEST_CASE("SphereShape - ComputeTransformedAABB is unaffected by rotation", "[physics][sphere]") {
    SphereShape shape(2.0f);
    const glm::quat rot90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const AABB noRotation = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));
    const AABB withRotation = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f), rot90));

    REQUIRE(withRotation.GetMin().x == Catch::Approx(noRotation.GetMin().x));
    REQUIRE(withRotation.GetMin().y == Catch::Approx(noRotation.GetMin().y));
    REQUIRE(withRotation.GetMin().z == Catch::Approx(noRotation.GetMin().z));
    REQUIRE(withRotation.GetMax().x == Catch::Approx(noRotation.GetMax().x));
    REQUIRE(withRotation.GetMax().y == Catch::Approx(noRotation.GetMax().y));
    REQUIRE(withRotation.GetMax().z == Catch::Approx(noRotation.GetMax().z));
}

TEST_CASE("SphereShape - ComputeTransformedAABB is always a cube", "[physics][sphere]") {
    SphereShape shape(3.0f);
    const glm::quat rot = glm::angleAxis(glm::radians(37.0f), glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f)));
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(1.0f, -2.0f, 4.0f), rot));
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == Catch::Approx(halfExtents.y));
    REQUIRE(halfExtents.y == Catch::Approx(halfExtents.z));
}

TEST_CASE("SphereShape - ComputeTransformedAABB half-extents equal radius plus margin", "[physics][sphere]") {
    SphereShape shape(2.0f, 0.1f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == Catch::Approx(2.1f));
    REQUIRE(halfExtents.y == Catch::Approx(2.1f));
    REQUIRE(halfExtents.z == Catch::Approx(2.1f));
}

TEST_CASE("SphereShape - ComputeTransformedAABB with zero margin half-extents equal radius", "[physics][sphere]") {
    SphereShape shape(3.0f, 0.0f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == Catch::Approx(3.0f));
    REQUIRE(halfExtents.y == Catch::Approx(3.0f));
    REQUIRE(halfExtents.z == Catch::Approx(3.0f));
}
