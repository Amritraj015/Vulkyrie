#pragma once

#include "defines.h"

namespace Vulkyrie::Core {
    /** @brief Represents a duration of time in seconds and provides utility methods for time conversion. */
    class Timestep {
        public:
            /** @brief Constructs a Timestep object representing a duration of time.
             * @param time The time duration in seconds. Default is 0.0f.
             */
            explicit Timestep(const float time = 0.0f) : _time(time) {
            }

            /** @brief Converts the Timestep to a float representing seconds.
             * @returns The time duration in seconds.
             */
            explicit operator f32() const {
                return _time;
            }

            /** @brief Gets the time duration in seconds.
             * @returns The time duration in seconds.
             */
            [[nodiscard]] f32 GetSeconds() const {
                return _time;
            }

            /** @brief Gets the time duration in milliseconds.
             * @returns The time duration in milliseconds.
             */
            [[nodiscard]] f32 GetMilliseconds() const {
                return _time * 1000.0f;
            }

        private:
            /** @brief The time duration in seconds. */
            f32 _time;
    };
} // namespace Vulkyrie::Core
