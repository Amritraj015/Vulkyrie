#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings &settings)
        : _settings(settings)
        , _entityManager()
        , _bodyComponentStore()
        , _rigidBodyComponentStore()
        , _colliderComponentStore()
        , _transformComponentStore()
        , _collisionSystem(*this, _context.GetBoxShapeHalfEdgeMesh())
        , _dynamicsSystem(*this, _gravityEnabled, _settings.Gravity)
        , _sleepLinearVelocitySquared(_settings.DefaultSleepLinearVelocity * _settings.DefaultSleepLinearVelocity)
        , _sleepAngularVelocitySquared(_settings.DefaultSleepAngularVelocity * _settings.DefaultSleepAngularVelocity)
        , _gravityEnabled(true) {
    }

    PhysicsWorld::~PhysicsWorld() {

        // // Destroy all the joints that have not been removed
        // for (auto &joint : _jointComponentStore.GetJointComponents()) {
        //     destroyJoint(joint);
        // }

        size_t index = _rigidBodies.size();

        while (index != 0) {
            index--;
            DestroyRigidBody(*_rigidBodies[index]);
        }

        // VASSERT(_jointComponentStore.GetTotalComponentCount() == 0, "Joint Component Store must be empty.");
        VASSERT(_rigidBodies.size() == 0, "_rigidBodies size should be 0.");
        // VASSERT(mCollisionBodies.size() == 0, "");
        VASSERT(_bodyComponentStore.GetTotalComponentCount() == 0, "Body Component Store must be empty.");
        VASSERT(_transformComponentStore.GetTotalComponentCount() == 0, "Transform Component Store must be empty.");
        VASSERT(_colliderComponentStore.GetTotalComponentCount() == 0, "Collider Component Store must be empty.");
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

    void PhysicsWorld::SetSleepingEnabled(bool enabled) {
        VTRACE("PhysicsWorld: {} - Updating SleepingEnabled from {} to {}.", GetWorldName(), _settings.EnableSleeping, enabled);

        _settings.EnableSleeping = enabled;

        if (_settings.EnableSleeping) {
            for (auto *rigidBody : _rigidBodies) {
                rigidBody->SetIsSleeping(false);
            }
        }
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

    void PhysicsWorld::setJointStatus(Entity jointEntity, bool enabled) {
    }

    void PhysicsWorld::solveContactsAndConstraints(Timestep timeStep) {
    }

    void PhysicsWorld::solvePositionCorrection() {
        for (size_t i = 0; i < _settings.PositionSolverIterations; ++i) {
            _constraintSolverSystem.SolvePositionConstraints();
        }
    }

    void PhysicsWorld::createIslands() {
        // VASSERT(_processContactPairsOrderIslands.size() == 0, "_processContactPairsOrderIslands size must be 0.");
        //
        // const size_t totalRigidBodies = _rigidBodyComponentStore.GetTotalComponentCount();
        // for (size_t i = 0; i < totalRigidBodies; ++i) {
        //     _rigidBodyComponentStore.SetInIslandAtIndex(i, false);
        // }
    }

    void PhysicsWorld::updateSleepingBodies(Timestep timeStep) {
        for (size_t i = 0; i < _islands.GetTotalIslands(); ++i) {
            f32 minSleepTime = VE_DECIMAL_MAX;

            for (size_t b = 0; b < _islands.TotalBodiesInIsland[i]; ++i) {
                const Entity bodyEntity = _islands.BodyEntities[_islands.StartingBodyIndexForIsland[i] + b];
                const size_t bodyIndex = _rigidBodyComponentStore.GetEntityIndex(bodyEntity);

                if (_rigidBodyComponentStore.GetBodyTypeAtIndex(bodyIndex) == BodyType::Static) continue;

                const f32 linearVelocitySquared = glm::length2(_rigidBodyComponentStore.GetLinearVelocityAtIndex(bodyIndex));
                const f32 angularVelocitySquared = glm::length2(_rigidBodyComponentStore.GetAngularVelocityAtIndex(bodyIndex));
                const bool isAllowedToSleep = _rigidBodyComponentStore.CanSleepAtIndex(bodyIndex);

                if (linearVelocitySquared > _sleepLinearVelocitySquared || angularVelocitySquared > _sleepAngularVelocitySquared || isAllowedToSleep) {
                    _rigidBodyComponentStore.SetSleepTimeAtIndex(bodyIndex, f32(0.0));
                    minSleepTime = f32(0.0);
                } else {
                    const f32 newSleepTime = _rigidBodyComponentStore.GetSleepTimeAtIndex(bodyIndex) + timeStep.GetSeconds();
                    _rigidBodyComponentStore.SetSleepTimeAtIndex(bodyIndex, newSleepTime);

                    if (newSleepTime < minSleepTime) {
                        minSleepTime = newSleepTime;
                    }
                }
            }

            // If the velocity of all the bodies of the island is under the
            // sleeping velocity threshold for a period of time larger than
            // the time required to become a sleeping body
            if (minSleepTime >= _settings.TimeToSleep) {

                // Put all the bodies of the island to sleep
                for (size_t b = 0; b < _islands.TotalBodiesInIsland[i]; ++b) {

                    const Entity bodyEntity = _islands.BodyEntities[_islands.StartingBodyIndexForIsland[i] + b];
                    RigidBody &body = _rigidBodyComponentStore.GetRigidBody(bodyEntity);
                    body.SetIsSleeping(true);
                }
            }
        }
    }

    void PhysicsWorld::addJointToBodies(Entity bodyOne, Entity bodyTwo, Entity joint) {
        _rigidBodyComponentStore.AddJointToBody(bodyOne, joint);

        VTRACE("PhysicsWorld: {} - Adding Joint {} to Body {}.", GetWorldName(), joint.GetID(), bodyOne.GetID());

        _rigidBodyComponentStore.AddJointToBody(bodyTwo, joint);

        VTRACE("PhysicsWorld: {} - Adding Joint {} to Body {}.", GetWorldName(), joint.GetID(), bodyTwo.GetID());
    }

    void PhysicsWorld::updateBodiesInverseWorldInertiaTensors() {

        // const size_t componentCount = _rigidBodyComponentStore.GetActiveComponentCount();
        // for (size_t i = 0; i < componentCount; i++) {
        //     const Entity boydEntity = _rigidBodyComponentStore.GetEntityAtIndex(i);
        //
        //     const Matrix3x3 orientation = _transformComponentStore.GetTransform(boydEntity).Rotation.getMatrix();
        //
        //     RigidBody::ComputeWorldInertiaTensorInverse(
        //         orientation, mRigidBodyComponents.mInverseInertiaTensorsLocal[i], mRigidBodyComponents.mInverseInertiaTensorsWorld[i]);
        // }
    }

} // namespace Vulkyrie
