#pragma once

#include "vlkypch.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/vulkan/vulkan_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    class VulkanPipelineBuilder final {
    public:
        VulkanPipelineBuilder() = default;

        VE_DELETE_MOVE_AND_COPY(VulkanPipelineBuilder);

        explicit VulkanPipelineBuilder(VulkanContext *context, VulkanHostAllocator *allocator) noexcept;

        ~VulkanPipelineBuilder();

        [[nodiscard]] VulkanPipeline BuildGraphicsPipeline(const GraphicsPipelineDescriptor descriptor, std::span<const VulkanShaderModule> stages);
        [[nodiscard]] VulkanPipeline BuildComputePipeline(const ComputePipelineDescriptor descriptor, const VulkanShaderModule stage);
        [[nodiscard]] std::expected<VkPipelineLayout, StatusCode> GetOrCreateLayout(u32 pushConstantBytes);

    private:
        struct LayoutEntry final {
            VkPipelineLayout Layout{ VK_NULL_HANDLE };
            u32 PushConstantBytes{ 0 };

            LayoutEntry(VkPipelineLayout layout, u32 pushConstantBytes)
                : Layout(layout)
                , PushConstantBytes(pushConstantBytes) {
            }
        };

        VulkanContext *pContext{ nullptr };
        VulkanHostAllocator *pHostAllocator{ nullptr };
        RendererVector<LayoutEntry> mLayouts;
    };

} // namespace Vulkyrie
