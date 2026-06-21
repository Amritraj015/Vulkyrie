#pragma once

#include "vlkypch.h"
#include "physics/body/body.h"

namespace Vulkyrie {

    /** @brief The RigidBody class represents a rigid body in the physics world. It is a specific type of Body that has mass, velocity, and inertia properties,
     * and it responds to forces and torques according to the laws of rigid body dynamics. A RigidBody can be static (immovable), dynamic (movable), or
     * kinematic (movable but not affected by forces). The RigidBody class provides methods to manipulate the body's physical properties, such as mass,
     * velocity, damping, and inertia tensor, as well as methods to apply forces and torques to the body. It serves as the main interface for interacting with
     * rigid bodies in the physics simulation.
     */
    class RigidBody final : public Body {
    public:
        /** @brief Constructs a RigidBody instance associated with the given entity and physics world. The entity must have a TransformComponent associated
         * with it in the physics world for the rigid body to function correctly. The rigid body will manage the physical properties and interactions of the
         * entity within the physics simulation.
         * @param entity The entity to which this rigid body is associated. Must have a TransformComponent in the physics world.
         * @param physicsWorld The physics world that this rigid body belongs to. The rigid body will interact with this world for collision detection and
         * response. */
        RigidBody(Entity entity, PhysicsWorld &physicsWorld);

        VE_DELETE_MOVE_AND_COPY(RigidBody);

        /** @brief Default destructor for Body. */
        ~RigidBody() override = default;

        /** @brief Retrieves the mass of this rigid body.
         * @returns The mass of this rigid body. */
        [[nodiscard]] VE_INLINE f32 GetMass() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetMass(_entity);
        }

