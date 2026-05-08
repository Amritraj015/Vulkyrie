#pragma once

#include "physics/body/body.h"

namespace Vulkyrie {

    class RigidBody final : public Body {
        public:
            RigidBody(Entity entity, PhysicsWorld &physicsWorld);

            // Delete the copy constructor and the copy assignment operator to prevent copying of RigidBody instances,
            // as they are meant to be unique entities within the physics world and should not be duplicated.
            RigidBody(const RigidBody &) = delete;
            RigidBody &operator=(const RigidBody &) = delete;

            // Delete the move constructor and the move assignment operator to prevent moving of RigidBody instances,
            // as they are tightly coupled with their entity and physics world, and moving them could lead to dangling references and other issues.
            RigidBody(RigidBody &&) = delete;
            RigidBody &operator=(RigidBody &&) = delete;

            /** @brief Default destructor for Body. */
            ~RigidBody() override = default;

            [[nodiscard]] VE_FORCE_INLINE f32 GetMass() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetMass(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLinearVelocity() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetLinearVelocity(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetAngularVelocity() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetAngularVelocity(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalInertiaTensor() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetLocalInertiaTensor(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalCenterOfMass() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetLocalCenterOfMass(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE f32 GetLinearDamping() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetLinearDamping(_entity);
            }

            VE_FORCE_INLINE void SetLinearDamping(f32 linearDamping) {
                VASSERT(linearDamping >= 0.0f, "Linear damping must be greater than or equal to zero.");

                _physicsWorld.GetRigidBodyComponentStore().SetLinearDamping(_entity, linearDamping);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLinearLockAxisFactor() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetLinearLockAxisFactor(_entity);
            }

            VE_FORCE_INLINE void SetLinearLockAxisFactor(const glm::vec3 &lockAxisFactor) {
                _physicsWorld.GetRigidBodyComponentStore().SetLinearLockAxisFactor(_entity, lockAxisFactor);
            }

            [[nodiscard]] VE_FORCE_INLINE f32 GetAngularDamping() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetAngularDamping(_entity);
            }

            VE_FORCE_INLINE void SetAngularDamping(f32 angularDamping) {
                VASSERT(angularDamping >= 0.0f, "Angular damping must be greater than or equal to zero.");

                _physicsWorld.GetRigidBodyComponentStore().SetAngularDamping(_entity, angularDamping);
            }

            [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetAngularLockAxisFactor() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetAngularLockAxisFactor(_entity);
            }

            VE_FORCE_INLINE void SetAngularLockAxisFactor(const glm::vec3 &lockAxisFactor) {
                _physicsWorld.GetRigidBodyComponentStore().SetAngularLockAxisFactor(_entity, lockAxisFactor);
            }

            [[nodiscard]] VE_FORCE_INLINE BodyType GetBodyType() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetBodyType(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE bool GravityEnabled() const {
                return _physicsWorld.GetRigidBodyComponentStore().IsGravityEnabled(_entity);
            }

            VE_FORCE_INLINE void SetGravityEnabled(bool gravityEnabled) {
                _physicsWorld.GetRigidBodyComponentStore().SetGravityEnabled(_entity, gravityEnabled);
            }

            [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetAccumulatedForce() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetExternalForce(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetAccumulatedTorque() const {
                return _physicsWorld.GetRigidBodyComponentStore().GetExternalTorque(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE bool CanSleep() const {
                return _physicsWorld.GetRigidBodyComponentStore().CanSleep(_entity);
            }

            [[nodiscard]] VE_FORCE_INLINE bool IsSleeping() const {
                return _physicsWorld.GetRigidBodyComponentStore().IsSleeping(_entity);
            }

            void SetTransform(const TransformComponent &transform) override;
            void SetMass(f32 mass);
            void SetLinearVelocity(const glm::vec3 &velocity);
            void SetAngularVelocity(const glm::vec3 &angularVelocity);
            void SetLocalInertiaTensor(const glm::vec3 &localInertiaTensor);
            void SetLocalCenterOfMass(const glm::vec3 &localCenterOfMass);
            void UpdateLocalCenterOfMassFromColliders();
            // void UpdateLocalInertiaTensorFromColliders();
            // void UpdateMassFromColliders();
            // void UpdateMassPropertiesFromColliders();
            void SetBodyType(BodyType bodyType);
            void SetIsSleeping(bool sleeping);

            void ApplyLocalForceAtCenterOfMass(const glm::vec3 &force);
            void ApplyWorldForceAtCenterOfMass(const glm::vec3 &force);
            void ApplyLocalForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
            void ApplyWorldForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
            void ApplyLocalForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);
            void ApplyWorldForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);

            void ApplyLocalTorque(const glm::vec3 &torque);
            void ApplyWorldTorque(const glm::vec3 &torque);

            void ResetForce();
            void ResetTorque();

            void SetCanSleep(bool canSleep);
            void SetIsActive(bool isActive) override;
            // Collider &AddCollider(CollisionShape *collisionShape, const TransformComponent &transform) override;
            // void RemoveCollider(Collider *collider) override;

        private:
            void enableOverlappingPairs();
            void checkForDisabledOverlappingPairs();
            glm::vec3 computeCenterOfMass();
    };

} // namespace Vulkyrie
