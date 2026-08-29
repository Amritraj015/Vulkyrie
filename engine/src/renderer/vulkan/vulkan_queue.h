#pragma once

#include "vlkypch.h"
#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/rhi/rhi_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanQueue final {
    public:
        VulkanQueue();

        VulkanQueue(VulkanContext *context, VkQueue queueHandle, u32 familyIndex, QueueType queueType);

        ~VulkanQueue() = default;

        void Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals);

        [[nodiscard]] VE_INLINE QueueType Type() const noexcept {
            return mQueueType;
        }

        [[nodiscard]] VE_INLINE u32 FamilyIndex() const noexcept {
            return mFamilyIndex;
        }

        [[nodiscard]] VE_INLINE VkQueue Handle() const noexcept {
            return mVkQueueHandle;
        }

    private:
        [[maybe_unused]] VulkanContext *pContext;
        VkQueue mVkQueueHandle;
        u32 mFamilyIndex;
        QueueType mQueueType;
    };

} // namespace Vulkyrie
