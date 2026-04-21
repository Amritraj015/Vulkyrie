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

    // AABB GetAABB() const;

    // glm::vec3 GetWorldPoint(const glm::vec3 &localPoint) const;
    // glm::vec3 GetWorldVector(const glm::vec3 &localVector) const;
    // glm::vec3 GetLocalPoint(const glm::vec3 &worldPoint) const;
    // glm::vec3 GetLocalVector(const glm::vec3 &worldVector) const;

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
