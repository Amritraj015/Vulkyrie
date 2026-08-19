#include "frame_graph_test_support.h"

#include "renderer/frame_graph/resources/frame_graph_buffer.h"
#include "renderer/frame_graph/resources/frame_graph_texture.h"

#include <catch2/catch_test_macros.hpp>

using namespace Vulkyrie;
using namespace Vulkyrie::FrameGraphTests;

namespace {

    using Texture = FrameGraphTexture<Backend>;
    using Buffer = FrameGraphBuffer<Backend>;
    using TargetHandle = FrameGraphHandle<Texture>;
    using ScratchHandle = FrameGraphHandle<Buffer>;

    constexpr TextureDescriptor TARGET{ .Width = 1920, .Height = 1080, .Format = Format::RGBA8Unorm };

    constexpr BufferDescriptor SCRATCH{ .Size = 4096, .Usage = BufferUsage::Storage };

} // namespace

static_assert(FrameGraphResourceType<Texture, Backend>, "FrameGraphTexture must satisfy the resource concept.");
static_assert(FrameGraphResourceType<Buffer, Backend>, "FrameGraphBuffer must satisfy the resource concept.");
static_assert(HasPlacedAcquire<Texture, Backend>, "FrameGraphTexture takes the placed Acquire so it can honour a plan where one exists.");
static_assert(HasMemoryRequirements<Texture>, "FrameGraphTexture reports memory requirements so the aliasing plan can size it.");

// ===========================================================================================
// The frame graph and the transient pool are one system
// ===========================================================================================

