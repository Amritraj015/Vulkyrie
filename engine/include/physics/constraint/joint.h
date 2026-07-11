#pragma once

#include "vlkypch.h"
#include "core/entity.h"
#include "core/time_step.h"

namespace Vulkyrie {

    class RigidBody;
    class PhysicsWorld;

    /** @brief Discriminator identifying which concrete joint type an entity represents. */
    enum class JointType : i32 { Fixed, Hinge, BallAndSocket, Slider };

    /** @brief Technique used by the constraint solver to correct positional drift between the two constrained bodies. */
    enum class JointsPositionCorrectionTechnique : i32 { BaumgarteJoints, NonLinearGaussSeidel };

    /** @brief Plain-data initialisation struct passed to the world when creating a joint.
     *
     * Callers fill in the two body pointers, select an optional position correction technique, and choose
     * whether the two bodies are allowed to collide with each other. Concrete joint types derive their own
     * `*Data` struct from this one to carry additional joint-specific parameters. */
    struct JointData {
        /** @brief Non-owning pointer to the first rigid body involved in the joint. */
        RigidBody *BodyOne;

        /** @brief Non-owning pointer to the second rigid body involved in the joint. */
        RigidBody *BodyTwo;

        /** @brief Discriminator identifying the concrete joint type. */
        JointType Type;

        /** @brief Position correction technique used by the constraint solver. Defaults to NonLinearGaussSeidel. */
        JointsPositionCorrectionTechnique PositionCorrectionTechnique;

        /** @brief Whether collision detection between the two constrained bodies is enabled. Defaults to true. */
        bool CollisionEnabled;

        /** @brief Constructs a JointData for a joint type whose body pointers are assigned later.
         * @param jointType The type of joint to create. */
        JointData(JointType jointType)
            : BodyOne(nullptr)
            , BodyTwo(nullptr)
            , Type(jointType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NonLinearGaussSeidel)
            , CollisionEnabled(true) {
        }

        /** @brief Constructs a JointData with both body pointers set immediately.
         * @param rigidBodyOne Non-owning pointer to the first body.
         * @param rigidBodyTwo Non-owning pointer to the second body.
         * @param constraintType The type of joint to create. */
        JointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, JointType constraintType)
            : BodyOne(rigidBodyOne)
            , BodyTwo(rigidBodyTwo)
            , Type(constraintType)
            , PositionCorrectionTechnique(JointsPositionCorrectionTechnique::NonLinearGaussSeidel)
            , CollisionEnabled(true) {
        }
    };

    /** @brief Abstract base class for all joint constraints between two rigid bodies.
     *
     * A Joint binds two RigidBody instances together and enforces a set of positional and/or rotational
     * constraints during each simulation step. Joint state is stored externally in the JointComponentStore
     * owned by the PhysicsWorld; the Joint object itself is a lightweight handle that holds the entity ID
     * and a reference to the world for store access. Derived classes implement the specific constraint
     * logic and expose joint-specific parameters. */
    class Joint {
    public:
        /** @brief Constructs a Joint handle for the given entity in the given physics world.
         * @param entity The entity that identifies this joint in the component stores.
         * @param physicsWorld Reference to the owning PhysicsWorld. */
        Joint(Entity entity, PhysicsWorld &physicsWorld);

        VE_DELETE_MOVE_AND_COPY(Joint);

        /** @brief Virtual destructor for Joint. */
        virtual ~Joint() = default;

        /** @brief Returns the entity that identifies this joint in the component stores.
         * @returns The entity ID of the joint. */
        [[nodiscard]] VE_INLINE Entity GetEntity() const {
            return _entity;
        }

        /** @brief Returns a non-owning pointer to the first rigid body involved in this joint.
         * @returns Pointer to body one. */
        RigidBody *GetBodyOne() const;

        /** @brief Returns a non-owning pointer to the second rigid body involved in this joint.
         * @returns Pointer to body two. */
        RigidBody *GetBodyTwo() const;

        /** @brief Returns the concrete type of this joint.
         * @returns The JointType discriminator stored in the JointComponentStore. */
        JointType GetJointType() const;

        /** @brief Returns whether collision detection between the two constrained bodies is enabled.
         * @returns True if collision is enabled between the two bodies, false otherwise. */
        bool CollisionEnabled() const;

        /** @brief Returns the reaction force (in Newtons) on body two required to satisfy this constraint.
         * Computed as the accumulated constraint impulse divided by the timestep duration.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns The world-space reaction force vector in Newtons. */
        virtual glm::vec3 GetReactionForce(Timestep timestep) const = 0;

        /** @brief Returns the reaction torque (in Newton-metres) on body two required to satisfy this constraint.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns The world-space reaction torque vector in Newton-metres. */
        virtual glm::vec3 GetReactionTorque(Timestep timestep) const = 0;

    protected:
        /** @brief Entity ID used to look up this joint's data in the component stores. */
        Entity _entity;

        /** @brief Reference to the owning PhysicsWorld, used to access all component stores. */
        PhysicsWorld &_physicsWorld;

        /** @brief Wakes both constrained bodies so the solver processes them on the next step.
         * Should be called whenever joint parameters change mid-simulation (e.g. limit toggled,
         * half-angle updated) to prevent the bodies from remaining asleep with stale impulse state. */
        void awakeBodies() const;
    };

} // namespace Vulkyrie
