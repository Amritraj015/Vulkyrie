#pragma once

#include "vlkypch.h"
#include "physics/components/rigid_body_component_store.h"
#include "core/time_step.h"

namespace Vulkyrie {

    struct ConstraintSolverData {
        Vulkyrie::Timestep Timestep;
        bool EnableWarmStartup;
        RigidBodyComponentStore &RigidBodyStore;
        JointComponentStore &JointStore;
    };

    class ConstraintSolverSystem final {
    public:
        ~ConstraintSolverSystem() = default;

        void SolvePositionConstraints();
    };

} // namespace Vulkyrie
