#pragma once

#include "physics/types/contact_pair.h"

namespace Vulkyrie {

    class OverlapCallback {
    public:
        OverlapCallback() = default;

        OverlapCallback(const OverlapCallback &) = delete;
        OverlapCallback &operator=(const OverlapCallback &) = delete;

        OverlapCallback(OverlapCallback &&) = delete;
        OverlapCallback &operator=(OverlapCallback &&) = delete;

        virtual ~OverlapCallback() = default;

        class Data final {
        public:
            Data(std::vector<ContactPair> &contactPairs, std::vector<ContactPair> &lostContactPairs, bool reportTriggersOnly);

            Data(const Data &) = delete;
            Data &operator=(const Data &) = delete;

            Data(Data &&) = delete;
            Data &operator=(Data &&) = delete;

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
