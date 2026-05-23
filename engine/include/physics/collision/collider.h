#pragma once

#include "core/entity.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/materials/material.h"

namespace Vulkyrie {
    class Body;

    /** @brief The Collider class represents a collision component attached to a Body in the physics simulation. It manages the collision properties and
     * interactions of the entity within the physics simulation, and it is attached to a Body for movement and transformation. The Collider class provides
     * methods to access and modify the collision shape, material properties, and collision filtering settings of the collider, as well as methods to check for
     * collisions with other colliders and to retrieve the collider's axis-aligned bounding box (AABB) in world space. It serves as the main interface for
     * interacting with colliders in the physics simulation, allowing users to define the collision geometry and behavior of entities in the physics world.
     */
    class Collider final {
    public:
        /** @brief Constructs a Collider instance associated with the given entity and body. The entity must have a ColliderComponent associated with it in
         * the physics world for the collider to function correctly. The collider will manage the collision properties and interactions of the entity within
         * the physics simulation, and it will be attached to the specified body for movement and transformation. The collider will also manage its own
         * collision shape, material properties, and collision filtering settings. It serves as the main interface for interacting with colliders in the
         * physics simulation.
         * @param entity The entity with which this collider is associated. Must have a ColliderComponent in the physics world.
         * @param body The body to which this collider is attached. The collider will use the body's transform for movement and transformation. */
        Collider(Entity entity, Body &body);

        // Delete the copy constructor and the copy assignment operator to prevent copying of Collider instances.
        Collider(const Collider &) = delete;
        Collider &operator=(const Collider &) = delete;

        // Delete the move constructor and the move assignment operator to prevent moving of Collider instances.
        Collider(Collider &&) = delete;
        Collider &operator=(Collider &&) = delete;

        /** @brief Default destructor for Collider. */
        ~Collider() = default;

        /** @brief Retrieves the Entity associated with this Collider.
         * @returns The Entity associated with this Collider. */
        [[nodiscard]] VE_INLINE Entity GetEntity() const {
            return _entity;
        }

        /** @brief Retrieves a reference to the Body to which this Collider is attached. The body represents the parent physical entity that this collider
         * belongs to in the physics simulation. The collider uses the body's transform for movement and transformation, and it participates in collision
         * detection and response as part of the body.
         * @returns A reference to the Body to which this Collider is attached. */
        [[nodiscard]] VE_INLINE const Body &GetBody() const {
            return _body;
        }

        /** @brief Checks if this collider collides with the given axis-aligned bounding box (AABB). This method computes the AABB that encompasses the
         * collision shape of this collider in world space and tests it against the provided AABB for overlap. It returns true if there is any collision
         * (overlap) between this collider's AABB and the given AABB, and false otherwise.
         * @param aabb The axis-aligned bounding box to test for collision against this collider.
         * @returns True if this collider collides with the given AABB, false otherwise. */
        [[nodiscard]] VE_INLINE bool CollidesWith(const AABB &aabb) const {
            return aabb.CollidesWith(GetWorldSpaceAABB());
        }

        /** @brief Sets whether the shape of this collider has changed. This is important for ensuring that any changes to the collision shape are properly
         * communicated to the physics simulation, allowing it to update its internal state and recalculate collision detection and response as needed. When
         * the shape of a collider changes (e.g., resizing, changing properties), this method should be called with `hasChanged` set to true to notify the
         * physics simulation of the change. If `hasChanged` is set to false, it indicates that the shape has not changed and no updates are necessary.
         * @param hasChanged A boolean value indicating whether the shape of this collider has changed. Set to true if the shape has changed, false
         * otherwise. */
        void SetHasColliderShapeChanged(bool hasChanged) const;

        /** @brief Retrieves a reference to the CollisionShape associated with this collider. The CollisionShape defines the geometric shape used for
         * collision detection and response in the physics simulation.
         * @returns A reference to the CollisionShape associated with this collider. */
        [[nodiscard]] CollisionShape &GetCollisionShape();

        /** @brief Retrieves a const reference to the CollisionShape associated with this collider. The CollisionShape defines the geometric shape used for
         * collision detection and response in the physics simulation.
         * @returns A const reference to the CollisionShape associated with this collider. */
        [[nodiscard]] const CollisionShape &GetCollisionShape() const;

