#pragma once

#include "vlkypch.h"
#include "core/pair.h"
#include "physics/collision/collider.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/systems/broad_phase_system.h"
#include "physics/types/collision_callback.h"
#include "physics/types/collision_dispatch.h"
#include "physics/types/contact_manifold.h"
#include "physics/types/contact_manifold_data.h"
#include "physics/types/contact_pair.h"
#include "physics/types/contact_point.h"
#include "physics/types/event_listener.h"
#include "physics/types/half_edge_mesh.h"
#include "physics/types/narrow_phase_input.h"
#include "physics/types/overlap_callback.h"
#include "physics/types/overlapping_pairs.h"

namespace Vulkyrie {

    class CollisionSystem final {
    public:
        explicit CollisionSystem(PhysicsWorld &physicsWorld, HalfEdgeMesh &triangleHalfEdgeMesh);

        VE_DELETE_MOVE_AND_COPY(CollisionSystem);

        /** @brief Default destructor for CollisionSystem. */
        ~CollisionSystem() = default;

        VE_INLINE void AddCollider(Collider &collider, const AABB &aabb) {
            _broadPhaseSystem.AddCollider(collider, aabb);

            const u32 broadPhaseID = _colliderComponentStore.GetBroadPhaseID(collider.GetEntity());

            VASSERT(!_broadPhaseIDToColliderEntityMap.contains(broadPhaseID), "Broad-phase ID already exists in the map when trying to add a collider.");

            _broadPhaseIDToColliderEntityMap.emplace(broadPhaseID, collider.GetEntity());
        }

        VE_INLINE void UpdateCollider(Entity entity) {
            _broadPhaseSystem.UpdateCollider(entity);
        }

        VE_INLINE void UpdateColliders() {
            _broadPhaseSystem.UpdateColliders();
        }

        VE_INLINE void RemoveNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
            _nonCollidablePairs.erase(OverlappingPairs::ComputeBodiesIndexPair(bodyOneEntity, bodyTwoEntity));
        }

        VE_INLINE void RequestBroadPhaseCollisionCheck(Collider &collider) {
            if (collider.GetBroadPhaseID() != AABB_TREE_NULL_NODE) {
                _broadPhaseSystem.AddMovedCollider(collider.GetBroadPhaseID(), collider);
            }
        }

        [[nodiscard]] VE_INLINE const AABB GetWorldAABB(const Collider *collider) const {
            VASSERT(collider->GetBroadPhaseID() != AABB_TREE_NULL_NODE, "Invalid BroadPhaseID for collider.");
            return _broadPhaseSystem.GetFatAABB(collider->GetBroadPhaseID());
        }

        [[nodiscard]] VE_INLINE std::vector<ContactManifold> *GetCurrentContactManifolds() const {
            return _currentContactManifolds;
        }

        [[nodiscard]] VE_INLINE std::vector<ContactPoint> *GetCurrentContactPoints() const {
            return _currentContactPoints;
        }

        [[nodiscard]] VE_INLINE const std::vector<ContactPair> *GetCurrentContactPairs() const {
            return _currentContactPairs;
        }

        [[nodiscard]] VE_INLINE ContactPair &GetCurrentContactPairAtIndex(size_t index) {
            return (*_currentContactPairs)[index];
        }

        [[nodiscard]] VE_INLINE const OverlappingPair &GetOverlappingPair(u64 pairID) {
            return *_overlappingPairs.GetOverlappingPair(pairID);
        }

        VE_INLINE void EnableOverlappingPair(u64 pairID) {
            _overlappingPairs.EnablePair(pairID);
        }

        VE_INLINE void DisableOverlappingPair(u64 pairID) {
            _overlappingPairs.DisablePair(pairID);
        }

