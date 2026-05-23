#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    class UUID final {
    public:
        UUID();
        UUID(u64 uuid);
        UUID(const UUID &) = default;

        [[nodiscard]] VE_INLINE u64 GetUUID() const {
            return _uuid;
        }

        bool operator==(const UUID &other) const {
            return _uuid == other._uuid;
        }

    private:
        u64 _uuid = 0;
    };

} // namespace Vulkyrie
