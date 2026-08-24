// Measures what a frame actually pays for transients that come from `TransientPool` - the path
// `FrameGraphTexture` takes, rather than a synthetic resource type with a no-op Acquire. This is the row that
// moves when descriptor identity moves.
#include "support/benchmark_support.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <renderer/frame_graph/frame_graph.h>
#include <renderer/frame_graph/resources/frame_graph_texture.h>

#include "renderer/support/mock_backend.h"

#include <vector>

using namespace Vulkyrie;

namespace {

    using Backend = RendererTests::MockBackend;
    using Texture = FrameGraphTexture<Backend>;
    using TextureHandle = FrameGraphHandle<Texture>;

    /** @brief Transients declared per frame. */
    constexpr u32 TRANSIENT_COUNT = 32;

    /** @brief Distinct descriptors they are drawn from, cycled, so the pool has several buckets to serve. */
    constexpr u32 DESCRIPTOR_COUNT = 4;

    struct ProduceData {
    public:
        TextureHandle Output;
    };

    struct ConsumeData {
    public:
        u32 Unused = 0;
    };

    struct PooledFixture final {
    public:
        DeviceCreationInfo Info{ ApplicationInfo{ "PooledTransientsBenchmarks", { 1, 0, 0 } }, WindowHandle{}, Extent2D{ 800, 600 }, 256, 256, 16, 16 };
        Device<Backend> Dev{ Info };
        FrameContext<Backend> Frame{ Dev.Context(), 0, 1, 0 };
        FrameGraphContext<Backend> Context{ Dev, Frame };

        std::vector<TransientTextureID> Ids;

        PooledFixture() {
            for (u32 i = 0; i < DESCRIPTOR_COUNT; ++i) {
                Ids.push_back(
                    Dev.GetRegistry().Register(TextureDescriptor{ .Width = 256U + (i * 128U), .Height = 256U + (i * 128U), .Format = Format::RGBA8Unorm }));
            }
        }
    };

    /** @brief Declares one frame: N producers each taking a transient, and one consumer reading them all so
     * nothing is culled. */
    void DeclareFrame(FrameGraph<Backend> &graph, PooledFixture &fixture) {
        std::vector<TextureHandle> outputs;
        outputs.reserve(TRANSIENT_COUNT);

        for (u32 i = 0; i < TRANSIENT_COUNT; ++i) {
            const TransientTextureID id = fixture.Ids[i % DESCRIPTOR_COUNT];

            const ProduceData &data = graph.AddPass<ProduceData>(
                "Produce",
                [id](FrameGraph<Backend>::Builder &builder, ProduceData &d) { d.Output = builder.Create<Texture>("Transient", id); },
                [](const ProduceData &, FrameGraphPassContext<Backend> &) {});

            outputs.push_back(data.Output);
        }

        graph.AddPass<ConsumeData>(
            "Consume",
            [&outputs](FrameGraph<Backend>::Builder &builder, ConsumeData &) {
                for (const TextureHandle handle : outputs) {
                    (void)builder.Read(handle);
                }

                builder.MarkSideEffect();
            },
            [](const ConsumeData &, FrameGraphPassContext<Backend> &) {});
    }

} // namespace

TEST_CASE("Frame graph pooled transients", "[framegraph][pool]") {
    BENCHMARK_ADVANCED("Full frame, 32 pooled transients")(Catch::Benchmark::Chronometer meter) {
        PooledFixture fixture;
        FrameGraph<Backend> graph{ fixture.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 128 * 1024 } };

        // Warm-up frame so the arena, the node arrays and the pool all reach their steady state first.
        fixture.Dev.GetTransients().ResetFrame();
        DeclareFrame(graph, fixture);
        graph.Compile();
        graph.Execute(fixture.Frame);
        graph.Reset();

        meter.measure([&graph, &fixture] {
            fixture.Dev.GetTransients().ResetFrame();
            DeclareFrame(graph, fixture);
            graph.Compile();
            graph.Execute(fixture.Frame);

            const size_t ordered = graph.GetExecutionOrder().size();
            graph.Reset();

            return ordered;
        });
    };
}
