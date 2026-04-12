#pragma once

#include "core/entity.h"
#include "physics/physics_world.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/collider.h"

namespace Vulkyrie {

    class Body {
        public:
            Body(Entity entity, PhysicsWorld &physicsWorld)
                : _entity(entity)
                , _physicsWorld(physicsWorld) {
            }

            // Delete the copy constructor and the copy assignment operator to prevent copying of Body instances,
            // as they are meant to be unique entities within the physics world and should not be duplicated.
            Body(const Body &) = delete;
            Body &operator=(const Body &) = delete;

            virtual ~Body() = default;

            [[nodiscard]] VE_FORCE_INLINE Entity GetEntity() const {
                return _entity;
            }

            bool IsActive() const;
            virtual void SetIsActive(bool active);

            const TransformComponent &GetTransform() const;
            virtual void SetTransform(const TransformComponent &transform);

            virtual void AddCollider(CollisionShape *shape, const TransformComponent &transform);
            virtual void RemoveCollider(Collider *collider);
            Collider &GetCollider(size_t colliderIndex);
            const Collider &GetCollider(size_t colliderIndex) const;
            u32 GetColliderCount() const;

            void ContainsPoint(const glm::vec3 &point) const;
            [[nodiscard]] VE_FORCE_INLINE bool CollidesWith(const AABB &aabb) const {
                return aabb.CollidesWith(GetAABB());
            }
            AABB GetAABB() const;

            glm::vec3 GetWorldPoint(const glm::vec3 &localPoint) const;
            glm::vec3 GetWorldVector(const glm::vec3 &localVector) const;
            glm::vec3 GetLocalPoint(const glm::vec3 &worldPoint) const;
            glm::vec3 GetLocalVector(const glm::vec3 &worldVector) const;

        protected:
            Entity _entity;
            PhysicsWorld &_physicsWorld;
    };

} // namespace Vulkyrie
