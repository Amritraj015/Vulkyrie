#pragma once
#include "Defines.h"

namespace Vkr
{
    struct VulkanSwapchainSupportInfo
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
}