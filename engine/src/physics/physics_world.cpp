#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"

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

    RigidBody &PhysicsWorld::CreateRigidBody(const TransformComponent &transform) {
        Entity entity = _entityManager.CreateEntity();

        _transformComponentStore.AddComponent(entity, transform, true);

        auto *rigidBody = new RigidBody(entity, *this);

        VASSERT(nullptr != rigidBody, "Could not create RigidBody.");

        BodyComponent bodyComponent(rigidBody);
        _bodyComponentStore.AddComponent(entity, bodyComponent, true);

        RigidBodyComponent rigidBodyComponent(rigidBody, BodyType::Dynamic, transform.Position);
        _rigidBodyComponentStore.AddComponent(entity, rigidBodyComponent, true);

        _rigidBodies.push_back(rigidBody);

        return *rigidBody;
    }

    RigidBody &PhysicsWorld::GetRigidBody(size_t index) {
        VASSERT(index < _rigidBodies.size(), "Rigid Body index out of bounds");

        return *_rigidBodies[index];
    }

    const RigidBody &PhysicsWorld::GetRigidBody(size_t index) const {
        VASSERT(index < _rigidBodies.size(), "Rigid Body index out of bounds");

        return *_rigidBodies[index];
    }

    void PhysicsWorld::DestroyRigidBody(RigidBody &body) {
        body.RemoveAllColliders();

        const Entity entity = body.GetEntity();
        const std::vector<Entity> &jointEntities = _rigidBodyComponentStore.GetJoints(entity);

        // TODO: Finish this.
        if (jointEntities.size() > 0) {
            // DestroyJoint()
        }

        _bodyComponentStore.RemoveComponent(entity);
        _rigidBodyComponentStore.RemoveComponent(entity);
        _transformComponentStore.RemoveComponent(entity);
        _entityManager.DestroyEntity(entity);

        std::erase(_rigidBodies, &body);

        delete &body;
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
