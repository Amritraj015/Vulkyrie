#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/components/component_store.h"
#include "physics/constraint/ball_and_socket_joint.h"

namespace Vulkyrie {

    /** @brief Plain-data component passed to `BallAndSocketJointComponentStore::AddComponent`.
     *
     * Carries the configuration for a ball-and-socket joint's optional cone limit. All fields are
     * copied into the store's parallel arrays on insertion; callers may construct this struct on the
     * stack and pass by const reference. */
    struct BallAndSocketJointComponent final {
        /** @brief Half-angle (in radians) of the cone that constrains the relative rotation between bodies. Only used when ConeLimitEnabled is true. */
        f32 ConeLimitHalfAngle;

        /** @brief Whether the cone angular limit is active for this joint. */
        bool ConeLimitEnabled;

        /** @brief Constructs a BallAndSocketJointComponent with the specified cone limit parameters.
         * @param coneLimitEnabled Whether the cone angular limit should be active.
         * @param coneLimitHalfAngle Half-angle of the cone limit in radians. */
        BallAndSocketJointComponent(bool coneLimitEnabled, f32 coneLimitHalfAngle)
            : ConeLimitHalfAngle(coneLimitHalfAngle)
            , ConeLimitEnabled(coneLimitEnabled) {
        }
    };

    /** @brief Stores ball-and-socket joint components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that represents a ball-and-socket joint owns exactly one BallAndSocketJointComponent,
     * which binds it to a BallAndSocketJoint object and tracks all constraint solver state needed for
     * each simulation step: anchor points in local space, world-space lever arms, inertia tensors, the
     * translational impulse, bias vector, inverse mass matrix, and optional cone-limit state. The store
     * maintains the dense active-zone invariant inherited from ComponentStore: active components occupy
     * indices [0, _activeCount) and inactive components occupy [_activeCount, size). Swap operations
     * keep all parallel arrays in sync with _entities at all times, enabling efficient iteration during
     * physics updates. */
    class BallAndSocketJointComponentStore : public ComponentStore {
    public:
        /** @brief Constructs an instance of BallAndSocketJointComponentStore and reserves initial storage for all parallel arrays. */
        BallAndSocketJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJointComponentStore);

        /** @brief Destructor for BallAndSocketJointComponentStore. */
        ~BallAndSocketJointComponentStore() override = default;

        /** @brief Adds a BallAndSocketJointComponent to the specified entity. Active components are stored at the front of the
         * vector and inactive ones at the back to maintain dense packing for efficient iteration.
         * @param jointEntity The entity to which the component will be added. Must not already have a component.
         * @param component The BallAndSocketJointComponent to be added.
         * @param active Whether the joint entity is currently active. */
        void AddComponent(Entity jointEntity, const BallAndSocketJointComponent &component, bool active);

