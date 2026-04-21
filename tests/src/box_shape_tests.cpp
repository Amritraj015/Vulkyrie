#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <tuple>
#include <vector>
#include <vulkyrie.h>

#include "physics/collision/shapes/box_shape.h"

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

TEST_CASE("BoxShape - GetHalfExtents returns constructed half extents", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));

    REQUIRE(shape.GetHalfExtents() == glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("BoxShape - GetMargin returns zero when not specified", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetMargin() == 0.0f);
}

TEST_CASE("BoxShape - GetMargin returns constructed margin", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f), 0.05f);

    REQUIRE(shape.GetMargin() == 0.05f);
}

TEST_CASE("BoxShape - GetType returns ConvexPolyhedron", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetType() == CollisionShapeType::ConvexPolyhedron);
}

TEST_CASE("BoxShape - GetName returns Box", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetName() == CollisionShapeName::Box);
}

// ===========================================================================================
// Shape classification
// ===========================================================================================

TEST_CASE("BoxShape - IsConvex returns true", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.IsConvex() == true);
}

TEST_CASE("BoxShape - IsPolyhedral returns true", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.IsPolyhedral() == true);
}

// ===========================================================================================
// SetHalfExtents
// ===========================================================================================

TEST_CASE("BoxShape - SetHalfExtents updates GetHalfExtents", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));
    shape.SetHalfExtents(glm::vec3(4.0f, 5.0f, 6.0f));

    REQUIRE(shape.GetHalfExtents() == glm::vec3(4.0f, 5.0f, 6.0f));
}

TEST_CASE("BoxShape - SetHalfExtents updates GetLocalAABB", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));
    shape.SetHalfExtents(glm::vec3(2.0f, 3.0f, 4.0f));
    const AABB aabb = shape.GetLocalAABB();

    REQUIRE(aabb.GetMin() == glm::vec3(-2.0f, -3.0f, -4.0f));
    REQUIRE(aabb.GetMax() == glm::vec3(2.0f, 3.0f, 4.0f));
}

// ===========================================================================================
// Topology counts
// ===========================================================================================

TEST_CASE("BoxShape - GetFacesCount returns 6", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetFacesCount() == 6);
}

TEST_CASE("BoxShape - GetVerticesCount returns 8", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetVerticesCount() == 8);
}

TEST_CASE("BoxShape - GetHafEdgesCount returns 24", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.GetHafEdgesCount() == 24);
}

// ===========================================================================================
// GetCentroid
// ===========================================================================================

TEST_CASE("BoxShape - GetCentroid is at origin", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(shape.GetCentroid() == glm::vec3(0.0f));
}

// ===========================================================================================
// GetVertexPosition
// ===========================================================================================

TEST_CASE("BoxShape - All 8 vertices are distinct", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    std::vector<glm::vec3> verts;
    for (u32 i = 0; i < shape.GetVerticesCount(); ++i)
        verts.push_back(shape.GetVertexPosition(i));

    for (u32 i = 0; i < verts.size(); ++i)
        for (u32 j = i + 1; j < verts.size(); ++j)
            REQUIRE(verts[i] != verts[j]);
}

TEST_CASE("BoxShape - All vertex absolute coordinates equal the half extents", "[physics][box]") {
    const glm::vec3 half(1.0f, 2.0f, 3.0f);
    BoxShape shape(half);

    for (u32 i = 0; i < shape.GetVerticesCount(); ++i) {
        const glm::vec3 v = shape.GetVertexPosition(i);
        REQUIRE(std::abs(v.x) == Catch::Approx(half.x));
        REQUIRE(std::abs(v.y) == Catch::Approx(half.y));
        REQUIRE(std::abs(v.z) == Catch::Approx(half.z));
    }
}

TEST_CASE("BoxShape - All 8 sign combinations of half-extents are covered by vertices", "[physics][box]") {
    const glm::vec3 half(1.0f, 2.0f, 3.0f);
    BoxShape shape(half);

    std::set<std::tuple<int, int, int>> signCombinations;
    for (u32 i = 0; i < shape.GetVerticesCount(); ++i) {
        const glm::vec3 v = shape.GetVertexPosition(i);
        signCombinations.insert({v.x > 0 ? 1 : -1, v.y > 0 ? 1 : -1, v.z > 0 ? 1 : -1});
    }

    REQUIRE(signCombinations.size() == 8);
}

// ===========================================================================================
// GetFaceNormal
// ===========================================================================================

TEST_CASE("BoxShape - All 6 face normals are unit vectors", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    for (u32 i = 0; i < shape.GetFacesCount(); ++i) {
        const glm::vec3 n = shape.GetFaceNormal(i);
        REQUIRE(glm::length(n) == Catch::Approx(1.0f));
    }
}

