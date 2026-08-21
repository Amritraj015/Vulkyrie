#pragma once

#include "vlkypch.h"
#include "renderer/rhi/barrier_types.h"

namespace Vulkyrie {

    class VulkanCommandList {
    public:
        void Begin();
        void End();

        /** @brief Emits one pass's batched transitions as `vkCmdPipelineBarrier2`.
         * @param barriers The batch; a barrier with `AliasingTransition` set must transition from
         * `VK_IMAGE_LAYOUT_UNDEFINED` rather than from `Before.Layout`. */
        void EmitBarriers(std::span<const ResourceBarrier> barriers);
    };

} // namespace Vulkyrie
