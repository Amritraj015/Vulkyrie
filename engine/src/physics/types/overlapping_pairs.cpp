#include "physics/types/overlapping_pairs.h"

namespace Vulkyrie {

    OverlappingPairs::OverlappingPairs(PhysicsWorld &physicsWorld, std::unordered_set<std::pair<Entity, Entity>> &pairsThatCannotCollide)
        : _bodyComponentStore(physicsWorld.GetBodyComponentStore())
        , _colliderComponentStore(physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(physicsWorld.GetRigidBodyComponentStore())
        , _pairsThatCannotCollide(pairsThatCannotCollide) {
    }

} // namespace Vulkyrie
