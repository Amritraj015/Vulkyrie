#pragma once

#include "vlkypch.h"
#include "audio/audio_clip.h"
#include "audio/audio_source.h"

namespace Vulkyrie {

    /** @brief The AudioSystem class manages audio playback, including loading audio clips, playing sounds, and managing audio sources.
     * It serves as the central interface for all audio-related functionality in the engine. */
    class AudioSystem {
    public:
        /** @brief Initializes the audio system. */
        AudioSystem();

        VE_DELETE_MOVE_AND_COPY(AudioSystem);

        /** @brief Cleans up the audio system and releases all resources. */
        ~AudioSystem();

        /** @brief Loads an audio clip from the specified file path. Currently only supports WAV files.
         * @param filepath The path to the audio file to load.
         * @returns A pointer to the loaded AudioClip, or nullptr if loading failed.
         */
        AudioClip *LoadClip(const std::filesystem::path &filepath);

        /** @brief Plays a sound at the specified position with optional looping.
         * @param clip The AudioClip to play.
         * @param position The 3D position to play the sound at. Defaults to the origin (0, 0, 0).
         * @param loop Whether the sound should loop continuously. Defaults to false.
         * @returns An AudioHandle that can be used to control the playback of the sound.
         */
        AudioHandle PlaySound(AudioClip *clip, const glm::vec3 &position = { 0, 0, 0 }, bool loop = false);

        /** @brief Stops the sound associated with the given AudioHandle.
         * @param handle The AudioHandle of the sound to stop.
         */
        void Stop(AudioHandle handle);

        /** @brief Sets the position of the audio listener in 3D space.
         * @param postion The new position of the listener.
         */
        void SetListenerPosition(const glm::vec3 &position);

        /** @brief Sets the orientation of the audio listener in 3D space.
         * @param forward The forward direction vector of the listener.
         * @param up The up direction vector of the listener.
         */
        void SetListenerOrientation(const glm::vec3 &forward, const glm::vec3 &up);

        /** @brief Updates the audio system. This should be called once per frame to ensure proper audio processing. */
        void Update();

    private:
        /** @brief The maximum number of concurrent audio sources that the system can handle.
         * This limits how many sounds can be played simultaneously. */
        static constexpr size_t MAX_SOURCES = 32;

        /** @brief The audio device handle. */
        void *_device = nullptr;

        /** @brief The context used for audio processing. */
        void *_context = nullptr;

        /** @brief A pool of pre-created audio sources that can be used to play sounds. Each source can only play one sound at a time. */
        std::vector<AudioSource> _sources;

        /** @brief A cache of loaded audio clips, keyed by their file paths.
         * This allows for efficient reuse of audio data without needing to reload from disk. */
        std::unordered_map<std::filesystem::path, AudioClip> _clipCache;

        /** @brief Loads a WAV audio file from disk and fills the provided AudioClip with the loaded data.
         * @param filePath The path to the WAV file to load.
         * @param outClip The AudioClip to fill with the loaded audio data.
         * @returns true if the file was loaded successfully, false otherwise.
         */
        bool LoadWAV(const std::filesystem::path &filePath, AudioClip &outClip);
    };

} // namespace Vulkyrie