        void RemoveCollider(Collider &collider);
        void AddNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity);
        void NotifyOverlappingPairsToTestOverlap(Collider &collider);
        void ReportContactsAndTriggers();
        void ComputeCollisions();
        void CreateContacts();

        void TestOverlap(Body &bodyOne, Body &bodyTwo);
        void TestOverlap(Body &body, OverlapCallback &callback);
        void TestOverlap(OverlapCallback &callback);

        void TestCollision(Body &bodyOne, Body &bodyTwo, CollisionCallback &callback);
        void TestCollision(Body &body, CollisionCallback &callback);
        void TestCollision(CollisionCallback &callback);

    private:
        PhysicsWorld &_physicsWorld;
        ColliderComponentStore &_colliderComponentStore;
        RigidBodyComponentStore &_rigidBodyComponentStore;
        CollisionDispatch _collisionDispatch{};

        std::unordered_set<Pair<Entity, Entity>> _nonCollidablePairs{};
        OverlappingPairs _overlappingPairs;
        std::vector<Pair<u32, u32>> _broadphaseOverlappingPairs;
        BroadPhaseSystem _broadPhaseSystem;
        std::unordered_map<u32, Entity> _broadPhaseIDToColliderEntityMap;

        NarrowPhaseInput _narrowPhaseInput;
        std::vector<ContactPointData> _potentialContactPoints;
        std::vector<ContactManifoldData> _potentialContactManifolds;

        std::vector<ContactPair> _contactPairsOne;
        std::vector<ContactPair> _contactPairsTwo;
        std::vector<ContactPair> *_previousContactPairs;
        std::vector<ContactPair> *_currentContactPairs;
        std::vector<ContactPair> _lostContactPairs;
        std::unordered_map<u64, u32> _previousPairIDToContactPairIndexMap;

        std::vector<ContactManifold> _contactManifoldsOne;
        std::vector<ContactManifold> _contactManifoldsTwo;
        std::vector<ContactManifold> *_previousContactManifolds;
        std::vector<ContactManifold> *_currentContactManifolds;

        std::vector<ContactPoint> _contactPointsOne;
        std::vector<ContactPoint> _contactPointsTwo;
        std::vector<ContactPoint> *_previousContactPoints;
        std::vector<ContactPoint> *_currentContactPoints;

        HalfEdgeMesh &_triangleHalfEdgeMesh;

        size_t _previousPotentialContactManifoldsCount;
        size_t _previousPotentialContactPointsCount;

        void computeBroadPhase();
        void computeMiddlePhase(NarrowPhaseInput &batches, bool reportContacts, bool isWorldQuery);
        void computeMiddlePhaseCollisionSnapshot(std::vector<u64> &convexPairs, std::vector<u64> &concavePairs, NarrowPhaseInput &batches, bool reportContacts);
        void computeNarrowPhase();
        bool computeNarrowPhaseOverlapSnapshot(NarrowPhaseInput &batches, OverlapCallback *callback);

        bool computeNarrowPhaseCollisionSnapshot(NarrowPhaseInput &batches, CollisionCallback &callback);

        void computeOverlapSnapshotContactPairs(NarrowPhaseInput &batches, std::vector<ContactPair> &contactPair);
        void computeOverlapSnapshotContactPairs(NarrowPhaseDataBatch &batch,
                                                std::vector<ContactPair> &contactPairs,
                                                std::unordered_set<u64> overlappingContactPairIDs) const;
        void updateOverlappingPairs(const std::vector<Pair<u32, u32>> &overlappingNodes);
        void removeNonOverlappingPairs();
        void disableOverlappingPair(u64 pairID);
        void removeOverlappingPair(u64 pairID, bool notifyLostContact);
        void removeConvexOverlappingPairWithIndex(u64 pairIndex);
        void removeConcaveOverlappingPairWithIndex(u64 pairIndex);
        void addLostContactPair(OverlappingPair &pair);
        bool testNarrowPhaseCollision(NarrowPhaseInput &batches, bool clipWithPreviousAxisIfStillColliding);
        void computeConvexVsConcaveMiddlePhase(ConcaveOverlappingPair &overlappingPair, NarrowPhaseInput &batches, bool reportContacts);
        void swapPreviousAndCurrentContacts();
        void processPotentialContacts(NarrowPhaseDataBatch &batch,
                                      bool updateLastFrameData,
                                      std::vector<ContactPointData> &potentialContactPoints,
                                      std::vector<ContactManifoldData> &potentialContactManifolds,
                                      std::unordered_map<u64, u32> &mapPairIdToContactPairIndex,
                                      std::vector<ContactPair> &contactPairs);
        void processAllPotentialContacts(NarrowPhaseInput &batches,
                                         bool updateLastFrameInfo,
                                         std::vector<ContactPointData> &potentialContactPoints,
                                         std::vector<ContactManifoldData> &potentialContactManifolds,
                                         std::vector<ContactPair> &contactPairs);
        void reducePotentialContactManifolds(std::vector<ContactPair> &contactPairs,
                                             std::vector<ContactManifoldData> &potentialContactManifolds,
                                             const std::vector<ContactPointData> &potentialContactPoints) const;
        void addContactPairsToBodies();
        void computeMapPreviousContactPairs();
        void computeLostContactPairs();
        void createSnapshotContacts(std::vector<ContactPair> &contactPairs,
                                    std::vector<ContactManifold> &contactManifolds,
                                    std::vector<ContactPoint> &contactPoints,
                                    std::vector<ContactManifoldData> &potentialContactManifolds,
                                    std::vector<ContactPointData> &potentialContactPoints);
        void initContactsWithPreviousOnes();
        void reduceContactPoints(ContactManifoldData &manifold,
                                 const TransformComponent &shapeOneToWorldTransform,
                                 const std::vector<ContactPointData> &potentialContactPoints) const;

        void reportContacts(CollisionCallback &callback,
                            std::vector<ContactPair> &contactPairs,
                            // std::vector<ContactManifold> &manifolds,
                            std::vector<ContactPoint> &contactPoints,
                            std::vector<ContactPair> &lostContactPairs);

        void reportTriggers(EventListener &eventListener, std::vector<ContactPair> *contactPairs, std::vector<ContactPair> &lostContactPairs);
        // void reportDebugRenderingContacts(std::vector<ContactPair> *contactPairs,
        //                                   std::vector<ContactManifold> *manifolds,
        //                                   std::vector<ContactPoint> *contactPoints,
        //                                   std::vector<ContactPair> &lostContactPairs);
        f32 computePotentialManifoldLargestContactDepth(const ContactManifoldData &manifold, const std::vector<ContactPointData> &potentialContactPoints) const;
        void filterOverlappingPairs(Entity bodyEntity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const;
        void filterOverlappingPairs(Entity body1Entity, Entity body2Entity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const;
        void removeItemAtInArray(u32 array[], size_t index, u32 &arraySize) const;
        void removeDuplicatedContactPointsInManifold(ContactManifoldData &manifold, const std::vector<ContactPointData> &potentialContactPoints) const;
    };

} // namespace Vulkyrie
