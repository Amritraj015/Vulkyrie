#pragma once

#include "vlkypch.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/vulkan/vulkan_host_allocator.h"
#include "renderer/vulkan/vulkan_types.h"
#include <volk.h>

namespace Vulkyrie {

    class VulkanContext;

    /** @brief Turns an RHI pipeline descriptor and its compiled shader modules into a `VkPipeline`.
     *
     * The builder owns no pipelines: each `Build*` call hands its result to the caller, who is responsible for
     * destroying it. It does own the pipeline layouts, which are shared between every pipeline that agrees on
     * push-constant size and live until the builder does.
     *
     * Pipelines are always built for dynamic rendering, never a render pass, so the attachment formats come from the
     * descriptor's `RenderTargetLayout`. Vertex input is always empty: shaders read their geometry through the
     * context's descriptor heap rather than through vertex buffers. Viewport, scissor and blend constants are dynamic
     * and must be set on the command buffer.
     *
     * Non-copyable, non-movable, and destroyed against the `VulkanContext` it was built from, which must outlive it. */
    class VulkanPipelineBuilder final {
    public:
        /** @brief Constructs a builder bound to no device; only assignment from a constructed one makes it usable. */
        VulkanPipelineBuilder() = default;

        VE_DELETE_MOVE_AND_COPY(VulkanPipelineBuilder);

        /** @brief Binds the builder to the device it creates pipelines on.
         * @param context Owner of the device, pipeline cache and descriptor heap; must outlive the builder.
         * @param allocator Host allocation callbacks, forwarded to every Vulkan object created here. */
        explicit VulkanPipelineBuilder(VulkanContext *context, VulkanHostAllocator *allocator) noexcept;

        /** @brief Calls `Destroy`. */
        ~VulkanPipelineBuilder();

        /** @brief Creates a graphics pipeline for dynamic rendering.
         *
         * `stages` pairs positionally with the shader keys the descriptor fills in, skipping the empty ones, in the
         * order the descriptor declares them: task, mesh, vertex, tessellation control, tessellation evaluation,
         * fragment. Supplying a different number of modules than there are filled keys fails the call rather than
         * producing a partial pipeline.
         *
         * Descriptor state that the render target layout contradicts is dropped rather than passed on: depth test and
         * write need a depth format, and the stencil test needs a format that carries stencil.
         *
         * @param descriptor The pipeline state to create.
         * @param stages One compiled module per filled shader key, in descriptor order.
         * @returns The pipeline, or an invalid `VulkanPipeline` if creation failed. */
        [[nodiscard]] VulkanPipeline BuildGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor, std::span<const VulkanShaderModule> stages);

        /** @brief Creates a compute pipeline.
         * @param descriptor The pipeline state to create.
         * @param stage The compiled compute module.
         * @returns The pipeline, or an invalid `VulkanPipeline` if creation failed. */
        [[nodiscard]] VulkanPipeline BuildComputePipeline(const ComputePipelineDescriptor &descriptor, const VulkanShaderModule &stage);

        /** @brief Destroys every pipeline layout this builder handed out.
         *
         * Safe to call more than once, and safe on a builder that was never bound to a device. Pipelines already
         * built are not touched: their layout handles dangle from here on, so this is only called once nothing is
         * still using them. The builder stays usable and creates layouts again on the next `Build*` call. */
        void Destroy();

    private:
        /** @brief One cached pipeline layout and the push-constant size it was created for. */
        struct LayoutEntry final {
            VkPipelineLayout Layout{ VK_NULL_HANDLE };
            u32 PushConstantBytes{ 0 };

            LayoutEntry(VkPipelineLayout layout, u32 pushConstantBytes)
                : Layout(layout)
                , PushConstantBytes(pushConstantBytes) {
            }
        };

        /** @brief Layouts are keyed on push-constant size alone, so a handful covers a whole frame. */
        static constexpr usize kExpectedLayoutCount = 8;

        /** @brief A pointer to the Vulkan context. */
        VulkanContext *pContext{ nullptr };

        /** @brief A pointer to the Host memory allocator. */
        VulkanHostAllocator *pHostAllocator{ nullptr };

        /** @brief The list of created pipeline layouts and the push-constant size it was created for. */
        RendererVector<LayoutEntry> mLayouts;

        /** @brief Returns the layout for a push-constant size, creating and caching it on first use.
         *
         * Every layout names the context's one descriptor set layout and a single push-constant range visible to all
         * stages, so the size is the whole key. The returned handle stays owned by the builder.
         *
         * @param pushConstantBytes Size of the pipeline's push-constant block; 0 for no push constants.
         * @returns The layout, or the status code of the failure. */
        [[nodiscard]] std::expected<VkPipelineLayout, StatusCode> getOrCreateLayout(u32 pushConstantBytes);
    };

} // namespace Vulkyrie
