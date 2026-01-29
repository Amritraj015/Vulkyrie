#pragma once

#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {
    class FrameBuffer {
        public:
            virtual ~FrameBuffer() = default;

        public:
            static Ref<FrameBuffer> Create(Vulkyrie::Core::GraphicsAPI api);
    };
} // namespace Vulkyrie::Renderer
