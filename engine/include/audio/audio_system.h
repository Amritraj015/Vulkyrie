#pragma once

#include "audio/audio_clip.h"
#include "audio/audio_source.h"

namespace Vulkyrie::Audio {

    class AudioSystem {
        public:
            AudioSystem();
            ~AudioSystem();

            AudioSystem(const AudioSystem &) = delete;
            AudioSystem(AudioSystem &&) = delete;
            AudioSystem &operator=(const AudioSystem &) = delete;
            AudioSystem &operator=(AudioSystem &&) = delete;

            // Load WAV (16-bit PCM mono/stereo)
            AudioClip *LoadClip(const std::filesystem::path &filename);
            AudioHandle PlaySound(AudioClip *clip, const glm::vec3 &position = { 0, 0, 0 }, bool loop = false);
            void Stop(AudioHandle handle);
            void SetListenerPosition(const glm::vec3 &pos);
            void SetListenerOrientation(const glm::vec3 &forward, const glm::vec3 &up);
            void Update();

        private:
            static constexpr size_t MAX_SOURCES = 32;

            std::vector<AudioSource> sources;
            std::unordered_map<std::filesystem::path, AudioClip> clipCache;

            bool LoadWAV(std::filesystem::path filePath, AudioClip &clip);
    };

} // namespace Vulkyrie::Audio
