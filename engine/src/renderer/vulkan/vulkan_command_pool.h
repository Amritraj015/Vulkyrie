#pragma once

#include "renderer/vulkan/vulkan_context.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanCommandPool final {
    public:
        VulkanCommandPool() = default;

        VE_DELETE_COPY(VulkanCommandPool);

        VulkanCommandPool(VulkanCommandPool &&) noexcept;
        VulkanCommandPool &operator=(VulkanCommandPool &&) noexcept;

        ~VulkanCommandPool();

        static std::optional<VulkanCommandPool>
        Create(VulkanContext *context, u32 queueFamilyIndex, QueueType queueType, VulkanHostAllocator *allocator, size_t commandListCapacity);

        [[nodiscard]] VulkanCommandList *Acquire();

        void ResetAll();

        [[nodiscard]] VkCommandPool Handle() const noexcept {
            return mCommandPoolHandle;
        }

    private:
        VulkanCommandPool(
            VulkanHostAllocator *allocator, VulkanContext *context, VkCommandPool pool, QueueType queueType, u32 queueFamilyIndex, size_t commandListCapacity);

        RendererVector<VulkanCommandList> mCommandList;
        VulkanHostAllocator *pHostAllocator{ nullptr };
        VulkanContext *pContext{ nullptr };
        VkCommandPool mCommandPoolHandle{ VK_NULL_HANDLE };
        size_t mCapacity{ 0 };
        u32 mQueueFamilyIndex{ kInvalidQueueFamilyIndex };
        u32 mNextFree{ 0 };
        QueueType mQueueType{ QueueType::Count };

        void reset(VulkanCommandPool &pool);
    };

} // namespace Vulkyrie
