#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"

namespace Vulkyrie {

    PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings &settings)
        : _settings(settings)
        , _entityManager()
        , _bodyStore()
        , _rigidBodyStore()
        , _colliderStore()
        , _transformStore()
        , _jointStore()
        , _collisionSystem(*this, _context.GetBoxShapeHalfEdgeMesh())
        , _constraintSolverSystem(*this, _enableWarmStartup)
        , _dynamicsSystem(*this, _gravityEnabled, _settings.Gravity)
        , _contactSolverSystem(*this, _islands, _settings.RestitutionCoefficient)
        , _eventListener(nullptr)
        , _sleepLinearVelocitySquared(_settings.DefaultSleepLinearVelocity * _settings.DefaultSleepLinearVelocity)
        , _sleepAngularVelocitySquared(_settings.DefaultSleepAngularVelocity * _settings.DefaultSleepAngularVelocity)
        , _gravityEnabled(true)
        , _enableWarmStartup(true) {
    }

    PhysicsWorld::~PhysicsWorld() {
        // Destroy all the joints that have not been removed.
        for (size_t i = 0; _jointStore.GetTotalComponentCount(); ++i) {
            DestroyJoint(_jointStore.GetJointAtIndex(i));
        }

        size_t index = _rigidBodies.size();

        while (index != 0) {
            index--;
            DestroyRigidBody(*_rigidBodies[index]);
        }

        VASSERT(_jointStore.GetTotalComponentCount() == 0, "Joint Component Store must be empty.");
        VASSERT(_rigidBodies.size() == 0, "_rigidBodies size should be 0.");
        // VASSERT(mCollisionBodies.size() == 0, "");
        VASSERT(_bodyStore.GetTotalComponentCount() == 0, "Body Component Store must be empty.");
        VASSERT(_transformStore.GetTotalComponentCount() == 0, "Transform Component Store must be empty.");
        VASSERT(_colliderStore.GetTotalComponentCount() == 0, "Collider Component Store must be empty.");
    }

    void PhysicsWorld::Update(Timestep timestep) {
        (void)timestep;
    }

    RigidBody &PhysicsWorld::CreateRigidBody(const TransformComponent &transform) {
        Entity entity = _entityManager.CreateEntity();

        _transformStore.AddComponent(entity, transform, true);

        auto *rigidBody = new RigidBody(entity, *this);

        VASSERT(nullptr != rigidBody, "Could not create RigidBody.");

        BodyComponent bodyComponent(rigidBody);
        _bodyStore.AddComponent(entity, bodyComponent, true);

        RigidBodyComponent rigidBodyComponent(rigidBody, BodyType::Dynamic, transform.Position);
        _rigidBodyStore.AddComponent(entity, rigidBodyComponent, true);

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
        const std::vector<Entity> &jointEntities = _rigidBodyStore.GetJoints(entity);

        // TODO: Finish this.
        if (jointEntities.size() > 0) {
            // DestroyJoint(_jointComponentStore.GetJoint(jointEntities[0]))
        }

        _bodyStore.RemoveComponent(entity);
        _rigidBodyStore.RemoveComponent(entity);
        _transformStore.RemoveComponent(entity);
        _entityManager.DestroyEntity(entity);

        std::erase(_rigidBodies, &body);

        delete &body;
    }

    void PhysicsWorld::SetActiveStatusForBody(Entity entity, bool active) {
        const bool isCurrentlyActive = !_bodyStore.IsDisabled(entity);

        // If the body is already in the desired active state, we can skip changing it and return early.
        if (active == isCurrentlyActive) {
            return;
        }

        // Else, activate or deactivate the body from all component stores.
        _bodyStore.SetActiveStatus(entity, active);
        _transformStore.SetActiveStatus(entity, active);
        _rigidBodyStore.SetActiveStatus(entity, active);

        const std::vector<Entity> &colliderEntities = _bodyStore.GetColliders(entity);

        for (Entity colliderEntity : colliderEntities) {
            _colliderStore.SetActiveStatus(colliderEntity, active);
        }
    }

    void PhysicsWorld::setJointStatus(Entity jointEntity, bool enabled) {
        (void)jointEntity;
        (void)enabled;
    }

    void PhysicsWorld::solveContactsAndConstraints(Timestep timeStep) {
        (void)timeStep;
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

            for (size_t b = 0; b < _islands.TotalBodiesInIsland[i]; ++b) {
                const Entity bodyEntity = _islands.BodyEntities[_islands.StartingBodyIndexForIsland[i] + b];
                const size_t bodyIndex = _rigidBodyStore.GetEntityIndex(bodyEntity);

                if (_rigidBodyStore.GetBodyTypeAtIndex(bodyIndex) == BodyType::Static) continue;

                const f32 linearVelocitySquared = glm::length2(_rigidBodyStore.GetLinearVelocityAtIndex(bodyIndex));
                const f32 angularVelocitySquared = glm::length2(_rigidBodyStore.GetAngularVelocityAtIndex(bodyIndex));
                const bool isAllowedToSleep = _rigidBodyStore.CanSleepAtIndex(bodyIndex);

                if (linearVelocitySquared > _sleepLinearVelocitySquared || angularVelocitySquared > _sleepAngularVelocitySquared || !isAllowedToSleep) {
                    _rigidBodyStore.SetSleepTimeAtIndex(bodyIndex, f32(0.0));
                    minSleepTime = f32(0.0);
                } else {
                    const f32 newSleepTime = _rigidBodyStore.GetSleepTimeAtIndex(bodyIndex) + timeStep.GetSeconds();
                    _rigidBodyStore.SetSleepTimeAtIndex(bodyIndex, newSleepTime);

                    if (newSleepTime < minSleepTime) {
                        minSleepTime = newSleepTime;
                    }
                }
            }

            if (minSleepTime >= _settings.TimeToSleep) {
                for (size_t b = 0; b < _islands.TotalBodiesInIsland[i]; ++b) {
                    const Entity bodyEntity = _islands.BodyEntities[_islands.StartingBodyIndexForIsland[i] + b];
                    RigidBody &body = _rigidBodyStore.GetRigidBody(bodyEntity);
                    body.SetIsSleeping(true);
                }
            }
        }
    }

    void PhysicsWorld::addJointToBodies(Entity bodyOne, Entity bodyTwo, Entity joint) {
        _rigidBodyStore.AddJointToBody(bodyOne, joint);

        VTRACE("PhysicsWorld: {} - Adding Joint {} to Body {}.", GetWorldName(), joint.GetID(), bodyOne.GetID());

        _rigidBodyStore.AddJointToBody(bodyTwo, joint);

        VTRACE("PhysicsWorld: {} - Adding Joint {} to Body {}.", GetWorldName(), joint.GetID(), bodyTwo.GetID());
    }

    void PhysicsWorld::updateBodiesInverseWorldInertiaTensors() {
        for (size_t i = 0; i < _rigidBodyStore.GetActiveComponentCount(); i++) {
            const Entity boydEntity = _rigidBodyStore.GetEntityAtIndex(i);
            const glm::mat3 orientation = glm::mat3_cast(_transformStore.GetTransform(boydEntity).Rotation);
            const glm::vec3 &localInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(i);
            glm::mat3 &worldInertiaTensor = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(i);

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(orientation, localInertiaTensor, worldInertiaTensor);
        }
    }

} // namespace Vulkyrie
