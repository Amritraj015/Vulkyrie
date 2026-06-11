#pragma once

#include "vlkypch.h"
#include "physics/components/component_store.h"
#include "physics/constraint/ball_and_socket_joint.h"

namespace Vulkyrie {

    struct BallAndSocketJointComponent final {
        f32 ConeLimitHalfAngle;
        bool ConeLimitEnabled;

        BallAndSocketJointComponent(bool coneLimitEnabled, f32 coneLimitHalfAngle)
            : ConeLimitHalfAngle(coneLimitHalfAngle)
            , ConeLimitEnabled(coneLimitEnabled) {
        }
    };

    class BallAndSocketJointComponentStore : public ComponentStore {
    public:
        BallAndSocketJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJointComponentStore);

        ~BallAndSocketJointComponentStore() override = default;

        void AddComponent(Entity jointEntity, const BallAndSocketJointComponent &component, bool active);

        [[nodiscard]] VE_INLINE BallAndSocketJoint *GetJoint(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE BallAndSocketJoint *GetJointAtIndex(size_t componentIndex) const;
        VE_INLINE void SetJoint(Entity jointEntity, BallAndSocketJoint *joint) const;
        VE_INLINE void SetJointAtIndex(size_t componentIndex, BallAndSocketJoint *joint) const;

        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex) const;
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity, const glm::vec3 &localAnchorPointBodyOne);
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBodyOne);

        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex) const;
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity, const glm::vec3 &localAnchorPointBodyTwo);
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBodyTwo);

        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1World(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1WorldAtIndex(size_t componentIndex) const;
        VE_INLINE void SetR1World(Entity jointEntity, const glm::vec3 &r1World);
        VE_INLINE void SetR1WorldAtIndex(size_t componentIndex, const glm::vec3 &r1World);

        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2World(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2WorldAtIndex(size_t componentIndex) const;
        VE_INLINE void SetR2World(Entity jointEntity, const glm::vec3 &r2World);
        VE_INLINE void SetR2WorldAtIndex(size_t componentIndex, const glm::vec3 &r2World);

        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex) const;
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity, const glm::mat3 &i1);
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i1);

        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex) const;
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity, const glm::mat3 &i1);
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i1);

        [[nodiscard]] VE_INLINE glm::vec3 &GetBiasVector(Entity jointEntity);
        [[nodiscard]] VE_INLINE glm::vec3 &GetBiasVectorAtIndex(size_t componentIndex);
        VE_INLINE void SetBiasVector(Entity jointEntity, const glm::vec3 &biasVector);
        VE_INLINE void SetBiasVectorAtIndex(size_t componentIndex, const glm::vec3 &biasVector);

        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassMatrix(Entity jointEntity);
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassMatrixAtIndex(size_t componentIndex);
        VE_INLINE void SetInverseMassMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix);
        VE_INLINE void SetInverseMassMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix);

        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulse(Entity jointEntity);
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseAtIndex(size_t componentIndex);
        VE_INLINE void SetImpulse(Entity jointEntity, const glm::vec3 &impulse);
        VE_INLINE void SetImpulseAtIndex(size_t componentIndex, const glm::vec3 &impulse);

        [[nodiscard]] VE_INLINE bool ConeLimitEnabled(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE bool ConeLimitEnabledAtIndex(size_t componentIndex) const;
        VE_INLINE void SetConeLimitEnabledFlag(Entity jointEntity, bool isLimitEnabled);
        VE_INLINE void SetConeLimitEnabledFlagAtIndex(size_t componentIndex, bool isLimitEnabled);

        [[nodiscard]] VE_INLINE bool GetConeLimitImpulse(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE bool GetConeLimitImpulseAtIndex(size_t componentIndex) const;
        VE_INLINE void SetConeLimitImpulse(Entity jointEntity, f32 impulse);
        VE_INLINE void SetConeLimitImpulseAtIndex(size_t componentIndex, f32 impulse);

        [[nodiscard]] VE_INLINE f32 GetConeLimitHalfAngle(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE f32 GetConeLimitHalfAngleAtIndex(size_t componentIndex) const;
        VE_INLINE void SetConeLimitHalfAngle(Entity jointEntity, f32 halfAngle);
        VE_INLINE void SetConeLimitHalfAngleAtIndex(size_t componentIndex, f32 halfAngle);

        [[nodiscard]] VE_INLINE bool GetInverseMassMatrixConeLimit(Entity jointEntity) const;
        [[nodiscard]] VE_INLINE bool GetInverseMassMatrixConeLimitAtIndex(size_t componentIndex) const;
        VE_INLINE void SetInverseMassMatrixConeLimit(Entity jointEntity, f32 inverseMassMatrix);
        VE_INLINE void SetInverseMassMatrixConeLimitAtIndex(size_t componentIndex, f32 inverseMassMatrix);

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        std::vector<BallAndSocketJoint *> _joints;
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyOne;
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyTwo;
        std::vector<glm::vec3> _r1WorldSpace;
        std::vector<glm::vec3> _r2WorldSpace;
        std::vector<glm::mat3> _bodyOneInertiaTensorsInWorldSpace;
        std::vector<glm::mat3> _bodyTwoInertiaTensorsInWorldSpace;
        std::vector<glm::vec3> _biasVectors;
        std::vector<glm::mat3> _inverseMassMatrices;
        std::vector<glm::vec3> _impulses;
        std::vector<u8> _coneLimitEnabledFlags;
        std::vector<f32> _coneLimitImpulses;
        std::vector<f32> _coneLimitHalfAngles;
        std::vector<f32> _inverseMassMatrixConeLimits;
        std::vector<f32> _coneLimitBiases;
        std::vector<u8> _coneLimitViolatedFlags;
        std::vector<glm::vec3> _coneLimitAxisOneCrossTwoProducts;
    };

} // namespace Vulkyrie
