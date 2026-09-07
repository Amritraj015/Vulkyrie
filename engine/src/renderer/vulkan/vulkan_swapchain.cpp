#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"

namespace Vulkyrie {

    namespace {

        constexpr inline std::array<VkFormat, 2> kPreferredSurfaceFormats{
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
        };

        [[nodiscard]] std::expected<VkSurfaceFormatKHR, StatusCode> TryChooseSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
            u32 surfaceFormatCount = 0;
            VE_VK_EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr),
                         StatusCode::FailedToQueryPhysicalDeviceSurfaceFormats);

            RendererVector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            VE_VK_EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()),
                         StatusCode::FailedToQueryPhysicalDeviceSurfaceFormats);

            for (const VkFormat preferredFormat : kPreferredSurfaceFormats) {
                for (const VkSurfaceFormatKHR &availableFormat : surfaceFormats) {
                    if (availableFormat.format == preferredFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }
            }

            VERROR("Surface does not support the required swapchain format/color space combination.");

            return std::unexpected(StatusCode::RequiredSwapchainSurfaceFormatNotSupported);
        }

        [[nodiscard]] std::expected<VkPresentModeKHR, StatusCode> TryChoosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool vsync) {
            if (vsync) {
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            u32 count = 0;
            VE_VK_EXPECT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr),
                         StatusCode::FailedToQueryPhysicalDevicePresentModes);

            RendererVector<VkPresentModeKHR> presentModes(count);
            VE_VK_EXPECT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModes.data()),
                         StatusCode::FailedToQueryPhysicalDevicePresentModes);

            bool hasMailbox = false;
            bool hasImmediate = false;

            for (const VkPresentModeKHR mode : presentModes) {
                if (VK_PRESENT_MODE_MAILBOX_KHR == mode) hasMailbox = true;
                if (VK_PRESENT_MODE_IMMEDIATE_KHR == mode) hasImmediate = true;
            }

            if (hasMailbox) return VK_PRESENT_MODE_MAILBOX_KHR;
            if (hasImmediate) return VK_PRESENT_MODE_IMMEDIATE_KHR;

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        [[nodiscard]] VkSemaphore TryCreateBinarySemaphore(VkDevice device, VulkanHostAllocator *allocator) {
            const VkSemaphoreCreateInfo semaphoreCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .flags = 0,
            };

            VkSemaphore semaphore = VK_NULL_HANDLE;

            vkCreateSemaphore(device, &semaphoreCreateInfo, allocator->Callbacks(), &semaphore);

            return semaphore;
        }

    } // namespace

    VulkanSwapchain::VulkanSwapchain(VulkanContext *context, VulkanHostAllocator *allocator)
        : pContext(context)
        , pHostAllocator(allocator) {
    }

    VulkanSwapchain::VulkanSwapchain(VulkanSwapchain &&other) noexcept
        : mVkSwapchainImages(std::move(other.mVkSwapchainImages))
        , mVkAcquireSemaphores(std::move(other.mVkAcquireSemaphores))
        , mVkPresentSemaphores(std::move(other.mVkPresentSemaphores))
        , pContext(other.pContext)
        , pHostAllocator(other.pHostAllocator)
        , mVkSwapchain(other.mVkSwapchain)
        , mWidth(other.mWidth)
        , mHeight(other.mHeight)
        , mSemaphoreCursor(other.mSemaphoreCursor)
        , mFormat(other.mFormat)
        , mVSync(other.mVSync) {

        other.pContext = nullptr;
        other.mVkSwapchain = VK_NULL_HANDLE;
    }

    VulkanSwapchain &VulkanSwapchain::operator=(VulkanSwapchain &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        // Since another swapchain is being move assigned to this object,
        // we need to make sure that the "this" swapchain is destroyed
        // before the "other" swapchain is moved to "this" one.
        destroySwapchain();

        mVkSwapchainImages = std::move(other.mVkSwapchainImages);
        mVkAcquireSemaphores = std::move(other.mVkAcquireSemaphores);
        mVkPresentSemaphores = std::move(other.mVkPresentSemaphores);
        pContext = other.pContext;
        pHostAllocator = other.pHostAllocator;
        mVkSwapchain = other.mVkSwapchain;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mSemaphoreCursor = other.mSemaphoreCursor;
        mFormat = other.mFormat;
        mVSync = other.mVSync;

        other.pContext = nullptr;
        other.mVkSwapchain = VK_NULL_HANDLE;

        return *this;
    }

    VulkanSwapchain::~VulkanSwapchain() {
        destroySwapchain();
    }

    std::expected<VulkanSwapchain, StatusCode> VulkanSwapchain::Create(VulkanContext *context, Extent2D extents, bool vsync, VulkanHostAllocator *allocator) {
        VulkanSwapchain swapchain{ context, allocator };

        if (const StatusCode code = swapchain.Recreate(extents, vsync); StatusCode::Successful != code) {
            return std::unexpected(code);
        }

        return swapchain;
    }

    StatusCode VulkanSwapchain::Recreate(Extent2D extents, bool vsync) {
        const VkPhysicalDevice physicalDevice = pContext->PhysicalDevice();
        const VkSurfaceKHR surface = pContext->Surface();

        // Try to query surface capabilities.
        VkSurfaceCapabilitiesKHR capabilities{};
        VE_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities),
                    StatusCode::FailedToQueryPhysicalDeviceSurfaceCapabilities);

        VkExtent2D imageExtents = { .width = extents.Width, .height = extents.Height };
        if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
            imageExtents = capabilities.currentExtent;
        } else {
            imageExtents.width = std::clamp(extents.Width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            imageExtents.height = std::clamp(extents.Height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        // The resolved extent, not the caller's: a minimized window reports 0x0
        // while the caller still holds the last non-zero size.
        if (0 == imageExtents.width || 0 == imageExtents.height) {
            mWidth = 0;
            mHeight = 0;

            return StatusCode::InvalidSizeForVulkanSwapchain;
        }

        // Try to figure out the surface image format and present mode to use.
        const std::expected<VkSurfaceFormatKHR, StatusCode> surfaceFormat = TryChooseSurfaceFormat(physicalDevice, surface);
        const std::expected<VkPresentModeKHR, StatusCode> presentMode = TryChoosePresentMode(physicalDevice, surface, vsync);

        if (!surfaceFormat.has_value()) {
            return surfaceFormat.error();
        } else if (!presentMode.has_value()) {
            return presentMode.error();
        }

        u32 imageCount = capabilities.minImageCount + 1;

        if (0 != capabilities.maxImageCount) {
            imageCount = std::min(imageCount, capabilities.maxImageCount);
        }

        // STORAGE alongside COLOR_ATTACHMENT so a compute pass can write the
        // backbuffer directly. TRANSFER_DST for a blit-to-screen path, and
        // TRANSFER_SRC so the frame can be read back for a screenshot.
        constexpr VkImageUsageFlags kRequiredImageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

        // Only COLOR_ATTACHMENT is guaranteed; creation is invalid if any other is missing.
        if (kRequiredImageUsage != (capabilities.supportedUsageFlags & kRequiredImageUsage)) {
            VERROR("Surface does not support the required swapchain image usage flags.");

            return StatusCode::RequiredSwapchainImageUsageNotSupported;
        }

        if (0 == (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
            VERROR("Surface does not support opaque composite alpha.");

            return StatusCode::RequiredSwapchainCompositeAlphaNotSupported;
        }

        VkSwapchainKHR oldSwapchain = mVkSwapchain;

        // New swapchain creation info.
        const VkSwapchainCreateInfoKHR swapchainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat->format,
            .imageColorSpace = surfaceFormat->colorSpace,
            .imageExtent = imageExtents,
            .imageArrayLayers = 1,
            .imageUsage = kRequiredImageUsage,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = VK_NULL_HANDLE,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = *presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = oldSwapchain,
        };

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;

        // Try to create the new swapchain.
        const VkResult result = vkCreateSwapchainKHR(pContext->Device(), &swapchainCreateInfo, pHostAllocator->Callbacks(), &newSwapchain);

        if (VK_SUCCESS != result) {
            pContext->MarkDeviceLost();

            return StatusCode::FailedToCreateVulkanSwapchain;
        }

        mVkSwapchain = newSwapchain;
        mFormat = FromVulkanToVulkyrieFormat(surfaceFormat->format);
        mWidth = imageExtents.width;
        mHeight = imageExtents.height;
        mVSync = vsync;

        // In-flight submissions and the presentation engine can still reference the
        // retired views and semaphores.
        if (VK_NULL_HANDLE != oldSwapchain) {
            pContext->WaitIdle();

            destroyImageViews();
            destroySemaphores();

            vkDestroySwapchainKHR(pContext->Device(), oldSwapchain, pHostAllocator->Callbacks());
        }

        // Try to get swapchain images.
        u32 swapchainImageCount = 0;
        VE_VK_CHECK(vkGetSwapchainImagesKHR(pContext->Device(), newSwapchain, &swapchainImageCount, nullptr), StatusCode::FailedToGetVulkanSwapchainImages);
        RendererVector<VkImage> rawImages(swapchainImageCount);
        VE_VK_CHECK(vkGetSwapchainImagesKHR(pContext->Device(), newSwapchain, &swapchainImageCount, rawImages.data()),
                    StatusCode::FailedToGetVulkanSwapchainImages);

        mVkSwapchainImages.resize(swapchainImageCount);

        // Re-create new image views.
        for (usize i = 0; i < mVkSwapchainImages.size(); ++i) {
            const VkImageViewCreateInfo imageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .flags = 0,
                .image = rawImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surfaceFormat->format,
                .components =
                    VkComponentMapping{
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            VkImageView imageView = VK_NULL_HANDLE;

            VE_VK_CHECK(vkCreateImageView(pContext->Device(), &imageViewCreateInfo, pHostAllocator->Callbacks(), &imageView),
                        StatusCode::FailedToCreateVulkanSwapchainImageView);

            mVkSwapchainImages[i] = VulkanImage{
                .ImageHandle = rawImages[i],
                .ImageViewHandle = imageView,
                .ImageAllocation = VK_NULL_HANDLE,
                .Format = surfaceFormat->format,
                .Width = imageExtents.width,
                .Height = imageExtents.height,
                .Depth = 1,
                .BindlessIndex = kInvalidRendererIndex,
                .Mips = 1,
                .Layers = 1,
            };
        }

        // Present semaphores are indexed BY IMAGE INDEX: the wait belongs to the
        // image being presented, and a present may still be pending on any image
        // the driver has not reclaimed.
        //
        // Acquire semaphores are NOT, and this is the asymmetry worth remembering:
        // vkAcquireNextImageKHR needs the semaphore BEFORE it returns the index, so
        // indexing them by image is impossible. They cycle on their own cursor,
        // sized one deeper than the image count so a cursor wrap can never reuse a
        // semaphore whose acquire has not yet been waited on.
        //
        // Both sets are rebuilt on every recreate, never reused on a matching count:
        // a dropped frame leaves its acquire semaphore signalled forever, and
        // vkAcquireNextImageKHR requires an un-signalled one.
        mVkPresentSemaphores.reserve(swapchainImageCount);

        for (u32 i = 0; i < swapchainImageCount; ++i) {
            const VkSemaphore semaphore = TryCreateBinarySemaphore(pContext->Device(), pHostAllocator);

            if (VK_NULL_HANDLE == semaphore) {
                return StatusCode::FailedToCreateVulkanPresentSemaphore;
            }

            mVkPresentSemaphores.push_back(semaphore);
        }

        const u32 acquireCount = swapchainImageCount + 1;
        mVkAcquireSemaphores.reserve(acquireCount);

        for (u32 i = 0; i < acquireCount; ++i) {
            const VkSemaphore semaphore = TryCreateBinarySemaphore(pContext->Device(), pHostAllocator);

            if (VK_NULL_HANDLE == semaphore) {
                return StatusCode::FailedToCreateVulkanAcquireSwapchainImageSemaphore;
            }

            mVkAcquireSemaphores.push_back(semaphore);
        }

        mSemaphoreCursor = 0;

        return StatusCode::Successful;
    }

    VulkanSwapchain::AcquiredImage VulkanSwapchain::Acquire(u64 timeoutNs) {
        // A failed Recreate leaves neither swapchain nor semaphores.
        if (VK_NULL_HANDLE == mVkSwapchain || mVkAcquireSemaphores.empty()) {
            return AcquiredImage{};
        }

        const VkSemaphore acquireSemaphore = mVkAcquireSemaphores[mSemaphoreCursor];
        u32 index = 0;

        const VkResult result = vkAcquireNextImageKHR(pContext->Device(), mVkSwapchain, timeoutNs, acquireSemaphore, VK_NULL_HANDLE, &index);

        // Suboptimal still hands back a usable image; the caller decides when to recreate.
        if (VK_SUCCESS == result || VK_SUBOPTIMAL_KHR == result) {
            mSemaphoreCursor = (mSemaphoreCursor + 1) % static_cast<u32>(mVkAcquireSemaphores.size());

            return AcquiredImage{
                .Image = mVkSwapchainImages[index],
                .AcquireSemaphore = acquireSemaphore,
                .PresentSemaphore = mVkPresentSemaphores[index],
                .Index = index,
                .SubOptimal = (VK_SUBOPTIMAL_KHR == result),
            };
        }

        // The cursor stays put below: nothing signalled the semaphore, so it is still
        // pristine. Advancing here is the leak that shows up as a hang after enough
        // resizes, the next acquire waiting on a semaphore nothing will ever signal.
        if (VK_ERROR_OUT_OF_DATE_KHR != result && VK_TIMEOUT != result && VK_NOT_READY != result) {
            pContext->MarkDeviceLost();
        }

        return AcquiredImage{};
    }

    bool VulkanSwapchain::Present(u32 imageIndex, VkSemaphore renderFinishedSemaphore) {
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &mVkSwapchain,
            .pImageIndices = &imageIndex,
            .pResults = VK_NULL_HANDLE,
        };

        const VkResult result = vkQueuePresentKHR(pContext->GraphicsQueue().Handle(), &presentInfo);

        if (VK_SUCCESS == result) return true;
        if (VK_SUBOPTIMAL_KHR == result || VK_ERROR_OUT_OF_DATE_KHR == result) return false;

        pContext->MarkDeviceLost();

        return false;
    }

    void VulkanSwapchain::destroySwapchain() {
        if (nullptr == pContext || VK_NULL_HANDLE == pContext->Device()) {
            return;
        }

        pContext->WaitIdle();

        destroyImageViews();
        destroySemaphores();

        if (VK_NULL_HANDLE != mVkSwapchain) {
            vkDestroySwapchainKHR(pContext->Device(), mVkSwapchain, pHostAllocator->Callbacks());
        }

        mVkSwapchain = VK_NULL_HANDLE;
    }

    void VulkanSwapchain::destroyImageViews() {
        for (const VulkanImage &image : mVkSwapchainImages) {
            if (VK_NULL_HANDLE != image.ImageViewHandle) {
                vkDestroyImageView(pContext->Device(), image.ImageViewHandle, pHostAllocator->Callbacks());
            }
        }

        mVkSwapchainImages.clear();
    }

    void VulkanSwapchain::destroySemaphores() {
        for (const VkSemaphore semaphore : mVkAcquireSemaphores) {
            if (VK_NULL_HANDLE != semaphore) {
                vkDestroySemaphore(pContext->Device(), semaphore, pHostAllocator->Callbacks());
            }
        }

        for (const VkSemaphore semaphore : mVkPresentSemaphores) {
            if (VK_NULL_HANDLE != semaphore) {
                vkDestroySemaphore(pContext->Device(), semaphore, pHostAllocator->Callbacks());
            }
        }

        mVkAcquireSemaphores.clear();
        mVkPresentSemaphores.clear();
    }

} // namespace Vulkyrie
