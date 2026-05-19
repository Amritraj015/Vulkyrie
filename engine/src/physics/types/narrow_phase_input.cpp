#include "physics/types/narrow_phase_input.h"

namespace Vulkyrie {

    NarrowPhaseInput::NarrowPhaseInput()
        : _sphereVsSphereBatch()
        , _sphereVsCapsuleBatch()
        , _capsuleVsCapsuleBatch()
        , _sphereVsConvexPolyhedronBatch()
        , _capsuleVsConvexPolyhedronBatch()
        , _convexPolyhedronVsConvexPolyhedronBatch() {
    }

} // namespace Vulkyrie
