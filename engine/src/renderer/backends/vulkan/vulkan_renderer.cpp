#include "vlkypch.h"
#include "renderer/renderer_impl.h"
#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/vulkan/vulkan_backend.h"

namespace Vulkyrie {

    static_assert(RendererBackend<VulkanBackend>, "VulkanBackend does not satisfy RendererBackend concept.");

    template class RendererImpl<VulkanBackend>;

    Scope<Renderer> CreateVulkanRenderer(const DeviceCreationInfo &info) {
        return CreateScope<RendererImpl<VulkanBackend>>(info);
    }

} // namespace Vulkyrie
