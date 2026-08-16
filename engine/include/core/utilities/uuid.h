#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    class UUID final {
    public:
        /** @brief Creates a random UUID. */
        UUID();

        /** @brief Creates a copy from a provided UUID. */
        UUID(u64 uuid);

        UUID(const UUID &) = default;

        /** @brief Gets the stored UUID. */
        [[nodiscard]] VE_INLINE u64 GetUUID() const {
            return _uuid;
        }

        [[nodiscard]] VE_INLINE constexpr bool operator==(const UUID &other) const noexcept = default;

    private:
        u64 _uuid = 0;
    };

} // namespace Vulkyrie
