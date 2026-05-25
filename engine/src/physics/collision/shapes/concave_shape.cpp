#include "physics/collision/shapes/concave_shape.h"

namespace Vulkyrie {

    ConcaveShape::ConcaveShape(CollisionShapeName name, const glm::vec3 &scale)
        : CollisionShape(CollisionShapeType::Concave, name)
        , _scale(scale)
        , _triangleRaycastSide(TriangleRaycastSide::Front) {
    }

    f32 ConcaveShape::GetVolume() const {
        // Compute the local AABB.
        const AABB aabb = GetLocalAABB();
        const glm::vec3 &max = aabb.GetMax();
        const glm::vec3 &min = aabb.GetMin();

        // Compute the lengths of the AABB along each axis.
        const f32 lengthX = max.x - min.x;
        const f32 lengthY = max.y - min.y;
        const f32 lengthZ = max.z - min.z;

        // The volume of the AABB is the product of its lengths along each axis.
        return lengthX * lengthY * lengthZ;
    }

} // namespace Vulkyrie
