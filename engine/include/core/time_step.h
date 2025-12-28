#pragma once

#include "defines.h"

namespace Vulkyrie::Core {
    class Timestep {
        public:
            explicit Timestep(const float time = 0.0f) : _time(time) {
            }

            explicit operator float() const {
                return _time;
            }

            [[nodiscard]] f32 GetSeconds() const {
                return _time;
            }

            [[nodiscard]] f32 GetMilliseconds() const {
                return _time * 1000.0f;
            }

        private:
            float _time;
    };
}