#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include <vulkan/vulkan_core.h>

namespace Vulkyrie {

    VulkanQueue::VulkanQueue()
        : pHostAllocator(nullptr)
        , pContext(nullptr)
        , mVkQueueHandle(VK_NULL_HANDLE)
        , mVkTimelineSemaphore(VK_NULL_HANDLE)
        , mNextValue(1)
        , mFamilyIndex(kInvalidQueueFamilyIndex)
        , mQueueIndex(kInvalidQueueIndex)
        , mQueueType(QueueType::Count) {
    }

    VulkanQueue::VulkanQueue(VulkanHostAllocator *allocator,
                             VulkanContext *context,
                             VkQueue queueHandle,
                             QueueType queueType,
                             u32 familyIndex,
                             u32 queueIndex,
                             VkSemaphore timelineSemaphore)
        : pHostAllocator(allocator)
        , pContext(context)
        , mVkQueueHandle(queueHandle)
        , mVkTimelineSemaphore(timelineSemaphore)
        , mNextValue(1)
        , mFamilyIndex(familyIndex)
        , mQueueIndex(queueIndex)
        , mQueueType(queueType) {
    }

    VulkanQueue::VulkanQueue(VulkanQueue &&other) noexcept
        : pHostAllocator(other.pHostAllocator)
        , pContext(other.pContext)
        , mVkQueueHandle(other.mVkQueueHandle)
        , mVkTimelineSemaphore(other.mVkTimelineSemaphore)
        , mNextValue(other.mNextValue)
        , mFamilyIndex(other.mFamilyIndex)
        , mQueueIndex(other.mQueueIndex)
        , mQueueType(other.mQueueType) {
        other.pHostAllocator = nullptr;
        other.pContext = nullptr;
        other.mVkQueueHandle = VK_NULL_HANDLE;
        other.mVkTimelineSemaphore = VK_NULL_HANDLE;
        other.mNextValue = 0;
        other.mFamilyIndex = 0;
        other.mQueueIndex = 0;
        other.mQueueType = QueueType::Count;
    }

    VulkanQueue &VulkanQueue::operator=(VulkanQueue &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        if (nullptr != pContext && VK_NULL_HANDLE != mVkTimelineSemaphore && VK_NULL_HANDLE != pContext->Device()) {
            vkDestroySemaphore(pContext->Device(), mVkTimelineSemaphore, pHostAllocator->Callbacks());
        }

        pHostAllocator = other.pHostAllocator;
        pContext = other.pContext;
        mVkQueueHandle = other.mVkQueueHandle;
        mVkTimelineSemaphore = other.mVkTimelineSemaphore;
        mNextValue = other.mNextValue;
        mFamilyIndex = other.mFamilyIndex;
        mQueueIndex = other.mQueueIndex;
        mQueueType = other.mQueueType;

        other.pHostAllocator = nullptr;
        other.pContext = nullptr;
        other.mVkQueueHandle = VK_NULL_HANDLE;
        other.mVkTimelineSemaphore = VK_NULL_HANDLE;
        other.mNextValue = 0;
        other.mFamilyIndex = 0;
        other.mQueueIndex = 0;
        other.mQueueType = QueueType::Count;

        return *this;
    }

    VulkanQueue::~VulkanQueue() {
        if (nullptr != pContext && VK_NULL_HANDLE != mVkTimelineSemaphore && VK_NULL_HANDLE != pContext->Device()) {
            vkDestroySemaphore(pContext->Device(), mVkTimelineSemaphore, pHostAllocator->Callbacks());
        }
    }

    std::optional<VulkanQueue>
    VulkanQueue::Get(VulkanContext *context, QueueType queueType, u32 queueFamilyIndex, u32 queueIndex, VulkanHostAllocator *allocator) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(context->Device(), queueFamilyIndex, queueIndex, &queue);

        if (VK_NULL_HANDLE == queue) {
            return std::nullopt;
        }

        VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 2, // TODO: Make this frames in flight count;
        };

        VkSemaphoreCreateInfo timelineSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCreateInfo,
            .flags = 0,
        };

        VkSemaphore timelineSemaphore = VK_NULL_HANDLE;

        VE_VK_TRY_CREATE(vkCreateSemaphore(context->Device(), &timelineSemaphoreCreateInfo, allocator->Callbacks(), &timelineSemaphore));

#if defined(VE_VK_ENABLE_VALIDATION)

        switch (queueType) {
            case QueueType::Compute:
                context->SetDebugName("ComputeQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("ComputeTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            case QueueType::Graphics:
                context->SetDebugName("GraphicsQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("GraphicsTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            case QueueType::Transfer:
                context->SetDebugName("TransferQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("TransferTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            case QueueType::SparseBinding:
                context->SetDebugName("SparseBindingQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("SparseBindingTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            case QueueType::VideoEncode:
                context->SetDebugName("VideoEncodeQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("VideoEncodeTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            case QueueType::VideoDecode:
                context->SetDebugName("VideoDecodeQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("VideoDecodeTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
            default:
                context->SetDebugName("UnknownQueue", VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
                context->SetDebugName("UnknownTimeline", VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));
                break;
        }

#endif

        return VulkanQueue{ allocator, context, queue, queueType, queueFamilyIndex, queueIndex, timelineSemaphore };
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
