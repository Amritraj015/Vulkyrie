#pragma once

#include "core/pair.h"
#include "physics/constraint/contact_point.h"
#include "physics/physics_constants.h"
#include "physics/collision/collider.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/systems/broad_phase_system.h"
#include "physics/types/collision_dispatch.h"
#include "physics/types/contact_manifold.h"
#include "physics/types/contact_manifold_data.h"
#include "physics/types/contact_pair.h"
#include "physics/types/narrow_phase_algorithm.h"
#include "physics/types/narrow_phase_input.h"
#include "physics/types/overlapping_pairs.h"

namespace Vulkyrie {

    class PhysicsWorld;

    class CollisionSystem final {
    public:
        explicit CollisionSystem(PhysicsWorld &physicsWorld);

        // Delete the copy constructor and copy assignment operator.
        CollisionSystem(const CollisionSystem &) = delete;
        CollisionSystem &operator=(const CollisionSystem &) = delete;

        // Delete the move constructor and move assignment operator.
        CollisionSystem(CollisionSystem &&) = delete;
        CollisionSystem &operator=(CollisionSystem &&) = delete;

        /** @brief Default destructor for CollisionSystem. */
        ~CollisionSystem() = default;

        VE_FORCE_INLINE void AddCollider(Collider &collider, const AABB &aabb) {
            _broadPhaseSystem.AddCollider(collider, aabb);

            const i32 broadPhaseID = _colliderComponentStore.GetBroadPhaseID(collider.GetEntity());

            VASSERT(!_broadPhaseIDToColliderEntityMap.contains(broadPhaseID), "Broad-phase ID already exists in the map when trying to add a collider.");

            _broadPhaseIDToColliderEntityMap[broadPhaseID] = collider.GetEntity();
        }

        void RemoveCollider(Collider &collider);

        VE_FORCE_INLINE void UpdateCollider(Entity entity) {
            _broadPhaseSystem.UpdateCollider(entity);
        }

        VE_FORCE_INLINE void UpdateColliders() {
            _broadPhaseSystem.UpdateColliders();
        }

        void AddNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity);
        void RemoveNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity);

        VE_FORCE_INLINE void RequestBroadPhaseCollisionCheck(Collider &collider) {
            if (collider.GetBroadPhaseID() != -1) {
                _broadPhaseSystem.AddMovedCollider(collider.GetBroadPhaseID(), collider);
            }
        }

        void NotifyOverlappingPairsToTestOverlap(Collider &collider);
        NarrowPhaseAlgorithm SelectNarrowPhaseAlgorithm(const CollisionShapeType shapeOne, const CollisionShapeType shapeTwo) const;

    private:
        PhysicsWorld &_physicsWorld;
        ColliderComponentStore &_colliderComponentStore;
        RigidBodyComponentStore &_rigidBodyComponentStore;
        CollisionDispatch _collisionDispatch;

        std::unordered_set<Pair<Entity, Entity>> _nonCollidablePairs;
        OverlappingPairs _overlappingPairs;
        std::vector<std::pair<i32, i32>> _broadphaseOverlappingPairsToTest;
        BroadPhaseSystem _broadPhaseSystem;
        std::unordered_map<i32, Entity> _broadPhaseIDToColliderEntityMap;

        NarrowPhaseInput _narrowPhaseInput;
        std::vector<ContactPointData> _potentialContactPoints;
        std::vector<ContactManifoldData> _potentialContactManifolds;

        std::vector<ContactPair> _contactPairsOne;
        std::vector<ContactPair> _contactPairsTwo;
        std::vector<ContactPair> *_previousContactPairs;
        std::vector<ContactPair> *_currentContactPairs;
        std::vector<ContactPair> _lostContactPairs;
        std::unordered_map<u64, u32> _previousMapPairIDToContactPairIndex;

        std::vector<ContactManifold> _contactManifoldsOne;
        std::vector<ContactManifold> _contactManifoldsTwo;
        std::vector<ContactManifold> *_previousContactManifolds;
        std::vector<ContactManifold> *_currentContactManifolds;

        std::vector<ContactPoint> _contactPointsOne;
        std::vector<ContactPoint> _contactPointsTwo;
        std::vector<ContactPoint> *_previousContactPoints;
        std::vector<ContactPoint> *_currentContactPoints;

        u32 _previousPotentialContactManifoldsCount;
        u32 _previousPotentialContactPointsCount;
    };

} // namespace Vulkyrie
