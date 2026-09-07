#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"

namespace Vulkyrie {

    namespace {

        inline constexpr std::array<StaticString, static_cast<usize>(QueueType::Count)> kCommandBufferNames{
#define X(name) #name "QueueCommandBuffer",
            VE_RENDERER_QUEUE_TYPES(X)
#undef X
        };

        [[maybe_unused]] VE_INLINE constexpr StaticString ToCommandBufferDebugName(QueueType queueType) {
            return kCommandBufferNames[static_cast<usize>(queueType)];
        }

    } // namespace

    VulkanCommandList::VulkanCommandList(VulkanContext *context, VkCommandBuffer commandBuffer, enum QueueType queueType)
        : pContext(context)
        , mVkCommandBufferHandle(commandBuffer)
        , mQueueType(queueType) {
    }

    std::optional<VulkanCommandList> VulkanCommandList::Create(VulkanContext *context, VkCommandPool pool, enum QueueType queueType) {
        const VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VE_VK_TRY_CREATE(vkAllocateCommandBuffers(context->Device(), &allocateInfo, &commandBuffer));

#if defined(VE_VK_ENABLE_VALIDATION)

        context->SetDebugName(ToCommandBufferDebugName(queueType), VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<u64>(commandBuffer));

#endif

        return VulkanCommandList{ context, commandBuffer, queueType };
    }

    void VulkanCommandList::Begin() {
        // const VkCommandBufferBeginInfo beginInfo{
        //     .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        //     .pNext = VK_NULL_HANDLE,
        //     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        //     .pInheritanceInfo = VK_NULL_HANDLE,
        // };
        //
        // if (VK_SUCCESS != vkBeginCommandBuffer(mVkCommandBufferHandle, &beginInfo)) {
        //     pContext->MarkDeviceLost();
        //
        //     return;
        // }
    }

    void VulkanCommandList::End() {
        if (VK_SUCCESS != vkEndCommandBuffer(mVkCommandBufferHandle)) {
            pContext->MarkDeviceLost();
        }
    }

    void VulkanCommandList::EmitBarriers(std::span<const ResourceBarrier> barriers) {
        // TODO: translate to VkImageMemoryBarrier2 / VkBufferMemoryBarrier2 and issue one
        // vkCmdPipelineBarrier2. A barrier with AliasingTransition set must transition from
        // VK_IMAGE_LAYOUT_UNDEFINED rather than from Before.Layout.
        (void)barriers;
    }

} // namespace Vulkyrie
