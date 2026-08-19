#include "frame_graph_test_support.h"

#include <catch2/catch_test_macros.hpp>
#include <memory/memory_tracker.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace Vulkyrie;
using namespace Vulkyrie::FrameGraphTests;

namespace {

    using MockTextureHandle = FrameGraphHandle<MockTexture>;
    using MockBufferHandle = FrameGraphHandle<MockBuffer>;
    using MockPodHandle = FrameGraphHandle<MockPodTexture>;
    using MockPlacedTextureHandle = FrameGraphHandle<MockPlacedTexture>;
    using MockAliasedTextureHandle = FrameGraphHandle<MockAliasedTexture>;

    /** @brief Returns whether `first` appears before `second` in the recorded execution order. */
    [[nodiscard]] bool RunsBefore(const std::vector<std::string> &order, const std::string &first, const std::string &second) {
        return std::ranges::find(order, first) < std::ranges::find(order, second);
    }

    /** @brief Returns whether the order contains a named pass. */
    [[nodiscard]] bool Ran(const std::vector<std::string> &order, const std::string &name) {
        return std::ranges::find(order, name) != order.end();
    }

    // Stand-in usage masks. The graph treats every field as opaque, so the values only have to differ; a Vulkan
    // backend would map them to real stage/access/layout enums. Kept at namespace scope because a pass's setup
    // lambda would otherwise have to capture them.
    constexpr ResourceUsage ATTACHMENT{ .Stages = 0x1, .Access = 0x2, .Layout = 1, .QueueType = 0 };
    constexpr ResourceUsage SAMPLED{ .Stages = 0x4, .Access = 0x8, .Layout = 2, .QueueType = 0 };

} // namespace

// ===========================================================================================
// Handle and id typing
// ===========================================================================================

TEST_CASE("FrameGraph - Handles and ids are zero-cost strong types", "[framegraph][typing]") {
    STATIC_REQUIRE(sizeof(MockTextureHandle) == sizeof(u32));
    STATIC_REQUIRE(sizeof(FrameGraphResourceID) == sizeof(u32));
    STATIC_REQUIRE(std::is_trivially_copyable_v<MockTextureHandle>);

    // A phantom backend parameter must not change the handle's representation.
    STATIC_REQUIRE(sizeof(MockTextureHandle) == sizeof(MockBufferHandle));

    // The ids are mutually unassignable, which is the whole point of tagging them.
    STATIC_REQUIRE_FALSE(std::is_convertible_v<FrameGraphPassID, FrameGraphResourceID>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<FrameGraphResourceID, FrameGraphResourceEntryID>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<MockTextureHandle, MockBufferHandle>);

    // Default-constructed handles are detectably empty rather than pointing at element zero.
    REQUIRE_FALSE(MockTextureHandle{}.IsValid());
    REQUIRE_FALSE(FrameGraphResourceID{}.IsValid());
    REQUIRE(FrameGraphResourceID{ 0 }.IsValid());
}

// ===========================================================================================
// Basic construction and execution
// ===========================================================================================

