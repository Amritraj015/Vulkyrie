#pragma once

#include "core/entity.h"
#include "physics/physics_world.h"
#include "physics/collision/narrowphase/collision_dispatcher.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    struct LastFrameCollisionInfo {
            bool IsValid;
            bool IsObsolete;
            bool WasColliding;
            bool WasUsingGJKAlgorithm;
            bool WasUsingSATAlgorithm;

            glm::vec3 GJKSeparatingAxis;

            LastFrameCollisionInfo()
                : IsValid(false)
                , IsObsolete(false)
                , WasColliding(false)
                , WasUsingGJKAlgorithm(false)
                , WasUsingSATAlgorithm(false)
                , GJKSeparatingAxis(glm::vec3(0, 1, 0)) {
                // , satIsAxisFacePolyhedron1(false)
                // , satIsAxisFacePolyhedron2(false)
                // , satMinAxisFaceIndex(0)
                // , satMinEdge1Index(0)
                // , satMinEdge2Index(0) {
            }
    };

    struct OverlappingPair {
        public:
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
            Vulkyrie::LastFrameCollisionInfo LastFrameCollisionInfo;

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
            std::unordered_map<size_t, LastFrameCollisionInfo *> LastFrameCollisionInfoMap;

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
                , LastFrameCollisionInfoMap() {
            }
    };

    class OverlappingPairs final {
        public:
            explicit OverlappingPairs(PhysicsWorld &physicsWorld, std::unordered_set<std::pair<Entity, Entity>> &pairsThatCannotCollide);

            OverlappingPairs(const OverlappingPairs &) = delete;
            OverlappingPairs &operator=(const OverlappingPairs &) = delete;

            OverlappingPairs(OverlappingPairs &&) = delete;
            OverlappingPairs &operator=(OverlappingPairs &&) = delete;

            ~OverlappingPairs() = default;

            void EnablePair(size_t pairID);
            void DisablePair(size_t pairID);
            void EnableConvexPairWithIndex(size_t pairIndex);
            void DisableConvexPairWithIndex(size_t pairIndex);
            bool IsPairDisabled(size_t pairID) const;
            size_t AddPair(i32 colliderOneBroadPhaseID, i32 colliderTwoBroadPhaseID, bool isConvexPair);
            void RemovePair(size_t pairID);
            void RemoveConvexPairWithIndex(size_t pairIndex, bool removeFromColliders = true);
            void RemoveLastFrameCollisionData();
            void UpdateCollidingInLastFrame();
            void SetRequiresCollisionCheck(size_t pairID, bool requiresCollisionCheck);
            OverlappingPair &GetPair(size_t pairID);

            VE_FORCE_INLINE static std::pair<Entity, Entity> Compute(Entity bodyOneEntity, Entity bodyTwoEntity) {
                VASSERT(bodyOneEntity != bodyTwoEntity, "Cannot compute overlapping pair for the same entity.");

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
            std::unordered_map<i32, size_t> _convexPairIDToIndexMap;
            std::unordered_map<i32, size_t> _disabledConvexPairIDToIndexMap;
            std::unordered_map<i32, size_t> _concavePairIDToIndexMap;
            std::unordered_map<i32, size_t> _disabledConcavePairIDToIndexMap;

            BodyComponentStore &_bodyComponentStore;
            ColliderComponentStore &_colliderComponentStore;
            RigidBodyComponentStore &_rigidBodyComponentStore;
            std::unordered_set<std::pair<Entity, Entity>> &_pairsThatCannotCollide;
    };

} // namespace Vulkyrie
