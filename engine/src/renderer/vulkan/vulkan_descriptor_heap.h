#pragma once

#include <volk.h>

namespace Vulkyrie {

    class VulkanDescriptorHeap final {
    public:
        [[nodiscard]] VkDescriptorSetLayout Layout() const noexcept {
            return VK_NULL_HANDLE;
        }
    };

} // namespace Vulkyrie
