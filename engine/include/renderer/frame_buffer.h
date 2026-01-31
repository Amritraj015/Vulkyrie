#pragma once

#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {

    struct FrameBufferSpecification {
            u32 Width = 0;
            u32 Height = 0;
    };

    class FrameBuffer {
        public:
            virtual ~FrameBuffer() = default;

            // virtual void Bind() const = 0;
            // virtual void Unbind() const = 0;
            // virtual void GetFrameBufferObjectID() const = 0;

            // static Ref<FrameBuffer> Create(const FrameBufferSpecification &specification);
            static Ref<FrameBuffer> Create();

        protected:
            FrameBufferSpecification _specification;
    };
} // namespace Vulkyrie::Renderer
