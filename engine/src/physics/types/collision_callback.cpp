#include "physics/types/collision_callback.h"

namespace Vulkyrie {

    CollisionCallback::Data::Data(std::vector<ContactPair> &contactPairs,
                                  std::vector<ContactManifold> &contactManifolds,
                                  std::vector<ContactPoint> &contactPoints,
                                  std::vector<ContactPair> &lostContactPairs)
        : _contactPairs(contactPairs)
        , _contactManifolds(contactManifolds)
        , _contactPoints(contactPoints)
        , _lostContactPairs(lostContactPairs) {

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
} // namespace Vulkyrie
