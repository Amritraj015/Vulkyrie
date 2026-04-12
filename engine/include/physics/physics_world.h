#pragma once

#include "physics/physics_world_settings.h"
#include "core/entity_manager.h"
// #include "physics/components/body_component_store.h"
// #include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"
// #include "physics/components/collider_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld {
        public:
            explicit PhysicsWorld(const PhysicsWorldSettings &settings)
                : _settings(settings)
                , _entityManager() {
                // , _bodyComponentStore()
                // , _colliderComponentStore()
                // , _transformComponentStore() {
            }

            [[nodiscard]] VE_FORCE_INLINE TransformComponentStore &GetTransformComponentStore() {
                return _transformComponentStore;
            }

            void Update() {
            }

        private:
            PhysicsWorldSettings _settings;
            EntityManager _entityManager;
            // BodyComponentStore _bodyComponentStore;
            // RigidBodyComponentStore _rigidBodyComponentStore;
            // ColliderComponentStore _colliderComponentStore;
            TransformComponentStore _transformComponentStore;
    };

} // namespace Vulkyrie
