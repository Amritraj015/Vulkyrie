#pragma once
#include "Defines.h"
#include "VulkanImage.h"

namespace Vkr
{
    struct VulkanSwapchain
    {
        VkSurfaceFormatKHR imageFormat;
        u8 maxFramesInFlight;
        VkSwapchainKHR handle;
        u32 imageCount;
        VkImage *images;
        VkImageView *views;
        VulkanImage depthAttachment;
    };
}