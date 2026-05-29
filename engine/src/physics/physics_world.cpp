#include "physics/physics_world.h"

namespace Vulkyrie {

    PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings &settings)
        : _gravityEnabled(true)
        , _settings(settings)
        , _entityManager()
        , _bodyComponentStore()
        , _rigidBodyComponentStore()
        , _colliderComponentStore()
        , _transformComponentStore()
        , _collisionSystem(*this, _context.GetBoxShapeHalfEdgeMesh())
        , _dynamicsSystem(*this, _gravityEnabled, _settings.Gravity) {
    }

    void PhysicsWorld::Update() {
    }

    void PhysicsWorld::SetActiveStatusForBody(Entity entity, bool active) {
        const bool isCurrentlyActive = !_bodyComponentStore.IsDisabled(entity);

        // If the body is already in the desired active state, we can skip changing it and return early.
        if (active == isCurrentlyActive) {
            return;
        }

        // Else, activate or deactivate the body from all component stores.
        _bodyComponentStore.SetActiveStatus(entity, active);
        _transformComponentStore.SetActiveStatus(entity, active);
        _rigidBodyComponentStore.SetActiveStatus(entity, active);

        const std::vector<Entity> &colliderEntities = _bodyComponentStore.GetColliders(entity);

        for (Entity colliderEntity : colliderEntities) {
            _colliderComponentStore.SetActiveStatus(colliderEntity, active);
        }
    }

} // namespace Vulkyrie
