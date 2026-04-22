#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    RigidBodyComponentStore::RigidBodyComponentStore() {
        _rigidBodies.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _canSleepFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _isSleepingFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _sleepTimes.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyTypes.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _linearVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _angularVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _externalForces.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _externalTorques.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _linearDampings.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _angularDampings.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _masses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMasses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localInertiaTensors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseLocalInertiaTensors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseWorldInertiaTensors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _constrainedLinearVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _constrainedAngularVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _splitLinearVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _splitAngularVelocities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _constrainedPositions.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _constrainedOrientations.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localCenterOfMasses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _worldCenterOfMasses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _gravityEnabledFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _isInIslandFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _joints.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _contactPairs.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _linearLockAxisFactors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _angularLockAxisFactors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void RigidBodyComponentStore::AddComponent(Entity entity, const RigidBodyComponent &component, bool active) {
        VASSERT(!HasComponent(entity), "Entity already has a ColliderComponent.");

        size_t index = _entities.size();

        // Append the new component and its associated entity to the end of their respective vectors.
        _rigidBodies.push_back(component.Body);
        _canSleepFlags.push_back(true);
        _isSleepingFlags.push_back(false);
        _sleepTimes.push_back(f32(0.0f));
        _bodyTypes.push_back(component.Type);
        _linearVelocities.push_back(glm::vec3(0.0f));
        _angularVelocities.push_back(glm::vec3(0.0f));
        _externalForces.push_back(glm::vec3(0.0f));
        _externalTorques.push_back(glm::vec3(0.0f));
        _linearDampings.push_back(f32(0.0f));
        _angularDampings.push_back(f32(0.0f));
        _masses.push_back(f32(1.0f));
        _inverseMasses.push_back(f32(1.0f));
        _localInertiaTensors.push_back(glm::vec3(1.0f));
        _inverseLocalInertiaTensors.push_back(glm::vec3(1.0f));
        _inverseWorldInertiaTensors.push_back(glm::identity<glm::mat3>());
        _constrainedLinearVelocities.push_back(glm::vec3(0.0f));
        _constrainedAngularVelocities.push_back(glm::vec3(0.0f));
        _splitLinearVelocities.push_back(glm::vec3(0.0f));
        _splitAngularVelocities.push_back(glm::vec3(0.0f));
        _constrainedPositions.push_back(glm::vec3(0.0f));
        _constrainedOrientations.push_back(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        _localCenterOfMasses.push_back(glm::vec3(0.0f));
        _worldCenterOfMasses.push_back(component.WorldPosition);
        _gravityEnabledFlags.push_back(true);
        _isInIslandFlags.push_back(false);
        _joints.emplace_back();
        _contactPairs.emplace_back();
        _linearLockAxisFactors.push_back(glm::vec3(1.0f));
        _angularLockAxisFactors.push_back(glm::vec3(1.0f));
        _entities.push_back(entity);

        if (active) {
            // Swap the new component into the active zone if inactive components sit between it and _activeCount.
            // swapComponents updates _entityToComponentIndex for both swapped entries;
            // otherwise we set the mapping ourselves since no swap is needed.
            if (index != _activeCount) {
                swapComponents(index, _activeCount);
            } else {
                _entityToComponentIndex[entity] = index;
            }

            // Grow the active zone to include the newly added component.
            _activeCount++;
        } else {
            // Inactive components stay at the end; just record the mapping.
            _entityToComponentIndex[entity] = index;
        }
    }

    void RigidBodyComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_rigidBodies[indexA], _rigidBodies[indexB]);
        std::swap(_canSleepFlags[indexA], _canSleepFlags[indexB]);
        std::swap(_isSleepingFlags[indexA], _isSleepingFlags[indexB]);
        std::swap(_sleepTimes[indexA], _sleepTimes[indexB]);
        std::swap(_bodyTypes[indexA], _bodyTypes[indexB]);
        std::swap(_linearVelocities[indexA], _linearVelocities[indexB]);
        std::swap(_angularVelocities[indexA], _angularVelocities[indexB]);
        std::swap(_externalForces[indexA], _externalForces[indexB]);
        std::swap(_externalTorques[indexA], _externalTorques[indexB]);
        std::swap(_linearDampings[indexA], _linearDampings[indexB]);
        std::swap(_angularDampings[indexA], _angularDampings[indexB]);
        std::swap(_masses[indexA], _masses[indexB]);
        std::swap(_inverseMasses[indexA], _inverseMasses[indexB]);
        std::swap(_localInertiaTensors[indexA], _localInertiaTensors[indexB]);
        std::swap(_inverseLocalInertiaTensors[indexA], _inverseLocalInertiaTensors[indexB]);
        std::swap(_inverseWorldInertiaTensors[indexA], _inverseWorldInertiaTensors[indexB]);
        std::swap(_constrainedLinearVelocities[indexA], _constrainedLinearVelocities[indexB]);
        std::swap(_constrainedAngularVelocities[indexA], _constrainedAngularVelocities[indexB]);
        std::swap(_splitLinearVelocities[indexA], _splitLinearVelocities[indexB]);
        std::swap(_splitAngularVelocities[indexA], _splitAngularVelocities[indexB]);
        std::swap(_constrainedPositions[indexA], _constrainedPositions[indexB]);
        std::swap(_constrainedOrientations[indexA], _constrainedOrientations[indexB]);
        std::swap(_localCenterOfMasses[indexA], _localCenterOfMasses[indexB]);
        std::swap(_worldCenterOfMasses[indexA], _worldCenterOfMasses[indexB]);
        std::swap(_gravityEnabledFlags[indexA], _gravityEnabledFlags[indexB]);
        std::swap(_isInIslandFlags[indexA], _isInIslandFlags[indexB]);
        std::swap(_joints[indexA], _joints[indexB]);
        std::swap(_contactPairs[indexA], _contactPairs[indexB]);
        std::swap(_linearLockAxisFactors[indexA], _linearLockAxisFactors[indexB]);
        std::swap(_angularLockAxisFactors[indexA], _angularLockAxisFactors[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void RigidBodyComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no ColliderComponent available to be removed.");

        _rigidBodies.pop_back();
        _canSleepFlags.pop_back();
        _isSleepingFlags.pop_back();
        _sleepTimes.pop_back();
        _bodyTypes.pop_back();
        _linearVelocities.pop_back();
        _angularVelocities.pop_back();
        _externalForces.pop_back();
        _externalTorques.pop_back();
        _linearDampings.pop_back();
        _angularDampings.pop_back();
        _masses.pop_back();
        _inverseMasses.pop_back();
        _localInertiaTensors.pop_back();
        _inverseLocalInertiaTensors.pop_back();
        _inverseWorldInertiaTensors.pop_back();
        _constrainedLinearVelocities.pop_back();
        _constrainedAngularVelocities.pop_back();
        _splitLinearVelocities.pop_back();
        _splitAngularVelocities.pop_back();
        _constrainedPositions.pop_back();
        _constrainedOrientations.pop_back();
        _localCenterOfMasses.pop_back();
        _worldCenterOfMasses.pop_back();
        _gravityEnabledFlags.pop_back();
        _isInIslandFlags.pop_back();
        _joints.pop_back();
        _contactPairs.pop_back();
        _linearLockAxisFactors.pop_back();
        _angularLockAxisFactors.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
