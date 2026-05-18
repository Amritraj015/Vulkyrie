#pragma once

#include "core/entity.h"
#include "physics/physics_world.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/shapes/aabb.h"

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
        // as they are tightly coupled with their entity and physics world, and moving them could lead to dangling references and other issues.
        Body(Body &&) = delete;
        Body &operator=(Body &&) = delete;

        /** @brief Default destructor for Body. */
        virtual ~Body() = default;

        /** @brief Retrieves the Entity associated with this Body.
         * @returns The Entity associated with this Body. */
        [[nodiscard]] VE_FORCE_INLINE Entity GetEntity() const {
            return _entity;
        }

        /** @brief Retrieves a reference to the PhysicsWorld that this body belongs to.
         * @returns A reference to the PhysicsWorld that this body belongs to. */
        [[nodiscard]] VE_FORCE_INLINE PhysicsWorld &GetPhysicsWorld() const {
            return _physicsWorld;
        }

        /** @brief Checks whether this body is active in the physics simulation. An active body participates in collision detection and response, while an
         * inactive body does not. The active state of a body can be used to temporarily disable its physical interactions without removing it from the
         * physics world.
         * @returns True if this body is active in the simulation, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsActive() const {
            return _physicsWorld.GetBodyComponentStore().IsBodyActive(_entity);
        }

        /** @brief Sets whether this body is active in the physics simulation. An active body participates in collision detection and response, while an
         * inactive body does not. Setting a body to inactive can be used to temporarily disable its physical interactions without removing it from the
         * physics world.
         * @param active True to set this body as active in the simulation, false to set it as inactive. */
        virtual void SetIsActive(bool active);

        /** @brief Retrieves the current transform of this body, which includes its position and rotation in world space. The transform defines the body's
         * location and orientation in the physics world, and it affects how the body's colliders are positioned and how it interacts with other bodies. The
         * returned transform is a reference to the TransformComponent associated with this body's entity in the physics world.
         * @returns A const reference to the TransformComponent representing this body's transform in world space. */
        [[nodiscard]] VE_FORCE_INLINE const TransformComponent &GetTransform() const {
            return _physicsWorld.GetTransformComponentStore().GetTransform(_entity);
        }

        /** @brief Sets the transform of this body. The transform includes the position and rotation of the body in world space. Changing the body's
         * transform will affect the positions of its colliders and how it interacts with other bodies in the physics simulation. The provided transform
         * will be applied to the body, and any attached colliders will be updated accordingly to maintain their relative positions and orientations to the
         * body.
         * @param transform The new transform to be applied to this body, including its position and rotation in world space. */
        virtual void SetTransform(const TransformComponent &transform);

        /** @brief Retrieves a reference to the Collider at the specified index in this body's collider list. The index must be less than the number of
         * colliders currently attached to this body, as returned by GetColliderCount(). This method allows access to the colliders for manipulation or
         * querying of their properties and collision geometry.
         * @param colliderIndex The index of the collider to retrieve, which must be in the range [0, GetColliderCount()).
         * @returns A reference to the Collider at the specified index in this body's collider list. */
        [[nodiscard]] Collider &GetCollider(size_t colliderIndex);

        /** @brief Retrieves a const reference to the Collider at the specified index in this body's collider list. The index must be less than the number
         * of colliders currently attached to this body, as returned by GetColliderCount(). This method allows read-only access to the colliders for
         * querying their properties and collision geometry without modifying them.
         * @param colliderIndex The index of the collider to retrieve, which must be in the range [0, GetColliderCount()).
         * @returns A const reference to the Collider at the specified index in this body's collider list. */
        [[nodiscard]] const Collider &GetCollider(size_t colliderIndex) const;

        /** @brief Retrieves the number of colliders currently attached to this body.
         * @returns The number of colliders currently attached to this body. */
        [[nodiscard]] VE_FORCE_INLINE size_t GetColliderCount() const {
            return _physicsWorld.GetBodyComponentStore().GetColliders(_entity).size();
        }

        /** @brief Adds a new collider to this body using the specified collision shape and local transform. This method creates a new Collider instance
         * based on the provided collision shape and local transform, attaches it to this body, and adds it to the physics simulation. The new collider will
         * be included in the body's collision geometry and will affect how the body interacts with other bodies in the simulation. The method returns a
         * reference to the newly created Collider, which can be used for further manipulation or querying of its properties.
         * @param collisionShape The CollisionShape that defines the geometric representation of the new collider to be added to this body.
         * @param transform The local transform that specifies the position and rotation of the new collider relative to this body's origin.
         * @returns A reference to the newly created Collider that has been added to this body. This collider will be part of the body's collision geometry
         * and can be used for further manipulation or querying of its properties. */
        [[nodiscard]] virtual Collider &AddCollider(CollisionShape &collisionShape, const TransformComponent &transform);

        /** @brief Removes the specified collider from this body. This method detaches the collider from the body and removes it from the physics
         * simulation, which will affect the body's collision geometry and interactions with other bodies. After calling this method, the specified collider
         * will no longer be associated with this body, and the body's AABB will be updated accordingly to reflect the change in its colliders. The caller
         * is responsible for ensuring that the collider being removed is currently attached to this body.
         * @param collider The collider to be removed from this body. This collider must currently be attached to the body for the removal to be valid. */
        virtual void RemoveCollider(Collider &collider);

        /** @brief Removes all colliders currently attached to this body. This method iterates through all colliders associated with the body and removes
         * them from the physics simulation, effectively clearing the body's collision geometry. After calling this method, the body will have no colliders
         * attached to it, and its AABB will be empty until new colliders are added. */
        void RemoveAllColliders();

        /** @brief Checks if the specified point in world space is contained within any of the colliders attached to this body. This method iterates through
         * all colliders associated with the body and checks if the point lies within any of them, returning true if it does and false otherwise.
         * @param point The point in world space to be checked for containment within the body's colliders.
         * @returns True if the specified point is contained within any of the colliders attached to this body, false otherwise. */
        [[nodiscard]] bool ContainsPoint(const glm::vec3 &point) const;

        /** @brief Checks if this body collides with the given axis-aligned bounding box (AABB). This method computes the AABB that encompasses all
         * colliders attached to this body and tests it against the provided AABB for overlap. It returns true if there is any collision (overlap) between
         * the body's AABB and the given AABB, and false otherwise.
         * @param aabb The axis-aligned bounding box to test for collision against this body.
         * @returns True if this body collides with the given AABB, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool CollidesWith(const AABB &aabb) const {
            return aabb.CollidesWith(GetAABB());
        }

        /** @brief Computes and retrieves the axis-aligned bounding box (AABB) that encompasses all colliders attached to this body. This method iterates
         * through all colliders associated with the body, computes their world-space AABBs, and combines them to produce a single AABB that fully contains
         * the body's collision geometry.
         * @returns The AABB that encompasses all colliders attached to this body. */
        [[nodiscard]] AABB GetAABB() const;

        /** @brief Transforms a point from the body's local space to world space using the body's current transform. This method applies the body's position
         * and rotation to the given local point to compute its corresponding position in world space.
         * @param localPoint The point in the body's local space to be transformed to world space.
         * @returns The corresponding point in world space after applying the body's transform. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetWorldPoint(const glm::vec3 &localPoint) const {
            return GetTransform() * localPoint;
        }

        /** @brief Transforms a vector from the body's local space to world space using the body's current rotation. This method applies the body's rotation
         * to the given local vector to compute its corresponding direction in world space. Note that this transformation does not apply the body's
         * position, as vectors represent directions rather than points.
         * @param localVector The vector in the body's local space to be transformed to world space.
         * @returns The corresponding vector in world space after applying the body's rotation. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetWorldVector(const glm::vec3 &localVector) const {
            return GetTransform().Rotation * localVector;
        }

        /** @brief Transforms a point from world space to the body's local space using the inverse of the body's current transform. This method applies the
         * inverse of the body's position and rotation to the given world point to compute its corresponding position in the body's local space.
         * @param worldPoint The point in world space to be transformed to the body's local space.
         * @returns The corresponding point in the body's local space after applying the inverse of the body's transform. */
        [[nodiscard]] glm::vec3 GetLocalPoint(const glm::vec3 &worldPoint) const;

        /** @brief Transforms a vector from world space to the body's local space using the inverse of the body's current rotation. This method applies the
         * inverse of the body's rotation to the given world vector to compute its corresponding direction in the body's local space. Note that this
         * transformation does not apply the body's position, as vectors represent directions rather than points.
         * @param worldVector The vector in world space to be transformed to the body's local space.
         * @returns The corresponding vector in the body's local space after applying the inverse of the body's rotation. */
        [[nodiscard]] glm::vec3 GetLocalVector(const glm::vec3 &worldVector) const;

        /** @brief Scans all colliders attached to this body and sets the has-simulation-colliders flag to true on the body's component if at least one
         * simulation collider is found. This method should be called whenever colliders are added to or removed from the body to keep the flag consistent
         * with the current collider state. */
        void UpdateHasSimulationCollidersFlag();

    protected:
        /** @brief The Entity associated with this Body. This entity must have a TransformComponent in the physics world for the body to function correctly.
         * The body manages the colliders and transform of this entity within the physics simulation. */
        Entity _entity;

        /** @brief Reference to the PhysicsWorld that this body belongs to. The body interacts with this world for collision detection and response, and it
         * uses the physics world to access component stores and manage its colliders and transform. */
        PhysicsWorld &_physicsWorld;

        /** @brief Requests a broad-phase collision check for this body. This method should be called whenever the body's colliders are added, removed, or
         * modified in a way that affects their AABBs, to ensure that the broad-phase collision detection system is updated with the latest information
         * about the body's collision geometry. The physics world will handle the actual scheduling and execution of the broad-phase collision check based
         * on this request. */
        void requestBroadPhaseCollisionCheck();

        void updateBroadPhaseState();
    };

} // namespace Vulkyrie
