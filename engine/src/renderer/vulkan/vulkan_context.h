#pragma once

#include "renderer/rhi/barrier_types.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/vulkan/vulkan_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext final {
    public:
        explicit VulkanContext(const DeviceCreationInfo &info);

        ~VulkanContext();

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
        const DeviceCapabilities &QueryCapabilities() const;
        bool DeviceLost() const;

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContextCreated;
        }

    private:
        VulkanHostAllocator mHostAllocator;
        DeviceCreationInfo mDeviceCreationInfo;
        ValidationConfig mValidationConfig;
        DeviceCapabilities mCapabilities;

        VkInstance mVkInstance;
        VkSurfaceKHR mVkSurface;
        VkPhysicalDevice mVkPhysicalDevice;
        VkDevice mVkDevice;
        bool mContextCreated;

        StatusCode selectSuitablePhysicalDevice();
    };

} // namespace Vulkyrie
