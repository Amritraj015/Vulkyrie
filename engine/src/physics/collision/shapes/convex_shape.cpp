#include "physics/collision/shapes/convex_shape.h"
#include "physics/physics_constants.h"
#include "core/constants.h"

namespace Vulkyrie {

    ConvexShape::ConvexShape(CollisionShapeType type, CollisionShapeName name, f32 margin, u32 id)
        : CollisionShape(type, name, id)
        , _margin(margin) {
    }

    glm::vec3 ConvexShape::GetLocalSupportPointWithMargin(const glm::vec3 &direction) const {
        glm::vec3 supportPoint = GetLocalSupportPointWithoutMargin(direction);

        if (_margin != f32(0.0)) {
            glm::vec3 unitVector(0.0f, -1.0f, 0.0f);

            if (glm::length2(unitVector) > VE_MACHINE_EPSILON * VE_MACHINE_EPSILON) {
                unitVector = glm::normalize(direction);
            }

            supportPoint += unitVector * _margin;
        }

        return supportPoint;
    }

} // namespace Vulkyrie
