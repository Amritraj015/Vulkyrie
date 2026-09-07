#include "renderer/vulkan/vulkan_pipeline_builder.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"

namespace Vulkyrie {

    namespace {

        // The state every pipeline this builder produces leaves to the command buffer.
        // Anything named here is ignored in the corresponding create-info field.
        constexpr std::array<VkDynamicState, 3> kDynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        };

        /** @brief Maps a single RHI shader stage to the Vulkan stage bit.
         * @param stage One stage, not a combination.
         * @returns The matching bit; the vertex bit for anything that names no single stage. */
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
                // ShaderStage is a bitmask; a combination has no single bit to name.
                default:
                    return VK_SHADER_STAGE_VERTEX_BIT;
            }
        }

        /** @brief Expands an RGBA write mask into Vulkan colour component flags.
         * @param mask One bit per channel, red in the low bit.
         * @returns The matching component flags. */
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
        mLayouts.reserve(kExpectedLayoutCount);
    }

    VulkanPipelineBuilder::~VulkanPipelineBuilder() {
        Destroy();
    }

    VulkanPipeline VulkanPipelineBuilder::BuildGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor, std::span<const VulkanShaderModule> stages) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");

        // Declaration order is the contract: it decides which module in `stages` each filled key claims.
        struct StageSlot final {
            const ShaderKey *Key;
            ShaderStage Stage;
        };

        const StageSlot slots[] = {
            { &descriptor.TaskShader, ShaderStage::Task },
            { &descriptor.MeshShader, ShaderStage::Mesh },
            { &descriptor.VertexShader, ShaderStage::Vertex },
            { &descriptor.TessellationControlShader, ShaderStage::TessellationControl },
            { &descriptor.TessellationEvaluationShader, ShaderStage::TessellationEvaluation },
            { &descriptor.FragmentShader, ShaderStage::Fragment },
        };

        VkPipelineShaderStageCreateInfo stageInfos[std::size(slots)]{};
        u32 stageCount = 0;

        for (const StageSlot &slot : slots) {
            if (!slot.Key->Valid()) {
                continue;
            }

            VASSERT(slot.Key->ShaderStage == slot.Stage, "ShaderKey sits in a descriptor field that does not match its own stage.");

            if (stageCount >= stages.size() || !stages[stageCount].Valid()) {
                VERROR("Graphics pipeline: no valid shader module at index {}; modules must pair with the valid shader keys in slot order.", stageCount);
                return {};
            }

            stageInfos[stageCount] = VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .flags = 0,
                .stage = ToVkStage(slot.Stage),
                .module = stages[stageCount].ModuleHandle,
                .pName = "main",
                .pSpecializationInfo = VK_NULL_HANDLE,
            };

            stageCount++;
        }

        if (0 == stageCount || stageCount != stages.size()) {
            VERROR("Graphics pipeline: {} shader modules supplied for {} valid shader keys.", stages.size(), stageCount);
            return {};
        }

        const bool hasTessellation = descriptor.TessellationControlShader.Valid() || descriptor.TessellationEvaluationShader.Valid();

        if (hasTessellation && (!descriptor.TessellationControlShader.Valid() || !descriptor.TessellationEvaluationShader.Valid() ||
                                descriptor.PatchControlPoints == 0 || descriptor.Topology != PrimitiveTopology::PatchList)) {
            VERROR("Graphics pipeline: tessellation needs both tessellation stages, a non-zero PatchControlPoints and PatchList topology.");
            return {};
        }

        if (descriptor.RenderTargetLayout.ColorCount > kMaxColorAttachments) {
            VERROR("Graphics pipeline: {} color attachments exceeds the {} supported.", descriptor.RenderTargetLayout.ColorCount, kMaxColorAttachments);
            return {};
        }

        const VkPipelineVertexInputStateCreateInfo vertexInput{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            // Geometry is read through the descriptor heap, so there is nothing to bind.
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = VK_NULL_HANDLE,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = VK_NULL_HANDLE,
        };

        const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .topology = ToVkTopology(descriptor.Topology),
            .primitiveRestartEnable = VK_FALSE,
        };

        const VkPipelineViewportStateCreateInfo viewport{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            // Counts still have to be right; the values themselves arrive as dynamic state.
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
            .polygonMode = ToVkPolygonMode(descriptor.Raster.FillMode),
            .cullMode = ToVkCullMode(descriptor.Raster.Cull),
            .frontFace = ToVkFrontFace(descriptor.Raster.FrontFace),
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
            .rasterizationSamples = ToVkSampleCount(descriptor.RenderTargetLayout.Samples),
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = VK_NULL_HANDLE,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        const bool hasDepth = descriptor.RenderTargetLayout.DepthFormat != Format::Undefined;
        const bool hasStencil = IsStencilFormat(descriptor.RenderTargetLayout.DepthFormat);
        const VkPipelineDepthStencilStateCreateInfo depthStencil{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            // A depth test with no depth attachment is a validation error, so the
            // layout has the final say over what the desc asked for.
            .depthTestEnable = (hasDepth && descriptor.DepthStencil.DepthTest) ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = (hasDepth && descriptor.DepthStencil.DepthWrite) ? VK_TRUE : VK_FALSE,
            .depthCompareOp = ToVkCompareOp(descriptor.DepthStencil.DepthCompare),
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = (hasStencil && descriptor.DepthStencil.StencilTest) ? VK_TRUE : VK_FALSE,
            .front = VkStencilOpState{},
            .back = VkStencilOpState{},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };

        VkPipelineColorBlendAttachmentState blends[kMaxColorAttachments]{};
        VkFormat colorFormats[kMaxColorAttachments]{};

        for (u32 i = 0; i < descriptor.RenderTargetLayout.ColorCount; ++i) {
            const BlendState &b = descriptor.Blends[i];
            blends[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable = b.Enable ? VK_TRUE : VK_FALSE,
                .srcColorBlendFactor = ToVkBlendFactor(b.SrcColor),
                .dstColorBlendFactor = ToVkBlendFactor(b.DstColor),
                .colorBlendOp = ToVkBlendOp(b.ColorOp),
                .srcAlphaBlendFactor = ToVkBlendFactor(b.SrcAlpha),
                .dstAlphaBlendFactor = ToVkBlendFactor(b.DstAlpha),
                .alphaBlendOp = ToVkBlendOp(b.AlphaOp),
                .colorWriteMask = ToVkWriteMask(b.WriteMask),
            };
            colorFormats[i] = ToVkFormat(descriptor.RenderTargetLayout.ColorFormats[i]);
        }

        const VkPipelineTessellationStateCreateInfo tessellation{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .patchControlPoints = descriptor.PatchControlPoints,
        };

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
        const VkFormat depthFormat = hasDepth ? ToVkFormat(descriptor.RenderTargetLayout.DepthFormat) : VK_FORMAT_UNDEFINED;
        const VkPipelineRenderingCreateInfo rendering{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .viewMask = 0,
            .colorAttachmentCount = descriptor.RenderTargetLayout.ColorCount,
            .pColorAttachmentFormats = colorFormats,
            .depthAttachmentFormat = depthFormat,
            // Only when the format actually carries stencil; naming a stencil
            // format the attachment does not have fails pipeline creation.
            .stencilAttachmentFormat = hasStencil ? depthFormat : VK_FORMAT_UNDEFINED,
        };

        const std::expected<VkPipelineLayout, StatusCode> layout = getOrCreateLayout(descriptor.PushConstantBytes);

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
            .pTessellationState = hasTessellation ? &tessellation : nullptr,
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

        const VkResult r = vkCreateGraphicsPipelines(pContext->Device(), pContext->PipelineCache(), 1, &info, pHostAllocator->Callbacks(), &out.PipelineHandle);

        if (VK_SUCCESS != r) {
            VERROR("vkCreateGraphicsPipelines failed with Vulkan error code: {}", std::to_underlying(r));
            return {};
        }

        out.LayoutHandle = *layout;
        out.IsCompute = false;

#if defined(VE_VK_ENABLE_VALIDATION)
        pContext->SetDebugName(descriptor.DebugName, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(out.PipelineHandle));
#endif

        return out;
    }

    VulkanPipeline VulkanPipelineBuilder::BuildComputePipeline(const ComputePipelineDescriptor &descriptor, const VulkanShaderModule &stage) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");

        if (!stage.Valid()) {
            VERROR("Compute pipeline: shader module is null.");
            return {};
        }

        const std::expected<VkPipelineLayout, StatusCode> layout = getOrCreateLayout(descriptor.PushConstantBytes);

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
            VERROR("vkCreateComputePipelines failed with Vulkan error code: {}", std::to_underlying(r));
            return {};
        }

        out.LayoutHandle = *layout;
        out.IsCompute = true;

