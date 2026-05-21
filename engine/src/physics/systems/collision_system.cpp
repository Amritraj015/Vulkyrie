#include "physics/systems/collision_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    CollisionSystem::CollisionSystem(PhysicsWorld &physicsWorld)
        : _physicsWorld(physicsWorld)
        , _colliderComponentStore(_physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(_physicsWorld.GetRigidBodyComponentStore())
        , _collisionDispatch()
        , _overlappingPairs(physicsWorld, _nonCollidablePairs, _collisionDispatch)
        , _broadPhaseSystem(physicsWorld)
        , _narrowPhaseInput() {
    }

    void CollisionSystem::RemoveCollider(Collider &collider) {
        const i32 broadPhaseID = collider.GetBroadPhaseID();

        VASSERT(broadPhaseID != -1, "Collider does not have a valid broad-phase ID when trying to remove it from the collision system.");
        VASSERT(_broadPhaseIDToColliderEntityMap.contains(broadPhaseID), "Broad-phase ID does not exist in the map when trying to remove a collider.");

        const std::vector<u64> &overlappingPairs = _colliderComponentStore.GetOverlappingPairs(collider.GetEntity());

        while (overlappingPairs.size() > 0) {
            removeOverlappingPair(overlappingPairs[0], false);
        }

        _broadPhaseIDToColliderEntityMap.erase(broadPhaseID);
        _broadPhaseSystem.RemoveCollider(collider);
    }

    void CollisionSystem::AddNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
        (void)bodyOneEntity;
        (void)bodyTwoEntity;
    }

    void CollisionSystem::RemoveNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
        (void)bodyOneEntity;
        (void)bodyTwoEntity;
    }

    void CollisionSystem::NotifyOverlappingPairsToTestOverlap(Collider &collider) {
        const std::vector<u64> &overlappingPairs = _colliderComponentStore.GetOverlappingPairs(collider.GetEntity());

        for (const auto overlappingPair : overlappingPairs) {
            // Notify that the overlapping pair needs to be testbed for overlap
            _overlappingPairs.SetRequiresCollisionCheck(overlappingPair, true);
        }
    }

    void CollisionSystem::ComputeCollisions() {
        // Compute broad-phase collision detection to generate potential collision pairs.
        computeBroadPhase();

        // Compute middle-phase collision detection to filter broad-phase pairs
        // and generate contact points/manifolds for narrow-phase testing.
        computeMiddlePhase(_narrowPhaseInput, true, false);

        // Compute narrow-phase collision detection to generate final contact points and manifolds,
        // and to update the overlapping pair state for the current step.
        computeNarrowPhase();
    }

    void CollisionSystem::computeBroadPhase() {
        VASSERT(_broadphaseOverlappingPairs.size() == 0, "Broad-phase overlapping pair list should be empty at the start of broad-phase computation.");

        // Query the broad-phase system to compute the pairs of colliders that
        // are potentially overlapping based on their AABBs in the dynamic AABB tree.
        _broadPhaseSystem.ComputeOverlappingPairs(_broadphaseOverlappingPairs);

        // Create new overlapping pairs for any broad-phase pairs that are not already
        // in the overlapping pair list, and update existing overlapping pairs with
        // the new broad-phase pair information.
        updateOverlappingPairs(_broadphaseOverlappingPairs);

        // Remove non-overlapping pairs from the overlapping pair list.
        removeNonOverlappingPairs();

        // Clear the broad-phase overlapping pairs list to prepare
        // for the next broad-phase computation in the next step.
        _broadphaseOverlappingPairs.clear();
    }

    // void CollisionSystem::computeMiddlePhase(NarrowPhaseInput &batches, bool reportContacts, bool isWorldQuery) {
    // }
    //
    // void CollisionSystem::computeMiddlePhaseCollisionSnapshot(std::vector<u64> &convexPairs,
    //                                                           std::vector<u64> &concavePairs,
    //                                                           NarrowPhaseInput &batches,
    //                                                           bool reportContacts) {
    // }

    void CollisionSystem::computeNarrowPhase() {
        // Swap the previous and current contact buffers to prepare for the new narrow-phase collision detection results.
        swapPreviousAndCurrentContacts();

        // Reserve space in the potential contact manifolds and points vectors based on the counts from the previous frame. This is an optimization to avoid
        // unnecessary reallocations during narrow-phase collision detection, as the number of potential contact manifolds and points generated during
        // narrow-phase is often similar from frame to frame, especially in stable scenes.
        _potentialContactManifolds.reserve(_previousPotentialContactManifoldsCount);
        _potentialContactPoints.reserve(_previousPotentialContactPointsCount);

        // Perform narrow-phase collision detection on all overlapping pairs that require a collision check this frame.
        testNarrowPhaseCollision(_narrowPhaseInput, true);

        // Process all potential contacts after narrow-phase collision detection.
        processAllPotentialContacts(_narrowPhaseInput, true, _potentialContactPoints, _potentialContactManifolds, *_currentContactPairs);

        // After processing all potential contacts, we can perform manifold reduction to limit the number of contact manifolds per pair to a reasonable
        // maximum, which helps to improve the performance of the constraint solver while still maintaining good collision response quality.
        reducePotentialContactManifolds(*_currentContactPairs, _potentialContactManifolds, _potentialContactPoints);

        // Add the contact pairs generated during narrow-phase collision detection to the corresponding rigid bodies.
        addContactPairsToBodies();

        VASSERT(_currentContactManifolds->size() == 0, "Current contact manifolds should be empty at the end of narrow-phase collision detection.");
        VASSERT(_currentContactPoints->size() == 0, "Current contact points should be empty at the end of narrow-phase collision detection.");
    }

    // bool CollisionSystem::computeNarrowPhaseOverlapSnapshot(NarrowPhaseInput &batches, OverlapCallback &callback) {
    //     return false;
    // }
    //
    // bool CollisionSystem::computeNarrowPhaseCollisionSnapshot(NarrowPhaseInput &batches, CollisionCallback &callback) {
    //     return false;
    // }
    //
    // void CollisionSystem::computeOverlapSnapshotContactPair(NarrowPhaseInput &batches, std::vector<ContactPair> &contactPair) {
    // }
    //
    // void CollisionSystem::computeOverlapSnapshotContactPair(NarrowPhaseDataBatch &batch,
    //                                                         std::vector<ContactPair> &contactPairs,
    //                                                         std::unordered_set<u64> overlappingContactPairIDs) const {
    // }
    //
    // void CollisionSystem::updateOverlappingPairs(const std::vector<Pair<i32, i32>> &overlappingNodes) {
    // }

    void CollisionSystem::removeNonOverlappingPairs() {
    }

    void CollisionSystem::disableOverlappingPair(u64 pairID) {
        _overlappingPairs.DisablePair(pairID);
    }

    void CollisionSystem::removeOverlappingPair(u64 pairID, bool notifyLostContact) {
        OverlappingPair *pair = _overlappingPairs.GetOverlappingPair(pairID);

        // If the pair was colliding in the last frame and we are supposed to notify
        // about lost contact, add this pair to the list of lost contact pairs.
        if (pair->WereCollidingLastFrame && notifyLostContact) {
            addLostContactPair(*pair);
        }

        // Remove the pair from the overlapping pair list, which will also
        // handle any necessary cleanup and state updates for this pair.
        _overlappingPairs.RemovePair(pairID);
    }

    void CollisionSystem::removeConvexOverlappingPairWithIndex(u64 pairIndex) {
        // Get the convex overlapping pair at the specified index.
        OverlappingPair &pair = _overlappingPairs._convexPairs[pairIndex];

        // If the pair was colliding in the last frame, add it to the list of lost contact pairs before
        // removing it from the active convex pairs list. This ensures that we can report lost contacts
        // for pairs that are being removed due to no longer overlapping, but were still colliding in the previous frame.
        if (pair.WereCollidingLastFrame) {
            addLostContactPair(pair);
        }

        // Remove the convex pair from the active convex pairs list,
        // which will also handle any necessary cleanup and state updates for this pair.
        _overlappingPairs.RemoveConvexPairWithIndex(pairIndex, true);
    }

    void CollisionSystem::removeConcaveOverlappingPairWithIndex(u64 pairIndex) {
        // Get the concave overlapping pair at the specified index.
        OverlappingPair &pair = _overlappingPairs._concavePairs[pairIndex];

        // If the pair was colliding in the last frame, add it to the list of lost contact pairs before
        // removing it from the active concave pairs list. This ensures that we can report lost contacts
        // for pairs that are being removed due to no longer overlapping, but were still colliding in the previous frame.
        if (pair.WereCollidingLastFrame) {
            addLostContactPair(pair);
        }

        // Remove the concave pair from the active concave pairs list,
        // which will also handle any necessary cleanup and state updates for this pair.
        _overlappingPairs.RemoveConcavePairWithIndex(pairIndex, true);
    }

    void CollisionSystem::addLostContactPair(OverlappingPair &pair) {
        // Get the indices of the colliders in the collider component store for both colliders in the pair.
        const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(pair.ColliderOneEntity);
        const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(pair.ColliderTwoEntity);

        // Get the entities of the rigid bodies associated with both colliders in the pair.
        const Entity bodyOneEntity = _colliderComponentStore.GetEntityAtIndex(colliderOneIndex);
        const Entity bodyTwoEntity = _colliderComponentStore.GetEntityAtIndex(colliderTwoIndex);

        // Determine if either collider in the pair is a trigger, which will affect how we report lost contacts for this pair.
        const bool isColliderOneTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex);
        const bool isColliderTwoTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);
        const bool isTrigger = isColliderOneTrigger || isColliderTwoTrigger;

        // Compute the new contact pair index.
        const size_t contactPairIndex = _lostContactPairs.size();

        // Create a new ContactPair entry for this lost contact pair and add it to the list of lost contact pairs.
        _lostContactPairs.emplace_back(
            pair.PairID, bodyOneEntity, bodyTwoEntity, pair.ColliderOneEntity, pair.ColliderTwoEntity, contactPairIndex, true, isTrigger);
    }

    bool CollisionSystem::testNarrowPhaseCollision(NarrowPhaseInput &batches, bool clipWithPreviousAxisIfStillColliding) {
        bool isColliding = false;

        SphereVsSphereAlgorithm &sphereVsSphereAlgorithm = _collisionDispatch.GetSphereVsSphereAlgorithm();
        SphereVsCapsuleAlgorithm &sphereVsCapsuleAlgorithm = _collisionDispatch.GetSphereVsCapsuleAlgorithm();
        SphereVsConvexPolyhedronAlgorithm &sphereVsConvexPolyhedronAlgorithm = _collisionDispatch.GetSphereVsConvexPolyhedronAlgorithm();
        CapsuleVsCapsuleAlgorithm &capsuleVsCapsuleAlgorithm = _collisionDispatch.GetCapsuleVsCapsuleAlgorithm();
        CapsuleVsConvexPolyhedronAlgorithm &capsuleVsConvexPolyhedronAlgorithm = _collisionDispatch.GetCapsuleVsConvexPolyhedronAlgorithm();
        ConvexPolyhedronVsConvexPolyhedronAlgorithm &convexPolyhedronVsConvexPolyhedronAlgorithm =
            _collisionDispatch.GetConvexPolyhedronVsConvexPolyhedronAlgorithm();

        NarrowPhaseDataBatch &sphereVsSphereBatch = batches.GetSphereVsSphereBatch();
        NarrowPhaseDataBatch &sphereVsCapsuleBatch = batches.GetSphereVsCapsuleBatch();
        NarrowPhaseDataBatch &sphereVsConvexPolyhedronBatch = batches.GetSphereVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &capsuleVsCapsuleBatch = batches.GetCapsuleVsCapsuleBatch();
        NarrowPhaseDataBatch &capsuleVsConvexPolyhedronBatch = batches.GetCapsuleVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &convexPolyhedronVsConvexPolyhedronBatch = batches.GetConvexPolyhedronVsConvexPolyhedronBatch();

        size_t batchSize = sphereVsSphereBatch.Data.size();
        if (batchSize > 0) {
            isColliding |= sphereVsSphereAlgorithm.PerformCollisionCheck(sphereVsSphereBatch, 0, batchSize);
        }

        batchSize = sphereVsCapsuleBatch.Data.size();
        if (batchSize > 0) {
            isColliding |= sphereVsCapsuleAlgorithm.PerformCollisionCheck(sphereVsCapsuleBatch, 0, batchSize);
        }

        batchSize = sphereVsConvexPolyhedronBatch.Data.size();
        if (batchSize > 0) {
            isColliding |=
                sphereVsConvexPolyhedronAlgorithm.PerformCollisionCheck(sphereVsConvexPolyhedronBatch, 0, batchSize, clipWithPreviousAxisIfStillColliding);
        }

        batchSize = capsuleVsCapsuleBatch.Data.size();
        if (batchSize > 0) {
            isColliding |= capsuleVsCapsuleAlgorithm.PerformCollisionCheck(capsuleVsCapsuleBatch, 0, batchSize);
        }

        batchSize = capsuleVsConvexPolyhedronBatch.Data.size();
        if (batchSize > 0) {
            isColliding |=
                capsuleVsConvexPolyhedronAlgorithm.PerformCollisionCheck(capsuleVsConvexPolyhedronBatch, 0, batchSize, clipWithPreviousAxisIfStillColliding);
        }

        batchSize = convexPolyhedronVsConvexPolyhedronBatch.Data.size();
        if (batchSize > 0) {
            isColliding |= convexPolyhedronVsConvexPolyhedronAlgorithm.PerformCollisionCheck(
                convexPolyhedronVsConvexPolyhedronBatch, 0, batchSize, clipWithPreviousAxisIfStillColliding);
        }

        return isColliding;
    }

    void CollisionSystem::computeConvexVsConcaveMiddlePhase(ConcaveOverlappingPair &overlappingPair, NarrowPhaseInput &batches, bool reportContacts) {
    }

    void CollisionSystem::swapPreviousAndCurrentContacts() {
        if (_previousContactPairs == &_contactPairsOne) {
            _previousContactPairs = &_contactPairsTwo;
            _currentContactPairs = &_contactPairsOne;

            _previousContactManifolds = &_contactManifoldsTwo;
            _currentContactManifolds = &_contactManifoldsOne;

            _previousContactPoints = &_contactPointsTwo;
            _currentContactPoints = &_contactPointsOne;
        } else {
            _previousContactPairs = &_contactPairsOne;
            _currentContactPairs = &_contactPairsTwo;

            _previousContactManifolds = &_contactManifoldsOne;
            _currentContactManifolds = &_contactManifoldsTwo;

            _previousContactPoints = &_contactPointsOne;
            _currentContactPoints = &_contactPointsTwo;
        }
    }

    void CollisionSystem::processPotentialContacts(NarrowPhaseDataBatch &batch,
                                                   bool updateLastFrameInfo,
                                                   std::vector<ContactPointData> &potentialContactPoints,
                                                   std::vector<ContactManifoldData> &potentialContactManifolds,
                                                   std::unordered_map<u64, u32> &mapPairIdToContactPairIndex,
                                                   std::vector<ContactPair> &contactPairs) {
    }

    void CollisionSystem::processAllPotentialContacts(NarrowPhaseInput &batches,
                                                      bool updateLastFrameInfo,
                                                      std::vector<ContactPointData> &potentialContactPoints,
                                                      std::vector<ContactManifoldData> &potentialContactManifolds,
                                                      std::vector<ContactPair> &contactPairs) {

        VASSERT(contactPairs.size() == 0, "Contact pairs list should be empty at the start of processing potential contacts.");

        std::unordered_map<u64, u32> pairIdToContactPairIndexMap;
        pairIdToContactPairIndexMap.reserve(_previousPairIDToContactPairIndexMap.size());

        NarrowPhaseDataBatch &sphereVsSphereBatch = batches.GetSphereVsSphereBatch();
        NarrowPhaseDataBatch &sphereVsCapsuleBatch = batches.GetSphereVsCapsuleBatch();
        NarrowPhaseDataBatch &sphereVsConvexPolyhedronBatch = batches.GetSphereVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &capsuleVsCapsuleBatch = batches.GetCapsuleVsCapsuleBatch();
        NarrowPhaseDataBatch &capsuleVsConvexPolyhedronBatch = batches.GetCapsuleVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &convexPolyhedronVsConvexPolyhedronBatch = batches.GetConvexPolyhedronVsConvexPolyhedronBatch();

        processPotentialContacts(
            sphereVsSphereBatch, updateLastFrameInfo, potentialContactPoints, potentialContactManifolds, pairIdToContactPairIndexMap, contactPairs);

        processPotentialContacts(
            sphereVsCapsuleBatch, updateLastFrameInfo, potentialContactPoints, potentialContactManifolds, pairIdToContactPairIndexMap, contactPairs);

        processPotentialContacts(
            sphereVsConvexPolyhedronBatch, updateLastFrameInfo, potentialContactPoints, potentialContactManifolds, pairIdToContactPairIndexMap, contactPairs);

        processPotentialContacts(
            capsuleVsCapsuleBatch, updateLastFrameInfo, potentialContactPoints, potentialContactManifolds, pairIdToContactPairIndexMap, contactPairs);

        processPotentialContacts(
            capsuleVsConvexPolyhedronBatch, updateLastFrameInfo, potentialContactPoints, potentialContactManifolds, pairIdToContactPairIndexMap, contactPairs);

        processPotentialContacts(convexPolyhedronVsConvexPolyhedronBatch,
                                 updateLastFrameInfo,
                                 potentialContactPoints,
                                 potentialContactManifolds,
                                 pairIdToContactPairIndexMap,
                                 contactPairs);
    }

    void CollisionSystem::reducePotentialContactManifolds(std::vector<ContactPair> *contactPairs,
                                                          std::vector<ContactManifoldData> &potentialContactManifolds,
                                                          const std::vector<ContactPointData> &potentialContactPoints) const {
    }

    void CollisionSystem::createContacts() {
    }

    void CollisionSystem::addContactPairsToBodies() {
    }

    void CollisionSystem::computeMapPreviousContactPairs() {
    }

    void CollisionSystem::computeLostContactPairs() {
    }

    void CollisionSystem::createSnapshotContacts(std::vector<ContactPair> &contactPairs,
                                                 std::vector<ContactManifold> &contactManifolds,
                                                 std::vector<ContactPoint> &contactPoints,
                                                 std::vector<ContactManifoldData> &potentialContactManifolds,
                                                 std::vector<ContactPointData> &potentialContactPoints) {
    }

    void CollisionSystem::initContactsWithPreviousOnes() {
    }

    void CollisionSystem::reduceContactPoints(ContactManifoldData &manifold,
                                              const TransformComponent &shape1ToWorldTransform,
                                              const std::vector<ContactPointData> &potentialContactPoints) const {
    }

    void CollisionSystem::reportContacts(CollisionCallback &callback,
                                         std::vector<ContactPair> &contactPairs,
                                         std::vector<ContactManifold> &manifolds,
                                         std::vector<ContactPoint> &contactPoints,
                                         std::vector<ContactPair> &lostContactPairs) {
    }

    void CollisionSystem::reportDebugRenderingContacts(std::vector<ContactPair> *contactPairs,
                                                       std::vector<ContactManifold> *manifolds,
                                                       std::vector<ContactPoint> *contactPoints,
                                                       std::vector<ContactPair> &lostContactPairs) {
    }

    f32 CollisionSystem::computePotentialManifoldLargestContactDepth(const ContactManifoldData &manifold,
                                                                     const std::vector<ContactPointData> &potentialContactPoints) const {
    }

    void CollisionSystem::processSmoothMeshContacts(OverlappingPair *pair) {
    }

    void CollisionSystem::filterOverlappingPairs(Entity bodyEntity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const {
    }

    void CollisionSystem::filterOverlappingPairs(Entity body1Entity, Entity body2Entity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const {
    }

    void CollisionSystem::removeItemAtInArray(u32 array[], u8 index, u8 &arraySize) const {
    }

    void CollisionSystem::removeDuplicatedContactPointsInManifold(ContactManifoldData &manifold,
                                                                  const std::vector<ContactPointData> &potentialContactPoints) const {
    }

} // namespace Vulkyrie
