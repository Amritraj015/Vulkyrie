#include "physics/collision/shapes/capsule_shape.h"

namespace Vulkyrie {

    CapsuleShape::CapsuleShape(f32 radius, f32 height)
        : ConvexShape(CollisionShapeType::Capsule, CollisionShapeName::Capsule, radius)
        , _halfHeight(height * 0.5f) {
        VASSERT(radius > 0.0f, "Radius must be positive for capsule shape.");
        VASSERT(height > 0.0f, "Height must be positive for capsule shape.");
    }

    glm::vec3 CapsuleShape::GetLocalInertiaTensor(f32 mass) const {
        // The inertia tensor for a capsule is computed as a weighted sum of
        // the inertia of the cylindrical part and the two hemispherical caps.
        // The formulas below assume the capsule is aligned with the y-axis.
        //
        // Let r = _margin (radius), h = height of the cylindrical part (2 * _halfHeight)
        //
        // The total mass is split between the cylinder and the two hemispheres:
        //   mass_cylinder = mass * (2*r) / (4*r + 3*h)
        //   mass_spheres  = mass * (3*h) / (4*r + 3*h)
        //
        // The inertia tensor for a solid cylinder (about its center):
        //   I_cylinder_xx = I_cylinder_zz = (1/12) * m * (3*r^2 + h^2)
        //   I_cylinder_yy = (1/2) * m * r^2
        //
        // The inertia tensor for a solid sphere (about its center):
        //   I_sphere = (2/5) * m * r^2
        //
        // The code below combines these using the parallel axis theorem and appropriate mass ratios.
        const f32 height = _halfHeight + _halfHeight;
        const f32 radiusSquared = _margin * _margin;
        const f32 heightSquared = height * height;
        const f32 radiusSquaredDoubled = radiusSquared + radiusSquared;
        const f32 factorOne = f32(2.0f) * _margin / (f32(4.0f) * _margin + f32(3.0f) * height); // mass ratio for spheres
        const f32 factorTwo = f32(3.0f) * height / (f32(4.0f) * _margin + f32(3.0f) * height);  // mass ratio for cylinder
        const f32 sum1 = f32(0.4f) * radiusSquaredDoubled;                                      // (2/5) * r^2 for both spheres

        const f32 sum2 = f32(0.75) * height * _margin + f32(0.5) * heightSquared;     // parallel axis terms for spheres
        const f32 sum3 = f32(0.25) * radiusSquared + f32(1.0 / 12.0) * heightSquared; // cylinder inertia
        const f32 IxxAndzz = factorOne * mass * (sum1 + sum2) + factorTwo * mass * sum3;
        const f32 Iyy = factorOne * mass * sum1 + factorTwo * mass * f32(0.25) * radiusSquaredDoubled;

        return glm::vec3(IxxAndzz, Iyy, IxxAndzz);
    }

    bool CapsuleShape::ContainsPoint(const glm::vec3 &localPoint) const {
        // Checks if a point is inside the capsule.
        // The capsule consists of a cylinder (centered on the y-axis, height 2*_halfHeight, radius _margin)
        // and two hemispherical end caps (centered at y = +_halfHeight and y = -_halfHeight).
        //
        // The point is inside the capsule if it is:
        //   - inside the infinite cylinder (x^2 + z^2 < r^2, and y between the caps), or
        //   - inside either hemisphere (distance to cap center < r)
        const f32 diffYCenterSphere1 = localPoint.y - _halfHeight; // distance to top cap center
        const f32 diffYCenterSphere2 = localPoint.y + _halfHeight; // distance to bottom cap center
        const f32 xSquare = localPoint.x * localPoint.x;
        const f32 zSquare = localPoint.z * localPoint.z;
        const f32 squareRadius = _margin * _margin;

        // Cylinder check: point is within radius in xz-plane and between the caps in y
        // Hemisphere checks: point is within radius of either cap center
        return ((xSquare + zSquare) < squareRadius && localPoint.y < _halfHeight && localPoint.y > -_halfHeight) ||
               (xSquare + zSquare + diffYCenterSphere1 * diffYCenterSphere1) < squareRadius ||
               (xSquare + zSquare + diffYCenterSphere2 * diffYCenterSphere2) < squareRadius;
    }

} // namespace Vulkyrie
