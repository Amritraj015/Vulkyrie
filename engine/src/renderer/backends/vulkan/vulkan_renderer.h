#pragma once

namespace Vulkyrie {

    Scope<Renderer> CreateVulkanRenderer(const DeviceCreationInfo &) {
        return CreateScope<RendererImpl<VulkanBackend>>();
    }

} // namespace Vulkyrie