TEST_CASE("FrameGraphTexture - Disjoint lifetimes share one pooled image", "[framegraph][transientpool][resources]") {
    // The end-to-end claim: the graph derives lifetimes while compiling, hands them to the resource type at
    // acquire time, and the pool turns two same-descriptor requests with disjoint intervals into one image. Before
    // this wiring existed the two halves computed the same answer independently and neither used the other's.
    //
    // Three stages, not two: a stage that reads its predecessor's output keeps that output live through its own
    // pass, so consecutive stages always overlap. Only Stage0 and Stage2 are genuinely disjoint.
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    TargetHandle first, second;

    struct SeedData {
        TargetHandle Output;
        PassProbe Probe;
    };

    struct StageData {
        TargetHandle Input;
        TargetHandle Output;
        PassProbe Probe;
    };

    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph<Backend>::Builder &builder, SeedData &data) {
            data.Output = builder.Create<Texture>("Stage0", TARGET);
            first = data.Output;
        },
        [](const SeedData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<Texture>("Stage1", TARGET);
            second = data.Output;
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    // Stage0's texture died at the end of Stage1, so this one can take it over.
    graph.AddPass<StageData>(
        "Stage2",
        [&second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(second);
            data.Output = builder.Create<Texture>("Stage2", TARGET);
            builder.MarkSideEffect();
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // Two images for three textures. The pool only reaches that answer because the graph told it the intervals;
    // without them every request looks live-forever and each texture gets its own image.
    REQUIRE(fixture.Dev.GetTransients().GetStats().ImagesCreatedThisFrame == 2);
    REQUIRE(fixture.Dev.Context().ImagesCreated() == 2);

    // And the graph's own byte-packing plan agrees on which pair can share, even though this backend cannot bind
    // two resources to one allocation and so never honours the offsets.
    REQUIRE(graph.GetAliasingReport().ResourceCount == 3);
    REQUIRE(graph.GetAliasingReport().SavedBytes() > 0);
}

TEST_CASE("FrameGraphTexture - Overlapping lifetimes get separate pooled images", "[framegraph][transientpool][resources]") {
    // The control for the test above: same descriptors, but both textures are live at once, so the pool must not
    // hand the same image to both.
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    TargetHandle first, second;

    struct ProduceData {
        TargetHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        TargetHandle First;
        TargetHandle Second;
        PassProbe Probe;
    };

    graph.AddPass<ProduceData>(
        "ProduceA",
        [&first](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<Texture>("A", TARGET);
            first = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<ProduceData>(
        "ProduceB",
        [&second](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<Texture>("B", TARGET);
            second = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    // Reading both at once is what forces their lifetimes to overlap.
    graph.AddPass<ConsumeData>(
        "Combine",
        [&first, &second](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
            data.First = builder.Read(first);
            data.Second = builder.Read(second);
            builder.MarkSideEffect();
        },
        [](const ConsumeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(fixture.Dev.GetTransients().GetStats().ImagesCreatedThisFrame == 2);
}

TEST_CASE("FrameGraphTexture - A pass binds the image the pool handed over", "[framegraph][resources]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    u32 boundImageId = 0;

    struct PassData {
        TargetHandle Target;
        u32 *BoundImageId = nullptr;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "Bind",
        [&boundImageId](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Target = builder.Create<Texture>("Target", TARGET);
            data.BoundImageId = &boundImageId;
            builder.MarkSideEffect();
        },
        [](const PassData &data, const FrameGraphResources<Backend> &resources, FrameGraphPassContext<Backend> &) {
            *data.BoundImageId = resources.Get(data.Target).Image().Id;
        });

    graph.Compile();
    graph.Execute(fixture.Context);

    // The mock context hands out ids from one, so a non-zero id means the pass saw a real acquisition rather than
    // a default-constructed handle.
    REQUIRE(boundImageId == 1);
}

TEST_CASE("FrameGraphTexture - Release clears the handle so a stale bind is visible", "[framegraph][resources]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    TargetHandle target;

    struct PassData {
        TargetHandle Target;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "Draw",
        [&target](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Target = builder.Create<Texture>("Target", TARGET);
            target = data.Target;
            builder.MarkSideEffect();
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // The pool reclaims in bulk at frame end, so releasing hands nothing back - but the resource drops its handle,
    // which is what turns a use-after-release into an obvious zero rather than a bind of whatever took the image.
    REQUIRE(graph.GetResource(target).Image().Id == 0);
}

TEST_CASE("FrameGraphBuffer - Sizing is exact and reaches the aliasing report", "[framegraph][resources]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    struct PassData {
        ScratchHandle Scratch;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "Compute",
        [](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Scratch = builder.Create<Buffer>("Scratch", SCRATCH);
            builder.MarkSideEffect();
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    REQUIRE(graph.GetAliasingReport().ResourceCount == 1);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == SCRATCH.Size);
}

// ===========================================================================================
// What the API refuses to let a pass do
// ===========================================================================================

namespace {

    /** @brief Whether a callable is accepted as a pass body, i.e. whether it converts to the plain function pointer
     * `AddPass` requires. Named so the check is a concept substitution rather than a hard error. */
    template <typename TExecute, typename TPassData>
    concept UsableAsPassBody =
        std::convertible_to<TExecute, void (*)(const TPassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &)>;

    struct ProbeData {
        TargetHandle Target;
    };

    /** @brief Stand-in for the state a pass body might be tempted to capture. */
    inline i32 gCounter = 0;

} // namespace

TEST_CASE("FrameGraph - A capturing pass body is not a pass body", "[framegraph][typing]") {
    // The rule AddPass enforces with a static_assert, checked here without having to fail a compile: a body that
    // captures cannot become a function pointer, so it cannot be stored, so it cannot race under RecordParallel or
    // dangle past the frame that declared it.
    const auto captureless = [](const ProbeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++gCounter; };
    const auto capturing = [&](const ProbeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++gCounter; };

    STATIC_REQUIRE(UsableAsPassBody<decltype(captureless), ProbeData>);
    STATIC_REQUIRE_FALSE(UsableAsPassBody<decltype(capturing), ProbeData>);
}

TEST_CASE("FrameGraph - A pass cannot drive a resource's lifetime", "[framegraph][typing]") {
    // Both halves of the door being shut. There is no mutable resource accessor to reach at all, and what a pass
    // does get is a const reference, through which the non-const lifetime hooks are unreachable.
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const FrameGraphResources<Backend> &>().Get(std::declval<TargetHandle>())), const Texture &>);
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<FrameGraph<Backend> &>().GetResource(std::declval<TargetHandle>())), const Texture &>);
    STATIC_REQUIRE_FALSE(LifetimeReachableThroughConst<Texture, Backend>);
}
