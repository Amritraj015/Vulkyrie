#include "physics/collision/collider.h"
#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    Collider::Collider(Entity entity, Body &body)
        : _entity(entity)
        , _body(body) {
    }

    void Collider::SetHasColliderShapeChanged(bool hasChanged) const {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetCollisionShapeChangedSize(_entity, hasChanged);
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

    void Collider::SetLocalToBodyTransform(const TransformComponent &transform) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetLocalToBodyTransform(_entity, transform);

        const TransformComponent &bodyTransform = _body.GetPhysicsWorld().GetTransformComponentStore().GetTransform(_body.GetEntity());
        const TransformComponent localToWorld = bodyTransform * transform;
        _body.GetPhysicsWorld().GetColliderComponentStore().SetLocalToWorldTransform(_entity, localToWorld);

        RigidBody *rigidBody = dynamic_cast<RigidBody *>(&_body);

        if (nullptr != rigidBody) {
            rigidBody->SetIsSleeping(false);
        }
    }

    const TransformComponent &Collider::GetLocalToWorldTransform() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetLocalToWorldTransform(_entity);
    }

    const AABB Collider::GetWorldSpaceAABB() const {
        const CollisionShape &shape = GetCollisionShape();
        return shape.ComputeTransformedAABB(GetLocalToWorldTransform());
    }

    bool Collider::ContainsPoint(const glm::vec3 &worldSpacePoint) const {

        const TransformComponent localToWorld = _body.GetPhysicsWorld().GetTransformComponentStore().GetTransform(_body.GetEntity()) *
                                                _body.GetPhysicsWorld().GetColliderComponentStore().GetLocalToBodyTransform(_entity);

        const glm::vec3 localPoint = glm::inverse(localToWorld.Rotation) * (worldSpacePoint - localToWorld.Position);
        const CollisionShape &collisionShape = _body.GetPhysicsWorld().GetColliderComponentStore().GetCollisionShape(_entity);
        return collisionShape.ContainsPoint(localPoint);
    }

    Material &Collider::GetMaterial() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetMaterial(_entity);
    }

    void Collider::SetMaterial(const Material &material) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetMaterial(_entity, material);
    }

    u16 Collider::GetCollisionCategoryBits() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollisionCategoryBits(_entity);
    }

    void Collider::SetCollisionCategoryBits(u16 collisionCategoryBits) {
        ColliderComponentStore &colliderComponentStore = _body.GetPhysicsWorld().GetColliderComponentStore();
        colliderComponentStore.SetCollisionCategoryBits(_entity, collisionCategoryBits);

        _body.GetPhysicsWorld().GetCollisionSystem().RequestBroadPhaseCollisionCheck(*this);

        VTRACE("Collider {}: Set Collision category bits: {}", colliderComponentStore.GetBroadPhaseID(_entity), collisionCategoryBits);
    }

    u16 Collider::GetCollidesWithMaskBits() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetCollidesWithMaskBits(_entity);
    }

    void Collider::SetCollidesWithMaskBits(u16 maskBits) {
        ColliderComponentStore &colliderComponentStore = _body.GetPhysicsWorld().GetColliderComponentStore();
        colliderComponentStore.SetCollidesWithMaskBits(_entity, maskBits);

        _body.GetPhysicsWorld().GetCollisionSystem().RequestBroadPhaseCollisionCheck(*this);

        VTRACE("Collider {}: Set Collides With mask bits: {}", colliderComponentStore.GetBroadPhaseID(_entity), maskBits);
    }

    i32 Collider::GetBroadPhaseID() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().GetBroadPhaseID(_entity);
    }

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

    void Collider::SetSimulationCollider(bool isSimulationCollider) {
        // Set the simulation collider flag for this collider.
        _body.GetPhysicsWorld().GetColliderComponentStore().SetSimulationCollider(_entity, isSimulationCollider);

        if (isSimulationCollider) {
            // If this collider is now a simulation collider,
            // ensure the owning body is marked as having simulation colliders so it will be included in the simulation step.
            _body.GetPhysicsWorld().GetBodyComponentStore().SetHasSimulationColliders(_body.GetEntity(), true);
        } else {
            // If this collider is no longer a simulation collider,
            // check if the owning body still has any simulation colliders and update its flag accordingly.
            _body.UpdateHasSimulationCollidersFlag();
        }

        // If this collider is now a simulation collider, it should not be a trigger.
        if (isSimulationCollider && IsTrigger()) {
            SetTrigger(false);
        }
    }

    bool Collider::IsQueryCollider() const {
        return _body.GetPhysicsWorld().GetColliderComponentStore().IsQueryCollider(_entity);
    }

    void Collider::SetQueryCollider(bool isQueryCollider) {
        _body.GetPhysicsWorld().GetColliderComponentStore().SetQueryCollider(_entity, isQueryCollider);
    }

} // namespace Vulkyrie
