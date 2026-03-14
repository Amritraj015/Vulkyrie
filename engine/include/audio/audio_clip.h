#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Audio {

    class AudioClip final {
        public:
            u32 Buffer = 0;
            i32 Format = 0;
            i32 Frequency = 0;

            AudioClip() = default;
            ~AudioClip();

            AudioClip(AudioClip &&other) noexcept
                : Buffer(other.Buffer), Format(other.Format), Frequency(other.Frequency) {
                other.Buffer = 0;
            }

            AudioClip &operator=(AudioClip &&other) noexcept {
                if (this != &other) {
                    Buffer = other.Buffer;
                    Format = other.Format;
                    Frequency = other.Frequency;
                    other.Buffer = 0;
                }
                return *this;
            }

            AudioClip(const AudioClip &) = delete;
            AudioClip &operator=(const AudioClip &) = delete;
    };

} // namespace Vulkyrie::Audio
