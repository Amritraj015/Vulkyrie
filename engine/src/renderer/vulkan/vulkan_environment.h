#pragma once

#include <volk.h>

namespace Vulkyrie {

    std::vector<const VkLayerProperties> EnumerateInstanceLayers();
    std::vector<const VkExtensionProperties> EnumerateInstanceExtensions();
    std::vector<const VkExtensionProperties> EnumerateDeviceExtensions();

    bool HasInstanceLayer();
    bool HasInstanceExtension();
    bool HasDeviceExtension();

} // namespace Vulkyrie
