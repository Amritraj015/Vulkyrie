#pragma once

#include "physics/physics_world_settings.h"
#include "ecs/components/transform_component_manager.h"

namespace Vulkyrie::Physics {

    class PhysicsWorld {
        public:
            PhysicsWorld(const PhysicsWorldSettings &settings)
                : _settings(settings) {
            }

        private:
            PhysicsWorldSettings _settings;
            TransformComponentManager _transformComponentManager;
    };

} // namespace Vulkyrie::Physics