        /** @brief Retrieves a reference to the local-to-body transform associated with this collider. The local-to-body transform represents the
         * transformation that converts coordinates from the collider's local space to the body's local space, which is essential for accurate collision
         * detection and response in the physics simulation.
         * @returns A reference to the local-to-body transform associated with this collider. This transform can be used to convert coordinates from the
         * collider's local space to the body's local space for accurate collision detection and response. */
        [[nodiscard]] const TransformComponent &GetLocalToBodyTransform() const;

        /** @brief Sets the local-to-body transform for this collider. The local-to-body transform represents the transformation that converts coordinates
         * from the collider's local space to the body's local space, which is essential for accurate collision detection and response in the physics
         * simulation. Changing this transform will affect how the collider is positioned and oriented relative to the body, which in turn affects collision
         * detection and response.
         * @param transform The new local-to-body transform to be set for this collider. This transform can be used to convert coordinates from the
         * collider's local space to the body's local space for accurate collision detection and response. */
        void SetLocalToBodyTransform(const TransformComponent &transform);

        /** @brief Retrieves a reference to the local-to-world transform associated with this collider. The local-to-world transform represents the
         * transformation that converts coordinates from the collider's local space to world space, which is essential for accurate collision detection and
         * response in the physics simulation.
         * @returns A const reference to the local-to-world transform associated with this collider. This transform can be used to convert coordinates from
         * the collider's local space to world space for accurate collision detection and response. */
        [[nodiscard]] const TransformComponent &GetLocalToWorldTransform() const;

        /** @brief Retrieves the axis-aligned bounding box (AABB) that encompasses the collision shape of this collider in world space. The AABB is a
         * simple geometric representation that can be used for broad-phase collision detection and spatial queries in the physics simulation. This method
         * computes the AABB based on the collider's collision shape and its local-to-world transform, providing an efficient way to test for potential
         * collisions with other colliders or spatial structures in the simulation.
         * @returns The axis-aligned bounding box (AABB) that encompasses the collision shape of this collider in world space. */
        [[nodiscard]] const AABB GetWorldSpaceAABB() const;

        /** @brief Checks if the specified point in world space is contained within the collision shape of this collider. This method transforms the point
         * from world space to the collider's local space using the inverse of the local-to-world transform, and then checks if the point lies within the
         * collision shape. It returns true if the point is contained within the collider's shape, and false otherwise.
         * @param worldSpacePoint The point in world space to be checked for containment within this collider's collision shape.
         * @returns True if the specified point is contained within the collision shape of this collider, false otherwise. */
        [[nodiscard]] bool ContainsPoint(const glm::vec3 &worldSpacePoint) const;

        /** @brief Retrieves a reference to the Material associated with this collider. The Material defines the physical properties of the collider, such
         * as friction and restitution, which affect how it interacts with other colliders in the physics simulation.
         * @returns A reference to the Material associated with this collider. */
        [[nodiscard]] Material &GetMaterial() const;

        /** @brief Sets the material properties for this collider. The Material defines the physical properties of the collider, such as friction and
         * restitution, which affect how it interacts with other colliders in the physics simulation. Changing the material properties will affect the
         * collision response of this collider when it interacts with other colliders in the simulation.
         * @param material The Material object containing the new material properties to be applied to this collider. */
        void SetMaterial(const Material &material);

        /** @brief Retrieves the collision category bits for this collider. Collision category bits are used for collision filtering in the physics
         * simulation, allowing you to specify which categories of colliders this collider belongs to.
         * @returns The collision category bitmask for this collider. */
        [[nodiscard]] u16 GetCollisionCategoryBits() const;

        /** @brief Sets the collision category bits for this collider. Collision category bits are used for collision filtering in the physics simulation,
         * allowing you to specify which categories of colliders this collider belongs to. Changing the collision category bits will affect how this
         * collider interacts with other colliders in the simulation based on their collides-with mask bits.
         * @param collisionCategoryBits The new collision category bitmask to assign to this collider. */
        void SetCollisionCategoryBits(u16 collisionCategoryBits);

