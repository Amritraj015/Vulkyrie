#include "physics/collision/shapes/collision_shape.h"

namespace Vulkyrie {

    CollisionShape::CollisionShape(CollisionShapeType type, CollisionShapeName name)
        : _type(type)
        , _name(name) {
    }

} // namespace Vulkyrie
