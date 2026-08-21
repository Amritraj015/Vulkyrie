#pragma once

#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/vulkan/vulkan_types.h"

namespace Vulkyrie {

    class VulkanContext final {
    public:
        explicit VulkanContext(const DeviceCreationInfo &info);

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

        void WaitIdle() const;
        const DeviceCapabilities &QueryCapabilities() const;
        bool DeviceLost() const;

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContextCreated;
        }

    private:
        DeviceCapabilities mCapabilities;
        bool mContextCreated;
    };

} // namespace Vulkyrie
