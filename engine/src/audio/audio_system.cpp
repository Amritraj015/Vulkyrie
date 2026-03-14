#include "audio/audio_system.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie::Audio {

    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;

    AudioSystem::AudioSystem() {
        // Initialize OpenAL
        device = alcOpenDevice(nullptr);
        assert(device && "Failed to open OpenAL device");

        context = alcCreateContext(device, nullptr);
        alcMakeContextCurrent(context);

        // Pre-create sources
        sources.resize(MAX_SOURCES);
        for (size_t i = 0; i < MAX_SOURCES; ++i) {
            alGenSources(1, &sources[i].Source);
            sources[i].Active = false;
            sources[i].Handle.ID = static_cast<u32>(i);
            sources[i].Handle.Valid = false;
            sources[i].Looping = false;
        }

        // Listener defaults
        alListener3f(AL_POSITION, 0, 0, 0);
        alListener3f(AL_VELOCITY, 0, 0, 0);
        f32 ori[] = { 0, 0, -1, 0, 1, 0 }; // forward, up
        alListenerfv(AL_ORIENTATION, ori);
    }

    AudioSystem::~AudioSystem() {
        for (auto &src : sources) {
            if (src.Source != 0) {
                alDeleteSources(1, &src.Source);
            }
        }

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    AudioClip *AudioSystem::LoadClip(const std::filesystem::path &filename) {
        if (clipCache.find(filename) != clipCache.end()) {
            return &clipCache[filename];
        }

        AudioClip clip;
        if (!LoadWAV(filename.c_str(), clip)) {
            VERROR("Failed to load WAV file: {}", filename.c_str());
            return nullptr;
        }

        clipCache[filename] = std::move(clip);
        return &clipCache[filename];
    }

    AudioHandle AudioSystem::PlaySound(AudioClip *clip, const glm::vec3 &position, bool loop) {
        // Find a free source
        for (auto &src : sources) {
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
        if (!handle.Valid || handle.ID >= sources.size()) return;
        sources[handle.ID].Stop();
    }

    void AudioSystem::SetListenerPosition(const glm::vec3 &pos) {
        alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    }

    void AudioSystem::SetListenerOrientation(const glm::vec3 &forward, const glm::vec3 &up) {
        f32 ori[6] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
        alListenerfv(AL_ORIENTATION, ori);
    }

    void AudioSystem::Update() {
        // Check which sources finished playing and mark them free
        for (auto &src : sources) {
            if (src.Active && !src.IsPlaying()) {
                src.Active = false;
                src.Handle.Valid = false;
            }
        }
    }

    bool AudioSystem::LoadWAV(std::filesystem::path filePath, AudioClip &clip) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) return false;

        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0) return false;

        file.ignore(4);
        char wave[4];
        file.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0) return false;

        char chunkId[4];
        i32 chunkSize;

        // find fmt
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char *>(&chunkSize), 4);
        if (std::strncmp(chunkId, "fmt ", 4) != 0) return false;

        short audioFormat, numChannels;
        i32 sampleRate;
        file.read(reinterpret_cast<char *>(&audioFormat), 2);
        file.read(reinterpret_cast<char *>(&numChannels), 2);
        file.read(reinterpret_cast<char *>(&sampleRate), 4);
        file.ignore(6); // byteRate+blockAlign
        short bitsPerSample;
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
