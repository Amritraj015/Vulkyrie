#include "renderer/vulkan/vulkan_command_pool.h"
#include "renderer/vulkan/vulkan_utilities.h"

namespace Vulkyrie {

    VulkanCommandPool::VulkanCommandPool(
        VulkanHostAllocator *allocator, VulkanContext *context, VkCommandPool pool, QueueType queueType, u32 queueFamilyIndex, size_t commandListCapacity)
        : mCommandList()
        , pHostAllocator(allocator)
        , pContext(context)
        , mCommandPoolHandle(pool)
        , mCapacity(commandListCapacity)
        , mQueueFamilyIndex(queueFamilyIndex)
        , mQueueType(queueType) {
        mCommandList.reserve(mCapacity);
    }

    VulkanCommandPool::VulkanCommandPool(VulkanCommandPool &&other) noexcept
        : mCommandList(std::move(other.mCommandList))
        , pHostAllocator(other.pHostAllocator)
        , pContext(other.pContext)
        , mCommandPoolHandle(other.mCommandPoolHandle)
        , mCapacity(other.mCapacity)
        , mQueueFamilyIndex(other.mQueueFamilyIndex)
        , mNextFree(other.mNextFree)
        , mQueueType(other.mQueueType) {

        reset(other);
    }

    VulkanCommandPool &VulkanCommandPool::operator=(VulkanCommandPool &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        if (nullptr != pContext && VK_NULL_HANDLE != pContext->Device() && VK_NULL_HANDLE != mCommandPoolHandle) {
            vkDestroyCommandPool(pContext->Device(), mCommandPoolHandle, pHostAllocator->Callbacks());
        }

        pHostAllocator = other.pHostAllocator;
        mCommandList = std::move(other.mCommandList);
        pContext = other.pContext;
        mCommandPoolHandle = other.mCommandPoolHandle;
        mCapacity = other.mCapacity;
        mQueueFamilyIndex = other.mQueueFamilyIndex;
        mNextFree = other.mNextFree;
        mQueueType = other.mQueueType;

        reset(other);

        return *this;
    }

    VulkanCommandPool::~VulkanCommandPool() {
        if (VK_NULL_HANDLE == mCommandPoolHandle) {
            return;
        }

        vkDestroyCommandPool(pContext->Device(), mCommandPoolHandle, pHostAllocator->Callbacks());

        mCommandPoolHandle = VK_NULL_HANDLE;
    }

    std::optional<VulkanCommandPool>
    VulkanCommandPool::Create(VulkanContext *context, u32 queueFamilyIndex, QueueType queueType, VulkanHostAllocator *allocator, size_t commandListCapacity) {
        const VkCommandPoolCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = queueFamilyIndex,
        };

        VkCommandPool commandPool = VK_NULL_HANDLE;

        VE_VK_TRY_CREATE(vkCreateCommandPool(context->Device(), &info, allocator->Callbacks(), &commandPool));

#if defined(VE_VK_ENABLE_VALIDATION)

        switch (queueType) {
            case QueueType::Compute:
                context->SetDebugName("ComputeCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            case QueueType::Graphics:
                context->SetDebugName("GraphicsCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            case QueueType::Transfer:
                context->SetDebugName("TransferCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            case QueueType::SparseBinding:
                context->SetDebugName("SparseBindingCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            case QueueType::VideoEncode:
                context->SetDebugName("VideoEncodeCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            case QueueType::VideoDecode:
                context->SetDebugName("VideoDecodeCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
            default:
                context->SetDebugName("UnknownCommandPool", VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(commandPool));
                break;
        }

#endif

        return std::make_optional(VulkanCommandPool{ allocator, context, commandPool, queueType, queueFamilyIndex, commandListCapacity });
    }

    VulkanCommandList *VulkanCommandPool::Acquire() {
        if (mNextFree < mCommandList.size()) {
            return &mCommandList[mNextFree++];
        }

        if (mCommandList.size() == mCapacity) {
            VWARN("CommandPool exhausted, raise capacity.");

            return nullptr;
        }

        std::optional<VulkanCommandList> commandList = VulkanCommandList::Create(pContext, mCommandPoolHandle, mQueueType);

        if (!commandList.has_value()) {
            pContext->MarkDeviceLost();

            return nullptr;
        }

        ++mNextFree;

        mCommandList.push_back(std::move(*commandList));

        return &mCommandList.back();
    }

    void VulkanCommandPool::ResetAll() {
        if (VK_SUCCESS != vkResetCommandPool(pContext->Device(), mCommandPoolHandle, 0)) {
            pContext->MarkDeviceLost();
        }
    }

    void VulkanCommandPool::reset(VulkanCommandPool &pool) {
        pool.pHostAllocator = nullptr;
        pool.pContext = nullptr;
        pool.mCommandPoolHandle = VK_NULL_HANDLE;
        pool.mCapacity = 0;
        pool.mQueueFamilyIndex = kInvalidQueueFamilyIndex;
        pool.mNextFree = 0;
        pool.mQueueType = QueueType::Count;
    }

} // namespace Vulkyrie
