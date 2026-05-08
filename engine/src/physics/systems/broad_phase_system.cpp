#include "physics/systems/broad_phase_system.h"

namespace Vulkyrie {

    BroadPhaseSystem::BroadPhaseSystem(ColliderComponentStore &colliderComponentStore,
                                       TransformComponentStore &transformComponentStore,
                                       RigidBodyComponentStore &rigidBodyComponentStore)
        : _aabbTree()
        , _colliderComponentStore(colliderComponentStore)
        , _transformComponentStore(transformComponentStore)
        , _rigidBodyComponentStore(rigidBodyComponentStore) {
    }

} // namespace Vulkyrie
