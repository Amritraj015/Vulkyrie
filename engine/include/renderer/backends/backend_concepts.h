#pragma once

#include "core/types/static_string.h"
#include "core/graphics_api.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    namespace detail {

        template <typename B>
        concept RendererBackendTypes = requires {
            // Types.
            typename B::Context;
            typename B::Queue;
            typename B::CommandList;
            typename B::CommandPool;
            typename B::Swapchain;

            // Handles.
            typename B::Image;
            typename B::Buffer;
            typename B::Sampler;
            typename B::Pipeline;
            typename B::ShaderModule;
        };

        template <typename B>
        concept RendererBackendHandlesArePOD = std::is_trivially_copyable_v<typename B::Image> && std::is_trivially_copyable_v<typename B::Buffer> &&
                                               std::is_trivially_copyable_v<typename B::Sampler> && std::is_trivially_copyable_v<typename B::Pipeline> &&
                                               std::is_trivially_copyable_v<typename B::ShaderModule>;

        template <typename B>
        concept RendererBackendConstants = requires {
            { B::kName } -> std::convertible_to<StaticString>;
            { B::kType } -> std::convertible_to<GraphicsAPI>;
            { B::kFramesInFlight } -> std::convertible_to<u32>;

            // Binding model. Not a device query: whether a given adapter can support
            // the chosen model is a PRECONDITION checked during device selection --
            // a Vulkan device without adequate descriptor indexing is rejected there,
            // because a GPU-driven meshlet renderer has no degraded path.
            { B::kUsesBindlessHeap } -> std::convertible_to<bool>;
            // GL has GLsync fences, not monotonic u64 timeline values -- there is no
            // value to return, so TimelineTarget() cannot exist for it.
            { B::kHasTimelineSync } -> std::convertible_to<bool>;
            // GL has no barrier objects carrying layouts; the code shape differs.
            { B::kHasExplicitBarriers } -> std::convertible_to<bool>;
            // GL cannot place two textures on one allocation; the op is inexpressible.
            { B::kHasMemoryAliasing } -> std::convertible_to<bool>;
            // GL contexts are thread-affine; recording is a serial replay.
            { B::kRecordsInParallel } -> std::convertible_to<bool>;
        };

        template <typename B>
        concept RendererBackendContextOps = requires(typename B::Context &c,
                                                     const typename B::Context &cb,
                                                     const ImageDescriptor &id,
                                                     const BufferDescriptor &bd,
                                                     const SamplerDescriptor &sd,
                                                     const ShaderBlob &blob,
                                                     const GraphicsPipelineDescriptor &gpd,
                                                     const ComputePipelineDescriptor &cpd,
                                                     const typename B::Image &image,
                                                     const typename B::Buffer &buffer,
                                                     const typename B::Sampler &sampler,
                                                     const typename B::Pipeline &pipeline,
                                                     const typename B::ShaderModule &shaderModule) {
            { c.CreateImage(id) } -> std::same_as<typename B::Image>;
            { c.CreateBuffer(bd) } -> std::same_as<typename B::Buffer>;
            { c.CreateSampler(sd) } -> std::same_as<typename B::Sampler>;
            { c.CreateShaderModule(blob) } -> std::same_as<typename B::ShaderModule>;
            { c.CreateGraphicsPipeline(gpd) } -> std::same_as<typename B::Pipeline>;
            { c.CreateComputePipeline(cpd) } -> std::same_as<typename B::Pipeline>;

            { c.DestroyImage(image) } -> std::same_as<bool>;
            { c.DestroyBuffer(buffer) } -> std::same_as<bool>;
            { c.DestroySampler(sampler) } -> std::same_as<bool>;
            { c.DestroyShaderModule(shaderModule) } -> std::same_as<bool>;
            { c.CreatePipeline(pipeline) } -> std::same_as<bool>;

            { cb.WaitIdle() } -> std::same_as<void>;
            { cb.QueryCapabilities() } -> std::convertible_to<const DeviceCapabilities &>;
            { cb.DeviceLost() } -> std::same_as<bool>;
        };

        template <typename B>
        concept RendererBackendCommandOps = requires(typename B::CommandList &cl) {
            { cl.Begin() } -> std::same_as<void>;
            { cl.End() } -> std::same_as<void>;
        };

        template <typename B>
        concept RendererBackendQueueOps =
            requires(B::Queue &b, std::span<const typename B::CommandList *const> lists, std::span<u64> waits, std::span<u64> signals) {
                { b.Submit(lists, waits, signals) } -> std::same_as<void>;
                { b.Type() } noexcept -> std::same_as<QueueType>;
            };

    } // namespace detail

    template <typename B>
    concept RendererBackend = detail::RendererBackendTypes<B> && detail::RendererBackendHandlesArePOD<B> && detail::RendererBackendConstants<B> &&
                              detail::RendererBackendContextOps<B> && detail::RendererBackendCommandOps<B> && detail::RendererBackendQueueOps<B>;

} // namespace Vulkyrie
