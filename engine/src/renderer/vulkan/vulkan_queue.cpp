#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_types.h"

namespace Vulkyrie {

    VulkanQueue::VulkanQueue()
        : pContext(nullptr)
        , mVkQueueHandle(VK_NULL_HANDLE)
        , mFamilyIndex(kInvalidQueueFamily)
        , mQueueType(QueueType::Count) {
    }

    VulkanQueue::VulkanQueue(VulkanContext *context, VkQueue queueHandle, u32 familyIndex, QueueType queueType)
        : pContext(context)
        , mVkQueueHandle(queueHandle)
        , mFamilyIndex(familyIndex)
        , mQueueType(queueType) {
    }

    void VulkanQueue::Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals) {
        (void)lists;
        (void)waits;
        (void)signals;
    }

} // namespace Vulkyrie
