#include "renderer/backends/vulkan/vulkan_renderer_backend.h"

namespace Vulkyrie {

    VulkanRendererBackend::VulkanRendererBackend(GraphicsAPI graphicsApi)
        : RendererBackend(graphicsApi) {
    }

    VulkanRendererBackend::~VulkanRendererBackend() {
    }

    void VulkanRendererBackend::SwapBuffers() {
    }

    BufferHandle VulkanRendererBackend::CreateBuffer(std::span<f32> data) {
        return CreateBuffer(data.size() * sizeof(f32), data);
    }

    BufferHandle VulkanRendererBackend::CreateBuffer(size_t size, std::span<f32> data) {
        (void)size;
        (void)data;
        return {};
    }

    void VulkanRendererBackend::SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) {
        (void)handle;
        (void)startIndex;
        (void)data;
    }

    void VulkanRendererBackend::DestroyBuffer(const BufferHandle &handle) {
        (void)handle;
    }

} // namespace Vulkyrie
