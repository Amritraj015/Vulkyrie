#pragma once

#include "renderer/backends/renderer_backend.h"
#include <glad/glad.h> // GLAD must be included before GLFW.
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    struct OpenGLBufferResource {
    public:
        u32 BufferID;
        u32 Generation;
    };

    class OpenGLRendererBackend final : public RendererBackend {
    public:
        OpenGLRendererBackend(GraphicsAPI graphicsApi);

        VE_DELETE_MOVE_AND_COPY(OpenGLRendererBackend);

        ~OpenGLRendererBackend() override;

        [[nodiscard]] StatusCode Initialize() override;

        void SwapBuffers() override;
        [[nodiscard]] BufferHandle CreateBuffer(std::span<f32> data) override;
        [[nodiscard]] BufferHandle CreateBuffer(size_t size, std::span<f32> data) override;
        void SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) override;
        void DestroyBuffer(const BufferHandle &handle) override;

    private:
        GLFWwindow *_windowHandle;

        std::vector<OpenGLBufferResource> _bufferResources;
        std::vector<size_t> _freeIndices;
    };

} // namespace Vulkyrie
