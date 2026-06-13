#pragma once

namespace Vulkyrie {

    class BallAndSocketJointSolverSystem {
    public:
        [[nodiscard]] VE_INLINE static f32 ComputeCurrentConeHalfAngle([[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyOne,
                                                                       [[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyTwo) {
            return f32(0.0);
        }
    };

} // namespace Vulkyrie
