#pragma once

#include "core/entity.h"
#include "physics/physics_world.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/collider.h"

namespace Vulkyrie {

    /** @brief The Body class represents a physical entity in the physics world. It is associated with an Entity and manages its colliders and transform. A Body
     * can have multiple colliders attached to it, which define its collision geometry. The Body class provides methods to manipulate the body's transform, add
     * and remove colliders, and query collision information. It serves as the main interface for interacting with physical entities in the physics simulation.
     */
    class Body {
        public:
            /** @brief Constructs a Body instance associated with the given entity and physics world. The entity must have a TransformComponent associated with
             * it in the physics world for the body to function correctly. The body will manage the colliders and transform of the entity within the physics
             * simulation.
             * @param entity The entity to which this body is associated. Must have a TransformComponent in the physics world.
             * @param physicsWorld The physics world that this body belongs to. The body will interact with this world for collision detection and response. */
            Body(Entity entity, PhysicsWorld &physicsWorld);

            // Delete the copy constructor and the copy assignment operator to prevent copying of Body instances,
            // as they are meant to be unique entities within the physics world and should not be duplicated.
            Body(const Body &) = delete;
            Body &operator=(const Body &) = delete;

            // Delete the move constructor and the move assignment operator to prevent moving of Body instances,
            // as they are tightly coupled with their entity and physics world, and moving them could lead
            Body(Body &&) = delete;
            Body &operator=(Body &&) = delete;

            virtual ~Body() = default;

            [[nodiscard]] VE_FORCE_INLINE Entity GetEntity() const {
                return _entity;
            }

            [[nodiscard]] VE_FORCE_INLINE PhysicsWorld &GetPhysicsWorld() const {
                return _physicsWorld;
            }

            bool IsActive() const;
            // virtual void SetIsActive(bool active);

            const TransformComponent &GetTransform() const;
            virtual void SetTransform(const TransformComponent &transform);

            // virtual void AddCollider(CollisionShape *shape, const TransformComponent &transform);
            // virtual void RemoveCollider(Collider *collider);

            Collider &GetCollider(size_t colliderIndex);
            const Collider &GetCollider(size_t colliderIndex) const;

            /** @brief Retrieves the number of colliders currently attached to this body.
             * @return The number of colliders currently attached to this body. */
            size_t GetColliderCount() const;

            /** @brief Checks if the specified point in world space is contained within any of the colliders attached to this body. This method iterates through
             * all colliders associated with the body and checks if the point lies within any of them, returning true if it does and false otherwise.
             * @param point The point in world space to be checked for containment within the body's colliders.
             * @return True if the specified point is contained within any of the colliders attached to this body, false otherwise. */
            bool ContainsPoint(const glm::vec3 &point) const;

            // [[nodiscard]] VE_FORCE_INLINE bool CollidesWith(const AABB &aabb) const {
            //     return aabb.CollidesWith(GetAABB());
            // }
            // AABB GetAABB() const;
            //
            // glm::vec3 GetWorldPoint(const glm::vec3 &localPoint) const;
            // glm::vec3 GetWorldVector(const glm::vec3 &localVector) const;
            // glm::vec3 GetLocalPoint(const glm::vec3 &worldPoint) const;
            // glm::vec3 GetLocalVector(const glm::vec3 &worldVector) const;

            void UpdateHasSimulationCollidersFlag();

        protected:
            Entity _entity;
            PhysicsWorld &_physicsWorld;
    };

} // namespace Vulkyrie
