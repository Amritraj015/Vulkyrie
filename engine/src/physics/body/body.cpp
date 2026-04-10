#include "physics/body/body.h"

namespace Vulkyrie {

    // bool IsActive() const;
    // void SetIsActive(bool active);

    // const TransformComponent &GetTransform() const;

    void Body::SetTransform(const TransformComponent &transform) {
        _physicsWorld._transformComponentStore.SetTransform(_entity, transform);
    }

    // void AddCollider(CollisionShape *shape, const TransformComponent &transform);
    // void RemoveCollider(Collider *collider);
    // Collider &GetCollider(size_t colliderIndex);
    // const Collider &GetCollider(size_t colliderIndex) const;
    // u32 GetColliderCount() const;

    // void ContainsPoint(const glm::vec3 &point) const;
    // AABB GetAABB() const;

    // glm::vec3 GetWorldPoint(const glm::vec3 &localPoint) const;
    // glm::vec3 GetWorldVector(const glm::vec3 &localVector) const;
    // glm::vec3 GetLocalPoint(const glm::vec3 &worldPoint) const;
    // glm::vec3 GetLocalVector(const glm::vec3 &worldVector) const;

} // namespace Vulkyrie
