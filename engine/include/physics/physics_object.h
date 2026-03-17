#pragma once

#include "core/time_step.h"

namespace Vulkyrie::Physics {

    class PhysicsObject {
        public:
            void update(Vulkyrie::Core::Timestep deltaTime);
    };

} // namespace Vulkyrie::Physics
