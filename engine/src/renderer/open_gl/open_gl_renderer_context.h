#pragma once

#include "renderer/renderer_context.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    struct OpenGLBufferResource {
        public:
            u32 BufferID;
            u32 Generation;
    };

    class OpenGLRendererContext final : public RendererContext {
        public:
            /** @brief Constructs a new OpenGLGraphicsContext with the given window handle.
             * @param windowHandle The handle to the window.
             */
            OpenGLRendererContext(void *windowHandle);

            OpenGLRendererContext(const OpenGLRendererContext &) = delete;
            OpenGLRendererContext &operator=(const OpenGLRendererContext &) = delete;

            OpenGLRendererContext(OpenGLRendererContext &&) = delete;
            OpenGLRendererContext &operator=(OpenGLRendererContext &&) = delete;

            ~OpenGLRendererContext() override;

            StatusCode Initialize() override;
            void SwapBuffers() override;

            BufferHandle CreateBuffer(std::span<f32> data) override;
            BufferHandle CreateBuffer(size_t size, std::span<f32> data) override;
            void SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) override;
            void DestroyBuffer(const BufferHandle &handle) override;

        private:
            GLFWwindow *_windowHandle;

            // TODO: Need to remove this from hre.
            std::vector<OpenGLBufferResource> _bufferResources;
            std::vector<size_t> _freeIndices;
            // TODO: Need to remove this from hre.
    };
} // namespace Vulkyrie
