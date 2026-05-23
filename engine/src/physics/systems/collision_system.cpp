#include "physics/systems/collision_system.h"
#include "physics/physics_world.h"
#include "physics/body/body.h"
#include "vlkypch.h"

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

        // Remove all overlapping pairs that involve this collider.
        // This is necessary to ensure that any pairs involving this collider
        // are removed from the overlapping pair list and will not generate
        // collision callbacks after the collider is removed from the broad phase system.
        while (overlappingPairs.size() > 0) {
            removeOverlappingPair(overlappingPairs[0], false);
        }

        // Remove the broad-phase ID to collider entity mapping for this collider.
        _broadPhaseIDToColliderEntityMap.erase(broadPhaseID);

        // Remove the collider from the broad-phase system,
        // which will also remove it from the AABB tree and the set of moved colliders.
        _broadPhaseSystem.RemoveCollider(collider);
    }

    void CollisionSystem::AddNonCollidablePair(Entity bodyOneEntity, Entity bodyTwoEntity) {
        _nonCollidablePairs.emplace(OverlappingPairs::ComputeBodiesIndexPair(bodyOneEntity, bodyTwoEntity));

        std::vector<u64> toBeRemoved;
        const std::vector<Entity> &colliderEntities = _physicsWorld.GetBodyComponentStore().GetColliders(bodyOneEntity);

        // Iterate through the colliders of the first body and check their
        // overlapping pairs to find any pairs that involve the second body.
        // If any such pairs are found, they should be removed from the overlapping pairs list,
        // else they will continue to generate collision callbacks even though the bodies are now non-collidable.
        for (const Entity colliderEntity : colliderEntities) {
            const std::vector<u64> &overlappingPairs = _colliderComponentStore.GetOverlappingPairs(colliderEntity);

            for (const u64 pairID : overlappingPairs) {
                const OverlappingPair *pair = _overlappingPairs.GetOverlappingPair(pairID);

                VASSERT(nullptr != pair, "Overlapping pair ID in the overlapping pairs of a collider does not exist in the overlapping pairs manager.");

                const Entity overlappingBodyOneEntity = _colliderComponentStore.GetBodyEntity(pair->ColliderOneEntity);
                const Entity overlappingBodyTwoEntity = _colliderComponentStore.GetBodyEntity(pair->ColliderTwoEntity);

                if (overlappingBodyOneEntity == bodyTwoEntity || overlappingBodyTwoEntity == bodyTwoEntity) {
                    toBeRemoved.push_back(pairID);
                }
            }
        }

        // Remove the overlapping pairs that needs to be removed.
        for (const u64 pairID : toBeRemoved) {
            removeOverlappingPair(pairID, true);
        }
    }

    void CollisionSystem::NotifyOverlappingPairsToTestOverlap(Collider &collider) {
        const std::vector<u64> &overlappingPairs = _colliderComponentStore.GetOverlappingPairs(collider.GetEntity());

        for (const auto overlappingPair : overlappingPairs) {
            // Notify that the overlapping pair needs to be testbed for overlap
            _overlappingPairs.SetRequiresCollisionCheck(overlappingPair, true);
        }
    }

    void CollisionSystem::ReportContactsAndTriggers() {
        // Report contacts and triggers to the user.
        EventListener *eventListener = _physicsWorld.GetEventListener();

        if (nullptr != eventListener) {
            reportContacts(*(eventListener), _currentContactPairs, _currentContactManifolds, _currentContactPoints, _lostContactPairs);
            reportTriggers(*(eventListener), _currentContactPairs, _lostContactPairs);
        }

        // Report contacts for debug rendering (if enabled)
        if (_physicsWorld.IsDebugRenderingEnabled()) {
            reportDebugRenderingContacts(_currentContactPairs, _currentContactManifolds, _currentContactPoints, _lostContactPairs);
        }

        _overlappingPairs.UpdateCollidingInLastFrame();
        _lostContactPairs.clear();
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

    // void CollisionSystem::TestOverlap(Body &bodyOne, Body &bodyTwo) {
    //     // Compute broad-phase collision detection to generate potential collision pairs.
    //     computeBroadPhase();
    //
    //     std::vector<u64> concavePairs;
    //     std::vector<u64> convexPairs;
    //
    //     // Filter the overlapping pairs to only get the ones that involve the specified bodies,
    //     // and separate them into convex and concave pairs for more efficient processing in the middle phase.
    //     filterOverlappingPairs(bodyOne.GetEntity(), bodyTwo.GetEntity(), convexPairs, concavePairs);
    //
    //     if (convexPairs.size() > 0 || concavePairs.size() > 0) {
    //         // Compute middle-phase collision detection.
    //         computeMiddlePhaseCollisionSnapshot(convexPairs, concavePairs, _narrowPhaseInput, false);
    //
    //         // Compute narrow-phase collision detection.
    //         computeNarrowPhaseOverlapSnapshot(_narrowPhaseInput, nullptr);
    //     }
    // }
    //
    // void CollisionSystem::TestOverlap(Body &body, OverlapCallback &callback) {
    // }
    //
    // void CollisionSystem::TestOverlap(OverlapCallback &callback) {
    //     NarrowPhaseInput batches(_);
    // }

    void CollisionSystem::TestCollision(Body &bodyOne, Body &bodyTwo, CollisionCallback &callback) {
        NarrowPhaseInput narrowPhaseInput;

        // Compute broad-phase collision detection.
        computeBroadPhase();

        // Filter overlapping pairs to get only the ones with the selected bodies involved.
        std::vector<u64> convexPairs;
        std::vector<u64> concavePairs;

        filterOverlappingPairs(bodyOne.GetEntity(), bodyTwo.GetEntity(), convexPairs, concavePairs);

        if (convexPairs.size() > 0 || concavePairs.size() > 0) {
            // Compute middle-phase collision detection.
            computeMiddlePhaseCollisionSnapshot(convexPairs, concavePairs, narrowPhaseInput, true);

            // Compute narrow-phase collision detection and report contacts.
            computeNarrowPhaseCollisionSnapshot(narrowPhaseInput, callback);
        }
    }

    void CollisionSystem::TestCollision(Body &body, CollisionCallback &callback) {
        NarrowPhaseInput narrowPhaseInput;

        // Compute the broad-phase collision detection.
        computeBroadPhase();

        // Filter the overlapping pairs to get only the ones with the selected body involved.
        std::vector<u64> convexPairs;
        std::vector<u64> concavePairs;

        filterOverlappingPairs(body.GetEntity(), convexPairs, concavePairs);

        if (convexPairs.size() > 0 || concavePairs.size() > 0) {
            // Compute the middle-phase collision detection.
            computeMiddlePhaseCollisionSnapshot(convexPairs, concavePairs, narrowPhaseInput, true);

            // Compute the narrow-phase collision detection and report contacts.
            computeNarrowPhaseCollisionSnapshot(narrowPhaseInput, callback);
        }
    }

    void CollisionSystem::TestCollision(CollisionCallback &callback) {
        NarrowPhaseInput narrowPhaseInput;

        // Compute the broad-phase collision detection.
        computeBroadPhase();

        // Compute the middle-phase collision detection.
        computeMiddlePhase(narrowPhaseInput, true, true);

        // Compute the narrow-phase collision detection and report contacts.
        computeNarrowPhaseCollisionSnapshot(narrowPhaseInput, callback);
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

    void CollisionSystem::computeMiddlePhase(NarrowPhaseInput &batches, bool reportContacts, bool isWorldQuery) {
    }

    void CollisionSystem::computeMiddlePhaseCollisionSnapshot(std::vector<u64> &convexPairs,
                                                              std::vector<u64> &concavePairs,
                                                              NarrowPhaseInput &batches,
                                                              bool reportContacts) {
    }

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

    bool CollisionSystem::computeNarrowPhaseOverlapSnapshot(NarrowPhaseInput &batches, OverlapCallback *callback) {
        // Perform narrow-phase collision detection to determine if the colliders
        // in the batches are overlapping, without generating contact points or manifolds.
        bool isColliding = testNarrowPhaseCollision(batches, false);

        if (isColliding && nullptr != callback) {
            std::vector<ContactPair> contactPairs;
            std::vector<ContactPair> lostContactPairs;

            // Compute the overlapping colliders
            computeOverlapSnapshotContactPairs(batches, contactPairs);

            // Report overlapping colliders
            OverlapCallback::Data callbackData(contactPairs, lostContactPairs, false);
            (*callback).OnOverlap(callbackData);
        }

        return isColliding;
    }

    bool CollisionSystem::computeNarrowPhaseCollisionSnapshot(NarrowPhaseInput &batches, CollisionCallback &callback) {
        bool isColliding = testNarrowPhaseCollision(batches, false);

        if (isColliding) {
            std::vector<ContactPointData> potentialContactPoints;
            std::vector<ContactManifoldData> potentialContactManifolds;
            std::vector<ContactPair> contactPairs;
            std::vector<ContactPair> lostContactPairs;
            std::vector<ContactManifold> contactManifolds;
            std::vector<ContactPoint> contactPoints;

            // Process all the potential contacts after narrow-phase collision.
            processAllPotentialContacts(batches, true, potentialContactPoints, potentialContactManifolds, contactPairs);

            // Reduce the number of contact points in the manifolds.
            reducePotentialContactManifolds(contactPairs, potentialContactManifolds, potentialContactPoints);

            // Create the actual contact manifolds and contact points.
            createSnapshotContacts(contactPairs, contactManifolds, contactPoints, potentialContactManifolds, potentialContactPoints);

            // Report the contacts to the client through the callback concept.
            reportContacts(callback, contactPairs, contactManifolds, contactPoints, lostContactPairs);
        }

        return isColliding;
    }

    void CollisionSystem::computeOverlapSnapshotContactPairs(NarrowPhaseInput &batches, std::vector<ContactPair> &contactPair) {
        std::unordered_set<u64> overlappingContactPairIDs;

        // Get the narrow phase data batches to test for collision.
        NarrowPhaseDataBatch &sphereVsSphereBatch = batches.GetSphereVsSphereBatch();
        NarrowPhaseDataBatch &sphereVsCapsuleBatch = batches.GetSphereVsCapsuleBatch();
        NarrowPhaseDataBatch &sphereVsConvexPolyhedronBatch = batches.GetSphereVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &capsuleVsCapsuleBatch = batches.GetCapsuleVsCapsuleBatch();
        NarrowPhaseDataBatch &capsuleVsConvexPolyhedronBatch = batches.GetCapsuleVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &convexPolyhedronVsConvexPolyhedronBatch = batches.GetConvexPolyhedronVsConvexPolyhedronBatch();

        // Process each narrow-phase data batch to compute contact pairs for the current overlap snapshot.
        // This will involve testing each pair of colliders in the batch for overlap,
        computeOverlapSnapshotContactPairs(sphereVsSphereBatch, contactPair, overlappingContactPairIDs);
        computeOverlapSnapshotContactPairs(sphereVsCapsuleBatch, contactPair, overlappingContactPairIDs);
        computeOverlapSnapshotContactPairs(sphereVsConvexPolyhedronBatch, contactPair, overlappingContactPairIDs);
        computeOverlapSnapshotContactPairs(capsuleVsCapsuleBatch, contactPair, overlappingContactPairIDs);
        computeOverlapSnapshotContactPairs(capsuleVsConvexPolyhedronBatch, contactPair, overlappingContactPairIDs);
        computeOverlapSnapshotContactPairs(convexPolyhedronVsConvexPolyhedronBatch, contactPair, overlappingContactPairIDs);
    }

    void CollisionSystem::computeOverlapSnapshotContactPairs(NarrowPhaseDataBatch &batch,
                                                             std::vector<ContactPair> &contactPairs,
                                                             std::unordered_set<u64> overlappingContactPairIDs) const {
        // For each narrow-phase data entry in the batch, check if the colliders are overlapping and if so,
        // create a ContactPair for this overlapping pair if it hasn't already been created for this pair ID.
        for (size_t i = 0; i < batch.Data.size(); i++) {
            const NarrowPhaseData &data = batch.Data[i];

            if (data.IsColliding && !overlappingContactPairIDs.contains(data.OverlappingPairID)) {
                const Entity colliderOneEntity = data.ColliderOneEntity;
                const Entity colliderTwoEntity = data.ColliderTwoEntity;

                const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
                const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

                const Entity bodyOneEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderOneIndex);
                const Entity bodyTwoEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderTwoIndex);

                const bool isTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex) || _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);
                const u32 contactPairIndex = static_cast<u32>(contactPairs.size());

                contactPairs.emplace_back(
                    data.OverlappingPairID, bodyOneEntity, bodyTwoEntity, colliderOneEntity, colliderTwoEntity, contactPairIndex, false, isTrigger);

                overlappingContactPairIDs.insert(data.OverlappingPairID);
            }

            batch.ResetContactPoints(i);
        }
    }

    void CollisionSystem::updateOverlappingPairs(const std::vector<Pair<i32, i32>> &overlappingNodes) {
    }

    void CollisionSystem::removeNonOverlappingPairs() {
        // Iterate through the active convex overlapping pairs and remove any pairs that no longer overlap according to the broad-phase system.
        for (size_t i = 0; i < _overlappingPairs._convexPairs.size(); ++i) {
            ConvexOverlappingPair &pair = _overlappingPairs._convexPairs[i];

            if (pair.RequiresCollisionCheck) {
                if (_broadPhaseSystem.TestOverlap(pair.ColliderOneBroadPhaseID, pair.ColliderTwoBroadPhaseID)) {
                    pair.RequiresCollisionCheck = false;
                } else {
                    removeConvexOverlappingPairWithIndex(i);
                    --i; // Decrement index to account for the removed pair and the shifted elements in the vector.
                }
            }
        }

        // Iterate through the active concave overlapping pairs and remove any pairs that no longer overlap according to the broad-phase system.
        for (size_t i = 0; i < _overlappingPairs._concavePairs.size(); ++i) {
            ConcaveOverlappingPair &pair = _overlappingPairs._concavePairs[i];

            if (pair.RequiresCollisionCheck) {
                if (_broadPhaseSystem.TestOverlap(pair.ColliderOneBroadPhaseID, pair.ColliderTwoBroadPhaseID)) {
                    pair.RequiresCollisionCheck = false;
                } else {
                    removeConcaveOverlappingPairWithIndex(i);
                    --i; // Decrement index to account for the removed pair and the shifted elements in the vector.
                }
            }
        }
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

        // Get references to the narrow-phase algorithms from the collision dispatcher for each pair of shape types.
        SphereVsSphereAlgorithm &sphereVsSphereAlgorithm = _collisionDispatch.GetSphereVsSphereAlgorithm();
        SphereVsCapsuleAlgorithm &sphereVsCapsuleAlgorithm = _collisionDispatch.GetSphereVsCapsuleAlgorithm();
        SphereVsConvexPolyhedronAlgorithm &sphereVsConvexPolyhedronAlgorithm = _collisionDispatch.GetSphereVsConvexPolyhedronAlgorithm();
        CapsuleVsCapsuleAlgorithm &capsuleVsCapsuleAlgorithm = _collisionDispatch.GetCapsuleVsCapsuleAlgorithm();
        CapsuleVsConvexPolyhedronAlgorithm &capsuleVsConvexPolyhedronAlgorithm = _collisionDispatch.GetCapsuleVsConvexPolyhedronAlgorithm();
        ConvexPolyhedronVsConvexPolyhedronAlgorithm &convexPolyhedronVsConvexPolyhedronAlgorithm =
            _collisionDispatch.GetConvexPolyhedronVsConvexPolyhedronAlgorithm();

        // Get the narrow phase data batches to test for collision.
        NarrowPhaseDataBatch &sphereVsSphereBatch = batches.GetSphereVsSphereBatch();
        NarrowPhaseDataBatch &sphereVsCapsuleBatch = batches.GetSphereVsCapsuleBatch();
        NarrowPhaseDataBatch &sphereVsConvexPolyhedronBatch = batches.GetSphereVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &capsuleVsCapsuleBatch = batches.GetCapsuleVsCapsuleBatch();
        NarrowPhaseDataBatch &capsuleVsConvexPolyhedronBatch = batches.GetCapsuleVsConvexPolyhedronBatch();
        NarrowPhaseDataBatch &convexPolyhedronVsConvexPolyhedronBatch = batches.GetConvexPolyhedronVsConvexPolyhedronBatch();

        // For each narrow-phase data batch, if there are any pairs to test in the batch,
        // perform the collision check using the corresponding narrow-phase algorithm.
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

    void CollisionSystem::reducePotentialContactManifolds(std::vector<ContactPair> &contactPairs,
                                                          std::vector<ContactManifoldData> &potentialContactManifolds,
                                                          const std::vector<ContactPointData> &potentialContactPoints) const {
        // For each contact pair, if the number of potential contact manifolds exceeds the maximum allowed, we can perform a reduction step to remove some of
        // the manifolds based on their depth, which can help to improve the performance of the constraint solver while still maintaining good collision
        // response quality. The depth of a contact manifold can be computed as the largest penetration depth among its contact points, which gives us an
        // indication of how significant the collision is for that manifold. By removing the manifolds with the smallest depth, we can focus the constraint
        // solver on the most significant collisions while still maintaining good overall collision response quality.
        for (ContactPair &pair : contactPairs) {
            while (pair.PotentialContactManifoldsCount > MAX_CONTACT_MANIFOLDS) {
                f32 minDepth = std::numeric_limits<f32>::max();
                u32 minDepthManifoldIndex = -1;

                // Iterate through the potential contact manifolds for this pair to find the one
                // with the smallest depth, which will be the candidate for removal in this reduction step.
                for (u32 j = 0; j < pair.PotentialContactManifoldsCount; ++j) {
                    ContactManifoldData &manifoldData = potentialContactManifolds[pair.PotentialContactManifoldIndices[j]];

                    VASSERT(manifoldData.TotalPotentialContactPoints > 0,
                            "Contact manifold data should have at least one potential contact point when trying to reduce potential contact manifolds.");

                    // Compute the depth of this contact manifold as the largest penetration depth among its contact points.
                    const f32 depth = computePotentialManifoldLargestContactDepth(manifoldData, potentialContactPoints);

                    // If this manifold has a smaller depth than the current minimum, update
                    // the minimum depth and the index of the manifold with the minimum depth.
                    if (depth < minDepth) {
                        minDepth = depth;
                        minDepthManifoldIndex = j;
                    }
                }

                VASSERT(minDepthManifoldIndex >= 0, "Minimum depth manifold index should be valid when trying to reduce potential contact manifolds.");

                // Remove the potential contact manifold with the smallest depth from this contact pair.
                pair.RemovePotentialManifoldAtIndex(minDepthManifoldIndex);
            }
        }

        // After reducing the number of potential contact manifolds for each contact pair,
        // we can also perform an additional step to remove any duplicated contact points
        // in the remaining manifolds, which can further improve the performance
        // of the constraint solver without sacrificing collision response quality.
        for (ContactPair &pair : contactPairs) {
            for (u32 i = 0; i < pair.PotentialContactManifoldsCount; ++i) {
                ContactManifoldData &manifoldData = potentialContactManifolds[pair.PotentialContactManifoldIndices[i]];

                VASSERT(manifoldData.TotalPotentialContactPoints > 0,
                        "Contact manifold data should have at least one potential contact point when trying to reduce potential contact manifolds.");

                // If the number of potential contact points in this manifold exceeds the maximum allowed,
                // we can perform a reduction step to remove duplicated contact points based on their positions,
                // which can help to further improve the performance of the constraint solver while still maintaining good collision response quality.
                if (manifoldData.TotalPotentialContactPoints > MAX_CONTACT_POINTS_IN_MANIFOLD) {
                    TransformComponent shapeOneLocalToWorldTransform = _colliderComponentStore.GetLocalToWorldTransform(pair.ColliderOneEntity);

                    reduceContactPoints(manifoldData, shapeOneLocalToWorldTransform, potentialContactPoints);
                }

                VASSERT(manifoldData.TotalPotentialContactPoints <= MAX_CONTACT_POINTS_IN_MANIFOLD,
                        "Contact manifold data should have less than or equal to the maximum contact points in manifold after trying to reduce potential "
                        "contact manifolds.");

                // Remove the duplicated contact points in the manifold (if any)
                removeDuplicatedContactPointsInManifold(manifoldData, potentialContactPoints);
            }
        }
    }

    void CollisionSystem::createContacts() {
    }

    void CollisionSystem::addContactPairsToBodies() {
        // TODO: See if we can use size_t instead of u32.
        for (u32 i = 0; i < static_cast<u32>(_currentContactPairs->size()); ++i) {
            const ContactPair &pair = (*_currentContactPairs)[i];

            // Add the contact pair index to the contact pair list of both bodies involved in this contact pair.
            // This allows us to quickly access all contact pairs associated with a body during constraint solving.
            _rigidBodyComponentStore.AddContactPair(pair.BodyOneEntity, i);
            _rigidBodyComponentStore.AddContactPair(pair.BodyTwoEntity, i);
        }
    }

    void CollisionSystem::computeMapPreviousContactPairs() {
        // Clear the map from the previous frame's contact pair IDs to their indices in the previous contact pairs list,
        // as we will rebuild this map based on the current contact pairs after narrow-phase collision detection.
        _previousPairIDToContactPairIndexMap.clear();

        // Build a map from contact pair ID to its index in the previous contact pairs list for quick lookup during narrow-phase collision detection.
        for (u32 i = 0; i < static_cast<u32>(_currentContactPairs->size()); ++i) {
            _previousPairIDToContactPairIndexMap.emplace((*_currentContactPairs)[i].PairID, i);
        }
    }

    void CollisionSystem::computeLostContactPairs() {
        // For each convex overlapping pair,
        // check if they were colliding in the last frame but are no longer colliding in the current frame.
        // If so, add them to the list of lost contact pairs.
        for (size_t i = 0; i < _overlappingPairs._convexPairs.size(); ++i) {
            const ConvexOverlappingPair &pair = _overlappingPairs._convexPairs[i];

            if (pair.WereCollidingLastFrame && !pair.AreCollidingThisFrame) {

                VASSERT(_colliderComponentStore.HasComponent(pair.ColliderOneEntity),
                        "Collider one entity in convex pair should exist in the collider component store when computing lost contact pairs.");
                VASSERT(_colliderComponentStore.HasComponent(pair.ColliderTwoEntity),
                        "Collider two entity in convex pair should exist in the collider component store when computing lost contact pairs.");

                addLostContactPair(const_cast<ConvexOverlappingPair &>(pair));
            }
        }

        // For each concave overlapping pair,
        // check if they were colliding in the last frame but are no longer colliding in the current frame.
        // If so, add them to the list of lost contact pairs.
        for (size_t i = 0; i < _overlappingPairs._concavePairs.size(); ++i) {
            const ConcaveOverlappingPair &pair = _overlappingPairs._concavePairs[i];

            if (pair.WereCollidingLastFrame && !pair.AreCollidingThisFrame) {

                VASSERT(_colliderComponentStore.HasComponent(pair.ColliderOneEntity),
                        "Collider one entity in concave pair should exist in the collider component store when computing lost contact pairs.");
                VASSERT(_colliderComponentStore.HasComponent(pair.ColliderTwoEntity),
                        "Collider two entity in concave pair should exist in the collider component store when computing lost contact pairs.");

                addLostContactPair(const_cast<ConcaveOverlappingPair &>(pair));
            }
        }
    }

    void CollisionSystem::createSnapshotContacts(std::vector<ContactPair> &contactPairs,
                                                 std::vector<ContactManifold> &contactManifolds,
                                                 std::vector<ContactPoint> &contactPoints,
                                                 std::vector<ContactManifoldData> &potentialContactManifolds,
                                                 std::vector<ContactPointData> &potentialContactPoints) {
        contactManifolds.reserve(contactPairs.size());
        contactPoints.reserve(contactManifolds.size());

        // Iterate through the contact pairs and create contact manifolds and contact points for each pair
        // based on the potential contact manifolds and points generated during narrow-phase collision detection.
        for (ContactPair &pair : contactPairs) {
            VASSERT(pair.ContactManifoldCount > 0, "Contact pair should have at least one contact manifold when trying to create snapshot contacts.");

            pair.ContactManifoldIndex = static_cast<u32>(contactManifolds.size());
            pair.ContactManifoldCount = pair.PotentialContactManifoldsCount;
            pair.ContactPointIndex = static_cast<u32>(contactPoints.size());

            // Iterate through the potential contact manifolds for this pair and create contact manifolds and contact points for each one.
            for (u32 i = 0; i < pair.PotentialContactManifoldsCount; ++i) {
                ContactManifoldData &manifoldData = potentialContactManifolds[pair.PotentialContactManifoldIndices[i]];

                VASSERT(manifoldData.TotalPotentialContactPoints > 0,
                        "Contact manifold data should have at least one potential contact point when trying to create snapshot contacts.");

                const u32 contactPointIndex = static_cast<u32>(contactPoints.size());
                const u8 contactPointCount = static_cast<u8>(manifoldData.TotalPotentialContactPoints);

                pair.ContactPointCount += contactPointCount;

                contactManifolds.emplace_back(
                    pair.BodyOneEntity, pair.BodyTwoEntity, pair.ColliderOneEntity, pair.ColliderTwoEntity, contactPointIndex, contactPointCount);

                VASSERT(manifoldData.TotalPotentialContactPoints > 0,
                        "Contact manifold data should have at least one potential contact point when trying to create snapshot contacts.");

                // Iterate through the potential contact points for this manifold and create contact points for each one.
                for (u32 j = 0; j < manifoldData.TotalPotentialContactPoints; ++j) {
                    const ContactPointData &pointData = potentialContactPoints[manifoldData.PotentialContactPointsIndices[j]];

                    // Add the contact point to the list.
                    contactPoints.emplace_back(pointData);
                }
            }
        }
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
        // Report contacts if there are any contact pairs or lost contact pairs to report.
        if (contactPairs.size() + lostContactPairs.size() > 0) {
            CollisionCallback::Data callbackData(contactPairs, manifolds, contactPoints, lostContactPairs);

            callback.OnCollision(callbackData);
        }
    }

    void CollisionSystem::reportTriggers(EventListener &eventListener, std::vector<ContactPair> *contactPairs, std::vector<ContactPair> &lostContactPairs) {
        // Report trigger events if there are any contact pairs or lost contact pairs to report.
        if (contactPairs->size() + lostContactPairs.size() > 0) {
            OverlapCallback::Data callbackData(*contactPairs, lostContactPairs, true, *mWorld);
            eventListener.OnTrigger(callbackData);
        }
    }

    void CollisionSystem::reportDebugRenderingContacts(std::vector<ContactPair> *contactPairs,
                                                       std::vector<ContactManifold> *manifolds,
                                                       std::vector<ContactPoint> *contactPoints,
                                                       std::vector<ContactPair> &lostContactPairs) {
        // Report contacts for debug rendering if there are any contact pairs or lost contact pairs to report.
        if (contactPairs->size() + lostContactPairs.size() > 0) {
            CollisionCallback::Data callbackData(contactPairs, manifolds, contactPoints, lostContactPairs, *mWorld);
            mWorld->mDebugRenderer.onContact(callbackData);
        }
    }

    f32 CollisionSystem::computePotentialManifoldLargestContactDepth(const ContactManifoldData &manifold,
                                                                     const std::vector<ContactPointData> &potentialContactPoints) const {
        f32 largestDepth = 0.0f;

        VASSERT(
            manifold.TotalPotentialContactPoints > 0,
            "Manifold should have at least one potential contact point when trying to compute the largest contact depth among the potential contact points.");

        // Iterate through the potential contact points in the manifold and find the largest penetration depth among them.
        // This can be used as a heuristic for determining the quality of the contact manifold
        // and for deciding whether to keep or discard it during manifold reduction.
        for (u32 i = 0; i < manifold.TotalPotentialContactPoints; ++i) {
            const ContactPointData &point = potentialContactPoints[manifold.PotentialContactPointsIndices[i]];

            if (point.PenetrationDepth > largestDepth) {
                largestDepth = point.PenetrationDepth;
            }
        }

        return largestDepth;
    }

    void CollisionSystem::filterOverlappingPairs(Entity bodyEntity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const {
        // Iterate through the active convex overlapping pairs and add the pair IDs
        // of any pairs that involve the specified body entity to the convexPairs list.
        for (const auto &pair : _overlappingPairs._convexPairs) {
            if (_colliderComponentStore.GetBodyEntity(pair.ColliderOneEntity) == bodyEntity ||
                _colliderComponentStore.GetBodyEntity(pair.ColliderTwoEntity) == bodyEntity) {
                convexPairs.push_back(pair.PairID);
            }
        }

        // Iterate through the active concave overlapping pairs and add the pair IDs
        // of any pairs that involve the specified body entity to the concavePairs list.
        for (const auto &pair : _overlappingPairs._concavePairs) {
            if (_colliderComponentStore.GetBodyEntity(pair.ColliderOneEntity) == bodyEntity ||
                _colliderComponentStore.GetBodyEntity(pair.ColliderTwoEntity) == bodyEntity) {
                concavePairs.push_back(pair.PairID);
            }
        }
    }

    void
    CollisionSystem::filterOverlappingPairs(Entity bodyOneEntity, Entity bodyTwoEntity, std::vector<u64> &convexPairs, std::vector<u64> &concavePairs) const {
        // Iterate through the active convex overlapping pairs and add the pair IDs
        // of any pairs that involve both specified body entities to the convexPairs list.
        for (const auto &pair : _overlappingPairs._convexPairs) {
            const Entity colliderOneBodyEntity = _colliderComponentStore.GetBodyEntity(pair.ColliderOneEntity);
            const Entity colliderTwoBodyEntity = _colliderComponentStore.GetBodyEntity(pair.ColliderTwoEntity);

            if ((colliderOneBodyEntity == bodyOneEntity && colliderTwoBodyEntity == bodyTwoEntity) ||
                (colliderOneBodyEntity == bodyTwoEntity && colliderTwoBodyEntity == bodyOneEntity)) {
                convexPairs.push_back(pair.PairID);
            }
        }

        // Iterate through the active concave overlapping pairs and add the pair IDs
        // of any pairs that involve both specified body entities to the concavePairs list.
        for (const auto &pair : _overlappingPairs._concavePairs) {
            const Entity colliderOneBodyEntity = _colliderComponentStore.GetBodyEntity(pair.ColliderOneEntity);
            const Entity colliderTwoBodyEntity = _colliderComponentStore.GetBodyEntity(pair.ColliderTwoEntity);

            if ((colliderOneBodyEntity == bodyOneEntity && colliderTwoBodyEntity == bodyTwoEntity) ||
                (colliderOneBodyEntity == bodyTwoEntity && colliderTwoBodyEntity == bodyOneEntity)) {
                concavePairs.push_back(pair.PairID);
            }
        }
    }

    void CollisionSystem::removeItemAtInArray(u32 array[], u8 index, u8 &arraySize) const {
        VASSERT(index < arraySize, "Index to remove should be within the bounds of the array size.");
        VASSERT(arraySize > 0, "Array size should be greater than zero when trying to remove an item from the array.");

        array[index] = array[arraySize - 1];
        arraySize--;
    }

    void CollisionSystem::removeDuplicatedContactPointsInManifold(ContactManifoldData &manifold,
                                                                  const std::vector<ContactPointData> &potentialContactPoints) const {
        VASSERT(manifold.TotalPotentialContactPoints > 0,
                "Manifold should have at least one potential contact point when trying to remove duplicated contact points.");

        constexpr f32 distanceThresholdSquared = SAME_CONTACT_POINT_DISTANCE_THRESHOLD * SAME_CONTACT_POINT_DISTANCE_THRESHOLD;

        // Iterate through the potential contact points in the manifold
        // and remove any points that are too close to each other,
        // as they are likely duplicates that can cause instability in the constraint solver.
        for (size_t i = 0; i < manifold.TotalPotentialContactPoints; ++i) {
            const ContactPointData &pointA = potentialContactPoints[manifold.PotentialContactPointsIndices[i]];

            for (size_t j = i + 1; j < manifold.TotalPotentialContactPoints; ++j) {
                const ContactPointData &pointB = potentialContactPoints[manifold.PotentialContactPointsIndices[j]];
                const f32 distanceSquared = glm::length2(pointA.LocalSpaceContactPointOnBodyOne - pointB.LocalSpaceContactPointOnBodyOne);

                if (distanceSquared < distanceThresholdSquared) {
                    manifold.PotentialContactPointsIndices[j] = manifold.PotentialContactPointsIndices[manifold.TotalPotentialContactPoints - 1];
                    --manifold.TotalPotentialContactPoints;
                    --j; // Decrement j to check the new point that was swapped into index j after removing the duplicate.
                }
            }
        }

        VASSERT(manifold.TotalPotentialContactPoints > 0, "Manifold should still have at least one potential contact point after removing duplicates.");
    }

} // namespace Vulkyrie
