#pragma once

#include "renderer/vulkan/vulkan_host_allocator.h"
#include "vlkypch.h"
#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/rhi/rhi_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanQueue final {
    public:
        VulkanQueue();

        VE_DELETE_COPY(VulkanQueue);

        VulkanQueue(VulkanQueue &&other) noexcept;
        VulkanQueue &operator=(VulkanQueue &&other) noexcept;

        ~VulkanQueue();

        [[nodiscard]] static std::optional<VulkanQueue>
        Get(VulkanContext *context, QueueType queueType, u32 queueFamilyIndex, u32 queueIndex, VulkanHostAllocator *allocator);

        void Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals);

        [[nodiscard]] VE_INLINE QueueType Type() const noexcept {
            return mQueueType;
        }

        [[nodiscard]] VE_INLINE u32 FamilyIndex() const noexcept {
            return mFamilyIndex;
        }

        [[nodiscard]] VE_INLINE u32 QueueIndex() const noexcept {
            return mQueueIndex;
        }

        [[nodiscard]] VE_INLINE VkQueue Handle() const noexcept {
            return mVkQueueHandle;
        }

        [[nodiscard]] VE_INLINE VkSemaphore Timeline() const noexcept {
            return mVkTimelineSemaphore;
        }

        [[nodiscard]] u64 NextSignalValue() const noexcept {
            return mNextValue;
        }

    private:
        VulkanQueue(VulkanHostAllocator *allocator,
                    VulkanContext *context,
                    VkQueue queueHandle,
                    QueueType queueType,
                    u32 familyIndex,
                    u32 queueIndex,
                    VkSemaphore timelineSemaphore);

        VulkanHostAllocator *pHostAllocator;
        VulkanContext *pContext;
        VkQueue mVkQueueHandle;
        VkSemaphore mVkTimelineSemaphore;
        u64 mNextValue;
        u32 mFamilyIndex;
        u32 mQueueIndex;
        QueueType mQueueType;
    };

} // namespace Vulkyrie
