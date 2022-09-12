#pragma once
#include "Defines.h"

namespace Vkr {
	struct ImageInfo {
		VkImageType imageType;
		u32 width;
		u32 height;
		VkFormat format;
		VkImageTiling tiling;
		VkImageUsageFlags usage;
		VkMemoryPropertyFlags memoryFlags;
		bool createView;
		VkImageAspectFlags viewAspectFlags;
	};
}