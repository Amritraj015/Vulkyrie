#pragma once

// Mock resource types and the graph fixture shared by every frame graph test file. The resource types record what
// the graph did to them through `context.Device.Context()` - the same route a real resource type takes to reach its
// backend - so the tests exercise the typed context rather than a side channel.
#include <vulkyrie.h>

#include "../support/mock_backend.h"

#include "renderer/frame_graph/frame_graph.h"

#include <atomic>
#include <string>
#include <vector>

namespace Vulkyrie::FrameGraphTests {

    using Backend = RendererTests::MockBackend;
    using AliasingBackend = RendererTests::MockAliasingBackend;
    using MockContext = RendererTests::MockContext;

    /** @brief Appends an event to the device's log. */
    template <RendererBackend B> void RecordEvent(const FrameGraphContext<B> &context, std::string event) {
        context.Device.Context().Events.push_back(std::move(event));
    }

    /** @brief Where a test pass body reports what ran.
     *
     * Pass bodies cannot capture, so a test wires one of these up in `setup` - which may capture, because it runs
     * immediately and is never stored - and the body reads it back out of the pass data. Every field is optional;
     * a body touches only the ones its test cares about. */
    struct PassProbe {
    public:
        /** @brief Bumped once per execution. */
        i32 *Counter = nullptr;

        /** @brief Bumped once per execution, for the parallel paths where several bodies run at once. */
        std::atomic<i32> *AtomicCounter = nullptr;

        /** @brief Set to true on execution. */
        bool *Flag = nullptr;

        /** @brief Appended with `Label` on execution, for asserting on the compiled order. */
        std::vector<std::string> *Order = nullptr;

        /** @brief Appended with `IntLabel` on execution. */
        std::vector<int> *IntOrder = nullptr;

        const char *Label = "";

        int IntLabel = 0;
    };

    /** @brief The shared captureless pass body: reports through whatever its probe has wired up.
     *
     * Passed as `&RunProbe<PassData>` - a plain function pointer, which is exactly what the graph requires and what
     * a capturing lambda could never be. */
    template <typename TPassData> void RunProbe(const TPassData &data, FrameGraphPassContext<Backend> &) {
        const PassProbe &probe = data.Probe;

        if (nullptr != probe.Counter) {
            ++*probe.Counter;
        }

        if (nullptr != probe.AtomicCounter) {
            probe.AtomicCounter->fetch_add(1, std::memory_order_relaxed);
        }

        if (nullptr != probe.Flag) {
            *probe.Flag = true;
        }

        if (nullptr != probe.Order) {
            probe.Order->push_back(probe.Label);
        }

        if (nullptr != probe.IntOrder) {
            probe.IntOrder->push_back(probe.IntLabel);
        }
    }

    struct MockTextureDescriptor {
    public:
        u32 Width = 0;
        u32 Height = 0;
        std::string Format;
    };

    /** @brief A texture that records every callback and reports memory requirements, so it exercises the lifecycle
     * hooks, the access hooks and the aliasing planner at once. */
    struct MockTexture {
    public:
        using Descriptor = MockTextureDescriptor;

        bool Acquired = false;
        bool Released = false;
        ResourceLifetime Lifetime{};
        i32 PreReadCount = 0;
        i32 PreWriteCount = 0;

        void Acquire(const Descriptor &descriptor, ResourceLifetime lifetime, const FrameGraphContext<Backend> &context) {
            Acquired = true;
            Released = false;
            Lifetime = lifetime;

            // Remembered rather than re-derived on release, which is the whole reason `Release` needs no
            // descriptor: a resource that has been acquired already knows what it is.
            mExtent = extent(descriptor);

            RecordEvent(context, "Acquire:" + mExtent);
        }

        void Release(const FrameGraphContext<Backend> &context) {
            Released = true;
            RecordEvent(context, "Release:" + mExtent);
        }

        void PreRead(const ResourceUsage &usage, const FrameGraphContext<Backend> &context) {
            ++PreReadCount;
            RecordEvent(context, "PreRead:layout=" + std::to_string(std::to_underlying(usage.Layout)));
        }

        void PreWrite(const ResourceUsage &usage, const FrameGraphContext<Backend> &context) {
            ++PreWriteCount;
            RecordEvent(context, "PreWrite:layout=" + std::to_string(std::to_underlying(usage.Layout)));
        }

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor, const Device<Backend> &device) const {
            (void)device;

            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }

    private:
        [[nodiscard]] static std::string extent(const Descriptor &descriptor) {
            return std::to_string(descriptor.Width) + "x" + std::to_string(descriptor.Height);
        }

