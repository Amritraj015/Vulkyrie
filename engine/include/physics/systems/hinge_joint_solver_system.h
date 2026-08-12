#pragma once

#include "core/ecs/entity.h"
#include "physics/components/hinge_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    /** @brief Sequential-impulse solver for all active hinge joints.
     *
     * A hinge (revolute) joint lets two bodies rotate relative to each other around a single axis, like a door
     * hinge or a wheel axle. It removes five of the six relative degrees of freedom via two coupled constraint
     * groups: three translation constraints (the anchor points must coincide) and two rotation constraints (the
     * body-one hinge axis a1 must stay aligned with the body-two hinge axis a2). The remaining rotational degree
     * of freedom can optionally be bounded by angle limits (one-sided constraints keeping the hinge angle within
     * [lowerLimit, upperLimit]) and/or driven by a motor (a torque-capped constraint pushing the relative angular
     * speed around the axis toward a target). This system implements the standard solver pipeline:
     * InitializeBeforeSolving() precomputes the per-joint constraint data once per step, WarmStart() re-applies
     * the previous step's accumulated impulses, SolveVelocityConstraint() is iterated to enforce the
     * velocity-level constraints, and SolvePositionConstraint() is iterated to correct residual position and
     * orientation drift (Non-Linear Gauss-Seidel).
     *
     * The system holds references to the component stores owned by the PhysicsWorld and operates on the active
     * components of the HingeJointComponentStore. It owns no state of its own; all per-joint solver data lives
     * in the store. It is non-copyable and non-movable. */
    class HingeJointSolverSystem final {
    public:
        /** @brief Constructs the solver, binding it to the stores it operates on.
         * @param world The physics world whose component stores supply the bodies and joints to solve.
         * @param enableWarmStartup Reference to the world's warm-start toggle; when false, accumulated
         *        impulses are reset each step instead of being carried over by WarmStart(). */
        explicit HingeJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(HingeJointSolverSystem);

        ~HingeJointSolverSystem() = default;

        /** @brief Precomputes per-joint solver data for every active joint at the start of a simulation step.
         *
         * Computes the world-space lever arms (r1, r2), caches the world-space inverse inertia tensors, derives
         * the world-space hinge axis a1 and the b2/c2 basis of the two axis-alignment constraints, and builds
         * and inverts the constraint mass matrices (K=JM^-1J^t): the 3x3 translation matrix, the 2x2 rotation
         * matrix and, when the motor or a violated limit needs it, the shared 1x1 limit/motor matrix. Also
         * evaluates the Baumgarte bias terms, re-evaluates the current hinge angle to update the limit-violation
         * states (resetting a limit's accumulated impulse when its violation state changes), and resets all
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
         * non-negative (one-sided constraints), the motor impulse is clamped to the per-step torque budget
         * maxMotorTorque * timestep, and the rotation/translation impulses are unclamped equality constraints.
         * Typically called multiple times per step by the constraint solver.
         * @param timestep The duration of the current simulation step, used to convert the maximum motor
         *        torque into a per-step impulse budget. */
        void SolveVelocityConstraint(Timestep timestep);

        /** @brief Performs one position-constraint solver iteration over all active joints.
         *
         * Corrects residual drift using the Non-Linear Gauss-Seidel technique: recomputes the lever arms,
         * inertia tensors, hinge-axis basis and mass matrices from the current constrained state, then applies
         * pseudo-velocities to nudge the bodies' positions/orientations toward satisfying the violated angle
         * limits, the hinge-axis alignment and the anchor-point coincidence. Skips joints not configured for
         * Non-Linear Gauss-Seidel correction. */
        void SolvePositionConstraint();

        /** @brief Computes the joint's current rotation angle around the hinge axis.
         *
         * Extracts the angle of the bodies' relative rotation (measured from the rest orientation captured at
         * joint creation) around the world-space hinge axis, normalized into [-pi; pi] and then remapped near
         * the configured limits so limit errors are measured against the closest equivalent angle (modulo
         * 2*pi). Reads the joint's cached world-space hinge axis, so the axis must be up to date;
         * InitializeBeforeSolving() and SolvePositionConstraint() refresh it before calling.
         * @param jointEntity The entity of the joint whose angle to compute.
         * @param bodyOneOrientation The current orientation of body 1.
         * @param bodyTwoOrientation The current orientation of body 2.
         * @returns The current hinge angle in radians. */
        [[nodiscard]] VE_INLINE f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
            return ComputeCurrentHingeAngleAtIndex(_hingeJointStore.GetEntityIndex(jointEntity), bodyOneOrientation, bodyTwoOrientation);
        }

        /** @brief Computes the joint's current rotation angle around the hinge axis (see ComputeCurrentHingeAngle()).
         *
         * Index-based variant used by the solver loops, which already hold the component index and so avoid
         * the entity-to-index hash lookup of the entity-based overload.
         * @param jointComponentIndex The component index of the joint in the hinge joint store.
         * @param bodyOneOrientation The current orientation of body 1.
         * @param bodyTwoOrientation The current orientation of body 2.
         * @returns The current hinge angle in radians. */
        f32 ComputeCurrentHingeAngleAtIndex(size_t jointComponentIndex, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation);

    private:
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

        /** @brief A reference to RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief A reference to TransformComponentStore. */
        TransformComponentStore &_transformStore;

        /** @brief A reference to JointComponentStore. */
        JointComponentStore &_jointStore;

        /** @brief A reference to HingeJointComponentStore. */
        HingeJointComponentStore &_hingeJointStore;

        /** @brief Reference to the world's warm-start toggle; when false, accumulated impulses are reset each step. */
        bool &_enableWarmStartup;

        /** @brief A full turn (2*pi), used when normalizing hinge angles. */
        constexpr static f32 TWICE_PI = 2 * std::numbers::pi_v<f32>;

        /** @brief Maps an angle to the equivalent angle in the range [-pi; pi].
         * @param angle The angle to normalize, in radians.
         * @returns The equivalent angle in [-pi; pi]. */
        f32 computeNormalizedAngle(f32 angle) const;

        /** @brief Remaps an angle to the equivalent angle (modulo 2*pi) closest to the joint's limit range.
         *
         * Given an input angle in [-pi; pi] that falls outside [lowerLimitAngle, upperLimitAngle], returns the
         * representation of the same physical angle - shifted by a full turn when appropriate, so the result
         * lies in [-2*pi; 2*pi] - that is nearest to one of the two limits, so the limit constraints correct
         * toward the closer limit. Angles already inside the limits, or any angle when the limit range is
         * degenerate (upper <= lower), are returned unchanged.
         * @param inputAngle The angle to remap, in the range [-pi; pi].
         * @param lowerLimitAngle The joint's lower angle limit in radians.
         * @param upperLimitAngle The joint's upper angle limit in radians.
         * @returns The equivalent angle closest to the limit range. */
        f32 computeCorrespondingAngleNearLimits(f32 inputAngle, f32 lowerLimitAngle, f32 upperLimitAngle) const;
    };

} // namespace Vulkyrie