#if defined(VE_VK_ENABLE_VALIDATION)
        pContext->SetDebugName(descriptor.DebugName, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(out.PipelineHandle));
#endif

        return out;
    }

    void VulkanPipelineBuilder::Destroy() {
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

    std::expected<VkPipelineLayout, StatusCode> VulkanPipelineBuilder::getOrCreateLayout(u32 pushConstantBytes) {
        VASSERT(nullptr != pContext, "VulkanContext cannot be nullptr.");
        const u32 maxPushConstantBytes = pContext->GetVulkanDeviceCapabilities().Limits.MaxPushConstantBytes;

        if (pushConstantBytes > maxPushConstantBytes) {
            VERROR("Push constant block of {} bytes exceeds the device limit of {}.", pushConstantBytes, maxPushConstantBytes);
            return std::unexpected(StatusCode::FailedToCreateVulkanPipelineLayout);
        }

        for (const LayoutEntry &entry : mLayouts) {
            if (entry.PushConstantBytes == pushConstantBytes) {
                return entry.Layout;
            }
        }

        const VkPushConstantRange range{
            // Visible to every stage, so the layout depends on the block's size and nothing else.
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = pushConstantBytes,
        };

        const VkDescriptorSetLayout descriptorSetLayout = pContext->Heap().Layout();

        // VASSERT(VK_NULL_HANDLE != descriptorSetLayout, "descriptorSetLayout cannot be VK_NULL_HANDLE.");

        VkPipelineLayoutCreateInfo layoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .setLayoutCount = 0,
            .pSetLayouts = &descriptorSetLayout,
            // .setLayoutCount = 1,
            // .pSetLayouts = &descriptorSetLayout,
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
