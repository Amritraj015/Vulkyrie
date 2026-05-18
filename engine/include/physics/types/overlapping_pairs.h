#pragma once

#include "core/asserts.h"
#include "core/entity.h"
#include "core/pair.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/types/collision_dispatch.h"
#include "physics/types/narrow_phase_algorithm.h"
#include "physics/types/last_frame_collision_data.h"

namespace Vulkyrie {

    class PhysicsWorld;

    struct OverlappingPair {
        size_t PairID;
        i32 ColliderOneBroadPhaseID;
        i32 ColliderTwoBroadPhaseID;
        Entity ColliderOneEntity;
        Entity ColliderTwoEntity;
        bool RequiresCollisionCheck;
        NarrowPhaseAlgorithm NarrowPhaseAlgorithmToUse;
        bool WereCollidingLastFrame;
        bool AreCollidingThisFrame;
        bool IsEnabled;

        explicit OverlappingPair(size_t pairID,
                                 i32 colliderOneBroadPhaseID,
                                 i32 colliderTwoBroadPhaseID,
                                 Entity colliderOneEntity,
                                 Entity colliderTwoEntity,
                                 NarrowPhaseAlgorithm narrowPhaseAlgorithmToUse,
                                 bool isEnabled)
            : PairID(pairID)
            , ColliderOneBroadPhaseID(colliderOneBroadPhaseID)
            , ColliderTwoBroadPhaseID(colliderTwoBroadPhaseID)
            , ColliderOneEntity(colliderOneEntity)
            , ColliderTwoEntity(colliderTwoEntity)
            , RequiresCollisionCheck(false)
            , NarrowPhaseAlgorithmToUse(narrowPhaseAlgorithmToUse)
            , WereCollidingLastFrame(false)
            , AreCollidingThisFrame(false)
            , IsEnabled(isEnabled) {
        }

        virtual ~OverlappingPair() = default;
    };

    struct ConvexOverlappingPair : public OverlappingPair {
    public:
        Vulkyrie::LastFrameCollisionData LastFrameCollisionInfo;

        explicit ConvexOverlappingPair(size_t pairID,
                                       i32 colliderOneBroadPhaseID,
                                       i32 colliderTwoBroadPhaseID,
                                       Entity colliderOneEntity,
                                       Entity colliderTwoEntity,
                                       NarrowPhaseAlgorithm narrowPhaseAlgorithmToUse,
                                       bool isEnabled)
            : OverlappingPair(
                  pairID, colliderOneBroadPhaseID, colliderTwoBroadPhaseID, colliderOneEntity, colliderTwoEntity, narrowPhaseAlgorithmToUse, isEnabled)
            , LastFrameCollisionInfo() {};
    };

    struct ConcaveOverlappingPair : public OverlappingPair {
    public:
        bool IsFirstShapeConvex;
        std::unordered_map<size_t, LastFrameCollisionData *> LastFrameCollisionDataMap;

        explicit ConcaveOverlappingPair(size_t pairID,
                                        i32 colliderOneBroadPhaseID,
                                        i32 colliderTwoBroadPhaseID,
                                        Entity colliderOneEntity,
                                        Entity colliderTwoEntity,
                                        NarrowPhaseAlgorithm narrowPhaseAlgorithmToUse,
                                        bool isEnabled,
                                        bool isFirstShapeConvex)
            : OverlappingPair(
                  pairID, colliderOneBroadPhaseID, colliderTwoBroadPhaseID, colliderOneEntity, colliderTwoEntity, narrowPhaseAlgorithmToUse, isEnabled)
            , IsFirstShapeConvex(isFirstShapeConvex)
            , LastFrameCollisionDataMap() {
        }

        void DestroyLastFrameCollisionData() {
            for (auto &entry : LastFrameCollisionDataMap) {
                delete entry.second;
            }

            LastFrameCollisionDataMap.clear();
        }

        LastFrameCollisionData *AddLastFrameCollisionDataIfNecessary(u32 shapeOneID, u32 shapeTwoID) {
            u32 maxShapeID = shapeOneID;
            u32 minShapeID = shapeTwoID;

            if (shapeTwoID > shapeOneID) {
                maxShapeID = shapeTwoID;
                minShapeID = shapeOneID;
            }

            const u64 shapeIDPair = PairNumbers(maxShapeID, minShapeID);

            auto it = LastFrameCollisionDataMap.find(shapeIDPair);

            if (it != LastFrameCollisionDataMap.end()) {
                it->second->IsObsolete = false;
                return it->second;
            } else {
                LastFrameCollisionData *newData = new LastFrameCollisionData();
                LastFrameCollisionDataMap[shapeIDPair] = newData;
                return newData;
            }
        }

        void ClearObsoleteLastFrameCollisionData() {
            for (auto it = LastFrameCollisionDataMap.begin(); it != LastFrameCollisionDataMap.end();) {
                if (it->second->IsObsolete) {
                    delete it->second;
                    it = LastFrameCollisionDataMap.erase(it);
                } else {
                    it->second->IsObsolete = true;
                    ++it;
                }
            }
        }
    };

