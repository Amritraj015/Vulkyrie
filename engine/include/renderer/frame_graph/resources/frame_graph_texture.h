#pragma once

#include "renderer/texture_specification.h"

namespace Vulkyrie::Renderer {

    class FrameGraphTexture {
        public:
            struct Descriptor : public TextureSpecification {};

            void Create(const FrameGraphTexture::Descriptor &descriptor, void *context);
            void Destroy(const FrameGraphTexture::Descriptor &descriptor, void *context);
    };

} // namespace Vulkyrie::Renderer
