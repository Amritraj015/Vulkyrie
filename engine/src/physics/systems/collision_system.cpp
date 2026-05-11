#include "physics/systems/collision_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    CollisionSystem::CollisionSystem(PhysicsWorld &physicsWorld)
        : _physicsWorld(physicsWorld)
        , _colliderComponentStore(_physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(_physicsWorld.GetRigidBodyComponentStore()) {
    }

} // namespace Vulkyrie
