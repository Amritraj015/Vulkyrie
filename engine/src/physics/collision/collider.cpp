#include "physics/collision/collider.h"
#include "physics/body/body.h"

namespace Vulkyrie {

    Collider::Collider(Entity entity, Body &body)
        : _entity(entity)
        , _body(body) {
    }

    CollisionShape &Collider::GetCollisionShape() {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollisionShape(_entity);
    }

    const CollisionShape &Collider::GetCollisionShape() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollisionShape(_entity);
    }

    const TransformComponent &Collider::GetLocalToBodyTransform() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetLocalToBodyTransform(_entity);
    }

    // void Collider::SetLocalToBodyTransform(const TransformComponent &transform) {
    // }

    const TransformComponent &Collider::GetLocalToWorldTransform() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetLocalToWorldTransform(_entity);
    }

    const AABB Collider::GetWorldSpaceAABB() const {
        const CollisionShape &shape = GetCollisionShape();
        return shape.ComputeTransformedAABB(GetLocalToWorldTransform());
    }

    // bool Collider::ContainsPoint(const glm::vec3 &point) const {
    // }

    Material &Collider::GetMaterial() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetMaterial(_entity);
    }

    void Collider::SetMaterial(const Material &material) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetMaterial(_entity, material);
    }

    u16 Collider::GetCollisionCategoryBits() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollisionCategoryBits(_entity);
    }

    // void Collider::SetCollisionCategoryBits(u16 collisionCategoryBits) {
    // }

    u16 Collider::GetCollidesWithMaskBits() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollidesWithMaskBits(_entity);
    }

    // void Collider::SetCollidesWithMaskBits(u16 maskBits) {
    // }

    bool Collider::IsTrigger() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().IsTrigger(_entity);
    }

    void Collider::SetTrigger(bool isTrigger) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetTrigger(_entity, isTrigger);

        // If this collider is now a trigger, it should not be a simulation collider, and if it is no longer a trigger, it should be a simulation collider.
        // This ensures that triggers generate overlap events but do not participate in collision response, while non-trigger colliders do participate in
        // simulation.
        if (isTrigger && IsSimulationCollider()) {
            SetSimulationCollider(false);
        }
    }

    bool Collider::IsSimulationCollider() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().IsSimulationCollider(_entity);
    }

    // void Collider::SetSimulationCollider(bool isSimulationCollider) {
    // }

    bool Collider::IsQueryCollider() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().IsQueryCollider(_entity);
    }

    void Collider::SetQueryCollider(bool isQueryCollider) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetQueryCollider(_entity, isQueryCollider);
    }

} // namespace Vulkyrie
