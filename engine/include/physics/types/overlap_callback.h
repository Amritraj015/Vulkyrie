#pragma once

#include "vlkypch.h"
#include "physics/types/contact_pair.h"

namespace Vulkyrie {

    class OverlapCallback {
    public:
        OverlapCallback() = default;

        VE_DELETE_MOVE_AND_COPY(OverlapCallback);

        virtual ~OverlapCallback() = default;

        class Data final {
        public:
            Data(std::vector<ContactPair> &contactPairs, std::vector<ContactPair> &lostContactPairs, bool reportTriggersOnly);

            VE_DELETE_MOVE_AND_COPY(Data);

            ~Data() = default;

        private:
            std::vector<ContactPair> &_contactPairs;
            std::vector<ContactPair> &_lostContactPairs;
            std::vector<size_t> _contactPairIndices;
            std::vector<size_t> _lostContactPairIndices;
        };

        virtual void OnOverlap(const Data &overlapData) = 0;
    };

} // namespace Vulkyrie
