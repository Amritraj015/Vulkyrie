#include "physics/collision/shapes/convex_polyhedron_shape.h"

namespace Vulkyrie {

    ConvexPolyhedronShape::ConvexPolyhedronShape(CollisionShapeName name, f32 margin, u32 id)
        : ConvexShape(CollisionShapeType::ConvexPolyhedron, name, margin, id) {
    }

} // namespace Vulkyrie
