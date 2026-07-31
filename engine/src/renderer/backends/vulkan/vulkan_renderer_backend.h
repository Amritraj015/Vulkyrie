#pragma once

#include "renderer/backends/renderer_backend.h"

namespace Vulkyrie {

    class VulkanRendererBackend final : public RendererBackend {
    public:
        VulkanRendererBackend(GraphicsAPI graphicsApi);

        VE_DELETE_MOVE_AND_COPY(VulkanRendererBackend);

        ~VulkanRendererBackend() override;

        [[nodiscard]] StatusCode Initialize() override {
            return StatusCode::Successful;
        }

        void SwapBuffers() override;
        [[nodiscard]] BufferHandle CreateBuffer(std::span<f32> data) override;
        [[nodiscard]] BufferHandle CreateBuffer(size_t size, std::span<f32> data) override;
        void SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) override;
        void DestroyBuffer(const BufferHandle &handle) override;
    };

} // namespace Vulkyrie
