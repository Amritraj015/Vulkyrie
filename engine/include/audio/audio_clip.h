#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief The AudioClip class represents a loaded audio clip, containing the necessary information and resources to play the audio. */
    class AudioClip final {
    public:
        /** @brief The buffer handle for the audio clip. */
        u32 Buffer = 0;

        /** @brief The format of the audio source. */
        i32 Format = 0;

        /** @brief The frequency (sample rate) of the audio source. */
        i32 Frequency = 0;

        /** @brief Default constructor for AudioClip. Initializes an empty audio clip. */
        AudioClip() = default;

        AudioClip(AudioClip &&other) noexcept;
        AudioClip &operator=(AudioClip &&other) noexcept;

        VE_DELETE_COPY(AudioClip);

        /** @brief Cleans up the audio clip by deleting the underlying resources associated with this clip. */
        ~AudioClip();
    };

} // namespace Vulkyrie
