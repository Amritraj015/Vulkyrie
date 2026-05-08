#include "physics/physics_world.h"

namespace Vulkyrie {

    PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings &settings)
        : _settings(settings)
        , _entityManager()
        , _bodyComponentStore()
        , _rigidBodyComponentStore()
        , _colliderComponentStore()
        , _transformComponentStore()
        , _collisionSystem(_colliderComponentStore) {
    }

    void PhysicsWorld::Update() {
    }

} // namespace Vulkyrie
