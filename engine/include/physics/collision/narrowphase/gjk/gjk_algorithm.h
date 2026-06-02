#pragma once

#include "vlkypch.h"
#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    /**
     * @brief GJK (Gilbert-Johnson-Keerthi) Algorithm for narrow-phase collision detection.
     * 
     * This class implements the GJK algorithm, which is used to determine if two convex shapes
     * intersect and compute contact information. The algorithm works by constructing a simplex
     * in Minkowski difference space (A - B) and iteratively refining it to find the closest
     * point to the origin.
     * 
     * ## Algorithm Overview:
     * 1. Start with an initial search direction
     * 2. Compute support points on both shapes in that direction
     * 3. Add the Minkowski difference support point to the simplex
     * 4. Find the point on the simplex closest to the origin
     * 5. Repeat with the new search direction until convergence
     * 
     * ## Hybrid Technique:
     * - Runs GJK on shapes WITHOUT margins first
     * - If shapes are separated: returns SEPARATED
     * - If shapes overlap only in margins: computes shallow contact (COLLIDE_IN_MARGIN)
     * - If shapes deeply interpenetrate: returns INTERPENETRATE (requires EPA/SAT)
     * 
     * ## Optimizations:
     * - Frame coherence: Caches separating axis from previous frame
     * - Early termination: Multiple convergence checks to avoid unnecessary iterations
     * - Batch processing: Processes multiple collision pairs efficiently
     * 
     * @note This implementation is based on ReactPhysics3D's GJK implementation
     * @see https://en.wikipedia.org/wiki/Gilbert%E2%80%93Johnson%E2%80%93Keerthi_distance_algorithm
     */
    class GJKAlgorithm final {
    public:
        /**
         * @brief Result enumeration for GJK collision detection.
         */
        enum class GJKResult : i32 {
            Separated,       ///< Shapes are separated (no collision, even with margins)
            CollideInMargin, ///< Shapes overlap only in margins (shallow penetration, contact generated)
            Interpenetrate   ///< Shapes deeply penetrate (requires EPA or SAT for contact resolution)
        };

        /** @brief Default constructor. */
        GJKAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        GJKAlgorithm(const GJKAlgorithm &) = delete;
        GJKAlgorithm &operator=(const GJKAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        GJKAlgorithm(GJKAlgorithm &&) = delete;
        GJKAlgorithm &operator=(GJKAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~GJKAlgorithm() = default;

        /**
         * @brief Performs narrow-phase collision detection on a batch of shape pairs using GJK.
         * 
         * This method processes multiple collision pairs using the GJK (Gilbert-Johnson-Keerthi)
         * algorithm to determine if they are separated, touching in their margins, or deeply
         * interpenetrating.
         * 
         * ## Hybrid Technique Implementation:
         * - First, GJK runs on original shapes WITHOUT margins
         * - If shapes are separated: returns SEPARATED
         * - If shapes overlap only in margins: computes shallow penetration (COLLIDE_IN_MARGIN)
         * - If original shapes (without margin) deeply intersect: returns INTERPENETRATE
         * 
         * ## Frame Coherence Optimization:
         * The algorithm caches the separating axis from the previous frame to accelerate
         * convergence for objects in close proximity across multiple frames.
         * 
         * @param batch Batch of narrow-phase collision data containing shape pairs and transforms
         * @param batchStartIndex Starting index in the batch to begin processing
         * @param totalBatchItems Number of items to process from the batch
         * @param gjkResults Output vector that stores the GJKResult for each processed pair
         * 
         * @pre All shapes must be convex (enforced by assertion)
         * @pre Shape margins must be positive (enforced by assertion)
         * @pre gjkResults.size() must equal batchStartIndex before each result is added
         * 
         * @post gjkResults contains one result per processed collision pair
         * @post Contact points are added to the batch if ReportContacts is true and collision occurs
         * 
         * @note The algorithm operates in the local space of the first shape for numerical stability
         * @note Frame coherence is used when available to accelerate convergence
         */
        void PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t totalBatchItems, std::vector<GJKResult> &gjkResults);
    };

} // namespace Vulkyrie
