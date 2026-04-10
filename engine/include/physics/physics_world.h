#pragma once

#include "physics/physics_world_settings.h"
#include "core/entity_manager.h"
#include "physics/components/body_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld {
        public:
            explicit PhysicsWorld(const PhysicsWorldSettings &settings)
                : _settings(settings)
                , _entityManager()
                , _bodyComponentStore()
                , _transformComponentStore() {
            }

            void Update() {
            }

        private:
            PhysicsWorldSettings _settings;
            EntityManager _entityManager;
            BodyComponentStore _bodyComponentStore;
            RigidBodyComponentStore _rigidBodyComponentStore;
            TransformComponentStore _transformComponentStore;

            friend class Body;
    };

} // namespace Vulkyrie
