#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <vulkyrie.h>

#include "physics/collision/shapes/capsule_shape.h"

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

TEST_CASE("CapsuleShape - GetRadius returns constructed radius", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 4.0f);

    REQUIRE(shape.GetRadius() == 2.0f);
}

TEST_CASE("CapsuleShape - GetHeight returns constructed height", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 6.0f);

    REQUIRE(shape.GetHeight() == Catch::Approx(6.0f));
}

TEST_CASE("CapsuleShape - GetType returns Capsule", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);

    REQUIRE(shape.GetType() == CollisionShapeType::Capsule);
}

TEST_CASE("CapsuleShape - GetName returns Capsule", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);

    REQUIRE(shape.GetName() == CollisionShapeName::Capsule);
}

// ===========================================================================================
// Shape classification
// ===========================================================================================

TEST_CASE("CapsuleShape - IsConvex returns true", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);

    REQUIRE(shape.IsConvex() == true);
}

TEST_CASE("CapsuleShape - IsPolyhedral returns false", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);

    REQUIRE(shape.IsPolyhedral() == false);
}

// ===========================================================================================
// SetRadius
// ===========================================================================================

TEST_CASE("CapsuleShape - SetRadius updates GetRadius", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    shape.SetRadius(3.0f);

    REQUIRE(shape.GetRadius() == 3.0f);
}

TEST_CASE("CapsuleShape - SetRadius updates GetLocalAABB", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    shape.SetRadius(2.0f);
    const AABB aabb = shape.GetLocalAABB();

    REQUIRE(aabb.GetMin().x == Catch::Approx(-2.0f));
    REQUIRE(aabb.GetMin().z == Catch::Approx(-2.0f));
    REQUIRE(aabb.GetMax().x == Catch::Approx(2.0f));
    REQUIRE(aabb.GetMax().z == Catch::Approx(2.0f));
}

TEST_CASE("CapsuleShape - SetRadius updates GetVolume", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    shape.SetRadius(2.0f);
    const f32 expected = glm::pi<f32>() * 4.0f * ((4.0f / 3.0f) * 2.0f + 2.0f * 2.0f);

    REQUIRE(shape.GetVolume() == Catch::Approx(expected));
}

// ===========================================================================================
// SetHeight
// ===========================================================================================

TEST_CASE("CapsuleShape - SetHeight updates GetHeight", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);
    shape.SetHeight(8.0f);

    REQUIRE(shape.GetHeight() == Catch::Approx(8.0f));
}

TEST_CASE("CapsuleShape - SetHeight updates GetLocalAABB", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);
    shape.SetHeight(10.0f);
    const AABB aabb = shape.GetLocalAABB();

    // Y extents: halfHeight + radius = 5 + 1 = 6
    REQUIRE(aabb.GetMin().y == Catch::Approx(-6.0f));
    REQUIRE(aabb.GetMax().y == Catch::Approx(6.0f));
}

TEST_CASE("CapsuleShape - SetHeight updates GetVolume", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 2.0f);
    shape.SetHeight(6.0f);
    const f32 expected = glm::pi<f32>() * 1.0f * ((4.0f / 3.0f) * 1.0f + 2.0f * 3.0f);

    REQUIRE(shape.GetVolume() == Catch::Approx(expected));
}

// ===========================================================================================
// GetLocalAABB
// ===========================================================================================

TEST_CASE("CapsuleShape - GetLocalAABB XZ extents equal radius", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 6.0f);
    const AABB aabb = shape.GetLocalAABB();

    REQUIRE(aabb.GetMin().x == Catch::Approx(-2.0f));
    REQUIRE(aabb.GetMin().z == Catch::Approx(-2.0f));
    REQUIRE(aabb.GetMax().x == Catch::Approx(2.0f));
    REQUIRE(aabb.GetMax().z == Catch::Approx(2.0f));
}

TEST_CASE("CapsuleShape - GetLocalAABB Y extents equal halfHeight plus radius", "[physics][capsule]") {
    CapsuleShape shape(1.5f, 4.0f);
    const AABB aabb = shape.GetLocalAABB();

    // halfHeight = 2.0, radius = 1.5, so Y extent = ±3.5
    REQUIRE(aabb.GetMin().y == Catch::Approx(-3.5f));
    REQUIRE(aabb.GetMax().y == Catch::Approx(3.5f));
}

