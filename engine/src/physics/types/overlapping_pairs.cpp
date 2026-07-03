#include "physics/types/overlapping_pairs.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    OverlappingPairs::OverlappingPairs(PhysicsWorld &physicsWorld, CollisionDispatch &collisionDispatch)
        : _bodyComponentStore(physicsWorld.GetBodyComponentStore())
        , _colliderComponentStore(physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(physicsWorld.GetRigidBodyComponentStore())
        , _collisionDispatch(collisionDispatch) {
    }

    OverlappingPairs::~OverlappingPairs() {
        while (!_convexPairs.empty()) {
            RemoveConvexPairWithIndex(_convexPairs.size() - 1, true);
        }

        while (!_concavePairs.empty()) {
            RemoveConcavePairWithIndex(_concavePairs.size() - 1, true);
        }

        while (!_disabledConvexPairs.empty()) {
            removeDisabledConvexPairWithIndex(_disabledConvexPairs.size() - 1, true);
        }

        while (!_disabledConcavePairs.empty()) {
            removeDisabledConcavePairWithIndex(_disabledConcavePairs.size() - 1, true);
        }
    }

    void OverlappingPairs::EnablePair(u64 pairID) {
        VASSERT(IsPairDisabled(pairID), "Trying to enable a pair that is not disabled.");
        VASSERT(!_convexPairIDToPairIndexMap.contains(pairID) && !_concavePairIDToPairIndexMap.contains(pairID),
                "Trying to enable a pair ID that already exists in the convex pairs or concave pairs.");

        auto itConvex = _disabledConvexPairIDToPairIndexMap.find(pairID);
        if (itConvex != _disabledConvexPairIDToPairIndexMap.end()) {
            EnableConvexPairWithIndex(itConvex->second);
        } else {
            auto itConcave = _disabledConcavePairIDToPairIndexMap.find(pairID);
            VASSERT(itConcave != _disabledConcavePairIDToPairIndexMap.end(),
                    "Trying to enable a pair ID that does not exist in the disabled convex pairs or disabled concave pairs.");
            EnableConcavePairWithIndex(itConcave->second);
        }
    }

    void OverlappingPairs::DisablePair(u64 pairID) {
        VASSERT(!IsPairDisabled(pairID), "Trying to disable a pair that is already disabled.");
        VASSERT(_convexPairIDToPairIndexMap.contains(pairID) || _concavePairIDToPairIndexMap.contains(pairID),
                "Trying to disable a pair ID that does not exist in the convex pairs or concave pairs.");

        auto itConvex = _convexPairIDToPairIndexMap.find(pairID);
        if (itConvex != _convexPairIDToPairIndexMap.end()) {
            DisableConvexPairWithIndex(itConvex->second);
        } else {
            auto itConcave = _concavePairIDToPairIndexMap.find(pairID);
            VASSERT(itConcave != _concavePairIDToPairIndexMap.end(), "Trying to disable a pair ID that does not exist in the convex pairs or concave pairs.");
            DisableConcavePairWithIndex(itConcave->second);
        }
    }

    void OverlappingPairs::EnableConvexPairWithIndex(size_t pairIndex) {
        ConvexOverlappingPair &pairToEnable = _disabledConvexPairs[pairIndex];

        VASSERT(!pairToEnable.IsEnabled, "Trying to enable a convex pair that is already enabled.");

        const size_t newIndex = _convexPairs.size();
        _convexPairIDToPairIndexMap[pairToEnable.PairID] = newIndex;

        _convexPairs.emplace_back(pairToEnable.PairID,
                                  pairToEnable.ColliderOneBroadPhaseID,
                                  pairToEnable.ColliderTwoBroadPhaseID,
                                  pairToEnable.ColliderOneEntity,
                                  pairToEnable.ColliderTwoEntity,
                                  pairToEnable.NarrowPhaseAlgorithmToUse,
                                  true);
        _convexPairs[newIndex].AreCollidingThisFrame = pairToEnable.AreCollidingThisFrame;
        _convexPairs[newIndex].WereCollidingLastFrame = pairToEnable.WereCollidingLastFrame;

        removeDisabledConvexPairWithIndex(pairIndex, false);
    }

    void OverlappingPairs::DisableConvexPairWithIndex(size_t pairIndex) {
        ConvexOverlappingPair &pairToDisable = _convexPairs[pairIndex];

        VASSERT(pairToDisable.IsEnabled, "Trying to disable a convex pair that is already disabled.");

        const size_t newIndex = _disabledConvexPairs.size();
        _disabledConvexPairIDToPairIndexMap[pairToDisable.PairID] = newIndex;

        _disabledConvexPairs.emplace_back(pairToDisable.PairID,
                                          pairToDisable.ColliderOneBroadPhaseID,
                                          pairToDisable.ColliderTwoBroadPhaseID,
                                          pairToDisable.ColliderOneEntity,
                                          pairToDisable.ColliderTwoEntity,
                                          pairToDisable.NarrowPhaseAlgorithmToUse,
                                          false);
        _disabledConvexPairs[newIndex].AreCollidingThisFrame = pairToDisable.AreCollidingThisFrame;
        _disabledConvexPairs[newIndex].WereCollidingLastFrame = pairToDisable.WereCollidingLastFrame;

        RemoveConvexPairWithIndex(pairIndex, false);
    }

    void OverlappingPairs::EnableConcavePairWithIndex(size_t pairIndex) {
        ConcaveOverlappingPair &pairToEnable = _disabledConcavePairs[pairIndex];

        VASSERT(!pairToEnable.IsEnabled, "Trying to enable a concave pair that is already enabled.");

        const size_t newIndex = _concavePairs.size();
        _concavePairIDToPairIndexMap[pairToEnable.PairID] = newIndex;

        _concavePairs.emplace_back(pairToEnable.PairID,
                                   pairToEnable.ColliderOneBroadPhaseID,
                                   pairToEnable.ColliderTwoBroadPhaseID,
                                   pairToEnable.ColliderOneEntity,
                                   pairToEnable.ColliderTwoEntity,
                                   pairToEnable.NarrowPhaseAlgorithmToUse,
                                   pairToEnable.IsFirstShapeConvex,
                                   true);
        _concavePairs[newIndex].AreCollidingThisFrame = pairToEnable.AreCollidingThisFrame;
        _concavePairs[newIndex].WereCollidingLastFrame = pairToEnable.WereCollidingLastFrame;

        removeDisabledConcavePairWithIndex(pairIndex, false);
    }

    void OverlappingPairs::DisableConcavePairWithIndex(size_t pairIndex) {
        ConcaveOverlappingPair &pairToDisable = _concavePairs[pairIndex];

        VASSERT(pairToDisable.IsEnabled, "Trying to disable a concave pair that is already disabled.");

        const size_t newIndex = _disabledConcavePairs.size();
        _disabledConcavePairIDToPairIndexMap[pairToDisable.PairID] = newIndex;

        _disabledConcavePairs.emplace_back(pairToDisable.PairID,
                                           pairToDisable.ColliderOneBroadPhaseID,
                                           pairToDisable.ColliderTwoBroadPhaseID,
                                           pairToDisable.ColliderOneEntity,
                                           pairToDisable.ColliderTwoEntity,
                                           pairToDisable.NarrowPhaseAlgorithmToUse,
                                           pairToDisable.IsFirstShapeConvex,
                                           false);
        _disabledConcavePairs[newIndex].AreCollidingThisFrame = pairToDisable.AreCollidingThisFrame;
        _disabledConcavePairs[newIndex].WereCollidingLastFrame = pairToDisable.WereCollidingLastFrame;

        RemoveConcavePairWithIndex(pairIndex, false);
    }

    size_t OverlappingPairs::AddPair(size_t colliderOneIndex, size_t colliderTwoIndex, bool isConvexPair) {
        VASSERT(_colliderComponentStore.GetBroadPhaseIDAtIndex(colliderOneIndex) && _colliderComponentStore.GetBroadPhaseIDAtIndex(colliderTwoIndex),
                "Trying to add a pair with broad-phase IDs that do not exist in the collider component store.");

        const CollisionShape *collisionShapeOne = &_colliderComponentStore.GetCollisionShapeAtIndex(colliderOneIndex);
        const CollisionShape *collisionShapeTwo = &_colliderComponentStore.GetCollisionShapeAtIndex(colliderTwoIndex);

        const Entity colliderOneEntity = _colliderComponentStore.GetEntityAtIndex(colliderOneIndex);
        const Entity colliderTwoEntity = _colliderComponentStore.GetEntityAtIndex(colliderTwoIndex);

        const u32 colliderOneBroadPhaseID = _colliderComponentStore.GetBroadPhaseIDAtIndex(colliderOneIndex);
        const u32 colliderTwoBroadPhaseID = _colliderComponentStore.GetBroadPhaseIDAtIndex(colliderTwoIndex);

        const u64 pairID = PairNumbers(std::max(colliderOneBroadPhaseID, colliderTwoBroadPhaseID), std::min(colliderOneBroadPhaseID, colliderTwoBroadPhaseID));

        if (isConvexPair) {
            VASSERT(!_convexPairIDToPairIndexMap.contains(pairID), "Trying to add a convex pair with a pair ID that already exists in the convex pairs.");
            NarrowPhaseAlgorithm narrowPhaseAlgorithmToUse =
                _collisionDispatch.SelectNarrowPhaseAlgorithm(collisionShapeOne->GetType(), collisionShapeTwo->GetType());
            _convexPairIDToPairIndexMap[pairID] = _convexPairs.size();

            _convexPairs.emplace_back(
                pairID, colliderOneBroadPhaseID, colliderTwoBroadPhaseID, colliderOneEntity, colliderTwoEntity, narrowPhaseAlgorithmToUse, true);
        } else {
            const bool isFirstShapeConvex = collisionShapeOne->IsConvex();

            VASSERT(!_concavePairIDToPairIndexMap.contains(pairID), "Trying to add a concave pair with a pair ID that already exists in the concave pairs.");

            NarrowPhaseAlgorithm narrowPhaseAlgorithm = _collisionDispatch.SelectNarrowPhaseAlgorithm(
                isFirstShapeConvex ? collisionShapeOne->GetType() : collisionShapeTwo->GetType(), CollisionShapeType::ConvexPolyhedron);

            _concavePairIDToPairIndexMap[pairID] = _concavePairs.size();

            _concavePairs.emplace_back(
                pairID, colliderOneBroadPhaseID, colliderTwoBroadPhaseID, colliderOneEntity, colliderTwoEntity, narrowPhaseAlgorithm, isFirstShapeConvex, true);
        }

        std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairsAtIndex(colliderOneIndex);
        std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairsAtIndex(colliderTwoIndex);

#if defined(VULKYRIE_DEBUG)
        VASSERT(std::find(overlappingPairsOne.begin(), overlappingPairsOne.end(), pairID) == overlappingPairsOne.end(),
                "Trying to add a pair with a pair ID that already exists in the overlapping pairs of the first collider.");

        VASSERT(std::find(overlappingPairsTwo.begin(), overlappingPairsTwo.end(), pairID) == overlappingPairsTwo.end(),
                "Trying to add a pair with a pair ID that already exists in the overlapping pairs of the second collider.");
#endif

        overlappingPairsOne.push_back(pairID);
        overlappingPairsTwo.push_back(pairID);

        return pairID;
    }

    void OverlappingPairs::RemovePair(u64 pairID) {
        VASSERT(
            _convexPairIDToPairIndexMap.contains(pairID) || _concavePairIDToPairIndexMap.contains(pairID) ||
                _disabledConvexPairIDToPairIndexMap.contains(pairID) || _disabledConcavePairIDToPairIndexMap.contains(pairID),
            "Trying to remove a pair with a pair ID that does not exist in the convex pairs, concave pairs, disabled convex pairs, or disabled concave pairs.");

        auto itConvex = _convexPairIDToPairIndexMap.find(pairID);
        if (itConvex != _convexPairIDToPairIndexMap.end()) {
            RemoveConvexPairWithIndex(itConvex->second, true);
            return;
        }

        auto itConcave = _concavePairIDToPairIndexMap.find(pairID);
        if (itConcave != _concavePairIDToPairIndexMap.end()) {
            RemoveConcavePairWithIndex(itConcave->second, true);
            return;
        }

        auto itDisabledConvex = _disabledConvexPairIDToPairIndexMap.find(pairID);
        if (itDisabledConvex != _disabledConvexPairIDToPairIndexMap.end()) {
            removeDisabledConvexPairWithIndex(itDisabledConvex->second, true);
            return;
        }

        auto itDisabledConcave = _disabledConcavePairIDToPairIndexMap.find(pairID);
        if (itDisabledConcave != _disabledConcavePairIDToPairIndexMap.end()) {
            removeDisabledConcavePairWithIndex(itDisabledConcave->second, true);
            return;
        }
    }

    void OverlappingPairs::RemoveConvexPairWithIndex(size_t pairIndex, bool removeFromColliders) {
        const size_t convexPairsCount = _convexPairs.size();

#if defined(VULKYRIE_DEBUG)
        std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(_convexPairs[pairIndex].ColliderOneEntity);
        std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(_convexPairs[pairIndex].ColliderTwoEntity);

        VASSERT(pairIndex < convexPairsCount, "Trying to remove a convex pair with an index that is out of bounds.");
        VASSERT(std::find(overlappingPairsOne.begin(), overlappingPairsOne.end(), _convexPairs[pairIndex].PairID) != overlappingPairsOne.end(),
                "Convex pair ID not found in the overlapping pairs of the first collider when trying to remove a convex pair.");

        VASSERT(std::find(overlappingPairsTwo.begin(), overlappingPairsTwo.end(), _convexPairs[pairIndex].PairID) != overlappingPairsTwo.end(),
                "Convex pair ID not found in the overlapping pairs of the second collider when trying to remove a convex pair.");
#endif

        if (removeFromColliders) {
            const ConvexOverlappingPair &pairToRemove = _convexPairs[pairIndex];
            std::vector<u64> &collisionPairOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
            std::vector<u64> &collisionPairTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

            // Remove the pair ID from the collision pairs of both colliders
            std::erase(collisionPairOne, pairToRemove.PairID);
            std::erase(collisionPairTwo, pairToRemove.PairID);
        }

        VASSERT(_convexPairIDToPairIndexMap[_convexPairs[pairIndex].PairID] == pairIndex,
                "Convex pair index map is out of sync when trying to remove a convex pair.");

        _convexPairIDToPairIndexMap.erase(_convexPairs[pairIndex].PairID);

        if (_convexPairs.size() > 1 && pairIndex != convexPairsCount - 1) {
            _convexPairIDToPairIndexMap[_convexPairs[convexPairsCount - 1].PairID] = pairIndex;
        }

        // Swap the pair to remove with the last pair and pop back
        // This is done to avoid shifting all elements after the removed pair,
        // which would be more expensive. Order of pairs is not preserved.
        _convexPairs[pairIndex] = _convexPairs[convexPairsCount - 1];
        _convexPairs.pop_back();
    }

    void OverlappingPairs::RemoveConcavePairWithIndex(size_t pairIndex, bool removeFromColliders) {
        const size_t concavePairsCount = _concavePairs.size();

#if defined(VULKYRIE_DEBUG)
        std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(_concavePairs[pairIndex].ColliderOneEntity);
        std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(_concavePairs[pairIndex].ColliderTwoEntity);

        VASSERT(pairIndex < concavePairsCount, "Trying to remove a concave pair with an index that is out of bounds.");
        VASSERT(std::find(overlappingPairsOne.begin(), overlappingPairsOne.end(), _concavePairs[pairIndex].PairID) != overlappingPairsOne.end(),
                "Concave pair ID not found in the overlapping pairs of the first collider when trying to remove a concave pair.");
        VASSERT(std::find(overlappingPairsTwo.begin(), overlappingPairsTwo.end(), _concavePairs[pairIndex].PairID) != overlappingPairsTwo.end(),
                "Concave pair ID not found in the overlapping pairs of the second collider when trying to remove a concave pair.");
#endif

        if (removeFromColliders) {
            const ConcaveOverlappingPair &pairToRemove = _concavePairs[pairIndex];
            std::vector<u64> &collisionPairOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
            std::vector<u64> &collisionPairTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

            // Remove the pair ID from the collision pairs of both colliders
            std::erase(collisionPairOne, pairToRemove.PairID);
            std::erase(collisionPairTwo, pairToRemove.PairID);
        }

        VASSERT(_concavePairIDToPairIndexMap[_concavePairs[pairIndex].PairID] == pairIndex,
                "Concave pair index map is out of sync when trying to remove a concave pair.");

        _concavePairIDToPairIndexMap.erase(_concavePairs[pairIndex].PairID);

        _concavePairs[pairIndex].DestroyLastFrameCollisionData();

        if (_concavePairs.size() > 1 && pairIndex != concavePairsCount - 1) {
            _concavePairIDToPairIndexMap[_concavePairs[concavePairsCount - 1].PairID] = pairIndex;
        }

        // Swap the pair to remove with the last pair and pop back
        // This is done to avoid shifting all elements after the removed pair,
        // which would be more expensive. Order of pairs is not preserved.
        _concavePairs[pairIndex] = _concavePairs[concavePairsCount - 1];
        _concavePairs.pop_back();
    }

    void OverlappingPairs::ClearObsoleteLastFrameCollisionData() {
        for (auto &pair : _concavePairs) {
            pair.ClearObsoleteLastFrameCollisionData();
        }
    }

    void OverlappingPairs::UpdateCollidingInLastFrame() {
        for (auto &pair : _convexPairs) {
            pair.WereCollidingLastFrame = pair.AreCollidingThisFrame;
        }

        for (auto &pair : _concavePairs) {
            pair.WereCollidingLastFrame = pair.AreCollidingThisFrame;
        }
    }

    void OverlappingPairs::removeDisabledConvexPairWithIndex(size_t pairIndex, bool removeFromColliders) {
        const ConvexOverlappingPair &pairToRemove = _disabledConvexPairs[pairIndex];

#if defined(VULKYRIE_DEBUG)
        std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
        std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

        VASSERT(pairIndex < _disabledConvexPairs.size(), "Trying to remove a disabled convex pair with an index that is out of bounds.");
        VASSERT(std::find(overlappingPairsOne.begin(), overlappingPairsOne.end(), pairToRemove.PairID) != overlappingPairsOne.end(),
                "Disabled convex pair ID not found in the overlapping pairs of the first collider when trying to remove a disabled convex pair.");
        VASSERT(std::find(overlappingPairsTwo.begin(), overlappingPairsTwo.end(), pairToRemove.PairID) != overlappingPairsTwo.end(),
                "Disabled convex pair ID not found in the overlapping pairs of the second collider when trying to remove a disabled convex pair.");
#endif

        if (removeFromColliders) {
            const ConvexOverlappingPair &pairToRemove = _disabledConvexPairs[pairIndex];
            std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
            std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

            // Remove the pair ID from the collision pairs of both colliders
            std::erase(overlappingPairsOne, pairToRemove.PairID);
            std::erase(overlappingPairsTwo, pairToRemove.PairID);
        }

        VASSERT(_disabledConvexPairIDToPairIndexMap[pairToRemove.PairID] == pairIndex,
                "Disabled convex pair index map is out of sync when trying to remove a disabled convex pair.");

        _disabledConvexPairIDToPairIndexMap.erase(pairToRemove.PairID);
        const size_t disabledConvexPairsCount = _disabledConvexPairs.size();

        if (disabledConvexPairsCount > 1 && pairIndex != disabledConvexPairsCount - 1) {
            _disabledConvexPairIDToPairIndexMap[_disabledConvexPairs[disabledConvexPairsCount - 1].PairID] = pairIndex;
        }

        // Swap the pair to remove with the last pair and pop back
        // This is done to avoid shifting all elements after the removed pair,
        // which would be more expensive. Order of pairs is not preserved.
        _disabledConvexPairs[pairIndex] = _disabledConvexPairs[disabledConvexPairsCount - 1];
        _disabledConvexPairs.pop_back();
    }

    void OverlappingPairs::removeDisabledConcavePairWithIndex(size_t pairIndex, bool removeFromColliders) {
        const ConcaveOverlappingPair &pairToRemove = _disabledConcavePairs[pairIndex];

#if defined(VULKYRIE_DEBUG)
        std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
        std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

        VASSERT(pairIndex < _disabledConcavePairs.size(), "Trying to remove a disabled concave pair with an index that is out of bounds.");
        VASSERT(std::find(overlappingPairsOne.begin(), overlappingPairsOne.end(), pairToRemove.PairID) != overlappingPairsOne.end(),
                "Disabled concave pair ID not found in the overlapping pairs of the first collider when trying to remove a disabled concave pair.");
        VASSERT(std::find(overlappingPairsTwo.begin(), overlappingPairsTwo.end(), pairToRemove.PairID) != overlappingPairsTwo.end(),
                "Disabled concave pair ID not found in the overlapping pairs of the second collider when trying to remove a disabled concave pair.");
#endif

        if (removeFromColliders) {
            std::vector<u64> &overlappingPairsOne = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderOneEntity);
            std::vector<u64> &overlappingPairsTwo = _colliderComponentStore.GetOverlappingPairs(pairToRemove.ColliderTwoEntity);

            // Remove the pair ID from the collision pairs of both colliders
            std::erase(overlappingPairsOne, pairToRemove.PairID);
            std::erase(overlappingPairsTwo, pairToRemove.PairID);
        }

        VASSERT(_disabledConcavePairIDToPairIndexMap[pairToRemove.PairID] == pairIndex,
                "Disabled concave pair index map is out of sync when trying to remove a disabled concave pair.");

        _disabledConcavePairIDToPairIndexMap.erase(pairToRemove.PairID);
        const size_t disabledConcavePairsCount = _disabledConcavePairs.size();

        if (disabledConcavePairsCount > 1 && pairIndex != disabledConcavePairsCount - 1) {
            _disabledConcavePairIDToPairIndexMap[_disabledConcavePairs[disabledConcavePairsCount - 1].PairID] = pairIndex;
        }

        // Swap the pair to remove with the last pair and pop back
        // This is done to avoid shifting all elements after the removed pair,
        // which would be more expensive. Order of pairs is not preserved.
        _disabledConcavePairs[pairIndex] = _disabledConcavePairs[disabledConcavePairsCount - 1];
        _disabledConcavePairs.pop_back();
    }

} // namespace Vulkyrie
