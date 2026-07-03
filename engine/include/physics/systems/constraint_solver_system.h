#pragma once

#include "vlkypch.h"
#include "core/time_step.h"
#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/systems/slider_joint_solver_system.h"

namespace Vulkyrie {

    /** @brief Top-level orchestrator for all joint constraint solving in the physics world.
     *
     * Vulkyrie supports four joint types - ball-and-socket, fixed, hinge and slider - each implemented by its
     * own sequential-impulse solver system (BallAndSocketJointSolverSystem, FixedJointSolverSystem,
     * HingeJointSolverSystem, SliderJointSolverSystem). ConstraintSolverSystem owns one instance of each and
     * fans every pipeline call out to all four, so the rest of the engine (PhysicsWorld's step loop) only has
     * to drive this single system rather than know about each joint type individually. It also owns the
     * Baumgarte stabilization constant BETA and derives the shared bias factor from it once per step, so every
     * joint type applies the same position-error correction strength.
     *
     * The solver pipeline mirrors the one each sub-system implements: Initialize() precomputes per-joint
     * constraint data and (when enabled) warm-starts every joint type for the step, SolveVelocityConstraints()
     * is iterated to enforce the velocity-level constraints, and SolvePositionConstraints() is iterated to
     * correct residual position and orientation drift (Non-Linear Gauss-Seidel). It also exposes
     * ComputeCurrentHingeAngle() as a read-only query path so joint-facing code (e.g. HingeJoint::GetAngle())
     * can ask the hinge solver for a joint's current angle without depending on HingeJointSolverSystem
     * directly. It is non-copyable and non-movable. */
    class ConstraintSolverSystem final {
    public:
        /** @brief Constructs the solver, constructing and binding all four per-joint-type sub-systems.
         * @param world The physics world whose component stores supply the bodies and joints to solve.
         * @param enableWarmStartup Reference to the world's warm-start toggle, forwarded to every sub-system;
         *        when false, each sub-system resets its accumulated impulses every step instead of carrying
         *        them over. */
        ConstraintSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(ConstraintSolverSystem);

        ~ConstraintSolverSystem() = default;

        /** @brief Computes a hinge joint's current rotation angle around its hinge axis.
         *
         * Thin forwarding wrapper around HingeJointSolverSystem::ComputeCurrentHingeAngle(), letting joint
         * objects (e.g. HingeJoint::GetAngle()) query the current angle through the world-owned constraint
         * solver instead of reaching into the hinge sub-system directly.
         * @param jointEntity The entity of the hinge joint whose angle to compute.
         * @param bodyOneOrientation The current orientation of body 1.
         * @param bodyTwoOrientation The current orientation of body 2.
         * @returns The current hinge angle in radians. */
        [[nodiscard]] VE_INLINE f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
            return _hingeJointSolverSystem.ComputeCurrentHingeAngle(jointEntity, bodyOneOrientation, bodyTwoOrientation);
        }

        /** @brief Precomputes per-joint solver data for every active joint of every type, then warm-starts.
         *
         * Derives the shared Baumgarte bias factor (BETA / timestep) and calls InitializeBeforeSolving() on
         * all four sub-systems with it, then, if warm-starting is enabled, calls WarmStart() on all four so
         * each joint's velocity solve begins from the previous step's accumulated impulses. Must be called
         * once per step before SolveVelocityConstraints().
         * @param timestep The duration of the current simulation step, used to derive the bias factor. */
        void Initialize(Timestep timestep);

        /** @brief Performs one velocity-constraint solver iteration over every active joint of every type.
         *
         * Calls SolveVelocityConstraint() on all four sub-systems in turn (ball-and-socket, fixed, hinge,
         * slider), each enforcing its own velocity-level constraints via sequential impulses. Typically
         * called multiple times per step to let the impulses converge.
         * @param timestep The duration of the current simulation step, forwarded to the hinge and slider
         *        sub-systems for their motor torque/force impulse budgets. */
        void SolveVelocityConstraints(Timestep timestep);

        /** @brief Performs one position-constraint solver iteration over every active joint of every type.
         *
         * Calls SolvePositionConstraint() on all four sub-systems in turn, each correcting its own residual
         * position and orientation drift using the Non-Linear Gauss-Seidel technique. Sub-systems skip joints
         * not configured for that correction technique. Typically called multiple times per step
         * (PhysicsWorld::_settings.PositionSolverIterations) to let the correction converge. */
        void SolvePositionConstraints();

    private:
        /** @brief The Baumgarte stabilization constant used to derive the position-error bias factor
         * (BETA / timestep) shared by every joint type's velocity constraints. */
        static constexpr f32 BETA = f32(0.2);

        /** @brief Solver for all active ball-and-socket joints. */
        BallAndSocketJointSolverSystem _ballAndSocketJointSolverSystem;

        /** @brief Solver for all active fixed joints. */
        FixedJointSolverSystem _fixedJointSolverSystem;

        /** @brief Solver for all active hinge joints. */
        HingeJointSolverSystem _hingeJointSolverSystem;

        /** @brief Solver for all active slider joints. */
        SliderJointSolverSystem _sliderJointSolverSystem;

        /** @brief Reference to the world's warm-start toggle, forwarded to every sub-system at construction. */
        bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