TEST_CASE("CapsuleShape - GetLocalAABB is centered at origin", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    const AABB aabb = shape.GetLocalAABB();

    REQUIRE(aabb.GetCenter() == glm::vec3(0.0f));
}

// ===========================================================================================
// GetVolume
// ===========================================================================================

TEST_CASE("CapsuleShape - GetVolume matches pi*r^2*(4/3*r + 2*halfHeight)", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 6.0f);
    // halfHeight = 3, radius = 2
    const f32 expected = glm::pi<f32>() * 4.0f * ((4.0f / 3.0f) * 2.0f + 2.0f * 3.0f);

    REQUIRE(shape.GetVolume() == Catch::Approx(expected));
}

TEST_CASE("CapsuleShape - GetVolume reduces to sphere volume when height is near zero", "[physics][capsule]") {
    // Use a very small height so the capsule approximates a sphere of given radius.
    // Volume = pi*r^2*(4/3*r + 2*h) → pi*r^2*(4/3*r) = (4/3)*pi*r^3 as h→0
    const f32 r = 3.0f;
    const f32 h = 0.0001f;
    CapsuleShape shape(r, h);
    const f32 sphereVolume = (4.0f / 3.0f) * std::numbers::pi_v<f32> * r * r * r;

    REQUIRE(shape.GetVolume() == Catch::Approx(sphereVolume).epsilon(0.01f));
}

TEST_CASE("CapsuleShape - GetVolume is larger when height increases", "[physics][capsule]") {
    CapsuleShape s1(1.0f, 2.0f);
    CapsuleShape s2(1.0f, 4.0f);

    REQUIRE(s2.GetVolume() > s1.GetVolume());
}

// ===========================================================================================
// GetLocalInertiaTensor
// ===========================================================================================

TEST_CASE("CapsuleShape - GetLocalInertiaTensor Ixx equals Izz (symmetric about Y)", "[physics][capsule]") {
    CapsuleShape shape(1.5f, 4.0f);
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(1.0f);

    REQUIRE(inertia.x == Catch::Approx(inertia.z));
}

TEST_CASE("CapsuleShape - GetLocalInertiaTensor Iyy is less than Ixx for tall capsule", "[physics][capsule]") {
    // For a tall capsule (height >> radius) Iyy (spin axis) < Ixx (tip-over axis)
    CapsuleShape shape(0.5f, 10.0f);
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(1.0f);

    REQUIRE(inertia.y < inertia.x);
}

TEST_CASE("CapsuleShape - GetLocalInertiaTensor scales linearly with mass", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    const glm::vec3 i1 = shape.GetLocalInertiaTensor(1.0f);
    const glm::vec3 i2 = shape.GetLocalInertiaTensor(5.0f);

    REQUIRE(i2.x == Catch::Approx(i1.x * 5.0f));
    REQUIRE(i2.y == Catch::Approx(i1.y * 5.0f));
    REQUIRE(i2.z == Catch::Approx(i1.z * 5.0f));
}

TEST_CASE("CapsuleShape - GetLocalInertiaTensor is zero for zero mass", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(0.0f);

    REQUIRE(inertia.x == Catch::Approx(0.0f));
    REQUIRE(inertia.y == Catch::Approx(0.0f));
    REQUIRE(inertia.z == Catch::Approx(0.0f));
}

// ===========================================================================================
// ContainsPoint
// ===========================================================================================

TEST_CASE("CapsuleShape - ContainsPoint returns true for origin", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f)) == true);
}

TEST_CASE("CapsuleShape - ContainsPoint returns true for point in cylinder region", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 6.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(1.0f, 0.0f, 0.0f)) == true);
}

TEST_CASE("CapsuleShape - ContainsPoint returns true for point in top hemisphere", "[physics][capsule]") {
    // halfHeight = 2, radius = 1: top cap center at y=2. Point at y=2.5 is 0.5 from cap center → inside
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 2.5f, 0.0f)) == true);
}

TEST_CASE("CapsuleShape - ContainsPoint returns true for point in bottom hemisphere", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, -2.5f, 0.0f)) == true);
}

TEST_CASE("CapsuleShape - ContainsPoint returns false for point outside on X axis", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(2.0f, 0.0f, 0.0f)) == false);
}

TEST_CASE("CapsuleShape - ContainsPoint returns false for point above top cap", "[physics][capsule]") {
    // halfHeight = 2, radius = 1: total Y extent = 3. Point at y=3 is on surface → outside (exclusive)
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 4.0f, 0.0f)) == false);
}

