#pragma once

#include "renderer/vulkan/vulkan_types.h"
#include "vlkypch.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/rhi/rhi_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanQueue final {
    public:
        VulkanQueue() = default;

        VE_DELETE_COPY(VulkanQueue);

        VulkanQueue(VulkanQueue &&other) noexcept;
        VulkanQueue &operator=(VulkanQueue &&other) noexcept;

        ~VulkanQueue();

        [[nodiscard]] static std::optional<VulkanQueue>
        TryAcquire(VulkanContext *context, QueueType queueType, u32 queueFamilyIndex, u32 queueIndex, VulkanHostAllocator *allocator);

        void Release();

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

        [[nodiscard]] u64 NextSignalValue() noexcept {
            return mNextValue++;
        }

        void WaitValue(u64 value) const;

    private:
        VulkanQueue(VulkanHostAllocator *allocator,
                    VulkanContext *context,
                    VkQueue queueHandle,
                    QueueType queueType,
                    u32 familyIndex,
                    u32 queueIndex,
                    VkSemaphore timelineSemaphore);

        VulkanHostAllocator *pHostAllocator{ nullptr };
        VulkanContext *pContext{ nullptr };
        VkQueue mVkQueueHandle{ VK_NULL_HANDLE };
        VkSemaphore mVkTimelineSemaphore{ VK_NULL_HANDLE };
        u64 mNextValue{ 0 };
        u32 mFamilyIndex{ kInvalidQueueFamilyIndex };
        u32 mQueueIndex{ kInvalidQueueIndex };
        QueueType mQueueType{ QueueType::Count };

        void reset(VulkanQueue &queue);
    };

} // namespace Vulkyrie