        /** @brief Retrieves the linear velocity of this rigid body.
         * @returns The linear velocity of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetLinearVelocity() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetLinearVelocity(_entity);
        }

        /** @brief Retrieves the angular velocity of this rigid body.
         * @returns The angular velocity of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetAngularVelocity() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetAngularVelocity(_entity);
        }

        /** @brief Retrieves the local inertia tensor of this rigid body, which represents the body's resistance to angular acceleration around its local
         * axes. The local inertia tensor is typically defined in the body's local space and is used to compute the body's response to torques and angular
         * forces.
         * @returns The local inertia tensor of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetLocalInertiaTensor() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetLocalInertiaTensor(_entity);
        }

        /** @brief Retrieves the local center of mass of this rigid body, which represents the point in the body's local space where its mass is
         * concentrated. The local center of mass is used to compute the body's response to forces and torques, and it affects how the body moves and
         * rotates in the physics simulation.
         * @returns The local center of mass of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetLocalCenterOfMass() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetLocalCenterOfMass(_entity);
        }

        /** @brief Retrieves the linear damping factor of this rigid body, which represents the rate at which the body's linear velocity decreases over time
         * due to drag forces. A higher linear damping value will cause the body to slow down more quickly, while a lower value will allow it to maintain
         * its velocity for longer. Linear damping is typically used to simulate effects such as air resistance or friction that act against the motion of
         * the body.
         * @returns The linear damping factor of this rigid body. */
        [[nodiscard]] VE_INLINE f32 GetLinearDamping() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetLinearDamping(_entity);
        }

        /** @brief Sets the linear damping factor of this rigid body, which represents the rate at which the body's linear velocity decreases over time due
         * to drag forces. A higher linear damping value will cause the body to slow down more quickly, while a lower value will allow it to maintain its
         * velocity for longer. Linear damping is typically used to simulate effects such as air resistance or friction that act against the motion of the
         * body.
         * @param linearDamping The linear damping factor to set for this rigid body. Must be greater than or equal to zero. */
        VE_INLINE void SetLinearDamping(f32 linearDamping) {
            VASSERT(linearDamping >= 0.0f, "Linear damping must be greater than or equal to zero.");

            _physicsWorld.GetRigidBodyComponentStore().SetLinearDamping(_entity, linearDamping);
        }

        /** @brief Retrieves the angular damping factor of this rigid body, which represents the rate at which the body's angular velocity decreases over
         * time due to drag forces. A higher angular damping value will cause the body to slow down its rotation more quickly, while a lower value will
         * allow it to maintain its angular velocity for longer. Angular damping is typically used to simulate effects such as air resistance or friction
         * that act against the rotation of the body.
         * @returns The angular damping factor of this rigid body. */
        [[nodiscard]] VE_INLINE f32 GetAngularDamping() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetAngularDamping(_entity);
        }

        /** @brief Sets the angular damping factor of this rigid body, which represents the rate at which the body's angular velocity decreases over time
         * due to drag forces. A higher angular damping value will cause the body to slow down its rotation more quickly, while a lower value will allow it
         * to maintain its angular velocity for longer. Angular damping is typically used to simulate effects such as air resistance or friction that act
         * against the rotation of the body.
         * @param angularDamping The angular damping factor to set for this rigid body. Must be greater than or equal to zero. */
        VE_INLINE void SetAngularDamping(f32 angularDamping) {
            VASSERT(angularDamping >= 0.0f, "Angular damping must be greater than or equal to zero.");

            _physicsWorld.GetRigidBodyComponentStore().SetAngularDamping(_entity, angularDamping);
        }

        /** @brief Retrieves the linear lock axis factor of this rigid body, which is a vector that indicates which axes of linear motion are locked or
         * constrained. Each component of the vector (x, y, z) corresponds to a specific axis in world space, and a value of 1.0f for a component indicates
         * that linear motion along that axis is fully locked (i.e., the body cannot move along that axis), while a value of 0.0f indicates that linear
         * motion along that axis is free (i.e., the body can move along that axis without constraint). Intermediate values between 0.0f and 1.0f can be
         * used to represent partial locking or damping along specific axes.
         * @returns The linear lock axis factor of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetLinearLockAxisFactor() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetLinearLockAxisFactor(_entity);
        }

        /** @brief Sets the linear lock axis factor of this rigid body, which is a vector that indicates which axes of linear motion are locked or
         * constrained. Each component of the vector (x, y, z) corresponds to a specific axis in world space, and a value of 1.0f for a component indicates
         * that linear motion along that axis is fully locked (i.e., the body cannot move along that axis), while a value of 0.0f indicates that linear
         * motion along that axis is free (i.e., the body can move along that axis without constraint). Intermediate values between 0.0f and 1.0f can be
         * used to represent partial locking or damping along specific axes.
         * @param lockAxisFactor The linear lock axis factor to set for this rigid body. Each component should be in the range [0.0f, 1.0f]. */
        VE_INLINE void SetLinearLockAxisFactor(const glm::vec3 &lockAxisFactor) {
            _physicsWorld.GetRigidBodyComponentStore().SetLinearLockAxisFactor(_entity, lockAxisFactor);
        }

        /** @brief Retrieves the angular lock axis factor of this rigid body, which is a vector that indicates which axes of angular motion (rotation) are
         * locked or constrained. Each component of the vector (x, y, z) corresponds to a specific axis in world space, and a value of 1.0f for a component
         * indicates that angular motion around that axis is fully locked (i.e., the body cannot rotate around that axis), while a value of 0.0f indicates
         * that angular motion around that axis is free (i.e., the body can rotate around that axis without constraint). Intermediate values between 0.0f
         * and 1.0f can be used to represent partial locking or damping around specific axes.
         * @returns The angular lock axis factor of this rigid body. */
        [[nodiscard]] VE_INLINE glm::vec3 GetAngularLockAxisFactor() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetAngularLockAxisFactor(_entity);
        }

        /** @brief Sets the angular lock axis factor of this rigid body, which is a vector that indicates which axes of angular motion (rotation) are locked
         * or constrained. Each component of the vector (x, y, z) corresponds to a specific axis in world space, and a value of 1.0f for a component
         * indicates that angular motion around that axis is fully locked (i.e., the body cannot rotate around that axis), while a value of 0.0f indicates
         * that angular motion around that axis is free (i.e., the body can rotate around that axis without constraint). Intermediate values between 0.0f
         * and 1.0f can be used to represent partial locking or damping around specific axes.
         * @param lockAxisFactor The angular lock axis factor to set for this rigid body. Each component should be in the range [0.0f, 1.0f]. */
        VE_INLINE void SetAngularLockAxisFactor(const glm::vec3 &lockAxisFactor) {
            _physicsWorld.GetRigidBodyComponentStore().SetAngularLockAxisFactor(_entity, lockAxisFactor);
        }

        /** @brief Retrieves the body type of this rigid body, which indicates whether the body is static, dynamic, or kinematic. The body type determines
         * how the body interacts with other bodies in the physics simulation and how it responds to forces and collisions. Static bodies are immovable and
         * do not respond to forces, dynamic bodies are fully simulated and respond to forces and collisions, and kinematic bodies are moved by setting
         * their velocity or transform directly and do not respond to forces but can affect dynamic bodies through collisions.
         * @returns The body type of this rigid body. */
        [[nodiscard]] VE_INLINE BodyType GetBodyType() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetBodyType(_entity);
        }

        /** @brief Checks whether gravity is enabled for this rigid body. If gravity is enabled, the body will be affected by the global gravity force in
         * the physics simulation, which will cause it to accelerate downwards over time. If gravity is disabled, the body will not be affected by gravity
         * and will only respond to other forces and collisions.
         * @returns True if gravity is enabled for this rigid body, false otherwise. */
        [[nodiscard]] VE_INLINE bool GravityEnabled() const {
            return _physicsWorld.GetRigidBodyComponentStore().IsGravityEnabled(_entity);
        }

        /** @brief Sets whether gravity is enabled for this rigid body. If gravity is enabled, the body will be affected by the global gravity force in the
         * physics simulation, which will cause it to accelerate downwards over time. If gravity is disabled, the body will not be affected by gravity and
         * will only respond to other forces and collisions.
         * @param gravityEnabled True to enable gravity for this rigid body, false to disable it. */
        VE_INLINE void SetGravityEnabled(bool gravityEnabled) {
            _physicsWorld.GetRigidBodyComponentStore().SetGravityEnabled(_entity, gravityEnabled);
        }

        /** @brief Retrieves the accumulated external force currently applied to this rigid body. This force represents the total force that has been
         * applied to the body from external sources (e.g., forces applied through the ApplyForce methods) during the current simulation step. The
         * accumulated force is used by the physics simulation to compute the body's acceleration and update its velocity and position accordingly.
         * @returns The accumulated external force currently applied to this rigid body. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetAccumulatedForce() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetExternalForce(_entity);
        }

        /** @brief Retrieves the accumulated external torque currently applied to this rigid body. This torque represents the total torque that has been
         * applied to the body from external sources (e.g., torques applied through the ApplyTorque methods) during the current simulation step. The
         * accumulated torque is used by the physics simulation to compute the body's angular acceleration and update its angular velocity and rotation
         * accordingly.
         * @returns The accumulated external torque currently applied to this rigid body. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetAccumulatedTorque() const {
            return _physicsWorld.GetRigidBodyComponentStore().GetExternalTorque(_entity);
        }

        /** @brief Checks whether this rigid body is allowed to sleep. If a body can sleep, it means that it is eligible to enter a sleeping state when it
         * comes to rest, which can improve performance by not simulating the body until it is woken up by an external force or collision. However, only
         * dynamic and kinematic bodies can sleep, so this method will return false for static bodies. If this method returns true, it means that the body
         * is allowed to sleep when it comes to rest, but it does not necessarily mean that the body is currently sleeping.
         * @returns True if this rigid body is allowed to sleep, false otherwise. */
        [[nodiscard]] VE_INLINE bool CanSleep() const {
            return _physicsWorld.GetRigidBodyComponentStore().CanSleep(_entity);
        }

        /** @brief Checks whether this rigid body is currently sleeping. A sleeping body is one that has come to rest and is not being actively simulated by
         * the physics engine to save computational resources. A sleeping body will not respond to forces or collisions until it is woken up by an external
         * force or collision. Only dynamic and kinematic bodies can sleep, so this method will return false for static bodies. If this method returns true,
         * it means that the body is currently sleeping and not being actively simulated in the physics world. */
        [[nodiscard]] VE_INLINE bool IsSleeping() const {
            return _physicsWorld.GetRigidBodyComponentStore().IsSleeping(_entity);
        }

        /** @brief Sets the mass of this rigid body. The mass represents the amount of matter in the body and affects how it responds to forces and
         * collisions according to Newton's second law of motion (F = m * a). A higher mass will result in less acceleration for a given force, while a
         * lower mass will result in more acceleration. The mass must be greater than zero for dynamic bodies, while static and kinematic bodies can have an
         * infinite mass (represented by a special value) since they do not respond to forces in the same way.
         * @param mass The mass to set for this rigid body. Must be greater than zero for dynamic bodies. Static and kinematic bodies can have an infinite
         * mass. */
        void SetMass(f32 mass);

        /** @brief Sets the linear velocity of this rigid body. The linear velocity represents the rate of change of the body's position in world space, and
         * it affects how the body moves through the physics simulation. Setting a new linear velocity will override any existing velocity and cause the
         * body to move in the direction and at the speed specified by the new velocity. This method will have no effect for static bodies, as they cannot
         * move.
         * @param velocity The linear velocity to set for this rigid body. */
        void SetLinearVelocity(const glm::vec3 &velocity);

        /** @brief Sets the angular velocity of this rigid body. The angular velocity represents the rate of change of the body's rotation in world space,
         * and it affects how the body rotates through the physics simulation. Setting a new angular velocity will override any existing angular velocity
         * and cause the body to rotate around its axes at the speed specified by the new angular velocity. This method will have no effect for static
         * bodies, as they cannot rotate.
         * @param angularVelocity The angular velocity to set for this rigid body. */
        void SetAngularVelocity(const glm::vec3 &angularVelocity);

        /** @brief Sets the local inertia tensor of this rigid body, which represents the body's resistance to angular acceleration around its local axes.
         * The local inertia tensor is typically defined in the body's local space and is used to compute the body's response to torques and angular forces.
         * Setting a new local inertia tensor will change how the body responds to torques and angular forces, which can affect its rotation and stability
         * in the physics simulation. This method allows you to manually specify the local inertia tensor for the body, but it should be used with care, as
         * an incorrect inertia tensor can lead to unrealistic behavior in the simulation. It is often more convenient to use the
         * UpdateLocalInertiaTensorFromColliders() method to automatically compute the inertia tensor based on the attached colliders.
         * @param localInertiaTensor The local inertia tensor to set for this rigid body. */
        void SetLocalInertiaTensor(const glm::vec3 &localInertiaTensor);

        /** @brief Sets the local center of mass of this rigid body, which represents the point in the body's local space where its mass is concentrated.
         * The local center of mass is used to compute the body's response to forces and torques, and it affects how the body moves and rotates in the
         * physics simulation. Setting a new local center of mass will change the point around which forces and torques are applied to the body, which can
         * affect its motion and rotation. This method allows you to manually specify the local center of mass for the body, but it should be used with
         * care, as an incorrect center of mass can lead to unrealistic behavior in the simulation. It is often more convenient to use the
         * UpdateLocalCenterOfMassFromColliders() method to automatically compute the center of mass based on the attached colliders.
         * @param localCenterOfMass The local center of mass to set for this rigid body. */
        void SetLocalCenterOfMass(const glm::vec3 &localCenterOfMass);

        /** @brief Updates the local center of mass of this rigid body based on the properties of its attached colliders. This method should be called
         * whenever the colliders attached to this body are modified (e.g., added, removed, or changed) to ensure that the body's center of mass is
         * consistent with its collision geometry. The method will compute the combined center of mass of all attached colliders and update the local center
         * of mass property in the rigid body component store. This allows the rigid body to accurately respond to forces and collisions based on its
         * current collider configuration. */
        void UpdateLocalCenterOfMassFromColliders();

        /** @brief Updates the local inertia tensor of this rigid body based on the properties of its attached colliders. This method should be called
         * whenever the colliders attached to this body are modified (e.g., added, removed, or changed) to ensure that the body's inertia tensor is
         * consistent with its collision geometry. The method will compute the combined inertia tensor of all attached colliders and update the local
         * inertia tensor property in the rigid body component store. This allows the rigid body to accurately respond to torques and angular forces based
         * on its current collider configuration. */
        void UpdateLocalInertiaTensorFromColliders();

        /** @brief Updates the mass of this rigid body based on the properties of its attached colliders. This method should be called whenever the
         * colliders attached to this body are modified (e.g., added, removed, or changed) to ensure that the body's mass is consistent with its collision
         * geometry. The method will compute the combined mass of all attached colliders and update the mass property in the rigid body component store.
         * This allows the rigid body to accurately respond to forces and collisions based on its current collider configuration. */
        void UpdateMassFromColliders();

        /** @brief Updates the mass properties of this rigid body (mass, local center of mass, and local inertia tensor) based on the properties of its
         * attached colliders. This method should be called whenever the colliders attached to this body are modified (e.g., added, removed, or changed) to
         * ensure that the body's mass properties are consistent with its collision geometry. The method will compute the combined mass, center of mass, and
         * inertia tensor of all attached colliders and update the corresponding properties in the rigid body component store. This allows the rigid body to
         * accurately respond to forces and collisions based on its current collider configuration. */
        void UpdateMassPropertiesFromColliders();

        /** @brief Sets the body type of this rigid body, which determines how it interacts with other bodies in the physics simulation and how it responds
         * to forces and collisions. The body type can be static (immovable), dynamic (movable), or kinematic (movable but not affected by forces). Changing
         * the body type will affect the behavior of the body in the simulation, so it should be done with care. For example, changing a dynamic body to
         * static will make it immovable and not respond to forces, while changing a static body to dynamic will allow it to move and respond to forces.
         * Kinematic bodies are moved by setting their velocity or transform directly and do not respond to forces but can affect dynamic bodies through
         * collisions.
         * @param bodyType The body type to set for this rigid body. */
        void SetBodyType(BodyType bodyType);

        /** @brief Applies a force to this rigid body at its center of mass. The provided force will be transformed from local space to world space using
         * the body's rotation from its transform, and then it will be added to any existing accumulated external force on the body. This will affect the
         * body's acceleration and movement in the physics simulation. If this is not a dynamic body, it should not have forces applied to it, so this
         * method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when a new force is applied,
         * so this method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in local space. This force will be transformed from local space to world space using the
         * body's rotation and then added to any existing accumulated external force on the body, affecting its acceleration and movement in the physics
         * simulation. */
        void ApplyLocalForceAtCenterOfMass(const glm::vec3 &force);

        /** @brief Applies a force to this rigid body at its center of mass. The provided force will be added to any existing accumulated external force on
         * the body, and it will affect the body's acceleration and movement in the physics simulation. If this is not a dynamic body, it should not have
         * forces applied to it, so this method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it
         * up when a new force is applied, so this method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in local space. This force will be added to any existing accumulated external force on the
         * body and will affect its acceleration and movement in the physics simulation. */
        void ApplyWorldForceAtCenterOfMass(const glm::vec3 &force);

        /** @brief Applies a force to this rigid body at a specific point in local space. The provided force will be transformed from local space to world
         * space using the body's rotation from its transform, and then it will be added to any existing accumulated external force on the body. This will
         * affect the body's acceleration and movement in the physics simulation. The point at which the force is applied will also affect the body's
         * rotation if it is not applied at the center of mass. If this is not a dynamic body, it should not have forces applied to it, so this method will
         * have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when a new force is applied, so this
         * method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in local space. This force will be transformed from local space to world space using the
         * body's rotation and then added to any existing accumulated external force on the body, affecting its acceleration and movement in the physics
         * simulation.
         * @param localPoint The point in local space where the force is applied to this rigid body. The location of the force application will affect the
         * body's rotation if it is not applied at the center of mass. */
        void ApplyLocalForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);

        /** @brief Applies a force to this rigid body at a specific point in local space. The provided force will be added to any existing accumulated
         * external force on the body, and it will affect the body's acceleration and movement in the physics simulation. The point at which the force is
         * applied will also affect the body's rotation if it is not applied at the center of mass. If this is not a dynamic body, it should not have forces
         * applied to it, so this method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when
         * a new force is applied, so this method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in local space. This force will be added to any existing accumulated external force on the
         * body and will affect its acceleration and movement in the physics simulation.
         * @param localPoint The point in local space where the force is applied to this rigid body. The location of the force application will affect the
         * body's rotation if it is not applied at the center of mass. */
        void ApplyWorldForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);

        /** @brief Applies a force to this rigid body at a specific point in world space. The provided force will be transformed from local space to world
         * space using the body's rotation from its transform, and then it will be added to any existing accumulated external force on the body. This will
         * affect the body's acceleration and movement in the physics simulation. The point at which the force is applied will also affect the body's
         * rotation if it is not applied at the center of mass. If this is not a dynamic body, it should not have forces applied to it, so this method will
         * have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when a new force is applied, so this
         * method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in local space. This force will be transformed from local space to world space using the
         * body's rotation and then added to any existing accumulated external force on the body, affecting its acceleration and movement in the physics
         * simulation.
         * @param worldPoint The point in world space where the force is applied to this rigid body. The location of the force application will affect the
         * body's rotation if it is not applied at the center of mass. */
        void ApplyLocalForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);

        /** @brief Applies a force to this rigid body at a specific point in world space. The provided force will be added to any existing accumulated
         * external force on the body, and it will affect the body's acceleration and movement in the physics simulation. The point at which the force is
         * applied will also affect the body's rotation if it is not applied at the center of mass. If this is not a dynamic body, it should not have forces
         * applied to it, so this method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when
         * a new force is applied, so this method will also set the body to be awake if it is currently sleeping.
         * @param force The force to apply to this rigid body in world space. This force will be added to any existing accumulated external force on the
         * body and will affect its acceleration and movement in the physics simulation.
         * @param worldPoint The point in world space where the force is applied to this rigid body. The location of the force application will affect the
         * body's rotation if it is not applied at the center of mass. */
        void ApplyWorldForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);

        /** @brief Applies a torque to this rigid body in local space. The provided torque will be transformed from local space to world space using the
         * body's rotation from its transform, and then it will be added to any existing accumulated external torque on the body. This will affect the
         * body's angular acceleration and rotation in the physics simulation. If this is not a dynamic body, it should not have torques applied to it, so
         * this method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to wake it up when a new torque is
         * applied, so this method will also set the body to be awake if it is currently sleeping.
         * @param torque The torque to apply to this rigid body in local space. */
        void ApplyLocalTorque(const glm::vec3 &torque);

        /** @brief Applies a torque to this rigid body in world space. The provided torque will be added to any existing accumulated external torque on the
         * body, and it will affect the body's angular acceleration and rotation in the physics simulation. If this is not a dynamic body, it should not
         * have torques applied to it, so this method will have no effect for static and kinematic bodies. If the body is currently sleeping, we need to
         * wake it up when a new torque is applied, so this method will also set the body to be awake if it is currently sleeping.
         * @param torque The torque to apply to this rigid body in world space. */
        void ApplyWorldTorque(const glm::vec3 &torque);

        /** @brief Resets the accumulated external force applied to this rigid body to zero. The body must be a dynamic body for this method to have any
         * effect, as static and kinematic bodies do not respond to forces. */
        void ResetForce();

        /** @brief Resets the accumulated external torque applied to this rigid body to zero. The body must be a dynamic body for this method to have any
         * effect, as static and kinematic bodies do not respond to torques. */
        void ResetTorque();

        /** @brief Sets whether this rigid body is allowed to sleep. If a body can sleep, it means that it is eligible to enter a sleeping state when it
         * comes to rest, which can improve performance by not simulating the body until it is woken up by an external force or collision. However, only
         * dynamic and kinematic bodies can sleep, so this method will have no effect for static bodies. If you set this to true for a dynamic or kinematic
         * body, it means that the body will be allowed to sleep when it comes to rest. If you set this to false, it means that the body will not be allowed
         * to sleep and will always be simulated in the physics world, even when it is at rest.
         * @param canSleep True to allow this rigid body to sleep when it comes to rest, false to prevent it from sleeping. */
        void SetCanSleep(bool canSleep);

        /** @brief Sets whether this rigid body is currently sleeping. A sleeping body is one that has come to rest and is not being actively simulated by
         * the physics engine to save computational resources. A sleeping body will not respond to forces or collisions until it is woken up by an external
         * force or collision. Only dynamic and kinematic bodies can sleep, so this method will have no effect for static bodies. If you set this to true,
         * it means that the body will be put to sleep and will not be actively simulated in the physics world until it is woken up by an external force or
         * collision. If you set this to false, it means that the body will be awake and actively simulated in the physics world, even if it is at rest.
         * @param sleeping True to put this rigid body to sleep, false to wake it up. */
        void SetIsSleeping(bool sleeping);

        /** @brief Overrides the SetTransform method from the Body class to set the transform of this rigid body. The transform includes the position and
         * rotation of the body in world space, and changing it will affect how the body interacts with other bodies in the physics simulation. When you set
         * a new transform for this rigid body, it will update the body's position and rotation in the physics world accordingly, and any attached colliders
         * will be updated to maintain their relative positions and orientations to the body. This method also handles waking up the body if it is currently
         * sleeping, since changing the transform should cause the body to become active in the simulation.
         * @param transform The new transform to be applied to this rigid body, including its position and rotation in world space. */
        void SetTransform(const TransformComponent &transform) override;

        /** @brief Overrides the SetIsActive method from the Body class to enable or disable this rigid body in the physics simulation. When a rigid body is
         * set to active, it will participate in collision detection and response, and it will be simulated according to its physical properties. When set
         * to inactive, the rigid body will not participate in collisions or be simulated, which can be useful for temporarily disabling a body without
         * removing it from the physics world. This method also handles enabling or disabling overlapping pairs for the body's colliders when its active
         * state changes.
         * @param isActive True to set this rigid body as active in the simulation, false to set it as inactive. */
        void SetIsActive(bool isActive) override;

        /** @brief Overrides the AddCollider method from the Body class to add a new collider to this rigid body using the specified collision shape and
         * local transform.
         * @param collisionShape The CollisionShape that defines the geometry of the new collider to be added to this rigid body.
         * @param transform The local transform of the new collider relative to this rigid body's origin.
         * @returns A reference to the newly created Collider that has been added to this rigid body. */
        Collider &AddCollider(CollisionShape &collisionShape, const TransformComponent &transform) override;

        /** @brief Overrides the RemoveCollider method from the Body class to remove a collider from this rigid body.
         * @param collider The collider to be removed from this rigid body. The collider must be currently attached to this body. */
        void RemoveCollider(Collider &collider) override;

        static VE_INLINE void ComputeWorldSpaceInertiaTensorInverse(const glm::mat3 &orientation,
                                                                    const glm::vec3 &inverseInertiaTensorLocal,
                                                                    glm::mat3 &outInverseInertiaTensorWorld) {
            outInverseInertiaTensorWorld[0][0] = orientation[0][0] * inverseInertiaTensorLocal.x;
            outInverseInertiaTensorWorld[0][1] = orientation[1][0] * inverseInertiaTensorLocal.x;
            outInverseInertiaTensorWorld[0][2] = orientation[2][0] * inverseInertiaTensorLocal.x;

            outInverseInertiaTensorWorld[1][0] = orientation[0][1] * inverseInertiaTensorLocal.y;
            outInverseInertiaTensorWorld[1][1] = orientation[1][1] * inverseInertiaTensorLocal.y;
            outInverseInertiaTensorWorld[1][2] = orientation[2][1] * inverseInertiaTensorLocal.y;

            outInverseInertiaTensorWorld[2][0] = orientation[0][2] * inverseInertiaTensorLocal.z;
            outInverseInertiaTensorWorld[2][1] = orientation[1][2] * inverseInertiaTensorLocal.z;
            outInverseInertiaTensorWorld[2][2] = orientation[2][2] * inverseInertiaTensorLocal.z;

            outInverseInertiaTensorWorld = orientation * outInverseInertiaTensorWorld;
        }

    private:
        void enableOverlappingPairs();
        void checkForDisabledOverlappingPairs();
        void awakeNeighborDisabledBodies();
    };

} // namespace Vulkyrie
