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

    /**
     * @brief Base struct representing a broad-phase overlapping pair.
     *
     * Produced by the broad-phase AABB tree when two collider bounding volumes overlap.
     * Stores the shared state used by all narrow-phase algorithms. Derived structs add
     * per-algorithm cached data for warm-starting the solver across frames.
     */
    struct OverlappingPair {
        /** @brief Unique pair identifier computed as PairNumbers(max(bp1, bp2), min(bp1, bp2)). */
        u64 PairID;

        /** @brief Broad-phase AABB tree ID of the first collider. */
        i32 ColliderOneBroadPhaseID;

        /** @brief Broad-phase AABB tree ID of the second collider. */
        i32 ColliderTwoBroadPhaseID;

        /** @brief Entity handle for the first collider component. */
        Entity ColliderOneEntity;

        /** @brief Entity handle for the second collider component. */
        Entity ColliderTwoEntity;

        /** @brief True if this pair requires a narrow-phase test this frame. */
        bool RequiresCollisionCheck;

        /** @brief Algorithm selected by the collision dispatcher for this shape-type combination. */
        NarrowPhaseAlgorithm NarrowPhaseAlgorithmToUse;

        /** @brief True if the pair was colliding during the previous simulation step. */
        bool WereCollidingLastFrame;

        /** @brief True if the pair is colliding during the current simulation step. */
        bool AreCollidingThisFrame;

        /** @brief False if collision response is suppressed (e.g., trigger volume or filtered pair). */
        bool IsEnabled;

        /**
         * @brief Constructs an OverlappingPair with all required fields.
         *
         * @param pairID                    Unique pair identifier (see PairNumbers).
         * @param colliderOneBroadPhaseID   Broad-phase ID of the first collider.
         * @param colliderTwoBroadPhaseID   Broad-phase ID of the second collider.
         * @param colliderOneEntity         Entity handle for the first collider.
         * @param colliderTwoEntity         Entity handle for the second collider.
         * @param narrowPhaseAlgorithmToUse Algorithm chosen by the collision dispatcher.
         * @param isEnabled                 Whether this pair participates in collision resolution.
         */
        explicit OverlappingPair(u64 pairID,
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

    /**
     * @brief Overlapping pair for two convex collision shapes.
     *
     * Extends OverlappingPair with a single LastFrameCollisionData entry that caches
     * the previous frame's GJK/EPA result, enabling warm-starting of the narrow-phase
     * solver for faster convergence.
     */
    struct ConvexOverlappingPair : public OverlappingPair {
    public:
        /** @brief Cached GJK/EPA result from the previous frame, used to warm-start the narrow-phase solver. */
        Vulkyrie::LastFrameCollisionData LastFrameCollisionInfo;

        /**
         * @brief Constructs a ConvexOverlappingPair.
         *
         * @param pairID                    Unique pair identifier (see PairNumbers).
         * @param colliderOneBroadPhaseID   Broad-phase ID of the first collider.
         * @param colliderTwoBroadPhaseID   Broad-phase ID of the second collider.
         * @param colliderOneEntity         Entity handle for the first collider.
         * @param colliderTwoEntity         Entity handle for the second collider.
         * @param narrowPhaseAlgorithmToUse Algorithm chosen by the collision dispatcher.
         * @param isEnabled                 Whether this pair participates in collision resolution.
         */
        explicit ConvexOverlappingPair(u64 pairID,
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

    /**
     * @brief Overlapping pair where one collision shape is a concave triangle mesh.
     *
     * Because a concave mesh decomposes into many convex triangles, each overlapping
     * sub-shape (triangle) must be tested independently in the narrow phase. A map of
     * LastFrameCollisionData keyed by a canonical sub-shape pair ID enables per-triangle
     * warm-starting of GJK/EPA across frames. Stale entries are pruned each frame by
     * ClearObsoleteLastFrameCollisionData().
     */
    struct ConcaveOverlappingPair : public OverlappingPair {
    public:
        /** @brief True if collider one is the convex shape; false if collider two is the convex shape. */
        bool IsFirstShapeConvex;

        /** @brief Per-sub-shape-pair collision cache. Key = PairNumbers(max, min) of sub-shape IDs. Owns the raw pointers. */
        std::unordered_map<size_t, LastFrameCollisionData *> LastFrameCollisionDataMap;

        /**
         * @brief Constructs a ConcaveOverlappingPair.
         *
         * @param pairID                    Unique pair identifier (see PairNumbers).
         * @param colliderOneBroadPhaseID   Broad-phase ID of the first collider.
         * @param colliderTwoBroadPhaseID   Broad-phase ID of the second collider.
         * @param colliderOneEntity         Entity handle for the first collider.
         * @param colliderTwoEntity         Entity handle for the second collider.
         * @param narrowPhaseAlgorithmToUse Algorithm chosen by the collision dispatcher.
         * @param isFirstShapeConvex        True if collider one is the convex shape.
         * @param isEnabled                 Whether this pair participates in collision resolution.
         */
        explicit ConcaveOverlappingPair(u64 pairID,
                                        i32 colliderOneBroadPhaseID,
                                        i32 colliderTwoBroadPhaseID,
                                        Entity colliderOneEntity,
                                        Entity colliderTwoEntity,
                                        NarrowPhaseAlgorithm narrowPhaseAlgorithmToUse,
                                        bool isFirstShapeConvex,
                                        bool isEnabled)
            : OverlappingPair(
                  pairID, colliderOneBroadPhaseID, colliderTwoBroadPhaseID, colliderOneEntity, colliderTwoEntity, narrowPhaseAlgorithmToUse, isEnabled)
            , IsFirstShapeConvex(isFirstShapeConvex) {
        }

        /** @brief Frees all LastFrameCollisionData entries owned by this pair and clears the map. */
        void DestroyLastFrameCollisionData() {
            for (auto &entry : LastFrameCollisionDataMap) {
                delete entry.second;
            }

            LastFrameCollisionDataMap.clear();
        }

        /**
         * @brief Retrieves or creates the LastFrameCollisionData for a sub-shape pair.
         *
         * The lookup key is canonicalized as PairNumbers(max(shapeOneID, shapeTwoID), min(shapeOneID, shapeTwoID))
         * so the same entry is found regardless of argument order. If an existing entry is found
         * it is marked non-obsolete; otherwise a new entry is heap-allocated and inserted.
         *
         * @param shapeOneID Sub-shape ID of the first shape (e.g., triangle index).
         * @param shapeTwoID Sub-shape ID of the second shape.
         * @returns Pointer to the existing or newly created LastFrameCollisionData.
         */
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
                auto *newData = new LastFrameCollisionData();
                LastFrameCollisionDataMap[shapeIDPair] = newData;
                return newData;
            }
        }

        /**
         * @brief Removes stale sub-shape collision cache entries and marks surviving entries obsolete.
         *
         * Any entry with IsObsolete == true is deleted and erased from the map. All remaining
         * entries are then marked obsolete so they will be pruned if not refreshed next frame.
         */
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

    /**
     * @brief Manages all broad-phase overlapping pairs produced by the AABB tree.
     *
     * Pairs are sorted into four pools: active convex, active concave, disabled convex,
     * and disabled concave. Disabled pairs are suppressed from collision response
     * (e.g., trigger volumes, filtered body pairs) but remain tracked to allow
     * re-enabling without re-running the broad phase.
     *
     * All pools use swap-and-pop removal backed by unordered index maps for O(1)
     * lookup, insertion, and removal.
     */
    class OverlappingPairs final {
        friend class CollisionSystem;

    public:
        /**
         * @brief Constructs the OverlappingPairs manager.
         *
         * @param physicsWorld           The owning physics world; component stores are borrowed from it.
         * @param pairsThatCannotCollide Set of body-entity pairs excluded from all collision.
         * @param collisionDispatch      Dispatch table mapping shape-type pairs to narrow-phase algorithms.
         */
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
        ~OverlappingPairs();

        /**
         * @brief Moves the pair with the given ID from the disabled pool to the active pool.
         * @param pairID The unique identifier of the pair to enable.
         */
        void EnablePair(u64 pairID);

        /**
         * @brief Moves the pair with the given ID from the active pool to the disabled pool.
         * @param pairID The unique identifier of the pair to disable.
         */
        void DisablePair(u64 pairID);

        /**
         * @brief Moves the convex pair at the given index from the disabled pool to the active convex pool.
         * @param pairIndex Index into the disabled convex pairs vector.
         */
        void EnableConvexPairWithIndex(size_t pairIndex);

        /**
         * @brief Moves the convex pair at the given index from the active pool to the disabled convex pool.
         * @param pairIndex Index into the active convex pairs vector.
         */
        void DisableConvexPairWithIndex(size_t pairIndex);
        /**
         * @brief Moves the concave pair at the given index from the disabled pool to the active concave pool.
         * @param pairIndex Index into the disabled concave pairs vector.
         */
        void EnableConcavePairWithIndex(size_t pairIndex);

        /**
         * @brief Moves the concave pair at the given index from the active pool to the disabled concave pool.
         * @param pairIndex Index into the active concave pairs vector.
         */
        void DisableConcavePairWithIndex(size_t pairIndex);

        /**
         * @brief Creates a new overlapping pair from two collider component-store indices.
         *
         * Selects the appropriate narrow-phase algorithm via the collision dispatcher, inserts
         * the pair into the correct pool (convex or concave), and records the pair ID in both
         * colliders' overlapping-pairs lists.
         *
         * @param colliderOneIndex Component-store index of the first collider.
         * @param colliderTwoIndex Component-store index of the second collider.
         * @param isConvexPair     True if both shapes are convex; false if one shape is a concave mesh.
         * @returns The unique pair ID of the newly created pair.
         */
        size_t AddPair(size_t colliderOneIndex, size_t colliderTwoIndex, bool isConvexPair);

        /**
         * @brief Removes the pair with the given ID from whichever pool it resides in.
         *
         * Searches active convex → active concave → disabled convex → disabled concave in order
         * and also removes the pair ID from both colliders' overlapping-pairs lists.
         *
         * @param pairID The pair ID to remove.
         */
        void RemovePair(u64 pairID);

        /**
         * @brief Removes the active convex pair at the given pool index.
         *
         * Uses swap-and-pop to maintain a packed array. O(1) removal.
         *
         * @param pairIndex           Index into the active convex pairs vector.
         * @param removeFromColliders If true, removes the pair ID from both colliders' overlapping-pairs lists.
         */
        void RemoveConvexPairWithIndex(size_t pairIndex, bool removeFromColliders = true);

        /**
         * @brief Removes the active concave pair at the given pool index.
         *
         * Destroys the pair's LastFrameCollisionDataMap before swapping. O(1) removal.
         *
         * @param pairIndex           Index into the active concave pairs vector.
         * @param removeFromColliders If true, removes the pair ID from both colliders' overlapping-pairs lists.
         */
        void RemoveConcavePairWithIndex(size_t pairIndex, bool removeFromColliders = true);

        /** @brief Prunes stale per-sub-shape collision cache entries across all active concave pairs. */
        void ClearObsoleteLastFrameCollisionData();

        /** @brief Rolls AreCollidingThisFrame into WereCollidingLastFrame for all active pairs. */
        void UpdateCollidingInLastFrame();

        /**
         * @brief Returns true if the pair with the given ID exists in either disabled pool.
         * @param pairID The unique identifier of the pair to query.
         * @returns True if the pair is in the disabled convex or disabled concave pool.
         */
        [[nodiscard]] VE_INLINE bool IsPairDisabled(u64 pairID) const {
            return _disabledConvexPairIDToPairIndexMap.contains(pairID) || _disabledConcavePairIDToPairIndexMap.contains(pairID);
        }

        /**
         * @brief Sets whether the active pair with the given ID needs a narrow-phase test this frame.
         * @param pairID                 The pair ID to update.
         * @param requiresCollisionCheck True to schedule a narrow-phase check; false to skip it.
         */
        VE_INLINE void SetRequiresCollisionCheck(u64 pairID, bool requiresCollisionCheck) {
            VASSERT(_convexPairIDToPairIndexMap.contains(pairID) || _concavePairIDToPairIndexMap.contains(pairID),
                    "Trying to set requires collision check for a pair ID that does not exist.");

            auto it = _convexPairIDToPairIndexMap.find(pairID);

            if (it != _convexPairIDToPairIndexMap.end()) {
                _convexPairs[it->second].RequiresCollisionCheck = requiresCollisionCheck;
            } else {
                _concavePairs[_concavePairIDToPairIndexMap[pairID]].RequiresCollisionCheck = requiresCollisionCheck;
            }
        }

        /**
         * @brief Returns a pointer to the OverlappingPair with the given ID, or nullptr if not found.
         *
         * Searches all four pools: active convex, active concave, disabled convex, disabled concave.
         *
         * @param pairID The unique identifier of the pair to look up.
         * @returns Pointer to the matching OverlappingPair, or nullptr if the ID is not found in any pool.
         */
        [[nodiscard]] VE_INLINE OverlappingPair *GetOverlappingPair(u64 pairID) {
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

        /**
         * @brief Returns a canonically ordered pair of body entities.
         *
         * The entity with the lower ID is always placed first, ensuring a consistent
         * pair key regardless of the order in which the two bodies are passed.
         *
         * @param bodyOneEntity First body entity.
         * @param bodyTwoEntity Second body entity.
         * @returns Ordered pair with the lower-ID entity first.
         */
        VE_INLINE static Pair<Entity, Entity> ComputeBodiesIndexPair(Entity bodyOneEntity, Entity bodyTwoEntity) {
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
        std::unordered_map<u64, size_t> _convexPairIDToPairIndexMap;
        std::unordered_map<u64, size_t> _disabledConvexPairIDToPairIndexMap;
        std::unordered_map<u64, size_t> _concavePairIDToPairIndexMap;
        std::unordered_map<u64, size_t> _disabledConcavePairIDToPairIndexMap;
        BodyComponentStore &_bodyComponentStore;
        ColliderComponentStore &_colliderComponentStore;
        RigidBodyComponentStore &_rigidBodyComponentStore;
        CollisionDispatch &_collisionDispatch;
        [[maybe_unused]] std::unordered_set<Pair<Entity, Entity>> &_pairsThatCannotCollide;

        void removeDisabledConvexPairWithIndex(size_t pairIndex, bool removeFromColliders);
        void removeDisabledConcavePairWithIndex(size_t pairIndex, bool removeFromColliders);
    };

} // namespace Vulkyrie
