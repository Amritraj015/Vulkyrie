#pragma once

// A minimal RendererBackend conformant to renderer/backend_concepts.h, so the transient pool and the frame graph
// can be exercised without a real GL/Vulkan device. Every create call hands out a fresh incrementing id; nothing
// touches the GPU.
#include "core/graphics_api.h"
#include "core/types/static_string.h"
#include "renderer/backend_concepts.h"
#include "renderer/rhi/barrier_types.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"

#include <string>
#include <vector>

namespace Vulkyrie::RendererTests {

    struct MockImage final {
        u32 Id = 0;
    };

    struct MockBuffer final {
        u32 Id = 0;
    };

    struct MockSampler final {
        u32 Id = 0;
    };

    struct MockPipeline final {
        u32 Id = 0;
    };

    struct MockShaderModule final {
        u32 Id = 0;
    };

    static_assert(std::is_trivially_copyable_v<MockImage>);
    static_assert(std::is_trivially_copyable_v<MockBuffer>);
    static_assert(std::is_trivially_copyable_v<MockSampler>);
    static_assert(std::is_trivially_copyable_v<MockPipeline>);
    static_assert(std::is_trivially_copyable_v<MockShaderModule>);

    class MockCommandList final {
    public:
        void Begin() {
        }

        void End() {
        }

        /** @brief Records one entry per call describing the batch it carried, so a test can assert on the exact
         * sequence of transitions rather than on individual flags. */
        void EmitBarriers(std::span<const ResourceBarrier> barriers) {
            std::string line = "Batch(" + std::to_string(barriers.size()) + "):";

            for (const ResourceBarrier &barrier : barriers) {
                line += " e" + std::to_string(barrier.Entry.Get()) + "[" + std::to_string(std::to_underlying(barrier.Before.Layout)) + "->" +
                        std::to_string(std::to_underlying(barrier.After.Layout)) + "]";

                // A discard names the stages it waits on rather than a layout it comes from, so print those instead.
                if (barrier.AliasingTransition) {
                    line += "{discard,waitStages=" + std::to_string(std::to_underlying(barrier.Before.Stages)) + "}";
                }
            }

            BarrierBatches.push_back(std::move(line));
        }

        /** @brief One entry per `EmitBarriers` call, in order. */
        std::vector<std::string> BarrierBatches;
    };

    class MockQueue final {
    public:
        /** @brief Templated on the list type so one queue serves every mock backend, whichever command list it
         * pairs with. */
        template <typename TCommandList> void Submit(std::span<const TCommandList *const>, std::span<u64>, std::span<u64>) {
        }

        [[nodiscard]] QueueType Type() const noexcept {
            return QueueType::Graphics;
        }
    };

    class MockCommandPool final {};

    class MockSwapchain final {};

    /** @brief Hands out ids rather than real GPU handles, and counts how many of each resource kind it created -
     * that count is what the pool tests assert on to prove reuse actually happened. */
    class MockContext final {
    public:
        explicit MockContext(const DeviceCreationInfo &) {
        }

        [[nodiscard]] StatusCode Initialize() {
            return StatusCode::Successful;
        }

        [[nodiscard]] MockImage CreateImage(const TextureDescriptor &) {
            return MockImage{ .Id = ++mImagesCreated };
        }

        [[nodiscard]] MockBuffer CreateBuffer(const BufferDescriptor &) {
            return MockBuffer{ .Id = ++mBuffersCreated };
        }

        [[nodiscard]] MockSampler CreateSampler(const SamplerDescriptor &) {
            return MockSampler{};
        }

        [[nodiscard]] MockShaderModule CreateShaderModule(const ShaderBlob &) {
            return MockShaderModule{};
        }

        [[nodiscard]] MockPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &) {
            return MockPipeline{};
        }

        [[nodiscard]] MockPipeline CreateComputePipeline(const ComputePipelineDescriptor &) {
            return MockPipeline{};
        }

        void DestroyImage(MockImage) {
        }

        void DestroyBuffer(MockBuffer) {
        }

        void DestroySampler(MockSampler) {
        }

        void DestroyShaderModule(MockShaderModule) {
        }

        void DestroyPipeline(MockPipeline) {
        }

        void WaitIdle() const {
        }

        /** @brief Sizes an image the way a driver would be asked to, so the resource types that route through the
         * backend have something to route to. */
        [[nodiscard]] ResourceMemoryRequirements GetImageMemoryRequirements(const TextureDescriptor &descriptor) const {
            ++mMemoryQueries;

            return ResourceMemoryRequirements{ .Size = EstimateTextureBytes(descriptor), .Alignment = 256 };
        }

        [[nodiscard]] ResourceMemoryRequirements GetBufferMemoryRequirements(const BufferDescriptor &descriptor) const {
            ++mMemoryQueries;

            return ResourceMemoryRequirements{ .Size = descriptor.Size, .Alignment = 256 };
        }

        /** @brief How many times the backend was actually asked to size something, so a test can prove the
         * per-descriptor cache in `Device` is doing its job rather than the driver being hit every frame. */
        [[nodiscard]] u32 MemoryQueries() const {
            return mMemoryQueries;
        }

        [[nodiscard]] const DeviceCapabilities &QueryCapabilities() const {
            return mCapabilities;
        }

        [[nodiscard]] bool DeviceLost() const {
            return false;
        }

