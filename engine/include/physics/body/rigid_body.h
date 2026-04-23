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

            void SetTransform([[maybe_unused]] const TransformComponent &transform) override;

            f32 GetMass() const;
            // void SetMass(f32 mass);

            glm::vec3 GetLinearVelocity() const;
            // void SetLinearVelocity(const glm::vec3 &velocity);

            glm::vec3 GetAngularVelocity() const;
            // void SetAngularVelocity(const glm::vec3 &angularVelocity);

            glm::vec3 GetLocalInertiaTensor() const;
            // void SetLocalInertiaTensor(const glm::vec3 &localInertiaTensor);

            glm::vec3 GetLocalCenterOfMass() const;
            // void SetLocalCenterOfMass(const glm::vec3 &localCenterOfMass);

            f32 GetLinearDamping() const;
            // void SetLinearDamping(f32 linearDamping);

            glm::vec3 GetLinearLockAxisFactor() const;
            void SetLinearLockAxisFactor(const glm::vec3 &lockAxisFactor);

            f32 GetAngularDamping() const;
            // void SetAngularDamping(f32 angularDamping);

            glm::vec3 GetAngularLockAxisFactor() const;
            void SetAngularLockAxisFactor(const glm::vec3 &lockAxisFactor);

            // void UpdateLocalCenterOfMassFromColliders();
            // void UpdateLocalInertiaTensorFromColliders();
            // void UpdateMassFromColliders();
            // void UpdateMassPropertiesFromColliders();

            BodyType GetBodyType() const;
            // void SetBodyType(BodyType bodyType);

            bool GravityEnabled() const;
            void SetGravityEnabled(bool gravityEnabled);

            // void SetIsSleeping(bool sleeping);

            // void ApplyLocalForceAtCenterOfMass(const glm::vec3 &force);
            // void ApplyWorldForceAtCenterOfMass(const glm::vec3 &force);

            // void ApplyLocalForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
            // void ApplyWorldForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
            // void ApplyLocalForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);
            // void ApplyWorldForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);

            // void ApplyLocalTorque(const glm::vec3 &torque);
            // void ApplyWorldTorque(const glm::vec3 &torque);

            void ResetForce();
            void ResetTorque();

            const glm::vec3 &GetAccumulatedForce() const;
            const glm::vec3 &GetAccumulatedTorque() const;

            bool CanSleep() const;
            // void SetCanSleep(bool canSleep);
            bool IsSleeping() const;

            // void SetIsActive(bool isActive) override;
            // Collider &AddCollider(CollisionShape *collisionShape, const TransformComponent &transform) override;
            // void RemoveCollider(Collider *collider) override;
    };

} // namespace Vulkyrie
