#pragma once

// A minimal RendererBackend conformant to renderer/backend_concepts.h, so TransientPool can be exercised without a
// real GL/Vulkan device. Every create call hands out a fresh incrementing id; nothing touches the GPU.
#include "core/graphics_api.h"
#include "core/types/static_string.h"
#include "renderer/backend_concepts.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie::TransientPoolTests {

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
    };

    class MockQueue final {
    public:
        void Submit(std::span<const MockCommandList *const>, std::span<u64>, std::span<u64>) {
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

        [[nodiscard]] bool DestroyImage(MockImage) {
            return true;
        }

        [[nodiscard]] bool DestroyBuffer(MockBuffer) {
            return true;
        }

        [[nodiscard]] bool DestroySampler(MockSampler) {
            return true;
        }

        [[nodiscard]] bool DestroyShaderModule(MockShaderModule) {
            return true;
        }

        [[nodiscard]] bool CreatePipeline(MockPipeline) {
            return true;
        }

        void WaitIdle() const {
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

    private:
        DeviceCapabilities mCapabilities{};
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

    static_assert(RendererBackend<MockBackend>, "MockBackend must satisfy the RendererBackend concept.");

} // namespace Vulkyrie::TransientPoolTests
