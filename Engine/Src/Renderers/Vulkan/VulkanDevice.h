#pragma once
#include "VulkanSwapchainSupportInfo.h"

namespace Vkr
{
    struct VulkanDevice
    {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VulkanSwapchainSupportInfo swapchainSupport;
        i32 graphicsQueueIndex;
        i32 presentQueueIndex;
        i32 transferQueueIndex;

        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkQueue transferQueue;

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memory;

        VkFormat depthFormat;
    };
}