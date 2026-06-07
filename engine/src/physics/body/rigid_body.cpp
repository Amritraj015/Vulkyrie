#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    RigidBody::RigidBody(Entity entity, PhysicsWorld &physicsWorld)
        : Body(entity, physicsWorld) {};

    void RigidBody::SetTransform(const TransformComponent &transform) {
        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();
        const glm::vec3 oldCenterOfMass = rigidBodyComponentStore.GetWorldCenterOfMass(_entity);
        const glm::vec3 centerOfMassLocal = rigidBodyComponentStore.GetLocalCenterOfMass(_entity);

        // Calculate the new world-space center of mass based on the
        // new transform and the local center of mass, and update it in the component store.
        const glm::vec3 newCenterOfMass = transform * centerOfMassLocal;
        rigidBodyComponentStore.SetWorldCenterOfMass(_entity, newCenterOfMass);

        // We need to adjust the linear velocity of the body to account for the change in the center of mass position.
        // The angular velocity remains unchanged, but the linear velocity needs to be updated based on the change in the center of mass position and the
        // current angular velocity of the body.
        // The formula for adjusting the linear velocity is: newLinearVelocity = oldLinearVelocity + cross(angularVelocity, newCenterOfMass - oldCenterOfMass)
        glm::vec3 linearVelocity = rigidBodyComponentStore.GetLinearVelocity(_entity);
        const glm::vec3 angularVelocity = rigidBodyComponentStore.GetAngularVelocity(_entity);
        linearVelocity += glm::cross(angularVelocity, newCenterOfMass - oldCenterOfMass);

        // Set the new linear velocity in the component store to account for the change in center of mass position.
        rigidBodyComponentStore.SetLinearVelocity(_entity, linearVelocity);

        // If the body is static, we need to update the constrained position and orientation to match the new transform,
        // since static bodies do not have velocities and their position and orientation are effectively "locked" in place.
        if (BodyType::Static == GetBodyType()) {
            rigidBodyComponentStore.SetConstrainedPosition(_entity, transform.Position);
            rigidBodyComponentStore.SetConstrainedOrientation(_entity, transform.Rotation);
        }

        // When we change the transform of a body, we need to wake it up if it's currently sleeping,
        // since a change in transform implies that the body has been moved or rotated,
        SetIsSleeping(false);

        // Finally, we can set the new transform in the TransformComponentStore
        // to update the body's position and rotation in the physics world.
        Body::SetTransform(transform);
    }

    void RigidBody::SetMass(f32 mass) {
        VASSERT(mass >= 0.0f, "Mass must be greater than or equal zero.");

        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();
        rigidBodyComponentStore.SetMass(_entity, mass);

        if (BodyType::Dynamic == GetBodyType()) {
            if (mass > 0.0f) {
                // For a dynamic body with positive mass, the inverse mass is simply 1 / mass.
                rigidBodyComponentStore.SetInverseMass(_entity, 1.0f / mass);
            } else {
                // For a dynamic body with zero mass, we treat it as immovable and set inverse mass to zero.
                rigidBodyComponentStore.SetInverseMass(_entity, 0.0f);
            }
        }
    }

    void RigidBody::SetLinearVelocity(const glm::vec3 &velocity) {
        // If this is a static body, it should not have its velocity changed,
        // so we can skip setting the velocity and return early.
        if (BodyType::Static == GetBodyType()) {
            return;
        }

        // Else, set the new linear velocity in the component store.
        _physicsWorld.GetRigidBodyComponentStore().SetLinearVelocity(_entity, velocity);

        // If the body is currently sleeping and the new velocity is non-zero, we need to wake it up,
        if (glm::dot(velocity, velocity) > 0.0f) {
            SetIsSleeping(false);
        }
    }

    void RigidBody::SetAngularVelocity(const glm::vec3 &angularVelocity) {
        // If this is a static body, it should not have its angular velocity changed,
        // so we can skip setting the angular velocity and return early.
        if (BodyType::Static == GetBodyType()) {
            return;
        }

        // Else, set the new angular velocity in the component store.
        _physicsWorld.GetRigidBodyComponentStore().SetAngularVelocity(_entity, angularVelocity);

        // If the body is currently sleeping and the new angular velocity is non-zero, we need to wake it up,
        if (glm::dot(angularVelocity, angularVelocity) > 0.0f) {
            SetIsSleeping(false);
        }
    }

    void RigidBody::SetLocalInertiaTensor(const glm::vec3 &localInertiaTensor) {
        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();

        // Set the new local inertia tensor in the component store.
        rigidBodyComponentStore.SetLocalInertiaTensor(_entity, localInertiaTensor);

        // If this is a dynamic body, we need to update the inverse mass and inverse inertia tensor in the component store as well.
        if (BodyType::Dynamic == GetBodyType()) {
            rigidBodyComponentStore.SetInverseLocalInertiaTensor(_entity,
                                                                 glm::vec3(localInertiaTensor.x > 0.0f ? 1.0f / localInertiaTensor.x : 0.0f,
                                                                           localInertiaTensor.y > 0.0f ? 1.0f / localInertiaTensor.y : 0.0f,
                                                                           localInertiaTensor.z > 0.0f ? 1.0f / localInertiaTensor.z : 0.0f));
        }
    }

    void RigidBody::SetLocalCenterOfMass(const glm::vec3 &localCenterOfMass) {
        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();

        // First, we need to calculate the new world-space center of mass based on the new local center of mass and the body's current transform.
        const glm::vec3 oldCenterOfMass = rigidBodyComponentStore.GetWorldCenterOfMass(_entity);
        const TransformComponent &transform = GetTransform();
        const glm::vec3 newCenterOfMass = transform * localCenterOfMass;

        // Update the local and world-space center of mass in the component store.
        rigidBodyComponentStore.SetLocalCenterOfMass(_entity, localCenterOfMass);
        rigidBodyComponentStore.SetWorldCenterOfMass(_entity, newCenterOfMass);

        // We need to adjust the linear velocity of the body to account for the change in the center of mass position.
        // The angular velocity remains unchanged, but the linear velocity needs to be updated based on the change in the center of mass position and the
        // current angular velocity of the body.
        // The formula for adjusting the linear velocity is: newLinearVelocity = oldLinearVelocity + cross(angularVelocity, newCenterOfMass - oldCenterOfMass)
        if (BodyType::Dynamic == GetBodyType()) {
            glm::vec3 linearVelocity = rigidBodyComponentStore.GetLinearVelocity(_entity);
            const glm::vec3 angularVelocity = rigidBodyComponentStore.GetAngularVelocity(_entity);
            linearVelocity += glm::cross(angularVelocity, newCenterOfMass - oldCenterOfMass);
            rigidBodyComponentStore.SetLinearVelocity(_entity, linearVelocity);
        }
    }

    void RigidBody::UpdateLocalCenterOfMassFromColliders() {
        ColliderComponentStore &colliderComponentStore = _physicsWorld.GetColliderComponentStore();

        f32 totalMass(0.0f);
        glm::vec3 centerOfMassLocal(0.0f);

        // Get all colliders associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // Iterate through each collider and accumulate the mass-weighted position of the center of mass in local space, as well as the total mass.
        for (Entity colliderEntity : colliderEntities) {
            const f32 colliderVolume = colliderComponentStore.GetCollisionShape(colliderEntity).GetVolume();
            const f32 colliderDensity = colliderComponentStore.GetMaterial(colliderEntity).GetDensity();

            // We can calculate the mass of the collider using its volume and density,
            // and then use that mass to weight the position of the center of mass contribution from this collider.
            const f32 colliderMass = colliderVolume * colliderDensity;

            const TransformComponent &localToBodyTransform = colliderComponentStore.GetLocalToBodyTransform(colliderEntity);

            // The center of mass contribution from this collider is its local position (from the local to body transform) weighted by its mass.
            centerOfMassLocal += localToBodyTransform.Position * colliderMass;

            // We also accumulate the total mass of the body by summing up the masses of all its colliders.
            totalMass += colliderMass;
        }

        // Finally, we can calculate the overall center of mass in local space
        // by dividing the accumulated mass-weighted position by the total mass.
        if (totalMass > 0.0f) {
            centerOfMassLocal /= totalMass;
        }

        // Set the new local center of mass in the component store, which will also
        // update the world center of mass and adjust the linear velocity accordingly.
        SetLocalCenterOfMass(centerOfMassLocal);
    }

    void RigidBody::UpdateLocalInertiaTensorFromColliders() {
        glm::mat3 inertiaTensor(0.0f);

        ColliderComponentStore &colliderComponentStore = _physicsWorld.GetColliderComponentStore();
        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // The parallel axis theorem requires the displacement from each collider's center to the body's
        // center of mass, so we need to fetch it before iterating over colliders.
        const glm::vec3 centerOfMassLocal = rigidBodyComponentStore.GetLocalCenterOfMass(_entity);

        for (Entity colliderEntity : colliderEntities) {
            const auto &shape = colliderComponentStore.GetCollisionShape(colliderEntity);
            const f32 colliderVolume = shape.GetVolume();
            const f32 colliderDensity = colliderComponentStore.GetMaterial(colliderEntity).GetDensity();
            const f32 colliderMass = colliderVolume * colliderDensity;

            // Get the inertia tensor of this shape in its own local frame (returned as a diagonal vec3).
            const glm::vec3 shapeInertiaDiag = shape.GetLocalInertiaTensor(colliderMass);

            // Rotate the diagonal shape tensor into body space: I_body = R * diag(I_shape) * R^T
            const TransformComponent &localToBodyTransform = colliderComponentStore.GetLocalToBodyTransform(colliderEntity);
            const glm::mat3 R = glm::mat3_cast(localToBodyTransform.Rotation);
            const glm::mat3 shapeInertiaMat(shapeInertiaDiag.x, 0.0f, 0.0f, 0.0f, shapeInertiaDiag.y, 0.0f, 0.0f, 0.0f, shapeInertiaDiag.z);
            const glm::mat3 bodyFrameTensor = R * shapeInertiaMat * glm::transpose(R);

            // Apply the parallel axis theorem to shift the tensor from the collider's center to the
            // body's center of mass: I_shifted = I_body + m * (|d|^2 * I3 - d (x) d)
            const glm::vec3 d = localToBodyTransform.Position - centerOfMassLocal;
            const glm::mat3 parallelAxisTerm = colliderMass * (glm::dot(d, d) * glm::mat3(1.0f) - glm::outerProduct(d, d));

            inertiaTensor += bodyFrameTensor + parallelAxisTerm;
        }

        // The result is a symmetric 3x3 matrix. We store only the diagonal (principal moments)
        // as the local inertia tensor, which is exact when all collider axes are body-aligned and
        // is a standard approximation for compound shapes in game physics.
        SetLocalInertiaTensor(glm::vec3(inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2]));
    }

    void RigidBody::UpdateMassFromColliders() {
        f32 totalMass(0.0f);
        ColliderComponentStore &colliderComponentStore = _physicsWorld.GetColliderComponentStore();

        // Get all colliders associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // Iterate through each collider and accumulate the total mass by summing up the mass of each collider,
        // which can be calculated using its volume and density from the collider component store.
        for (Entity colliderEntity : colliderEntities) {
            const f32 colliderVolume = colliderComponentStore.GetCollisionShape(colliderEntity).GetVolume();
            const f32 colliderDensity = colliderComponentStore.GetMaterial(colliderEntity).GetDensity();
            const f32 colliderMass = colliderVolume * colliderDensity;

            totalMass += colliderMass;
        }

        // Finally, we can set the total mass of the body in the component store,
        // which will also update the inverse mass for dynamic bodies.
        SetMass(totalMass);
    }

    void RigidBody::UpdateMassPropertiesFromColliders() {
        ColliderComponentStore &colliderComponentStore = _physicsWorld.GetColliderComponentStore();
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // ── Pass 1: accumulate total mass and mass-weighted local center of mass ──
        f32 totalMass(0.0f);
        glm::vec3 centerOfMassLocal(0.0f);

        for (Entity colliderEntity : colliderEntities) {
            const CollisionShape &shape = colliderComponentStore.GetCollisionShape(colliderEntity);
            const f32 colliderMass = shape.GetVolume() * colliderComponentStore.GetMaterial(colliderEntity).GetDensity();
            centerOfMassLocal += colliderComponentStore.GetLocalToBodyTransform(colliderEntity).Position * colliderMass;
            totalMass += colliderMass;
        }

        if (totalMass > 0.0f) {
            centerOfMassLocal /= totalMass;
        }

        // Commit mass (also updates inverse mass for dynamic bodies).
        SetMass(totalMass);

        // Commit local/world center of mass and adjust linear velocity to account for the CoM shift.
        SetLocalCenterOfMass(centerOfMassLocal);

        // ── Pass 2: accumulate inertia tensor using the now-known center of mass ──
        glm::mat3 inertiaTensor(0.0f);

        for (Entity colliderEntity : colliderEntities) {
            const CollisionShape &shape = colliderComponentStore.GetCollisionShape(colliderEntity);
            const f32 colliderMass = shape.GetVolume() * colliderComponentStore.GetMaterial(colliderEntity).GetDensity();
            const glm::vec3 shapeInertiaDiag = shape.GetLocalInertiaTensor(colliderMass);

            const TransformComponent &localToBodyTransform = colliderComponentStore.GetLocalToBodyTransform(colliderEntity);
            const glm::mat3 R = glm::mat3_cast(localToBodyTransform.Rotation);
            const glm::mat3 shapeInertiaMat(shapeInertiaDiag.x, 0.0f, 0.0f, 0.0f, shapeInertiaDiag.y, 0.0f, 0.0f, 0.0f, shapeInertiaDiag.z);
            const glm::mat3 bodyFrameTensor = R * shapeInertiaMat * glm::transpose(R);

            const glm::vec3 d = localToBodyTransform.Position - centerOfMassLocal;
            const glm::mat3 parallelAxisTerm = colliderMass * (glm::dot(d, d) * glm::mat3(1.0f) - glm::outerProduct(d, d));
            inertiaTensor += bodyFrameTensor + parallelAxisTerm;
        }

        // Commit inertia tensor (also updates inverse tensor for dynamic bodies).
        SetLocalInertiaTensor(glm::vec3(inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2]));
    }

    void RigidBody::SetBodyType(BodyType bodyType) {
        if (GetBodyType() == bodyType) {
            return;
        }

        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();
        rigidBodyComponentStore.SetBodyType(_entity, bodyType);

        if (BodyType::Static == bodyType) {
            rigidBodyComponentStore.SetLinearVelocity(_entity, glm::vec3(0));
            rigidBodyComponentStore.SetAngularVelocity(_entity, glm::vec3(0));

            const TransformComponent &transform = GetTransform();
            rigidBodyComponentStore.SetConstrainedPosition(_entity, transform.Position);
            rigidBodyComponentStore.SetConstrainedOrientation(_entity, transform.Rotation);
        }

        if (BodyType::Static == bodyType || BodyType::Kinematic == bodyType) {
            rigidBodyComponentStore.SetInverseMass(_entity, 0.0f);
            rigidBodyComponentStore.SetInverseLocalInertiaTensor(_entity, glm::vec3(0));
            rigidBodyComponentStore.SetInverseWorldInertiaTensor(_entity, glm::mat3(0));
        } else {
            const f32 mass = GetMass();

            if (mass > 0.0f) {
                rigidBodyComponentStore.SetInverseMass(_entity, 1.0f / mass);
            } else {
                rigidBodyComponentStore.SetInverseMass(_entity, 0.0f);
            }

            const glm::vec3 localInertiaTensor = GetLocalInertiaTensor();
            rigidBodyComponentStore.SetInverseLocalInertiaTensor(_entity,
                                                                 glm::vec3(localInertiaTensor.x > 0.0f ? 1.0f / localInertiaTensor.x : 0.0f,
                                                                           localInertiaTensor.y > 0.0f ? 1.0f / localInertiaTensor.y : 0.0f,
                                                                           localInertiaTensor.z > 0.0f ? 1.0f / localInertiaTensor.z : 0.0f));
        }

        _physicsWorld.SetActiveStatusForBody(_entity, BodyType::Static != bodyType);

        SetIsSleeping(false);

        if (BodyType::Static == bodyType) {
            checkForDisabledOverlappingPairs();
        }

        rigidBodyComponentStore.SetExternalForce(_entity, glm::vec3(0));
        rigidBodyComponentStore.SetExternalTorque(_entity, glm::vec3(0));
    }

    void RigidBody::SetIsSleeping(bool sleeping) {
        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();
        const bool isCurrentlySleeping = rigidBodyComponentStore.IsSleeping(_entity);

        if (sleeping == isCurrentlySleeping) {
            return;
        }

        if (GetBodyType() == BodyType::Static) {
            return;
        }

        if (!IsActive()) {
            VASSERT(isCurrentlySleeping, "Inactive bodies should always be sleeping.");
            return;
        }

        if (sleeping || isCurrentlySleeping) {
            rigidBodyComponentStore.SetSleepTime(_entity, 0.0f);
        }

        rigidBodyComponentStore.SetIsSleeping(_entity, sleeping);

        _physicsWorld.SetActiveStatusForBody(_entity, !sleeping);

        if (sleeping) {

            checkForDisabledOverlappingPairs();

            rigidBodyComponentStore.SetLinearVelocity(_entity, glm::vec3(0));
            rigidBodyComponentStore.SetAngularVelocity(_entity, glm::vec3(0));
            rigidBodyComponentStore.SetExternalForce(_entity, glm::vec3(0));
            rigidBodyComponentStore.SetExternalTorque(_entity, glm::vec3(0));
        } else {

            enableOverlappingPairs();

            requestBroadPhaseCollisionCheck();
        }
    }

    void RigidBody::ApplyLocalForceAtCenterOfMass(const glm::vec3 &force) {
        // Convert the local-space force to world space by applying the body's rotation from its transform.
        const glm::vec3 worldForce = GetTransform().Rotation * force;

        // Then we can apply the transformed world-space force at the center of mass of the body.
        ApplyWorldForceAtCenterOfMass(worldForce);
    }

    void RigidBody::ApplyWorldForceAtCenterOfMass(const glm::vec3 &force) {
        // If this not a dynamic body, it should not have forces applied to it,
        // so we can skip applying the force and return early.
        if (BodyType::Dynamic != GetBodyType()) {
            return;
        }

        // If the body is currently sleeping, we need to wake it up when a new force is applied,
        if (IsSleeping()) {
            SetIsSleeping(false);
        }

        // First, we need to calculate the new accumulated external force by adding the new force to
        // any existing external force in the component store, and update it in the component store.
        const glm::vec3 &externalForce = GetAccumulatedForce();
        _physicsWorld.GetRigidBodyComponentStore().SetExternalForce(_entity, externalForce + force);
    }

    void RigidBody::ApplyLocalForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint) {
        // Convert the local-space force to world space by applying the body's rotation from its transform.
        const glm::vec3 worldForce = GetTransform().Rotation * force;

        // Then we can apply the transformed world-space force at the specified local point on the body.
        ApplyWorldForceAtLocalPoint(worldForce, localPoint);
    }

    void RigidBody::ApplyWorldForceAtLocalPoint(const glm::vec3 &force, const glm::vec3 &localPoint) {
        // If this is not a dynamic body, it should not have forces applied to it,
        // so we can skip applying the force and return early.
        if (BodyType::Dynamic != GetBodyType()) {
            return;
        }

        // If the body is currently sleeping, we need to wake it up when a new force is applied,
        if (IsSleeping()) {
            SetIsSleeping(false);
        }

        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();

        // First, we need to calculate the new accumulated external force by adding the new force to
        // any existing external force in the component store, and update it in the component store.
        const glm::vec3 &externalForce = GetAccumulatedForce();
        rigidBodyComponentStore.SetExternalForce(_entity, externalForce + force);

        // We also need to calculate the torque generated by applying the force at the specified local
        // point and accumulate it with any existing external torque in the component store.
        const glm::vec3 &externalTorque = GetAccumulatedTorque();
        const glm::vec3 &centerOfMassWorld = rigidBodyComponentStore.GetWorldCenterOfMass(_entity);
        const glm::vec3 worldPoint = GetTransform() * localPoint;
        const glm::vec3 torqueFromForce = glm::cross(worldPoint - centerOfMassWorld, force);
        rigidBodyComponentStore.SetExternalTorque(_entity, externalTorque + torqueFromForce);
    }

    void RigidBody::ApplyLocalForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint) {
        // Convert the local-space force to world space by applying the body's rotation from its transform.
        const glm::vec3 worldForce = GetTransform().Rotation * force;

        // Then we can apply the transformed world-space force at the specified world point on the body.
        ApplyWorldForceAtWorldPoint(worldForce, worldPoint);
    }

    void RigidBody::ApplyWorldForceAtWorldPoint(const glm::vec3 &force, const glm::vec3 &worldPoint) {
        // If this is not a dynamic body, it should not have forces applied to it,
        // so we can skip applying the force and return early.
        if (BodyType::Dynamic != GetBodyType()) {
            return;
        }

        // If the body is currently sleeping, we need to wake it up when a new force is applied,
        if (IsSleeping()) {
            SetIsSleeping(false);
        }

        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();

        // First, we need to calculate the new accumulated external force by adding the new force to
        // any existing external force in the component store, and update it in the component store.
        const glm::vec3 &externalForce = GetAccumulatedForce();
        rigidBodyComponentStore.SetExternalForce(_entity, externalForce + force);

        // We also need to calculate the torque generated by applying the force at the specified world
        // point and accumulate it with any existing external torque in the component store.
        const glm::vec3 &externalTorque = GetAccumulatedTorque();
        const glm::vec3 &centerOfMassWorld = rigidBodyComponentStore.GetWorldCenterOfMass(_entity);
        const glm::vec3 torqueFromForce = glm::cross(worldPoint - centerOfMassWorld, force);
        rigidBodyComponentStore.SetExternalTorque(_entity, externalTorque + torqueFromForce);
    }

    void RigidBody::ApplyLocalTorque(const glm::vec3 &torque) {
        // Convert the local-space torque to world space by applying the body's rotation from its transform.
        const glm::vec3 worldTorque = GetTransform().Rotation * torque;

        // Then we can apply the transformed world-space torque to the body.
        ApplyWorldTorque(worldTorque);
    }

    void RigidBody::ApplyWorldTorque(const glm::vec3 &torque) {
        // If this is not a dynamic body, it should not have torques applied to it,
        // so we can skip applying the torque and return early.
        if (BodyType::Dynamic != GetBodyType()) {
            return;
        }

        RigidBodyComponentStore &rigidBodyComponentStore = _physicsWorld.GetRigidBodyComponentStore();

        // Else, accumulate the new torque with any existing external torque in the component store.
        const glm::vec3 currentTorque = rigidBodyComponentStore.GetExternalTorque(_entity);
        const glm::vec3 newTorque = currentTorque + torque;
        rigidBodyComponentStore.SetExternalTorque(_entity, newTorque);

        // If the body is currently sleeping and the new accumulated torque is non-zero, we need to wake it up,
        if (glm::dot(newTorque, newTorque) > 0.0f) {
            SetIsSleeping(false);
        }
    }

    void RigidBody::ResetForce() {
        // If the body is not dynamic, it should not have forces applied to it, so we can skip resetting.
        if (BodyType::Dynamic != GetBodyType()) return;

        // Else reset the accumulated force to zero for the next simulation step.
        _physicsWorld.GetRigidBodyComponentStore().SetExternalForce(_entity, glm::vec3(0.0f));
    }

    void RigidBody::ResetTorque() {
        // If the body is not dynamic, it should not have torques applied to it, so we can skip resetting.
        if (BodyType::Dynamic != GetBodyType()) return;

        // Else reset the accumulated torque to zero for the next simulation step.
        _physicsWorld.GetRigidBodyComponentStore().SetExternalTorque(_entity, glm::vec3(0.0f));
    }

    void RigidBody::SetCanSleep(bool canSleep) {
        // If this is a static body, it should not be able to sleep or wake up since it's always inactive,
        if (BodyType::Static == GetBodyType()) {
            return;
        }

        // Set the can sleep flag in the component store,
        // which will allow or disallow the body from being put to sleep by the physics simulation.
        _physicsWorld.GetRigidBodyComponentStore().SetCanSleep(_entity, canSleep);

        // If canSleep is false, we need to wake the body up if it's currently sleeping, since it should not be allowed to sleep.
        if (!canSleep) {
            SetIsSleeping(false);
        }
    }

    void RigidBody::SetIsActive(bool isActive) {
        // If the body is currently in the desired active state,
        // we can skip changing it and return early.
        if (IsActive() == isActive) {
            return;
        }

        // When activating a body, we need to wake it up if it's currently sleeping,
        // since an active body should be able to participate in the simulation and respond to forces.
        SetIsSleeping(!isActive);

        // Set the active status on the body component store,
        // which will also disable the body in the physics world when deactivating.
        Body::SetIsActive(isActive);
    }

    Collider &RigidBody::AddCollider(CollisionShape &collisionShape, const TransformComponent &transform) {
        Entity colliderEntity = _physicsWorld.GetEntityManager().CreateEntity();
        Collider *collider = new Collider(colliderEntity, *this);

        const TransformComponent localToWorldTransform = GetTransform() * transform;
        const PhysicsWorldSettings &settings = _physicsWorld.GetSettings();
        Material material(settings.FrictionCoefficient, settings.RestitutionCoefficient);

        ColliderComponent colliderComponent{
            _entity,
            collider,
            transform,
            &collisionShape,
            0x0001, // Default to category 1
            0xFFFF, // Default to colliding with all categories
            localToWorldTransform,
            material // Default material
        };
        const bool isDisabled = _physicsWorld.GetRigidBodyComponentStore().IsDisabled(_entity);
        _physicsWorld.GetColliderComponentStore().AddComponent(colliderEntity, colliderComponent, !isDisabled);
        _physicsWorld.GetBodyComponentStore().AddColliderToBody(_entity, colliderEntity);

        collisionShape.AddCollider(*collider);

        const bool isActive = IsActive();

        if (isActive) {
            const AABB aabb = collisionShape.ComputeTransformedAABB(localToWorldTransform);
            _physicsWorld.GetCollisionSystem().AddCollider(*collider, aabb);
        }

        _physicsWorld.GetBodyComponentStore().SetHasSimulationColliders(_entity, true);

        return *collider;
    }

    void RigidBody::RemoveCollider(Collider &collider) {
        awakeNeighborDisabledBodies();

        Body::RemoveCollider(collider);
    }

    void RigidBody::enableOverlappingPairs() {
        // const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);
        //
        // for (Entity colliderEntity : colliderEntities) {
        //
        //     OverlappingPairs::OverlappingPair* pair = mWorld.mCollisionDetection.mOverlappingPairs.getOverlappingPair(overlappingPairs[j]);
        //
        //     if (!pair->isEnabled) {
        //
        //         mWorld.mCollisionDetection.mOverlappingPairs.enablePair(overlappingPairs[j]);
        //     }
        // }
    }

    void RigidBody::checkForDisabledOverlappingPairs() {
        // const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);
        //
        // for (Entity colliderEntity : colliderEntities) {
        //
        //     OverlappingPairs::OverlappingPair* pair = mWorld.mCollisionDetection.mOverlappingPairs.getOverlappingPair(overlappingPairs[j]);
        //
        //     const Entity body1Entity = mWorld.mCollidersComponents.getBody(pair->collider1);
        //     const Entity body2Entity = mWorld.mCollidersComponents.getBody(pair->collider2);
        //
        //     const bool isBody1Disabled = mWorld.mRigidBodyComponents.getIsEntityDisabled(body1Entity);
        //     const bool isBody2Disabled = mWorld.mRigidBodyComponents.getIsEntityDisabled(body2Entity);
        //
        //     // If both bodies of the pair are disabled, we disable the overlapping pair
        //     if (isBody1Disabled && isBody2Disabled) {
        //
        //         mWorld.mCollisionDetection.disableOverlappingPair(overlappingPairs[j]);
        //     }
        //
        // }
    }

    void RigidBody::awakeNeighborDisabledBodies() {
    }

} // namespace Vulkyrie
