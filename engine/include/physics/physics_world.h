#pragma once

#include "core/entity_manager.h"
#include "physics/physics_world_settings.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"
#include "physics/systems/collision_system.h"

namespace Vulkyrie {

    class PhysicsWorld {
    public:
        explicit PhysicsWorld(const PhysicsWorldSettings &settings);

        /** @brief Provides access to the settings of the physics world, which include parameters such as gravity, time step, and other global
         * configurations that affect the behavior of the physics simulation. This method allows other parts of the physics system to query the current
         * settings and adjust their behavior accordingly during simulation updates.
         * @returns A reference to the PhysicsWorldSettings that contains the configuration parameters for the physics world. */
        [[nodiscard]] VE_FORCE_INLINE const PhysicsWorldSettings &GetSettings() const {
            return _settings;
        }

        /** @brief Provides access to the EntityManager, which manages the creation and destruction of entities in the physics world. The EntityManager is
         * responsible for generating unique entity identifiers, tracking entity lifetimes, and providing an interface for creating and destroying entities.
         * This method allows other parts of the physics system to create new entities or query existing ones as needed during simulation updates.
         * @returns A reference to the EntityManager that manages entities in the physics world. */
        [[nodiscard]] VE_FORCE_INLINE EntityManager &GetEntityManager() {
            return _entityManager;
        }

        /** @brief Provides access to the BodyComponentStore, which manages the BodyComponents associated with entities in the physics world. The
         * BodyComponentStore is responsible for storing and managing the physical properties and behavior of bodies in the physics simulation, including
         * their colliders and active state. This method allows other parts of the physics system to retrieve and modify the body components of entities as
         * needed during simulation updates. The BodyComponentStore also maintains the dense active-zone invariant for efficient iteration over active
         * bodies in the simulation.
         * @returns A reference to the BodyComponentStore that manages the BodyComponents of entities in the physics world. */
        [[nodiscard]] VE_FORCE_INLINE BodyComponentStore &GetBodyComponentStore() {
            return _bodyComponentStore;
        }

        /** @brief Provides access to the RigidBodyComponentStore, which manages the RigidBodyComponents associated with entities in the physics world. The
         * RigidBodyComponentStore is responsible for storing and managing the physical properties and behavior of rigid bodies in the physics simulation,
         * including their mass, inertia, velocity, and sleep state. This method allows other parts of the physics system to retrieve and modify the rigid
         * body components of entities as needed during simulation updates. The RigidBodyComponentStore also maintains the dense active-zone invariant for
         * efficient iteration over active rigid bodies in the simulation.
         * @returns A reference to the RigidBodyComponentStore that manages the RigidBodyComponents of entities in the physics world. */
        [[nodiscard]] VE_FORCE_INLINE RigidBodyComponentStore &GetRigidBodyComponentStore() {
            return _rigidBodyComponentStore;
        }

        /** @brief Provides access to the ColliderComponentStore, which manages the ColliderComponents associated with entities in the physics world. The
         * ColliderComponentStore is responsible for storing and managing the collision properties and behavior of entities in the physics simulation,
         * including their colliders, collision shapes, and material properties. This method allows other parts of the physics system to retrieve and modify
         * the colliders of entities as needed during simulation updates. The ColliderComponentStore also maintains the dense active-zone invariant for
         * efficient iteration over active colliders in the simulation.
         * @returns A reference to the ColliderComponentStore that manages the ColliderComponents of entities in the physics world. */
        [[nodiscard]] VE_FORCE_INLINE ColliderComponentStore &GetColliderComponentStore() {
            return _colliderComponentStore;
        }

        /** @brief Provides access to the TransformComponentStore, which manages the TransformComponents associated with entities in the physics world. The
         * TransformComponentStore is responsible for storing and managing the position and orientation of entities in 3D space, which is essential for
         * accurate collision detection and response in the physics simulation. This method allows other parts of the physics system to retrieve and modify
         * the transforms of entities as needed during simulation updates.
         * @returns A reference to the TransformComponentStore that manages the TransformComponents of entities in the physics world. */
        [[nodiscard]] VE_FORCE_INLINE TransformComponentStore &GetTransformComponentStore() {
            return _transformComponentStore;
        }

        [[nodiscard]] VE_FORCE_INLINE CollisionSystem &GetCollisionSystem() {
            return _collisionSystem;
        }

        void Update();

        void SetActiveStatusForBody(Entity entity, bool disabled);

    private:
        PhysicsWorldSettings _settings;
        EntityManager _entityManager;
        BodyComponentStore _bodyComponentStore;
        RigidBodyComponentStore _rigidBodyComponentStore;
        ColliderComponentStore _colliderComponentStore;
        TransformComponentStore _transformComponentStore;

        CollisionSystem _collisionSystem;
    };

} // namespace Vulkyrie
