#pragma once

#include "vlkypch.h"
#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    enum class JointType : i32 { Fixed, Hinge, BallAndSocket, Slider, Spring };
    enum class JointsPositionCorrectionTechnique : i32 { BAUMGARTE_JOINTS, NON_LINEAR_GAUSS_SEIDEL };

    struct JointInfo {
        RigidBody *BodyOne;
        RigidBody *BodyTwo;
        JointType Type;
        JointsPositionCorrectionTechnique PositionCorrectionTechnique;
        bool CollisionEnabled;

        JointInfo(JointType jointType)
            : BodyOne(nullptr)
            , BodyTwo(nullptr)
            , Type(jointType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NON_LINEAR_GAUSS_SEIDEL)
            , CollisionEnabled(true) {
        }

        JointInfo(RigidBody *rigidBody1, RigidBody *rigidBody2, JointType constraintType)
            : BodyOne(rigidBody1)
            , BodyTwo(rigidBody2)
            , Type(constraintType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NON_LINEAR_GAUSS_SEIDEL)
            , CollisionEnabled(true) {
        }
    };

    class Joint {
    public:
        Joint(Entity entity, const JointInfo &info, PhysicsWorld &physicsWorld);

        Joint(const Joint &) = delete;
        Joint &operator=(const Joint &) = delete;

        Joint(Joint &&) = delete;
        Joint &operator=(Joint &&) = delete;

        virtual ~Joint() = default;

        [[nodiscard]] VE_INLINE Entity GetEntity() const {
            return _entity;
        }

        RigidBody *GetBodyOne() const;
        RigidBody *GetBodyTwo() const;
        BodyType GetBodyOneType() const;
        bool CollisionEnabled() const;

        virtual glm::vec3 GetReactionForce(Timestep timestep) const = 0;
        virtual glm::vec3 GetReactionTorque(Timestep timestep) const = 0;

    protected:
        Entity _entity;
        PhysicsWorld &_physicsWorld;
    };

} // namespace Vulkyrie
