#pragma once

#include <volk.h>

namespace Vulkyrie {

    class VulkanDescriptorHeap final {
    public:
        [[nodiscard]] VkDescriptorSetLayout Layout() const noexcept;
    };

} // namespace Vulkyrie
