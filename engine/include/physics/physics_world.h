#pragma once

#include "physics/physics_world_settings.h"
// #include "ecs/entity_manager.h"

namespace Vulkyrie::Physics {

    class PhysicsWorld {
        public:
            PhysicsWorld(const PhysicsWorldSettings &settings)
                : _settings(settings) {
            }

        private:
            PhysicsWorldSettings _settings;
    };

} // namespace Vulkyrie::Physics
