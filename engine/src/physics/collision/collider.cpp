#include "physics/collision/collider.h"

namespace Vulkyrie {

    Collider::Collider(Entity entity, const Body &body)
        : _entity(entity)
        , _body(body) {
    }

    // CollisionShape &Collider::GetCollisionShape() {
    // }

    // const CollisionShape &Collider::GetCollisionShape() const {
    // }
    //
    // const TransformComponent &Collider::GetLocalToBodyTransform() const {
    // }
    // void Collider::SetLocalToBodyTransform(const TransformComponent &transform) {
    // }
    //
    // const TransformComponent &Collider::GetLocalToWorldTransform() const {
    // }
    //
    // const AABB &Collider::GetWorldSpaceAABB() const {
    // }
    //
    // bool Collider::ContainsPoint(const glm::vec3 &point) const {
    // }
    //
    // Material &Collider::GetMaterial() const {
    // }
    // void Collider::SetMaterial(const Material &material) {
    // }
    //
    // u16 Collider::GetCollisionCategoryBits() const {
    // }
    // void Collider::SetCollisionCategoryBits(u16 collisionCategoryBits) {
    // }
    //
    // u16 Collider::GetCollidesWithMaskBits() const {
    // }
    // void Collider::SetCollidesWithMaskBits(u16 maskBits) {
    // }
    //
    // bool Collider::IsTrigger() const {
    // }
    // void Collider::SetTrigger(bool isTrigger) {
    // }
    //
    // bool Collider::IsSimulationCollider() const {
    // }
    // void Collider::SetSimulationCollider(bool isSimulationCollider) {
    // }
    //
    // bool Collider::IsQueryCollider() const {
    // }
    // void Collider::SetQueryCollider(bool isQueryCollider) {
    // }

} // namespace Vulkyrie
