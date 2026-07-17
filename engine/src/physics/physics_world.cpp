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
        , _contactSolverSystem(*this, _islands, _settings.RestitutionVelocityThreshold)
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
        VASSERT(_bodyStore.GetTotalComponentCount() == 0, "Body Component Store must be empty.");
        VASSERT(_transformStore.GetTotalComponentCount() == 0, "Transform Component Store must be empty.");
        VASSERT(_colliderStore.GetTotalComponentCount() == 0, "Collider Component Store must be empty.");
    }

    void PhysicsWorld::Update(Timestep timestep) {
        _collisionSystem.ComputeCollisions();

        createIslands();

        _collisionSystem.CreateContacts();

        _collisionSystem.ReportContactsAndTriggers();

        updateBodiesInverseWorldInertiaTensors();

        enableDisableJoints();

        _dynamicsSystem.IntegrateVelocities(timestep);

        solveContactsAndConstraints(timestep);

        _dynamicsSystem.IntegratePositions(timestep, _contactSolverSystem.IsSplitImpulseActive());

        solvePositionCorrection();

        _dynamicsSystem.UpdateStates();

        _collisionSystem.UpdateColliders();

        if (_settings.EnableSleeping) {
            updateSleepingBodies(timestep);
        }

        _dynamicsSystem.ResetForcesAndTorques();

        _islands.Clear();

        _processContactPairsOrderIslands.clear();
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

        if (jointEntities.size() > 0) {
            DestroyJoint(_jointStore.GetJoint(jointEntities[0]));
        }

        _bodyStore.RemoveComponent(entity);
        _rigidBodyStore.RemoveComponent(entity);
        _transformStore.RemoveComponent(entity);
        _entityManager.DestroyEntity(entity);

        std::erase(_rigidBodies, &body);

        delete &body;
    }

    Joint &PhysicsWorld::CreateJoint(const JointData &jointInfo) {
        const Entity entity = _entityManager.CreateEntity();
        const bool jointEnabled =
            _rigidBodyStore.EntityEnabled(jointInfo.BodyOne->GetEntity()) && _rigidBodyStore.EntityEnabled(jointInfo.BodyTwo->GetEntity());
        Joint *newJoint = nullptr;

        switch (jointInfo.Type) {
            case JointType::BallAndSocket: {
                BallAndSocketJointComponent ballAndSocketJointComponent(false, std::numbers::pi_v<f32>);
                _basJointStore.AddComponent(entity, ballAndSocketJointComponent, jointEnabled);

                const auto &data = static_cast<const BallAndSocketJointData &>(jointInfo);
                auto *joint = new BallAndSocketJoint(entity, *this, data);

                newJoint = joint;
                _basJointStore.SetJoint(entity, joint);

                break;
            }

            case JointType::Slider: {
                const auto &data = static_cast<const SliderJointData &>(jointInfo);

                SliderJointComponent sliderJointComponent(
                    data.LimitEnabled, data.MotorEnabled, data.MinTranslationLimit, data.MaxTranslationLimit, data.MotorSpeed, data.MaxMotorForce);
                _sliderJointStore.AddComponent(entity, sliderJointComponent, jointEnabled);

                auto *joint = new SliderJoint(entity, *this, data);

                newJoint = joint;
                _sliderJointStore.SetJoint(entity, joint);

                break;
            }

            case JointType::Hinge: {
                const auto &data = static_cast<const HingeJointData &>(jointInfo);

                HingeJointComponent hingeJointComponent(
                    data.LimitEnabled, data.MotorEnabled, data.MinAngleLimit, data.MaxAngleLimit, data.MotorSpeed, data.MaxMotorTorque);
                _hingeJointStore.AddComponent(entity, hingeJointComponent, jointEnabled);

                auto *joint = new HingeJoint(entity, *this, data);

                newJoint = joint;
                _hingeJointStore.SetJoint(entity, joint);

                break;
            }

            case JointType::Fixed: {
                _fixedJointStore.AddComponent(entity, jointEnabled);

                const auto &data = static_cast<const FixedJointData &>(jointInfo);
                auto *joint = new FixedJoint(entity, *this, data);

                newJoint = joint;
                _fixedJointStore.SetJoint(entity, joint);

                break;
            }
        }

        JointComponent jointComponent(jointInfo.BodyOne->GetEntity(),
                                      jointInfo.BodyTwo->GetEntity(),
                                      newJoint,
                                      jointInfo.Type,
                                      jointInfo.PositionCorrectionTechnique,
                                      jointInfo.CollisionEnabled);

        _jointStore.AddComponent(entity, jointComponent, jointEnabled);

        if (!jointInfo.CollisionEnabled) {
            _collisionSystem.AddNonCollidablePair(jointInfo.BodyOne->GetEntity(), jointInfo.BodyTwo->GetEntity());
        }

        VTRACE("Physics World: {}, New joint type {} created with ID: {}",
               _settings.Name,
               static_cast<i32>(newJoint->GetJointType()),
               newJoint->GetEntity().GetID());

        addJointToBodies(jointInfo.BodyOne->GetEntity(), jointInfo.BodyTwo->GetEntity(), entity);

        return *newJoint;
    }

    void PhysicsWorld::DestroyJoint(const Joint &joint) {
        RigidBody *bodyOne = joint.GetBodyOne();
        RigidBody *bodyTwo = joint.GetBodyTwo();

        const Entity bodyOneEntity = bodyOne->GetEntity();
        const Entity bodyTwoEntity = bodyTwo->GetEntity();
        const Entity jointEntity = joint.GetEntity();

        // If the collision between the two bodies of the constraint was disabled
        if (!joint.CollisionEnabled()) {

            // Remove the pair of bodies from the set of body pairs that cannot collide with each other
            _collisionSystem.RemoveNonCollidablePair(bodyOneEntity, bodyTwoEntity);
        }

        // Wake up the two bodies of the joint
        bodyOne->SetIsSleeping(false);
        bodyTwo->SetIsSleeping(false);

        // Remove the joint from the joint array of the bodies involved in the joint
        _rigidBodyStore.RemoveJointFromBody(bodyOneEntity, joint.GetEntity());
        _rigidBodyStore.RemoveJointFromBody(bodyTwoEntity, joint.GetEntity());

        // Destroy the corresponding entity and its components
        _jointStore.RemoveComponent(jointEntity);

        if (_basJointStore.HasComponent(jointEntity)) {
            _basJointStore.RemoveComponent(jointEntity);
        }

        if (_fixedJointStore.HasComponent(jointEntity)) {
            _fixedJointStore.RemoveComponent(jointEntity);
        }

        if (_hingeJointStore.HasComponent(jointEntity)) {
            _hingeJointStore.RemoveComponent(jointEntity);
        }

        if (_sliderJointStore.HasComponent(jointEntity)) {
            _sliderJointStore.RemoveComponent(jointEntity);
        }

        _entityManager.DestroyEntity(jointEntity);

        // Call the destructor of the joint
        delete &joint;
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

        for (const Entity colliderEntity : colliderEntities) {
            _colliderStore.SetActiveStatus(colliderEntity, active);
        }
    }

    void PhysicsWorld::setJointStatus(Entity jointEntity, bool enabled) {
        if (enabled == _jointStore.IsDisabled(jointEntity)) {
            return;
        }

        _jointStore.SetActiveStatus(jointEntity, enabled);

        if (_basJointStore.HasComponent(jointEntity)) {
            _basJointStore.SetActiveStatus(jointEntity, enabled);
        }

        if (_fixedJointStore.HasComponent(jointEntity)) {
            _fixedJointStore.SetActiveStatus(jointEntity, enabled);
        }

        if (_hingeJointStore.HasComponent(jointEntity)) {
            _hingeJointStore.SetActiveStatus(jointEntity, enabled);
        }

        if (_sliderJointStore.HasComponent(jointEntity)) {
            _sliderJointStore.SetActiveStatus(jointEntity, enabled);
        }
    }

    void PhysicsWorld::enableDisableJoints() {
        const size_t totalComponents = _jointStore.GetTotalComponentCount();

        std::vector<Entity> jointsEntities;
        jointsEntities.reserve(totalComponents);

        for (size_t i = 0; i < totalComponents; i++) {
            jointsEntities.push_back(_jointStore.GetEntityAtIndex(i));
        }

        for (size_t i = 0; i < totalComponents; i++) {
            const size_t jointEntityIndex = _jointStore.GetEntityIndex(jointsEntities[i]);
            const Entity bodyOne = _jointStore.GetBodyOneEntityAtIndex(jointEntityIndex);
            const Entity bodyTwo = _jointStore.GetBodyTwoEntityAtIndex(jointEntityIndex);

            setJointStatus(jointsEntities[i], _bodyStore.IsBodyActive(bodyOne) && _bodyStore.IsBodyActive(bodyTwo));
        }
    }

    void PhysicsWorld::solveContactsAndConstraints(Timestep timestep) {
        // Initialize the contact solver system.
        _contactSolverSystem.Initialize(_collisionSystem.GetCurrentContactManifolds(), _collisionSystem.GetCurrentContactPoints());

        // Initialize the constraint solver system.
        _constraintSolverSystem.Initialize(timestep);

        for (u16 i = 0; i < _settings.VelocitySolverIterations; i++) {
            // Solve velocity constrains.
            _constraintSolverSystem.SolveVelocityConstraints(timestep);

            // Solve for contacts.
            _contactSolverSystem.Solve(timestep);
        }

        // Store impulses.
        _contactSolverSystem.StoreImpulses();

        // Reset the contact solver system.
        _contactSolverSystem.Reset();
    }

    void PhysicsWorld::solvePositionCorrection() {
        // Solve position constraints.
        for (u16 i = 0; i < _settings.PositionSolverIterations; ++i) {
            _constraintSolverSystem.SolvePositionConstraints();
        }
    }

    void PhysicsWorld::createIslands() {
        VASSERT(_processContactPairsOrderIslands.size() == 0, "_processContactPairsOrderIslands size must be 0.");

        const size_t totalRigidBodies = _rigidBodyStore.GetTotalComponentCount();
        for (size_t i = 0; i < totalRigidBodies; ++i) {
            _rigidBodyStore.SetInIslandAtIndex(i, false);
        }

        const size_t nbJointsComponents = _jointStore.GetTotalComponentCount();
        for (size_t i = 0; i < nbJointsComponents; i++) {
            _jointStore.SetJointInIslandFlagsAtIndex(i, false);
        }

        _islands.ReserveMemory();

        std::vector<Entity> bodyEntitiesToVisit;
        bodyEntitiesToVisit.reserve(_islands.GetMaxBodiesInIslandInLastFrame());

        std::vector<Entity> staticBodiesAddedToIsland;
        staticBodiesAddedToIsland.reserve(16);

        size_t totalManifolds = 0;

        for (size_t b = 0; b < _rigidBodyStore.GetActiveComponentCount(); ++b) {
            if (_rigidBodyStore.IsInIslandAtIndex(b)) {
                continue;
            }

            if (BodyType::Static == _rigidBodyStore.GetBodyTypeAtIndex(b)) {
                continue;
            }

            bodyEntitiesToVisit.clear();

            _rigidBodyStore.SetInIslandAtIndex(b, true);
            bodyEntitiesToVisit.push_back(_rigidBodyStore.GetEntityAtIndex(b));

            const size_t islandIndex = _islands.AddIsland(totalManifolds);

            while (bodyEntitiesToVisit.size() > 0) {
                const Entity bodyToVisitEntity = bodyEntitiesToVisit.back();
                bodyEntitiesToVisit.pop_back();

                _islands.AddBodyToIsland(bodyToVisitEntity);

                RigidBody &rigidBodyToVisit = _rigidBodyStore.GetRigidBody(bodyToVisitEntity);

                rigidBodyToVisit.SetIsSleeping(false);

                const size_t bodyToVisitIndex = _rigidBodyStore.GetEntityIndex(bodyToVisitEntity);

                if (BodyType::Static == _rigidBodyStore.GetBodyTypeAtIndex(bodyToVisitIndex)) {
                    staticBodiesAddedToIsland.push_back(bodyToVisitEntity);

                    continue;
                }

                for (const u32 contactPairIndex : _rigidBodyStore.GetContactPairsAtIndex(bodyToVisitIndex)) {
                    ContactPair &pair = _collisionSystem.GetCurrentContactPairAtIndex(contactPairIndex);

                    if (pair.IsAlreadyInIsland) {
                        continue;
                    }

                    pair.IsAlreadyInIsland = true;

                    const bool isCollider1SimulationCollider = _colliderStore.IsSimulationCollider(pair.ColliderOneEntity);
                    const bool isCollider2SimulationCollider = _colliderStore.IsSimulationCollider(pair.ColliderTwoEntity);

                    if (!isCollider1SimulationCollider || !isCollider2SimulationCollider) {
                        continue;
                    }

                    const Entity otherBodyEntity = pair.BodyOneEntity == bodyToVisitEntity ? pair.BodyTwoEntity : pair.BodyOneEntity;

                    VASSERT(_colliderStore.IsSimulationCollider(pair.ColliderOneEntity), "Collider entity 1 must be a simulation collider.");
                    VASSERT(_colliderStore.IsSimulationCollider(pair.ColliderTwoEntity), "Collider entity 2 must be a simulation collider.");
                    VASSERT(!_colliderStore.IsTrigger(pair.ColliderOneEntity), "Collider entity 1 must not be a trigger.");
                    VASSERT(!_colliderStore.IsTrigger(pair.ColliderTwoEntity), "Collider entity 2 must not be a trigger.");

                    const size_t otherBodyIndex = _rigidBodyStore.GetEntityIndex(otherBodyEntity);

                    if (_bodyStore.HasSimulationColliders(otherBodyEntity)) {
                        _processContactPairsOrderIslands.push_back(contactPairIndex);

                        VASSERT(pair.PotentialContactManifoldsCount > 0, "Total potential manifold count must be greater than 0.");

                        totalManifolds += pair.PotentialContactManifoldsCount;

                        _islands.TotalContactManifolds[islandIndex] += pair.PotentialContactManifoldsCount;

                        if (_rigidBodyStore.IsInIslandAtIndex(otherBodyIndex)) {
                            continue;
                        }

                        bodyEntitiesToVisit.push_back(otherBodyEntity);

                        _rigidBodyStore.SetInIslandAtIndex(otherBodyIndex, true);
                    }
                }

                // For each joint in which the current body is involved
                const std::vector<Entity> &joints = _rigidBodyStore.GetJoints(rigidBodyToVisit.GetEntity());

                for (size_t i = 0; i < joints.size(); i++) {

                    const size_t jointComponentIndex = _jointStore.GetEntityIndex(joints[i]);

                    // Check if the current joint has already been added into an island
                    if (_jointStore.IsEntityInIslandAtIndex(jointComponentIndex)) {
                        continue;
                    }

                    // Add the joint into the island
                    _jointStore.SetJointInIslandFlagsAtIndex(jointComponentIndex, true);

                    const Entity body1Entity = _jointStore.GetBodyOneEntityAtIndex(jointComponentIndex);
                    const Entity body2Entity = _jointStore.GetBodyTwoEntityAtIndex(jointComponentIndex);
                    const Entity otherBodyEntity = body1Entity == bodyToVisitEntity ? body2Entity : body1Entity;

                    const size_t otherBodyIndex = _rigidBodyStore.GetEntityIndex(otherBodyEntity);

                    // Check if the other body has already been added to the island
                    if (_rigidBodyStore.IsInIslandAtIndex(otherBodyIndex)) {
                        continue;
                    }

                    // Insert the other body into the stack of bodies to visit
                    bodyEntitiesToVisit.push_back(otherBodyEntity);

                    _rigidBodyStore.SetInIslandAtIndex(otherBodyIndex, true);
                }
            }

            for (const Entity entity : staticBodiesAddedToIsland) {
                VASSERT(BodyType::Static == _rigidBodyStore.GetBodyType(entity), "Body type must be static.");

                _rigidBodyStore.SetInIsland(entity, false);
            }

            staticBodiesAddedToIsland.clear();
        }

        const std::vector<ContactPair> *contactPairs = _collisionSystem.GetCurrentContactPairs();

        if (nullptr != contactPairs) {
            for (const ContactPair &pair : *contactPairs) {
                _rigidBodyStore.RemoveAllContactPairs(pair.BodyOneEntity);
                _rigidBodyStore.RemoveAllContactPairs(pair.BodyTwoEntity);
            }
        }
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
