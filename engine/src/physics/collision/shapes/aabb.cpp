#include "physics/collision/shapes/aabb.h"

namespace Vulkyrie {

    AABB::AABB(const glm::vec3 &minCoordinates, const glm::vec3 &maxCoordinates)
        : _minCoordinates(minCoordinates)
        , _maxCoordinates(maxCoordinates) {
        VASSERT_EXPR(minCoordinates.x <= maxCoordinates.x && minCoordinates.y <= maxCoordinates.y && minCoordinates.z <= maxCoordinates.z,
                     "Min coordinates must not exceed max coordinates.");
    }

} // namespace Vulkyrie
