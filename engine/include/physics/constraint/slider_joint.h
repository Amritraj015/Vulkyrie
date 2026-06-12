#pragma once

#include "core/entity.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    struct SliderJointData final : public JointData {};

    class SliderJoint final : public Joint {
    public:
        SliderJoint(Entity entity, PhysicsWorld &world, const SliderJointData &data);

        VE_DELETE_MOVE_AND_COPY(SliderJoint);

        ~SliderJoint() override = default;

        glm::vec3 GetReactionForce(Timestep timestep) const override;
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
    };

} // namespace Vulkyrie
