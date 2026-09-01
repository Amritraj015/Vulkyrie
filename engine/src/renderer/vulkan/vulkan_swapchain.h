#pragma once

#include "renderer/vulkan/vulkan_context.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanSwapchain {
    public:
        //     VulkanSwapchain(VulkanContext *context, u32 width, u32 height);
        //
        //     [[nodiscard]] StatusCode Recreate(u32 width, u32 height);
        //
        //     [[nodiscard]] VE_INLINE u32 Height() const noexcept {
        //         return mHeight;
        //     }
        //
        //     [[nodiscard]] VE_INLINE u32 Width() const noexcept {
        //         return mWidth;
        //     }
        //
        //     [[nodiscard]] VE_INLINE VkSwapchainKHR Handle() const noexcept {
        //         return mVkSwapchain;
        //     }
        //
        // private:
        //     VulkanContext *pContext;
        //     VkSurfaceKHR mVkSurface;
        //     VkSwapchainKHR mVkSwapchain;
        //     std::array<VkImage, 2> mVkSwapchainImages;
        //     std::array<VkImageView, 2> mVkSwapchainImageViews;
        //
        //     std::array<VkSemaphore, 2> mVkAcquireImageSemaphores;
        //     std::array<VkSemaphore, 2> mVkRenderCompleteSemaphores;
        //
        //     u32 mWidth;
        //     u32 mHeight;
    };

} // namespace Vulkyrie
