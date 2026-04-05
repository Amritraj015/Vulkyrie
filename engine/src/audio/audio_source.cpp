#include "audio/audio_source.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie {

    void AudioSource::SetPosition(const glm::vec3 &position) {
        Position = position;
        alSource3f(Source, AL_POSITION, position.x, position.y, position.z);
    }

    void AudioSource::SetVolume(f32 gain) {
        alSourcef(Source, AL_GAIN, gain);
    }

    void AudioSource::Stop() {
        alSourceStop(Source);
        Active = false;
        Handle.Valid = false;
    }

    bool AudioSource::IsPlaying() {
        ALint state;
        alGetSourcei(Source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

} // namespace Vulkyrie
