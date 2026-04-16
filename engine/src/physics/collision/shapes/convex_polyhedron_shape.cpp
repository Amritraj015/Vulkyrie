#include "physics/collision/shapes/convex_polyhedron_shape.h"

namespace Vulkyrie {

    ConvexPolyhedronShape::ConvexPolyhedronShape(CollisionShapeName name, f32 margin)
        : ConvexShape(CollisionShapeType::ConvexPolyhedron, name, margin) {
    }

} // namespace Vulkyrie
