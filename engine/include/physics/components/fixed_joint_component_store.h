#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/components/component_store.h"
#include "physics/constraint/fixed_joint.h"

namespace Vulkyrie {

    /** @brief Stores fixed joint components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that represents a fixed joint owns exactly one component, which binds it to a
     * FixedJoint object and tracks all constraint solver state needed for each simulation step:
     * anchor points in local space, world-space lever arms, inertia tensors, translational and
     * rotational impulses, their respective inverse mass matrices and bias vectors, and the initial
     * inverse orientation difference used to enforce the rotational constraint. The store maintains
     * the dense active-zone invariant inherited from ComponentStore: active components occupy
     * indices [0, _activeCount) and inactive components occupy [_activeCount, size). Swap operations
     * keep all parallel arrays in sync with _entities at all times, enabling efficient iteration
     * during physics updates. */
    class FixedJointComponentStore : public ComponentStore {
    public:
        /** @brief Constructs an instance of FixedJointComponentStore and reserves initial storage for all parallel arrays. */
        FixedJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(FixedJointComponentStore);

        /** @brief Destructor for FixedJointComponentStore. */
        ~FixedJointComponentStore() override = default;

        /** @brief Adds a component for the specified joint entity. Active components are stored at the
         * front of the vector and inactive ones at the back to maintain dense packing for efficient iteration.
         * @param jointEntity The entity to which the component will be added. Must not already have a component.
         * @param active Whether the joint entity is currently active. */
        void AddComponent(Entity jointEntity, bool active);

        /** @brief Retrieves the FixedJoint pointer associated with the specified entity.
         * @param jointEntity The entity whose joint is to be retrieved. Must have a component.
         * @returns Non-owning pointer to the FixedJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE FixedJoint *GetJoint(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _joints[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves the FixedJoint pointer at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Non-owning pointer to the FixedJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE FixedJoint *GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            return _joints[componentIndex];
        }

        /** @brief Sets the FixedJoint pointer for the specified entity.
         * @param jointEntity The entity whose joint pointer is to be set. Must have a component.
         * @param joint Non-owning pointer to the FixedJoint to associate with the entity. */
        VE_INLINE void SetJoint(Entity jointEntity, FixedJoint *joint) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _joints[_entityToComponentIndex.find(jointEntity)->second] = joint;
        }

        /** @brief Sets the FixedJoint pointer at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param joint Non-owning pointer to the FixedJoint to store at the index. */
        VE_INLINE void SetJointAtIndex(size_t componentIndex, FixedJoint *joint) {
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
         * @param localAnchorPointBody1 The new local-space anchor point on body one. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity, const glm::vec3 &localAnchorPointBody1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyOne[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBody1;
        }

        /** @brief Sets the anchor point on body one in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param localAnchorPointBody1 The new local-space anchor point on body one. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBody1) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyOne.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyOne.");
            _localSpaceAnchorPointsOnBodyOne[componentIndex] = localAnchorPointBody1;
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
         * @param localAnchorPointBody2 The new local-space anchor point on body two. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity, const glm::vec3 &localAnchorPointBody2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyTwo[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBody2;
        }

        /** @brief Sets the anchor point on body two in that body's local space at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param localAnchorPointBody2 The new local-space anchor point on body two. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBody2) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyTwo.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyTwo.");
            _localSpaceAnchorPointsOnBodyTwo[componentIndex] = localAnchorPointBody2;
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

        /** @brief Retrieves a mutable reference to the accumulated translational impulse for the specified entity.
         * @param jointEntity The entity whose translational impulse is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3D translational impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseTranslation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the accumulated translational impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3D translational impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseTranslationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            return _impulseTranslations[componentIndex];
        }

        /** @brief Sets the accumulated translational impulse for the specified entity.
         * @param jointEntity The entity whose translational impulse is to be set. Must have a component.
         * @param impulseTranslation The new 3D translational impulse vector. */
        VE_INLINE void SetImpulseTranslation(Entity jointEntity, const glm::vec3 &impulseTranslation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second] = impulseTranslation;
        }

