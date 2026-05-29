#include "physics/collision/shapes/box_shape.h"

namespace Vulkyrie {

    BoxShape::BoxShape(const glm::vec3 &halfExtents, PhysicsContext &context)
        : ConvexPolyhedronShape(CollisionShapeName::Box)
        , _halfExtents(halfExtents)
        , _boxShapeHalfEdgeMesh(context.GetBoxShapeHalfEdgeMesh()) {
        VASSERT(halfExtents.x > 0.0f && halfExtents.y > 0.0f && halfExtents.z > 0.0f, "Half extents must be positive for box shape.");
    }

} // namespace Vulkyrie