TEST_CASE("BoxShape - All 6 face normals are axis-aligned", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    for (u32 i = 0; i < shape.GetFacesCount(); ++i) {
        const glm::vec3 n = shape.GetFaceNormal(i);
        const int zeros = (n.x == 0.0f ? 1 : 0) + (n.y == 0.0f ? 1 : 0) + (n.z == 0.0f ? 1 : 0);
        REQUIRE(zeros == 2);
    }
}

TEST_CASE("BoxShape - All 6 axis directions are represented by face normals", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    std::set<std::tuple<float, float, float>> normals;
    for (u32 i = 0; i < shape.GetFacesCount(); ++i) {
        const glm::vec3 n = shape.GetFaceNormal(i);
        normals.insert({n.x, n.y, n.z});
    }

    REQUIRE(normals.size() == 6);
}

// ===========================================================================================
// GetLocalAABB
// ===========================================================================================

TEST_CASE("BoxShape - GetLocalAABB min equals negative half extents", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(shape.GetLocalAABB().GetMin() == glm::vec3(-2.0f, -3.0f, -4.0f));
}

TEST_CASE("BoxShape - GetLocalAABB max equals positive half extents", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(shape.GetLocalAABB().GetMax() == glm::vec3(2.0f, 3.0f, 4.0f));
}

TEST_CASE("BoxShape - GetLocalAABB is centered at origin", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 5.0f, 2.0f));

    REQUIRE(shape.GetLocalAABB().GetCenter() == glm::vec3(0.0f));
}

// ===========================================================================================
// GetVolume
// ===========================================================================================

TEST_CASE("BoxShape - GetVolume matches 8*hx*hy*hz", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const float expected = 8.0f * 1.0f * 2.0f * 3.0f;

    REQUIRE(shape.GetVolume() == Catch::Approx(expected));
}

TEST_CASE("BoxShape - GetVolume for unit cube (half=0.5) equals 1", "[physics][box]") {
    BoxShape shape(glm::vec3(0.5f));

    REQUIRE(shape.GetVolume() == Catch::Approx(1.0f));
}

TEST_CASE("BoxShape - GetVolume scales correctly when half extents double", "[physics][box]") {
    BoxShape s1(glm::vec3(1.0f));
    BoxShape s2(glm::vec3(2.0f));

    REQUIRE(s2.GetVolume() == Catch::Approx(s1.GetVolume() * 8.0f));
}

// ===========================================================================================
// GetLocalInertiaTensor
// ===========================================================================================

TEST_CASE("BoxShape - GetLocalInertiaTensor for unit cube has equal components", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(1.0f);

    REQUIRE(inertia.x == Catch::Approx(inertia.y));
    REQUIRE(inertia.y == Catch::Approx(inertia.z));
}

TEST_CASE("BoxShape - GetLocalInertiaTensor matches formula for non-uniform box", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const float mass = 1.0f;
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(mass);

    REQUIRE(inertia.x == Catch::Approx((1.0f / 3.0f) * mass * (4.0f + 9.0f)));
    REQUIRE(inertia.y == Catch::Approx((1.0f / 3.0f) * mass * (1.0f + 9.0f)));
    REQUIRE(inertia.z == Catch::Approx((1.0f / 3.0f) * mass * (1.0f + 4.0f)));
}

TEST_CASE("BoxShape - GetLocalInertiaTensor components are not all equal for non-uniform box", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(1.0f);

    REQUIRE(inertia.x != Catch::Approx(inertia.y));
    REQUIRE(inertia.y != Catch::Approx(inertia.z));
}

TEST_CASE("BoxShape - GetLocalInertiaTensor scales linearly with mass", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::vec3 i1 = shape.GetLocalInertiaTensor(1.0f);
    const glm::vec3 i2 = shape.GetLocalInertiaTensor(4.0f);

    REQUIRE(i2.x == Catch::Approx(i1.x * 4.0f));
    REQUIRE(i2.y == Catch::Approx(i1.y * 4.0f));
    REQUIRE(i2.z == Catch::Approx(i1.z * 4.0f));
}

TEST_CASE("BoxShape - GetLocalInertiaTensor is zero for zero mass", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::vec3 inertia = shape.GetLocalInertiaTensor(0.0f);

    REQUIRE(inertia.x == Catch::Approx(0.0f));
    REQUIRE(inertia.y == Catch::Approx(0.0f));
    REQUIRE(inertia.z == Catch::Approx(0.0f));
}

// ===========================================================================================
// ContainsPoint
// ===========================================================================================

TEST_CASE("BoxShape - ContainsPoint returns true for origin", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f)) == true);
}

TEST_CASE("BoxShape - ContainsPoint returns true for point strictly inside", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(1.0f, 1.0f, 1.0f)) == true);
}

