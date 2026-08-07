#include "support/benchmark_support.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <renderer/frame_graph/frame_graph.h>

#include <string>
#include <vector>

using namespace Vulkyrie;

namespace {

    /** @brief A backend with a POD descriptor and no callbacks, so a benchmark measures the graph rather than the
     * backend. Reports memory requirements so the aliasing planner is exercised too. */
    struct BenchTexture {
    public:
        struct Descriptor {
        public:
            u32 Width = 0;
            u32 Height = 0;
        };

        void Create(const Descriptor &, const FrameGraphContext &) {
        }

        void Destroy(const Descriptor &, const FrameGraphContext &) {
        }

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor) const {
            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }
    };

    using TextureHandle = FrameGraphHandle<BenchTexture>;

    /** @brief Passes in the synthetic graph, matching the plan's 40-pass / 60-resource reference shape. */
    constexpr u32 PASS_COUNT = 40;

    /** @brief How many earlier outputs a pass reads, which is what gives the graph its fan-in. */
    constexpr u32 READS_PER_PASS = 2;

    /** @brief Declares a synthetic 40-pass graph: a spine of producers, each reading a couple of earlier outputs,
     * with a final side-effect pass so nothing upstream is culled. Returns the accumulator the pass bodies feed so
     * a caller can consume it.
     * @param graph The graph to populate.
     * @param sink Accumulator the pass bodies write to, keeping their work from being optimized away. */
    void DeclareSyntheticGraph(FrameGraph &graph, u64 &sink) {
        std::vector<TextureHandle> outputs;
        outputs.reserve(PASS_COUNT);

        struct PassData {
            TextureHandle Output;
        };

        struct PresentData {};

        for (u32 pass = 0; pass < PASS_COUNT; ++pass) {
            graph.AddPass<PassData>(
                "Pass",
                [&outputs, pass](FrameGraph::Builder &builder, PassData &data) {
                    for (u32 read = 0; read < READS_PER_PASS && read < pass; ++read) {
                        (void)builder.Read(outputs[pass - read - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Target", BenchTexture::Descriptor{ 1920, 1080 });
                    outputs.push_back(data.Output);
                },
                [&sink, pass](const PassData &, FrameGraph &, const FrameGraphContext &) { sink += Bench::BusyWork(pass, Bench::TINY_WORK); });
        }

        graph.AddPass<PresentData>(
            "Present",
            [&outputs](FrameGraph::Builder &builder, PresentData &) {
                (void)builder.Read(outputs.back());
                builder.MarkSideEffect();
            },
            [&sink](const PresentData &, FrameGraph &, const FrameGraphContext &) { sink += 1; });
    }

} // namespace

TEST_CASE("Frame graph construction", "[framegraph]") {
    BENCHMARK_ADVANCED("Declare 41 passes into a warm graph")(Catch::Benchmark::Chronometer meter) {
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;

        // One warm-up frame so the arena and every node array reach their high-water mark; what is measured is the
        // steady state, which is the state the engine actually runs in.
        DeclareSyntheticGraph(graph, sink);
        graph.Compile();
        graph.Reset();

        meter.measure([&graph, &sink] {
            DeclareSyntheticGraph(graph, sink);
            graph.Reset();
            return sink;
        });
    };

    BENCHMARK_ADVANCED("Declare 41 passes into a fresh graph")(Catch::Benchmark::Chronometer meter) {
        // The cold path, for comparison: every buffer and the arena grow from nothing.
        meter.measure([] {
            FrameGraph graph;
            u64 sink = 0;
            DeclareSyntheticGraph(graph, sink);
            return sink + graph.GetPassCount();
        });
    };
}

TEST_CASE("Frame graph compilation", "[framegraph]") {
    BENCHMARK_ADVANCED("Compile 41 passes / 41 resources")(Catch::Benchmark::Chronometer meter) {
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;
        DeclareSyntheticGraph(graph, sink);

        // Compile is idempotent, so it can be measured repeatedly against one declared graph.
        meter.measure([&graph] {
            graph.Compile();
            return graph.GetExecutionOrder().size();
        });
    };

    BENCHMARK_ADVANCED("Declare + compile a full frame")(Catch::Benchmark::Chronometer meter) {
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;

        DeclareSyntheticGraph(graph, sink);
        graph.Compile();
        graph.Reset();

        meter.measure([&graph, &sink] {
            DeclareSyntheticGraph(graph, sink);
            graph.Compile();
            const size_t ordered = graph.GetExecutionOrder().size();
            graph.Reset();
            return ordered;
        });
    };
}

TEST_CASE("Frame graph execution", "[framegraph]") {
    BENCHMARK_ADVANCED("Execute 41 passes (serial, fused)")(Catch::Benchmark::Chronometer meter) {
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;
        DeclareSyntheticGraph(graph, sink);
        graph.Compile();

        meter.measure([&graph, &sink] {
            graph.Execute(FrameGraphContext{});
            return sink;
        });
    };

    BENCHMARK_ADVANCED("Record + Submit (serial)")(Catch::Benchmark::Chronometer meter) {
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;
        DeclareSyntheticGraph(graph, sink);
        graph.Compile();

        meter.measure([&graph, &sink] {
            const FrameGraphContext context{};
            graph.Record(context);
            graph.Submit(context);
            return sink;
        });
    };

    BENCHMARK_ADVANCED("Record + Submit (parallel)")(Catch::Benchmark::Chronometer meter) {
        // The pass bodies here are deliberately tiny, so this row mostly reports the fan-out overhead rather than
        // a speed-up. It is the baseline the number to beat once passes record real command buffers.
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        std::atomic<u64> sink{ 0 };

        std::vector<TextureHandle> outputs;
        outputs.reserve(PASS_COUNT);

        struct PassData {
            TextureHandle Output;
        };

        struct PresentData {};

        for (u32 pass = 0; pass < PASS_COUNT; ++pass) {
            graph.AddPass<PassData>(
                "Pass",
                [&outputs, pass](FrameGraph::Builder &builder, PassData &data) {
                    for (u32 read = 0; read < READS_PER_PASS && read < pass; ++read) {
                        (void)builder.Read(outputs[pass - read - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Target", BenchTexture::Descriptor{ 1920, 1080 });
                    outputs.push_back(data.Output);
                },
                [&sink, pass](const PassData &, FrameGraph &, const FrameGraphContext &) {
                    sink.fetch_add(Bench::BusyWork(pass, Bench::SMALL_WORK), std::memory_order_relaxed);
                });
        }

        graph.AddPass<PresentData>(
            "Present",
            [&outputs](FrameGraph::Builder &builder, PresentData &) {
                (void)builder.Read(outputs.back());
                builder.MarkSideEffect();
            },
            [&sink](const PresentData &, FrameGraph &, const FrameGraphContext &) { sink.fetch_add(1, std::memory_order_relaxed); });

        graph.Compile();

        meter.measure([&graph, &sink] {
            const FrameGraphContext context{};
            graph.RecordParallel(context);
            graph.Submit(context);
            return sink.load(std::memory_order_relaxed);
        });
    };
}

TEST_CASE("Frame graph culling", "[framegraph]") {
    BENCHMARK_ADVANCED("Compile a graph that is half dead")(Catch::Benchmark::Chronometer meter) {
        // Half the passes feed the present chain and half are dead ends, so the cull worklist actually has work to
        // propagate rather than terminating immediately.
        FrameGraph graph{ FrameGraphConfig{ .ExpectedPasses = 128, .ExpectedResources = 256, .InitialArenaBytes = 128 * 1024 } };
        u64 sink = 0;

        std::vector<TextureHandle> live;
        live.reserve(PASS_COUNT);

        struct PassData {
            TextureHandle Output;
        };

        struct PresentData {};

        for (u32 pass = 0; pass < PASS_COUNT; ++pass) {
            graph.AddPass<PassData>(
                "Live",
                [&live, pass](FrameGraph::Builder &builder, PassData &data) {
                    if (pass > 0) {
                        (void)builder.Read(live[pass - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Live", BenchTexture::Descriptor{ 512, 512 });
                    live.push_back(data.Output);
                },
                [&sink](const PassData &, FrameGraph &, const FrameGraphContext &) { ++sink; });

            graph.AddPass<PassData>(
                "Dead",
                [](FrameGraph::Builder &builder, PassData &data) { data.Output = builder.Create<BenchTexture>("Dead", BenchTexture::Descriptor{ 512, 512 }); },
                [&sink](const PassData &, FrameGraph &, const FrameGraphContext &) { ++sink; });
        }

        graph.AddPass<PresentData>(
            "Present",
            [&live](FrameGraph::Builder &builder, PresentData &) {
                (void)builder.Read(live.back());
                builder.MarkSideEffect();
            },
            [&sink](const PresentData &, FrameGraph &, const FrameGraphContext &) { ++sink; });

        meter.measure([&graph] {
            graph.Compile();
            return graph.GetExecutionOrder().size();
        });
    };
}
