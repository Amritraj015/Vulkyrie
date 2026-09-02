#pragma once

#include "renderer/vulkan/vulkan_context.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanCommandPool final {
    public:
        [[nodiscard]] VulkanCommandList &Acquire();

        void ResetAll();

        [[nodiscard]] VkCommandPool Handle() const noexcept {
            return mCommandPoolHandle;
        }

    private:
        RendererVector<VulkanCommandList> mCommandList;
        VulkanContext *pContext;
        VkCommandPool mCommandPoolHandle;
        QueueType mQueueType;
    };

} // namespace Vulkyrie
