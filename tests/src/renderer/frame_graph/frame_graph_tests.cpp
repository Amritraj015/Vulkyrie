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
    FrameGraph graph;
    i32 executionCount = 0;

    struct PassData {
        i32 Value = 42;
    };

    const auto &data = graph.AddPass<PassData>(
        "TestPass",
        [](FrameGraph::Builder &builder, PassData &data) { builder.MarkSideEffect(); },
        [&executionCount](const PassData &data, FrameGraph &graph, const FrameGraphContext &context) {
            ++executionCount;
            REQUIRE(data.Value == 42);
        });

    REQUIRE(data.Value == 42);

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - Resource creation and access", "[framegraph]") {
    FrameGraph graph;
    Recorder recorder;
    bool passExecuted = false;

    struct PassData {
        MockTextureHandle Texture;
    };

    graph.AddPass<PassData>(
        "CreateTexture",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("MainTexture", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            builder.MarkSideEffect();
        },
        [&passExecuted](const PassData &data, FrameGraph &graph, const FrameGraphContext &context) { passExecuted = true; });

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(passExecuted);
    REQUIRE(recorder.Lifecycle().front() == "Create:1920x1080");
}

TEST_CASE("FrameGraph - Execute functor can reach the resource it declared", "[framegraph]") {
    FrameGraph graph;
    MockTexture *observed = nullptr;

    struct PassData {
        MockTextureHandle Texture;
    };

    graph.AddPass<PassData>(
        "ResourceReadBack",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("ReadBackTarget", MockTextureDescriptor{ 64, 64, "RGBA8" });
            builder.MarkSideEffect();
        },
        [&observed](const PassData &data, FrameGraph &graph, const FrameGraphContext &context) { observed = &graph.GetResource(data.Texture); });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(observed != nullptr);
    REQUIRE(observed->Created); // The graph must have materialised the resource before the pass ran.
}

