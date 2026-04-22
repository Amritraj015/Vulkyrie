#include "physics/body/body.h"

namespace Vulkyrie {

    Body::Body(Entity entity, PhysicsWorld &physicsWorld)
        : _entity(entity)
        , _physicsWorld(physicsWorld) {
    }

    bool Body::IsActive() const {
        return _physicsWorld.GetBodyComponentStore().IsBodyActive(_entity);
    }

    // void SetIsActive(bool active);

    const TransformComponent &Body::GetTransform() const {
        return _physicsWorld.GetTransformComponentStore().GetTransform(_entity);
    }

    void Body::SetTransform(const TransformComponent &transform) {
        _physicsWorld.GetTransformComponentStore().SetTransform(_entity, transform);
    }

    // void AddCollider(CollisionShape *shape, const TransformComponent &transform);
    // void RemoveCollider(Collider *collider);

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

    size_t Body::GetColliderCount() const {
        return _physicsWorld.GetBodyComponentStore().GetColliders(_entity).size();
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
        const TransformComponent &bodyTransform = _physicsWorld.GetTransformComponentStore().GetTransform(_entity);

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

    glm::vec3 Body::GetWorldPoint(const glm::vec3 &localPoint) const {
        return _physicsWorld.GetTransformComponentStore().GetTransform(_entity) * localPoint;
    }

    glm::vec3 Body::GetWorldVector(const glm::vec3 &localVector) const {
        return _physicsWorld.GetTransformComponentStore().GetTransform(_entity).Rotation * localVector;
    }

    glm::vec3 Body::GetLocalPoint(const glm::vec3 &worldPoint) const {
        const TransformComponent &transform = _physicsWorld.GetTransformComponentStore().GetTransform(_entity);
        const glm::quat inverseRotation = glm::inverse(transform.Rotation);
        const glm::vec3 localPoint = inverseRotation * (worldPoint - transform.Position);

        return localPoint;
    }

    glm::vec3 Body::GetLocalVector(const glm::vec3 &worldVector) const {
        const TransformComponent &transform = _physicsWorld.GetTransformComponentStore().GetTransform(_entity);
        const glm::quat inverseRotation = glm::inverse(transform.Rotation);
        const glm::vec3 localVector = inverseRotation * worldVector;

        return localVector;
    }

    void Body::UpdateHasSimulationCollidersFlag() {
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(_entity);

        for (Entity colliderEntity : colliderEntities) {
            const bool isSimulationCollider = _physicsWorld.GetColliderComponentStore().IsSimulationCollider(colliderEntity);

            if (isSimulationCollider) {
                _physicsWorld.GetBodyComponentStore().SetHasSimulationColliders(_entity, true);
                return;
            }
        }
    }

} // namespace Vulkyrie
