#pragma once

#include "vlkypch.h"
#include "physics/components/ball_and_socket_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    /** @brief Sequential-impulse solver for all active ball-and-socket joints.
     *
     * A ball-and-socket joint constrains the anchor points of two bodies to coincide, removing the three
     * translational degrees of freedom while leaving rotation free, with an optional cone limit that caps
     * the relative swing angle between the bodies. This system implements the standard solver pipeline:
     * InitializeBeforeSolving() precomputes the per-joint constraint data (lever arms, inertia tensors,
     * mass matrices, bias terms) once per step, WarmStart() re-applies the previous step's accumulated
     * impulse, SolveVelocityConstraint() is iterated to enforce the velocity-level constraint, and
     * SolvePositionConstraint() is iterated to correct residual position drift (Non-Linear Gauss-Seidel).
     *
     * The system holds references to the component stores owned by the PhysicsWorld and operates on the
     * active components of the BallAndSocketJointComponentStore. It owns no state of its own; all per-joint
     * solver data lives in the store. It is non-copyable and non-movable. */
    class BallAndSocketJointSolverSystem final {
    public:
        /** @brief Constructs the solver, binding it to the stores it operates on.
         * @param world The physics world whose component stores supply the bodies and joints to solve.
         * @param enableWarmStartup Reference to the world's warm-start toggle; when false, accumulated
         *        impulses are reset each step instead of being carried over by WarmStart(). */
        explicit BallAndSocketJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJointSolverSystem);

        ~BallAndSocketJointSolverSystem() = default;

        /** @brief Computes the current cone half-angle between the two cone-limit axes in world space.
         *
         * The dot product is clamped to [-1, 1] before the acos: rounding on two unit vectors can push it
         * slightly past +/-1 (common when the axes are nearly parallel), which would otherwise yield NaN.
         * @param coneLimitWorldAxisBodyOne The world-space cone axis of body one (typically the unit r1 vector).
         * @param coneLimitWorldAxisBodyTwo The world-space cone axis of body two (typically the unit -r2 vector).
         * @returns The angle (in radians) between the two axes, i.e. the current half-angle of the swing cone. */
        [[nodiscard]] VE_INLINE static f32 ComputeCurrentConeHalfAngle(glm::vec3 coneLimitWorldAxisBodyOne, glm::vec3 coneLimitWorldAxisBodyTwo) {
            return std::acos(std::clamp(glm::dot(coneLimitWorldAxisBodyOne, coneLimitWorldAxisBodyTwo), f32(-1.0), f32(1.0)));
        }

        /** @brief Precomputes per-joint solver data for every active joint at the start of a simulation step.
         *
         * Computes the world-space lever arms (r1, r2), caches the world-space inverse inertia tensors,
         * builds and inverts the 3x3 translational mass matrix K=JM^-1J^t, evaluates the Baumgarte bias
         * vector, and sets up the cone-limit state (rotation axis, mass matrix, bias, and violation flag).
         * Also resets the accumulated impulse when warm-starting is disabled. Must be called once per step
         * before WarmStart() and SolveVelocityConstraint().
         * @param biasFactor The Baumgarte stabilization factor (BETA / timestep) scaling position error into
         *        the velocity-constraint bias. */
        void InitializeBeforeSolving(f32 biasFactor);

        /** @brief Re-applies the previous step's accumulated translational and cone-limit impulses.
         *
         * Seeds the solver with the impulse retained from the prior step so the iterative solve converges
         * faster. Has an effect only when warm-starting is enabled; otherwise the impulse was reset by
         * InitializeBeforeSolving(). Must be called after InitializeBeforeSolving() and before
         * SolveVelocityConstraint(). */
        void WarmStart();

        /** @brief Performs one velocity-constraint solver iteration over all active joints.
         *
         * Computes and applies the corrective impulses that drive the relative velocity at the anchor point
         * to satisfy the translational constraint (plus the unilateral cone-limit constraint when enabled and
         * violated), accumulating the result into the per-joint impulse. Typically called multiple times per
         * step by the constraint solver. */
        void SolveVelocityConstraint();

        /** @brief Performs one position-constraint solver iteration over all active joints.
         *
         * Corrects residual position and orientation drift using the Non-Linear Gauss-Seidel technique:
         * recomputes the lever arms and inertia tensors from the current constrained orientations, then applies
         * pseudo-velocities to nudge the bodies' positions/orientations toward satisfying the translational and
         * cone-limit constraints. Skips joints not configured for Non-Linear Gauss-Seidel correction. */
        void SolvePositionConstraint();

    private:
        /** @brief A reference to RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief A reference to TransformComponentStore. */
        TransformComponentStore &_transformStore;

        /** @brief A reference to JointComponentStore. */
        JointComponentStore &_jointStore;

        /** @brief A reference to BallAndSocketJointComponentStore. */
        BallAndSocketJointComponentStore &_basStore;

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
