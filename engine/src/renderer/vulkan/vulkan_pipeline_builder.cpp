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

        const ShaderKey *keys[] = {
            &descriptor.VertexShader,
            &descriptor.FragmentShader,
            &descriptor.MeshShader,
            &descriptor.TaskShader,
        };

        VkPipelineShaderStageCreateInfo stageInfos[std::size(keys)]{};
        u32 stageCount = 0;

        for (const ShaderKey *key : keys) {
            if (!key->Valid()) {
                continue;
            }

            VASSERT(stageCount < stages.size(), "Fewer modules than valid shader keys; see the contract on BuildGraphicsPipeline.");
            VASSERT(stages[stageCount].Valid(), "ShaderStage invalid.");

            stageInfos[stageCount] = VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .flags = 0,
                .stage = ToVkStage(key->ShaderStage),
                .module = stages[stageCount].ModuleHandle,
                .pName = "main",
                .pSpecializationInfo = VK_NULL_HANDLE,
            };

            stageCount++;
        }

        VASSERT(stageCount == stages.size(), "More modules than valid shader keys; see the contract on BuildGraphics.");
        VASSERT(stageCount > 0, "stageCount must be greater 0");

        const VkPipelineVertexInputStateCreateInfo vertexInput{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = VK_NULL_HANDLE,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = VK_NULL_HANDLE,
        };

        const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .topology = static_cast<VkPrimitiveTopology>(ToVkTopology(descriptor.Topology)),
            .primitiveRestartEnable = VK_FALSE,
        };

        const VkPipelineViewportStateCreateInfo viewport{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = VK_NULL_HANDLE,
            .scissorCount = 1,
            .pScissors = VK_NULL_HANDLE,
        };

        const VkPipelineRasterizationStateCreateInfo raster{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .depthClampEnable = descriptor.Raster.DepthClamp ? VK_TRUE : VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = descriptor.Raster.FillMode == PolygonFillMode::Line ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
            .cullMode = static_cast<VkCullModeFlags>(ToVkCullMode(descriptor.Raster.Cull)),
            .frontFace = descriptor.Raster.FrontFace == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = descriptor.Raster.DepthBiasEnabled ? VK_TRUE : VK_FALSE,
            .depthBiasConstantFactor = descriptor.Raster.DepthBiasConstant,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = descriptor.Raster.DepthBiasSlope,
            .lineWidth = 1.0f,
        };

        const VkPipelineMultisampleStateCreateInfo multisample{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .rasterizationSamples = static_cast<VkSampleCountFlagBits>(ToVkSampleCount(descriptor.RenderTargetLayout.Samples)),
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = VK_NULL_HANDLE,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        const bool hasDepth = descriptor.RenderTargetLayout.DepthFormat != Format::Undefined;
        const VkPipelineDepthStencilStateCreateInfo depthStencil{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            // A depth test with no depth attachment is a validation error, so the
            // layout has the final say over what the desc asked for.
            .depthTestEnable = (hasDepth && descriptor.DepthStencil.DepthTest) ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = (hasDepth && descriptor.DepthStencil.DepthWrite) ? VK_TRUE : VK_FALSE,
            .depthCompareOp = static_cast<VkCompareOp>(ToVkCompareOp(descriptor.DepthStencil.DepthCompare)),
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = descriptor.DepthStencil.StencilTest ? VK_TRUE : VK_FALSE,
            .front = VkStencilOpState{},
            .back = VkStencilOpState{},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };

        VASSERT(descriptor.RenderTargetLayout.ColorCount <= kMaxColorAttachments, "Color attachments must be <= kMaxColorAttachments");
        VkPipelineColorBlendAttachmentState blends[kMaxColorAttachments]{};
        VkFormat colorFormats[kMaxColorAttachments]{};

        for (u32 i = 0; i < descriptor.RenderTargetLayout.ColorCount; ++i) {
            const BlendState &b = descriptor.Blends[i];
            blends[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable = b.Enable ? VK_TRUE : VK_FALSE,
                .srcColorBlendFactor = static_cast<VkBlendFactor>(ToVkBlendFactor(b.SrcColor)),
                .dstColorBlendFactor = static_cast<VkBlendFactor>(ToVkBlendFactor(b.DstColor)),
                .colorBlendOp = static_cast<VkBlendOp>(ToVkBlendOp(b.ColorOp)),
                .srcAlphaBlendFactor = static_cast<VkBlendFactor>(ToVkBlendFactor(b.SrcAlpha)),
                .dstAlphaBlendFactor = static_cast<VkBlendFactor>(ToVkBlendFactor(b.DstAlpha)),
                .alphaBlendOp = static_cast<VkBlendOp>(ToVkBlendOp(b.AlphaOp)),
                .colorWriteMask = ToVkWriteMask(b.WriteMask),
            };
            colorFormats[i] = FromVulkyrieToVulkanFormat(descriptor.RenderTargetLayout.ColorFormats[i]);
        }

        const VkPipelineColorBlendStateCreateInfo colorBlend{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = descriptor.RenderTargetLayout.ColorCount,
            .pAttachments = blends,
            .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
        };

        const VkPipelineDynamicStateCreateInfo dynamic{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .dynamicStateCount = static_cast<u32>(kDynamicStates.size()),
            .pDynamicStates = kDynamicStates.data(),
        };

        // Dynamic rendering: the attachment formats live here instead of in a
        // VkRenderPass, which is why RenderTargetLayout is part of the cache key.
        const VkFormat depthFormat = hasDepth ? FromVulkyrieToVulkanFormat(descriptor.RenderTargetLayout.DepthFormat) : VK_FORMAT_UNDEFINED;
        const VkPipelineRenderingCreateInfo rendering{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .viewMask = 0,
            .colorAttachmentCount = descriptor.RenderTargetLayout.ColorCount,
            .pColorAttachmentFormats = colorFormats,
            .depthAttachmentFormat = depthFormat,
            // Only when the format actually carries stencil; naming a stencil
            // format the attachment does not have fails pipeline creation.
            .stencilAttachmentFormat = IsStencilFormat(descriptor.RenderTargetLayout.DepthFormat) ? depthFormat : VK_FORMAT_UNDEFINED,
        };

        const std::expected<VkPipelineLayout, StatusCode> layout = GetOrCreateLayout(descriptor.PushConstantBytes);

        if (!layout.has_value()) {
            return {};
        }

        const VkGraphicsPipelineCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering,
            .flags = 0,
            .stageCount = stageCount,
            .pStages = stageInfos,
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pTessellationState = nullptr,
            .pViewportState = &viewport,
            .pRasterizationState = &raster,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlend,
            .pDynamicState = &dynamic,
            .layout = *layout,
            // No render pass: that is what VkPipelineRenderingCreateInfo replaces.
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        VulkanPipeline out{};

        const VkResult r = vkCreateGraphicsPipelines(pContext->Device(), pContext->PipelineCache(), 1, &info, nullptr, &out.PipelineHandle);

        if (r != VK_SUCCESS) {
            return {};
        }

        out.LayoutHandle = *layout;
        out.IsCompute = false;

#if defined(VE_VK_ENABLE_VALIDATION)
        pContext->SetDebugName(descriptor.DebugName, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(out.PipelineHandle));
#endif

        return out;
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
