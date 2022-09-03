#pragma once
#include "Defines.h"

namespace Vkr
{
    struct VulkanSwapchainSupportInfo
    {
        VkSurfaceCapabilitiesKHR capabilities;
        u32 formatCount = 0;
        VkSurfaceFormatKHR *formats;
        u32 presentationCount = 0;
        VkPresentModeKHR *presentModes;
    };
}