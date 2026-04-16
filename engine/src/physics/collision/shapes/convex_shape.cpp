#include "physics/collision/shapes/convex_shape.h"

namespace Vulkyrie {

    ConvexShape::ConvexShape(CollisionShapeType type, CollisionShapeName name, f32 margin)
        : CollisionShape(type, name)
        , _margin(margin) {
    }

} // namespace Vulkyrie
