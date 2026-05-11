#include "physics/collision/narrowphase/sphere_vs_sphere_algorithm.h"

namespace Vulkyrie {

    bool SphereVsSphereAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
        }

        return collisionDetected;
    }

} // namespace Vulkyrie
