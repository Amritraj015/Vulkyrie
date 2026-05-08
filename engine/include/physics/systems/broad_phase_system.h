#pragma once

#include "physics/collision/broadphase/dynamic_aabb_tree.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/transform_component_store.h"
#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    class BroadPhaseSystem final {
        public:
            BroadPhaseSystem(ColliderComponentStore &colliderComponentStore,
                             TransformComponentStore &transformComponentStore,
                             RigidBodyComponentStore &rigidBodyComponentStore);

            BroadPhaseSystem(const BroadPhaseSystem &) = delete;
            BroadPhaseSystem &operator=(const BroadPhaseSystem &) = delete;

            BroadPhaseSystem(BroadPhaseSystem &&) = delete;
            BroadPhaseSystem &operator=(BroadPhaseSystem &&) = delete;

            ~BroadPhaseSystem() = default;

            void AddCollider(Collider &collider, const AABB &aabb) {
                VASSERT(collider.GetBroadPhaseID() == -1, "Collider is already in the broad phase system.");

                const i32 nodeID = _aabbTree.AddObject(aabb, &collider);

                _colliderComponentStore.SetBroadPhaseID(collider.GetEntity(), nodeID);
            }

        private:
            DynamicAABBTree _aabbTree;
            ColliderComponentStore &_colliderComponentStore;
            TransformComponentStore &_transformComponentStore;
            RigidBodyComponentStore &_rigidBodyComponentStore;
            std::unordered_set<i32> _movedShapes;

            void AddMovedCollider(i32 broadPhaseID, Collider &collider) {
                VASSERT(broadPhaseID != -1, "Collider must already be in the broad phase system to be moved.");

                _movedShapes.insert(broadPhaseID);
            }
    };

} // namespace Vulkyrie
