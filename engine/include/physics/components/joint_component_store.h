#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/entity.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    class Joint;
    enum class JointType;
    enum class JointsPositionCorrectionTechnique;

    struct JointComponent {
        const Entity BodyOneEntity;
        const Entity BodyTwoEntity;
        Vulkyrie::Joint *Joint;
        Vulkyrie::JointType JointType;
        Vulkyrie::JointsPositionCorrectionTechnique PositionCorrectionTechnique;
        bool CollisionEnabled;

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

    class JointComponentStore : public ComponentStore {
    public:
        JointComponentStore();

        VE_DELETE_MOVE_AND_COPY(JointComponentStore);

        /** @brief Default destructor for JointComponentStore */
        ~JointComponentStore() override = default;

        void AddComponent(Entity entity, const JointComponent &component, bool active = true);

        [[nodiscard]] VE_INLINE Entity GetBodyOneEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyOneEntities.size(), "componentIndex out of bounds of _bodyOneEntities.");

            return _bodyOneEntities[componentIndex];
        }

        [[nodiscard]] VE_INLINE Entity GetBodyOneEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "GetBodyOneEntity called for unknown entity.");

            return _bodyOneEntities[_entityToComponentIndex.find(entity)->second];
        }

        [[nodiscard]] VE_INLINE Entity GetBodyTwoEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyTwoEntities.size(), "componentIndex out of bounds of _bodyTwoEntities.");

            return _bodyTwoEntities[componentIndex];
        }

        [[nodiscard]] VE_INLINE Entity GetBodyTwoEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "GetBodyTwoEntity called for unknown entity.");

            return _bodyTwoEntities[_entityToComponentIndex.find(entity)->second];
        }

        [[nodiscard]] VE_INLINE const Joint &GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");

            return *_joints[componentIndex];
        }

        [[nodiscard]] VE_INLINE const Joint &GetJoint(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJoint called for unknown entity.");

            return *_joints[_entityToComponentIndex.find(entity)->second];
        }

        [[nodiscard]] VE_INLINE JointType GetJointTypesAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _jointTypes.size(), "componentIndex out of bounds of _jointTypes.");

            return _jointTypes[componentIndex];
        }

        [[nodiscard]] VE_INLINE JointType GetJointTypes(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJointTypes called for unknown entity.");

            return _jointTypes[_entityToComponentIndex.find(entity)->second];
        }

        [[nodiscard]] VE_INLINE JointsPositionCorrectionTechnique GetJointsPositionCorrectionTechniqueAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _positionCorrectionTechniques.size(), "componentIndex out of bounds of _positionCorrectionTechniques.");

            return _positionCorrectionTechniques[componentIndex];
        }

        [[nodiscard]] VE_INLINE JointsPositionCorrectionTechnique GetJointsPositionCorrectionTechnique(Entity entity) const {
            VASSERT(HasComponent(entity), "GetJointsPositionCorrectionTechnique called for unknown entity.");

            return _positionCorrectionTechniques[_entityToComponentIndex.find(entity)->second];
        }

        VE_INLINE void SetPositionCorrectionTechniques(Entity entity, JointsPositionCorrectionTechnique technique) {
            VASSERT(HasComponent(entity), "SetPositionCorrectionTechniques called for unknown entity.");

            _positionCorrectionTechniques[_entityToComponentIndex.find(entity)->second] = technique;
        }

        VE_INLINE void SetPositionCorrectionTechniquesAtIndex(size_t componentIndex, JointsPositionCorrectionTechnique technique) {
            VASSERT(componentIndex < _positionCorrectionTechniques.size(), "componentIndex out of bounds of _positionCorrectionTechniques.");

            _positionCorrectionTechniques[componentIndex] = technique;
        }

        [[nodiscard]] VE_INLINE bool IsCollisionEnabledForEntityAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _collisionEnabledFlags.size(), "componentIndex out of bounds of _collisionEnabledFlags.");

            return static_cast<bool>(_collisionEnabledFlags[componentIndex]);
        }

        [[nodiscard]] VE_INLINE bool IsCollisionEnabledForEntity(Entity entity) const {
            VASSERT(HasComponent(entity), "IsCollisionEnabledForEntity called for unknown entity.");

            return static_cast<bool>(_collisionEnabledFlags[_entityToComponentIndex.find(entity)->second]);
        }

        VE_INLINE void SetCollisionEnabledFlag(Entity entity, bool collisionEnabled) {
            VASSERT(HasComponent(entity), "SetCollisionEnabledFlag called for unknown entity.");

            _collisionEnabledFlags[_entityToComponentIndex.find(entity)->second] = static_cast<u8>(collisionEnabled);
        }

        VE_INLINE void SetCollisionEnabledFlagAtIndex(size_t componentIndex, bool collisionEnabled) {
            VASSERT(componentIndex < _collisionEnabledFlags.size(), "componentIndex out of bounds of _collisionEnabledFlags.");

            _collisionEnabledFlags[componentIndex] = static_cast<u8>(collisionEnabled);
        }

        [[nodiscard]] VE_INLINE bool IsEntityInIslandAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _jointInIslandFlags.size(), "componentIndex out of bounds of _jointInIslandFlags.");

            return static_cast<bool>(_jointInIslandFlags[componentIndex]);
        }

        [[nodiscard]] VE_INLINE bool IsEntityInIsland(Entity entity) const {
            VASSERT(HasComponent(entity), "IsEntityInIsland called for unknown entity.");

            return static_cast<bool>(_jointInIslandFlags[_entityToComponentIndex.find(entity)->second]);
        }

        VE_INLINE void SetJointInIslandFlags(Entity entity, bool isInIsland) {
            VASSERT(HasComponent(entity), "SetJointInIslandFlags called for unknown entity.");

            _jointInIslandFlags[_entityToComponentIndex.find(entity)->second] = static_cast<u8>(isInIsland);
        }

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
        std::vector<Entity> _bodyOneEntities;
        std::vector<Entity> _bodyTwoEntities;
        std::vector<Joint *> _joints;
        std::vector<JointType> _jointTypes;
        std::vector<JointsPositionCorrectionTechnique> _positionCorrectionTechniques;
        std::vector<u8> _collisionEnabledFlags;
        std::vector<u8> _jointInIslandFlags;
    };

} // namespace Vulkyrie
