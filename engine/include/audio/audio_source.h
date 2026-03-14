#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Audio {

    struct AudioHandle {
        public:
            u32 ID;
            bool Valid;
    };

    class AudioSource {
        public:
            u32 Source;
            bool Active;
            AudioHandle Handle;
            glm::vec3 Position;
            bool Looping;

            void SetPosition(const glm::vec3 &position);
            void SetVolume(f32 gain);
            void Stop();
            bool IsPlaying();
    };

} // namespace Vulkyrie::Audio
