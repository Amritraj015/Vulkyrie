#pragma once

#include "core/entity.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/collision/collider.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    struct ColliderComponent final {
        public:
            Vulkyrie::Entity Entity;
            AABB LocalAABB;
            Vulkyrie::Collider &Collider;
            const TransformComponent &LocalToBodyTransform;
            Vulkyrie::CollisionShape &CollisionShape;
            u16 CollisionCategoryMaskBits;
            u16 CollidesWithMaskBits;
            const TransformComponent &LocalToWorldTransform;

            ColliderComponent(Vulkyrie::Entity entity,
                              const AABB &localAABB,
                              Vulkyrie::Collider &collider,
                              const TransformComponent &localToBodyTransform,
                              Vulkyrie::CollisionShape &collisionShape,
                              u16 collisionCategoryMaskBits,
                              u16 collidesWithMaskBits,
                              const TransformComponent &localToWorldTransform)
                : Entity(entity)
                , LocalAABB(localAABB)
                , Collider(collider)
                , LocalToBodyTransform(localToBodyTransform)
                , CollisionShape(collisionShape)
                , CollisionCategoryMaskBits(collisionCategoryMaskBits)
                , CollidesWithMaskBits(collidesWithMaskBits)
                , LocalToWorldTransform(localToWorldTransform) {
            }
    };

    class ColliderComponentStore final : public ComponentStore {
        public:
            ColliderComponentStore() = default;
            ~ColliderComponentStore() override = default;

        protected:
            void swapComponents(size_t indexA, size_t indexB) override;
            void removeLastComponentAndEntity() override;

        private:
    };

} // namespace Vulkyrie
