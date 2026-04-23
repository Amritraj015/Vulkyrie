#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    RigidBody::RigidBody(Entity entity, PhysicsWorld &physicsWorld)
        : Body(entity, physicsWorld) {};

    void RigidBody::SetTransform([[maybe_unused]] const TransformComponent &transform) {
    }

    f32 RigidBody::GetMass() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetMass(_entity);
    }

    // void RigidBody::SetMass(f32 mass) {
    // }

    glm::vec3 RigidBody::GetLinearVelocity() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetLinearVelocity(_entity);
    }
    // void SetLinearVelocity(const glm::vec3 &velocity);

    glm::vec3 RigidBody::GetAngularVelocity() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetAngularVelocity(_entity);
    }
    // void SetAngularVelocity(const glm::vec3 &angularVelocity);

    glm::vec3 RigidBody::GetLocalInertiaTensor() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetLocalInertiaTensor(_entity);
    }
    // void SetLocalInertiaTensor(const glm::vec3 &localInertiaTensor);

    glm::vec3 RigidBody::GetLocalCenterOfMass() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetLocalCenterOfMass(_entity);
    }
    // void SetLocalCenterOfMass(const glm::vec3 &localCenterOfMass);

    f32 RigidBody::GetLinearDamping() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetLinearDamping(_entity);
    }
    // void SetLinearDamping(f32 linearDamping);

    glm::vec3 RigidBody::GetLinearLockAxisFactor() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetLinearLockAxisFactor(_entity);
    }

    void RigidBody::SetLinearLockAxisFactor(const glm::vec3 &lockAxisFactor) {
        _physicsWorld.GetRigidBodyComponentStore().SetLinearLockAxisFactor(_entity, lockAxisFactor);
    }

    f32 RigidBody::GetAngularDamping() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetAngularDamping(_entity);
    }
    // void SetAngularDamping(f32 angularDamping);

    glm::vec3 RigidBody::GetAngularLockAxisFactor() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetAngularLockAxisFactor(_entity);
    }

    void RigidBody::SetAngularLockAxisFactor(const glm::vec3 &lockAxisFactor) {
        _physicsWorld.GetRigidBodyComponentStore().SetAngularLockAxisFactor(_entity, lockAxisFactor);
    }

    // void UpdateLocalCenterOfMassFromColliders();
    // void UpdateLocalInertiaTensorFromColliders();
    // void UpdateMassFromColliders();
    // void UpdateMassPropertiesFromColliders();

    BodyType RigidBody::GetBodyType() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetBodyType(_entity);
    }

    // void SetBodyType(BodyType bodyType);

    bool RigidBody::GravityEnabled() const {
        return _physicsWorld.GetRigidBodyComponentStore().IsGravityEnabled(_entity);
    }

    void RigidBody::SetGravityEnabled(bool gravityEnabled) {
        _physicsWorld.GetRigidBodyComponentStore().SetGravityEnabled(_entity, gravityEnabled);
    }

    // void SetIsSleeping(bool sleeping);

    // void ApplyLocalForceAtCenterOfMass(const glm::vec3 &force);
    // void ApplyWorldForceAtCenterOfMass(const glm::vec3 &force);

    // void ApplyLocalForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
    // void ApplyWorldForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint);
    // void ApplyLocalForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);
    // void ApplyWorldForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint);

    // void ApplyLocalTorque(const glm::vec3 &torque);
    // void ApplyWorldTorque(const glm::vec3 &torque);

    void RigidBody::ResetForce() {
        // If the body is not dynamic, it should not have forces applied to it, so we can skip resetting.
        if (_physicsWorld.GetRigidBodyComponentStore().GetBodyType(_entity) != BodyType::DYNAMIC) return;

        // Else reset the accumulated force to zero for the next simulation step.
        _physicsWorld.GetRigidBodyComponentStore().SetExternalForce(_entity, glm::vec3(0.0f));
    }

    void RigidBody::ResetTorque() {
        // If the body is not dynamic, it should not have torques applied to it, so we can skip resetting.
        if (_physicsWorld.GetRigidBodyComponentStore().GetBodyType(_entity) != BodyType::DYNAMIC) return;

        // Else reset the accumulated torque to zero for the next simulation step.
        _physicsWorld.GetRigidBodyComponentStore().SetExternalTorque(_entity, glm::vec3(0.0f));
    }

    const glm::vec3 &RigidBody::GetAccumulatedForce() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetExternalForce(_entity);
    }

    const glm::vec3 &RigidBody::GetAccumulatedTorque() const {
        return _physicsWorld.GetRigidBodyComponentStore().GetExternalTorque(_entity);
    }

    bool RigidBody::CanSleep() const {
        return _physicsWorld.GetRigidBodyComponentStore().CanSleep(_entity);
    }
    // void SetCanSleep(bool canSleep);

    bool RigidBody::IsSleeping() const {
        return _physicsWorld.GetRigidBodyComponentStore().IsSleeping(_entity);
    }

    // void SetIsActive(bool isActive) override;
    // Collider &AddCollider(CollisionShape *collisionShape, const TransformComponent &transform) override;
    // void RemoveCollider(Collider *collider) override;
} // namespace Vulkyrie
