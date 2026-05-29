#include "physics/systems/collision_system.h"
#include "physics/collision/shapes/concave_shape.h"
#include "physics/collision/shapes/convex_shape.h"
#include "physics/physics_world.h"
#include "physics/body/body.h"
#include <glm/matrix.hpp>

namespace Vulkyrie {

    CollisionSystem::CollisionSystem(PhysicsWorld &physicsWorld, HalfEdgeMesh &triangleHalfEdgeMesh)
        : _physicsWorld(physicsWorld)
        , _colliderComponentStore(_physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(_physicsWorld.GetRigidBodyComponentStore())
        , _collisionDispatch()
        , _overlappingPairs(physicsWorld, _nonCollidablePairs, _collisionDispatch)
        , _broadPhaseSystem(physicsWorld)
        , _narrowPhaseInput()
        , _triangleHalfEdgeMesh(triangleHalfEdgeMesh) {
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
            reportContacts(*(eventListener), *_currentContactPairs, *_currentContactManifolds, *_currentContactPoints, _lostContactPairs);
            reportTriggers(*(eventListener), _currentContactPairs, _lostContactPairs);
        }

        // TODO: Implement the following debug rendering of contacts and triggers, which can be enabled through a debug flag in the physics world.
        // Report contacts for debug rendering (if enabled)
        // if (_physicsWorld.IsDebugRenderingEnabled()) {
        //     reportDebugRenderingContacts(_currentContactPairs, _currentContactManifolds, _currentContactPoints, _lostContactPairs);
        // }

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

    void CollisionSystem::TestOverlap(Body &bodyOne, Body &bodyTwo) {
        // Compute broad-phase collision detection to generate potential collision pairs.
        computeBroadPhase();

        std::vector<u64> concavePairs;
        std::vector<u64> convexPairs;

        // Filter the overlapping pairs to only get the ones that involve the specified bodies,
        // and separate them into convex and concave pairs for more efficient processing in the middle phase.
        filterOverlappingPairs(bodyOne.GetEntity(), bodyTwo.GetEntity(), convexPairs, concavePairs);

        if (convexPairs.size() > 0 || concavePairs.size() > 0) {
            // Compute middle-phase collision detection.
            computeMiddlePhaseCollisionSnapshot(convexPairs, concavePairs, _narrowPhaseInput, false);

            // Compute narrow-phase collision detection.
            computeNarrowPhaseOverlapSnapshot(_narrowPhaseInput, nullptr);
        }
    }

    void CollisionSystem::TestOverlap(Body &body, OverlapCallback &callback) {
        NarrowPhaseInput narrowPhaseInput;

        // Compute broad-phase collision detection.
        computeBroadPhase();

        std::vector<u64> convexPairs;
        std::vector<u64> concavePairs;

        // Filter the overlapping pairs to get only the ones with the selected body involved.
        filterOverlappingPairs(body.GetEntity(), convexPairs, concavePairs);

        // If there are any overlapping pairs involving the specified body,
        // we need to perform middle-phase and narrow-phase collision detection to determine if
        // they are actually overlapping, and to report the overlapping pairs through the callback.
        if (convexPairs.size() > 0 || concavePairs.size() > 0) {

            // Compute the middle-phase collision detection
            computeMiddlePhaseCollisionSnapshot(convexPairs, concavePairs, narrowPhaseInput, false);

            // Compute the narrow-phase collision detection
            computeNarrowPhaseOverlapSnapshot(narrowPhaseInput, &callback);
        }
    }

    void CollisionSystem::TestOverlap(OverlapCallback &callback) {
        NarrowPhaseInput narrowPhaseInput;

        // Compute broad-phase collision detection.
        computeBroadPhase();

        // Compute middle-phase collision detection.
        computeMiddlePhase(narrowPhaseInput, false, true);

        // Compute narrow-phase collision detection and report overlapping shapes.
        computeNarrowPhaseOverlapSnapshot(narrowPhaseInput, &callback);
    }

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
        // Clear last frame collision data for all active pairs,
        // which will be used to determine if a pair is still colliding in the current frame during narrow-phase collision detection.
        _overlappingPairs.ClearObsoleteLastFrameCollisionData();

        // Store the count of active collider components.
        const size_t activeColliderComponents = _colliderComponentStore.GetActiveComponentCount();

        // Process the convex pairs to generate narrow-phase input batches for narrow-phase collision detection.
        for (auto &pair : _overlappingPairs._convexPairs) {
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != -1,
                    "Collider one in a convex pair does not have a valid broad-phase ID when trying to compute middle-phase collision.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity) != -1,
                    "Collider two in a convex pair does not have a valid broad-phase ID when trying to compute middle-phase collision.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != _colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity),
                    "Collider one and collider two in a convex pair have the same broad-phase ID when trying to compute middle-phase collision.");

            // Mark the pair as not colliding for the current frame.
            // This will be updated during narrow-phase collision detection if the pair is found to be colliding.
            pair.AreCollidingThisFrame = false;

            // Store the collider entities for both colliders in the overlapping pair.
            const Entity colliderOneEntity = pair.ColliderOneEntity;
            const Entity colliderTwoEntity = pair.ColliderTwoEntity;

            // Store the collider indices for both colliders for faster access to their components in the collider component store.
            const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
            const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

            // Check if either collider is a trigger, since triggers can generate contact events even though they don't produce collision response.
            const bool isColliderOneTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex);
            const bool isColliderTwoTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);

            // Check if either collider is a query collider.
            const bool isWorldQueryColliderOne = _colliderComponentStore.IsQueryColliderAtIndex(colliderOneIndex);
            const bool isWorldQueryColliderTwo = _colliderComponentStore.IsQueryColliderAtIndex(colliderTwoIndex);

            // Check if either collider is a simulation collider.
            const bool isColliderOneSimulationCollider = _colliderComponentStore.IsSimulationColliderAtIndex(colliderOneIndex);
            const bool isColliderTwoSimulationCollider = _colliderComponentStore.IsSimulationColliderAtIndex(colliderTwoIndex);

            // Store if either collider is a simulation collider or a trigger.
            const bool isColliderOneSimulationColliderOrTrigger = isColliderOneSimulationCollider || isColliderOneTrigger;
            const bool isColliderTwoSimulationColliderOrTrigger = isColliderTwoSimulationCollider || isColliderTwoTrigger;

            // For world queries, we want to consider pairs where both colliders are query colliders.
            // For regular collision detection, we want to consider pairs where both colliders are either simulation colliders or triggers,
            // since triggers can generate contact events even though they don't produce collision response.
            if ((isWorldQuery && isWorldQueryColliderOne && isWorldQueryColliderTwo) ||
                (!isWorldQuery && isColliderOneSimulationColliderOrTrigger && isColliderTwoSimulationColliderOrTrigger)) {

                // Store the active status of both colliders to avoid redundant checks in the narrow-phase processing.
                const bool isBodyOneActive = colliderOneIndex < activeColliderComponents;
                const bool isBodyTwoActive = colliderTwoIndex < activeColliderComponents;

                // For world queries, we want to test all pairs of query colliders regardless of their active state,
                // since the user is explicitly asking for overlap information.
                // For regular collision detection, we only want to test pairs where at least one collider is active,
                // since inactive colliders should not generate collision callbacks or response.
                if (isWorldQuery || (!isWorldQuery && (isBodyOneActive || isBodyTwoActive))) {
                    CollisionShape &colliderOneShape = _colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex);
                    CollisionShape &colliderTwoShape = _colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex);

                    batches.AddNarrowPhaseTest(pair.PairID,
                                               colliderOneEntity,
                                               colliderTwoEntity,
                                               colliderOneShape,
                                               colliderTwoShape,
                                               _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderOneIndex),
                                               _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderTwoIndex),
                                               pair.NarrowPhaseAlgorithmToUse,
                                               reportContacts,
                                               pair.LastFrameCollisionInfo

                    );
                }
            }
        }

        // Process the concave pairs to generate narrow-phase input batches for narrow-phase collision detection.
        for (auto &pair : _overlappingPairs._concavePairs) {
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != -1,
                    "Collider one in a concave pair does not have a valid broad-phase ID when trying to compute middle-phase collision.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity) != -1,
                    "Collider two in a concave pair does not have a valid broad-phase ID when trying to compute middle-phase collision.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != _colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity),
                    "Collider one and collider two in a concave pair have the same broad-phase ID when trying to compute middle-phase collision.");

            // Mark the pair as not colliding for the current frame.
            // This will be updated during narrow-phase collision detection if the pair is found to be colliding.
            pair.AreCollidingThisFrame = false;

            // Store the collider entities for both colliders in the overlapping pair.
            const Entity colliderOneEntity = pair.ColliderOneEntity;
            const Entity colliderTwoEntity = pair.ColliderTwoEntity;

            // Store the collider indices for both colliders for faster access to their components in the collider component store.
            const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
            const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

            // Check if either collider is a trigger, since triggers can generate contact events even though they don't produce collision response.
            const bool isColliderOneTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex);
            const bool isColliderTwoTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);

            // Check if either collider is a query collider.
            const bool isWorldQueryColliderOne = _colliderComponentStore.IsQueryColliderAtIndex(colliderOneIndex);
            const bool isWorldQueryColliderTwo = _colliderComponentStore.IsQueryColliderAtIndex(colliderTwoIndex);

            // Check if either collider is a simulation collider.
            const bool isColliderOneSimulationCollider = _colliderComponentStore.IsSimulationColliderAtIndex(colliderOneIndex);
            const bool isColliderTwoSimulationCollider = _colliderComponentStore.IsSimulationColliderAtIndex(colliderTwoIndex);

            // Store if either collider is a simulation collider or a trigger.
            const bool isColliderOneSimulationColliderOrTrigger = isColliderOneSimulationCollider || isColliderOneTrigger;
            const bool isColliderTwoSimulationColliderOrTrigger = isColliderTwoSimulationCollider || isColliderTwoTrigger;

            // For world queries, we want to consider pairs where both colliders are query colliders.
            // For regular collision detection, we want to consider pairs where both colliders are either simulation colliders or triggers,
            // since triggers can generate contact events even though they don't produce collision response.
            if ((isWorldQuery && isWorldQueryColliderOne && isWorldQueryColliderTwo) ||
                (!isWorldQuery && isColliderOneSimulationColliderOrTrigger && isColliderTwoSimulationColliderOrTrigger)) {

                // Store the active status of both colliders to avoid redundant checks in the narrow-phase processing.
                const bool isBodyOneActive = colliderOneIndex < activeColliderComponents;
                const bool isBodyTwoActive = colliderTwoIndex < activeColliderComponents;

                // For world queries, we want to test all pairs of query colliders regardless of their active state,
                // since the user is explicitly asking for overlap information.
                // For regular collision detection, we only want to test pairs where at least one collider is active,
                // since inactive colliders should not generate collision callbacks or response.
                if (isWorldQuery || (!isWorldQuery && (isBodyOneActive || isBodyTwoActive))) {
                    computeConvexVsConcaveMiddlePhase(pair, batches, reportContacts);
                }
            }
        }
    }

    void CollisionSystem::computeMiddlePhaseCollisionSnapshot(std::vector<u64> &convexPairs,
                                                              std::vector<u64> &concavePairs,
                                                              NarrowPhaseInput &batches,
                                                              bool reportContacts) {
        // Clear obsolete last-frame collision data for all active pairs before processing the current frame's pairs.
        // This ensures that any per-sub-shape collision cache entries that are not refreshed during the middle-phase
        // processing of the current frame will be pruned, which helps to keep the memory usage of the collision cache in check over time.
        _overlappingPairs.ClearObsoleteLastFrameCollisionData();

        // Process the convex pairs to generate narrow-phase input batches for narrow-phase collision detection.
        for (const auto &pairID : convexPairs) {
            const u64 pairIndex = _overlappingPairs._convexPairIDToPairIndexMap[pairID];

            VASSERT(pairIndex < _overlappingPairs._convexPairs.size(),
                    "Convex pair index is out of bounds when trying to compute middle-phase collision snapshot.");

            ConvexOverlappingPair &pair = _overlappingPairs._convexPairs[pairIndex];
            const Entity colliderOneEntity = pair.ColliderOneEntity;
            const Entity colliderTwoEntity = pair.ColliderTwoEntity;
            const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
            const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

            VASSERT(_colliderComponentStore.GetBroadPhaseIDAtIndex(colliderOneIndex) != -1,
                    "Collider one in a convex pair does not have a valid broad-phase ID when trying to compute middle-phase collision snapshot.");
            VASSERT(_colliderComponentStore.GetBroadPhaseIDAtIndex(colliderTwoIndex) != -1,
                    "Collider two in a convex pair does not have a valid broad-phase ID when trying to compute middle-phase collision snapshot.");
            VASSERT(_colliderComponentStore.GetBroadPhaseIDAtIndex(colliderOneIndex) != _colliderComponentStore.GetBroadPhaseIDAtIndex(colliderTwoIndex),
                    "Collider one and collider two in a convex pair have the same broad-phase ID when trying to compute middle-phase collision snapshot.");

            CollisionShape &colliderOneShape = _colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex);
            CollisionShape &colliderTwoShape = _colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex);
            const TransformComponent &localToWorldTransformOne = _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderOneIndex);
            const TransformComponent &localToWorldTransformTwo = _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderTwoIndex);

            // Add the convex pair to the narrow-phase input batches for narrow-phase collision detection.
            batches.AddNarrowPhaseTest(pairID,
                                       colliderOneEntity,
                                       colliderTwoEntity,
                                       colliderOneShape,
                                       colliderTwoShape,
                                       localToWorldTransformOne,
                                       localToWorldTransformTwo,
                                       pair.NarrowPhaseAlgorithmToUse,
                                       reportContacts,
                                       pair.LastFrameCollisionInfo);
        }

        // Process the concave pairs to generate narrow-phase input batches for narrow-phase collision detection.
        for (const auto &pairID : concavePairs) {
            ConcaveOverlappingPair &pair = _overlappingPairs._concavePairs[_overlappingPairs._concavePairIDToPairIndexMap[pairID]];

            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != -1,
                    "Collider one in a concave pair does not have a valid broad-phase ID when trying to compute middle-phase collision snapshot.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity) != -1,
                    "Collider two in a concave pair does not have a valid broad-phase ID when trying to compute middle-phase collision snapshot.");
            VASSERT(_colliderComponentStore.GetBroadPhaseID(pair.ColliderOneEntity) != _colliderComponentStore.GetBroadPhaseID(pair.ColliderTwoEntity),
                    "Collider one and collider two in a concave pair have the same broad-phase ID when trying to compute middle-phase collision snapshot.");

            // Add the concave pair to the narrow-phase input batches for narrow-phase collision detection.
            computeConvexVsConcaveMiddlePhase(pair, batches, reportContacts);
        }
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
        for (const auto &pair : overlappingNodes) {
            VASSERT(pair.First != -1 && pair.Second != -1,
                    "Broad-phase overlapping pair contains an invalid broad-phase ID when trying to update overlapping pairs.");

            // Make sure to only process pairs of different broad-phase IDs.
            if (pair.First != pair.Second) {
                // Get the collider entities for both colliders in the pair.
                const Entity colliderOneEntity = _broadPhaseIDToColliderEntityMap[pair.First];
                const Entity colliderTwoEntity = _broadPhaseIDToColliderEntityMap[pair.Second];

                // Get the collider indices for both colliders in the pair.
                const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
                const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

                // Get the body entities for both colliders in the pair.
                const Entity bodyOneEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderOneIndex);
                const Entity bodyTwoEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderTwoIndex);

                // Only consider this pair of colliders for narrow-phase collision detection if they belong to different bodies,
                // since colliders on the same body should not collide with each other.
                if (bodyOneEntity != bodyTwoEntity) {

                    // Create a pair of body entities for this pair of colliders, which will be used to check if this pair of colliders is in the list of
                    // non-collidable pairs (i.e. pairs of colliders that should be ignored for collision detection because they belong to the same rigid body
                    // or to a pair of rigid bodies that are set to ignore collision with each other).
                    const Pair<Entity, Entity> bodyPair = OverlappingPairs::ComputeBodiesIndexPair(bodyOneEntity, bodyTwoEntity);

                    if (!_nonCollidablePairs.contains(bodyPair)) {
                        // Create a unique pair ID for this pair of colliders by combining their broad-phase IDs in a consistent order.
                        const u64 pairID = PairNumbers(std::max(pair.First, pair.Second), std::min(pair.First, pair.Second));

                        // Check if this broad-phase overlapping pair is already in the overlapping pair list.
                        OverlappingPair *overlappingPair = _overlappingPairs.GetOverlappingPair(pairID);

                        // If the broad-phase system reports this pair as overlapping but it's not already in the overlapping pair list,
                        // we need to create a new overlapping pair for it.
                        if (nullptr == overlappingPair) {
                            // Get the collides-with mask bits for both colliders in the pair,
                            // which will be used to check if their collision filters allow them to collide.
                            const u16 shapeOneCollidesWithMaskBits = _colliderComponentStore.GetCollidesWithMaskBitsAtIndex(colliderOneIndex);
                            const u16 shapeTwoCollidesWithMaskBits = _colliderComponentStore.GetCollidesWithMaskBitsAtIndex(colliderTwoIndex);

                            // Get the collision category bits for both colliders in the pair,
                            // which will be used to check if their collision filters allow them to collide.
                            const u16 shapeOneCollisionCategoryBits = _colliderComponentStore.GetCollisionCategoryBitsAtIndex(colliderOneIndex);
                            const u16 shapeTwoCollisionCategoryBits = _colliderComponentStore.GetCollisionCategoryBitsAtIndex(colliderTwoIndex);

                            // Check if the collision filters of the two colliders in the pair allow them
                            // to collide based on their collision category bits and collides-with mask bits.
                            if ((shapeOneCollidesWithMaskBits & shapeTwoCollisionCategoryBits) != 0 &&
                                (shapeTwoCollidesWithMaskBits & shapeOneCollisionCategoryBits) != 0) {

                                // Check if either collider in the pair has a convex shape.
                                const bool isShapeOneConvex = _colliderComponentStore.GetColliderAtIndex(colliderOneIndex).GetCollisionShape().IsConvex();
                                const bool isShapeTwoConvex = _colliderComponentStore.GetColliderAtIndex(colliderTwoIndex).GetCollisionShape().IsConvex();

                                // If at least one of the colliders in the pair has a convex shape,
                                // we need to add this pair to the overlapping pair list for narrow-phase collision detection.
                                if (isShapeOneConvex || isShapeTwoConvex) {
                                    _overlappingPairs.AddPair(colliderOneIndex, colliderTwoIndex, isShapeOneConvex && isShapeTwoConvex);
                                }
                            }
                        } else {
                            // If the broad-phase system reports this pair as overlapping and it's already in the overlapping pair list, we can simply mark it
                            // as not requiring a collision check for this frame, since it will be processed during narrow-phase collision detection based on
                            // the existing overlapping pair information in the overlapping pair list.
                            overlappingPair->RequiresCollisionCheck = false;
                        }
                    }
                }
            }
        }
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
        const Entity colliderOneEntity = overlappingPair.ColliderOneEntity;
        const Entity colliderTwoEntity = overlappingPair.ColliderTwoEntity;

        const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
        const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

        const TransformComponent &shapeOneLocalToWorldTransform = _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderOneIndex);
        const TransformComponent &shapeTwoLocalToWorldTransform = _colliderComponentStore.GetLocalToWorldTransformAtIndex(colliderTwoIndex);

        TransformComponent convexToConcaveTransform;
        ConvexShape *convexShape = nullptr;
        ConcaveShape *concaveShape = nullptr;

        // Determine which collider in the pair has the convex shape and which one has the concave shape,
        // and compute the transform from the convex shape's local space to the concave shape's local space.
        if (overlappingPair.IsFirstShapeConvex) {
            convexShape = static_cast<ConvexShape *>(&_colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex));
            concaveShape = static_cast<ConcaveShape *>(&_colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex));
            convexToConcaveTransform = shapeTwoLocalToWorldTransform.Inverse() * shapeOneLocalToWorldTransform;
        } else {
            convexShape = static_cast<ConvexShape *>(&_colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex));
            concaveShape = static_cast<ConcaveShape *>(&_colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex));
            convexToConcaveTransform = shapeOneLocalToWorldTransform.Inverse() * shapeTwoLocalToWorldTransform;
        }

        VASSERT(convexShape->IsConvex(),
                "Convex shape in a convex vs concave pair does not have a convex collision shape when trying to compute convex vs concave middle-phase.");
        VASSERT(!concaveShape->IsConvex(),
                "Concave shape in a convex vs concave pair does not have a concave collision shape when trying to compute convex vs concave middle-phase.");
        VASSERT(overlappingPair.NarrowPhaseAlgorithmToUse != NarrowPhaseAlgorithm::NoCollisionCheck,
                "Narrow-phase algorithm to use for a convex vs concave pair is not set when trying to compute convex vs concave middle-phase.");

        const AABB aabb = convexShape->ComputeTransformedAABB(convexToConcaveTransform);

        std::vector<glm::vec3> triangleVertices;
        std::vector<glm::vec3> triangleVerticesNormals;
        std::vector<u32> shapeIDs;
        triangleVertices.reserve(64);
        triangleVerticesNormals.reserve(64);
        shapeIDs.reserve(64);

        // Compute the triangles of the concave shape that overlap with the convex shape's AABB in the concave shape's local space.
        concaveShape->ComputeOverlappingTriangles(aabb, triangleVertices, triangleVerticesNormals, shapeIDs);

        VASSERT(triangleVertices.size() == triangleVerticesNormals.size(),
                "Triangle vertices and triangle vertex normals arrays should have the same size after computing overlapping triangles for convex vs concave "
                "middle-phase.");

        VASSERT(shapeIDs.size() == triangleVertices.size() / 3,
                "Shape IDs array size should be equal to the number of triangles (i.e. triangle vertices array size divided by 3) after computing overlapping "
                "triangles for "
                "convex vs concave middle-phase.");

        VASSERT(triangleVertices.size() % 3 == 0,
                "Triangle vertices array size should be a multiple of 3 after computing overlapping triangles for convex vs concave middle-phase.");

        VASSERT(triangleVerticesNormals.size() % 3 == 0,
                "Triangle vertex normals array size should be a multiple of 3 after computing overlapping triangles for convex vs concave middle-phase.");

        const bool isColliderOneTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex);
        const bool isColliderTwoTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);
        reportContacts = reportContacts && !isColliderOneTrigger && !isColliderTwoTrigger;

        CollisionShape *shapeOne = nullptr;
        CollisionShape *shapeTwo = nullptr;

        if (overlappingPair.IsFirstShapeConvex) {
            shapeOne = convexShape;
        } else {
            shapeTwo = convexShape;
        }

        // For each triangle of the concave shape that overlaps with the convex shape's AABB,
        // create a triangle collision shape and add a narrow-phase test for this triangle
        // against the convex shape in the narrow-phase input batches.
        for (size_t i = 0; i < shapeIDs.size(); ++i) {
            // Create a triangle collision shape (the allocated memory for the TriangleShape
            // will be released in the destructor of the NarrowPhaseDataBatch.
            auto *triangleShape = new TriangleShape(&(triangleVertices[i * 3]), &(triangleVerticesNormals[i * 3]), shapeIDs[i], _triangleHalfEdgeMesh);

            if (overlappingPair.IsFirstShapeConvex) {
                shapeTwo = triangleShape;
            } else {
                shapeOne = triangleShape;
            }

            // Add a collision info for the two collision shapes into the overlapping pair (if not present yet).
            LastFrameCollisionData *lastFrameInfo = overlappingPair.AddLastFrameCollisionDataIfNecessary(shapeOne->GetID(), shapeTwo->GetID());

            // Create a narrow-phase test for this triangle against the convex shape in the narrow-phase input batches,
            // which will be processed later during narrow-phase collision detection.
            batches.AddNarrowPhaseTest(overlappingPair.PairID,
                                       colliderOneEntity,
                                       colliderTwoEntity,
                                       *shapeOne,
                                       *shapeTwo,
                                       shapeOneLocalToWorldTransform,
                                       shapeTwoLocalToWorldTransform,
                                       overlappingPair.NarrowPhaseAlgorithmToUse,
                                       reportContacts,
                                       *lastFrameInfo);
        }
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
                                                   bool updateLastFrameData,
                                                   std::vector<ContactPointData> &potentialContactPoints,
                                                   std::vector<ContactManifoldData> &potentialContactManifolds,
                                                   std::unordered_map<u64, u32> &mapPairIdToContactPairIndex,
                                                   std::vector<ContactPair> &contactPairs) {
        const auto &settings = _physicsWorld.GetSettings();
        const f32 cosAngleSimilarContactManifold = settings.CosAngleSimilarContactManifold;

        for (size_t d = 0; d < batch.Data.size(); ++d) {
            NarrowPhaseData &data = batch.Data[d];

            // Update the last-frame collision state for every entry in the batch unconditionally,
            // so that pairs which stop colliding are correctly marked as non-colliding next frame.
            if (updateLastFrameData) {
                data.LastFrameCollisionInfo.WasColliding = data.IsColliding;
                data.LastFrameCollisionInfo.IsValid = true;
            }

            if (data.IsColliding) {
                const u64 pairID = data.OverlappingPairID;
                OverlappingPair *overlappingPair = _overlappingPairs.GetOverlappingPair(pairID);

                VASSERT(nullptr != overlappingPair,
                        "Overlapping pair should exist for a colliding narrow-phase data entry when trying to process potential contacts.");

                // Mark the overlapping pair as colliding in the current frame so that the event system
                // can distinguish new, persisting, and lost contacts after all batches are processed.
                overlappingPair->AreCollidingThisFrame = true;

                const Entity colliderOneEntity = data.ColliderOneEntity;
                const Entity colliderTwoEntity = data.ColliderTwoEntity;

                const size_t colliderOneIndex = _colliderComponentStore.GetEntityIndex(colliderOneEntity);
                const size_t colliderTwoIndex = _colliderComponentStore.GetEntityIndex(colliderTwoEntity);

                const Entity bodyOneEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderOneIndex);
                const Entity bodyTwoEntity = _colliderComponentStore.GetBodyEntityAtIndex(colliderTwoIndex);

                const bool isTrigger = _colliderComponentStore.IsTriggerAtIndex(colliderOneIndex) || _colliderComponentStore.IsTriggerAtIndex(colliderTwoIndex);

                VASSERT((isTrigger && data.ContactPointCount == 0) || (!isTrigger && data.ContactPointCount > 0),
                        "Trigger pairs should not have contact points and non-trigger pairs should have at least one contact point when trying to process "
                        "potential contacts.");

                const bool isShapeOneConvex = _colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex).IsConvex();
                const bool isShapeTwoConvex = _colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex).IsConvex();

                // Convex vs convex: each narrow-phase entry maps 1-to-1 to exactly one ContactPair
                // and one ContactManifoldData, so we can create them unconditionally here.
                if (isShapeOneConvex && isShapeTwoConvex) {
                    const u32 newContactPairIndex = static_cast<u32>(contactPairs.size());

                    contactPairs.emplace_back(pairID,
                                              bodyOneEntity,
                                              bodyTwoEntity,
                                              colliderOneEntity,
                                              colliderTwoEntity,
                                              newContactPairIndex,
                                              overlappingPair->WereCollidingLastFrame,
                                              isTrigger);

                    ContactPair &contactPair = contactPairs[newContactPairIndex];

                    // Triggers don't generate contact points, so only build the manifold when there are points to store.
                    if (data.ContactPointCount > 0) {
                        const u32 contactManifoldIndex = static_cast<u32>(potentialContactManifolds.size());
                        potentialContactManifolds.emplace_back(pairID);
                        ContactManifoldData &contactManifoldData = potentialContactManifolds[contactManifoldIndex];

                        // firstContactPointIndex is the offset into the shared potentialContactPoints array
                        // where this manifold's points begin, allowing the indices stored in the manifold to
                        // correctly reference their entries even after further points are appended.
                        const u32 firstContactPointIndex = static_cast<u32>(potentialContactPoints.size());

                        for (u32 i = 0; i < data.ContactPointCount; ++i) {
                            if (contactManifoldData.TotalPotentialContactPoints < MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD) {
                                // Add the contact point to the manifold.
                                contactManifoldData.PotentialContactPointsIndices[contactManifoldData.TotalPotentialContactPoints] = firstContactPointIndex + i;
                                contactManifoldData.TotalPotentialContactPoints++;

                                // Add the contact point to the array of potential contact points
                                const ContactPointData &contactPoint = data.ContactPoints[i];

                                potentialContactPoints.push_back(contactPoint);
                            }
                        }

                        VASSERT(data.ContactPointCount > 0,
                                "Colliding narrow-phase data should have at least one contact point when trying to process potential contacts.");
                        VASSERT(pairID == contactManifoldData.PairID,
                                "Contact manifold data pair ID should match the narrow-phase data pair ID when trying to process potential contacts.");

                        contactPair.PotentialContactManifoldIndices[0] = contactManifoldIndex;
                        contactPair.PotentialContactManifoldsCount = 1;
                    }
                } else {
                    // Convex vs concave: the concave shape is decomposed into many triangles by the middle phase,
                    // each generating a separate narrow-phase entry that all share the same overlapping pair ID.
                    // We therefore reuse a single ContactPair across all triangle sub-entries for this pair,
                    // inserting one only on the first encounter and looking it up on subsequent ones.
                    auto it = mapPairIdToContactPairIndex.find(pairID);
                    ContactPair *contactPair = nullptr;

                    if (it == mapPairIdToContactPairIndex.end()) {
                        // First triangle for this pair — create a new ContactPair and register it in the map.
                        const u32 newContactPairIndex = static_cast<u32>(contactPairs.size());
                        contactPairs.emplace_back(pairID,
                                                  bodyOneEntity,
                                                  bodyTwoEntity,
                                                  colliderOneEntity,
                                                  colliderTwoEntity,
                                                  newContactPairIndex,
                                                  overlappingPair->WereCollidingLastFrame,
                                                  isTrigger);

                        contactPair = &contactPairs[newContactPairIndex];
                        mapPairIdToContactPairIndex[pairID] = newContactPairIndex;
                    } else {
                        // Subsequent triangle for the same pair — reuse the existing ContactPair.
                        VASSERT(it->first == pairID,
                                "Pair ID in the map should match the narrow-phase data pair ID when trying to process potential contacts.");

                        const u32 contactPairIndex = it->second;
                        contactPair = &contactPairs[contactPairIndex];
                    }

                    VASSERT(nullptr != contactPair,
                            "Contact pair should exist for a colliding narrow-phase data entry when trying to process potential contacts for convex vs concave "
                            "pairs.");

                    if (data.ContactPointCount > 0) {

                        for (u8 i = 0; i < data.ContactPointCount; ++i) {
                            const ContactPointData &contactPoint = data.ContactPoints[i];
                            const u32 contactPointIndex = static_cast<u32>(potentialContactPoints.size());
                            potentialContactPoints.push_back(contactPoint);

                            bool similarManifoldFound = false;

                            // Try to merge this contact point into an existing manifold whose first point has
                            // a similar normal direction (dot product >= cosAngleSimilarContactManifold).
                            // This groups geometrically coherent contacts together, which improves solver stability.
                            for (u8 m = 0; m < contactPair->PotentialContactManifoldsCount; ++m) {

                                const u32 contactManifoldIndex = contactPair->PotentialContactManifoldIndices[m];
                                ContactManifoldData &contactManifoldData = potentialContactManifolds[contactManifoldIndex];

                                VASSERT(contactManifoldData.TotalPotentialContactPoints > 0,
                                        "Contact manifold data should have at least one potential contact point when trying to process potential contacts for "
                                        "convex vs concave "
                                        "pairs.");

                                if (contactManifoldData.TotalPotentialContactPoints < MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD) {
                                    // Get the first contact point of the current manifold.
                                    const uint manifoldContactPointIndex = contactManifoldData.PotentialContactPointsIndices[0];
                                    const ContactPointData &manifoldContactPoint = potentialContactPoints[manifoldContactPointIndex];

                                    // If we have found a corresponding manifold for the new contact point
                                    // (a manifold with a similar contact normal direction)
                                    const f32 dotProduct = glm::dot(manifoldContactPoint.WorldSpaceContactNormal, contactPoint.WorldSpaceContactNormal);

                                    if (dotProduct >= cosAngleSimilarContactManifold) {
                                        // Add the contact point to the manifold.
                                        contactManifoldData.PotentialContactPointsIndices[contactManifoldData.TotalPotentialContactPoints] = contactPointIndex;
                                        contactManifoldData.TotalPotentialContactPoints++;

                                        similarManifoldFound = true;

                                        break;
                                    }
                                }
                            }

                            // No existing manifold had a compatible normal — open a new one for this contact point.
                            if (!similarManifoldFound && contactPair->PotentialContactManifoldsCount < MAX_POTENTIAL_CONTACT_MANIFOLDS) {

                                // Create a new potential contact manifold for the overlapping pair
                                u32 contactManifoldIndex = static_cast<u32>(potentialContactManifolds.size());
                                potentialContactManifolds.push_back(pairID);
                                ContactManifoldData &contactManifoldData = potentialContactManifolds[contactManifoldIndex];

                                // Add the contact point to the manifold
                                contactManifoldData.PotentialContactPointsIndices[0] = contactPointIndex;
                                contactManifoldData.TotalPotentialContactPoints = 1;

                                VASSERT(nullptr != contactPair,
                                        "Contact pair should exist when trying to add a new potential contact manifold for a colliding narrow-phase data "
                                        "entry for convex vs concave "
                                        "pairs.");

                                // Add the contact manifold to the overlapping pair contact
                                VASSERT(potentialContactManifolds[contactManifoldIndex].TotalPotentialContactPoints > 0,
                                        "New contact manifold should have at least one potential contact point when trying to add a new potential contact "
                                        "manifold for a colliding narrow-phase data "
                                        "entry for convex vs concave "
                                        "pairs.");

                                VASSERT(contactPair->PairID == contactManifoldData.PairID,
                                        "Contact manifold data pair ID should match the contact pair ID when trying to add a new potential contact "
                                        "manifold for a colliding narrow-phase data "
                                        "entry for convex vs concave "
                                        "pairs.");

                                contactPair->PotentialContactManifoldIndices[contactPair->PotentialContactManifoldsCount] = contactManifoldIndex;
                                contactPair->PotentialContactManifoldsCount++;
                            }

                            VASSERT(contactPair->PotentialContactManifoldsCount > 0,
                                    "Contact pair should have at least one potential contact manifold when trying to process potential contacts for convex "
                                    "vs concave pairs.");
                        }
                    }
                }

                // Release the contact points stored in the batch entry now that they have been
                // moved into the shared potentialContactPoints array, freeing per-frame memory.
                batch.ResetContactPoints(d);
            }
        }
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
        // For each contact pair, if the number of potential contact manifolds exceeds the maximum allowed, we can perform a reduction step to remove some
        // of the manifolds based on their depth, which can help to improve the performance of the constraint solver while still maintaining good collision
        // response quality. The depth of a contact manifold can be computed as the largest penetration depth among its contact points, which gives us an
        // indication of how significant the collision is for that manifold. By removing the manifolds with the smallest depth, we can focus the constraint
        // solver on the most significant collisions while still maintaining good overall collision response quality.
        for (ContactPair &pair : contactPairs) {
            while (pair.PotentialContactManifoldsCount > MAX_CONTACT_MANIFOLDS) {
                f32 minDepth = std::numeric_limits<f32>::max();
                u32 minDepthManifoldIndex = std::numeric_limits<u32>::max();

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

    // TODO: Implement this.
    void CollisionSystem::createContacts() {
        // _currentContactManifolds->reserve(_currentContactPairs->size());
        // _currentContactPoints->reserve(_currentContactManifolds->size());
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
        const auto &settings = _physicsWorld.GetSettings();
        const f32 persistentContactDistanceThreshold = settings.PersistentContactDistanceThresholdSquared;
        const f32 cosAngleSimilarContactManifold = settings.CosAngleSimilarContactManifold;

        // Iterate through the current contact pairs and try to initialize their contact manifolds
        // and contact points with the contact manifolds and contact points of the previous contact
        // pair with the same pair ID (if any), which can help to improve the stability of the
        // contact constraints and reduce jitter in the collision response.
        for (ContactPair &currentContactPair : *_currentContactPairs) {
            auto itPreviousContactPair = _previousPairIDToContactPairIndexMap.find(currentContactPair.PairID);

            // If there is a contact pair in the previous frame with the same pair ID as the current contact pair, we can try to initialize the contact
            // manifolds and contact points of the current contact pair with the contact manifolds and contact points of the previous contact pair.
            if (itPreviousContactPair != _previousPairIDToContactPairIndexMap.end()) {
                const u32 previousContactPairIndex = itPreviousContactPair->second;
                const ContactPair &previousContactPair = (*_previousContactPairs)[previousContactPairIndex];

                /****************************************************************************/
                // Contact Manifolds Initialization with Previous Contact Manifolds.
                /****************************************************************************/

                const u32 contactManifoldIndex = currentContactPair.ContactManifoldIndex;
                const u32 contactManifoldCount = currentContactPair.ContactManifoldCount;

                // Iterate through the contact manifolds for this contact pair and try to find a matching
                // contact manifold in the previous contact pair based on the similarity of their contact normals.
                for (u32 m = contactManifoldIndex; m < contactManifoldIndex + contactManifoldCount; ++m) {
                    VASSERT(m < _currentContactManifolds->size(),
                            "Contact manifold index should be within bounds of current contact manifolds list when trying to initialize contacts with previous "
                            "ones.");

                    ContactManifold &currentContactManifold = (*_currentContactManifolds)[m];

                    VASSERT(currentContactManifold.ContactPointCount > 0,
                            "Contact manifold should have at least one contact point when trying to initialize contacts with previous ones.");

                    ContactPoint &currentContactPoint = (*_currentContactPoints)[currentContactManifold.ContactPointIndex];
                    const glm::vec3 currentContactPointNormal = currentContactPoint.GetWorldSpaceContactNormal();

                    const u32 previousContactManifoldIndex = previousContactPair.ContactManifoldIndex;
                    const u32 previousContactManifoldCount = previousContactPair.ContactManifoldCount;

                    // Iterate through the contact manifolds in the previous contact pair to find a contact manifold that has a similar contact normal to the
                    // current contact manifolds contact normal based on the cosine of the angle between their contact normals. If such a contact manifold is
                    // found, we can transfer the relevant data from the previous contact manifold to the current contact manifold, which can help to improve
                    // the stability of the contact constraints and reduce jitter in the collision response. We use the cosine of the angle between the contact
                    // normals to determine if the contact manifolds are similar enough to transfer data, as this allows us to account for cases where the
                    // contact normals may not be exactly the same due to small variations in the collision response or numerical precision issues, while still
                    // ensuring that we only transfer data between contact manifolds that are reasonably similar in terms of their contact normals, which is
                    // important for maintaining good collision response quality.
                    for (u32 p = previousContactManifoldIndex; p < previousContactManifoldIndex + previousContactManifoldCount; ++p) {
                        ContactManifold &previousContactManifold = (*_previousContactManifolds)[p];

                        VASSERT(previousContactManifold.ContactPointCount > 0,
                                "Contact manifold should have at least one contact point when trying to initialize contacts with previous ones.");

                        ContactPoint &previousContactPoint = (*_previousContactPoints)[previousContactManifold.ContactPointIndex];

                        if (glm::dot(previousContactPoint.GetWorldSpaceContactNormal(), currentContactPointNormal) >= cosAngleSimilarContactManifold) {
                            // Transfer data from the previous contact manifold to the current one.
                            currentContactManifold.FrictionVectorOne = previousContactManifold.FrictionVectorOne;
                            currentContactManifold.FrictionVectorTwo = previousContactManifold.FrictionVectorTwo;
                            currentContactManifold.FrictionImpulseOne = previousContactManifold.FrictionImpulseOne;
                            currentContactManifold.FrictionImpulseTwo = previousContactManifold.FrictionImpulseTwo;
                            currentContactManifold.FrictionTwistImpulse = previousContactManifold.FrictionTwistImpulse;

                            break;
                        }
                    }
                }

                /****************************************************************************/
                // Contact Points Initialization with Previous Contact Points.
                /*****************************************************************************/

                const u32 contactPointIndex = currentContactPair.ContactPointIndex;
                const u32 contactPointCount = currentContactPair.ContactPointCount;

                // Iterate through the contact points for this contact pair and try to find a matching
                // contact point in the previous contact pair based on the distance between their contact points.
                for (u32 c = contactPointIndex; c < contactPointIndex + contactPointCount; ++c) {
                    VASSERT(
                        c < _currentContactPoints->size(),
                        "Contact point index should be within bounds of current contact points list when trying to initialize contacts with previous ones.");

                    ContactPoint &currentContactPoint = (*_currentContactPoints)[c];
                    const glm::vec3 currentLocalSpaceContactPointOnBodyOne = currentContactPoint.GetLocalSpaceContactPointOnBodyOne();

                    const u32 previousContactPointIndex = previousContactPair.ContactPointIndex;
                    const u32 previousContactPointCount = previousContactPair.ContactPointCount;

                    // Iterate through the contact points in the previous contact pair to find a contact point that is close enough to the current contact point
                    // based on the distance between their contact points in the local space of body one. If such a contact point is found, we can transfer the
                    // relevant data from the previous contact point to the current contact point, which can help to improve the stability of the contact
                    // constraints and reduce jitter in the collision response.
                    for (u32 p = previousContactPointIndex; p < previousContactPointIndex + previousContactPointCount; ++p) {
                        ContactPoint &previousContactPoint = (*_previousContactPoints)[p];
                        const f32 distanceSquared =
                            glm::distance2(previousContactPoint.GetLocalSpaceContactPointOnBodyOne(), currentLocalSpaceContactPointOnBodyOne);

                        if (distanceSquared <= persistentContactDistanceThreshold) {
                            // Transfer data from the previous contact point to the current one.
                            currentContactPoint.SetPenetrationImpulse(previousContactPoint.GetPenetrationImpulse());
                            currentContactPoint.SetIsRestingContact(previousContactPoint.IsRestingContact());

                            break;
                        }
                    }
                }
            }
        }
    }

    void CollisionSystem::reduceContactPoints(ContactManifoldData &manifold,
                                              const TransformComponent &shapeOneToWorldTransform,
                                              const std::vector<ContactPointData> &potentialContactPoints) const {

        u32 candidatePointsCount = manifold.TotalPotentialContactPoints;

        VASSERT(candidatePointsCount > MAX_CONTACT_POINTS_IN_MANIFOLD,
                "Contact manifold should have more than the maximum contact points in manifold when trying to reduce contact points in a manifold.");

        u32 candidatePointsIndices[MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD];

        for (u32 i = 0; i < candidatePointsCount; ++i) {
            candidatePointsIndices[i] = manifold.PotentialContactPointsIndices[i];
        }

        u32 pointsIndicesToKeep[MAX_CONTACT_POINTS_IN_MANIFOLD]{};
        const TransformComponent worldToShapeOneTransform = shapeOneToWorldTransform.Inverse();

        // Transform the contact normal into the local space of the first shape so that all
        // subsequent area calculations are done in a consistent local frame.
        const glm::vec3 shapeOneSpaceContactNormal =
            worldToShapeOneTransform.Rotation * potentialContactPoints[candidatePointsIndices[0]].WorldSpaceContactNormal;

        // --- Point 1: pick the point with the greatest projection onto a fixed search direction.
        // Using a constant direction produces stable, frame-coherent contact sets.
        const glm::vec3 searchDirection(1);
        f32 maxDotProduct = -std::numeric_limits<f32>::max();
        u32 elementIndexToKeep = 0;

        for (u32 i = 0; i < candidatePointsCount; ++i) {
            const ContactPointData &point = potentialContactPoints[candidatePointsIndices[i]];
            const f32 dotProduct = glm::dot(searchDirection, point.LocalSpaceContactPointOnBodyOne);

            if (dotProduct > maxDotProduct) {
                maxDotProduct = dotProduct;
                elementIndexToKeep = i;
            }
        }

        pointsIndicesToKeep[0] = candidatePointsIndices[elementIndexToKeep];
        removeItemAtInArray(candidatePointsIndices, elementIndexToKeep, candidatePointsCount);

        // --- Point 2: pick the point farthest from point 1.
        // This maximizes the length of the first edge of the contact polygon.
        f32 maxDistanceSquared(0.0f);
        elementIndexToKeep = 0;
        const ContactPointData &pointZero = potentialContactPoints[pointsIndicesToKeep[0]];

        for (u32 i = 0; i < candidatePointsCount; ++i) {
            const ContactPointData &element = potentialContactPoints[candidatePointsIndices[i]];
            const f32 distanceSquared = glm::distance2(element.LocalSpaceContactPointOnBodyOne, pointZero.LocalSpaceContactPointOnBodyOne);

            if (distanceSquared >= maxDistanceSquared) {
                maxDistanceSquared = distanceSquared;
                elementIndexToKeep = i;
            }
        }

        pointsIndicesToKeep[1] = candidatePointsIndices[elementIndexToKeep];
        removeItemAtInArray(candidatePointsIndices, elementIndexToKeep, candidatePointsCount);

        // --- Point 3: pick the point that forms the largest-area triangle with points 1 and 2.
        // We track both the most positive and most negative signed areas so we can choose the
        // correct winding regardless of the contact normal orientation.
        u32 thirdPointMaxAreaIndex = 0;
        u32 thirdPointMinAreaIndex = 0;

        f32 minArea(0.0f);
        f32 maxArea(0.0f);

        const glm::vec3 &p0 = potentialContactPoints[pointsIndicesToKeep[0]].LocalSpaceContactPointOnBodyOne;
        const glm::vec3 &p1 = potentialContactPoints[pointsIndicesToKeep[1]].LocalSpaceContactPointOnBodyOne;

        for (u32 i = 0; i < candidatePointsCount; ++i) {
            const glm::vec3 &element = potentialContactPoints[candidatePointsIndices[i]].LocalSpaceContactPointOnBodyOne;

            const glm::vec3 edgeOne = p0 - element;
            const glm::vec3 edgeTwo = p1 - element;

            // Signed area of the triangle formed by this candidate and the first two kept points,
            // projected onto the contact normal.
            const f32 area = glm::dot(glm::cross(edgeOne, edgeTwo), shapeOneSpaceContactNormal);

            if (area >= maxArea) {
                maxArea = area;
                thirdPointMaxAreaIndex = i;
            }

            if (area <= minArea) {
                minArea = area;
                thirdPointMinAreaIndex = i;
            }
        }

        // Keep whichever candidate produces the larger absolute area, and record its sign so
        // the fourth-point search knows which winding to look for.
        bool isPreviousAreaPositive;
        if (maxArea > -minArea) {
            isPreviousAreaPositive = true;
            pointsIndicesToKeep[2] = candidatePointsIndices[thirdPointMaxAreaIndex];
            removeItemAtInArray(candidatePointsIndices, thirdPointMaxAreaIndex, candidatePointsCount);
        } else {
            isPreviousAreaPositive = false;
            pointsIndicesToKeep[2] = candidatePointsIndices[thirdPointMinAreaIndex];
            removeItemAtInArray(candidatePointsIndices, thirdPointMinAreaIndex, candidatePointsCount);
        }

        // --- Point 4: pick the point that maximizes the total covered area by forming the
        // largest triangle of opposite winding against any edge of the triangle from points 1-3.
        // Opposite winding ensures the fourth point lies outside the existing triangle,
        // giving the broadest possible contact patch.
        f32 largestArea(0.0f);
        elementIndexToKeep = 0;
        f32 area;

        for (u32 i = 0; i < candidatePointsCount; ++i) {
            const ContactPointData &element = potentialContactPoints[candidatePointsIndices[i]];

            // Test the candidate against each edge of the triangle made by the first three points.
            for (u8 j = 0; j < 3; ++j) {
                u32 edgeVertexOneIndex = j;
                u32 edgeVertexTwoIndex = j < 2 ? j + 1 : 0;

                const ContactPointData &pointToKeepEdgeV1 = potentialContactPoints[pointsIndicesToKeep[edgeVertexOneIndex]];
                const ContactPointData &pointToKeepEdgeV2 = potentialContactPoints[pointsIndicesToKeep[edgeVertexTwoIndex]];

                const glm::vec3 newToFirst = pointToKeepEdgeV1.LocalSpaceContactPointOnBodyOne - element.LocalSpaceContactPointOnBodyOne;
                const glm::vec3 newToSecond = pointToKeepEdgeV2.LocalSpaceContactPointOnBodyOne - element.LocalSpaceContactPointOnBodyOne;

                area = glm::dot(glm::cross(newToFirst, newToSecond), shapeOneSpaceContactNormal);

                // We want the triangle with the greatest magnitude but opposite sign to point 3's
                // winding, so the four points together form a convex contact patch.
                if (isPreviousAreaPositive && area <= largestArea) {
                    largestArea = area;
                    elementIndexToKeep = i;
                } else if (!isPreviousAreaPositive && area >= largestArea) {
                    largestArea = area;
                    elementIndexToKeep = i;
                }
            }
        }

        pointsIndicesToKeep[3] = candidatePointsIndices[elementIndexToKeep];
        removeItemAtInArray(candidatePointsIndices, elementIndexToKeep, candidatePointsCount);

        // Only keep the four selected contact points in the manifold.
        manifold.PotentialContactPointsIndices[0] = pointsIndicesToKeep[0];
        manifold.PotentialContactPointsIndices[1] = pointsIndicesToKeep[1];
        manifold.PotentialContactPointsIndices[2] = pointsIndicesToKeep[2];
        manifold.PotentialContactPointsIndices[3] = pointsIndicesToKeep[3];
        manifold.TotalPotentialContactPoints = 4;
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
            OverlapCallback::Data callbackData(*contactPairs, lostContactPairs, true);
            eventListener.OnTrigger(callbackData);
        }
    }

    // void CollisionSystem::reportDebugRenderingContacts(std::vector<ContactPair> *contactPairs,
    //                                                    std::vector<ContactManifold> *manifolds,
    //                                                    std::vector<ContactPoint> *contactPoints,
    //                                                    std::vector<ContactPair> &lostContactPairs) {
    //     // Report contacts for debug rendering if there are any contact pairs or lost contact pairs to report.
    //     if (contactPairs->size() + lostContactPairs.size() > 0) {
    //         CollisionCallback::Data callbackData(contactPairs, manifolds, contactPoints, lostContactPairs, *mWorld);
    //         _physicsWorld.GetDebugRenderer().OnContact(callbackData);
    //     }
    // }

    f32 CollisionSystem::computePotentialManifoldLargestContactDepth(const ContactManifoldData &manifold,
                                                                     const std::vector<ContactPointData> &potentialContactPoints) const {
        f32 largestDepth = 0.0f;

        VASSERT(manifold.TotalPotentialContactPoints > 0,
                "Manifold should have at least one potential contact point when trying to compute the largest contact depth among the potential contact "
                "points.");

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

    void CollisionSystem::removeItemAtInArray(u32 array[], size_t index, u32 &arraySize) const {
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
