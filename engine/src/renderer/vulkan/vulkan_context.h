#pragma once

#include "renderer/rhi/barrier_types.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_types.h"
#include <volk.h>
#include <vk_mem_alloc.h>

namespace Vulkyrie {

    struct FrameResources {
        VkCommandPool CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        VkSemaphore ImageAcquiredSemaphore = VK_NULL_HANDLE;
    };

    class VulkanContext final {
    public:
        explicit VulkanContext(const DeviceCreationInfo &info);

        ~VulkanContext();

        [[nodiscard]] VE_INLINE const DeviceCapabilities &QueryCapabilities() const {
            return mCapabilities.ToDeviceCapabilities();
        }

        StatusCode Initialize();

        VulkanImage CreateImage(const TextureDescriptor &descriptor);
        VulkanBuffer CreateBuffer(const BufferDescriptor &descriptor);
        VulkanSampler CreateSampler(const SamplerDescriptor &descriptor);
        VulkanShaderModule CreateShaderModule(const ShaderBlob &blob);
        VulkanPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor);
        VulkanPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor);

        void DestroyImage(VulkanImage image);
        void DestroyBuffer(VulkanBuffer buffer);
        void DestroySampler(VulkanSampler sampler);
        void DestroyShaderModule(VulkanShaderModule shaderModule);
        void DestroyPipeline(VulkanPipeline pipeline);

        /** @brief Returns what an image of this shape costs, straight from the driver.
         *
         * `vkGetDeviceImageMemoryRequirements` (Vulkan 1.3 / VK_KHR_maintenance4) answers this from a
         * `VkImageCreateInfo` without creating an image, which is what makes it usable while the frame graph is
         * still planning. It is also the only sound source: tiling, mip-tail packing and alignment are the
         * driver's, and a packer fed a CPU-side guess places resources overlapping.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] ResourceMemoryRequirements GetImageMemoryRequirements(const TextureDescriptor &descriptor) const;

        /** @brief Returns what a buffer of this shape costs, via `vkGetDeviceBufferMemoryRequirements`.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] ResourceMemoryRequirements GetBufferMemoryRequirements(const BufferDescriptor &descriptor) const;

        void WaitIdle() const;
        bool DeviceLost() const;

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContextCreated;
        }

        [[nodiscard]] VE_INLINE VulkanQueue &GraphicsQueue() noexcept {
            return mGraphicsQueue;
        }

        [[nodiscard]] VE_INLINE VulkanQueue &TransferQueue() noexcept {
            return mTransferQueue;
        }

        [[nodiscard]] VE_INLINE VulkanQueue &ComputeQueue() noexcept {
            return mComputeQueue;
        }

        // TODO: remove this.
        void test();

    private:
        constexpr static VkFormat SWAPCHAIN_FORMAT{ VK_FORMAT_B8G8R8A8_SRGB };
        constexpr static VkFormat DEPTH_FORMAT{ VK_FORMAT_D32_SFLOAT };

        VulkanHostAllocator mHostAllocator;
        VulkanDeviceCapabilities mCapabilities;
        DeviceCreationInfo mDeviceCreationInfo;
        ValidationConfig mValidationConfig;

        VkInstance mVkInstance;
        VkSurfaceKHR mVkSurface;
        VkPhysicalDevice mVkPhysicalDevice;
        VkDevice mVkDevice;
        VmaAllocator mVmaAllocator;

        // ------------------------------------
        // TODO: Move to VulkanSwapchain class.
        VkSwapchainKHR mVkSwapchain;
        RendererVector<VkImage> mVkSwapchainImages;
        RendererVector<VkImageView> mVkSwapchainImageViews;
        RendererVector<VkSemaphore> mVkRenderCompleteSemaphores;
        VkImage mVkDepthImage;
        VkImageView mVkDepthImageView;
        VmaAllocation mVmaDepthImageAllocation;

        // TODO: Shader modules.
        VkShaderModule mVkVertexShaderModule;
        VkShaderModule mVkFragmentShaderModule;
        VkPipelineLayout mVkPipelineLayout;
        VkPipeline mVkGraphicsPipeline;
        VkSemaphore mVkTimelineSemaphore;
        std::array<FrameResources, 2> mFrameResources; // TODO: Change the size of this array to Backend::kFramesInFlight

        u64 frameIndex = 0;
        u32 MaxFramesInFlight = 2;
        u32 nextSignalValue = MaxFramesInFlight + 1;
        bool requireSwapchainRecreate = false;
        // ------------------------------------

        VulkanQueue mGraphicsQueue;
        VulkanQueue mTransferQueue;
        VulkanQueue mComputeQueue;
        VulkanQueue mPresentQueue;

        bool mContextCreated;

        StatusCode createInstance();
        StatusCode createSurface();
        StatusCode selectSuitablePhysicalDevice();
        StatusCode createLogicalDevice();
        StatusCode initializeVulkanMemoryAllocator();
        StatusCode createSwapchain();
        StatusCode createShaders();
        StatusCode createSynchronizationResources();
        StatusCode createCommandBuffers();

        void destroySwapchain();
    };

} // namespace Vulkyrie
