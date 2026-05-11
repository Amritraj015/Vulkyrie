#pragma once

#include "physics/collision/collider.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld;

    class CollisionSystem {
        public:
            explicit CollisionSystem(PhysicsWorld &physicsWorld);

            ~CollisionSystem() = default;

            VE_FORCE_INLINE void AddCollider([[maybe_unused]] Collider &collider, [[maybe_unused]] const AABB &aabb) {
            }

            void NotifyOverlappingPairsToTestOverlap([[maybe_unused]] Collider &collider) {
                // const std::vector<i32> &overlappingPairs = _colliderComponentStore.GetCollisionPairs(collider.GetEntity());
                //
                // for (const auto overlappingPair : overlappingPairs) {
                //     Notify that the overlapping pair needs to be testbed for overlap
                //     _overlappingPairs.SetNeedToTestOverlap(overlappingPair, true);
                // }
            }

            void RemoveCollider([[maybe_unused]] Collider &collider) {
            }

            VE_FORCE_INLINE void RequestBroadPhaseCollisionCheck(Collider &collider) {
                if (collider.GetBroadPhaseID() != -1) {
                    // _broadPhaseSystem.AddMovedCollider(collider.GetBroadPhaseID(), collider);
                }
            }

        private:
            PhysicsWorld &_physicsWorld;
            ColliderComponentStore &_colliderComponentStore;
            RigidBodyComponentStore &_rigidBodyComponentStore;
    };

} // namespace Vulkyrie
