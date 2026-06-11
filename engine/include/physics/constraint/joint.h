#pragma once

#include "vlkypch.h"
#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    enum class JointType : i32 { Fixed, Hinge, BallAndSocket, Slider, Spring };
    enum class JointsPositionCorrectionTechnique : i32 { BaumgarteJoints, NonLinearGaussSeidel };

    struct JointData {
        RigidBody *BodyOne;
        RigidBody *BodyTwo;
        JointType Type;
        JointsPositionCorrectionTechnique PositionCorrectionTechnique;
        bool CollisionEnabled;

        JointData(JointType jointType)
            : BodyOne(nullptr)
            , BodyTwo(nullptr)
            , Type(jointType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NonLinearGaussSeidel)
            , CollisionEnabled(true) {
        }

        JointData(RigidBody *rigidBody1, RigidBody *rigidBody2, JointType constraintType)
            : BodyOne(rigidBody1)
            , BodyTwo(rigidBody2)
            , Type(constraintType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NonLinearGaussSeidel)
            , CollisionEnabled(true) {
        }
    };

    class Joint {
    public:
        Joint(Entity entity, PhysicsWorld &physicsWorld);

        VE_DELETE_MOVE_AND_COPY(Joint);

        virtual ~Joint() = default;

        [[nodiscard]] VE_INLINE Entity GetEntity() const {
            return _entity;
        }

        RigidBody *GetBodyOne() const;
        RigidBody *GetBodyTwo() const;
        JointType GetJointType() const;
        bool CollisionEnabled() const;

        virtual glm::vec3 GetReactionForce(Timestep timestep) const = 0;
        virtual glm::vec3 GetReactionTorque(Timestep timestep) const = 0;

    protected:
        Entity _entity;
        PhysicsWorld &_physicsWorld;

        void awakeBodies() const;
    };

} // namespace Vulkyrie
