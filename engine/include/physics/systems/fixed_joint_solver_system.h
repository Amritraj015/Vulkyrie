#pragma once

#include "vlkypch.h"
#include "physics/components/fixed_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    /** @brief Sequential-impulse solver for all active fixed joints.
     *
     * A fixed joint rigidly welds two bodies together, holding their relative position and orientation at the
     * rest pose captured when the joint was created. It removes all six relative degrees of freedom via two
     * coupled constraint groups: three translation constraints (the anchor points must coincide) and three
     * rotation constraints (the relative orientation must stay at the initial difference q0). This system
     * implements the standard solver pipeline: InitializeBeforeSolving() precomputes the per-joint constraint
     * data (lever arms, inertia tensors, the translation and rotation mass matrices, bias terms) once per step,
     * WarmStart() re-applies the previous step's accumulated impulses, SolveVelocityConstraint() is iterated to
     * enforce the velocity-level constraints, and SolvePositionConstraint() is iterated to correct residual
     * position and orientation drift (Non-Linear Gauss-Seidel).
     *
     * The system holds references to the component stores owned by the PhysicsWorld and operates on the active
     * components of the FixedJointComponentStore. It owns no state of its own; all per-joint solver data lives
     * in the store. It is non-copyable and non-movable. */
    class FixedJointSolverSystem {
    public:
        /** @brief Constructs the solver, binding it to the stores it operates on.
         * @param world The physics world whose component stores supply the bodies and joints to solve.
         * @param enableWarmStartup Reference to the world's warm-start toggle; when false, accumulated
         *        impulses are reset each step instead of being carried over by WarmStart(). */
        explicit FixedJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(FixedJointSolverSystem);

        ~FixedJointSolverSystem() = default;

        /** @brief Precomputes per-joint solver data for every active joint at the start of a simulation step.
         *
         * Computes the world-space lever arms (r1, r2), caches the world-space inverse inertia tensors, builds
         * and inverts both the 3x3 translation mass matrix and the 3x3 rotation mass matrix (K=JM^-1J^t), and
         * evaluates the Baumgarte bias vectors for the translation (anchor-point separation) and rotation
         * (orientation error) constraints. Also resets the accumulated impulses when warm-starting is disabled.
         * Must be called once per step before WarmStart() and SolveVelocityConstraint().
         * @param biasFactor The Baumgarte stabilization factor (BETA / timestep) scaling position error into
         *        the velocity-constraint bias. */
        void InitializeBeforeSolving(f32 biasFactor);

        /** @brief Re-applies the previous step's accumulated translation and rotation impulses.
         *
         * Seeds the solver with the impulses retained from the prior step so the iterative solve converges
         * faster. Has an effect only when warm-starting is enabled; otherwise the impulses were reset by
         * InitializeBeforeSolving(). Must be called after InitializeBeforeSolving() and before
         * SolveVelocityConstraint(). */
        void WarmStart();

        /** @brief Performs one velocity-constraint solver iteration over all active joints.
         *
         * Computes and applies the corrective impulses that drive the relative velocity at the anchor point to
         * zero (the three translation constraints) and the relative angular velocity to zero (the three rotation
         * constraints), accumulating the result into the per-joint impulses. The translation constraints are
         * solved first and the rotation constraints reuse the updated velocities (Gauss-Seidel). Typically called
         * multiple times per step by the constraint solver. */
        void SolveVelocityConstraint();

        /** @brief Performs one position-constraint solver iteration over all active joints.
         *
         * Corrects residual position and orientation drift using the Non-Linear Gauss-Seidel technique:
         * recomputes the lever arms and inertia tensors from the current constrained orientations, then applies
         * pseudo-velocities to nudge the bodies' positions/orientations toward satisfying the translation and
         * rotation constraints. Skips joints not configured for Non-Linear Gauss-Seidel correction. */
        void SolvePositionConstraint();

    private:
        /** @brief A reference to RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief A reference to TransformComponentStore. */
        TransformComponentStore &_transformStore;

        /** @brief A reference to JointComponentStore. */
        JointComponentStore &_jointStore;

        /** @brief A reference to FixedJointComponentStore. */
        FixedJointComponentStore &_fixedJointStore;

        /** @brief Reference to the world's warm-start toggle; when false, accumulated impulses are reset each step. */
        bool &_enableWarmStartup;

        /** @brief Per-joint component indices resolved by InitializeBeforeSolving(). */
        struct JointIndices {
            size_t JointIndex;
            size_t BodyOneIndex;
            size_t BodyTwoIndex;
        };

        /** @brief Per-joint component indices, parallel to the joint store's active components. Resolved once
         * per step by InitializeBeforeSolving() and reused by the other phases, so the per-iteration solver
         * loops avoid repeating the entity-to-index hash lookups. Only valid for the current step. */
        std::vector<JointIndices> _jointIndices;
    };

} // namespace Vulkyrie
