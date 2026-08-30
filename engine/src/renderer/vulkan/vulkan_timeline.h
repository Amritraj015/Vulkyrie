#pragma once

#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanTimeline final {
    public:
        VulkanTimeline() = default;

        VE_DELETE_MOVE_AND_COPY(VulkanTimeline);

        ~VulkanTimeline();

    private:
        VulkanContext *pContext = nullptr;
        VkSemaphore mVkTimelineSemaphore = VK_NULL_HANDLE;
    };

} // namespace Vulkyrie
