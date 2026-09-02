#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_context.h"

namespace Vulkyrie {

    VulkanQueue::VulkanQueue()
        : pContext(nullptr)
        , mVkQueueHandle(VK_NULL_HANDLE)
        , mFamilyIndex(kInvalidQueueFamilyIndex)
        , mQueueIndex(kInvalidQueueIndex)
        , mQueueType(QueueType::Count) {
    }

    VulkanQueue::VulkanQueue(VulkanContext *context, VkQueue queueHandle, QueueType queueType, u32 familyIndex, u32 queueIndex)
        : pContext(context)
        , mVkQueueHandle(queueHandle)
        , mFamilyIndex(familyIndex)
        , mQueueIndex(queueIndex)
        , mQueueType(queueType) {
    }

    std::optional<VulkanQueue> VulkanQueue::Get(VulkanContext *context, QueueType queueType, u32 queueFamilyIndex, u32 queueIndex) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(context->Device(), queueFamilyIndex, queueIndex, &queue);

        if (VK_NULL_HANDLE == queue) {
            return std::nullopt;
        }

        return VulkanQueue{ context, queue, queueType, queueFamilyIndex, queueIndex };
    }

    void VulkanQueue::Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals) {
        (void)lists;
        (void)waits;
        (void)signals;

        // // ensure swapchain image is actually viable to start color output
        // VkSemaphoreSubmitInfo imageAcquireWaitInfo{};
        // imageAcquireWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        // imageAcquireWaitInfo.semaphore = imageAcquireSemaphore;
        // imageAcquireWaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; // wait before drawing to image
        //                                                                                   // signal that the image can be presented
        //
        // // render work completion signal
        // VkSemaphoreSubmitInfo semSignal1{};
        // semSignal1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        // semSignal1.semaphore = mVkRenderCompleteSemaphores[imageIndex];
        // semSignal1.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        //
        // // entire frame is completed (timeline)
        // VkSemaphoreSubmitInfo semSignal2{};
        // semSignal2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        // semSignal2.semaphore = mVkTimelineSemaphore;
        // semSignal2.value = signalValue;
        // semSignal2.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        // RendererVector<VkSemaphoreSubmitInfo> semaphoreSignals{ semSignal1, semSignal2 };
        //
        // VkCommandBufferSubmitInfo cmdSubmitInfo{};
        // cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        // cmdSubmitInfo.commandBuffer = res.CommandBuffer;
        //
        // VkSubmitInfo2 submitInfo{};
        // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        // submitInfo.waitSemaphoreInfoCount = 1;
        // submitInfo.pWaitSemaphoreInfos = &imageAcquireWaitInfo; // ensure the image is ready
        // submitInfo.commandBufferInfoCount = 1;
        // submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
        // submitInfo.signalSemaphoreInfoCount = static_cast<u32>(semaphoreSignals.size());
        // submitInfo.pSignalSemaphoreInfos = semaphoreSignals.data();
        //
        // vkQueueSubmit2(mVkQueueHandle, 1, &submitInfo, VK_NULL_HANDLE);
    }

} // namespace Vulkyrie