        /** @brief Retrieves the BallAndSocketJoint pointer associated with the specified entity.
         * @param jointEntity The entity whose joint is to be retrieved. Must have a component.
         * @returns Non-owning pointer to the BallAndSocketJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE BallAndSocketJoint *GetJoint(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _joints[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the BallAndSocketJoint pointer at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Non-owning pointer to the BallAndSocketJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE BallAndSocketJoint *GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            return _joints[componentIndex];
        }

        /** @brief Sets the BallAndSocketJoint pointer for the specified entity.
         * @param jointEntity The entity whose joint pointer is to be set. Must have a component.
         * @param joint Non-owning pointer to the BallAndSocketJoint to associate with the entity. */
        VE_INLINE void SetJoint(Entity jointEntity, BallAndSocketJoint *joint) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _joints[_entityToComponentIndex.find(jointEntity)->second] = joint;
        }

        /** @brief Sets the BallAndSocketJoint pointer at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param joint Non-owning pointer to the BallAndSocketJoint to store at the index. */
        VE_INLINE void SetJointAtIndex(size_t componentIndex, BallAndSocketJoint *joint) {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            _joints[componentIndex] = joint;
        }

        /** @brief Retrieves the anchor point on body one expressed in that body's local space for the specified entity.
         * @param jointEntity The entity whose anchor point is to be retrieved. Must have a component.
         * @returns Const reference to the local-space anchor point on body one. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _localSpaceAnchorPointsOnBodyOne[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the anchor point on body one expressed in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the local-space anchor point on body one. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyOne.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyOne.");
            return _localSpaceAnchorPointsOnBodyOne[componentIndex];
        }

        /** @brief Sets the anchor point on body one in that body's local space for the specified entity.
         * @param jointEntity The entity whose anchor point is to be set. Must have a component.
         * @param localAnchorPointBodyOne The new local-space anchor point on body one. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity, const glm::vec3 &localAnchorPointBodyOne) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyOne[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBodyOne;
        }

        /** @brief Sets the anchor point on body one in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param localAnchorPointBodyOne The new local-space anchor point on body one. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBodyOne) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyOne.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyOne.");
            _localSpaceAnchorPointsOnBodyOne[componentIndex] = localAnchorPointBodyOne;
        }

        /** @brief Retrieves the anchor point on body two expressed in that body's local space for the specified entity.
         * @param jointEntity The entity whose anchor point is to be retrieved. Must have a component.
         * @returns Const reference to the local-space anchor point on body two. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _localSpaceAnchorPointsOnBodyTwo[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the anchor point on body two expressed in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the local-space anchor point on body two. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyTwo.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyTwo.");
            return _localSpaceAnchorPointsOnBodyTwo[componentIndex];
        }

        /** @brief Sets the anchor point on body two in that body's local space for the specified entity.
         * @param jointEntity The entity whose anchor point is to be set. Must have a component.
         * @param localAnchorPointBodyTwo The new local-space anchor point on body two. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity, const glm::vec3 &localAnchorPointBodyTwo) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyTwo[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBodyTwo;
        }

        /** @brief Sets the anchor point on body two in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param localAnchorPointBodyTwo The new local-space anchor point on body two. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBodyTwo) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyTwo.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyTwo.");
            _localSpaceAnchorPointsOnBodyTwo[componentIndex] = localAnchorPointBodyTwo;
        }

        /** @brief Retrieves the world-space vector from body one's centre of mass to the joint anchor (r1) for the specified entity.
         * @param jointEntity The entity whose r1 vector is to be retrieved. Must have a component.
         * @returns Const reference to the world-space r1 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1World(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r1WorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the world-space vector from body one's centre of mass to the joint anchor (r1) at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the world-space r1 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1WorldAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _r1WorldSpace.size(), "componentIndex out of bounds of _r1WorldSpace.");
            return _r1WorldSpace[componentIndex];
        }

        /** @brief Sets the world-space lever arm from body one's centre of mass to the joint anchor (r1) for the specified entity.
         * @param jointEntity The entity whose r1 vector is to be set. Must have a component.
         * @param r1World The new world-space r1 lever arm. */
        VE_INLINE void SetR1World(Entity jointEntity, const glm::vec3 &r1World) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r1WorldSpace[_entityToComponentIndex.find(jointEntity)->second] = r1World;
        }

        /** @brief Sets the world-space lever arm from body one's centre of mass to the joint anchor (r1) at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param r1World The new world-space r1 lever arm. */
        VE_INLINE void SetR1WorldAtIndex(size_t componentIndex, const glm::vec3 &r1World) {
            VASSERT(componentIndex < _r1WorldSpace.size(), "componentIndex out of bounds of _r1WorldSpace.");
            _r1WorldSpace[componentIndex] = r1World;
        }

        /** @brief Retrieves the world-space vector from body two's centre of mass to the joint anchor (r2) for the specified entity.
         * @param jointEntity The entity whose r2 vector is to be retrieved. Must have a component.
         * @returns Const reference to the world-space r2 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2World(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r2WorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the world-space vector from body two's centre of mass to the joint anchor (r2) at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the world-space r2 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2WorldAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _r2WorldSpace.size(), "componentIndex out of bounds of _r2WorldSpace.");
            return _r2WorldSpace[componentIndex];
        }

        /** @brief Sets the world-space lever arm from body two's centre of mass to the joint anchor (r2) for the specified entity.
         * @param jointEntity The entity whose r2 vector is to be set. Must have a component.
         * @param r2World The new world-space r2 lever arm. */
        VE_INLINE void SetR2World(Entity jointEntity, const glm::vec3 &r2World) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r2WorldSpace[_entityToComponentIndex.find(jointEntity)->second] = r2World;
        }

        /** @brief Sets the world-space lever arm from body two's centre of mass to the joint anchor (r2) at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param r2World The new world-space r2 lever arm. */
        VE_INLINE void SetR2WorldAtIndex(size_t componentIndex, const glm::vec3 &r2World) {
            VASSERT(componentIndex < _r2WorldSpace.size(), "componentIndex out of bounds of _r2WorldSpace.");
            _r2WorldSpace[componentIndex] = r2World;
        }

        /** @brief Retrieves the world-space inertia tensor of body one for the specified entity.
         * @param jointEntity The entity whose inertia tensor is to be retrieved. Must have a component.
         * @returns Const reference to the world-space inertia tensor of body one. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _bodyOneInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the world-space inertia tensor of body one at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the world-space inertia tensor of body one. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyOneInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyOneInertiaTensorsInWorldSpace.");
            return _bodyOneInertiaTensorsInWorldSpace[componentIndex];
        }

        /** @brief Sets the world-space inertia tensor of body one for the specified entity.
         * @param jointEntity The entity whose inertia tensor is to be set. Must have a component.
         * @param i1 The new world-space inertia tensor of body one. */
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity, const glm::mat3 &i1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _bodyOneInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = i1;
        }

        /** @brief Sets the world-space inertia tensor of body one at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param i1 The new world-space inertia tensor of body one. */
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i1) {
            VASSERT(componentIndex < _bodyOneInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyOneInertiaTensorsInWorldSpace.");
            _bodyOneInertiaTensorsInWorldSpace[componentIndex] = i1;
        }

        /** @brief Retrieves the world-space inertia tensor of body two for the specified entity.
         * @param jointEntity The entity whose inertia tensor is to be retrieved. Must have a component.
         * @returns Const reference to the world-space inertia tensor of body two. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _bodyTwoInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the world-space inertia tensor of body two at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the world-space inertia tensor of body two. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyTwoInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyTwoInertiaTensorsInWorldSpace.");
            return _bodyTwoInertiaTensorsInWorldSpace[componentIndex];
        }

        /** @brief Sets the world-space inertia tensor of body two for the specified entity.
         * @param jointEntity The entity whose inertia tensor is to be set. Must have a component.
         * @param i1 The new world-space inertia tensor of body two. */
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity, const glm::mat3 &i1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _bodyTwoInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = i1;
        }

        /** @brief Sets the world-space inertia tensor of body two at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param i1 The new world-space inertia tensor of body two. */
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i1) {
            VASSERT(componentIndex < _bodyTwoInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyTwoInertiaTensorsInWorldSpace.");
            _bodyTwoInertiaTensorsInWorldSpace[componentIndex] = i1;
        }

        /** @brief Retrieves a mutable reference to the position-correction bias vector for the specified entity.
         * The bias vector encodes the Baumgarte stabilization term applied to the translational constraint.
         * @param jointEntity The entity whose bias vector is to be retrieved. Must have a component.
         * @returns Mutable reference to the bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetBiasVector(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _biasVectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the position-correction bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetBiasVectorAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _biasVectors.size(), "componentIndex out of bounds of _biasVectors.");
            return _biasVectors[componentIndex];
        }

        /** @brief Sets the position-correction bias vector for the specified entity.
         * @param jointEntity The entity whose bias vector is to be set. Must have a component.
         * @param biasVector The new bias vector encoding the Baumgarte stabilization term. */
        VE_INLINE void SetBiasVector(Entity jointEntity, const glm::vec3 &biasVector) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _biasVectors[_entityToComponentIndex.find(jointEntity)->second] = biasVector;
        }

        /** @brief Sets the position-correction bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param biasVector The new bias vector encoding the Baumgarte stabilization term. */
        VE_INLINE void SetBiasVectorAtIndex(size_t componentIndex, const glm::vec3 &biasVector) {
            VASSERT(componentIndex < _biasVectors.size(), "componentIndex out of bounds of _biasVectors.");
            _biasVectors[componentIndex] = biasVector;
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the translational constraint of the specified entity.
         * K = J * M^-1 * J^T, where J is the constraint Jacobian.
         * @param jointEntity The entity whose inverse mass matrix is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3x3 inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the translational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3x3 inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassMatrices.size(), "componentIndex out of bounds of _inverseMassMatrices.");
            return _inverseMassMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix (K) for the translational constraint of the specified entity.
         * @param jointEntity The entity whose inverse mass matrix is to be set. Must have a component.
         * @param inverseMassMatrix The new 3x3 inverse mass matrix. */
        VE_INLINE void SetInverseMassMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass matrix (K) for the translational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param inverseMassMatrix The new 3x3 inverse mass matrix. */
        VE_INLINE void SetInverseMassMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassMatrices.size(), "componentIndex out of bounds of _inverseMassMatrices.");
            _inverseMassMatrices[componentIndex] = inverseMassMatrix;
        }

        /** @brief Retrieves a mutable reference to the accumulated translational constraint impulse for the specified entity.
         * This is the total impulse applied to satisfy the ball-and-socket position constraint in the current frame.
         * @param jointEntity The entity whose accumulated impulse is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3D impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulse(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulses[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the accumulated translational constraint impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3D impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulses.size(), "componentIndex out of bounds of _impulses.");
            return _impulses[componentIndex];
        }

        /** @brief Sets the accumulated translational constraint impulse for the specified entity.
         * @param jointEntity The entity whose impulse is to be set. Must have a component.
         * @param impulse The new accumulated 3D impulse vector. */
        VE_INLINE void SetImpulse(Entity jointEntity, const glm::vec3 &impulse) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulses[_entityToComponentIndex.find(jointEntity)->second] = impulse;
        }

        /** @brief Sets the accumulated translational constraint impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param impulse The new accumulated 3D impulse vector. */
        VE_INLINE void SetImpulseAtIndex(size_t componentIndex, const glm::vec3 &impulse) {
            VASSERT(componentIndex < _impulses.size(), "componentIndex out of bounds of _impulses.");
            _impulses[componentIndex] = impulse;
        }

        /** @brief Returns whether the cone angular limit is enabled for the specified entity.
         * @param jointEntity The entity to query. Must have a component.
         * @returns True if the cone limit is active, false otherwise. */
        [[nodiscard]] VE_INLINE bool ConeLimitEnabled(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_coneLimitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns whether the cone angular limit is enabled at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns True if the cone limit is active, false otherwise. */
        [[nodiscard]] VE_INLINE bool ConeLimitEnabledAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _coneLimitEnabledFlags.size(), "componentIndex out of bounds of _coneLimitEnabledFlags.");
            return static_cast<bool>(_coneLimitEnabledFlags[componentIndex]);
        }

        /** @brief Sets the cone angular limit enabled flag for the specified entity.
         * @param jointEntity The entity whose cone limit flag is to be set. Must have a component.
         * @param isLimitEnabled True to activate the cone limit, false to disable it. */
        VE_INLINE void SetConeLimitEnabledFlag(Entity jointEntity, bool isLimitEnabled) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _coneLimitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isLimitEnabled);
        }

        /** @brief Sets the cone angular limit enabled flag at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param isLimitEnabled True to activate the cone limit, false to disable it. */
        VE_INLINE void SetConeLimitEnabledFlagAtIndex(size_t componentIndex, bool isLimitEnabled) {
            VASSERT(componentIndex < _coneLimitEnabledFlags.size(), "componentIndex out of bounds of _coneLimitEnabledFlags.");
            _coneLimitEnabledFlags[componentIndex] = static_cast<u8>(isLimitEnabled);
        }

        /** @brief Retrieves the accumulated scalar impulse applied to enforce the cone limit for the specified entity.
         * @param jointEntity The entity whose cone limit impulse is to be retrieved. Must have a component.
         * @returns The scalar impulse value for the cone limit constraint. */
        [[nodiscard]] VE_INLINE f32 GetConeLimitImpulse(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _coneLimitImpulses[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the accumulated scalar impulse applied to enforce the cone limit at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The scalar impulse value for the cone limit constraint. */
        [[nodiscard]] VE_INLINE f32 GetConeLimitImpulseAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _coneLimitImpulses.size(), "componentIndex out of bounds of _coneLimitImpulses.");
            return _coneLimitImpulses[componentIndex];
        }

        /** @brief Sets the accumulated cone limit impulse for the specified entity.
         * @param jointEntity The entity whose cone limit impulse is to be set. Must have a component.
         * @param impulse The new scalar impulse value for the cone limit constraint. */
        VE_INLINE void SetConeLimitImpulse(Entity jointEntity, f32 impulse) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _coneLimitImpulses[_entityToComponentIndex.find(jointEntity)->second] = impulse;
        }

        /** @brief Sets the accumulated cone limit impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param impulse The new scalar impulse value for the cone limit constraint. */
        VE_INLINE void SetConeLimitImpulseAtIndex(size_t componentIndex, f32 impulse) {
            VASSERT(componentIndex < _coneLimitImpulses.size(), "componentIndex out of bounds of _coneLimitImpulses.");
            _coneLimitImpulses[componentIndex] = impulse;
        }

        /** @brief Retrieves the half-angle of the cone limit (in radians) for the specified entity.
         * @param jointEntity The entity whose cone limit half-angle is to be retrieved. Must have a component.
         * @returns The cone limit half-angle in radians. */
        [[nodiscard]] VE_INLINE f32 GetConeLimitHalfAngle(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _coneLimitHalfAngles[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the half-angle of the cone limit (in radians) at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The cone limit half-angle in radians. */
        [[nodiscard]] VE_INLINE f32 GetConeLimitHalfAngleAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _coneLimitHalfAngles.size(), "componentIndex out of bounds of _coneLimitHalfAngles.");
            return _coneLimitHalfAngles[componentIndex];
        }

        /** @brief Sets the half-angle of the cone limit for the specified entity.
         * @param jointEntity The entity whose cone limit half-angle is to be set. Must have a component.
         * @param halfAngle The new cone limit half-angle in radians. */
        VE_INLINE void SetConeLimitHalfAngle(Entity jointEntity, f32 halfAngle) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _coneLimitHalfAngles[_entityToComponentIndex.find(jointEntity)->second] = halfAngle;
        }

        /** @brief Sets the half-angle of the cone limit at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param halfAngle The new cone limit half-angle in radians. */
        VE_INLINE void SetConeLimitHalfAngleAtIndex(size_t componentIndex, f32 halfAngle) {
            VASSERT(componentIndex < _coneLimitHalfAngles.size(), "componentIndex out of bounds of _coneLimitHalfAngles.");
            _coneLimitHalfAngles[componentIndex] = halfAngle;
        }

        /** @brief Retrieves the effective inverse mass (scalar K) for the cone limit constraint of the specified entity.
         * @param jointEntity The entity whose cone limit inverse mass is to be retrieved. Must have a component.
         * @returns The scalar inverse mass value for the cone limit constraint. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixConeLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrixConeLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the effective inverse mass (scalar K) for the cone limit constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The scalar inverse mass value for the cone limit constraint. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixConeLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _inverseMassMatrixConeLimits.size(), "componentIndex out of bounds of _inverseMassMatrixConeLimits.");
            return _inverseMassMatrixConeLimits[componentIndex];
        }

        /** @brief Sets the effective inverse mass (scalar K) for the cone limit constraint of the specified entity.
         * @param jointEntity The entity whose cone limit inverse mass is to be set. Must have a component.
         * @param inverseMassMatrix The new scalar inverse mass value for the cone limit constraint. */
        VE_INLINE void SetInverseMassMatrixConeLimit(Entity jointEntity, f32 inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrixConeLimits[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass (scalar K) for the cone limit constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param inverseMassMatrix The new scalar inverse mass value for the cone limit constraint. */
        VE_INLINE void SetInverseMassMatrixConeLimitAtIndex(size_t componentIndex, f32 inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassMatrixConeLimits.size(), "componentIndex out of bounds of _inverseMassMatrixConeLimits.");
            _inverseMassMatrixConeLimits[componentIndex] = inverseMassMatrix;
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Non-owning pointers to the runtime BallAndSocketJoint objects, one per joint entity. */
        std::vector<BallAndSocketJoint *> _joints;

        /** @brief Anchor points for each joint expressed in body one's local space. Used each frame to recompute the world-space lever arm r1. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyOne;

        /** @brief Anchor points for each joint expressed in body two's local space. Used each frame to recompute the world-space lever arm r2. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyTwo;

        /** @brief World-space vectors from body one's centre of mass to the joint anchor (r1), recomputed each simulation step. */
        std::vector<glm::vec3> _r1WorldSpace;

        /** @brief World-space vectors from body two's centre of mass to the joint anchor (r2), recomputed each simulation step. */
        std::vector<glm::vec3> _r2WorldSpace;

        /** @brief World-space inertia tensors of body one, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyOneInertiaTensorsInWorldSpace;

        /** @brief World-space inertia tensors of body two, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyTwoInertiaTensorsInWorldSpace;

        /** @brief Baumgarte position-correction bias vectors for the translational constraint, computed each simulation step. */
        std::vector<glm::vec3> _biasVectors;

        /** @brief Effective inverse mass matrices (K = J * M^-1 * J^T) for the translational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassMatrices;

        /** @brief Accumulated translational constraint impulses, warm-started across solver iterations within a step. */
        std::vector<glm::vec3> _impulses;

        /** @brief Flags (stored as u8 for dense packing) indicating whether the cone angular limit is active for each joint. */
        std::vector<u8> _coneLimitEnabledFlags;

        /** @brief Accumulated scalar impulses applied to enforce the cone angular limit, one per joint. */
        std::vector<f32> _coneLimitImpulses;

        /** @brief Half-angles (in radians) defining the cone limit for each joint. */
        std::vector<f32> _coneLimitHalfAngles;

        /** @brief Effective scalar inverse masses (K) for the cone limit constraint, computed each simulation step. */
        std::vector<f32> _inverseMassMatrixConeLimits;

        /** @brief Baumgarte position-correction bias scalars for the cone limit constraint, computed each simulation step. */
        std::vector<f32> _coneLimitBiases;

        /** @brief Flags (stored as u8 for dense packing) indicating whether the cone limit was violated and is actively being enforced. */
        std::vector<u8> _coneLimitViolatedFlags;

        /** @brief Cross products of the cone limit axes (axis1 x axis2) in world space, cached each step for use in the cone limit Jacobian. */
        std::vector<glm::vec3> _coneLimitAxisOneCrossTwoProducts;
    };

} // namespace Vulkyrie