        [[nodiscard]] bool ContextCreated() const {
            return true;
        }

        [[nodiscard]] u32 ImagesCreated() const {
            return mImagesCreated;
        }

        [[nodiscard]] u32 BuffersCreated() const {
            return mBuffersCreated;
        }

        /** @brief Everything the frame graph's mock resource types did, in order, so a test can assert on the exact
         * sequence rather than on individual flags. Reached the same way a real resource type reaches its backend:
         * `context.Device.Context()`. */
        std::vector<std::string> Events;

        /** @brief Returns only the acquire/release events, for lifetime assertions that should not be disturbed by
         * the access hooks. */
        [[nodiscard]] std::vector<std::string> Lifecycle() const {
            std::vector<std::string> filtered;

            for (const std::string &event : Events) {
                if (event.starts_with("Acquire") || event.starts_with("Release")) {
                    filtered.push_back(event);
                }
            }

            return filtered;
        }

    private:
        DeviceCapabilities mCapabilities{};

        /** @brief Mutable because the sizing queries are const, the way a real backend's are. */
        mutable u32 mMemoryQueries = 0;

        u32 mImagesCreated = 0;
        u32 mBuffersCreated = 0;
    };

    struct MockBackend final {
        using Context = MockContext;
        using Queue = MockQueue;
        using CommandList = MockCommandList;
        using CommandPool = MockCommandPool;
        using Swapchain = MockSwapchain;

        using Image = MockImage;
        using Buffer = MockBuffer;
        using Sampler = MockSampler;
        using Pipeline = MockPipeline;
        using ShaderModule = MockShaderModule;

        static constexpr StaticString kName = "Mock";
        static constexpr GraphicsAPI kType = GraphicsAPI::None;
        static constexpr u32 kFramesInFlight = 2;
        static constexpr bool kUsesBindlessHeap = false;
        static constexpr bool kHasTimelineSync = false;
        static constexpr bool kHasExplicitBarriers = false;
        static constexpr bool kHasMemoryAliasing = false;
        static constexpr bool kRecordsInParallel = false;
    };

    /** @brief `MockBackend`, but able to bind two resources to one allocation. Everything else is identical, so a
     * test can run the same graph on both and see exactly what `kHasMemoryAliasing` gates: whether the frame
     * graph's byte-offset plan is handed to resource types, or whether they are left to take their own storage and
     * let the transient pool do the reuse. */
    struct MockAliasingBackend final {
        using Context = MockContext;
        using Queue = MockQueue;
        using CommandList = MockCommandList;
        using CommandPool = MockCommandPool;
        using Swapchain = MockSwapchain;

        using Image = MockImage;
        using Buffer = MockBuffer;
        using Sampler = MockSampler;
        using Pipeline = MockPipeline;
        using ShaderModule = MockShaderModule;

        static constexpr StaticString kName = "MockAliasing";
        static constexpr GraphicsAPI kType = GraphicsAPI::None;
        static constexpr u32 kFramesInFlight = 2;
        static constexpr bool kUsesBindlessHeap = false;
        static constexpr bool kHasTimelineSync = false;
        static constexpr bool kHasExplicitBarriers = true;
        static constexpr bool kHasMemoryAliasing = true;
        static constexpr bool kRecordsInParallel = false;
    };

    /** @brief A command list that discards barriers rather than recording them, the way a real backend without
     * barrier objects does. `MockCommandList` builds a string per batch so tests can assert on the exact sequence -
     * useful there, but it makes the mock the dominant cost in anything measuring the graph. */
    class MockSilentCommandList final {
    public:
        void Begin() {
        }

        void End() {
        }

        void EmitBarriers(std::span<const ResourceBarrier> barriers) {
            (void)barriers;
        }
    };

    /** @brief A backend that records in parallel and whose command list does nothing.
     *
     * For benchmarks: `MockBackend` cannot record in parallel, so `RecordParallel` against it degrades to `Record`
     * and measures the wrong path entirely; and its command list allocates a string per barrier batch, which buries
     * whatever the graph costs. This one models a backend where the backend's own work is free, so what is left in
     * the measurement is the graph. */
    struct MockParallelBackend final {
        using Context = MockContext;
        using Queue = MockQueue;
        using CommandList = MockSilentCommandList;
        using CommandPool = MockCommandPool;
        using Swapchain = MockSwapchain;

        using Image = MockImage;
        using Buffer = MockBuffer;
        using Sampler = MockSampler;
        using Pipeline = MockPipeline;
        using ShaderModule = MockShaderModule;

        static constexpr StaticString kName = "MockParallel";
        static constexpr GraphicsAPI kType = GraphicsAPI::None;
        static constexpr u32 kFramesInFlight = 2;
        static constexpr bool kUsesBindlessHeap = false;
        static constexpr bool kHasTimelineSync = false;
        static constexpr bool kHasExplicitBarriers = true;
        static constexpr bool kHasMemoryAliasing = false;
        static constexpr bool kRecordsInParallel = true;
    };

    static_assert(RendererBackend<MockBackend>, "MockBackend must satisfy the RendererBackend concept.");
    static_assert(RendererBackend<MockParallelBackend>, "MockParallelBackend must satisfy the RendererBackend concept.");
    static_assert(RendererBackend<MockAliasingBackend>, "MockAliasingBackend must satisfy the RendererBackend concept.");

} // namespace Vulkyrie::RendererTests
