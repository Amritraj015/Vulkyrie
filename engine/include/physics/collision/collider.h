#pragma once

#include "core/entity.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/materials/material.h"

namespace Vulkyrie {
    class Body;

    class Collider final {
        public:
            Collider(Entity entity, Body &body);

            Collider(const Collider &) = delete;
            Collider &operator=(const Collider &) = delete;

            Collider(Collider &&) = delete;
            Collider &operator=(Collider &&) = delete;

            ~Collider() = default;

            [[nodiscard]] VE_FORCE_INLINE Entity GetEntity() const {
                return _entity;
            }

            [[nodiscard]] VE_FORCE_INLINE const Body &GetBody() const {
                return _body;
            }

            [[nodiscard]] CollisionShape &GetCollisionShape();
            [[nodiscard]] const CollisionShape &GetCollisionShape() const;

            [[nodiscard]] const TransformComponent &GetLocalToBodyTransform() const;
            void SetLocalToBodyTransform(const TransformComponent &transform);

            [[nodiscard]] const TransformComponent &GetLocalToWorldTransform() const;

            [[nodiscard]] const AABB GetWorldSpaceAABB() const;

            [[nodiscard]] VE_FORCE_INLINE bool CollidesWith(const AABB &aabb) const {
                return aabb.CollidesWith(GetWorldSpaceAABB());
            }

            [[nodiscard]] bool ContainsPoint(const glm::vec3 &worldSpacePoint) const;

            [[nodiscard]] Material &GetMaterial() const;
            void SetMaterial(const Material &material);

            [[nodiscard]] u16 GetCollisionCategoryBits() const;
            void SetCollisionCategoryBits(u16 collisionCategoryBits);

            [[nodiscard]] u16 GetCollidesWithMaskBits() const;
            void SetCollidesWithMaskBits(u16 maskBits);

            [[nodiscard]] i32 GetBroadPhaseID() const;

            [[nodiscard]] bool IsTrigger() const;
            void SetTrigger(bool isTrigger);

            [[nodiscard]] bool IsSimulationCollider() const;
            void SetSimulationCollider(bool isSimulationCollider);

            [[nodiscard]] bool IsQueryCollider() const;
            void SetQueryCollider(bool isQueryCollider);

        private:
            Entity _entity;
            Body &_body;
    };

} // namespace Vulkyrie
