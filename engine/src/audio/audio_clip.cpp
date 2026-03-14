#include "audio/audio_clip.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie::Audio {

    AudioClip::~AudioClip() {
        if (Buffer != 0) {
            alDeleteBuffers(1, &Buffer);
        }
    }

} // namespace Vulkyrie::Audio
