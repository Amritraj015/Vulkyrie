#include "audio/audio_clip.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie::Audio {

    AudioClip::AudioClip(AudioClip &&other) noexcept
        : Buffer(other.Buffer)
        , Format(other.Format)
        , Frequency(other.Frequency) {
        other.Buffer = 0;
    }

    AudioClip &AudioClip::operator=(AudioClip &&other) noexcept {
        if (this != &other) {
            if (Buffer != 0) {
                alDeleteBuffers(1, &Buffer);
            }

            Buffer = other.Buffer;
            Format = other.Format;
            Frequency = other.Frequency;
            other.Buffer = 0;
        }

        return *this;
    }

    AudioClip::~AudioClip() {
        if (Buffer != 0) {
            alDeleteBuffers(1, &Buffer);
        }
    }

} // namespace Vulkyrie::Audio
