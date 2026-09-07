#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include "renderer/vulkan/vulkan_backend.h"
#include <vulkan/vulkan_core.h>

namespace Vulkyrie {

    namespace {

        inline constexpr std::array<std::array<StaticString, 2>, static_cast<usize>(QueueType::Count)> kQueueTypesDebugNames{ {
#define X(name)                                                                                                                                                \
    {                                                                                                                                                          \
        #name "Queue",                                                                                                                                         \
        #name "QueueTimeline",                                                                                                                                 \
    },
            VE_RENDERER_QUEUE_TYPES(X)
#undef X
        } };

        [[maybe_unused]] VE_INLINE constexpr const std::array<StaticString, 2> &ToQueueTypesDebugNames(QueueType queueType) {
            return kQueueTypesDebugNames[static_cast<usize>(queueType)];
        }

    } // namespace

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
        , mNextValue(VulkanBackend::kFramesInFlight + 1)
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
        reset(other);
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

        reset(other);

        return *this;
    }

    VulkanQueue::~VulkanQueue() {
        Release();
    }

    std::optional<VulkanQueue>
    VulkanQueue::TryAcquire(VulkanContext *context, QueueType queueType, u32 queueFamilyIndex, u32 queueIndex, VulkanHostAllocator *allocator) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(context->Device(), queueFamilyIndex, queueIndex, &queue);

        if (VK_NULL_HANDLE == queue) {
            return std::nullopt;
        }

        VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = VulkanBackend::kFramesInFlight,
        };

        VkSemaphoreCreateInfo timelineSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCreateInfo,
            .flags = 0,
        };

        VkSemaphore timelineSemaphore = VK_NULL_HANDLE;

        VE_VK_TRY_CREATE(vkCreateSemaphore(context->Device(), &timelineSemaphoreCreateInfo, allocator->Callbacks(), &timelineSemaphore));

#if defined(VE_VK_ENABLE_VALIDATION)

        const std::array<StaticString, 2> &debugNames = ToQueueTypesDebugNames(queueType);
        context->SetDebugName(debugNames[0], VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue));
        context->SetDebugName(debugNames[1], VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(timelineSemaphore));

#endif

        return VulkanQueue{ allocator, context, queue, queueType, queueFamilyIndex, queueIndex, timelineSemaphore };
    }

    void VulkanQueue::Release() {
        if (nullptr != pContext && VK_NULL_HANDLE != mVkTimelineSemaphore && VK_NULL_HANDLE != pContext->Device()) {
            vkDestroySemaphore(pContext->Device(), mVkTimelineSemaphore, pHostAllocator->Callbacks());
        }

        reset(*this);
    }

    void VulkanQueue::Submit(std::span<const VulkanCommandList *const> lists, std::span<u64> waits, std::span<u64> signals) {
        (void)lists;
        (void)waits;
        (void)signals;
    }

    void VulkanQueue::WaitValue(u64 value) const {
        const VkSemaphoreWaitInfo waitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &mVkTimelineSemaphore,
            .pValues = &value,
        };

        while (true) {
            const VkResult result = vkWaitSemaphores(pContext->Device(), &waitInfo, UINT64_MAX);

            if (VK_SUCCESS == result) return;
            if (VK_TIMEOUT == result) continue;

            if (VK_ERROR_DEVICE_LOST == result) {
                pContext->MarkDeviceLost();

                VASSERT(false, "Vulkan device lost while waiting for timeline semaphore for queue type: {}", static_cast<u8>(mQueueType));
            }

            return;
        }
    }

    void VulkanQueue::reset(VulkanQueue &queue) {
        queue.pHostAllocator = nullptr;
        queue.pContext = nullptr;
        queue.mVkQueueHandle = VK_NULL_HANDLE;
        queue.mVkTimelineSemaphore = VK_NULL_HANDLE;
        queue.mNextValue = 0;
        queue.mFamilyIndex = kInvalidQueueFamilyIndex;
        queue.mQueueIndex = kInvalidQueueIndex;
        queue.mQueueType = QueueType::Count;
    }

} // namespace Vulkyrie
