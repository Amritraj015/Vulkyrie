#pragma once

#include "core/entity_manager.h"
#include "physics/physics_world_settings.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld {
        public:
            explicit PhysicsWorld(const PhysicsWorldSettings &settings);

            [[nodiscard]] VE_FORCE_INLINE BodyComponentStore &GetBodyComponentStore() {
                return _bodyComponentStore;
            }

            [[nodiscard]] VE_FORCE_INLINE RigidBodyComponentStore &GetRigidBodyComponentStore() {
                return _rigidBodyComponentStore;
            }

            [[nodiscard]] VE_FORCE_INLINE ColliderComponentStore &GetColliderComponentStore() {
                return _colliderComponentStore;
            }

            [[nodiscard]] VE_FORCE_INLINE TransformComponentStore &GetTransformComponentStore() {
                return _transformComponentStore;
            }

            void Update();

        private:
            PhysicsWorldSettings _settings;
            EntityManager _entityManager;
            BodyComponentStore _bodyComponentStore;
            RigidBodyComponentStore _rigidBodyComponentStore;
            ColliderComponentStore _colliderComponentStore;
            TransformComponentStore _transformComponentStore;
    };

} // namespace Vulkyrie
