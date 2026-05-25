#include "physics/collision/shapes/convex_polyhedron_shape.h"

namespace Vulkyrie {

    ConvexPolyhedronShape::ConvexPolyhedronShape(CollisionShapeName name)
        : ConvexShape(CollisionShapeType::ConvexPolyhedron, name) {
    }

} // namespace Vulkyrie
