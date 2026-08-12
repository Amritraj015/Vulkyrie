#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/ecs/entity.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    class Joint;
    enum class JointType;
    enum class JointsPositionCorrectionTechnique;

    /** @brief Plain-data component passed to `JointComponentStore::AddComponent`.
     *
     * Carries all configuration required to register a joint between two body entities. All fields
     * are copied into the store's parallel arrays on insertion; callers may construct this struct on
     * the stack and pass by const reference. */
    struct JointComponent {
        /** @brief The entity representing the first body involved in the joint. */
        const Entity BodyOneEntity;

        /** @brief The entity representing the second body involved in the joint. */
        const Entity BodyTwoEntity;

        /** @brief Non-owning pointer to the runtime Joint object that implements the constraint. */
        Vulkyrie::Joint *Joint;

        /** @brief Discriminator indicating which concrete joint type this component represents. */
        Vulkyrie::JointType JointType;

        /** @brief The position correction technique used by the constraint solver for this joint. */
        Vulkyrie::JointsPositionCorrectionTechnique PositionCorrectionTechnique;

        /** @brief Whether collision detection between the two constrained bodies is enabled. */
        bool CollisionEnabled;

        /** @brief Constructs a JointComponent with the specified parameters.
         * @param bodyOneEntity The entity representing the first body.
         * @param bodyTwoEntity The entity representing the second body.
         * @param joint Non-owning pointer to the runtime Joint object.
         * @param jointType Discriminator for the concrete joint type.
         * @param positionCorrectionTechnique Position correction technique used by the solver.
         * @param collisionEnabled Whether collision between the two bodies is enabled. */
        JointComponent(Entity bodyOneEntity,
                       Entity bodyTwoEntity,
                       Vulkyrie::Joint *joint,
                       Vulkyrie::JointType jointType,
                       Vulkyrie::JointsPositionCorrectionTechnique positionCorrectionTechnique,
                       bool collisionEnabled)
            : BodyOneEntity(bodyOneEntity)
            , BodyTwoEntity(bodyTwoEntity)
            , Joint(joint)
            , JointType(jointType)
            , PositionCorrectionTechnique(positionCorrectionTechnique)
            , CollisionEnabled(collisionEnabled) {
        }
    };

    /** @brief Stores joint components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that represents a joint in the physics simulation owns exactly one JointComponent,
     * which binds it to a Joint object and tracks the two body entities it constrains, the joint type,
     * the position correction technique, and per-joint flags such as collision enable and island
     * membership. The store maintains the dense active-zone invariant inherited from ComponentStore:
     * active components occupy indices [0, _activeCount) and inactive components occupy
     * [_activeCount, size). Swap operations keep all parallel arrays in sync with _entities at all
     * times, enabling efficient iteration during physics updates. */
    class JointComponentStore : public ComponentStore {
    public:
        /** @brief Constructs an instance of JointComponentStore and reserves initial storage for all parallel arrays. */
        JointComponentStore();

        VE_DELETE_MOVE_AND_COPY(JointComponentStore);

        /** @brief Destructor for JointComponentStore. */
        ~JointComponentStore() override = default;

        /** @brief Adds a JointComponent to the specified entity. Active components are stored at the front of the
         * vector and inactive ones at the back to maintain dense packing for efficient iteration.
         * @param entity The entity to which the JointComponent will be added. Must not already have a component.
         * @param component The JointComponent to be added.
         * @param active Whether the joint entity is currently active. */
        void AddComponent(Entity entity, const JointComponent &component, bool active = true);

        /** @brief Retrieves the first body entity for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The entity representing body one of the joint. */
        [[nodiscard]] VE_INLINE Entity GetBodyOneEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyOneEntities.size(), "componentIndex out of bounds of _bodyOneEntities.");
            return _bodyOneEntities[componentIndex];
        }

        /** @brief Retrieves the first body entity for the specified joint entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns The entity representing body one of the joint. */
        [[nodiscard]] VE_INLINE Entity GetBodyOneEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "GetBodyOneEntity called for unknown entity.");
            return _bodyOneEntities[_entityToComponentIndex.find(entity)->second];
        }

        /** @brief Retrieves the second body entity for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The entity representing body two of the joint. */
        [[nodiscard]] VE_INLINE Entity GetBodyTwoEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyTwoEntities.size(), "componentIndex out of bounds of _bodyTwoEntities.");
            return _bodyTwoEntities[componentIndex];
        }

        /** @brief Retrieves the second body entity for the specified joint entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns The entity representing body two of the joint. */
        [[nodiscard]] VE_INLINE Entity GetBodyTwoEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "GetBodyTwoEntity called for unknown entity.");
            return _bodyTwoEntities[_entityToComponentIndex.find(entity)->second];
        }

        /** @brief Retrieves a const reference to the runtime Joint at the given component index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns Const reference to the Joint object. */
        [[nodiscard]] VE_INLINE const Joint &GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            return *_joints[componentIndex];
        }

        /** @brief Retrieves a const reference to the runtime Joint for the specified entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns Const reference to the Joint object. */
        [[nodiscard]] VE_INLINE const Joint &GetJoint(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJoint called for unknown entity.");
            return *_joints[_entityToComponentIndex.find(entity)->second];
        }

        /** @brief Retrieves the joint type for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The JointType discriminator for the component. */
        [[nodiscard]] VE_INLINE JointType GetJointTypesAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _jointTypes.size(), "componentIndex out of bounds of _jointTypes.");
            return _jointTypes[componentIndex];
        }

        /** @brief Retrieves the joint type for the specified joint entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns The JointType discriminator for the joint. */
        [[nodiscard]] VE_INLINE JointType GetJointTypes(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJointTypes called for unknown entity.");
            return _jointTypes[_entityToComponentIndex.find(entity)->second];
        }

        /** @brief Retrieves the position correction technique for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns The JointsPositionCorrectionTechnique for the component. */
        [[nodiscard]] VE_INLINE JointsPositionCorrectionTechnique GetJointsPositionCorrectionTechniqueAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _positionCorrectionTechniques.size(), "componentIndex out of bounds of _positionCorrectionTechniques.");
            return _positionCorrectionTechniques[componentIndex];
        }

        /** @brief Retrieves the position correction technique for the specified joint entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns The JointsPositionCorrectionTechnique for the joint. */
        [[nodiscard]] VE_INLINE JointsPositionCorrectionTechnique GetJointsPositionCorrectionTechnique(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJointsPositionCorrectionTechnique called for unknown entity.");
            return _positionCorrectionTechniques[_entityToComponentIndex.find(entity)->second];
        }

        /** @brief Sets the position correction technique for the specified joint entity.
         * @param entity The joint entity to update. Must have a component.
         * @param technique The new position correction technique to assign. */
        VE_INLINE void SetPositionCorrectionTechniques(Entity entity, JointsPositionCorrectionTechnique technique) {
            VASSERT(HasComponent(entity), "SetPositionCorrectionTechniques called for unknown entity.");
            _positionCorrectionTechniques[_entityToComponentIndex.find(entity)->second] = technique;
        }

        /** @brief Sets the position correction technique for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param technique The new position correction technique to assign. */
        VE_INLINE void SetPositionCorrectionTechniquesAtIndex(size_t componentIndex, JointsPositionCorrectionTechnique technique) {
            VASSERT(componentIndex < _positionCorrectionTechniques.size(), "componentIndex out of bounds of _positionCorrectionTechniques.");
            _positionCorrectionTechniques[componentIndex] = technique;
        }

        /** @brief Returns whether collision detection is enabled between the two bodies for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns True if collision is enabled, false otherwise. */
        [[nodiscard]] VE_INLINE bool IsCollisionEnabledForEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _collisionEnabledFlags.size(), "componentIndex out of bounds of _collisionEnabledFlags.");
            return static_cast<bool>(_collisionEnabledFlags[componentIndex]);
        }

        /** @brief Returns whether collision detection is enabled between the two bodies for the specified joint entity.
         * @param entity The joint entity to query. Must have a component.
         * @returns True if collision is enabled, false otherwise. */
        [[nodiscard]] VE_INLINE bool IsCollisionEnabledForEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "IsCollisionEnabledForEntity called for unknown entity.");
            return static_cast<bool>(_collisionEnabledFlags[_entityToComponentIndex.find(entity)->second]);
        }

        /** @brief Sets the collision enabled flag for the specified joint entity.
         * @param entity The joint entity to update. Must have a component.
         * @param collisionEnabled True to enable collision between the two bodies, false to disable. */
        VE_INLINE void SetCollisionEnabledFlag(Entity entity, bool collisionEnabled) {
            VASSERT(HasComponent(entity), "SetCollisionEnabledFlag called for unknown entity.");
            _collisionEnabledFlags[_entityToComponentIndex.find(entity)->second] = static_cast<u8>(collisionEnabled);
        }

        /** @brief Sets the collision enabled flag for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param collisionEnabled True to enable collision between the two bodies, false to disable. */
        VE_INLINE void SetCollisionEnabledFlagAtIndex(size_t componentIndex, bool collisionEnabled) {
            VASSERT(componentIndex < _collisionEnabledFlags.size(), "componentIndex out of bounds of _collisionEnabledFlags.");
            _collisionEnabledFlags[componentIndex] = static_cast<u8>(collisionEnabled);
        }

        /** @brief Returns whether the joint at the given index has been added to a simulation island.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @returns True if the joint is part of an island, false otherwise. */
        [[nodiscard]] VE_INLINE bool IsEntityInIslandAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _jointInIslandFlags.size(), "componentIndex out of bounds of _jointInIslandFlags.");
            return static_cast<bool>(_jointInIslandFlags[componentIndex]);
        }

        /** @brief Returns whether the specified joint entity has been added to a simulation island.
         * @param entity The joint entity to query. Must have a component.
         * @returns True if the joint is part of an island, false otherwise. */
        [[nodiscard]] VE_INLINE bool IsEntityInIsland(Entity entity) const {
            VASSERT(HasComponent(entity), "IsEntityInIsland called for unknown entity.");
            return static_cast<bool>(_jointInIslandFlags[_entityToComponentIndex.find(entity)->second]);
        }

        /** @brief Sets the island membership flag for the specified joint entity.
         * @param entity The joint entity to update. Must have a component.
         * @param isInIsland True if the joint has been added to a simulation island, false otherwise. */
        VE_INLINE void SetJointInIslandFlags(Entity entity, bool isInIsland) {
            VASSERT(HasComponent(entity), "SetJointInIslandFlags called for unknown entity.");
            _jointInIslandFlags[_entityToComponentIndex.find(entity)->second] = static_cast<u8>(isInIsland);
        }

        /** @brief Sets the island membership flag for the component at the given index.
         * @param componentIndex Index into the parallel arrays. Must be in bounds.
         * @param isInIsland True if the joint has been added to a simulation island, false otherwise. */
        VE_INLINE void SetJointInIslandFlagsAtIndex(size_t componentIndex, bool isInIsland) {
            VASSERT(componentIndex < _jointInIslandFlags.size(), "componentIndex out of bounds of _jointInIslandFlags.");
            _jointInIslandFlags[componentIndex] = static_cast<u8>(isInIsland);
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Entities representing body one for each joint, parallel to all other arrays. */
        std::vector<Entity> _bodyOneEntities;

        /** @brief Entities representing body two for each joint, parallel to all other arrays. */
        std::vector<Entity> _bodyTwoEntities;

        /** @brief Non-owning pointers to the runtime Joint objects, one per joint entity. */
        std::vector<Joint *> _joints;

        /** @brief Discriminators identifying the concrete joint type for each entry, used for type-safe dispatch. */
        std::vector<JointType> _jointTypes;

        /** @brief Position correction techniques assigned to each joint for use by the constraint solver. */
        std::vector<JointsPositionCorrectionTechnique> _positionCorrectionTechniques;

        /** @brief Flags (stored as u8 for dense packing) indicating whether collision is enabled between the two bodies of each joint. */
        std::vector<u8> _collisionEnabledFlags;

        /** @brief Flags (stored as u8 for dense packing) indicating whether each joint has been assigned to a simulation island. */
        std::vector<u8> _jointInIslandFlags;
    };

} // namespace Vulkyrie