        /** @brief What this texture was last acquired as, so release can name itself. */
        std::string mExtent;
    };

    /** @brief A minimal resource type: no access hooks, no memory requirements. Exercises the paths where the graph
     * must cope with a type that opts into nothing. */
    struct MockBuffer {
    public:
        using Descriptor = size_t; // Size in bytes.

        bool Acquired = false;
        bool Released = false;

        void Acquire(const Descriptor &descriptor, ResourceLifetime, const FrameGraphContext<Backend> &context) {
            Acquired = true;
            RecordEvent(context, "AcquireBuffer:" + std::to_string(descriptor));
        }

        void Release(const FrameGraphContext<Backend> &context) {
            Released = true;
            RecordEvent(context, "ReleaseBuffer");
        }
    };

    /** @brief A resource type whose descriptor owns no heap memory, so a test can attribute every byte the graph
     * allocates to the graph itself rather than to the descriptor. */
    struct MockPodTexture {
    public:
        struct Descriptor {
        public:
            u32 Width = 0;
            u32 Height = 0;
        };

        void Acquire(const Descriptor &, ResourceLifetime, const FrameGraphContext<Backend> &) {
        }

        void Release(const FrameGraphContext<Backend> &) {
        }
    };

    /** @brief A texture that takes the placed `Acquire`, so a test can assert on the offsets the aliasing plan
     * hands the resource type rather than only on the totals the report publishes. */
    template <RendererBackend B> struct MockPlacedTextureFor {
    public:
        using Descriptor = MockTextureDescriptor;

        ResourcePlacement Placement{};
        ResourceLifetime Lifetime{};
        bool Acquired = false;

        void Acquire(const Descriptor &descriptor, ResourceLifetime lifetime, ResourcePlacement placement, const FrameGraphContext<B> &context) {
            Placement = placement;
            Lifetime = lifetime;
            Acquired = true;
            RecordEvent(context, "Acquire:" + std::to_string(descriptor.Width) + "@" + (placement.IsAliased ? std::to_string(placement.Offset) : "unplaced"));
        }

        void Release(const FrameGraphContext<B> &context) {
            RecordEvent(context, "Release");
        }

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor, const Device<B> &device) const {
            (void)device;

            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }
    };

    /** @brief The placed texture on a backend that cannot honour an offset, so it is always handed an unplaced
     * placement no matter what the plan computed. */
    using MockPlacedTexture = MockPlacedTextureFor<Backend>;

    /** @brief The placed texture on a backend that can bind two resources to one allocation, so the plan's offsets
     * actually reach it. */
    using MockAliasedTexture = MockPlacedTextureFor<AliasingBackend>;

    /** @brief Everything a frame graph test needs to execute a graph: a mock device, one frame's command lists, and
     * the typed context tying them together. */
    template <RendererBackend B> struct GraphFixtureFor {
    public:
        explicit GraphFixtureFor(const FrameGraphConfig &config = {})
            : Graph(Dev, config) {
        }

        DeviceCreationInfo Info{ ApplicationInfo{ "FrameGraphTests", { 1, 0, 0 } }, WindowHandle{}, Extent2D{ 800, 600 }, 64, 64, 16, 16 };
        Device<B> Dev{ Info };
        FrameContext<B> Frame{ Dev.Context(), 0, 1, 0 };
        FrameGraph<B> Graph;
        FrameGraphContext<B> Context{ Dev, Frame };

        /** @brief Everything the mock resource types recorded, in order. */
        [[nodiscard]] const std::vector<std::string> &Events() const {
            return Dev.Context().Events;
        }

        /** @brief Only the acquire/release events. */
        [[nodiscard]] std::vector<std::string> Lifecycle() const {
            return Dev.Context().Lifecycle();
        }

        /** @brief One entry per `EmitBarriers` call the graph made, describing the batch it carried. */
        [[nodiscard]] const std::vector<std::string> &BarrierBatches() {
            return Frame.AcquireCommandList(0, QueueType::Graphics).BarrierBatches;
        }
    };

    using GraphFixture = GraphFixtureFor<Backend>;

    using AliasingGraphFixture = GraphFixtureFor<AliasingBackend>;

    static_assert(FrameGraphResourceType<MockTexture, Backend>, "MockTexture must satisfy the resource concept.");
    static_assert(FrameGraphResourceType<MockBuffer, Backend>, "MockBuffer must satisfy the resource concept.");
    static_assert(FrameGraphResourceType<MockPodTexture, Backend>, "MockPodTexture must satisfy the resource concept.");
    static_assert(HasPreRead<MockTexture, Backend> && HasPreWrite<MockTexture, Backend>, "MockTexture implements both access hooks.");
    static_assert(!HasPreRead<MockBuffer, Backend> && !HasPreWrite<MockBuffer, Backend>, "MockBuffer opts out of the access hooks.");
    static_assert(HasMemoryRequirements<MockTexture, Backend>, "MockTexture reports memory requirements.");
    static_assert(!HasMemoryRequirements<MockBuffer, Backend>, "MockBuffer does not report memory requirements.");
    static_assert(FrameGraphResourceType<MockPlacedTexture, Backend>, "MockPlacedTexture must satisfy the resource concept through the placed Acquire.");
    static_assert(HasPlacedAcquire<MockPlacedTexture, Backend> && !HasPlainAcquire<MockPlacedTexture, Backend>,
                  "MockPlacedTexture takes only the placed Acquire.");
    static_assert(FrameGraphResourceType<MockAliasedTexture, AliasingBackend>, "MockAliasedTexture must satisfy the resource concept.");
    static_assert(HasPlainAcquire<MockTexture, Backend> && !HasPlacedAcquire<MockTexture, Backend>, "MockTexture takes only the plain Acquire.");

    /** @brief Whether a resource type's lifetime hooks are reachable through a const reference - which is all a
     * pass body ever gets. Named rather than written inline so the check is a concept substitution rather than a
     * hard error. */
    template <typename T, typename B>
    concept LifetimeReachableThroughConst = requires(const T &resource, const FrameGraphContext<B> &context) { resource.Release(context); };

    // The whole point of handing pass bodies a const view: a resource type's Acquire and Release are non-const, so
    // a pass cannot reach them and cannot take part in a lifetime the graph just finished planning.
    static_assert(!LifetimeReachableThroughConst<MockTexture, Backend>, "A pass must not be able to release a resource through the const view it is given.");

} // namespace Vulkyrie::FrameGraphTests