TEST_CASE("BoxShape - ContainsPoint returns false for point on face boundary", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 3.0f, 4.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(2.0f, 0.0f, 0.0f)) == false);
    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 3.0f, 0.0f)) == false);
    REQUIRE(shape.ContainsPoint(glm::vec3(0.0f, 0.0f, 4.0f)) == false);
}

TEST_CASE("BoxShape - ContainsPoint returns false for point outside", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(2.0f, 0.0f, 0.0f)) == false);
}

TEST_CASE("BoxShape - ContainsPoint returns false for corner point on boundary", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(1.0f, 2.0f, 3.0f)) == false);
}

TEST_CASE("BoxShape - ContainsPoint works with negative coordinates", "[physics][box]") {
    BoxShape shape(glm::vec3(3.0f));

    REQUIRE(shape.ContainsPoint(glm::vec3(-1.0f, -1.0f, -1.0f)) == true);
    REQUIRE(shape.ContainsPoint(glm::vec3(-3.0f, 0.0f, 0.0f)) == false);
}

// ===========================================================================================
// ComputeTransformedAABB
// ===========================================================================================

TEST_CASE("BoxShape - ComputeTransformedAABB with identity transform matches local AABB", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));

    REQUIRE(aabb.GetMin().x == Catch::Approx(-1.0f));
    REQUIRE(aabb.GetMin().y == Catch::Approx(-2.0f));
    REQUIRE(aabb.GetMin().z == Catch::Approx(-3.0f));
    REQUIRE(aabb.GetMax().x == Catch::Approx(1.0f));
    REQUIRE(aabb.GetMax().y == Catch::Approx(2.0f));
    REQUIRE(aabb.GetMax().z == Catch::Approx(3.0f));
}

TEST_CASE("BoxShape - ComputeTransformedAABB translation moves the AABB center", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f));
    const glm::vec3 pos(5.0f, -3.0f, 2.0f);
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(pos));

    REQUIRE(aabb.GetCenter().x == Catch::Approx(pos.x));
    REQUIRE(aabb.GetCenter().y == Catch::Approx(pos.y));
    REQUIRE(aabb.GetCenter().z == Catch::Approx(pos.z));
}

TEST_CASE("BoxShape - ComputeTransformedAABB 90-degree Z rotation swaps X and Y extents", "[physics][box]") {
    BoxShape shape(glm::vec3(2.0f, 1.0f, 1.0f));
    const glm::quat rot90z = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f), rot90z));
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == Catch::Approx(1.0f));
    REQUIRE(halfExtents.y == Catch::Approx(2.0f));
    REQUIRE(halfExtents.z == Catch::Approx(1.0f));
}

TEST_CASE("BoxShape - ComputeTransformedAABB 90-degree X rotation swaps Y and Z extents", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 1.0f));
    const glm::quat rot90x = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const AABB aabb = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f), rot90x));
    const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

    REQUIRE(halfExtents.x == Catch::Approx(1.0f));
    REQUIRE(halfExtents.y == Catch::Approx(1.0f));
    REQUIRE(halfExtents.z == Catch::Approx(2.0f));
}

TEST_CASE("BoxShape - ComputeTransformedAABB AABB is never smaller than local AABB", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const AABB localAABB = shape.GetLocalAABB();
    const glm::vec3 localHalfExtents = (localAABB.GetMax() - localAABB.GetMin()) * 0.5f;

    const glm::quat rot = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
    const AABB transformed = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f), rot));
    const glm::vec3 transformedHalfExtents = (transformed.GetMax() - transformed.GetMin()) * 0.5f;

    REQUIRE(transformedHalfExtents.x >= localHalfExtents.x - 1e-5f);
    REQUIRE(transformedHalfExtents.y >= localHalfExtents.y - 1e-5f);
    REQUIRE(transformedHalfExtents.z >= localHalfExtents.z - 1e-5f);
}

TEST_CASE("BoxShape - ComputeTransformedAABB 180-degree rotation produces same AABB as no rotation", "[physics][box]") {
    BoxShape shape(glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::quat rot180 = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const AABB noRot   = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f)));
    const AABB withRot = shape.ComputeTransformedAABB(MakeTransform(glm::vec3(0.0f), rot180));

    REQUIRE(withRot.GetMin().x == Catch::Approx(noRot.GetMin().x));
    REQUIRE(withRot.GetMin().y == Catch::Approx(noRot.GetMin().y));
    REQUIRE(withRot.GetMin().z == Catch::Approx(noRot.GetMin().z));
    REQUIRE(withRot.GetMax().x == Catch::Approx(noRot.GetMax().x));
    REQUIRE(withRot.GetMax().y == Catch::Approx(noRot.GetMax().y));
    REQUIRE(withRot.GetMax().z == Catch::Approx(noRot.GetMax().z));
}