TEST_CASE("CapsuleShape - ContainsPoint returns false for point below bottom cap", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, -4.0f, 0.0f)) == false);
}

// ===========================================================================================
// GetLocalSupportPointWithoutMargin
// ===========================================================================================

TEST_CASE("CapsuleShape - GetLocalSupportPointWithoutMargin returns top spine point for +Y direction", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 6.0f);
    // halfHeight = 3
    const glm::vec3 result = shape.GetLocalSupportPointWithoutMargin(glm::vec3(0.0f, 1.0f, 0.0f));

    REQUIRE(result.x == Catch::Approx(0.0f));
    REQUIRE(result.y == Catch::Approx(3.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("CapsuleShape - GetLocalSupportPointWithoutMargin returns bottom spine point for -Y direction", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 6.0f);
    const glm::vec3 result = shape.GetLocalSupportPointWithoutMargin(glm::vec3(0.0f, -1.0f, 0.0f));

    REQUIRE(result.x == Catch::Approx(0.0f));
    REQUIRE(result.y == Catch::Approx(-3.0f));
    REQUIRE(result.z == Catch::Approx(0.0f));
}

TEST_CASE("CapsuleShape - GetLocalSupportPointWithoutMargin returns top for lateral direction with positive Y", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    // Any direction with direction.y > 0 picks the top spine point
    const glm::vec3 result = shape.GetLocalSupportPointWithoutMargin(glm::normalize(glm::vec3(1.0f, 0.1f, 0.0f)));

    REQUIRE(result.y == Catch::Approx(2.0f));
}

// ===========================================================================================
// GetLocalSupportPointWithMargin
// ===========================================================================================

TEST_CASE("CapsuleShape - GetLocalSupportPointWithMargin in +Y direction has Y equal to halfHeight plus radius", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 6.0f);
    // halfHeight = 3, radius = 2 → top of capsule at y = 5
    const glm::vec3 result = shape.GetLocalSupportPointWithMargin(glm::vec3(0.0f, 1.0f, 0.0f));

    REQUIRE(result.y == Catch::Approx(5.0f));
}

TEST_CASE("CapsuleShape - GetLocalSupportPointWithMargin in -Y direction has Y equal to negative halfHeight minus radius", "[physics][capsule]") {
    CapsuleShape shape(2.0f, 6.0f);
    const glm::vec3 result = shape.GetLocalSupportPointWithMargin(glm::vec3(0.0f, -1.0f, 0.0f));

    REQUIRE(result.y == Catch::Approx(-5.0f));
}

TEST_CASE("CapsuleShape - GetLocalSupportPointWithMargin in +X direction has X equal to radius", "[physics][capsule]") {
    CapsuleShape shape(3.0f, 4.0f);
    const glm::vec3 result = shape.GetLocalSupportPointWithMargin(glm::vec3(1.0f, 0.0f, 0.0f));

    REQUIRE(result.x == Catch::Approx(3.0f));
}

// ===========================================================================================
// ComputeTransformedAABB
// ===========================================================================================

TEST_CASE("CapsuleShape - ComputeTransformedAABB at origin with identity rotation matches local AABB", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    const AABB local = shape.GetLocalAABB();
    const AABB transformed = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));

    REQUIRE(transformed.GetMin().x == Catch::Approx(local.GetMin().x));
    REQUIRE(transformed.GetMin().y == Catch::Approx(local.GetMin().y));
    REQUIRE(transformed.GetMin().z == Catch::Approx(local.GetMin().z));
    REQUIRE(transformed.GetMax().x == Catch::Approx(local.GetMax().x));
    REQUIRE(transformed.GetMax().y == Catch::Approx(local.GetMax().y));
    REQUIRE(transformed.GetMax().z == Catch::Approx(local.GetMax().z));
}

TEST_CASE("CapsuleShape - ComputeTransformedAABB translation moves center", "[physics][capsule]") {
    CapsuleShape shape(1.0f, 4.0f);
    const glm::vec3 pos(3.0f, -1.0f, 2.0f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(pos));

    REQUIRE(aabb.GetCenter().x == Catch::Approx(pos.x));
    REQUIRE(aabb.GetCenter().y == Catch::Approx(pos.y));
    REQUIRE(aabb.GetCenter().z == Catch::Approx(pos.z));
}
