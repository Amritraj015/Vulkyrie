#pragma once

#include "physics/collision/collider.h"
#include "physics/components/collider_component_store.h"

namespace Vulkyrie {

    class CollisionSystem {
        public:
            CollisionSystem(ColliderComponentStore &colliderComponentStore)
                : _colliderComponentStore(colliderComponentStore) {
            }

            ~CollisionSystem() = default;

            VE_FORCE_INLINE void AddCollider(Collider &collider, const AABB &aabb) {
            }

            void NotifyOverlappingPairsToTestOverlap(Collider &collider) {
                const std::vector<i32> &overlappingPairs = _colliderComponentStore.GetCollisionPairs(collider.GetEntity());

                for (const auto overlappingPair : overlappingPairs) {
                    // Notify that the overlapping pair needs to be testbed for overlap
                    // _overlappingPairs.SetNeedToTestOverlap(overlappingPair, true);
                }
            }

            void RemoveCollider(Collider &collider) {
            }

            VE_FORCE_INLINE void RequestBroadPhaseCollisionCheck(Collider &collider) {
                if (collider.GetBroadPhaseID() != -1) {
                    // _broadPhaseSystem.AddMovedCollider(collider.GetBroadPhaseID(), collider);
                }
            }

        private:
            ColliderComponentStore &_colliderComponentStore;
    };

} // namespace Vulkyrie
