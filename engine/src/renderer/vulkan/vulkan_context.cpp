#include "renderer/vulkan/vulkan_context.h"

namespace Vulkyrie {

    VulkanContext::VulkanContext(const DeviceCreationInfo &info) {
        // TODO: Finish this.
        (void)info;
    }

    void VulkanContext::WaitIdle() const {
        // TODO: vkDeviceWaitIdle() once a VkDevice exists here.
    }

    const DeviceCapabilities &VulkanContext::QueryCapabilities() const {
        return mCapabilities;
    }

    bool VulkanContext::DeviceLost() const {
        return false;
    }

} // namespace Vulkyrie
