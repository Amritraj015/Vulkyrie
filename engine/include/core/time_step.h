#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    /** @brief Represents a duration of time in seconds and provides utility methods for time conversion. */
    struct Timestep {
        public:
            /** @brief Constructs a Timestep object representing a duration of time.
             * @param time The time duration in seconds. Default is 0.0f.
             */
            constexpr explicit Timestep(const f32 seconds = 0.0f)
                : _seconds(seconds) {
            }

            /** @brief Converts the Timestep to a float representing seconds.
             * @returns The time duration in seconds.
             */
            constexpr explicit operator f32() const {
                return _seconds;
            }

            /** @brief Gets the time duration in seconds.
             * @returns The time duration in seconds.
             */
            [[nodiscard]] constexpr VE_FORCE_INLINE f32 GetSeconds() const {
                return _seconds;
            }

            /** @brief Gets the time duration in milliseconds.
             * @returns The time duration in milliseconds.
             */
            [[nodiscard]] constexpr VE_FORCE_INLINE f32 GetMilliseconds() const {
                return _seconds * 1000.0f;
            }

        private:
            /** @brief The time duration in seconds. */
            const f32 _seconds;
    };
} // namespace Vulkyrie
