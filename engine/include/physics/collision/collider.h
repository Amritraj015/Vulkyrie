#pragma once

#include "core/entity.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/materials/material.h"

namespace Vulkyrie {
    class Body;

    class Collider {
        public:
            Collider(Entity entity, const Body &body)
                : _entity(entity)
                , _body(body) {
            }

            virtual ~Collider() = default;

            Collider(const Collider &) = delete;
            Collider &operator=(const Collider &) = delete;

            Collider(Collider &&) = delete;
            Collider &operator=(Collider &&) = delete;

            [[nodiscard]] VE_FORCE_INLINE Entity GetEntity() const {
                return _entity;
            }

            [[nodiscard]] VE_FORCE_INLINE const Body &GetBody() const {
                return _body;
            }

            CollisionShape &GetCollisionShape();
            const CollisionShape &GetCollisionShape() const;

            const TransformComponent &GetLocalBodyTransform() const;
            void SetLocalBodyTransform(const TransformComponent &transform);

            const AABB &GetWorldSpaceAABB() const;

            [[nodiscard]] VE_FORCE_INLINE bool CollidesWith(const AABB &aabb) const {
                return aabb.CollidesWith(GetWorldSpaceAABB());
            }

            [[nodiscard]] VE_FORCE_INLINE bool ContainsPoint(const glm::vec3 point) const {
                return GetWorldSpaceAABB().ContainsPoint(point);
            }

            bool ContainsPoint(const glm::vec3 &point) const;

            [[nodiscard]] VE_FORCE_INLINE Material &GetMaterial() const;

        protected:
            Entity _entity;
            const Body &_body;
    };

} // namespace Vulkyrie
