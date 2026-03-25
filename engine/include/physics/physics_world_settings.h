#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Physics {

    struct PhysicsWorldSettings {
        public:
            std::string Name;
            glm::vec3 Gravity;
            f64 FrictionCoefficient;
    };

} // namespace Vulkyrie::Physics
