#include "physics/types/narrow_phase_input.h"

namespace Vulkyrie {

    NarrowPhaseInput::NarrowPhaseInput(OverlappingPairs &overlappingPairs)
        : _sphereVsSphereBatch(overlappingPairs)
        , _sphereVsCapsuleBatch(overlappingPairs)
        , _capsuleVsCapsuleBatch(overlappingPairs)
        , _sphereVsConvexPolyhedronBatch(overlappingPairs)
        , _capsuleVsConvexPolyhedronBatch(overlappingPairs)
        , _convexPolyhedronVsConvexPolyhedronBatch(overlappingPairs) {
    }

} // namespace Vulkyrie
