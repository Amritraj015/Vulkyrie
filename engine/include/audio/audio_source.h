#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    struct AudioHandle {
    public:
        u32 ID;
        bool Valid;
    };

    /** @brief The AudioSource class represents an individual audio source in the audio system, responsible for playing a specific sound at a given position in
     * 3D space. It contains information about the source's state, position, and playback control. */
    class AudioSource {
    public:
        /** @brief The source ID for this audio source. */
        u32 Source;

        /** @brief Indicates whether this audio source is currently active (playing a sound) or not. */
        bool Active;

        /** @brief The handle associated with this audio source, used for controlling playback. */
        AudioHandle Handle;

        /** @brief The 3D position of the audio source in space. */
        glm::vec3 Position;

        /** @brief Indicates whether the audio source should loop the sound continuously. */
        bool Looping;

        /** @brief Sets the 3D position of the audio source in space.
         * @param position The new position of the audio source. */
        void SetPosition(const glm::vec3 &position);

        /** @brief Sets the volume (gain) of the audio source.
         * @param gain The new volume level for the audio source. A value of 1.0f is normal volume, less than 1.0f is quieter, and greater than 1.0f is
         * louder. */
        void SetVolume(f32 gain);

        /** @brief Stops the audio source from playing and marks it as inactive. */
        void Stop();

        /** @brief Checks if the audio source is currently playing a sound.
         * @returns true if the audio source is playing, false otherwise. */
        bool IsPlaying();
    };

} // namespace Vulkyrie
