#include "physics/body/body.h"
#include "physics/types/material.h"

namespace Vulkyrie {

    Body::Body(Entity entity, PhysicsWorld &physicsWorld)
        : _entity(entity)
        , _physicsWorld(physicsWorld) {
    }

    void Body::SetTransform(const TransformComponent &transform) {
        // Update the body's transform component in the physics world with the new transform.
        _physicsWorld.GetTransformComponentStore().SetTransform(_entity, transform);

        // After updating the body's transform, we need to update the state of all colliders
        // attached to this body in the broad-phase system so that their AABBs
        // are updated based on the new transform and any overlapping pairs
        // involving these colliders will be tested for overlap again.
        updateBroadPhaseState();
    }

    Collider &Body::GetCollider(size_t colliderIndex) {
        VASSERT(colliderIndex < GetColliderCount(), "Collider index out of bounds.");

        Entity colliderEntity = _physicsWorld.GetBodyComponentStore().GetColliders(_entity)[colliderIndex];
        return _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);
    }

    const Collider &Body::GetCollider(size_t colliderIndex) const {
        VASSERT(colliderIndex < GetColliderCount(), "Collider index out of bounds.");

        Entity colliderEntity = _physicsWorld.GetBodyComponentStore().GetColliders(_entity)[colliderIndex];
        return _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);
    }

    bool Body::ContainsPoint(const glm::vec3 &point) const {
        // Get all colliders associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // Check if the point lies within any of the colliders.
        for (Entity colliderEntity : colliderEntities) {
            Collider &collider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);

            // If the point is contained within the collider, return true.
            if (collider.ContainsPoint(point)) {
                return true;
            }
        }

        // Else the point is not contained within any of the colliders, return false.
        return false;
    }

    AABB Body::GetAABB() const {
        // Get all colliders entities associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // If there are no colliders, return an empty AABB at the body's position.
        if (colliderEntities.empty()) {
            return AABB(glm::vec3(0.0f), glm::vec3(0.0f));
        }

        // Get the body's transform to world space.
        const TransformComponent &bodyTransform = GetTransform();

        // Initialize the body's AABB to the world-space AABB of the first collider.
        Collider &firstCollider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntities[0]);
        AABB bodyAABB = firstCollider.GetCollisionShape().ComputeTransformedAABB(bodyTransform * firstCollider.GetLocalToBodyTransform());

        // Merge the body's AABB with the world-space AABBs of the remaining colliders.
        for (Entity colliderEntity : colliderEntities) {
            Collider &collider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);
            AABB colliderAABB = collider.GetCollisionShape().ComputeTransformedAABB(bodyTransform * collider.GetLocalToBodyTransform());

            bodyAABB.MergeWithAABB(colliderAABB);
        }

        // Return the final merged AABB that encompasses all colliders attached to this body.
        return bodyAABB;
    }

    glm::vec3 Body::GetLocalPoint(const glm::vec3 &worldPoint) const {
        return GetTransform().Inverse() * worldPoint;
    }

    glm::vec3 Body::GetLocalVector(const glm::vec3 &worldVector) const {
        const TransformComponent &transform = GetTransform();
        const glm::quat inverseRotation = glm::inverse(transform.Rotation);
        const glm::vec3 localVector = inverseRotation * worldVector;

        return localVector;
    }

    void Body::UpdateHasSimulationCollidersFlag() {
        auto &bodyComponentStore = _physicsWorld.GetBodyComponentStore();

        // Get all collider entities associated with this body.
        const std::vector<Entity> &colliderEntities = bodyComponentStore.GetColliders(_entity);

        // Iterate through the colliders and check if any of them are simulation colliders.
        // If at least one is found, set the flag to true and return early.
        for (Entity colliderEntity : colliderEntities) {
            const bool isSimulationCollider = _physicsWorld.GetColliderComponentStore().IsSimulationCollider(colliderEntity);

            if (isSimulationCollider) {
                bodyComponentStore.SetHasSimulationColliders(_entity, true);
                return;
            }
        }
    }

    void Body::RemoveAllColliders() {
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // TODO: This can be optimized by batching the removal of colliders from the
        // collision system instead of doing it one by one in the RemoveCollider method.
        for (Entity colliderEntity : colliderEntities) {
            RemoveCollider(_physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity));
        }
    }

    void Body::SetIsActive(bool active) {
        auto &bodyComponentStore = _physicsWorld.GetBodyComponentStore();

        // If the body is already in the desired active state, do nothing.
        if (bodyComponentStore.IsBodyActive(_entity) == active) return;

        // Else update the body's active flag. We deliberately do not move the body between the enabled and
        // disabled component zones here: that transition is driven by the sleeping state in
        // RigidBody::SetIsActive() (via SetIsSleeping() -> PhysicsWorld::SetActiveStatusForBody()). This
        // mirrors ReactPhysics3D, whose Body::setIsActive() only updates the active flag and the body's
        // colliders, leaving the enabled/disabled partition to the sleeping machinery.
        bodyComponentStore.SetBodyActive(_entity, active);

        // When activating the body, we need to add its colliders to the collision system so that it can participate in collision detection.
        if (active) {
            const TransformComponent &transform = GetTransform();
            const std::vector<Entity> &colliderEntities = bodyComponentStore.GetColliders(_entity);

            for (Entity colliderEntity : colliderEntities) {
                Collider &collider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);
                const TransformComponent &localToBodyTransform = _physicsWorld.GetColliderComponentStore().GetLocalToBodyTransform(colliderEntity);

                const auto aabb = collider.GetCollisionShape().ComputeTransformedAABB(transform * localToBodyTransform);

                _physicsWorld.GetCollisionSystem().AddCollider(collider, aabb);
            }
        } else {
            // When deactivating the body, we need to remove its colliders from the collision system so that it no longer participates in collision detection.
            const std::vector<Entity> &colliderEntities = bodyComponentStore.GetColliders(_entity);

            for (Entity colliderEntity : colliderEntities) {
                Collider &collider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);

                if (collider.GetBroadPhaseID() != AABB_TREE_NULL_NODE) {
                    _physicsWorld.GetCollisionSystem().RemoveCollider(collider);
                }
            }
        }
    }

    Collider &Body::AddCollider(CollisionShape &collisionShape, const TransformComponent &transform) {
        const Entity colliderEntity = _physicsWorld.GetEntityManager().CreateEntity();
        auto *collider = new Collider(colliderEntity, *this);

        const TransformComponent localToWorldTransform = GetTransform() * transform;
        const PhysicsWorldSettings &settings = _physicsWorld.GetSettings();
        const Material material(settings.FrictionCoefficient, settings.RestitutionCoefficient);

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

        const bool isActive = IsActive();
        _physicsWorld.GetColliderComponentStore().AddComponent(colliderEntity, colliderComponent, isActive);
        _physicsWorld.GetBodyComponentStore().AddColliderToBody(_entity, colliderEntity);

        collisionShape.AddCollider(*collider);

        if (isActive) {
            const AABB aabb = collisionShape.ComputeTransformedAABB(localToWorldTransform);
            _physicsWorld.GetCollisionSystem().AddCollider(*collider, aabb);
        }

        return *collider;
    }

    void Body::RemoveCollider(Collider &collider) {
        if (collider.GetBroadPhaseID() != AABB_TREE_NULL_NODE) {
            _physicsWorld.GetCollisionSystem().RemoveCollider(collider);
        }

        auto &bodyComponentStore = _physicsWorld.GetBodyComponentStore();

        bodyComponentStore.RemoveColliderFromBody(_entity, collider.GetEntity());

        collider.GetCollisionShape().RemoveCollider(collider);

        _physicsWorld.GetColliderComponentStore().RemoveComponent(collider.GetEntity());

        _physicsWorld.GetEntityManager().DestroyEntity(collider.GetEntity());

        if (bodyComponentStore.HasSimulationColliders(_entity)) {
            UpdateHasSimulationCollidersFlag();
        }

        delete &collider;
    }

    void Body::requestBroadPhaseCollisionCheck() {
        // Get all collider entities associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        // Request a broad phase collision check for each collider to update their positions in the broad phase system
        // and ensure that any new or moved colliders are correctly accounted for in the next collision detection phase.
        for (Entity colliderEntity : colliderEntities) {
            Collider &collider = _physicsWorld.GetColliderComponentStore().GetCollider(colliderEntity);
            _physicsWorld.GetCollisionSystem().RequestBroadPhaseCollisionCheck(collider);
        }
    }

    void Body::updateBroadPhaseState() {
        // Get all collider entities associated with this body.
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        for (Entity colliderEntity : colliderEntities) {
            // Update local-to-world transform for this collider in the collider component store based on the body's current transform.
            const TransformComponent &bodyTransform = GetTransform();
            const TransformComponent &localToBodyTransform = _physicsWorld.GetColliderComponentStore().GetLocalToBodyTransform(colliderEntity);
            _physicsWorld.GetColliderComponentStore().SetLocalToWorldTransform(colliderEntity, bodyTransform * localToBodyTransform);

            // After updating the local-to-world transform for this collider,
            // we need to update its position in the broad phase system to
            // ensure that it is correctly accounted for in collision detection.
            _physicsWorld.GetCollisionSystem().UpdateCollider(colliderEntity);
        }
    }

} // namespace Vulkyrie
