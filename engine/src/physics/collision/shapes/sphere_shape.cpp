#include "physics/collision/shapes/sphere_shape.h"

namespace Vulkyrie {

    SphereShape::SphereShape(f32 radius)
        : ConvexShape(CollisionShapeType::Sphere, CollisionShapeName::Sphere, radius) {
        VASSERT(radius > 0.0f, "Radius must be positive for sphere shape.");
    }

} // namespace Vulkyrie
