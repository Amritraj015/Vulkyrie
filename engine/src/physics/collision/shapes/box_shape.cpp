#include "physics/collision/shapes/box_shape.h"

namespace Vulkyrie {

    BoxShape::BoxShape(const glm::vec3 &halfExtents, f32 margin)
        : ConvexPolyhedronShape(CollisionShapeName::Box, margin)
        , _halfExtents(halfExtents) {
        VASSERT(halfExtents.x > 0.0f && halfExtents.y > 0.0f && halfExtents.z > 0.0f, "Half extents must be positive for box shape.");
    }

} // namespace Vulkyrie
