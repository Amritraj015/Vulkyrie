#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/collider.h"

namespace Vulkyrie {

    CollisionShape::CollisionShape(CollisionShapeType type, CollisionShapeName name)
        : _type(type)
        , _name(name) {
    }

    AABB CollisionShape::ComputeTransformedAABB(const TransformComponent &transform) const {
        // Start from the shape's local (untransformed) AABB
        AABB aabb = GetLocalAABB();

        // Decompose the local AABB into center + half-extents for the transformation
        const glm::vec3 center = aabb.GetCenter();
        const glm::vec3 halfExtents = (aabb.GetMax() - aabb.GetMin()) * 0.5f;

        // Build the rotation matrix from the transform's quaternion
        const glm::mat3 matrix = glm::mat3_cast(transform.Rotation);

        // Rotate the local center and apply the world-space translation
        const glm::vec3 newCenter = matrix * center + transform.Position;

        // The tightest AABB around a rotated box has half-extents equal to |R| * localHalfExtents,
        // where |R| is the element-wise absolute value of the rotation matrix. This avoids
        // a branching per-element loop while producing the same result.
        const glm::mat3 absMatrix(glm::abs(matrix[0]), glm::abs(matrix[1]), glm::abs(matrix[2]));
        const glm::vec3 newHalfExtents = absMatrix * halfExtents;

        aabb.SetMinMax(newCenter - newHalfExtents, newCenter + newHalfExtents);

        return aabb;
    }

    void CollisionShape::NotifyCollidersOfShapeChange() const {
        for (Collider *collider : _colliders) {
            collider->SetHasColliderShapeChanged(true);
        }
    }

} // namespace Vulkyrie
