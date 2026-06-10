#pragma once

namespace Vulkyrie {

    class ConstraintSolverSystem final {
    public:
        ~ConstraintSolverSystem() = default;

        void SolvePositionConstraints();
    };

} // namespace Vulkyrie
