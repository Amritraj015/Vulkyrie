#pragma once

#include "physics/physics_world_settings.h"
#include "ecs/entity_manager.h"
#include "ecs/components/transform_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld {
        public:
            explicit PhysicsWorld(const PhysicsWorldSettings &settings)
                : _settings(settings) {
            }

            void Update() {
            }

        private:
            PhysicsWorldSettings _settings;
            EntityManager _entityManager;
            TransformComponentStore _transformComponentStore;
    };

} // namespace Vulkyrie
