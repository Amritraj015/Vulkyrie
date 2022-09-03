#pragma once
#include "Defines.h"

namespace Vkr
{
    struct VulkanImage
    {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        u32 width;
        u32 height;
    };
}