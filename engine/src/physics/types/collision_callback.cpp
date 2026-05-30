#include "physics/types/collision_callback.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    CollisionCallback::ContactPair::ContactPair(const Vulkyrie::ContactPair &contactPair,
                                                std::vector<Vulkyrie::ContactPoint> &contactPoints,
                                                PhysicsWorld &world,
                                                bool isLostContactPair)
        : _contactPair(contactPair)
        , _contactPoints(contactPoints)
        , _physicsWorld(world)
        , _isLostContactPair(isLostContactPair) {
    }

    Body &CollisionCallback::ContactPair::GetBodyOne() const {
        return _physicsWorld.GetBodyComponentStore().GetBody(_contactPair.BodyOneEntity);
    }

    Body &CollisionCallback::ContactPair::GetBodyTwo() const {
        return _physicsWorld.GetBodyComponentStore().GetBody(_contactPair.BodyTwoEntity);
    }

    Collider &CollisionCallback::ContactPair::GetColliderOne() const {
        return _physicsWorld.GetColliderComponentStore().GetCollider(_contactPair.ColliderOneEntity);
    }

    Collider &CollisionCallback::ContactPair::GetColliderTwo() const {
        return _physicsWorld.GetColliderComponentStore().GetCollider(_contactPair.ColliderTwoEntity);
    }

    CollisionCallback::Data::Data(std::vector<Vulkyrie::ContactPair> &contactPairs,
                                  std::vector<Vulkyrie::ContactManifold> &contactManifolds,
                                  std::vector<Vulkyrie::ContactPoint> &contactPoints,
                                  std::vector<Vulkyrie::ContactPair> &lostContactPairs,
                                  PhysicsWorld &world)
        : _contactPairs(contactPairs)
        , _contactManifolds(contactManifolds)
        , _contactPoints(contactPoints)
        , _lostContactPairs(lostContactPairs)
        , _physicsWorld(world) {

        _contactPairIndices.reserve(_contactPairs.size());
        _lostContactPairIndices.reserve(_lostContactPairs.size());

        // Filter the contact pairs to only keep the contact events (not the overlap/trigger events)
        for (size_t i = 0; i < _contactPairs.size(); ++i) {
            if (!_contactPairs[i].IsTrigger) {
                _contactPairIndices.push_back(i);
            }
        }

        // Filter the lost contact pairs to only keep the contact events (not the overlap/trigger events)
        for (size_t i = 0; i < _lostContactPairs.size(); ++i) {
            if (!_lostContactPairs[i].IsTrigger) {
                _lostContactPairIndices.push_back(i);
            }
        }
    }

    CollisionCallback::ContactPair CollisionCallback::Data::GetContactPair(size_t index) const {
        VASSERT(index < GetContactPairCount(), "Contact pair index out of bounds in CollisionCallback::Data::GetContactPair.");

        if (index < _contactPairIndices.size()) {
            // Return the contact pair.
            return CollisionCallback::ContactPair((_contactPairs)[_contactPairIndices[index]], _contactPoints, _physicsWorld, false);
        }

        // Return the lost contact pair.
        const size_t lostContactPairIndex = index - _contactPairIndices.size();
        return CollisionCallback::ContactPair(_lostContactPairs[_lostContactPairIndices[lostContactPairIndex]], _contactPoints, _physicsWorld, true);
    }

} // namespace Vulkyrie
