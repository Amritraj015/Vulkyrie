#include "renderer/vulkan/vulkan_command_list.h"

namespace Vulkyrie {

    void VulkanCommandList::Begin() {
        // TODO: vkBeginCommandBuffer once a pool hands out real buffers.
    }

    void VulkanCommandList::End() {
        // TODO: vkEndCommandBuffer.
    }

    void VulkanCommandList::EmitBarriers(std::span<const ResourceBarrier> barriers) {
        // TODO: translate to VkImageMemoryBarrier2 / VkBufferMemoryBarrier2 and issue one
        // vkCmdPipelineBarrier2. A barrier with AliasingTransition set must transition from
        // VK_IMAGE_LAYOUT_UNDEFINED rather than from Before.Layout.
        (void)barriers;
    }

} // namespace Vulkyrie
