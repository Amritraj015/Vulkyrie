#pragma once

#include "physics/constraint/contact_point.h"
#include "physics/types/contact_pair.h"
#include "physics/types/contact_manifold.h"

namespace Vulkyrie {

    class CollisionCallback {
    public:
        CollisionCallback() = default;

        CollisionCallback(const CollisionCallback &) = delete;
        CollisionCallback &operator=(const CollisionCallback &) = delete;

        CollisionCallback(CollisionCallback &&) = delete;
        CollisionCallback &operator=(CollisionCallback &&) = delete;

        virtual ~CollisionCallback() = default;

        class Data final {
        public:
            Data(std::vector<ContactPair> &contactPairs,
                 std::vector<ContactManifold> &contactManifolds,
                 std::vector<ContactPoint> &contactPoints,
                 std::vector<ContactPair> &lostContactPairs);

            Data(const Data &) = delete;
            Data &operator=(const Data &) = delete;

            Data(Data &&) = delete;
            Data &operator=(Data &&) = delete;

            ~Data() = default;

            [[nodiscard]] VE_INLINE size_t GetContactPairCount() const {
                return _contactPairIndices.size() + _lostContactPairIndices.size();
            }

        private:
            std::vector<ContactPair> &_contactPairs;
            std::vector<ContactManifold> &_contactManifolds;
            std::vector<ContactPoint> &_contactPoints;
            std::vector<ContactPair> &_lostContactPairs;
            std::vector<size_t> _contactPairIndices;
            std::vector<size_t> _lostContactPairIndices;
        };

        virtual void OnCollision(const Data &collisionData) = 0;
    };

} // namespace Vulkyrie
