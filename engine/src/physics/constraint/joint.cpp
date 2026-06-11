#include "physics/constraint/joint.h"

namespace Vulkyrie {

    Joint::Joint(Entity entity, PhysicsWorld &world)
        : _entity(entity)
        , _physicsWorld(world) {
    }

    RigidBody *Joint::GetBodyOne() const {
        const Entity bodyOneEntity = _physicsWorld.GetJointComponentStore().GetBodyOneEntity(_entity);
        return &_physicsWorld.GetRigidBodyComponentStore().GetRigidBody(bodyOneEntity);
    }

    RigidBody *Joint::GetBodyTwo() const {
        const Entity bodyTwoEntity = _physicsWorld.GetJointComponentStore().GetBodyTwoEntity(_entity);
        return &_physicsWorld.GetRigidBodyComponentStore().GetRigidBody(bodyTwoEntity);
    }

    JointType Joint::GetJointType() const {
        return _physicsWorld.GetJointComponentStore().GetJointTypes(_entity);
    }

    bool Joint::CollisionEnabled() const {
        return _physicsWorld.GetJointComponentStore().IsCollisionEnabledForEntity(_entity);
    }

    void Joint::awakeBodies() const {
        const Entity bodyOneEntity = _physicsWorld.GetJointComponentStore().GetBodyOneEntity(_entity);
        const Entity bodyTwoEntity = _physicsWorld.GetJointComponentStore().GetBodyTwoEntity(_entity);

        // Get references to both bodies.
        RigidBody &rigidBodyOne = _physicsWorld.GetRigidBodyComponentStore().GetRigidBody(bodyOneEntity);
        RigidBody &rigidBodyTwo = _physicsWorld.GetRigidBodyComponentStore().GetRigidBody(bodyTwoEntity);

        // Wake up both bodies.
        rigidBodyOne.SetIsSleeping(false);
        rigidBodyTwo.SetIsSleeping(false);
    }

} // namespace Vulkyrie
