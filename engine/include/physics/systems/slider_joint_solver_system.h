#pragma once

#include "vlkypch.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/slider_joint_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    /** @brief Sequential-impulse solver for all active slider (prismatic) joints.
     *
     * A slider joint constrains two bodies to translate relative to each other along a single fixed axis, like
     * a piston or a drawer slide, while otherwise moving as one rigid body. It removes five of the six relative
     * degrees of freedom via two coupled constraint groups: two translation constraints (the anchor points must
     * stay aligned along the slider axis, spanned by the n1/n2 basis orthogonal to it) and three rotation
     * constraints (the relative orientation must stay at the initial difference q0). The remaining
     * translational degree of freedom along the slider axis can optionally be bounded by limits (one-sided
     * constraints keeping the anchor separation within [lowerLimit, upperLimit]) and/or driven by a motor (a
     * force-capped constraint pushing the relative linear speed along the axis toward a target). This system
     * implements the standard solver pipeline: InitializeBeforeSolving() precomputes the per-joint constraint
     * data once per step, WarmStart() re-applies the previous step's accumulated impulses,
     * SolveVelocityConstraint() is iterated to enforce the velocity-level constraints, and
     * SolvePositionConstraint() is iterated to correct residual position and orientation drift (Non-Linear
     * Gauss-Seidel).
     *
     * The system holds references to the component stores owned by the PhysicsWorld and operates on the active
     * components of the SliderJointComponentStore. It owns no state of its own; all per-joint solver data lives
     * in the store. It is non-copyable and non-movable. */
    class SliderJointSolverSystem {
    public:
        /** @brief Constructs the solver, binding it to the stores it operates on.
         * @param world The physics world whose component stores supply the bodies and joints to solve.
         * @param enableWarmStartup Reference to the world's warm-start toggle; when false, accumulated
         *        impulses are reset each step instead of being carried over by WarmStart(). */
        explicit SliderJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(SliderJointSolverSystem);

        ~SliderJointSolverSystem() = default;

        /** @brief Precomputes per-joint solver data for every active joint at the start of a simulation step.
         *
         * Computes the world-space lever arms (r1, r2) and the anchor-point separation u, caches the world-space
         * inverse inertia tensors, derives the world-space slider axis and its orthogonal n1/n2 basis, and builds
         * and inverts the constraint mass matrices (K=JM^-1J^t): the 2x2 translation matrix, the 3x3 rotation
         * matrix and, when a violated limit needs it, the shared 1x1 limit matrix (the motor's 1x1 mass matrix
         * has no angular component, since sliding is a pure translation). Also evaluates the Baumgarte bias
         * terms, re-evaluates the anchor separation along the slider axis to update the limit-violation states
         * (resetting a limit's accumulated impulse when its violation state changes), and resets all
         * accumulated impulses when warm-starting is disabled. Must be called once per step before WarmStart()
         * and SolveVelocityConstraint().
         * @param biasFactor The Baumgarte stabilization factor (BETA / timestep) scaling position error into
         *        the velocity-constraint bias. */
        void InitializeBeforeSolving(f32 biasFactor);

        /** @brief Re-applies the previous step's accumulated impulses.
         *
         * Seeds the solver with the translation, rotation, limit and motor impulses retained from the prior
         * step so the iterative solve converges faster. Has an effect only when warm-starting is enabled;
         * otherwise the impulses were reset by InitializeBeforeSolving(). Must be called after
         * InitializeBeforeSolving() and before SolveVelocityConstraint(). */
        void WarmStart();

        /** @brief Performs one velocity-constraint solver iteration over all active joints.
         *
         * Solves the constraints in sequence - limits, motor, rotation, translation - with each block reusing
         * the velocities updated by the previous one (Gauss-Seidel). The limit impulses are clamped to stay
         * non-negative (one-sided constraints), the motor impulse is clamped to the per-step force budget
         * maxMotorForce * timestep, and the rotation/translation impulses are unclamped equality constraints.
         * Typically called multiple times per step by the constraint solver.
         * @param timestep The duration of the current simulation step, used to convert the maximum motor
         *        force into a per-step impulse budget. */
        void SolveVelocityConstraint(Timestep timestep);

        /** @brief Performs one position-constraint solver iteration over all active joints.
         *
         * Corrects residual drift using the Non-Linear Gauss-Seidel technique: recomputes the lever arms,
         * inertia tensors, slider-axis basis and mass matrices from the current constrained state, then applies
         * pseudo-velocities to nudge the bodies' positions/orientations toward satisfying the violated
         * translation limits, the anchor-point alignment along the slider axis and the locked relative
         * orientation. Skips joints not configured for Non-Linear Gauss-Seidel correction. */
        void SolvePositionConstraint();

    private:
        /** @brief A reference to RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief A reference to TransformComponentStore. */
        TransformComponentStore &_transformStore;

        /** @brief A reference to JointComponentStore. */
        JointComponentStore &_jointStore;

        /** @brief A reference to SliderJointComponentStore. */
        SliderJointComponentStore &_sliderJointStore;

        /** @brief Reference to the world's warm-start toggle; when false, accumulated impulses are reset each step. */
        bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
