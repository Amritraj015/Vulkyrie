#pragma once

#include "core/entity.h"

namespace Vulkyrie {

    /**
     * @brief Holds all simulation islands produced during a single physics frame.
     *
     * An island is an isolated group of awake bodies connected to one another by contacts
     * or joints. Bodies in separate islands cannot influence each other within a frame, so
     * each island can be solved independently. The data is stored in parallel arrays indexed
     * by island index, with body entities stored sequentially and accessed via
     * `StartingBodyIndexForIsland` + `TotalBodiesInIsland`.
     *
     * Typical per-frame usage:
     *   1. Call `ReserveMemory()` once to pre-allocate based on last-frame counts.
     *   2. Call `AddIsland()` to open a new island, then `AddBodyToIsland()` for each member body.
     *   3. Process each island for constraint solving.
     *   4. Call `Clear()` at the end of the frame to reset state while preserving capacity hints
     *      for the next frame.
     */
    struct Islands final {
    public:
        /// For each island: index of the island's first contact manifold in the global manifold array.
        std::vector<size_t> ContactManifoldIndices;

        /// For each island: total number of contact manifolds in the island.
        std::vector<size_t> TotalContactManifolds;

        /// For each island: starting index into `BodyEntities` for this island's bodies.
        std::vector<size_t> StartingBodyIndexForIsland;

        /// For each island: total number of bodies belonging to the island.
        std::vector<size_t> TotalBodiesInIsland;

        /// Flat array of all body entities across all islands, stored sequentially per island.
        std::vector<Entity> BodyEntities;

        /** @brief Constructs an `Islands` instance with sensible default capacity hints.
         *
         * Initial values of 16 islands and 32 body entities match the ReactPhysics3D defaults
         * and provide a reasonable starting reservation before the first frame's data is available.
         */
        Islands()
            : _totalIslandsInLastFrame(16)
            , _totalBodyEntitiesInLastFrame(32)
            , _maxBodiesInIslandInLastFrame(0)
            , _maxBodiesInIslandInCurrentFrame(0) {
        }

        // Delete the copy constructor and the copy assignment operator.
        Islands(const Islands &) = delete;
        Islands &operator=(const Islands &) = delete;

        // Delete the move constructor and the move assignment operator.
        Islands(Islands &&) = delete;
        Islands &operator=(Islands &&) = delete;

        /** @brief Default destructor. */
        ~Islands() = default;

        /** @returns The number of islands created in the current frame. */
        [[nodiscard]] VE_INLINE size_t GetTotalIslands() const {
            return ContactManifoldIndices.size();
        }

        /** @returns The total number of body entities that were registered across all islands
         *           during the previous frame. Used to size `BodyEntities` reservations. */
        [[nodiscard]] VE_INLINE size_t GetTotalBodiesInIslandInLastFrame() const {
            return _totalBodyEntitiesInLastFrame;
        }

        /** @returns The maximum number of bodies that any single island contained in the
         *           previous frame. Useful for sizing per-island scratch buffers in solvers. */
        [[nodiscard]] VE_INLINE size_t GetMaxBodiesInIslandInLastFrame() const {
            return _maxBodiesInIslandInLastFrame;
        }

        /** @brief Adds a body entity to the most-recently created island.
         *
         * Must only be called after at least one island has been opened via `AddIsland()`.
         * @param bodyEntity The entity handle of the body to add.
         */
        VE_INLINE void AddBodyToIsland(Entity bodyEntity) {
            const size_t islandIndex = ContactManifoldIndices.size();

            BodyEntities.push_back(bodyEntity);
            TotalBodiesInIsland[islandIndex - 1]++;
        }

        /** @brief Opens a new island and returns its index.
         *
         * Records the starting contact-manifold index for the new island, then updates the
         * running maximum-bodies-per-island counter for the island that was just closed.
         *
         * @param contactManifoldStartIndex Index of the first contact manifold that belongs
         *                                  to this island in the global manifold array.
         * @returns The zero-based index of the newly created island.
         */
        [[nodiscard]] VE_INLINE size_t AddIsland(size_t contactManifoldStartIndex) {
            const size_t islandIndex = ContactManifoldIndices.size();
            ContactManifoldIndices.push_back(contactManifoldStartIndex);
            TotalContactManifolds.push_back(0);
            StartingBodyIndexForIsland.push_back(BodyEntities.size());
            TotalBodiesInIsland.push_back(0);

            if (islandIndex > 0 && TotalBodiesInIsland[islandIndex - 1] > _maxBodiesInIslandInCurrentFrame) {
                _maxBodiesInIslandInCurrentFrame = TotalBodiesInIsland[islandIndex - 1];
            }

            return islandIndex;
        }

        /** @brief Pre-allocates internal arrays using counts from the previous frame.
         *
         * Call once at the start of each frame before any `AddIsland()` / `AddBodyToIsland()`
         * calls to avoid repeated reallocations during island construction.
         */
        VE_INLINE void ReserveMemory() {
            ContactManifoldIndices.reserve(_totalIslandsInLastFrame);
            TotalContactManifolds.reserve(_totalIslandsInLastFrame);
            StartingBodyIndexForIsland.reserve(_totalIslandsInLastFrame);
            TotalBodiesInIsland.reserve(_totalIslandsInLastFrame);
            BodyEntities.reserve(_totalBodyEntitiesInLastFrame);
        }

        /** @brief Resets all island data at the end of a frame while retaining capacity hints.
         *
         * Finalizes the per-island maximum-bodies tracking for the last island (whose count
         * was not yet considered by `AddIsland()`), snapshots the current-frame statistics
         * into last-frame fields for use by the next frame's `ReserveMemory()`, then clears
         * all arrays.
         */
        VE_INLINE void Clear() {
            const size_t totalIslands = TotalContactManifolds.size();

            if (totalIslands > 0 && TotalBodiesInIsland[totalIslands - 1] > _maxBodiesInIslandInCurrentFrame) {
                _maxBodiesInIslandInCurrentFrame = TotalBodiesInIsland[totalIslands - 1];
            }

            _maxBodiesInIslandInLastFrame = _maxBodiesInIslandInCurrentFrame;
            _totalIslandsInLastFrame = totalIslands;

            _maxBodiesInIslandInCurrentFrame = 0;
            _totalBodyEntitiesInLastFrame = BodyEntities.size();

            ContactManifoldIndices.clear();
            TotalContactManifolds.clear();
            StartingBodyIndexForIsland.clear();
            TotalBodiesInIsland.clear();
            BodyEntities.clear();
        }

    private:
        /// Number of islands created in the previous frame; used to reserve `ContactManifoldIndices` et al.
        size_t _totalIslandsInLastFrame;

        /// Total body entities registered in the previous frame; used to reserve `BodyEntities`.
        size_t _totalBodyEntitiesInLastFrame;

        /// Largest body count seen in any single island during the previous frame.
        size_t _maxBodiesInIslandInLastFrame;

        /// Running maximum body count for any single island in the current frame.
        size_t _maxBodiesInIslandInCurrentFrame;
    };

} // namespace Vulkyrie
