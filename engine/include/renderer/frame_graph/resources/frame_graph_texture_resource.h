#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {

    class FrameGraphTextureResource {
        public:
            struct Descriptor {
                public:
                    u32 Height;
                    u32 Width;
            };

            void Create(const Descriptor &descriptor, void *context) {
            }

            void Destroy(const Descriptor &descriptor, void *context) {
            }
    };

} // namespace Vulkyrie::Renderer
