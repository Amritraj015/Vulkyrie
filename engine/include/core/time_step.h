#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief Represents a duration of time in seconds and provides utility methods for time conversion. */
    struct Timestep {
    public:
        /** @brief Constructs a Timestep object representing a duration of time.
         * @param seconds The time duration in seconds. Default is 0.0f.
         */
        constexpr explicit Timestep(f32 seconds = 0.0f)
            : _seconds(seconds) {
        }

        /** @brief Gets the time duration in seconds.
         * @returns The time duration in seconds.
         */
        [[nodiscard]] constexpr VE_INLINE f32 GetSeconds() const {
            return _seconds;
        }

        /** @brief Gets the time duration in milliseconds.
         * @returns The time duration in milliseconds.
         */
        [[nodiscard]] constexpr VE_INLINE f32 GetMilliseconds() const {
            return _seconds * 1000.0f;
        }

        /** @brief Orders two timesteps by duration. Yields `std::partial_ordering` because the
         * underlying duration is floating point, so a NaN duration compares as unordered against
         * everything (including itself) rather than silently ordering.
         * @param other The timestep to compare this one against.
         * @returns The three-way comparison result of the two durations.
         */
        [[nodiscard]] constexpr auto operator<=>(const Timestep &other) const = default;

        /** @brief Compares two timesteps for equal duration.
         * @param other The timestep to compare this one against.
         * @returns True if both timesteps represent the same duration.
         */
        [[nodiscard]] constexpr bool operator==(const Timestep &other) const = default;

    private:
        /** @brief The time duration in seconds. */
        f32 _seconds;
    };

} // namespace Vulkyrie
