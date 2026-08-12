#pragma once

#include "renderer/backends/vulkan/vulkan_types.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class VulkanContext {
    public:
        VulkanImage CreateImage(const ImageDescriptor &descriptor);
        VulkanBuffer CreateBuffer(const BufferDescriptor &descriptor);
        VulkanSampler CreateSampler(const SamplerDescriptor &descriptor);
        VulkanShaderModule CreateShaderModule(const ShaderBlob &blob);
        VulkanPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor);
        VulkanPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor);

        bool DestroyImage(VulkanImage image);
        bool DestroyBuffer(VulkanBuffer buffer);
        bool DestroySampler(VulkanSampler sampler);
        bool DestroyShaderModule(VulkanShaderModule shaderModule);
        bool CreatePipeline(VulkanPipeline pipeline);

        void WaitIdle() const;
        const DeviceCapabilities &QueryCapabilities() const;
        bool DeviceLost() const;

    private:
    };

} // namespace Vulkyrie
