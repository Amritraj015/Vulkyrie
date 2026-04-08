#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief Component that represents the position, rotation, and scale of an entity in 3D space. */
    struct TransformComponent final {
        public:
            /** Position of the entity in 3D space. */
            glm::vec3 Position;

            /** Rotation of the entity represented as a quaternion. */
            glm::quat Rotation;

            /** Scale factor for the entity in 3D space. */
            glm::vec3 Scale;
    };

} // namespace Vulkyrie