        /** @brief Retrieves the collides-with mask bits for this collider. The collides-with mask bits define which collision categories this collider will
         * respond to during collision detection in the physics simulation. A collision is only processed when (this->CollisionCategoryBits &
         * other->CollidesWithMaskBits) != 0 and vice versa.
         * @returns The collides-with bitmask for this collider. */
        [[nodiscard]] u16 GetCollidesWithMaskBits() const;

        /** @brief Sets the collides-with mask bits for this collider. The collides-with mask bits define which collision categories this collider will
         * respond to during collision detection in the physics simulation. A collision is only processed when (this->CollisionCategoryBits &
         * other->CollidesWithMaskBits) != 0 and vice versa. Changing the collides-with mask bits will affect how this collider interacts with other
         * colliders in the simulation based on their collision category bits.
         * @param maskBits The new collides-with bitmask to assign to this collider. */
        void SetCollidesWithMaskBits(u16 maskBits);

        /** @brief Retrieves the broad-phase ID for this collider. The broad-phase ID is an identifier used in the broad-phase collision detection system to
         * efficiently manage and query colliders in the physics simulation.
         * @returns The broad-phase ID for this collider. */
        [[nodiscard]] i32 GetBroadPhaseID() const;

        /** @brief Reports if this collider is a trigger. A trigger is a special type of collider that does not participate in physics simulation or
         * collision response but can be used to detect overlaps and trigger events.
         * @returns True if this collider is a trigger, false otherwise. */
        [[nodiscard]] bool IsTrigger() const;

        /** @brief Sets whether this collider is a trigger. A trigger is a special type of collider that does not participate in physics simulation or
         * collision response but can be used to detect overlaps and trigger events. Setting this collider as a trigger will cause it to ignore physical
         * collisions and instead only report overlap events when other colliders intersect with it in the physics simulation.
         * @param isTrigger True to set this collider as a trigger, false otherwise. */
        void SetTrigger(bool isTrigger);

        /** @brief Checks if this collider is a simulation collider. A simulation collider is a regular collider that participates in physics simulation and
         * collision response. If this collider is not a simulation collider, it may be used for other purposes such as queries or triggers, but it will not
         * affect the physics simulation or generate collision responses.
         * @returns True if this collider is a simulation collider, false otherwise. */
        [[nodiscard]] bool IsSimulationCollider() const;

        /** @brief Sets whether this collider is a simulation collider. A simulation collider is a regular collider that participates in physics simulation
         * and collision response. Setting this collider as a simulation collider will cause it to interact with other colliders in the physics simulation
         * and generate collision responses based on its material properties and collision geometry.
         * @param isSimulationCollider True to set this collider as a simulation collider, false otherwise. */
        void SetSimulationCollider(bool isSimulationCollider);

        /** @brief Checks if this collider is a query collider. A query collider is used for collision queries and does not participate in physics
         * simulation. If this collider is a query collider, it will be included in scene queries such as raycasts, shape casts, and overlap tests, but it
         * will not generate collision responses or affect the physics simulation.
         * @returns True if this collider is a query collider, false otherwise. */
        [[nodiscard]] bool IsQueryCollider() const;

        /** @brief Sets whether this collider is a query collider. A query collider is used for collision queries and does not participate in physics
         * simulation. Setting this collider as a query collider will cause it to be included in scene queries such as raycasts, shape casts, and overlap
         * tests, but it will not generate collision responses or affect the physics simulation.
         * @param isQueryCollider True to set this collider as a query collider, false otherwise. */
        void SetQueryCollider(bool isQueryCollider);

    private:
        /** @brief The Entity associated with this Collider. The entity must have a ColliderComponent associated with it in the physics world for the
         * collider to function correctly. This entity serves as the identifier for this collider within the physics simulation and allows it to be managed
         * and accessed through the physics world's component stores. The collider will use this entity to access its properties, collision shape, material,
         * and other relevant data stored in the ColliderComponentStore of the physics world. */
        Entity _entity;

        /** @brief Reference to the Body to which this Collider is attached. The body represents the parent physical entity that this collider belongs to in
         * the physics simulation. The collider uses the body's transform for movement and transformation, and it participates in collision detection and
         * response as part of the body. This reference allows the collider to access properties of the body and to interact with it during collision
         * detection and response calculations. */
        Body &_body;
    };

} // namespace Vulkyrie
