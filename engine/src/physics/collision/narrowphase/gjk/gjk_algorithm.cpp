#include "physics/collision/narrowphase/gjk/gjk_algorithm.h"

namespace Vulkyrie {

    bool GJKAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseInfoBatch,
                                             size_t batchStartIndex,
                                             size_t batchNbItems,
                                             std::vector<GJKResult> &gjkResults) {
        (void)narrowPhaseInfoBatch;
        (void)batchStartIndex;
        (void)batchNbItems;
        (void)gjkResults;
        // glm::vec3 supportPointOnShapeOne;
        // glm::vec3 supportPointOnShapeTwo;
        // glm::vec3 supportPointOnMinkowskiDifference;
        // glm::vec3 closestPointOnShapeOne;
        // glm::vec3 closestPointOnShapeTwo;
        // f32 previousDistanceSquared = std::numeric_limits<f32>::max();
        bool collisionDetected = false;

        return collisionDetected;
    }

} // namespace Vulkyrie
