#include "renderer/vulkan/vulkan_pipeline_builder.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"

namespace Vulkyrie {

    namespace {

        constexpr inline std::array<VkDynamicState, 2> kDynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        [[nodiscard]] VkShaderStageFlagBits ToVkStage(ShaderStage stage) noexcept {
            switch (stage) {
                case ShaderStage::Vertex:
                    return VK_SHADER_STAGE_VERTEX_BIT;
                case ShaderStage::TessellationControl:
                    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case ShaderStage::TessellationEvaluation:
                    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Geometry:
                    return VK_SHADER_STAGE_GEOMETRY_BIT;
                case ShaderStage::Fragment:
                    return VK_SHADER_STAGE_FRAGMENT_BIT;
                case ShaderStage::Compute:
                    return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::Task:
                    return VK_SHADER_STAGE_TASK_BIT_EXT;
                case ShaderStage::Mesh:
                    return VK_SHADER_STAGE_MESH_BIT_EXT;
                case ShaderStage::RayTracing:
                    return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                default:
                    return VK_SHADER_STAGE_VERTEX_BIT;
            }

            return VK_SHADER_STAGE_VERTEX_BIT;
        }

        [[nodiscard]] VkColorComponentFlags ToVkWriteMask(u8 mask) noexcept {
            VkColorComponentFlags out = 0;

            if (mask & 0x1) out |= VK_COLOR_COMPONENT_R_BIT;
            if (mask & 0x2) out |= VK_COLOR_COMPONENT_G_BIT;
            if (mask & 0x4) out |= VK_COLOR_COMPONENT_B_BIT;
            if (mask & 0x8) out |= VK_COLOR_COMPONENT_A_BIT;

            return out;
        }

    } // namespace

    VulkanPipelineBuilder::VulkanPipelineBuilder(VulkanContext *context, VulkanHostAllocator *allocator) noexcept
        : pContext(context)
        , pHostAllocator(allocator) {
        mLayouts.reserve(50);
    }

    VulkanPipelineBuilder::~VulkanPipelineBuilder() {
        if (nullptr == pContext || VK_NULL_HANDLE == pContext->Device()) {
            return;
        }

        for (const LayoutEntry &entry : mLayouts) {
            if (VK_NULL_HANDLE != entry.Layout) {
                vkDestroyPipelineLayout(pContext->Device(), entry.Layout, pHostAllocator->Callbacks());
            }
        }

        mLayouts.clear();
    }

    VulkanPipeline VulkanPipelineBuilder::BuildGraphicsPipeline(const GraphicsPipelineDescriptor descriptor, std::span<const VulkanShaderModule> stages) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");

        return {};
    }

    VulkanPipeline VulkanPipelineBuilder::BuildComputePipeline(const ComputePipelineDescriptor descriptor, const VulkanShaderModule stage) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");
        VASSERT(stage.Valid(), "ShaderStage must be valid.");

        const std::expected<VkPipelineLayout, StatusCode> layout = GetOrCreateLayout(descriptor.PushConstantBytes);

        if (!layout.has_value()) {
            return {};
        }

        const VkComputePipelineCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .stage =
                VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = VK_NULL_HANDLE,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                    .module = stage.ModuleHandle,
                    .pName = "main",
                    .pSpecializationInfo = VK_NULL_HANDLE,
                },
            .layout = *layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        VulkanPipeline out{};

        const VkResult r = vkCreateComputePipelines(pContext->Device(), pContext->PipelineCache(), 1, &info, pHostAllocator->Callbacks(), &out.PipelineHandle);

        if (VK_SUCCESS != r) {
            return {};
        }

        out.LayoutHandle = *layout;
        out.IsCompute = true;

#if defined(VE_VK_ENABLE_VALIDATION)
        pContext->SetDebugName(descriptor.DebugName, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(out.PipelineHandle));
#endif

        return out;
    }

    std::expected<VkPipelineLayout, StatusCode> VulkanPipelineBuilder::GetOrCreateLayout(u32 pushConstantBytes) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");
        VASSERT(pContext->GetVulkanDeviceCapabilities().Limits.MaxPushConstantBytes >= pushConstantBytes,
                "Push constant block exceeds the device limit. The layout would fail to "
                "create and every pipeline keyed on it with it.");

        for (const LayoutEntry &entry : mLayouts) {
            if (entry.PushConstantBytes == pushConstantBytes) {
                return entry.Layout;
            }
        }

        const VkPushConstantRange range{
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = pushConstantBytes,
        };

        const VkDescriptorSetLayout descriptorSetLayout = pContext->Heap().Layout();

        VASSERT(VK_NULL_HANDLE != descriptorSetLayout, "descriptorSetLayout cannot be VK_NULL_HANDLE.");

        VkPipelineLayoutCreateInfo layoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &descriptorSetLayout,
            .pushConstantRangeCount = pushConstantBytes > 0 ? 1u : 0u,
            .pPushConstantRanges = pushConstantBytes > 0 ? &range : VK_NULL_HANDLE,
        };

        VkPipelineLayout layout = VK_NULL_HANDLE;

        VE_VK_EXPECT(vkCreatePipelineLayout(pContext->Device(), &layoutCreateInfo, pHostAllocator->Callbacks(), &layout),
                     StatusCode::FailedToCreateVulkanPipelineLayout);

#if defined(VE_VK_ENABLE_VALIDATION)
        pContext->SetDebugName("PipelineLayout", VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<u64>(layout));
#endif

        mLayouts.emplace_back(layout, pushConstantBytes);

        return layout;
    }

} // namespace Vulkyrie
