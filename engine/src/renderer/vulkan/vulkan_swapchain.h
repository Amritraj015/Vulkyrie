#pragma once

#include "vlkypch.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/rhi/constants.h"
#include "renderer/rhi/formats.h"
#include "renderer/vulkan/vulkan_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    /** @brief The window's backbuffers, and the acquire/present handshake around them.
     *
     * Owns the swapchain, one image view per backbuffer, and the semaphores that order acquire, rendering and present.
     * A frame is `Acquire` -> render -> `Present`; either end reports that the swapchain no longer matches the surface,
     * and the caller answers with `Recreate` at a frame boundary. Non-copyable, movable, and destroyed against the
     * `VulkanContext` it was built from, which must outlive it. */
    class VulkanSwapchain final {
    public:
        /** @brief One acquired backbuffer and the semaphores a frame must wait on and signal.
         * `Index` is `kInvalidRendererIndex` when the acquire failed; nothing else in the struct is then valid. */
        struct AcquiredImage final {
            /** @brief The backbuffer to render into, owned by the swapchain. */
            VulkanImage Image{};

            /** @brief Signalled once the image is safe to write; the frame's first submission must wait on it. */
            VkSemaphore AcquireSemaphore{ VK_NULL_HANDLE };

            /** @brief To be signalled by the frame's last submission and handed back to `Present`. */
            VkSemaphore PresentSemaphore{ VK_NULL_HANDLE };

            /** @brief Position in the swapchain, to be passed back to `Present`. */
            u32 Index{ kInvalidRendererIndex };

            /** @brief The swapchain still presents but no longer matches the surface; recreate at the next frame boundary. */
            bool SubOptimal{ false };
        };

        /** @brief Constructs an empty swapchain that owns nothing; only assignment from `Create` makes it usable. */
        VulkanSwapchain() = default;

        VE_DELETE_COPY(VulkanSwapchain);

        /** @brief Takes over every handle from `other`, leaving it empty and safe to destroy.
         * @param other The swapchain to move from. */
        VulkanSwapchain(VulkanSwapchain &&other) noexcept;

        /** @brief Destroys what this object holds, then takes over every handle from `other`.
         * @param other The swapchain to move from.
         * @returns This swapchain. */
        VulkanSwapchain &operator=(VulkanSwapchain &&other) noexcept;

        /** @brief Waits for the device to go idle, then destroys the swapchain, its views and its semaphores. */
        ~VulkanSwapchain();

        /** @brief Height of the swapchain images in pixels.
         * @returns The height, 0 while the surface is unusable. */
        [[nodiscard]] VE_INLINE u32 Height() const noexcept {
            return mHeight;
        }

        /** @brief Width of the swapchain images in pixels.
         * @returns The width, 0 while the surface is unusable. */
        [[nodiscard]] VE_INLINE u32 Width() const noexcept {
            return mWidth;
        }

        /** @brief Format the backbuffers were created with.
         * @returns The format, `Format::Undefined` before the first successful `Recreate`. */
        [[nodiscard]] VE_INLINE Format ImageFormat() const noexcept {
            return mFormat;
        }

        /** @brief Number of backbuffers the driver handed out.
         * @returns The image count, 0 while the surface is unusable. */
        [[nodiscard]] VE_INLINE u32 ImageCount() const noexcept {
            return static_cast<u32>(mVkSwapchainImages.size());
        }

        /** @brief Builds the swapchain, its views and its semaphores, replacing any it already holds.
         * Replacing waits for the device to go idle first, so this must not be called with a frame in flight. Failure
         * leaves the object destructible but unusable: `Acquire` returns an invalid image until a later call succeeds.
         * @param extents Requested size, overridden by the surface whenever it reports one of its own.
         * @param vsync Whether to cap presentation to the display refresh rate.
         * @returns `StatusCode::Successful`, or the status code of the first step that failed. */
        [[nodiscard]] StatusCode Recreate(Extent2D extents, bool vsync);

        /** @brief Acquires the next backbuffer to render into.
         * @param timeoutNs How long to block for an available image.
         * @returns The acquired image, or one with `Index == kInvalidRendererIndex` when the caller must skip the frame and recreate. */
        [[nodiscard]] AcquiredImage Acquire(u64 timeoutNs = std::numeric_limits<u64>::max());

        /** @brief Queues an acquired backbuffer for presentation.
         * @param imageIndex `Index` from the matching `Acquire`.
         * @param renderFinishedSemaphore Signalled by the frame's last submission.
         * @returns False when the swapchain needs recreating; the frame is not presented. */
        [[nodiscard]] bool Present(u32 imageIndex, VkSemaphore renderFinishedSemaphore);

    private:
        /** @brief Records the owners an empty swapchain needs before `Recreate` can build against them.
         * @param context Owner of the device and surface.
         * @param allocator Host allocation callbacks. */
        VulkanSwapchain(VulkanContext *context, VulkanHostAllocator *allocator);

        /** @brief Builds a swapchain for the surface owned by `context`.
         * @param context Owner of the device and surface; must outlive the swapchain.
         * @param extents Requested size, overridden by the surface whenever it reports one of its own.
         * @param vsync Whether to cap presentation to the display refresh rate.
         * @param allocator Host allocation callbacks, forwarded to every Vulkan object created here.
         * @returns The swapchain, or the status code of the first step that failed. */
        [[nodiscard]] static std::expected<VulkanSwapchain, StatusCode>
        Create(VulkanContext *context, Extent2D extents, bool vsync, VulkanHostAllocator *allocator);

        /** @brief The backbuffers and the view each one is rendered through. The images belong to the driver, the views to this class. */
        RendererVector<VulkanImage> mVkSwapchainImages;

        /** @brief Handed out in turn on `mSemaphoreCursor`, one deeper than the image count. */
        RendererVector<VkSemaphore> mVkAcquireSemaphores;

        /** @brief Indexed by image index, because a present may still be pending on any image. */
        RendererVector<VkSemaphore> mVkPresentSemaphores;

        /** @brief Owner of the device and surface. Null marks a moved-from object, which destroys nothing. */
        VulkanContext *pContext{ nullptr };

        /** @brief Host allocation callbacks, passed to every Vulkan object this class creates and destroys. */
        VulkanHostAllocator *pHostAllocator{ nullptr };

        /** @brief The swapchain itself, null until the first successful `Recreate`. */
        VkSwapchainKHR mVkSwapchain{ VK_NULL_HANDLE };

        /** @brief Width the backbuffers were built at, which the surface may have chosen instead of the caller. */
        u32 mWidth{ 0 };

        /** @brief Height the backbuffers were built at, which the surface may have chosen instead of the caller. */
        u32 mHeight{ 0 };

        /** @brief Next acquire semaphore to hand out, advanced only when an acquire actually signalled one. */
        u32 mSemaphoreCursor{ 0 };

        /** @brief Format the backbuffers were created with. */
        Format mFormat{ Format::Undefined };

        /** @brief Whether presentation is capped to the display refresh rate. */
        bool mVSync{ false };

        /** @brief Waits for the device to go idle, then releases everything this object owns. */
        void destroySwapchain();

        /** @brief Destroys the backbuffer views and empties `mVkSwapchainImages`. The device must be idle first. */
        void destroyImageViews();

        /** @brief Destroys both semaphore sets and empties them. The device must be idle first. */
        void destroySemaphores();
    };

} // namespace Vulkyrie
