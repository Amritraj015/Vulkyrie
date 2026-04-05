#pragma once

#include "core/time_step.h"

namespace Vulkyrie {

    class PhysicsObject {
        public:
            void update(Timestep deltaTime);
    };

} // namespace Vulkyrie
