#pragma once

#include "core/entity.h"

namespace Vulkyrie {

    class HingeJointSolverSystem {
    public:
        f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation);

    private:
    };

} // namespace Vulkyrie
