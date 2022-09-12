#pragma once

namespace Vkr {
	struct PhysicalDeviceInfo {
		const VkPhysicalDeviceProperties *properties;
		const VkPhysicalDeviceFeatures *features;
		const DeviceRequirements *requirements;
	};
}