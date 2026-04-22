#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    RigidBody::RigidBody(Entity entity, PhysicsWorld &physicsWorld)
        : Body(entity, physicsWorld) {};

}
