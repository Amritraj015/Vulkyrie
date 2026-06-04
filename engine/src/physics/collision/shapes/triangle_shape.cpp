#include "physics/collision/shapes/triangle_shape.h"
#include "physics/physics_constants.h"

namespace Vulkyrie {

    TriangleShape::TriangleShape(const glm::vec3 vertices[3], const glm::vec3 vertexNormals[3], u32 shapeID, HalfEdgeMesh &halfEdgeStructure)
        : ConvexPolyhedronShape(CollisionShapeName::Triangle, 0.0f, shapeID)
        , _raycastSide(TriangleRaycastSide::Front)
        , _halfEdgeStructure(halfEdgeStructure) {
        _vertices[0] = vertices[0];
        _vertices[1] = vertices[1];
        _vertices[2] = vertices[2];

        // Compute the face normal from the two edge vectors using the right-hand rule (CCW winding).
        _normal = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);

        VASSERT(glm::length2(_normal) > VE_MACHINE_EPSILON * VE_MACHINE_EPSILON, "Triangle has zero-length normal — it may be degenerate (zero area).");

        _normal = glm::normalize(_normal);

        _vertexNormals[0] = vertexNormals[0];
        _vertexNormals[1] = vertexNormals[1];
        _vertexNormals[2] = vertexNormals[2];
    }

    TriangleShape::TriangleShape(const glm::vec3 vertices[3], u32 shapeID, HalfEdgeMesh &halfEdgeStructure)
        : ConvexPolyhedronShape(CollisionShapeName::Triangle, 0.0f, shapeID)
        , _raycastSide(TriangleRaycastSide::Front)
        , _halfEdgeStructure(halfEdgeStructure) {
        _vertices[0] = vertices[0];
        _vertices[1] = vertices[1];
        _vertices[2] = vertices[2];

        // Normal and vertex normals are intentionally zero — this constructor is for raycasting only.
        // Methods that require a valid normal (e.g. GetFaceNormal) must not be called on shapes built this way.
        _normal = glm::vec3(0.0f);
        _vertexNormals[0] = glm::vec3(0.0f);
        _vertexNormals[1] = glm::vec3(0.0f);
        _vertexNormals[2] = glm::vec3(0.0f);
    }

    AABB TriangleShape::ComputeTransformedAABB(const TransformComponent &transform) const {
        // Transform each vertex into world space, then compute the AABB as the
        // component-wise min/max across all three world-space positions.
        const glm::vec3 worldPoint1 = transform * _vertices[0];
        const glm::vec3 worldPoint2 = transform * _vertices[1];
        const glm::vec3 worldPoint3 = transform * _vertices[2];

        const glm::vec3 xAxis(worldPoint1.x, worldPoint2.x, worldPoint3.x);
        const glm::vec3 yAxis(worldPoint1.y, worldPoint2.y, worldPoint3.y);
        const glm::vec3 zAxis(worldPoint1.z, worldPoint2.z, worldPoint3.z);

        return AABB(
            glm::vec3(
                glm::min(xAxis.x, glm::min(xAxis.y, xAxis.z)), glm::min(yAxis.x, glm::min(yAxis.y, yAxis.z)), glm::min(zAxis.x, glm::min(zAxis.y, zAxis.z))),
            glm::vec3(
                glm::max(xAxis.x, glm::max(xAxis.y, xAxis.z)), glm::max(yAxis.x, glm::max(yAxis.y, yAxis.z)), glm::max(zAxis.x, glm::max(zAxis.y, zAxis.z))));
    }

    void TriangleShape::computeSmoothMeshContact(glm::vec3 localContactPointTriangle,
                                                 const TransformComponent &triangleShapeToWorldTransform,
                                                 const TransformComponent &worldToOtherShapeTransform,
                                                 const f32 penetrationDepth,
                                                 const bool isTriangleShape1,
                                                 glm::vec3 &outNewLocalContactPointOtherShape,
                                                 glm::vec3 &outSmoothWorldContactTriangleNormal) const {

        // Get the smooth contact normal of the mesh at the contact point on the triangle
        glm::vec3 triangleLocalNormal = computeSmoothLocalContactNormalForTriangle(localContactPointTriangle);

        // Convert the local contact normal into world-space
        glm::vec3 triangleWorldNormal = triangleShapeToWorldTransform.Rotation * triangleLocalNormal;

        // Penetration axis with direction from triangle to other shape
        glm::vec3 triangleToOtherShapePenAxis = isTriangleShape1 ? outSmoothWorldContactTriangleNormal : -outSmoothWorldContactTriangleNormal;

        // The triangle normal should be the one in the direction out of the current colliding face of the triangle
        if (glm::dot(triangleWorldNormal, triangleToOtherShapePenAxis) < f32(0.0)) {
            triangleWorldNormal = -triangleWorldNormal;
            triangleLocalNormal = -triangleLocalNormal;
        }

        // Compute the final contact normal from shape 1 to shape 2
        outSmoothWorldContactTriangleNormal = isTriangleShape1 ? triangleWorldNormal : -triangleWorldNormal;

        // Re-align the local contact point on the other shape such that it is aligned along the new contact normal
        glm::vec3 otherShapePointTriangleSpace = localContactPointTriangle - triangleLocalNormal * penetrationDepth;
        glm::vec3 otherShapePoint = worldToOtherShapeTransform * triangleShapeToWorldTransform * otherShapePointTriangleSpace;

        outNewLocalContactPointOtherShape = otherShapePoint;
    }

} // namespace Vulkyrie