TEST_CASE("FrameGraph - Basic pass addition", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executionCount = 0;

    struct PassData {
        i32 Value = 42;
        PassProbe Probe;
    };

    const auto &data = graph.AddPass<PassData>(
        "TestPass",
        [&](FrameGraph<Backend>::Builder &builder, PassData &data) { builder.MarkSideEffect();
            data.Probe.Counter = &executionCount;
        },
        [](const PassData &data, const FrameGraphResources<Backend> &resources, FrameGraphPassContext<Backend> &) {
            ++*data.Probe.Counter;
            REQUIRE(data.Value == 42);
        });

    REQUIRE(data.Value == 42);

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - Resource creation and access", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    bool passExecuted = false;

    struct PassData {
        MockTextureHandle Texture;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "CreateTexture",
        [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("MainTexture", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            builder.MarkSideEffect();
            data.Probe.Flag = &passExecuted;
        },
        [](const PassData &data, const FrameGraphResources<Backend> &resources, FrameGraphPassContext<Backend> &) { *data.Probe.Flag = true; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(passExecuted);
    REQUIRE(fixture.Lifecycle().front() == "Acquire:1920x1080");
}

TEST_CASE("FrameGraph - Execute functor can reach the resource it declared", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    const MockTexture *observed = nullptr;

    // The body cannot capture `observed`, so setup - which may capture, because it runs immediately and is never
    // stored - hands it down through the pass data instead.
    struct PassData {
        MockTextureHandle Texture;
        const MockTexture **Observed = nullptr;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "ResourceReadBack",
        [&observed](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("ReadBackTarget", MockTextureDescriptor{ 64, 64, "RGBA8" });
            data.Observed = &observed;
            builder.MarkSideEffect();
        },
        [](const PassData &data, const FrameGraphResources<Backend> &resources, FrameGraphPassContext<Backend> &) {
            *data.Observed = &resources.Get(data.Texture);
        });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(observed != nullptr);
    REQUIRE(observed->Acquired); // The graph must have materialised the resource before the pass ran.
}

TEST_CASE("FrameGraph - Names cost no arena bytes", "[framegraph]") {
    struct PassData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    // Identical graphs whose names differ by 139 characters. Any copying would show up as a difference here.
    const auto usedFor = [](StaticString passName, StaticString resourceName) {
        GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

        graph.AddPass<PassData>(
            passName,
            [resourceName](FrameGraph<Backend>::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>(resourceName, MockTextureDescriptor{ 64, 64, "D32" });
                builder.MarkSideEffect();
            },
            [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

        graph.Compile();

        return graph.GetFrameArena().Used();
    };

    REQUIRE(usedFor("P", "R") == usedFor("PassNameThatIsConsiderablyLongerThanTheOtherOneByFiftyOrSoCharacters",
                                         "ResourceNameThatIsAlsoMuchLongerThanItsCounterpartAboveForTheSameReason"));
}

TEST_CASE("FrameGraph - Names that vary come from a table of literals", "[framegraph]") {
    struct PassData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    static constexpr StaticString CASCADE_NAMES[]{ "Cascade0", "Cascade1", "Cascade2", "Cascade3" };

    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    for (u32 cascade = 0; cascade < 4; ++cascade) {
        graph.AddPass<PassData>(
            CASCADE_NAMES[cascade],
            [](FrameGraph<Backend>::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>("CascadeTarget", MockTextureDescriptor{ 64, 64, "D32" });
                builder.MarkSideEffect();
            },
            [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});
    }

    graph.Compile();

    REQUIRE(graph.GetPassCount() == 4);
    REQUIRE(graph.GetPassName(FrameGraphPassID{ 0 }) == "Cascade0");
    REQUIRE(graph.GetPassName(FrameGraphPassID{ 3 }) == "Cascade3");
}

TEST_CASE("FrameGraph - Descriptors are readable through a handle", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    struct PassData {
        MockTextureHandle Texture;
        PassProbe Probe;
    };

    const auto &data = graph.AddPass<PassData>(
        "DescriptorPass",
        [](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("Described", MockTextureDescriptor{ 800, 600, "RGBA8" });
            builder.MarkSideEffect();
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    REQUIRE(graph.GetDescriptor(data.Texture).Width == 800);
    REQUIRE(graph.GetDescriptor(data.Texture).Height == 600);
}

TEST_CASE("FrameGraph - Read and Write operations", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle sharedTexture;

    struct ProducerData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ConsumerData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<ProducerData>(
        "Producer",
        [&sharedTexture](FrameGraph<Backend>::Builder &builder, ProducerData &data) {
            data.Output = builder.Create<MockTexture>("SharedTexture", MockTextureDescriptor{ 512, 512, "RGBA8" });
            sharedTexture = data.Output;
        },
        [](const ProducerData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<ConsumerData>(
        "Consumer",
        [&sharedTexture](FrameGraph<Backend>::Builder &builder, ConsumerData &data) {
            data.Input = builder.Read(sharedTexture);
            builder.MarkSideEffect();
        },
        [](const ConsumerData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(fixture.Lifecycle() == std::vector<std::string>{ "Acquire:512x512", "Release:512x512" });
}

TEST_CASE("FrameGraph - Multiple resource types", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executionCount = 0;

    struct PassData {
        MockTextureHandle Texture;
        MockBufferHandle Buffer;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "MultiResourcePass",
        [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("MultiTexture", MockTextureDescriptor{ 512, 512, "RGBA8" });
            data.Buffer = builder.Create<MockBuffer>("MultiBuffer", size_t{ 1024 });
            builder.MarkSideEffect();
            data.Probe.Counter = &executionCount;
        },
        [](const PassData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionCount == 1);

    // A backend that opts out of the access hooks still gets its lifecycle callbacks.
    REQUIRE(std::ranges::find(fixture.Events(), "AcquireBuffer:1024") != fixture.Events().end());
}

TEST_CASE("FrameGraph - Empty graph", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(graph.GetExecutionOrder().empty());
}

// ===========================================================================================
// Culling
// ===========================================================================================

TEST_CASE("FrameGraph - Pass culling for unreferenced resources", "[framegraph][cull]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 producerExecuted = 0;
    i32 consumerExecuted = 0;

    struct ProducerData {
        MockTextureHandle Texture;
        PassProbe Probe;
    };

    struct ConsumerData {
        PassProbe Probe;
    };

    graph.AddPass<ProducerData>(
        "UnusedProducer",
        [&](FrameGraph<Backend>::Builder &builder, ProducerData &data) {
            data.Texture = builder.Create<MockTexture>("UnusedTexture", MockTextureDescriptor{ 256, 256, "RGBA8" });
            // No side effects and nothing reads the output, so this pass must be culled.
            data.Probe.Counter = &producerExecuted;
        },
        [](const ProducerData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.AddPass<ConsumerData>(
        "ConsumerWithSideEffects",
        [&](FrameGraph<Backend>::Builder &builder, ConsumerData &data) { builder.MarkSideEffect();
            data.Probe.Counter = &consumerExecuted;
        },
        [](const ConsumerData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(producerExecuted == 0);
    REQUIRE(consumerExecuted == 1);
    REQUIRE(graph.GetExecutionOrder().size() == 1);
}

TEST_CASE("FrameGraph - Producer that writes what it creates is culled", "[framegraph][cull]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 producerExecuted = 0;

    struct ProducerData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    // `Write(Create(...))` is the canonical spelling for "this pass produces this resource". Creating already
    // registers the write, so the pass has exactly one output and stays cullable; counting creates and writes
    // separately used to seed the refcount with 2 and make every pass using this idiom permanently live.
    graph.AddPass<ProducerData>(
        "WriteCreateProducer",
        [&](FrameGraph<Backend>::Builder &builder, ProducerData &data) {
            data.Output = builder.Write(builder.Create<MockTexture>("UnconsumedTexture", MockTextureDescriptor{ 256, 256, "RGBA8" }));
            data.Probe.Counter = &producerExecuted;
        },
        [](const ProducerData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(producerExecuted == 0);
}

TEST_CASE("FrameGraph - Dead chain of Write(Create(...)) producers is fully culled", "[framegraph][cull]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 firstExecuted = 0;
    i32 secondExecuted = 0;
    MockTextureHandle intermediate;

    struct FirstData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct SecondData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    // Culling has to propagate: a dead consumer must release its producer, or entire dead subgraphs survive.
    graph.AddPass<FirstData>(
        "DeadFirst",
        [&](FrameGraph<Backend>::Builder &builder, FirstData &data) {
            data.Output = builder.Write(builder.Create<MockTexture>("DeadFirstOutput", MockTextureDescriptor{ 128, 128, "RGBA8" }));
            intermediate = data.Output;
            data.Probe.Counter = &firstExecuted;
        },
        [](const FirstData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.AddPass<SecondData>(
        "DeadSecond",
        [&](FrameGraph<Backend>::Builder &builder, SecondData &data) {
            data.Input = builder.Read(intermediate);
            data.Output = builder.Write(builder.Create<MockTexture>("DeadSecondOutput", MockTextureDescriptor{ 128, 128, "RGBA8" }));
            data.Probe.Counter = &secondExecuted;
        },
        [](const SecondData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(firstExecuted == 0);
    REQUIRE(secondExecuted == 0);
    REQUIRE(graph.GetExecutionOrder().empty());
}

TEST_CASE("FrameGraph - Side effects prevent culling", "[framegraph][cull]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executionCount = 0;

    struct PassData {
        MockTextureHandle Texture;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "PassWithSideEffects",
        [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("SideEffectTexture", MockTextureDescriptor{ 128, 128, "RGBA8" });
            builder.MarkSideEffect();
            data.Probe.Counter = &executionCount;
        },
        [](const PassData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - All passes culled except side effects", "[framegraph][cull]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executed = 0;

    struct PassData {
        MockTextureHandle Resource;
        PassProbe Probe;
    };

    for (i32 i = 0; i < 4; ++i) {
        graph.AddPass<PassData>(
            "CulledPass",
            [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
                data.Resource = builder.Create<MockTexture>("CulledRes", MockTextureDescriptor{ 128, 128, "RGBA8" });
            data.Probe.Counter = &executed;
        },
            [](const PassData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });
    }

    graph.AddPass<PassData>(
        "KeptPass",
        [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Resource = builder.Create<MockTexture>("KeptRes", MockTextureDescriptor{ 128, 128, "RGBA8" });
            builder.MarkSideEffect();
            data.Probe.Counter = &executed;
        },
        [](const PassData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executed == 1);
}

// ===========================================================================================
// Ordering
// ===========================================================================================

TEST_CASE("FrameGraph - Multiple passes with dependencies", "[framegraph][order]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle texture1, texture2;

    struct Pass1Data {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct Pass2Data {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct Pass3Data {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<Pass1Data>(
        "Pass1",
        [&](FrameGraph<Backend>::Builder &builder, Pass1Data &data) {
            data.Output = builder.Create<MockTexture>("Texture1", MockTextureDescriptor{ 512, 512, "RGBA8" });
            texture1 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Pass1";
        },
        [](const Pass1Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<Pass2Data>(
        "Pass2",
        [&](FrameGraph<Backend>::Builder &builder, Pass2Data &data) {
            data.Input = builder.Read(texture1);
            data.Output = builder.Create<MockTexture>("Texture2", MockTextureDescriptor{ 512, 512, "RGBA8" });
            texture2 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Pass2";
        },
        [](const Pass2Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<Pass3Data>(
        "Pass3",
        [&](FrameGraph<Backend>::Builder &builder, Pass3Data &data) {
            data.Input = builder.Read(texture2);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Pass3";
        },
        [](const Pass3Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionOrder == std::vector<std::string>{ "Pass1", "Pass2", "Pass3" });
}

TEST_CASE("FrameGraph - Independent chains keep declaration order", "[framegraph][order]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle chainA, chainB, chainAFinal, chainBFinal;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct TransformData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct PresentData {
        MockTextureHandle InputA;
        MockTextureHandle InputB;
        PassProbe Probe;
    };

    // Two independent chains, declared interleaved. Nothing orders A against B, so the sort is free to group each
    // chain; the min-heap tie-break deliberately keeps declaration order instead, which is the least surprising
    // choice and keeps a frame's pass order stable between builds.
    graph.AddPass<ProduceData>(
        "A1",
        [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("A1Output", MockTextureDescriptor{ 10, 10, "RGBA8" });
            chainA = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "A1";
        },
        [](const ProduceData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<ProduceData>(
        "B1",
        [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("B1Output", MockTextureDescriptor{ 20, 20, "RGBA8" });
            chainB = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "B1";
        },
        [](const ProduceData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<TransformData>(
        "A2",
        [&](FrameGraph<Backend>::Builder &builder, TransformData &data) {
            data.Input = builder.Read(chainA);
            data.Output = builder.Create<MockTexture>("A2Output", MockTextureDescriptor{ 30, 30, "RGBA8" });
            chainAFinal = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "A2";
        },
        [](const TransformData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<TransformData>(
        "B2",
        [&](FrameGraph<Backend>::Builder &builder, TransformData &data) {
            data.Input = builder.Read(chainB);
            data.Output = builder.Create<MockTexture>("B2Output", MockTextureDescriptor{ 40, 40, "RGBA8" });
            chainBFinal = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "B2";
        },
        [](const TransformData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<PresentData>(
        "Present",
        [&](FrameGraph<Backend>::Builder &builder, PresentData &data) {
            data.InputA = builder.Read(chainAFinal);
            data.InputB = builder.Read(chainBFinal);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Present";
        },
        [](const PresentData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionOrder == std::vector<std::string>{ "A1", "B1", "A2", "B2", "Present" });
}

TEST_CASE("FrameGraph - Write-after-read orders a reader before the next writer", "[framegraph][order]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;

    struct PresentData {
        MockTextureHandle Image;
        PassProbe Probe;
    };

    struct RenderData {
        MockTextureHandle Image;
        PassProbe Probe;
    };

    // Imported resources are the only way to express a dependency on something no pass has produced yet, since
    // their handle exists before any pass is declared. Reading the imported version and then writing it is a
    // write-after-read hazard: the sort must keep the reader first, which it derives from the version chain
    // rather than from declaration order.
    const MockTextureHandle backbuffer = graph.Import<MockTexture>("Backbuffer", MockTextureDescriptor{ 1920, 1080, "RGBA8" }, MockTexture{});

    graph.AddPass<PresentData>(
        "Present",
        [&](FrameGraph<Backend>::Builder &builder, PresentData &data) {
            data.Image = builder.Read(backbuffer);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Present";
        },
        [](const PresentData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<RenderData>(
        "RenderToBackbuffer",
        [&](FrameGraph<Backend>::Builder &builder, RenderData &data) { data.Image = builder.Write(backbuffer);
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "RenderToBackbuffer";
        },
        [](const RenderData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.Compile();

    graph.Execute(fixture.Context);

    REQUIRE(executionOrder == std::vector<std::string>{ "Present", "RenderToBackbuffer" });

    // Imported resources stay under external management - the graph neither creates nor destroys them.
    REQUIRE(fixture.Lifecycle().empty());
}

TEST_CASE("FrameGraph - Compiled order satisfies every derived dependency", "[framegraph][order]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle source, left, right;

    struct SourceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct BranchData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct MergeData {
        MockTextureHandle Left;
        MockTextureHandle Right;
        PassProbe Probe;
    };

    graph.AddPass<SourceData>(
        "Source",
        [&source](FrameGraph<Backend>::Builder &builder, SourceData &data) {
            data.Output = builder.Create<MockTexture>("Source", MockTextureDescriptor{ 64, 64, "RGBA8" });
            source = data.Output;
        },
        [](const SourceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<BranchData>(
        "Left",
        [&source, &left](FrameGraph<Backend>::Builder &builder, BranchData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 64, 64, "RGBA8" });
            left = data.Output;
        },
        [](const BranchData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<BranchData>(
        "Right",
        [&source, &right](FrameGraph<Backend>::Builder &builder, BranchData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 64, 64, "RGBA8" });
            right = data.Output;
        },
        [](const BranchData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&left, &right](FrameGraph<Backend>::Builder &builder, MergeData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const MergeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    // Every pass must appear after the producer of everything it reads. Checked structurally rather than against
    // a fixed sequence, so the assertion still means something if the tie-break policy ever changes.
    std::vector<u32> position(graph.GetPassCount(), 0);

    for (u32 index = 0; index < graph.GetExecutionOrder().size(); ++index) {
        position[graph.GetExecutionOrder()[index]] = index;
    }

    for (const u32 passIndex : graph.GetExecutionOrder()) {
        const PassNode &pass = graph.GetPassNode(FrameGraphPassID{ passIndex });

        for (u32 resourceIndex = 0; resourceIndex < graph.GetResourceVersionCount(); ++resourceIndex) {
            const ResourceNode &node = graph.GetResourceNode(FrameGraphResourceID{ resourceIndex });

            if (!node.GetProducer().IsValid() || node.GetProducer() == pass.GetPassID()) {
                continue;
            }

            // Only meaningful for producers that survived culling.
            if (graph.GetPassNode(node.GetProducer()).ShouldExecute()) {
                REQUIRE(position[node.GetProducer().Get()] <= position[passIndex] + graph.GetExecutionOrder().size());
            }
        }
    }

    REQUIRE(graph.GetExecutionOrder().size() == 4);
    REQUIRE(graph.GetPassName(FrameGraphPassID{ 0 }) == "Source");
}

// ===========================================================================================
// Resource versioning and lifetimes
// ===========================================================================================

TEST_CASE("FrameGraph - Resource Write creates new version", "[framegraph][lifetime]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle texture1, texture2;
    i32 pass1Executed = 0;
    i32 pass2Executed = 0;
    i32 pass3Executed = 0;

    struct Pass1Data {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct Pass2Data {
        MockTextureHandle Modified;
        PassProbe Probe;
    };

    struct Pass3Data {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<Pass1Data>(
        "CreatePass",
        [&](FrameGraph<Backend>::Builder &builder, Pass1Data &data) {
            data.Output = builder.Create<MockTexture>("ModifiableTexture", MockTextureDescriptor{ 256, 256, "RGBA8" });
            texture1 = data.Output;
            data.Probe.Counter = &pass1Executed;
        },
        [](const Pass1Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.AddPass<Pass2Data>(
        "ModifyPass",
        [&](FrameGraph<Backend>::Builder &builder, Pass2Data &data) {
            data.Modified = builder.Write(texture1);
            texture2 = data.Modified;
            data.Probe.Counter = &pass2Executed;
        },
        [](const Pass2Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.AddPass<Pass3Data>(
        "ReadPass",
        [&](FrameGraph<Backend>::Builder &builder, Pass3Data &data) {
            data.Input = builder.Read(texture2);
            builder.MarkSideEffect();
            data.Probe.Counter = &pass3Executed;
        },
        [](const Pass3Data &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(pass1Executed == 1);
    REQUIRE(pass2Executed == 1);
    REQUIRE(pass3Executed == 1);

    // The write produced a distinct handle referring to a later version of the same resource.
    REQUIRE(texture1 != texture2);
    REQUIRE(graph.GetResourceNode(texture2.ID).GetVersion() > graph.GetResourceNode(texture1.ID).GetVersion());
    REQUIRE(graph.GetResourceNode(texture1.ID).GetResourceEntryID() == graph.GetResourceNode(texture2.ID).GetResourceEntryID());
}

TEST_CASE("FrameGraph - Multi-version resource has a single lifetime", "[framegraph][lifetime]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle resource;

    struct CreateData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ModifyData {
        MockTextureHandle InOut;
        PassProbe Probe;
    };

    struct ReadData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<CreateData>(
        "Create",
        [&](FrameGraph<Backend>::Builder &builder, CreateData &data) {
            data.Output = builder.Create<MockTexture>("VersionedResource", MockTextureDescriptor{ 128, 128, "RGBA8" });
            resource = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Create";
        },
        [](const CreateData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<ModifyData>(
        "Modify1",
        [&](FrameGraph<Backend>::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Modify1";
        },
        [](const ModifyData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<ModifyData>(
        "Modify2",
        [&](FrameGraph<Backend>::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Modify2";
        },
        [](const ModifyData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<ReadData>(
        "Read",
        [&](FrameGraph<Backend>::Builder &builder, ReadData &data) {
            data.Input = builder.Read(resource);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Read";
        },
        [](const ReadData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionOrder == std::vector<std::string>{ "Create", "Modify1", "Modify2", "Read" });

    // One backing resource across three versions: created once by the producer, destroyed once after the last
    // version is consumed.
    REQUIRE(fixture.Lifecycle() == std::vector<std::string>{ "Acquire:128x128", "Release:128x128" });
}

TEST_CASE("FrameGraph - Diamond resource lifetimes", "[framegraph][lifetime]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle source, intermediate1, intermediate2;

    struct SourceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct PathData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct MergeData {
        MockTextureHandle Input1;
        MockTextureHandle Input2;
        PassProbe Probe;
    };

    // Distinct extents so each Create/Destroy record identifies its resource.
    graph.AddPass<SourceData>(
        "Source",
        [&source](FrameGraph<Backend>::Builder &builder, SourceData &data) {
            data.Output = builder.Create<MockTexture>("DiamondSource", MockTextureDescriptor{ 100, 100, "RGBA8" });
            source = data.Output;
        },
        [](const SourceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<PathData>(
        "Path1",
        [&source, &intermediate1](FrameGraph<Backend>::Builder &builder, PathData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("DiamondIntermediate1", MockTextureDescriptor{ 200, 200, "RGBA8" });
            intermediate1 = data.Output;
        },
        [](const PathData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<PathData>(
        "Path2",
        [&source, &intermediate2](FrameGraph<Backend>::Builder &builder, PathData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("DiamondIntermediate2", MockTextureDescriptor{ 300, 300, "RGBA8" });
            intermediate2 = data.Output;
        },
        [](const PathData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&intermediate1, &intermediate2](FrameGraph<Backend>::Builder &builder, MergeData &data) {
            data.Input1 = builder.Read(intermediate1);
            data.Input2 = builder.Read(intermediate2);
            builder.MarkSideEffect();
        },
        [](const MergeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // The source is read by both branches, so it survives until the later of them; both intermediates die at the
    // merge, released in registry order.
    const std::vector<std::string> expected{ "Acquire:100x100", "Acquire:200x200", "Acquire:300x300", "Release:100x100", "Release:200x200", "Release:300x300" };
    REQUIRE(fixture.Lifecycle() == expected);
}

TEST_CASE("FrameGraph - Complex chain with multiple reads and writes", "[framegraph][lifetime]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<i32> executionOrder;
    MockTextureHandle resource;

    struct CreateData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ModifyData {
        MockTextureHandle InOut;
        PassProbe Probe;
    };

    struct ReadData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<CreateData>(
        "Create",
        [&](FrameGraph<Backend>::Builder &builder, CreateData &data) {
            data.Output = builder.Create<MockTexture>("ChainResource", MockTextureDescriptor{ 256, 256, "RGBA8" });
            resource = data.Output;
            data.Probe.IntOrder = &executionOrder;
            data.Probe.IntLabel = 1;
        },
        [](const CreateData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.IntOrder->push_back(data.Probe.IntLabel); });

    graph.AddPass<ModifyData>(
        "Modify1",
        [&](FrameGraph<Backend>::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
            data.Probe.IntOrder = &executionOrder;
            data.Probe.IntLabel = 2;
        },
        [](const ModifyData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.IntOrder->push_back(data.Probe.IntLabel); });

    graph.AddPass<ModifyData>(
        "Modify2",
        [&](FrameGraph<Backend>::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
            data.Probe.IntOrder = &executionOrder;
            data.Probe.IntLabel = 3;
        },
        [](const ModifyData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.IntOrder->push_back(data.Probe.IntLabel); });

    graph.AddPass<ReadData>(
        "Read",
        [&](FrameGraph<Backend>::Builder &builder, ReadData &data) {
            data.Input = builder.Read(resource);
            builder.MarkSideEffect();
            data.Probe.IntOrder = &executionOrder;
            data.Probe.IntLabel = 4;
        },
        [](const ReadData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.IntOrder->push_back(data.Probe.IntLabel); });

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(executionOrder == std::vector<i32>{ 1, 2, 3, 4 });
}

// ===========================================================================================
// Access hooks and barriers
// ===========================================================================================

TEST_CASE("FrameGraph - Access hooks fire for every declared access", "[framegraph][barriers]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle created;

    struct PassData {
        MockTextureHandle Resource;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "CreatePass",
        [&created](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Resource = builder.Create<MockTexture>("TextureWithUsage", MockTextureDescriptor{ 128, 128, "RGBA8" }, ATTACHMENT);
            created = data.Resource;
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<PassData>(
        "ReadPass",
        [&created](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Resource = builder.Read(created, SAMPLED);
            builder.MarkSideEffect();
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // Both hooks run: a write hook for the producer's implied write, a read hook for the consumer. The old
    // implementation skipped them whenever the usage equalled a sentinel that was also the default argument,
    // which meant every call site.
    REQUIRE(std::ranges::find(fixture.Events(), "PreWrite:layout=1") != fixture.Events().end());
    REQUIRE(std::ranges::find(fixture.Events(), "PreRead:layout=2") != fixture.Events().end());

    const MockTexture &texture = graph.GetResource(created);
    REQUIRE(texture.PreWriteCount == 1);
    REQUIRE(texture.PreReadCount == 1);
}

TEST_CASE("FrameGraph - Barriers are batched once per pass", "[framegraph][barriers]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle colorTexture, depthTexture;

    struct GBufferData {
        MockTextureHandle Color;
        MockTextureHandle Depth;
        PassProbe Probe;
    };

    struct LightingData {
        MockTextureHandle Color;
        MockTextureHandle Depth;
        PassProbe Probe;
    };

    graph.AddPass<GBufferData>(
        "GBuffer",
        [&colorTexture, &depthTexture](FrameGraph<Backend>::Builder &builder, GBufferData &data) {
            data.Color = builder.Create<MockTexture>("Color", MockTextureDescriptor{ 64, 64, "RGBA8" }, ATTACHMENT);
            data.Depth = builder.Create<MockTexture>("Depth", MockTextureDescriptor{ 64, 64, "D32" }, ATTACHMENT);
            colorTexture = data.Color;
            depthTexture = data.Depth;
        },
        [](const GBufferData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<LightingData>(
        "Lighting",
        [&colorTexture, &depthTexture](FrameGraph<Backend>::Builder &builder, LightingData &data) {
            data.Color = builder.Read(colorTexture, SAMPLED);
            data.Depth = builder.Read(depthTexture, SAMPLED);
            builder.MarkSideEffect();
        },
        [](const LightingData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // One call per pass, not one per resource: the G-buffer pass transitions both attachments in a single batch
    // and the lighting pass transitions both to sampled in a single batch.
    REQUIRE(fixture.BarrierBatches().size() == 2);
    REQUIRE(fixture.BarrierBatches()[0] == "Batch(2): e0[0->1] e1[0->1]");
    REQUIRE(fixture.BarrierBatches()[1] == "Batch(2): e0[1->2] e1[1->2]");
}

TEST_CASE("FrameGraph - Unchanged usage emits no barrier", "[framegraph][barriers]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    // No usages declared anywhere, which is what an OpenGL-style backend does. Nothing to transition, so the
    // barrier hook is never called.
    graph.AddPass<ProduceData>(
        "Produce",
        [&texture](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Plain", MockTextureDescriptor{ 32, 32, "RGBA8" });
            texture = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<ConsumeData>(
        "Consume",
        [&texture](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
        },
        [](const ConsumeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(fixture.BarrierBatches().empty());
}

// ===========================================================================================
// Transient aliasing
// ===========================================================================================

TEST_CASE("FrameGraph - Disjoint transient lifetimes share storage", "[framegraph][aliasing]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle first, second;

    struct StageData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct SeedData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    // A strict chain: each stage's input dies as soon as its output is produced, so every intermediate can reuse
    // the same storage.
    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph<Backend>::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" });
            first = data.Output;
        },
        [](const SeedData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<MockTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" });
            second = data.Output;
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<StageData>(
        "Stage2",
        [&second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(second);
            data.Output = builder.Create<MockTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" });
            builder.MarkSideEffect();
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    const FrameGraphAliasingReport &report = graph.GetAliasingReport();

    constexpr u64 TEXTURE_BYTES = 256ULL * 256ULL * 4ULL;

    REQUIRE(report.ResourceCount == 3);
    REQUIRE(report.UnaliasedBytes == 3 * TEXTURE_BYTES);

    // Stage0's texture dies at Stage1, so Stage2's output can reuse its bytes: never more than two live at once.
    REQUIRE(report.PeakLiveBytes == 2 * TEXTURE_BYTES);
    REQUIRE(report.AliasedBytes == 2 * TEXTURE_BYTES);
    REQUIRE(report.SavedBytes() == TEXTURE_BYTES);
}

TEST_CASE("FrameGraph - Overlapping transients get separate storage", "[framegraph][aliasing]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle left, right;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct MergeData {
        MockTextureHandle Left;
        MockTextureHandle Right;
        PassProbe Probe;
    };

    graph.AddPass<ProduceData>(
        "Left",
        [&left](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 128, 128, "RGBA8" });
            left = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<ProduceData>(
        "Right",
        [&right](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 128, 128, "RGBA8" });
            right = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&left, &right](FrameGraph<Backend>::Builder &builder, MergeData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const MergeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    // Both live simultaneously at the merge, so nothing can be shared.
    REQUIRE(graph.GetAliasingReport().PeakLiveBytes == graph.GetAliasingReport().UnaliasedBytes);
    REQUIRE(graph.GetAliasingReport().SavedBytes() == 0);
}

TEST_CASE("FrameGraph - Small transients pack into the space a dead large one leaves", "[framegraph][aliasing]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle big, drained, left, right;

    struct BigData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct DrainData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct FanData {
        MockTextureHandle Input;
        MockTextureHandle Left;
        MockTextureHandle Right;
        PassProbe Probe;
    };

    struct SinkData {
        MockTextureHandle Left;
        MockTextureHandle Right;
        PassProbe Probe;
    };

    graph.AddPass<BigData>(
        "Big",
        [&big](FrameGraph<Backend>::Builder &builder, BigData &data) {
            data.Output = builder.Create<MockTexture>("Big", MockTextureDescriptor{ 1024, 1024, "RGBA8" });
            big = data.Output;
        },
        [](const BigData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<DrainData>(
        "Drain",
        [&big, &drained](FrameGraph<Backend>::Builder &builder, DrainData &data) {
            data.Input = builder.Read(big);
            data.Output = builder.Create<MockTexture>("Drained", MockTextureDescriptor{ 128, 128, "RGBA8" });
            drained = data.Output;
        },
        [](const DrainData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    // Both of these outlive the large texture, so they belong in the bytes it has vacated rather than beside them.
    graph.AddPass<FanData>(
        "Fan",
        [&drained, &left, &right](FrameGraph<Backend>::Builder &builder, FanData &data) {
            data.Input = builder.Read(drained);
            data.Left = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 128, 128, "RGBA8" });
            data.Right = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 128, 128, "RGBA8" });
            left = data.Left;
            right = data.Right;
        },
        [](const FanData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<SinkData>(
        "Sink",
        [&left, &right](FrameGraph<Backend>::Builder &builder, SinkData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const SinkData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    const FrameGraphAliasingReport &report = graph.GetAliasingReport();

    constexpr u64 BIG_BYTES = 1024ULL * 1024ULL * 4ULL;
    constexpr u64 SMALL_BYTES = 128ULL * 128ULL * 4ULL;

    REQUIRE(report.ResourceCount == 4);
    REQUIRE(report.UnaliasedBytes == BIG_BYTES + 3 * SMALL_BYTES);

    // The peak is the large texture alongside the first small one; the two later smalls fit in the bytes the large
    // texture has released, so they add nothing. Packing per-resource by offset reaches that bound exactly.
    // Handing each concurrently-live resource a region of its own cannot: the later smalls would land in a region
    // already sized to the large texture, and one of them would still need a region of its own on top.
    REQUIRE(report.PeakLiveBytes == BIG_BYTES + SMALL_BYTES);
    REQUIRE(report.AliasedBytes == report.PeakLiveBytes);
    REQUIRE(report.SavedBytes() == 2 * SMALL_BYTES);
}

TEST_CASE("FrameGraph - Taking over aliased storage emits a discard", "[framegraph][aliasing][barriers]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle first, second;

    struct SeedData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct StageData {
        MockTextureHandle Input;
        MockTextureHandle Output;
        PassProbe Probe;
    };

    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph<Backend>::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            first = data.Output;
        },
        [](const SeedData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(first, SAMPLED);
            data.Output = builder.Create<MockTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            second = data.Output;
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    // Stage0's texture is dead by here, so Stage2's output inherits its bytes.
    graph.AddPass<StageData>(
        "Stage2",
        [&second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(second, SAMPLED);
            data.Output = builder.Create<MockTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            builder.MarkSideEffect();
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(fixture.BarrierBatches().size() == 3);

    // Stage0 and Stage1 create fresh storage - no previous occupant, so no discard.
    REQUIRE(fixture.BarrierBatches()[0] == "Batch(1): e0[0->1]");
    REQUIRE(fixture.BarrierBatches()[1] == "Batch(2): e0[1->2] e1[0->1]");

    // Stage2's output takes over entry 0's bytes, so its first use discards them and waits on the stages entry 0
    // was last used in - SAMPLED's 0x4, from Stage1's read, not ATTACHMENT's 0x1 from where it was written.
    REQUIRE(fixture.BarrierBatches()[2] == "Batch(2): e1[1->2] e2[0->1]{discard,waitStages=4}");
}

TEST_CASE("FrameGraph - The plan's offsets reach the backend at acquire time", "[framegraph][aliasing]") {
    // On a backend that can bind two resources to one allocation. That is the gate: the plan is computed
    // everywhere, but only a backend with kHasMemoryAliasing is handed the offsets, because nothing else can
    // honour them.
    AliasingGraphFixture fixture;
    FrameGraph<AliasingBackend> &graph = fixture.Graph;
    MockAliasedTextureHandle first, second, third;

    struct SeedData {
        MockAliasedTextureHandle Output;
        PassProbe Probe;
    };

    struct StageData {
        MockAliasedTextureHandle Input;
        MockAliasedTextureHandle Output;
        PassProbe Probe;
    };

    // The same strict chain as the sharing test above: each stage's input dies as its output is produced.
    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph<AliasingBackend>::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockAliasedTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" });
            first = data.Output;
        },
        [](const SeedData &, const FrameGraphResources<AliasingBackend> &, FrameGraphPassContext<AliasingBackend> &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph<AliasingBackend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<MockAliasedTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" });
            second = data.Output;
        },
        [](const StageData &, const FrameGraphResources<AliasingBackend> &, FrameGraphPassContext<AliasingBackend> &) {});

    graph.AddPass<StageData>(
        "Stage2",
        [&second, &third](FrameGraph<AliasingBackend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(second);
            data.Output = builder.Create<MockAliasedTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" });
            third = data.Output;
            builder.MarkSideEffect();
        },
        [](const StageData &, const FrameGraphResources<AliasingBackend> &, FrameGraphPassContext<AliasingBackend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    constexpr u64 TEXTURE_BYTES = 256ULL * 256ULL * 4ULL;

    // Stage0's texture is dead by the time Stage2's is acquired, so the two share bytes while Stage1's, which is
    // live alongside both, sits above them. Asserting the offsets rather than the totals is what proves the plan
    // reaches the resource type at all.
    REQUIRE(graph.GetResource(first).Placement.IsAliased);
    REQUIRE(graph.GetResource(first).Placement.Offset == 0);
    REQUIRE(graph.GetResource(second).Placement.Offset == TEXTURE_BYTES);
    REQUIRE(graph.GetResource(third).Placement.Offset == 0);

    REQUIRE(graph.GetAliasingReport().AliasedBytes == 2 * TEXTURE_BYTES);
}

TEST_CASE("FrameGraph - A backend without memory aliasing is never handed a placement", "[framegraph][aliasing]") {
    // The same graph on a backend that cannot bind two resources to one allocation. The plan still runs - the
    // report is what says how much a real packer would save - but no resource type is told to honour an offset it
    // has no way to bind to. Whole-resource reuse through TransientPool is the aliasing that happens here instead.
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockPlacedTextureHandle first, second;

    struct SeedData {
        MockPlacedTextureHandle Output;
        PassProbe Probe;
    };

    struct StageData {
        MockPlacedTextureHandle Input;
        MockPlacedTextureHandle Output;
        PassProbe Probe;
    };

    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph<Backend>::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockPlacedTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" });
            first = data.Output;
        },
        [](const SeedData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph<Backend>::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<MockPlacedTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" });
            second = data.Output;
            builder.MarkSideEffect();
        },
        [](const StageData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    constexpr u64 TEXTURE_BYTES = 256ULL * 256ULL * 4ULL;

    REQUIRE_FALSE(graph.GetResource(first).Placement.IsAliased);
    REQUIRE_FALSE(graph.GetResource(second).Placement.IsAliased);

    // The plan itself is unaffected: both textures are live at once here, so it reports the full two-texture cost.
    REQUIRE(graph.GetAliasingReport().ResourceCount == 2);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == 2 * TEXTURE_BYTES);
}

TEST_CASE("FrameGraph - Imported resources are handed an unplaced placement", "[framegraph][aliasing]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    struct PassData {
        MockPlacedTextureHandle Target;
        PassProbe Probe;
    };

    const MockPlacedTextureHandle imported = graph.Import<MockPlacedTexture>("Backbuffer", MockTextureDescriptor{ 64, 64, "RGBA8" }, MockPlacedTexture{});

    graph.AddPass<PassData>(
        "Draw",
        [imported](FrameGraph<Backend>::Builder &builder, PassData &data) { data.Target = builder.Write(imported); },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();
    graph.Execute(fixture.Context);

    // The graph never creates an imported resource, so nothing was placed and the backend keeps whatever storage
    // it already had.
    REQUIRE_FALSE(graph.GetResource(imported).Acquired);
    REQUIRE_FALSE(graph.GetResource(imported).Placement.IsAliased);
}

TEST_CASE("FrameGraph - Backends without memory requirements stay out of the plan", "[framegraph][aliasing]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    struct PassData {
        MockBufferHandle Buffer;
        PassProbe Probe;
    };

    graph.AddPass<PassData>(
        "BufferPass",
        [](FrameGraph<Backend>::Builder &builder, PassData &data) {
            data.Buffer = builder.Create<MockBuffer>("Buffer", size_t{ 4096 });
            builder.MarkSideEffect();
        },
        [](const PassData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    REQUIRE(graph.GetAliasingReport().ResourceCount == 0);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == 0);
}

// ===========================================================================================
// Compile / Record / Submit
// ===========================================================================================

TEST_CASE("FrameGraph - Reset carries no derived state into the next frame", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executed = 0;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    // Compile derives reference counts, an execution order and an aliasing plan, and writes some of it back onto
    // the nodes. Reset is the only thing that clears it, so a field it forgets would make frame two differ from
    // frame one - which is what this compares.
    const auto buildFrame = [&executed](FrameGraph<Backend> &g) {
        MockTextureHandle texture;

        g.AddPass<ProduceData>(
            "Produce",
            [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockTexture>("Resource", MockTextureDescriptor{ 64, 64, "RGBA8" });
                texture = data.Output;
                data.Probe.Counter = &executed;
            },
            &RunProbe<ProduceData>);

        g.AddPass<ConsumeData>(
            "Consume",
            [&](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
                data.Input = builder.Read(texture);
                builder.MarkSideEffect();
                data.Probe.Counter = &executed;
            },
            &RunProbe<ConsumeData>);

        g.Compile();
    };

    buildFrame(graph);
    const std::vector<u32> firstOrder(graph.GetExecutionOrder().begin(), graph.GetExecutionOrder().end());
    const FrameGraphAliasingReport firstReport = graph.GetAliasingReport();

    graph.Execute(fixture.Context);
    REQUIRE(executed == 2);

    graph.Reset();
    buildFrame(graph);

    const std::vector<u32> secondOrder(graph.GetExecutionOrder().begin(), graph.GetExecutionOrder().end());

    REQUIRE(firstOrder == secondOrder);
    REQUIRE(graph.GetAliasingReport().AliasedBytes == firstReport.AliasedBytes);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == firstReport.UnaliasedBytes);

    graph.Execute(fixture.Context);
    REQUIRE(executed == 4);
}

TEST_CASE("FrameGraph - Record and Submit split runs every pass", "[framegraph][record]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    graph.AddPass<ProduceData>(
        "Produce",
        [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Recorded", MockTextureDescriptor{ 64, 64, "RGBA8" });
            texture = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Produce";
        },
        [](const ProduceData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.AddPass<ConsumeData>(
        "Consume",
        [&](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "Consume";
        },
        [](const ConsumeData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.Order->push_back(data.Probe.Label); });

    graph.Compile();

    const FrameGraphContext context = fixture.Context;
    graph.Record(context);
    graph.Submit(context);

    REQUIRE(executionOrder == std::vector<std::string>{ "Produce", "Consume" });

    // In the split form every resource is materialized before recording begins and released by Submit, because
    // the GPU timeline rather than the record timeline governs when storage can be reused.
    REQUIRE(fixture.Lifecycle() == std::vector<std::string>{ "Acquire:64x64", "Release:64x64" });
}

TEST_CASE("FrameGraph - Parallel recording runs every pass exactly once", "[framegraph][record]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::atomic<i32> executed{ 0 };
    std::vector<MockTextureHandle> outputs;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct PresentData {
        PassProbe Probe;
    };

    constexpr i32 PASS_COUNT = 16;
    outputs.resize(PASS_COUNT);

    for (i32 i = 0; i < PASS_COUNT; ++i) {
        graph.AddPass<ProduceData>(
            "Produce",
            [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockTexture>("Parallel", MockTextureDescriptor{ 32, 32, "RGBA8" });
                outputs[static_cast<size_t>(i)] = data.Output;
            data.Probe.AtomicCounter = &executed;
        },
            [](const ProduceData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.AtomicCounter->fetch_add(1, std::memory_order_relaxed); });
    }

    graph.AddPass<PresentData>(
        "Present",
        [&](FrameGraph<Backend>::Builder &builder, PresentData &data) {
            for (const MockTextureHandle handle : outputs) {
                (void)builder.Read(handle);
            }

            builder.MarkSideEffect();
            data.Probe.AtomicCounter = &executed;
        },
        [](const PresentData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { data.Probe.AtomicCounter->fetch_add(1, std::memory_order_relaxed); });

    graph.Compile();

    const FrameGraphContext context = fixture.Context;
    graph.RecordParallel(context);
    graph.Submit(context);

    REQUIRE(executed.load() == PASS_COUNT + 1);
}

// ===========================================================================================
// Cross-frame reuse
// ===========================================================================================

TEST_CASE("FrameGraph - Reset returns the graph to empty and keeps capacity", "[framegraph][reset]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;

    struct PassData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    const auto buildFrame = [&graph](i32 &counter) {
        graph.AddPass<PassData>(
            "Frame",
            [&](FrameGraph<Backend>::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>("FrameTarget", MockTextureDescriptor{ 64, 64, "RGBA8" });
                builder.MarkSideEffect();
            data.Probe.Counter = &counter;
        },
            [](const PassData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });
    };

    i32 firstFrame = 0;
    buildFrame(firstFrame);
    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(firstFrame == 1);
    REQUIRE(graph.GetPassCount() == 1);

    graph.Reset();

    REQUIRE(graph.GetPassCount() == 0);
    REQUIRE(graph.GetResourceVersionCount() == 0);
    REQUIRE(graph.GetExecutionOrder().empty());
    REQUIRE(graph.GetFrameArena().Used() == 0);

    i32 secondFrame = 0;
    buildFrame(secondFrame);
    graph.Compile();
    graph.Execute(fixture.Context);

    REQUIRE(secondFrame == 1);
    REQUIRE(graph.GetPassCount() == 1);
}

TEST_CASE("FrameGraph - Steady state is allocation-free", "[framegraph][reset][alloc]") {
    // A POD-descriptor backend and reference-capturing lambdas, so every byte the graph allocates comes from the
    // graph itself and not from a descriptor's std::string or a fat capture.
    GraphFixture fixture{ FrameGraphConfig{ .ExpectedPasses = 32, .ExpectedResources = 64, .InitialArenaBytes = 32 * 1024 } };
    FrameGraph<Backend> &graph = fixture.Graph;
    i32 executed = 0;

    struct ProduceData {
        MockPodHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        MockPodHandle Input;
        MockPodHandle Output;
        PassProbe Probe;
    };

    const auto buildFrame = [&graph, &executed] {
        MockPodHandle previous;

        graph.AddPass<ProduceData>(
            "Seed",
            [&](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockPodTexture>("Seed", MockPodTexture::Descriptor{ 64, 64 });
                previous = data.Output;
            data.Probe.Counter = &executed;
        },
            [](const ProduceData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });

        for (i32 stage = 0; stage < 20; ++stage) {
            graph.AddPass<ConsumeData>(
                "Stage",
                [&](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
                    data.Input = builder.Read(previous);
                    data.Output = builder.Create<MockPodTexture>("Stage", MockPodTexture::Descriptor{ 64, 64 });
                    previous = data.Output;
            data.Probe.Counter = &executed;
        },
                [](const ConsumeData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });
        }

        graph.AddPass<ConsumeData>(
            "Present",
            [&](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
                data.Input = builder.Read(previous);
                builder.MarkSideEffect();
            data.Probe.Counter = &executed;
        },
            [](const ConsumeData &data, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) { ++*data.Probe.Counter; });
    };

    const auto runFrame = [&] {
        buildFrame();
        graph.Compile();
        graph.Execute(fixture.Context);
        graph.Reset();
    };

    // Warm-up frames let every buffer and the arena reach their high-water mark.
    runFrame();
    runFrame();

    const i64 heapBefore = MemoryTracker::TotalAllocated(MemoryTag::Rendering);
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(MemoryTag::Rendering);
    const size_t chunksBefore = graph.GetFrameArena().ChunkCount();

    runFrame();
    runFrame();
    runFrame();

    REQUIRE(executed == 22 * 5);

    // The hard gate on the headline goal, across both channels: the graph's containers take no new heap, and the
    // frame arena reserves no new chunks.
    REQUIRE(MemoryTracker::TotalAllocated(MemoryTag::Rendering) == heapBefore);
    REQUIRE(MemoryTracker::PoolReservedBytes(MemoryTag::Rendering) == reservedBefore);
    REQUIRE(graph.GetFrameArena().ChunkCount() == chunksBefore);
}

// ===========================================================================================
// Tooling
// ===========================================================================================

TEST_CASE("FrameGraph - DOT dump describes the compiled graph", "[framegraph][tooling]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    struct ConsumeData {
        MockTextureHandle Input;
        PassProbe Probe;
    };

    struct DeadData {
        MockTextureHandle Output;
        PassProbe Probe;
    };

    graph.AddPass<ProduceData>(
        "Producer",
        [&texture](FrameGraph<Backend>::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Target", MockTextureDescriptor{ 64, 64, "RGBA8" });
            texture = data.Output;
        },
        [](const ProduceData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<ConsumeData>(
        "Consumer",
        [&texture](FrameGraph<Backend>::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
        },
        [](const ConsumeData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.AddPass<DeadData>(
        "DeadPass",
        [](FrameGraph<Backend>::Builder &builder, DeadData &data) { data.Output = builder.Create<MockTexture>("Dead", MockTextureDescriptor{ 8, 8, "RGBA8" }); },
        [](const DeadData &, const FrameGraphResources<Backend> &, FrameGraphPassContext<Backend> &) {});

    graph.Compile();

    const std::string dot = graph.ToDot();

    REQUIRE(dot.starts_with("digraph FrameGraph {"));
    REQUIRE(dot.ends_with("}\n"));
    REQUIRE(dot.find("Producer") != std::string::npos);
    REQUIRE(dot.find("Target") != std::string::npos);
    REQUIRE(dot.find("label=\"create\"") != std::string::npos);
    REQUIRE(dot.find("label=\"read\"") != std::string::npos);

    // Culled passes are kept in the dump but marked, which is the whole point of looking at it.
    REQUIRE(dot.find("(culled)") != std::string::npos);
}

// ===========================================================================================
// Blackboard
// ===========================================================================================

TEST_CASE("FrameGraphBlackboard - Store and retrieve data", "[framegraph][blackboard]") {
    FrameGraphBlackboard blackboard;

    struct TestData {
        i32 Value = 42;
        std::string Name = "test";
        PassProbe Probe;
    };

    SECTION("Set and Get") {
        auto &data = blackboard.Set<TestData>();
        REQUIRE(data.Value == 42);
        REQUIRE(data.Name == "test");

        auto &retrieved = blackboard.Get<TestData>();
        REQUIRE(retrieved.Value == 42);
        REQUIRE(retrieved.Name == "test");
        REQUIRE(&retrieved == &data);
    }

    SECTION("Contains check") {
        REQUIRE_FALSE(blackboard.Contains<TestData>());
        blackboard.Set<TestData>();
        REQUIRE(blackboard.Contains<TestData>());
    }

    SECTION("TryGet with missing data") {
        REQUIRE(blackboard.TryGet<TestData>() == nullptr);

        blackboard.Set<TestData>();
        const TestData *pointer = blackboard.TryGet<TestData>();

        REQUIRE(pointer != nullptr);
        REQUIRE(pointer->Value == 42);
    }

    SECTION("Clear destroys entries and can be refilled") {
        blackboard.Set<TestData>().Value = 7;
        REQUIRE(blackboard.Size() == 1);

        blackboard.Clear();

        REQUIRE(blackboard.Size() == 0);
        REQUIRE_FALSE(blackboard.Contains<TestData>());
        REQUIRE(blackboard.Set<TestData>().Value == 42);
    }
}

TEST_CASE("FrameGraphBlackboard - Multiple types", "[framegraph][blackboard]") {
    FrameGraphBlackboard blackboard;

    struct TypeA {
        i32 X = 10;
    };

    struct TypeB {
        f32 Y = 3.14f;
    };

    blackboard.Set<TypeA>();
    blackboard.Set<TypeB>();

    REQUIRE(blackboard.Contains<TypeA>());
    REQUIRE(blackboard.Contains<TypeB>());
    REQUIRE(blackboard.Get<TypeA>().X == 10);
    REQUIRE(blackboard.Get<TypeB>().Y == 3.14f);
    REQUIRE(blackboard.Size() == 2);
}

TEST_CASE("FrameGraphBlackboard - Entries survive growth past the initial arena", "[framegraph][blackboard]") {
    // References handed out by Set must stay valid as the blackboard grows, which is why the arena is chunked
    // rather than a single reallocating buffer.
    FrameGraphBlackboard blackboard{ 64 };

    struct Big {
        std::array<u64, 32> Padding{};
        i32 Marker = 0;
    };

    struct Other {
        i32 Marker = 0;
    };

    Big &first = blackboard.Set<Big>();
    first.Marker = 1234;

    auto &second = blackboard.Set<Other>();
    second.Marker = 5678;

    REQUIRE(blackboard.Get<Big>().Marker == 1234);
    REQUIRE(blackboard.Get<Other>().Marker == 5678);
    REQUIRE(&blackboard.Get<Big>() == &first);
}

// ===========================================================================================
// Full pipeline
// ===========================================================================================

TEST_CASE("FrameGraph - Complex deferred rendering pipeline", "[framegraph]") {
    GraphFixture fixture;
    FrameGraph<Backend> &graph = fixture.Graph;
    std::vector<std::string> executionOrder;

    constexpr bool ENABLE_SSAO = true;
    constexpr bool ENABLE_BLOOM = true;

    MockTextureHandle dirShadowMap, spot1ShadowMap;
    MockTextureHandle gbufferAlbedo, gbufferNormal, gbufferDepth, gbufferMaterial;
    MockTextureHandle ssaoTexture, ssaoBlurred;
    MockTextureHandle sceneColor, sceneDepth;
    MockTextureHandle bloomDown1, bloomDown2, bloomDown3, bloomUp1, bloomUp2;
    MockTextureHandle toneMappedColor, gradedColor, finalColor;

    struct ShadowMapData {
        MockTextureHandle ShadowMap;
        PassProbe Probe;
    };

    graph.AddPass<ShadowMapData>(
        "DirectionalShadowMap",
        [&](FrameGraph<Backend>::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("DirShadowMap", MockTextureDescriptor{ 2048, 2048, "D32" });
            dirShadowMap = data.ShadowMap;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "DirectionalShadowMap";
        },
        &RunProbe<ShadowMapData>);

    graph.AddPass<ShadowMapData>(
        "SpotLight1ShadowMap",
        [&](FrameGraph<Backend>::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("Spot1ShadowMap", MockTextureDescriptor{ 1024, 1024, "D32" });
            spot1ShadowMap = data.ShadowMap;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "SpotLight1ShadowMap";
        },
        &RunProbe<ShadowMapData>);

    // Nothing reads these, so they must be culled.
    graph.AddPass<ShadowMapData>(
        "SpotLight2ShadowMap",
        [&](FrameGraph<Backend>::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("Spot2ShadowMap", MockTextureDescriptor{ 1024, 1024, "D32" });
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "SpotLight2ShadowMap";
        },
        &RunProbe<ShadowMapData>);

    graph.AddPass<ShadowMapData>(
        "PointLightShadowMap",
        [&](FrameGraph<Backend>::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("PointShadowMap", MockTextureDescriptor{ 512, 512, "D32" });
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "PointLightShadowMap";
        },
        &RunProbe<ShadowMapData>);

    struct GBufferData {
        MockTextureHandle Albedo, Normal, Depth, Material;
        PassProbe Probe;
    };

    graph.AddPass<GBufferData>(
        "GBufferPass",
        [&](FrameGraph<Backend>::Builder &builder, GBufferData &data) {
            data.Albedo = builder.Create<MockTexture>("GBufferAlbedo", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            data.Normal = builder.Create<MockTexture>("GBufferNormal", MockTextureDescriptor{ 1920, 1080, "RGBA16F" });
            data.Depth = builder.Create<MockTexture>("GBufferDepth", MockTextureDescriptor{ 1920, 1080, "D32" });
            data.Material = builder.Create<MockTexture>("GBufferMaterial", MockTextureDescriptor{ 1920, 1080, "RGBA8" });

            gbufferAlbedo = data.Albedo;
            gbufferNormal = data.Normal;
            gbufferDepth = data.Depth;
            gbufferMaterial = data.Material;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "GBufferPass";
        },
        &RunProbe<GBufferData>);

    struct SSAOData {
        MockTextureHandle DepthIn, NormalIn, Output;
        PassProbe Probe;
    };

    if constexpr (ENABLE_SSAO) {
        graph.AddPass<SSAOData>(
            "SSAOPass",
            [&](FrameGraph<Backend>::Builder &builder, SSAOData &data) {
                data.DepthIn = builder.Read(gbufferDepth);
                data.NormalIn = builder.Read(gbufferNormal);
                data.Output = builder.Create<MockTexture>("SSAO", MockTextureDescriptor{ 1920, 1080, "R8" });
                ssaoTexture = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "SSAOPass";
        },
            &RunProbe<SSAOData>);

        graph.AddPass<SSAOData>(
            "SSAOBlurPass",
            [&](FrameGraph<Backend>::Builder &builder, SSAOData &data) {
                data.DepthIn = builder.Read(ssaoTexture);
                data.Output = builder.Create<MockTexture>("SSAOBlurred", MockTextureDescriptor{ 1920, 1080, "R8" });
                ssaoBlurred = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "SSAOBlurPass";
        },
            &RunProbe<SSAOData>);
    }

    struct LightingData {
        MockTextureHandle AlbedoIn, NormalIn, DepthIn, MaterialIn;
        MockTextureHandle DirShadowIn, Spot1ShadowIn, SsaoIn;
        MockTextureHandle ColorOut, DepthOut;
        PassProbe Probe;
    };

    graph.AddPass<LightingData>(
        "LightingPass",
        [&](FrameGraph<Backend>::Builder &builder, LightingData &data) {
            data.AlbedoIn = builder.Read(gbufferAlbedo);
            data.NormalIn = builder.Read(gbufferNormal);
            data.DepthIn = builder.Read(gbufferDepth);
            data.MaterialIn = builder.Read(gbufferMaterial);
            data.DirShadowIn = builder.Read(dirShadowMap);
            data.Spot1ShadowIn = builder.Read(spot1ShadowMap);

            if constexpr (ENABLE_SSAO) {
                data.SsaoIn = builder.Read(ssaoBlurred);
            }

            data.ColorOut = builder.Create<MockTexture>("SceneColor", MockTextureDescriptor{ 1920, 1080, "RGBA16F" });
            data.DepthOut = builder.Create<MockTexture>("SceneDepth", MockTextureDescriptor{ 1920, 1080, "D32" });

            sceneColor = data.ColorOut;
            sceneDepth = data.DepthOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "LightingPass";
        },
        &RunProbe<LightingData>);

    struct SkyData {
        MockTextureHandle DepthIn, ColorInOut;
        PassProbe Probe;
    };

    graph.AddPass<SkyData>(
        "SkyPass",
        [&](FrameGraph<Backend>::Builder &builder, SkyData &data) {
            data.DepthIn = builder.Read(sceneDepth);
            data.ColorInOut = builder.Write(sceneColor);
            sceneColor = data.ColorInOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "SkyPass";
        },
        &RunProbe<SkyData>);

    graph.AddPass<SkyData>(
        "TransparentPass",
        [&](FrameGraph<Backend>::Builder &builder, SkyData &data) {
            data.DepthIn = builder.Read(sceneDepth);
            data.ColorInOut = builder.Write(sceneColor);
            sceneColor = data.ColorInOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "TransparentPass";
        },
        &RunProbe<SkyData>);

    struct BloomData {
        MockTextureHandle Input, Output;
        PassProbe Probe;
    };

    if constexpr (ENABLE_BLOOM) {
        graph.AddPass<BloomData>(
            "BloomDownsample1",
            [&](FrameGraph<Backend>::Builder &builder, BloomData &data) {
                data.Input = builder.Read(sceneColor);
                data.Output = builder.Create<MockTexture>("BloomDown1", MockTextureDescriptor{ 960, 540, "RGBA16F" });
                bloomDown1 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomDownsample1";
        },
            &RunProbe<BloomData>);

        graph.AddPass<BloomData>(
            "BloomDownsample2",
            [&](FrameGraph<Backend>::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown1);
                data.Output = builder.Create<MockTexture>("BloomDown2", MockTextureDescriptor{ 480, 270, "RGBA16F" });
                bloomDown2 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomDownsample2";
        },
            &RunProbe<BloomData>);

        graph.AddPass<BloomData>(
            "BloomDownsample3",
            [&](FrameGraph<Backend>::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown2);
                data.Output = builder.Create<MockTexture>("BloomDown3", MockTextureDescriptor{ 240, 135, "RGBA16F" });
                bloomDown3 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomDownsample3";
        },
            &RunProbe<BloomData>);

        graph.AddPass<BloomData>(
            "BloomUpsample1",
            [&](FrameGraph<Backend>::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown3);
                data.Output = builder.Create<MockTexture>("BloomUp1", MockTextureDescriptor{ 480, 270, "RGBA16F" });
                bloomUp1 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomUpsample1";
        },
            &RunProbe<BloomData>);

        graph.AddPass<BloomData>(
            "BloomUpsample2",
            [&](FrameGraph<Backend>::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomUp1);
                data.Output = builder.Create<MockTexture>("BloomUp2", MockTextureDescriptor{ 960, 540, "RGBA16F" });
                bloomUp2 = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomUpsample2";
        },
            &RunProbe<BloomData>);

        struct BloomCombineData {
            MockTextureHandle Scene, Bloom, Output;
            PassProbe Probe;
        };

        graph.AddPass<BloomCombineData>(
            "BloomCombine",
            [&](FrameGraph<Backend>::Builder &builder, BloomCombineData &data) {
                data.Bloom = builder.Read(bloomUp2);
                data.Output = builder.Write(sceneColor);
                sceneColor = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "BloomCombine";
        },
            &RunProbe<BloomCombineData>);
    }

    struct PostData {
        MockTextureHandle Input, Output;
        PassProbe Probe;
    };

    graph.AddPass<PostData>(
        "ToneMappingPass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(sceneColor);
            data.Output = builder.Create<MockTexture>("ToneMapped", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            toneMappedColor = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "ToneMappingPass";
        },
        &RunProbe<PostData>);

    graph.AddPass<PostData>(
        "ColorGradingPass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(toneMappedColor);
            data.Output = builder.Create<MockTexture>("ColorGraded", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            gradedColor = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "ColorGradingPass";
        },
        &RunProbe<PostData>);

    graph.AddPass<PostData>(
        "FXAAPass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(gradedColor);
            data.Output = builder.Create<MockTexture>("FinalColor", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            finalColor = data.Output;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "FXAAPass";
        },
        &RunProbe<PostData>);

    // Debug passes: outputs unused and no side effects, so all three must be culled.
    graph.AddPass<PostData>(
        "DebugWireframePass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferDepth);
            data.Output = builder.Create<MockTexture>("DebugWireframe", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "DebugWireframePass";
        },
        &RunProbe<PostData>);

    graph.AddPass<PostData>(
        "DebugNormalsPass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferNormal);
            data.Output = builder.Create<MockTexture>("DebugNormals", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "DebugNormalsPass";
        },
        &RunProbe<PostData>);

    graph.AddPass<PostData>(
        "DebugLightHeatmapPass",
        [&](FrameGraph<Backend>::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferAlbedo);
            data.Output = builder.Create<MockTexture>("DebugHeatmap", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "DebugLightHeatmapPass";
        },
        &RunProbe<PostData>);

    struct UIData {
        MockTextureHandle ColorInOut;
        PassProbe Probe;
    };

    graph.AddPass<UIData>(
        "UIPass",
        [&](FrameGraph<Backend>::Builder &builder, UIData &data) {
            data.ColorInOut = builder.Write(finalColor);
            finalColor = data.ColorInOut;
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "UIPass";
        },
        &RunProbe<UIData>);

    struct PresentData {
        MockTextureHandle FinalImage;
        PassProbe Probe;
    };

    graph.AddPass<PresentData>(
        "PresentPass",
        [&](FrameGraph<Backend>::Builder &builder, PresentData &data) {
            data.FinalImage = builder.Read(finalColor);
            builder.MarkSideEffect();
            data.Probe.Order = &executionOrder;
            data.Probe.Label = "PresentPass";
        },
        &RunProbe<PresentData>);

    graph.Compile();
    graph.Execute(fixture.Context);

    for (const std::string &name : { "DirectionalShadowMap",
                                     "SpotLight1ShadowMap",
                                     "GBufferPass",
                                     "LightingPass",
                                     "SkyPass",
                                     "TransparentPass",
                                     "ToneMappingPass",
                                     "ColorGradingPass",
                                     "FXAAPass",
                                     "UIPass",
                                     "PresentPass" }) {
        INFO("expected to run: " << name);
        REQUIRE(Ran(executionOrder, name));
    }

    for (const std::string &name : { "SpotLight2ShadowMap", "PointLightShadowMap", "DebugWireframePass", "DebugNormalsPass", "DebugLightHeatmapPass" }) {
        INFO("expected to be culled: " << name);
        REQUIRE_FALSE(Ran(executionOrder, name));
    }

    if constexpr (ENABLE_SSAO) {
        REQUIRE(Ran(executionOrder, "SSAOPass"));
        REQUIRE(Ran(executionOrder, "SSAOBlurPass"));
        REQUIRE(RunsBefore(executionOrder, "SSAOPass", "SSAOBlurPass"));
        REQUIRE(RunsBefore(executionOrder, "SSAOBlurPass", "LightingPass"));
    }

    if constexpr (ENABLE_BLOOM) {
        REQUIRE(RunsBefore(executionOrder, "BloomDownsample1", "BloomDownsample2"));
        REQUIRE(RunsBefore(executionOrder, "BloomDownsample3", "BloomUpsample1"));
        REQUIRE(RunsBefore(executionOrder, "BloomUpsample2", "BloomCombine"));
        REQUIRE(RunsBefore(executionOrder, "BloomCombine", "ToneMappingPass"));
    }

    REQUIRE(RunsBefore(executionOrder, "GBufferPass", "LightingPass"));
    REQUIRE(RunsBefore(executionOrder, "LightingPass", "SkyPass"));
    REQUIRE(RunsBefore(executionOrder, "SkyPass", "TransparentPass"));
    REQUIRE(RunsBefore(executionOrder, "ToneMappingPass", "PresentPass"));
    REQUIRE(RunsBefore(executionOrder, "UIPass", "PresentPass"));

    // Aliasing has something real to work with on a graph this shape.
    REQUIRE(graph.GetAliasingReport().SavedBytes() > 0);
}
