#include "audio/audio_system.h"
#include "core/asserts.h"
#include <AL/al.h>
#include <AL/alc.h>

namespace Vulkyrie {

    AudioSystem::AudioSystem() {
        // Initialize OpenAL
        _device = alcOpenDevice(nullptr);
        VASSERT_EXPR(_device, "Failed to open OpenAL device.");

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
        // Check if the clip is already loaded and return it if found.
        if (auto it = _clipCache.find(filepath); it != _clipCache.end()) {
            return &it->second;
        }

        AudioClip clip;

        // Load the WAV file and create an OpenAL buffer for it.
        if (!LoadWAV(filepath, clip)) {
            VERROR("Failed to load WAV file: {}", filepath.c_str());
            return nullptr;
        }

        // Cache the loaded clip and return a pointer to it.
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
        VWARN("All {} audio sources are in use. Sound dropped.", MAX_SOURCES);
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
        // Try to open the file in binary mode.
        std::ifstream file(filePath, std::ios::binary);

        // If the file failed to open, return false.
        if (!file) {
            return false;
        }

        // Read the RIFF header.
        char riff[4];
        file.read(riff, 4);

        // Check if the file is a valid RIFF/WAVE file.
        if (std::strncmp(riff, "RIFF", 4) != 0) {
            return false;
        }

        // Skip the next 4 bytes (file size) and read the WAVE header.
        file.ignore(4);
        char wave[4];
        file.read(wave, 4);

        // Check if the file is a valid WAVE file.
        if (std::strncmp(wave, "WAVE", 4) != 0) {
            return false;
        }

        // Read chunks until we find the "fmt " chunk and then the "data" chunk.
        char chunkId[4];
        i32 chunkSize;

        // find fmt
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char *>(&chunkSize), 4);

        // Check if the chunk is the "fmt " chunk.
        if (std::strncmp(chunkId, "fmt ", 4) != 0) {
            return false;
        }

        // Read the audio format, number of chanels and sample rate information from the "fmt " chunk.
        i16 audioFormat, numChannels;
        i32 sampleRate;
        file.read(reinterpret_cast<char *>(&audioFormat), 2);
        file.read(reinterpret_cast<char *>(&numChannels), 2);
        file.read(reinterpret_cast<char *>(&sampleRate), 4);
        file.ignore(6); // byteRate+blockAlign

        // Read bits per sample.
        i16 bitsPerSample;
        file.read(reinterpret_cast<char *>(&bitsPerSample), 2);

        // Skip to the "data" chunk, which contains the audio samples.
        while (true) {
            // If we fail to read the chunk header, return false.
            if (!file.read(chunkId, 4)) return false;

            // If we fail to read the chunk size, return false.
            if (!file.read(reinterpret_cast<char *>(&chunkSize), 4)) return false;

            // If this is the "data" chunk, break out of the loop to read the audio data.
            if (std::strncmp(chunkId, "data", 4) == 0) break;

            // Otherwise, skip this chunk and continue searching for the "data" chunk.
            file.seekg(chunkSize, std::ios::cur);
        }

        // Read the audio data from the "data" chunk into a buffer.
        std::vector<char> bufferData(chunkSize);
        file.read(bufferData.data(), chunkSize);

        // Determine the OpenAL format based on the number of channels and bits per sample.
        if (numChannels == 1 && bitsPerSample == 16) {
            clip.Format = AL_FORMAT_MONO16;
        } else if (numChannels == 2 && bitsPerSample == 16) {
            clip.Format = AL_FORMAT_STEREO16;
        } else {
            return false;
        }

        // Set the sample rate for the audio clip.
        clip.Frequency = sampleRate;

        // Generate an OpenAL buffer and fill it with the audio data.
        alGenBuffers(1, &clip.Buffer);
        alBufferData(clip.Buffer, clip.Format, bufferData.data(), static_cast<ALsizei>(bufferData.size()), clip.Frequency);

        // Return true to indicate that the WAV file was loaded successfully.
        return true;
    }

} // namespace Vulkyrie
