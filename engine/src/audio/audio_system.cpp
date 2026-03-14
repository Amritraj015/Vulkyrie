#include "audio/audio_system.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie::Audio {

    AudioSystem::AudioSystem() {
        // Initialize OpenAL
        _device = alcOpenDevice(nullptr);
        assert(_device && "Failed to open OpenAL device");

        _context = alcCreateContext(static_cast<ALCdevice *>(_device), nullptr);
        alcMakeContextCurrent(static_cast<ALCcontext *>(_context));

        // Pre-create sources
        _sources.resize(MAX_SOURCES);
        for (size_t i = 0; i < MAX_SOURCES; ++i) {
            alGenSources(1, &_sources[i].Source);
            _sources[i].Active = false;
            _sources[i].Handle.ID = static_cast<u32>(i);
            _sources[i].Handle.Valid = false;
            _sources[i].Looping = false;
        }

        // Listener defaults
        alListener3f(AL_POSITION, 0, 0, 0);
        alListener3f(AL_VELOCITY, 0, 0, 0);
        f32 ori[] = { 0, 0, -1, 0, 1, 0 }; // forward, up
        alListenerfv(AL_ORIENTATION, ori);
    }

    AudioSystem::~AudioSystem() {
        // Clean up sources
        for (auto &src : _sources) {
            if (src.Source != 0) {
                alDeleteSources(1, &src.Source);
            }
        }

        // Clean up OpenAL
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(static_cast<ALCcontext *>(_context));
        alcCloseDevice(static_cast<ALCdevice *>(_device));
    }

    AudioClip *AudioSystem::LoadClip(const std::filesystem::path &filepath) {
        if (auto it = _clipCache.find(filepath); it != _clipCache.end()) {
            return &it->second;
        }

        AudioClip clip;

        if (!LoadWAV(filepath, clip)) {
            VERROR("Failed to load WAV file: {}", filepath.c_str());
            return nullptr;
        }

        _clipCache[filepath] = std::move(clip);

        return &_clipCache[filepath];
    }

    AudioHandle AudioSystem::PlaySound(AudioClip *clip, const glm::vec3 &position, bool loop) {
        if (!clip || clip->Buffer == 0) {
            return AudioHandle{ 0, false };
        }

        // Find a free source
        for (auto &src : _sources) {
            if (!src.Active) {
                src.Active = true;
                src.Looping = loop;
                src.SetPosition(position);
                src.SetVolume(1.0f);
                src.Handle.Valid = true;

                alSourcei(src.Source, AL_BUFFER, clip->Buffer);
                alSourcei(src.Source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
                alSourcePlay(src.Source);

                return src.Handle;
            }
        }

        // No free source
        return AudioHandle{ 0, false };
    }

    void AudioSystem::Stop(AudioHandle handle) {
        if (!handle.Valid || handle.ID >= _sources.size()) return;
        _sources[handle.ID].Stop();
    }

    void AudioSystem::SetListenerPosition(const glm::vec3 &position) {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
    }

    void AudioSystem::SetListenerOrientation(const glm::vec3 &forward, const glm::vec3 &up) {
        f32 ori[6] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
        alListenerfv(AL_ORIENTATION, ori);
    }

    void AudioSystem::Update() {
        // Check which sources finished playing and mark them free
        for (auto &src : _sources) {
            if (src.Active && !src.IsPlaying()) {
                src.Active = false;
                src.Handle.Valid = false;
            }
        }
    }

    bool AudioSystem::LoadWAV(const std::filesystem::path &filePath, AudioClip &clip) {
        std::ifstream file(filePath, std::ios::binary);

        if (!file) {
            return false;
        }

        char riff[4];
        file.read(riff, 4);

        if (std::strncmp(riff, "RIFF", 4) != 0) {
            return false;
        }

        file.ignore(4);
        char wave[4];
        file.read(wave, 4);

        if (std::strncmp(wave, "WAVE", 4) != 0) {
            return false;
        }

        char chunkId[4];
        i32 chunkSize;

        // find fmt
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char *>(&chunkSize), 4);

        if (std::strncmp(chunkId, "fmt ", 4) != 0) {
            return false;
        }

        i16 audioFormat, numChannels;
        i32 sampleRate;
        file.read(reinterpret_cast<char *>(&audioFormat), 2);
        file.read(reinterpret_cast<char *>(&numChannels), 2);
        file.read(reinterpret_cast<char *>(&sampleRate), 4);
        file.ignore(6); // byteRate+blockAlign
        i16 bitsPerSample;
        file.read(reinterpret_cast<char *>(&bitsPerSample), 2);

        // skip to data
        while (true) {
            if (!file.read(chunkId, 4)) return false;
            if (!file.read(reinterpret_cast<char *>(&chunkSize), 4)) return false;
            if (std::strncmp(chunkId, "data", 4) == 0) break;

            file.seekg(chunkSize, std::ios::cur);
        }

        std::vector<char> bufferData(chunkSize);
        file.read(bufferData.data(), chunkSize);

        if (numChannels == 1 && bitsPerSample == 16) {
            clip.Format = AL_FORMAT_MONO16;
        } else if (numChannels == 2 && bitsPerSample == 16) {
            clip.Format = AL_FORMAT_STEREO16;
        } else {
            return false;
        }

        clip.Frequency = sampleRate;
        alGenBuffers(1, &clip.Buffer);
        alBufferData(clip.Buffer, clip.Format, bufferData.data(), static_cast<ALsizei>(bufferData.size()), clip.Frequency);

        return true;
    }

} // namespace Vulkyrie::Audio