    class OverlappingPairs final {
    public:
        explicit OverlappingPairs(PhysicsWorld &physicsWorld,
                                  std::unordered_set<Pair<Entity, Entity>> &pairsThatCannotCollide,
                                  CollisionDispatch &collisionDispatch);

        // Delete the copy constructor and copy assignment operator.
        OverlappingPairs(const OverlappingPairs &) = delete;
        OverlappingPairs &operator=(const OverlappingPairs &) = delete;

        // Delete the move constructor and move assignment operator.
        OverlappingPairs(OverlappingPairs &&) = delete;
        OverlappingPairs &operator=(OverlappingPairs &&) = delete;

        /** @brief Default destructor for OverlappingPairs. */
        ~OverlappingPairs() = default;

        void EnablePair(size_t pairID);
        void DisablePair(size_t pairID);
        void EnableConvexPairWithIndex(size_t pairIndex);
        void DisableConvexPairWithIndex(size_t pairIndex);
        void EnableConcavePairWithIndex(size_t pairIndex);
        void DisableConcavePairWithIndex(size_t pairIndex);
        size_t AddPair(i32 colliderOneIndex, i32 colliderTwoIndex, bool isConvexPair);
        void RemovePair(size_t pairID);
        void RemoveConvexPairWithIndex(size_t pairIndex, bool removeFromColliders = true);
        void RemoveConcavePairWithIndex(size_t pairIndex, bool removeFromColliders = true);
        void ClearObsoleteLastFrameCollisionData();
        void UpdateCollidingInLastFrame();

        [[nodiscard]] VE_FORCE_INLINE bool IsPairDisabled(size_t pairID) const {
            return _disabledConvexPairIDToPairIndexMap.contains(pairID) || _disabledConcavePairIDToPairIndexMap.contains(pairID);
        }

        VE_FORCE_INLINE void SetRequiresCollisionCheck(size_t pairID, bool requiresCollisionCheck) {
            VASSERT(_convexPairIDToPairIndexMap.contains(pairID) || _concavePairIDToPairIndexMap.contains(pairID),
                    "Trying to set requires collision check for a pair ID that does not exist.");

            auto it = _convexPairIDToPairIndexMap.find(pairID);

            if (it != _convexPairIDToPairIndexMap.end()) {
                _convexPairs[it->second].RequiresCollisionCheck = requiresCollisionCheck;
            } else {
                _concavePairs[_concavePairIDToPairIndexMap[pairID]].RequiresCollisionCheck = requiresCollisionCheck;
            }
        }

        [[nodiscard]] VE_FORCE_INLINE OverlappingPair *GetOverlappingPair(size_t pairID) {
            auto it = _convexPairIDToPairIndexMap.find(pairID);
            if (it != _convexPairIDToPairIndexMap.end()) {
                return &_convexPairs[it->second];
            }

            it = _concavePairIDToPairIndexMap.find(pairID);
            if (it != _concavePairIDToPairIndexMap.end()) {
                return &_concavePairs[it->second];
            }

            it = _disabledConvexPairIDToPairIndexMap.find(pairID);
            if (it != _disabledConvexPairIDToPairIndexMap.end()) {
                return &_disabledConvexPairs[it->second];
            }

            it = _disabledConcavePairIDToPairIndexMap.find(pairID);
            if (it != _disabledConcavePairIDToPairIndexMap.end()) {
                return &_disabledConcavePairs[it->second];
            }

            return nullptr;
        }

        VE_FORCE_INLINE static Pair<Entity, Entity> ComputeBodiesIndexPair(Entity bodyOneEntity, Entity bodyTwoEntity) {
            VASSERT(bodyOneEntity != bodyTwoEntity, "Cannot compute bodies index pair for the same entity.");

            if (bodyOneEntity.GetID() < bodyTwoEntity.GetID()) {
                return { bodyOneEntity, bodyTwoEntity };
            } else {
                return { bodyTwoEntity, bodyOneEntity };
            }
        }

    private:
        std::vector<ConvexOverlappingPair> _convexPairs;
        std::vector<ConvexOverlappingPair> _disabledConvexPairs;
        std::vector<ConcaveOverlappingPair> _concavePairs;
        std::vector<ConcaveOverlappingPair> _disabledConcavePairs;
        std::unordered_map<i32, size_t> _convexPairIDToPairIndexMap;
        std::unordered_map<i32, size_t> _disabledConvexPairIDToPairIndexMap;
        std::unordered_map<i32, size_t> _concavePairIDToPairIndexMap;
        std::unordered_map<i32, size_t> _disabledConcavePairIDToPairIndexMap;
        BodyComponentStore &_bodyComponentStore;
        ColliderComponentStore &_colliderComponentStore;
        RigidBodyComponentStore &_rigidBodyComponentStore;
        CollisionDispatch &_collisionDispatch;
        std::unordered_set<Pair<Entity, Entity>> &_pairsThatCannotCollide;

        void removeDisabledConvexPairWithIndex(size_t pairIndex, bool removeFromColliders);
        void removeDisabledConcavePairWithIndex(size_t pairIndex, bool removeFromColliders);
    };

} // namespace Vulkyrie
