#pragma once

#include "vlkypch.h"
#include "renderer/rhi/barrier_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanCommandList {
    public:
        VulkanCommandList() = default;

        VE_DELETE_COPY(VulkanCommandList);

        VulkanCommandList(VulkanCommandList &&) noexcept = default;
        VulkanCommandList &operator=(VulkanCommandList &&) noexcept = default;

        ~VulkanCommandList() = default;

        static std::optional<VulkanCommandList> Create(VulkanContext *context, VkCommandPool pool, enum QueueType queueType);

        void Begin();
        void End();

        [[nodiscard]] VE_INLINE VkCommandBuffer Handle() const noexcept {
            return mVkCommandBufferHandle;
        }

        [[nodiscard]] VE_INLINE QueueType QueueType() const noexcept {
            return mQueueType;
        }

        /** @brief Emits one pass's batched transitions as `vkCmdPipelineBarrier2`.
         * @param barriers The batch; a barrier with `AliasingTransition` set must transition from
         * `VK_IMAGE_LAYOUT_UNDEFINED` rather than from `Before.Layout`. */
        void EmitBarriers(std::span<const ResourceBarrier> barriers);

    private:
        explicit VulkanCommandList(VulkanContext *context, VkCommandBuffer commandBuffer, enum QueueType queueType);

        [[maybe_unused]] VulkanContext *pContext{ nullptr };
        VkCommandBuffer mVkCommandBufferHandle{ VK_NULL_HANDLE };
        enum QueueType mQueueType { QueueType::Count };
    };

} // namespace Vulkyrie
