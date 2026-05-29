#include "physics/types/overlap_callback.h"

namespace Vulkyrie {

    OverlapCallback::Data::Data(std::vector<ContactPair> &contactPairs, std::vector<ContactPair> &lostContactPairs, bool reportTriggersOnly)
        : _contactPairs(contactPairs)
        , _lostContactPairs(lostContactPairs) {

        _contactPairIndices.reserve(_contactPairs.size());
        _lostContactPairIndices.reserve(_lostContactPairs.size());

        // Filter the contact pairs to only keep the overlap/trigger events
        // if reportTriggersOnly is true, otherwise keep all contact pairs.
        for (size_t i = 0; i < _contactPairs.size(); ++i) {
            if (!reportTriggersOnly || _contactPairs[i].IsTrigger) {
                _contactPairIndices.push_back(i);
            }
        }

        // Filter the lost contact pairs to only keep the overlap/trigger events
        // if reportTriggersOnly is true, otherwise keep all lost contact pairs.
        for (size_t i = 0; i < _lostContactPairs.size(); ++i) {
            if (!reportTriggersOnly || _lostContactPairs[i].IsTrigger) {
                _lostContactPairIndices.push_back(i);
            }
        }
    }

} // namespace Vulkyrie
