#pragma once

#include "vlkypch.h"
#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class VulkanQueue {
    public:
        void Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals);
        QueueType Type() noexcept;
    };

} // namespace Vulkyrie