        /** @brief Sets the accumulated translational impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param impulseTranslation The new 3D translational impulse vector. */
        VE_INLINE void SetImpulseTranslationAtIndex(size_t componentIndex, const glm::vec3 &impulseTranslation) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            _impulseTranslations[componentIndex] = impulseTranslation;
        }

        /** @brief Retrieves a mutable reference to the accumulated rotational impulse for the specified entity.
         * @param jointEntity The entity whose rotational impulse is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3D rotational impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseRotation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseRotations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the accumulated rotational impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3D rotational impulse vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseRotationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            return _impulseRotations[componentIndex];
        }

        /** @brief Sets the accumulated rotational impulse for the specified entity.
         * @param jointEntity The entity whose rotational impulse is to be set. Must have a component.
         * @param impulseRotation The new 3D rotational impulse vector. */
        VE_INLINE void SetImpulseRotation(Entity jointEntity, const glm::vec3 &impulseRotation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseRotations[_entityToComponentIndex.find(jointEntity)->second] = impulseRotation;
        }

        /** @brief Sets the accumulated rotational impulse at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param impulseRotation The new 3D rotational impulse vector. */
        VE_INLINE void SetImpulseRotationAtIndex(size_t componentIndex, const glm::vec3 &impulseRotation) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            _impulseRotations[componentIndex] = impulseRotation;
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the translational constraint of the specified entity.
         * @param jointEntity The entity whose inverse mass matrix is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3x3 translational inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the translational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3x3 translational inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            return _inverseMassTranslationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix (K) for the translational constraint of the specified entity.
         * @param jointEntity The entity whose inverse mass matrix is to be set. Must have a component.
         * @param inverseMassMatrix The new 3x3 translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass matrix (K) for the translational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param inverseMassMatrix The new 3x3 translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            _inverseMassTranslationMatrices[componentIndex] = inverseMassMatrix;
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the rotational constraint of the specified entity.
         * @param jointEntity The entity whose inverse mass matrix is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3x3 rotational inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassRotationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the effective inverse mass matrix (K) for the rotational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3x3 rotational inverse mass matrix. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassRotationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            return _inverseMassRotationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix (K) for the rotational constraint of the specified entity.
         * @param jointEntity The entity whose inverse mass matrix is to be set. Must have a component.
         * @param inverseMassMatrix The new 3x3 rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass matrix (K) for the rotational constraint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param inverseMassMatrix The new 3x3 rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            _inverseMassRotationMatrices[componentIndex] = inverseMassMatrix;
        }

        /** @brief Retrieves a mutable reference to the Baumgarte translation bias vector for the specified entity.
         * @param jointEntity The entity whose translation bias is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3D translation bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetTranslationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _translationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the Baumgarte translation bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3D translation bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetTranslationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            return _translationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte translation bias vector for the specified entity.
         * @param jointEntity The entity whose translation bias is to be set. Must have a component.
         * @param translationBias The new 3D translation bias vector. */
        VE_INLINE void SetTranslationBias(Entity jointEntity, const glm::vec3 &translationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _translationBiases[_entityToComponentIndex.find(jointEntity)->second] = translationBias;
        }

        /** @brief Sets the Baumgarte translation bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param translationBias The new 3D translation bias vector. */
        VE_INLINE void SetTranslationBiasAtIndex(size_t componentIndex, const glm::vec3 &translationBias) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            _translationBiases[componentIndex] = translationBias;
        }

        /** @brief Retrieves a mutable reference to the Baumgarte rotation bias vector for the specified entity.
         * @param jointEntity The entity whose rotation bias is to be retrieved. Must have a component.
         * @returns Mutable reference to the 3D rotation bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetRotationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _rotationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the Baumgarte rotation bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the 3D rotation bias vector. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetRotationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _rotationBiases.size(), "componentIndex out of bounds of _rotationBiases.");
            return _rotationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte rotation bias vector for the specified entity.
         * @param jointEntity The entity whose rotation bias is to be set. Must have a component.
         * @param rotationBias The new 3D rotation bias vector. */
        VE_INLINE void SetRotationBias(Entity jointEntity, const glm::vec3 &rotationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _rotationBiases[_entityToComponentIndex.find(jointEntity)->second] = rotationBias;
        }

        /** @brief Sets the Baumgarte rotation bias vector at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param rotationBias The new 3D rotation bias vector. */
        VE_INLINE void SetRotationBiasAtIndex(size_t componentIndex, const glm::vec3 &rotationBias) {
            VASSERT(componentIndex < _rotationBiases.size(), "componentIndex out of bounds of _rotationBiases.");
            _rotationBiases[componentIndex] = rotationBias;
        }

        /** @brief Retrieves a mutable reference to the inverse of the initial orientation difference between the two bodies for the specified entity.
         * This quaternion is computed once at joint creation and used each step to drive the rotational constraint back to its rest configuration.
         * @param jointEntity The entity whose initial orientation difference inverse is to be retrieved. Must have a component.
         * @returns Mutable reference to the inverse orientation difference quaternion. */
        [[nodiscard]] VE_INLINE glm::quat &GetInitialOrientationDifferenceInverse(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _initialOrientationDifferenceInverses[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the inverse of the initial orientation difference at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Mutable reference to the inverse orientation difference quaternion. */
        [[nodiscard]] VE_INLINE glm::quat &GetInitialOrientationDifferenceInverseAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _initialOrientationDifferenceInverses.size(), "componentIndex out of bounds of _initialOrientationDifferenceInverses.");
            return _initialOrientationDifferenceInverses[componentIndex];
        }

        /** @brief Sets the inverse of the initial orientation difference between the two bodies for the specified entity.
         * @param jointEntity The entity whose initial orientation difference inverse is to be set. Must have a component.
         * @param initialOrientationDifferenceInverse The inverse of the quaternion representing the initial relative orientation. */
        VE_INLINE void SetInitialOrientationDifferenceInverse(Entity jointEntity, const glm::quat &initialOrientationDifferenceInverse) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _initialOrientationDifferenceInverses[_entityToComponentIndex.find(jointEntity)->second] = initialOrientationDifferenceInverse;
        }

        /** @brief Sets the inverse of the initial orientation difference at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param initialOrientationDifferenceInverse The inverse of the quaternion representing the initial relative orientation. */
        VE_INLINE void SetInitialOrientationDifferenceInverseAtIndex(size_t componentIndex, const glm::quat &initialOrientationDifferenceInverse) {
            VASSERT(componentIndex < _initialOrientationDifferenceInverses.size(), "componentIndex out of bounds of _initialOrientationDifferenceInverses.");
            _initialOrientationDifferenceInverses[componentIndex] = initialOrientationDifferenceInverse;
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Non-owning pointers to the runtime FixedJoint objects, one per joint entity. */
        std::vector<FixedJoint *> _joints;

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

        /** @brief Accumulated translational constraint impulses, warm-started across solver iterations within a step. */
        std::vector<glm::vec3> _impulseTranslations;

        /** @brief Accumulated rotational constraint impulses, warm-started across solver iterations within a step. */
        std::vector<glm::vec3> _impulseRotations;

        /** @brief Effective inverse mass matrices (K) for the translational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassTranslationMatrices;

        /** @brief Effective inverse mass matrices (K) for the rotational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassRotationMatrices;

        /** @brief Baumgarte position-correction bias vectors for the translational constraint, computed each simulation step. */
        std::vector<glm::vec3> _translationBiases;

        /** @brief Baumgarte position-correction bias vectors for the rotational constraint, computed each simulation step. */
        std::vector<glm::vec3> _rotationBiases;

        /** @brief Inverses of the initial relative orientations between the two bodies, computed once at joint creation and used to enforce the rotational constraint. */
        std::vector<glm::quat> _initialOrientationDifferenceInverses;
    };

} // namespace Vulkyrie
