#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    struct PhysicsWorldSettings {
        public:
            PhysicsWorldSettings(const std::string &name, const glm::vec3 &gravity = glm::vec3(0.0f, -9.81f, 0.0f), f32 frictionCoefficient = 0.5)
                : Name(name)
                , Gravity(gravity)
                , FrictionCoefficient(frictionCoefficient) {
            }

            std::string Name;
            glm::vec3 Gravity;
            f32 FrictionCoefficient;
    };

} // namespace Vulkyrie
