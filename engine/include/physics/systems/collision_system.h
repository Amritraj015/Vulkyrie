#pragma once

#include "core/asserts.h"
#include "physics/physics_constants.h"
#include "physics/collision/collider.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/collision/narrowphase/capsule_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/capsule_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/convex_polyhedron_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_sphere_algorithm.h"
#include "physics/types/narrow_phase_algorithm.h"

namespace Vulkyrie {

    class PhysicsWorld;
    class BroadPhaseSystem;

    class CollisionSystem {
        public:
            explicit CollisionSystem(PhysicsWorld &physicsWorld);

            // Delete the copy constructor and copy assignment operator.
            CollisionSystem(const CollisionSystem &) = delete;
            CollisionSystem &operator=(const CollisionSystem &) = delete;

            // Delete the move constructor and move assignment operator.
            CollisionSystem(CollisionSystem &&) = delete;
            CollisionSystem &operator=(CollisionSystem &&) = delete;

            /** @brief Destructor for CollisionSystem. */
            ~CollisionSystem();

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

            NarrowPhaseAlgorithm SelectNarrowPhaseAlgorithm(const CollisionShapeType shapeOne, const CollisionShapeType shapeTwo) const;

        private:
            PhysicsWorld &_physicsWorld;
            ColliderComponentStore &_colliderComponentStore;
            RigidBodyComponentStore &_rigidBodyComponentStore;
            std::unordered_set<std::pair<Entity, Entity>> _nonCollidablePairs;
            OverlappingPairs _overlappingPairs;
            std::vector<std::pair<i32, i32>> _broadphaseOverlappingPairsToTest;
            BroadPhaseSystem _broadPhaseSystem;

            NarrowPhaseAlgorithm _collisionMatrix[SUPPORTED_COLLISION_SHAPE_TYPE_COUNT][SUPPORTED_COLLISION_SHAPE_TYPE_COUNT];

            CapsuleVsCapsuleAlgorithm *_capsuleVsCapsuleAlgorithm;
            CapsuleVsConvexPolyhedronAlgorithm *_capsuleVsConvexPolyhedronAlgorithm;
            ConvexPolyhedronVsConvexPolyhedronAlgorithm *_convexPolyhedronVsConvexPolyhedronAlgorithm;
            SphereVsCapsuleAlgorithm *_sphereVsCapsuleAlgorithm;
            SphereVsConvexPolyhedronAlgorithm *_sphereVsConvexPolyhedronAlgorithm;
            SphereVsSphereAlgorithm *_sphereVsSphereAlgorithm;

            void populateCollisionMatrix();
            NarrowPhaseAlgorithm selectAlgorithm(i32 shapeOne, i32 shapeTwo);
    };

} // namespace Vulkyrie