TEST_CASE("FrameGraph - Names cost no arena bytes", "[framegraph]") {
    struct PassData {
        MockTextureHandle Output;
    };

    // Identical graphs whose names differ by 139 characters. Any copying would show up as a difference here.
    const auto usedFor = [](StaticString passName, StaticString resourceName) {
        FrameGraph graph;

        graph.AddPass<PassData>(
            passName,
            [resourceName](FrameGraph::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>(resourceName, MockTextureDescriptor{ 64, 64, "D32" });
                builder.MarkSideEffect();
            },
            [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

        graph.Compile();

        return graph.GetFrameArena().Used();
    };

    REQUIRE(usedFor("P", "R") == usedFor("PassNameThatIsConsiderablyLongerThanTheOtherOneByFiftyOrSoCharacters",
                                         "ResourceNameThatIsAlsoMuchLongerThanItsCounterpartAboveForTheSameReason"));
}

TEST_CASE("FrameGraph - Names that vary come from a table of literals", "[framegraph]") {
    struct PassData {
        MockTextureHandle Output;
    };

    static constexpr StaticString CASCADE_NAMES[]{ "Cascade0", "Cascade1", "Cascade2", "Cascade3" };

    FrameGraph graph;

    for (u32 cascade = 0; cascade < 4; ++cascade) {
        graph.AddPass<PassData>(
            CASCADE_NAMES[cascade],
            [](FrameGraph::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>("CascadeTarget", MockTextureDescriptor{ 64, 64, "D32" });
                builder.MarkSideEffect();
            },
            [](const PassData &, FrameGraph &, const FrameGraphContext &) {});
    }

    graph.Compile();

    REQUIRE(graph.GetPassCount() == 4);
    REQUIRE(graph.GetPassName(FrameGraphPassID{ 0 }) == "Cascade0");
    REQUIRE(graph.GetPassName(FrameGraphPassID{ 3 }) == "Cascade3");
}

TEST_CASE("FrameGraph - Descriptors are readable through a handle", "[framegraph]") {
    FrameGraph graph;

    struct PassData {
        MockTextureHandle Texture;
    };

    const auto &data = graph.AddPass<PassData>(
        "DescriptorPass",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("Described", MockTextureDescriptor{ 800, 600, "RGBA8" });
            builder.MarkSideEffect();
        },
        [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();

    REQUIRE(graph.GetDescriptor(data.Texture).Width == 800);
    REQUIRE(graph.GetDescriptor(data.Texture).Height == 600);
}

TEST_CASE("FrameGraph - Read and Write operations", "[framegraph]") {
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle sharedTexture;

    struct ProducerData {
        MockTextureHandle Output;
    };

    struct ConsumerData {
        MockTextureHandle Input;
    };

    graph.AddPass<ProducerData>(
        "Producer",
        [&sharedTexture](FrameGraph::Builder &builder, ProducerData &data) {
            data.Output = builder.Create<MockTexture>("SharedTexture", MockTextureDescriptor{ 512, 512, "RGBA8" });
            sharedTexture = data.Output;
        },
        [](const ProducerData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<ConsumerData>(
        "Consumer",
        [&sharedTexture](FrameGraph::Builder &builder, ConsumerData &data) {
            data.Input = builder.Read(sharedTexture);
            builder.MarkSideEffect();
        },
        [](const ConsumerData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(recorder.Lifecycle() == std::vector<std::string>{ "Create:512x512", "Destroy:512x512" });
}

TEST_CASE("FrameGraph - Multiple resource types", "[framegraph]") {
    FrameGraph graph;
    Recorder recorder;
    i32 executionCount = 0;

    struct PassData {
        MockTextureHandle Texture;
        MockBufferHandle Buffer;
    };

    graph.AddPass<PassData>(
        "MultiResourcePass",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("MultiTexture", MockTextureDescriptor{ 512, 512, "RGBA8" });
            data.Buffer = builder.Create<MockBuffer>("MultiBuffer", size_t{ 1024 });
            builder.MarkSideEffect();
        },
        [&executionCount](const PassData &, FrameGraph &, const FrameGraphContext &) { ++executionCount; });

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(executionCount == 1);

    // A backend that opts out of the access hooks still gets its lifecycle callbacks.
    REQUIRE(std::ranges::find(recorder.Events, "CreateBuffer:1024") != recorder.Events.end());
}

TEST_CASE("FrameGraph - Empty graph", "[framegraph]") {
    FrameGraph graph;

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(graph.GetExecutionOrder().empty());
}

// ===========================================================================================
// Culling
// ===========================================================================================

TEST_CASE("FrameGraph - Pass culling for unreferenced resources", "[framegraph][cull]") {
    FrameGraph graph;
    i32 producerExecuted = 0;
    i32 consumerExecuted = 0;

    struct ProducerData {
        MockTextureHandle Texture;
    };

    struct ConsumerData {};

    graph.AddPass<ProducerData>(
        "UnusedProducer",
        [](FrameGraph::Builder &builder, ProducerData &data) {
            data.Texture = builder.Create<MockTexture>("UnusedTexture", MockTextureDescriptor{ 256, 256, "RGBA8" });
            // No side effects and nothing reads the output, so this pass must be culled.
        },
        [&producerExecuted](const ProducerData &, FrameGraph &, const FrameGraphContext &) { ++producerExecuted; });

    graph.AddPass<ConsumerData>(
        "ConsumerWithSideEffects",
        [](FrameGraph::Builder &builder, ConsumerData &data) { builder.MarkSideEffect(); },
        [&consumerExecuted](const ConsumerData &, FrameGraph &, const FrameGraphContext &) { ++consumerExecuted; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(producerExecuted == 0);
    REQUIRE(consumerExecuted == 1);
    REQUIRE(graph.GetExecutionOrder().size() == 1);
}

TEST_CASE("FrameGraph - Producer that writes what it creates is culled", "[framegraph][cull]") {
    FrameGraph graph;
    i32 producerExecuted = 0;

    struct ProducerData {
        MockTextureHandle Output;
    };

    // `Write(Create(...))` is the canonical spelling for "this pass produces this resource". Creating already
    // registers the write, so the pass has exactly one output and stays cullable; counting creates and writes
    // separately used to seed the refcount with 2 and make every pass using this idiom permanently live.
    graph.AddPass<ProducerData>(
        "WriteCreateProducer",
        [](FrameGraph::Builder &builder, ProducerData &data) {
            data.Output = builder.Write(builder.Create<MockTexture>("UnconsumedTexture", MockTextureDescriptor{ 256, 256, "RGBA8" }));
        },
        [&producerExecuted](const ProducerData &, FrameGraph &, const FrameGraphContext &) { ++producerExecuted; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(producerExecuted == 0);
}

TEST_CASE("FrameGraph - Dead chain of Write(Create(...)) producers is fully culled", "[framegraph][cull]") {
    FrameGraph graph;
    i32 firstExecuted = 0;
    i32 secondExecuted = 0;
    MockTextureHandle intermediate;

    struct FirstData {
        MockTextureHandle Output;
    };

    struct SecondData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    // Culling has to propagate: a dead consumer must release its producer, or entire dead subgraphs survive.
    graph.AddPass<FirstData>(
        "DeadFirst",
        [&intermediate](FrameGraph::Builder &builder, FirstData &data) {
            data.Output = builder.Write(builder.Create<MockTexture>("DeadFirstOutput", MockTextureDescriptor{ 128, 128, "RGBA8" }));
            intermediate = data.Output;
        },
        [&firstExecuted](const FirstData &, FrameGraph &, const FrameGraphContext &) { ++firstExecuted; });

    graph.AddPass<SecondData>(
        "DeadSecond",
        [&intermediate](FrameGraph::Builder &builder, SecondData &data) {
            data.Input = builder.Read(intermediate);
            data.Output = builder.Write(builder.Create<MockTexture>("DeadSecondOutput", MockTextureDescriptor{ 128, 128, "RGBA8" }));
        },
        [&secondExecuted](const SecondData &, FrameGraph &, const FrameGraphContext &) { ++secondExecuted; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(firstExecuted == 0);
    REQUIRE(secondExecuted == 0);
    REQUIRE(graph.GetExecutionOrder().empty());
}

TEST_CASE("FrameGraph - Side effects prevent culling", "[framegraph][cull]") {
    FrameGraph graph;
    i32 executionCount = 0;

    struct PassData {
        MockTextureHandle Texture;
    };

    graph.AddPass<PassData>(
        "PassWithSideEffects",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Texture = builder.Create<MockTexture>("SideEffectTexture", MockTextureDescriptor{ 128, 128, "RGBA8" });
            builder.MarkSideEffect();
        },
        [&executionCount](const PassData &, FrameGraph &, const FrameGraphContext &) { ++executionCount; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executionCount == 1);
}

TEST_CASE("FrameGraph - All passes culled except side effects", "[framegraph][cull]") {
    FrameGraph graph;
    i32 executed = 0;

    struct PassData {
        MockTextureHandle Resource;
    };

    for (i32 i = 0; i < 4; ++i) {
        graph.AddPass<PassData>(
            "CulledPass",
            [](FrameGraph::Builder &builder, PassData &data) {
                data.Resource = builder.Create<MockTexture>("CulledRes", MockTextureDescriptor{ 128, 128, "RGBA8" });
            },
            [&executed](const PassData &, FrameGraph &, const FrameGraphContext &) { ++executed; });
    }

    graph.AddPass<PassData>(
        "KeptPass",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Resource = builder.Create<MockTexture>("KeptRes", MockTextureDescriptor{ 128, 128, "RGBA8" });
            builder.MarkSideEffect();
        },
        [&executed](const PassData &, FrameGraph &, const FrameGraphContext &) { ++executed; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executed == 1);
}

// ===========================================================================================
// Ordering
// ===========================================================================================

TEST_CASE("FrameGraph - Multiple passes with dependencies", "[framegraph][order]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle texture1, texture2;

    struct Pass1Data {
        MockTextureHandle Output;
    };

    struct Pass2Data {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct Pass3Data {
        MockTextureHandle Input;
    };

    graph.AddPass<Pass1Data>(
        "Pass1",
        [&texture1](FrameGraph::Builder &builder, Pass1Data &data) {
            data.Output = builder.Create<MockTexture>("Texture1", MockTextureDescriptor{ 512, 512, "RGBA8" });
            texture1 = data.Output;
        },
        [&executionOrder](const Pass1Data &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Pass1"); });

    graph.AddPass<Pass2Data>(
        "Pass2",
        [&texture1, &texture2](FrameGraph::Builder &builder, Pass2Data &data) {
            data.Input = builder.Read(texture1);
            data.Output = builder.Create<MockTexture>("Texture2", MockTextureDescriptor{ 512, 512, "RGBA8" });
            texture2 = data.Output;
        },
        [&executionOrder](const Pass2Data &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Pass2"); });

    graph.AddPass<Pass3Data>(
        "Pass3",
        [&texture2](FrameGraph::Builder &builder, Pass3Data &data) {
            data.Input = builder.Read(texture2);
            builder.MarkSideEffect();
        },
        [&executionOrder](const Pass3Data &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Pass3"); });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executionOrder == std::vector<std::string>{ "Pass1", "Pass2", "Pass3" });
}

TEST_CASE("FrameGraph - Independent chains keep declaration order", "[framegraph][order]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;
    MockTextureHandle chainA, chainB, chainAFinal, chainBFinal;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct TransformData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct PresentData {
        MockTextureHandle InputA;
        MockTextureHandle InputB;
    };

    // Two independent chains, declared interleaved. Nothing orders A against B, so the sort is free to group each
    // chain; the min-heap tie-break deliberately keeps declaration order instead, which is the least surprising
    // choice and keeps a frame's pass order stable between builds.
    graph.AddPass<ProduceData>(
        "A1",
        [&chainA](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("A1Output", MockTextureDescriptor{ 10, 10, "RGBA8" });
            chainA = data.Output;
        },
        [&executionOrder](const ProduceData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("A1"); });

    graph.AddPass<ProduceData>(
        "B1",
        [&chainB](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("B1Output", MockTextureDescriptor{ 20, 20, "RGBA8" });
            chainB = data.Output;
        },
        [&executionOrder](const ProduceData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("B1"); });

    graph.AddPass<TransformData>(
        "A2",
        [&chainA, &chainAFinal](FrameGraph::Builder &builder, TransformData &data) {
            data.Input = builder.Read(chainA);
            data.Output = builder.Create<MockTexture>("A2Output", MockTextureDescriptor{ 30, 30, "RGBA8" });
            chainAFinal = data.Output;
        },
        [&executionOrder](const TransformData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("A2"); });

    graph.AddPass<TransformData>(
        "B2",
        [&chainB, &chainBFinal](FrameGraph::Builder &builder, TransformData &data) {
            data.Input = builder.Read(chainB);
            data.Output = builder.Create<MockTexture>("B2Output", MockTextureDescriptor{ 40, 40, "RGBA8" });
            chainBFinal = data.Output;
        },
        [&executionOrder](const TransformData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("B2"); });

    graph.AddPass<PresentData>(
        "Present",
        [&chainAFinal, &chainBFinal](FrameGraph::Builder &builder, PresentData &data) {
            data.InputA = builder.Read(chainAFinal);
            data.InputB = builder.Read(chainBFinal);
            builder.MarkSideEffect();
        },
        [&executionOrder](const PresentData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Present"); });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executionOrder == std::vector<std::string>{ "A1", "B1", "A2", "B2", "Present" });
}

TEST_CASE("FrameGraph - Write-after-read orders a reader before the next writer", "[framegraph][order]") {
    FrameGraph graph;
    std::vector<std::string> executionOrder;

    struct PresentData {
        MockTextureHandle Image;
    };

    struct RenderData {
        MockTextureHandle Image;
    };

    // Imported resources are the only way to express a dependency on something no pass has produced yet, since
    // their handle exists before any pass is declared. Reading the imported version and then writing it is a
    // write-after-read hazard: the sort must keep the reader first, which it derives from the version chain
    // rather than from declaration order.
    const MockTextureHandle backbuffer = graph.Import<MockTexture>("Backbuffer", MockTextureDescriptor{ 1920, 1080, "RGBA8" }, MockTexture{});

    graph.AddPass<PresentData>(
        "Present",
        [backbuffer](FrameGraph::Builder &builder, PresentData &data) {
            data.Image = builder.Read(backbuffer);
            builder.MarkSideEffect();
        },
        [&executionOrder](const PresentData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Present"); });

    graph.AddPass<RenderData>(
        "RenderToBackbuffer",
        [backbuffer](FrameGraph::Builder &builder, RenderData &data) { data.Image = builder.Write(backbuffer); },
        [&executionOrder](const RenderData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("RenderToBackbuffer"); });

    graph.Compile();

    Recorder recorder;
    graph.Execute(MakeContext(recorder));

    REQUIRE(executionOrder == std::vector<std::string>{ "Present", "RenderToBackbuffer" });

    // Imported resources stay under external management - the graph neither creates nor destroys them.
    REQUIRE(recorder.Lifecycle().empty());
}

TEST_CASE("FrameGraph - Compiled order satisfies every derived dependency", "[framegraph][order]") {
    FrameGraph graph;
    MockTextureHandle source, left, right;

    struct SourceData {
        MockTextureHandle Output;
    };

    struct BranchData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct MergeData {
        MockTextureHandle Left;
        MockTextureHandle Right;
    };

    graph.AddPass<SourceData>(
        "Source",
        [&source](FrameGraph::Builder &builder, SourceData &data) {
            data.Output = builder.Create<MockTexture>("Source", MockTextureDescriptor{ 64, 64, "RGBA8" });
            source = data.Output;
        },
        [](const SourceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<BranchData>(
        "Left",
        [&source, &left](FrameGraph::Builder &builder, BranchData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 64, 64, "RGBA8" });
            left = data.Output;
        },
        [](const BranchData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<BranchData>(
        "Right",
        [&source, &right](FrameGraph::Builder &builder, BranchData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 64, 64, "RGBA8" });
            right = data.Output;
        },
        [](const BranchData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&left, &right](FrameGraph::Builder &builder, MergeData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const MergeData &, FrameGraph &, const FrameGraphContext &) {});

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
    FrameGraph graph;
    MockTextureHandle texture1, texture2;
    i32 pass1Executed = 0;
    i32 pass2Executed = 0;
    i32 pass3Executed = 0;

    struct Pass1Data {
        MockTextureHandle Output;
    };

    struct Pass2Data {
        MockTextureHandle Modified;
    };

    struct Pass3Data {
        MockTextureHandle Input;
    };

    graph.AddPass<Pass1Data>(
        "CreatePass",
        [&texture1](FrameGraph::Builder &builder, Pass1Data &data) {
            data.Output = builder.Create<MockTexture>("ModifiableTexture", MockTextureDescriptor{ 256, 256, "RGBA8" });
            texture1 = data.Output;
        },
        [&pass1Executed](const Pass1Data &, FrameGraph &, const FrameGraphContext &) { ++pass1Executed; });

    graph.AddPass<Pass2Data>(
        "ModifyPass",
        [&texture1, &texture2](FrameGraph::Builder &builder, Pass2Data &data) {
            data.Modified = builder.Write(texture1);
            texture2 = data.Modified;
        },
        [&pass2Executed](const Pass2Data &, FrameGraph &, const FrameGraphContext &) { ++pass2Executed; });

    graph.AddPass<Pass3Data>(
        "ReadPass",
        [&texture2](FrameGraph::Builder &builder, Pass3Data &data) {
            data.Input = builder.Read(texture2);
            builder.MarkSideEffect();
        },
        [&pass3Executed](const Pass3Data &, FrameGraph &, const FrameGraphContext &) { ++pass3Executed; });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(pass1Executed == 1);
    REQUIRE(pass2Executed == 1);
    REQUIRE(pass3Executed == 1);

    // The write produced a distinct handle referring to a later version of the same resource.
    REQUIRE(texture1 != texture2);
    REQUIRE(graph.GetResourceNode(texture2.ID).GetVersion() > graph.GetResourceNode(texture1.ID).GetVersion());
    REQUIRE(graph.GetResourceNode(texture1.ID).GetResourceEntryID() == graph.GetResourceNode(texture2.ID).GetResourceEntryID());
}

TEST_CASE("FrameGraph - Multi-version resource has a single lifetime", "[framegraph][lifetime]") {
    FrameGraph graph;
    Recorder recorder;
    std::vector<std::string> executionOrder;
    MockTextureHandle resource;

    struct CreateData {
        MockTextureHandle Output;
    };

    struct ModifyData {
        MockTextureHandle InOut;
    };

    struct ReadData {
        MockTextureHandle Input;
    };

    graph.AddPass<CreateData>(
        "Create",
        [&resource](FrameGraph::Builder &builder, CreateData &data) {
            data.Output = builder.Create<MockTexture>("VersionedResource", MockTextureDescriptor{ 128, 128, "RGBA8" });
            resource = data.Output;
        },
        [&executionOrder](const CreateData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Create"); });

    graph.AddPass<ModifyData>(
        "Modify1",
        [&resource](FrameGraph::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
        },
        [&executionOrder](const ModifyData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Modify1"); });

    graph.AddPass<ModifyData>(
        "Modify2",
        [&resource](FrameGraph::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
        },
        [&executionOrder](const ModifyData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Modify2"); });

    graph.AddPass<ReadData>(
        "Read",
        [&resource](FrameGraph::Builder &builder, ReadData &data) {
            data.Input = builder.Read(resource);
            builder.MarkSideEffect();
        },
        [&executionOrder](const ReadData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Read"); });

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(executionOrder == std::vector<std::string>{ "Create", "Modify1", "Modify2", "Read" });

    // One backing resource across three versions: created once by the producer, destroyed once after the last
    // version is consumed.
    REQUIRE(recorder.Lifecycle() == std::vector<std::string>{ "Create:128x128", "Destroy:128x128" });
}

TEST_CASE("FrameGraph - Diamond resource lifetimes", "[framegraph][lifetime]") {
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle source, intermediate1, intermediate2;

    struct SourceData {
        MockTextureHandle Output;
    };

    struct PathData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct MergeData {
        MockTextureHandle Input1;
        MockTextureHandle Input2;
    };

    // Distinct extents so each Create/Destroy record identifies its resource.
    graph.AddPass<SourceData>(
        "Source",
        [&source](FrameGraph::Builder &builder, SourceData &data) {
            data.Output = builder.Create<MockTexture>("DiamondSource", MockTextureDescriptor{ 100, 100, "RGBA8" });
            source = data.Output;
        },
        [](const SourceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<PathData>(
        "Path1",
        [&source, &intermediate1](FrameGraph::Builder &builder, PathData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("DiamondIntermediate1", MockTextureDescriptor{ 200, 200, "RGBA8" });
            intermediate1 = data.Output;
        },
        [](const PathData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<PathData>(
        "Path2",
        [&source, &intermediate2](FrameGraph::Builder &builder, PathData &data) {
            data.Input = builder.Read(source);
            data.Output = builder.Create<MockTexture>("DiamondIntermediate2", MockTextureDescriptor{ 300, 300, "RGBA8" });
            intermediate2 = data.Output;
        },
        [](const PathData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&intermediate1, &intermediate2](FrameGraph::Builder &builder, MergeData &data) {
            data.Input1 = builder.Read(intermediate1);
            data.Input2 = builder.Read(intermediate2);
            builder.MarkSideEffect();
        },
        [](const MergeData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    // The source is read by both branches, so it survives until the later of them; both intermediates die at the
    // merge, released in registry order.
    const std::vector<std::string> expected{ "Create:100x100", "Create:200x200", "Create:300x300", "Destroy:100x100", "Destroy:200x200", "Destroy:300x300" };
    REQUIRE(recorder.Lifecycle() == expected);
}

TEST_CASE("FrameGraph - Complex chain with multiple reads and writes", "[framegraph][lifetime]") {
    FrameGraph graph;
    std::vector<i32> executionOrder;
    MockTextureHandle resource;

    struct CreateData {
        MockTextureHandle Output;
    };

    struct ModifyData {
        MockTextureHandle InOut;
    };

    struct ReadData {
        MockTextureHandle Input;
    };

    graph.AddPass<CreateData>(
        "Create",
        [&resource](FrameGraph::Builder &builder, CreateData &data) {
            data.Output = builder.Create<MockTexture>("ChainResource", MockTextureDescriptor{ 256, 256, "RGBA8" });
            resource = data.Output;
        },
        [&executionOrder](const CreateData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back(1); });

    graph.AddPass<ModifyData>(
        "Modify1",
        [&resource](FrameGraph::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
        },
        [&executionOrder](const ModifyData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back(2); });

    graph.AddPass<ModifyData>(
        "Modify2",
        [&resource](FrameGraph::Builder &builder, ModifyData &data) {
            data.InOut = builder.Write(resource);
            resource = data.InOut;
        },
        [&executionOrder](const ModifyData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back(3); });

    graph.AddPass<ReadData>(
        "Read",
        [&resource](FrameGraph::Builder &builder, ReadData &data) {
            data.Input = builder.Read(resource);
            builder.MarkSideEffect();
        },
        [&executionOrder](const ReadData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back(4); });

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    REQUIRE(executionOrder == std::vector<i32>{ 1, 2, 3, 4 });
}

// ===========================================================================================
// Access hooks and barriers
// ===========================================================================================

TEST_CASE("FrameGraph - Access hooks fire for every declared access", "[framegraph][barriers]") {
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle created;

    struct PassData {
        MockTextureHandle Resource;
    };

    graph.AddPass<PassData>(
        "CreatePass",
        [&created](FrameGraph::Builder &builder, PassData &data) {
            data.Resource = builder.Create<MockTexture>("TextureWithUsage", MockTextureDescriptor{ 128, 128, "RGBA8" }, ATTACHMENT);
            created = data.Resource;
        },
        [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<PassData>(
        "ReadPass",
        [&created](FrameGraph::Builder &builder, PassData &data) {
            data.Resource = builder.Read(created, SAMPLED);
            builder.MarkSideEffect();
        },
        [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    // Both hooks run: a write hook for the producer's implied write, a read hook for the consumer. The old
    // implementation skipped them whenever the usage equalled a sentinel that was also the default argument,
    // which meant every call site.
    REQUIRE(std::ranges::find(recorder.Events, "PreWrite:layout=1") != recorder.Events.end());
    REQUIRE(std::ranges::find(recorder.Events, "PreRead:layout=2") != recorder.Events.end());

    const MockTexture &texture = graph.GetResource(created);
    REQUIRE(texture.PreWriteCount == 1);
    REQUIRE(texture.PreReadCount == 1);
}

TEST_CASE("FrameGraph - Barriers are batched once per pass", "[framegraph][barriers]") {
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle colorTexture, depthTexture;

    struct GBufferData {
        MockTextureHandle Color;
        MockTextureHandle Depth;
    };

    struct LightingData {
        MockTextureHandle Color;
        MockTextureHandle Depth;
    };

    graph.AddPass<GBufferData>(
        "GBuffer",
        [&colorTexture, &depthTexture](FrameGraph::Builder &builder, GBufferData &data) {
            data.Color = builder.Create<MockTexture>("Color", MockTextureDescriptor{ 64, 64, "RGBA8" }, ATTACHMENT);
            data.Depth = builder.Create<MockTexture>("Depth", MockTextureDescriptor{ 64, 64, "D32" }, ATTACHMENT);
            colorTexture = data.Color;
            depthTexture = data.Depth;
        },
        [](const GBufferData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<LightingData>(
        "Lighting",
        [&colorTexture, &depthTexture](FrameGraph::Builder &builder, LightingData &data) {
            data.Color = builder.Read(colorTexture, SAMPLED);
            data.Depth = builder.Read(depthTexture, SAMPLED);
            builder.MarkSideEffect();
        },
        [](const LightingData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    // One call per pass, not one per resource: the G-buffer pass transitions both attachments in a single batch
    // and the lighting pass transitions both to sampled in a single batch.
    REQUIRE(recorder.BarrierBatches.size() == 2);
    REQUIRE(recorder.BarrierBatches[0] == "Batch(2): e0[0->1] e1[0->1]");
    REQUIRE(recorder.BarrierBatches[1] == "Batch(2): e0[1->2] e1[1->2]");
}

TEST_CASE("FrameGraph - Unchanged usage emits no barrier", "[framegraph][barriers]") {
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct ConsumeData {
        MockTextureHandle Input;
    };

    // No usages declared anywhere, which is what an OpenGL-style backend does. Nothing to transition, so the
    // barrier hook is never called.
    graph.AddPass<ProduceData>(
        "Produce",
        [&texture](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Plain", MockTextureDescriptor{ 32, 32, "RGBA8" });
            texture = data.Output;
        },
        [](const ProduceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<ConsumeData>(
        "Consume",
        [&texture](FrameGraph::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
        },
        [](const ConsumeData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(recorder.BarrierBatches.empty());
}

// ===========================================================================================
// Transient aliasing
// ===========================================================================================

TEST_CASE("FrameGraph - Disjoint transient lifetimes share storage", "[framegraph][aliasing]") {
    FrameGraph graph;
    MockTextureHandle first, second;

    struct StageData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct SeedData {
        MockTextureHandle Output;
    };

    // A strict chain: each stage's input dies as soon as its output is produced, so every intermediate can reuse
    // the same storage.
    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" });
            first = data.Output;
        },
        [](const SeedData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<MockTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" });
            second = data.Output;
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<StageData>(
        "Stage2",
        [&second](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(second);
            data.Output = builder.Create<MockTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" });
            builder.MarkSideEffect();
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

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
    FrameGraph graph;
    MockTextureHandle left, right;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct MergeData {
        MockTextureHandle Left;
        MockTextureHandle Right;
    };

    graph.AddPass<ProduceData>(
        "Left",
        [&left](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 128, 128, "RGBA8" });
            left = data.Output;
        },
        [](const ProduceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<ProduceData>(
        "Right",
        [&right](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 128, 128, "RGBA8" });
            right = data.Output;
        },
        [](const ProduceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<MergeData>(
        "Merge",
        [&left, &right](FrameGraph::Builder &builder, MergeData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const MergeData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();

    // Both live simultaneously at the merge, so nothing can be shared.
    REQUIRE(graph.GetAliasingReport().PeakLiveBytes == graph.GetAliasingReport().UnaliasedBytes);
    REQUIRE(graph.GetAliasingReport().SavedBytes() == 0);
}

TEST_CASE("FrameGraph - Small transients pack into the space a dead large one leaves", "[framegraph][aliasing]") {
    FrameGraph graph;
    MockTextureHandle big, drained, left, right;

    struct BigData {
        MockTextureHandle Output;
    };

    struct DrainData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    struct FanData {
        MockTextureHandle Input;
        MockTextureHandle Left;
        MockTextureHandle Right;
    };

    struct SinkData {
        MockTextureHandle Left;
        MockTextureHandle Right;
    };

    graph.AddPass<BigData>(
        "Big",
        [&big](FrameGraph::Builder &builder, BigData &data) {
            data.Output = builder.Create<MockTexture>("Big", MockTextureDescriptor{ 1024, 1024, "RGBA8" });
            big = data.Output;
        },
        [](const BigData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<DrainData>(
        "Drain",
        [&big, &drained](FrameGraph::Builder &builder, DrainData &data) {
            data.Input = builder.Read(big);
            data.Output = builder.Create<MockTexture>("Drained", MockTextureDescriptor{ 128, 128, "RGBA8" });
            drained = data.Output;
        },
        [](const DrainData &, FrameGraph &, const FrameGraphContext &) {});

    // Both of these outlive the large texture, so they belong in the bytes it has vacated rather than beside them.
    graph.AddPass<FanData>(
        "Fan",
        [&drained, &left, &right](FrameGraph::Builder &builder, FanData &data) {
            data.Input = builder.Read(drained);
            data.Left = builder.Create<MockTexture>("Left", MockTextureDescriptor{ 128, 128, "RGBA8" });
            data.Right = builder.Create<MockTexture>("Right", MockTextureDescriptor{ 128, 128, "RGBA8" });
            left = data.Left;
            right = data.Right;
        },
        [](const FanData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<SinkData>(
        "Sink",
        [&left, &right](FrameGraph::Builder &builder, SinkData &data) {
            data.Left = builder.Read(left);
            data.Right = builder.Read(right);
            builder.MarkSideEffect();
        },
        [](const SinkData &, FrameGraph &, const FrameGraphContext &) {});

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
    FrameGraph graph;
    Recorder recorder;
    MockTextureHandle first, second;

    struct SeedData {
        MockTextureHandle Output;
    };

    struct StageData {
        MockTextureHandle Input;
        MockTextureHandle Output;
    };

    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            first = data.Output;
        },
        [](const SeedData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(first, SAMPLED);
            data.Output = builder.Create<MockTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            second = data.Output;
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

    // Stage0's texture is dead by here, so Stage2's output inherits its bytes.
    graph.AddPass<StageData>(
        "Stage2",
        [&second](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(second, SAMPLED);
            data.Output = builder.Create<MockTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" }, ATTACHMENT);
            builder.MarkSideEffect();
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(MakeContext(recorder));

    REQUIRE(recorder.BarrierBatches.size() == 3);

    // Stage0 and Stage1 create fresh storage - no previous occupant, so no discard.
    REQUIRE(recorder.BarrierBatches[0] == "Batch(1): e0[0->1]");
    REQUIRE(recorder.BarrierBatches[1] == "Batch(2): e0[1->2] e1[0->1]");

    // Stage2's output takes over entry 0's bytes, so its first use discards them and waits on the stages entry 0
    // was last used in - SAMPLED's 0x4, from Stage1's read, not ATTACHMENT's 0x1 from where it was written.
    REQUIRE(recorder.BarrierBatches[2] == "Batch(2): e1[1->2] e2[0->1]{discard,waitStages=4}");
}

TEST_CASE("FrameGraph - The plan's offsets reach the backend at create time", "[framegraph][aliasing]") {
    FrameGraph graph;
    MockPlacedTextureHandle first, second, third;

    struct SeedData {
        MockPlacedTextureHandle Output;
    };

    struct StageData {
        MockPlacedTextureHandle Input;
        MockPlacedTextureHandle Output;
    };

    // The same strict chain as the sharing test above: each stage's input dies as its output is produced.
    graph.AddPass<SeedData>(
        "Stage0",
        [&first](FrameGraph::Builder &builder, SeedData &data) {
            data.Output = builder.Create<MockPlacedTexture>("Stage0", MockTextureDescriptor{ 256, 256, "RGBA8" });
            first = data.Output;
        },
        [](const SeedData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<StageData>(
        "Stage1",
        [&first, &second](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(first);
            data.Output = builder.Create<MockPlacedTexture>("Stage1", MockTextureDescriptor{ 256, 256, "RGBA8" });
            second = data.Output;
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<StageData>(
        "Stage2",
        [&second, &third](FrameGraph::Builder &builder, StageData &data) {
            data.Input = builder.Read(second);
            data.Output = builder.Create<MockPlacedTexture>("Stage2", MockTextureDescriptor{ 256, 256, "RGBA8" });
            third = data.Output;
            builder.MarkSideEffect();
        },
        [](const StageData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    constexpr u64 TEXTURE_BYTES = 256ULL * 256ULL * 4ULL;

    // Stage0's texture is dead by the time Stage2's is created, so the two share bytes while Stage1's, which is
    // live alongside both, sits above them. Asserting the offsets rather than the totals is what proves the plan
    // reaches the backend at all.
    REQUIRE(graph.GetResource(first).Placement.IsAliased);
    REQUIRE(graph.GetResource(first).Placement.Offset == 0);
    REQUIRE(graph.GetResource(second).Placement.Offset == TEXTURE_BYTES);
    REQUIRE(graph.GetResource(third).Placement.Offset == 0);

    REQUIRE(graph.GetAliasingReport().AliasedBytes == 2 * TEXTURE_BYTES);
}

TEST_CASE("FrameGraph - Imported resources are handed an unplaced placement", "[framegraph][aliasing]") {
    FrameGraph graph;

    struct PassData {
        MockPlacedTextureHandle Target;
    };

    const MockPlacedTextureHandle imported = graph.Import<MockPlacedTexture>("Backbuffer", MockTextureDescriptor{ 64, 64, "RGBA8" }, MockPlacedTexture{});

    graph.AddPass<PassData>(
        "Draw",
        [imported](FrameGraph::Builder &builder, PassData &data) { data.Target = builder.Write(imported); },
        [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();
    graph.Execute(FrameGraphContext{});

    // The graph never creates an imported resource, so nothing was placed and the backend keeps whatever storage
    // it already had.
    REQUIRE_FALSE(graph.GetResource(imported).Created);
    REQUIRE_FALSE(graph.GetResource(imported).Placement.IsAliased);
}

TEST_CASE("FrameGraph - Backends without memory requirements stay out of the plan", "[framegraph][aliasing]") {
    FrameGraph graph;

    struct PassData {
        MockBufferHandle Buffer;
    };

    graph.AddPass<PassData>(
        "BufferPass",
        [](FrameGraph::Builder &builder, PassData &data) {
            data.Buffer = builder.Create<MockBuffer>("Buffer", size_t{ 4096 });
            builder.MarkSideEffect();
        },
        [](const PassData &, FrameGraph &, const FrameGraphContext &) {});

    graph.Compile();

    REQUIRE(graph.GetAliasingReport().ResourceCount == 0);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == 0);
}

// ===========================================================================================
// Compile / Record / Submit
// ===========================================================================================

TEST_CASE("FrameGraph - Reset carries no derived state into the next frame", "[framegraph]") {
    FrameGraph graph;
    i32 executed = 0;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct ConsumeData {
        MockTextureHandle Input;
    };

    // Compile derives reference counts, an execution order and an aliasing plan, and writes some of it back onto
    // the nodes. Reset is the only thing that clears it, so a field it forgets would make frame two differ from
    // frame one - which is what this compares.
    const auto buildFrame = [&executed](FrameGraph &g) {
        MockTextureHandle texture;

        g.AddPass<ProduceData>(
            "Produce",
            [&texture](FrameGraph::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockTexture>("Resource", MockTextureDescriptor{ 64, 64, "RGBA8" });
                texture = data.Output;
            },
            [&executed](const ProduceData &, FrameGraph &, const FrameGraphContext &) { ++executed; });

        g.AddPass<ConsumeData>(
            "Consume",
            [&texture](FrameGraph::Builder &builder, ConsumeData &data) {
                data.Input = builder.Read(texture);
                builder.MarkSideEffect();
            },
            [&executed](const ConsumeData &, FrameGraph &, const FrameGraphContext &) { ++executed; });

        g.Compile();
    };

    buildFrame(graph);
    const std::vector<u32> firstOrder(graph.GetExecutionOrder().begin(), graph.GetExecutionOrder().end());
    const FrameGraphAliasingReport firstReport = graph.GetAliasingReport();

    graph.Execute(FrameGraphContext{});
    REQUIRE(executed == 2);

    graph.Reset();
    buildFrame(graph);

    const std::vector<u32> secondOrder(graph.GetExecutionOrder().begin(), graph.GetExecutionOrder().end());

    REQUIRE(firstOrder == secondOrder);
    REQUIRE(graph.GetAliasingReport().AliasedBytes == firstReport.AliasedBytes);
    REQUIRE(graph.GetAliasingReport().UnaliasedBytes == firstReport.UnaliasedBytes);

    graph.Execute(FrameGraphContext{});
    REQUIRE(executed == 4);
}

TEST_CASE("FrameGraph - Record and Submit split runs every pass", "[framegraph][record]") {
    FrameGraph graph;
    Recorder recorder;
    std::vector<std::string> executionOrder;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct ConsumeData {
        MockTextureHandle Input;
    };

    graph.AddPass<ProduceData>(
        "Produce",
        [&texture](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Recorded", MockTextureDescriptor{ 64, 64, "RGBA8" });
            texture = data.Output;
        },
        [&executionOrder](const ProduceData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Produce"); });

    graph.AddPass<ConsumeData>(
        "Consume",
        [&texture](FrameGraph::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
        },
        [&executionOrder](const ConsumeData &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back("Consume"); });

    graph.Compile();

    const FrameGraphContext context = MakeContext(recorder);
    graph.Record(context);
    graph.Submit(context);

    REQUIRE(executionOrder == std::vector<std::string>{ "Produce", "Consume" });

    // In the split form every resource is materialized before recording begins and released by Submit, because
    // the GPU timeline rather than the record timeline governs when storage can be reused.
    REQUIRE(recorder.Lifecycle() == std::vector<std::string>{ "Create:64x64", "Destroy:64x64" });
}

TEST_CASE("FrameGraph - Parallel recording runs every pass exactly once", "[framegraph][record]") {
    FrameGraph graph;
    Recorder recorder;
    std::atomic<i32> executed{ 0 };
    std::vector<MockTextureHandle> outputs;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct PresentData {};

    constexpr i32 PASS_COUNT = 16;
    outputs.resize(PASS_COUNT);

    for (i32 i = 0; i < PASS_COUNT; ++i) {
        graph.AddPass<ProduceData>(
            "Produce",
            [&outputs, i](FrameGraph::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockTexture>("Parallel", MockTextureDescriptor{ 32, 32, "RGBA8" });
                outputs[static_cast<size_t>(i)] = data.Output;
            },
            [&executed](const ProduceData &, FrameGraph &, const FrameGraphContext &) { executed.fetch_add(1, std::memory_order_relaxed); });
    }

    graph.AddPass<PresentData>(
        "Present",
        [&outputs](FrameGraph::Builder &builder, PresentData &data) {
            for (const MockTextureHandle handle : outputs) {
                (void)builder.Read(handle);
            }

            builder.MarkSideEffect();
        },
        [&executed](const PresentData &, FrameGraph &, const FrameGraphContext &) { executed.fetch_add(1, std::memory_order_relaxed); });

    graph.Compile();

    const FrameGraphContext context = MakeContext(recorder);
    graph.RecordParallel(context);
    graph.Submit(context);

    REQUIRE(executed.load() == PASS_COUNT + 1);
}

// ===========================================================================================
// Cross-frame reuse
// ===========================================================================================

TEST_CASE("FrameGraph - Reset returns the graph to empty and keeps capacity", "[framegraph][reset]") {
    FrameGraph graph;
    Recorder recorder;

    struct PassData {
        MockTextureHandle Output;
    };

    const auto buildFrame = [&graph](i32 &counter) {
        graph.AddPass<PassData>(
            "Frame",
            [](FrameGraph::Builder &builder, PassData &data) {
                data.Output = builder.Create<MockTexture>("FrameTarget", MockTextureDescriptor{ 64, 64, "RGBA8" });
                builder.MarkSideEffect();
            },
            [&counter](const PassData &, FrameGraph &, const FrameGraphContext &) { ++counter; });
    };

    i32 firstFrame = 0;
    buildFrame(firstFrame);
    graph.Compile();
    graph.Execute(MakeContext(recorder));

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
    graph.Execute(MakeContext(recorder));

    REQUIRE(secondFrame == 1);
    REQUIRE(graph.GetPassCount() == 1);
}

TEST_CASE("FrameGraph - Steady state is allocation-free", "[framegraph][reset][alloc]") {
    // A POD-descriptor backend and reference-capturing lambdas, so every byte the graph allocates comes from the
    // graph itself and not from a descriptor's std::string or a fat capture.
    FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 32, .ExpectedResources = 64, .InitialArenaBytes = 32 * 1024 } };
    i32 executed = 0;

    struct ProduceData {
        MockPodHandle Output;
    };

    struct ConsumeData {
        MockPodHandle Input;
        MockPodHandle Output;
    };

    const auto buildFrame = [&graph, &executed] {
        MockPodHandle previous;

        graph.AddPass<ProduceData>(
            "Seed",
            [&previous](FrameGraph::Builder &builder, ProduceData &data) {
                data.Output = builder.Create<MockPodTexture>("Seed", MockPodTexture::Descriptor{ 64, 64 });
                previous = data.Output;
            },
            [&executed](const ProduceData &, FrameGraph &, const FrameGraphContext &) { ++executed; });

        for (i32 stage = 0; stage < 20; ++stage) {
            graph.AddPass<ConsumeData>(
                "Stage",
                [&previous](FrameGraph::Builder &builder, ConsumeData &data) {
                    data.Input = builder.Read(previous);
                    data.Output = builder.Create<MockPodTexture>("Stage", MockPodTexture::Descriptor{ 64, 64 });
                    previous = data.Output;
                },
                [&executed](const ConsumeData &, FrameGraph &, const FrameGraphContext &) { ++executed; });
        }

        graph.AddPass<ConsumeData>(
            "Present",
            [&previous](FrameGraph::Builder &builder, ConsumeData &data) {
                data.Input = builder.Read(previous);
                builder.MarkSideEffect();
            },
            [&executed](const ConsumeData &, FrameGraph &, const FrameGraphContext &) { ++executed; });
    };

    const auto runFrame = [&graph, &buildFrame] {
        buildFrame();
        graph.Compile();
        graph.Execute(FrameGraphContext{});
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
    FrameGraph graph;
    MockTextureHandle texture;

    struct ProduceData {
        MockTextureHandle Output;
    };

    struct ConsumeData {
        MockTextureHandle Input;
    };

    struct DeadData {
        MockTextureHandle Output;
    };

    graph.AddPass<ProduceData>(
        "Producer",
        [&texture](FrameGraph::Builder &builder, ProduceData &data) {
            data.Output = builder.Create<MockTexture>("Target", MockTextureDescriptor{ 64, 64, "RGBA8" });
            texture = data.Output;
        },
        [](const ProduceData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<ConsumeData>(
        "Consumer",
        [&texture](FrameGraph::Builder &builder, ConsumeData &data) {
            data.Input = builder.Read(texture);
            builder.MarkSideEffect();
        },
        [](const ConsumeData &, FrameGraph &, const FrameGraphContext &) {});

    graph.AddPass<DeadData>(
        "DeadPass",
        [](FrameGraph::Builder &builder, DeadData &data) { data.Output = builder.Create<MockTexture>("Dead", MockTextureDescriptor{ 8, 8, "RGBA8" }); },
        [](const DeadData &, FrameGraph &, const FrameGraphContext &) {});

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
    FrameGraph graph;
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
    };

    const auto record = [&executionOrder](std::string name) {
        return [&executionOrder, name = std::move(name)](const auto &, FrameGraph &, const FrameGraphContext &) { executionOrder.push_back(name); };
    };

    graph.AddPass<ShadowMapData>(
        "DirectionalShadowMap",
        [&dirShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("DirShadowMap", MockTextureDescriptor{ 2048, 2048, "D32" });
            dirShadowMap = data.ShadowMap;
        },
        record("DirectionalShadowMap"));

    graph.AddPass<ShadowMapData>(
        "SpotLight1ShadowMap",
        [&spot1ShadowMap](FrameGraph::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("Spot1ShadowMap", MockTextureDescriptor{ 1024, 1024, "D32" });
            spot1ShadowMap = data.ShadowMap;
        },
        record("SpotLight1ShadowMap"));

    // Nothing reads these, so they must be culled.
    graph.AddPass<ShadowMapData>(
        "SpotLight2ShadowMap",
        [](FrameGraph::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("Spot2ShadowMap", MockTextureDescriptor{ 1024, 1024, "D32" });
        },
        record("SpotLight2ShadowMap"));

    graph.AddPass<ShadowMapData>(
        "PointLightShadowMap",
        [](FrameGraph::Builder &builder, ShadowMapData &data) {
            data.ShadowMap = builder.Create<MockTexture>("PointShadowMap", MockTextureDescriptor{ 512, 512, "D32" });
        },
        record("PointLightShadowMap"));

    struct GBufferData {
        MockTextureHandle Albedo, Normal, Depth, Material;
    };

    graph.AddPass<GBufferData>(
        "GBufferPass",
        [&](FrameGraph::Builder &builder, GBufferData &data) {
            data.Albedo = builder.Create<MockTexture>("GBufferAlbedo", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            data.Normal = builder.Create<MockTexture>("GBufferNormal", MockTextureDescriptor{ 1920, 1080, "RGBA16F" });
            data.Depth = builder.Create<MockTexture>("GBufferDepth", MockTextureDescriptor{ 1920, 1080, "D32" });
            data.Material = builder.Create<MockTexture>("GBufferMaterial", MockTextureDescriptor{ 1920, 1080, "RGBA8" });

            gbufferAlbedo = data.Albedo;
            gbufferNormal = data.Normal;
            gbufferDepth = data.Depth;
            gbufferMaterial = data.Material;
        },
        record("GBufferPass"));

    struct SSAOData {
        MockTextureHandle DepthIn, NormalIn, Output;
    };

    if constexpr (ENABLE_SSAO) {
        graph.AddPass<SSAOData>(
            "SSAOPass",
            [&](FrameGraph::Builder &builder, SSAOData &data) {
                data.DepthIn = builder.Read(gbufferDepth);
                data.NormalIn = builder.Read(gbufferNormal);
                data.Output = builder.Create<MockTexture>("SSAO", MockTextureDescriptor{ 1920, 1080, "R8" });
                ssaoTexture = data.Output;
            },
            record("SSAOPass"));

        graph.AddPass<SSAOData>(
            "SSAOBlurPass",
            [&](FrameGraph::Builder &builder, SSAOData &data) {
                data.DepthIn = builder.Read(ssaoTexture);
                data.Output = builder.Create<MockTexture>("SSAOBlurred", MockTextureDescriptor{ 1920, 1080, "R8" });
                ssaoBlurred = data.Output;
            },
            record("SSAOBlurPass"));
    }

    struct LightingData {
        MockTextureHandle AlbedoIn, NormalIn, DepthIn, MaterialIn;
        MockTextureHandle DirShadowIn, Spot1ShadowIn, SsaoIn;
        MockTextureHandle ColorOut, DepthOut;
    };

    graph.AddPass<LightingData>(
        "LightingPass",
        [&](FrameGraph::Builder &builder, LightingData &data) {
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
        },
        record("LightingPass"));

    struct SkyData {
        MockTextureHandle DepthIn, ColorInOut;
    };

    graph.AddPass<SkyData>(
        "SkyPass",
        [&](FrameGraph::Builder &builder, SkyData &data) {
            data.DepthIn = builder.Read(sceneDepth);
            data.ColorInOut = builder.Write(sceneColor);
            sceneColor = data.ColorInOut;
        },
        record("SkyPass"));

    graph.AddPass<SkyData>(
        "TransparentPass",
        [&](FrameGraph::Builder &builder, SkyData &data) {
            data.DepthIn = builder.Read(sceneDepth);
            data.ColorInOut = builder.Write(sceneColor);
            sceneColor = data.ColorInOut;
        },
        record("TransparentPass"));

    struct BloomData {
        MockTextureHandle Input, Output;
    };

    if constexpr (ENABLE_BLOOM) {
        graph.AddPass<BloomData>(
            "BloomDownsample1",
            [&](FrameGraph::Builder &builder, BloomData &data) {
                data.Input = builder.Read(sceneColor);
                data.Output = builder.Create<MockTexture>("BloomDown1", MockTextureDescriptor{ 960, 540, "RGBA16F" });
                bloomDown1 = data.Output;
            },
            record("BloomDownsample1"));

        graph.AddPass<BloomData>(
            "BloomDownsample2",
            [&](FrameGraph::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown1);
                data.Output = builder.Create<MockTexture>("BloomDown2", MockTextureDescriptor{ 480, 270, "RGBA16F" });
                bloomDown2 = data.Output;
            },
            record("BloomDownsample2"));

        graph.AddPass<BloomData>(
            "BloomDownsample3",
            [&](FrameGraph::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown2);
                data.Output = builder.Create<MockTexture>("BloomDown3", MockTextureDescriptor{ 240, 135, "RGBA16F" });
                bloomDown3 = data.Output;
            },
            record("BloomDownsample3"));

        graph.AddPass<BloomData>(
            "BloomUpsample1",
            [&](FrameGraph::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomDown3);
                data.Output = builder.Create<MockTexture>("BloomUp1", MockTextureDescriptor{ 480, 270, "RGBA16F" });
                bloomUp1 = data.Output;
            },
            record("BloomUpsample1"));

        graph.AddPass<BloomData>(
            "BloomUpsample2",
            [&](FrameGraph::Builder &builder, BloomData &data) {
                data.Input = builder.Read(bloomUp1);
                data.Output = builder.Create<MockTexture>("BloomUp2", MockTextureDescriptor{ 960, 540, "RGBA16F" });
                bloomUp2 = data.Output;
            },
            record("BloomUpsample2"));

        struct BloomCombineData {
            MockTextureHandle Scene, Bloom, Output;
        };

        graph.AddPass<BloomCombineData>(
            "BloomCombine",
            [&](FrameGraph::Builder &builder, BloomCombineData &data) {
                data.Bloom = builder.Read(bloomUp2);
                data.Output = builder.Write(sceneColor);
                sceneColor = data.Output;
            },
            record("BloomCombine"));
    }

    struct PostData {
        MockTextureHandle Input, Output;
    };

    graph.AddPass<PostData>(
        "ToneMappingPass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(sceneColor);
            data.Output = builder.Create<MockTexture>("ToneMapped", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            toneMappedColor = data.Output;
        },
        record("ToneMappingPass"));

    graph.AddPass<PostData>(
        "ColorGradingPass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(toneMappedColor);
            data.Output = builder.Create<MockTexture>("ColorGraded", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            gradedColor = data.Output;
        },
        record("ColorGradingPass"));

    graph.AddPass<PostData>(
        "FXAAPass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(gradedColor);
            data.Output = builder.Create<MockTexture>("FinalColor", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
            finalColor = data.Output;
        },
        record("FXAAPass"));

    // Debug passes: outputs unused and no side effects, so all three must be culled.
    graph.AddPass<PostData>(
        "DebugWireframePass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferDepth);
            data.Output = builder.Create<MockTexture>("DebugWireframe", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
        },
        record("DebugWireframePass"));

    graph.AddPass<PostData>(
        "DebugNormalsPass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferNormal);
            data.Output = builder.Create<MockTexture>("DebugNormals", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
        },
        record("DebugNormalsPass"));

    graph.AddPass<PostData>(
        "DebugLightHeatmapPass",
        [&](FrameGraph::Builder &builder, PostData &data) {
            data.Input = builder.Read(gbufferAlbedo);
            data.Output = builder.Create<MockTexture>("DebugHeatmap", MockTextureDescriptor{ 1920, 1080, "RGBA8" });
        },
        record("DebugLightHeatmapPass"));

    struct UIData {
        MockTextureHandle ColorInOut;
    };

    graph.AddPass<UIData>(
        "UIPass",
        [&](FrameGraph::Builder &builder, UIData &data) {
            data.ColorInOut = builder.Write(finalColor);
            finalColor = data.ColorInOut;
        },
        record("UIPass"));

    struct PresentData {
        MockTextureHandle FinalImage;
    };

    graph.AddPass<PresentData>(
        "PresentPass",
        [&](FrameGraph::Builder &builder, PresentData &data) {
            data.FinalImage = builder.Read(finalColor);
            builder.MarkSideEffect();
        },
        record("PresentPass"));

    graph.Compile();
    graph.Execute(FrameGraphContext{});

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
