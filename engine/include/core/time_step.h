#pragma once

#include "defines.h"

namespace Vulkyrie::Core {
    class Timestep {
        public:
            Timestep(float time = 0.0f) : m_Time(time) {
            }

            operator float() const {
                return m_Time;
            }

            f32 GetSeconds() const {
                return m_Time;
            }

            f32 GetMilliseconds() const {
                return m_Time * 1000.0f;
            }

        private:
            float m_Time;
    };
}