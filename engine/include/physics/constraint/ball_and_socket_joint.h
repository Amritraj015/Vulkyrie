#pragma once

#include "physics/constraint/joint.h"

namespace Vulkyrie {

    class RigidBody;

    struct BallAndSocketJointData : public JointData {
        bool IsUsingLocalSpaceAnchors;
        glm::vec3 AnchorPointInWorldSpace;
        glm::vec3 AnchorPointInBodyOneLocalSpace;
        glm::vec3 AnchorPointInBodyTwoLocalSpace;

        BallAndSocketJointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, const glm::vec3 &anchorPointWorldSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::BallAndSocket)
            , IsUsingLocalSpaceAnchors(false)
            , AnchorPointInWorldSpace(anchorPointWorldSpace) {
        }

        BallAndSocketJointData(RigidBody *rigidBodyOne,
                               RigidBody *rigidBodyTwo,
                               const glm::vec3 &anchorPointInBodyOneLocalSpace,
                               const glm::vec3 &anchorPointInBodyTwoLocalSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::BallAndSocket)
            , IsUsingLocalSpaceAnchors(true)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace) {
        }
    };

    class BallAndSocketJoint final : public Joint {
    public:
        BallAndSocketJoint(Entity entity, PhysicsWorld &world, const BallAndSocketJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJoint);

        ~BallAndSocketJoint() override = default;

        void EnableOrDisableConeLimit(bool enable);
        bool ConeLimitEnabled() const;

        void SetConeLimitHalfAngle(f32 coneHalfAngle);
        f32 GetConeLimitHalfAngle() const;

        f32 GetConeHalfAngle() const;

        glm::vec3 GetReactionForce(Timestep timestep) const override;
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        static constexpr f32 BETA = f32(0.2);

        void resetLimits();
    };

} // namespace Vulkyrie
