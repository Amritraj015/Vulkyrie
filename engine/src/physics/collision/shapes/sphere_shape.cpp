#include "physics/collision/shapes/sphere_shape.h"

namespace Vulkyrie {

    SphereShape::SphereShape(f32 radius, f32 margin)
        : ConvexShape(CollisionShapeType::Sphere, CollisionShapeName::Sphere, margin)
        , _radius(radius) {
        VASSERT(radius > 0.0f, "Radius must be positive for sphere shape.");
    }

    AABB SphereShape::ComputeTransformedAABB(const TransformComponent &transform) const {
        // A sphere is rotationally symmetric, so its AABB is always a cube regardless of orientation.
        // The effective radius includes the collision margin so the broadphase doesn't miss contacts.
        const glm::vec3 halfExtents(_radius + _margin);

        // The local center is at the origin, so rotation has no effect — the world center is just the translation.
        return AABB(transform.Position - halfExtents, transform.Position + halfExtents);
    }

} // namespace Vulkyrie
