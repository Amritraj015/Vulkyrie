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

    /** @brief Reserve hints scaled to a graph's size, so every benchmark reaches the allocation-free steady state
     * the engine runs in rather than measuring the arena and node arrays growing.
     * @param passCount Passes the graph will hold. */
    [[nodiscard]] FrameGraphConfig ConfigFor(u32 passCount) {
        return FrameGraphConfig{ .ExpectedPasses = passCount + 8, .ExpectedResources = (passCount + 8) * 2, .InitialArenaBytes = 128 * 1024 };
    }

    /** @brief Declares a synthetic graph: a spine of producers, each reading a couple of earlier outputs, with a
     * final side-effect pass so nothing upstream is culled. Returns the accumulator the pass bodies feed so a
     * caller can consume it.
     * @param graph The graph to populate.
     * @param sink Accumulator the pass bodies write to, keeping their work from being optimized away.
     * @param passCount Producer passes to declare, one output texture each. */
    void DeclareSyntheticGraph(FrameGraph &graph, u64 &sink, u32 passCount = PASS_COUNT) {
        std::vector<TextureHandle> outputs;
        outputs.reserve(passCount);

        struct PassData {
            TextureHandle Output;
        };

        struct PresentData {};

        for (u32 pass = 0; pass < passCount; ++pass) {
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

    /** @brief Declares a graph where half the passes feed the present chain and half are dead ends, so the cull
     * worklist actually has work to propagate rather than terminating immediately.
     * @param graph The graph to populate.
     * @param sink Accumulator the pass bodies write to.
     * @param passCount Live passes to declare; the same number of dead ones is declared alongside them. */
    void DeclareHalfDeadGraph(FrameGraph &graph, u64 &sink, u32 passCount = PASS_COUNT) {
        std::vector<TextureHandle> live;
        live.reserve(passCount);

        struct PassData {
            TextureHandle Output;
        };

        struct PresentData {};

        for (u32 pass = 0; pass < passCount; ++pass) {
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
    }

    /** @brief Builds `count` graphs, each declared but left uncompiled, so `Compile` can be timed on its own.
     *
     * `Compile` cannot be measured by calling it repeatedly against a single declared graph: the `_compiled` guard
     * turns every call after the first into an early return, so what gets reported is a predicted branch rather
     * than the six derivation stages. Handing each measured run its own fresh graph is the only way to time the
     * real work, and declaring the pool here keeps that declaration outside the timed region.
     *
     * Sized to `Chronometer::runs()`, which Catch2 keeps at a handful for an operation this long, so the pool
     * stays small.
     *
     * @param count How many graphs to declare, i.e. the number of measured runs in one sample.
     * @param passCount Passes each graph will hold, used to size its reserve hints.
     * @param declare Builder invoked as `declare(graph, sink)` to populate each graph.
     * @param sink Accumulator the pass bodies write to.
     * @returns The declared, uncompiled graphs, one per measured run. */
    template <typename TDeclare> [[nodiscard]] std::vector<Scope<FrameGraph>> DeclaredGraphPool(u32 count, u32 passCount, TDeclare &&declare, u64 &sink) {
        std::vector<Scope<FrameGraph>> pool;
        pool.reserve(count);

        for (u32 i = 0; i < count; ++i) {
            Scope<FrameGraph> graph = CreateScope<FrameGraph>(ConfigFor(passCount));
            declare(*graph, sink);
            pool.push_back(std::move(graph));
        }

        return pool;
    }

    /** @brief Times one complete per-frame cycle - declare, compile, reset - which is exactly what the engine pays
     * each frame for a graph of this size.
     * @param meter The chronometer for the calling benchmark.
     * @param passCount Producer passes to declare. */
    void MeasureFullFrameCycle(Catch::Benchmark::Chronometer meter, u32 passCount) {
        FrameGraph graph{ ConfigFor(passCount) };
        u64 sink = 0;

        // A warm-up frame so the arena and every node array reach their high-water mark before anything is timed;
        // the steady state is the state the engine actually runs in.
        DeclareSyntheticGraph(graph, sink, passCount);
        graph.Compile();
        graph.Reset();

        meter.measure([&graph, &sink, passCount] {
            DeclareSyntheticGraph(graph, sink, passCount);
            graph.Compile();

            // Read before the reset: the execution order is empty once the graph has been returned to empty.
            const size_t ordered = graph.GetExecutionOrder().size();
            graph.Reset();

            return ordered;
        });
    }

    /** @brief Times `Compile` on its own for a graph of this size, each run against its own freshly declared graph.
     * @param meter The chronometer for the calling benchmark.
     * @param passCount Producer passes to declare. */
    void MeasureCompileOnly(Catch::Benchmark::Chronometer meter, u32 passCount) {
        u64 sink = 0;

        std::vector<Scope<FrameGraph>> pool = DeclaredGraphPool(
            static_cast<u32>(meter.runs()), passCount, [passCount](FrameGraph &graph, u64 &s) { DeclareSyntheticGraph(graph, s, passCount); }, sink);

        meter.measure([&pool](int run) {
            FrameGraph &graph = *pool[static_cast<size_t>(run)];
            graph.Compile();

            return graph.GetExecutionOrder().size();
        });
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
        u64 sink = 0;

        // One freshly declared graph per run - see DeclaredGraphPool for why a single graph cannot be reused.
        std::vector<Scope<FrameGraph>> pool =
            DeclaredGraphPool(static_cast<u32>(meter.runs()), PASS_COUNT, [](FrameGraph &graph, u64 &s) { DeclareSyntheticGraph(graph, s); }, sink);

        meter.measure([&pool](int run) {
            FrameGraph &graph = *pool[static_cast<size_t>(run)];
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
        u64 sink = 0;

        // Twice PASS_COUNT passes are declared, so the graph is sized for the live and dead halves together.
        std::vector<Scope<FrameGraph>> pool = DeclaredGraphPool(
            static_cast<u32>(meter.runs()), PASS_COUNT * 2, [](FrameGraph &graph, u64 &s) { DeclareHalfDeadGraph(graph, s); }, sink);

        meter.measure([&pool](int run) {
            FrameGraph &graph = *pool[static_cast<size_t>(run)];
            graph.Compile();

            return graph.GetExecutionOrder().size();
        });
    };
}

TEST_CASE("Frame graph scaling", "[framegraph]") {
    // How the per-frame cost grows across the sizes a scene is actually expected to produce. The two families
    // answer different questions: "Full frame cycle" is everything a frame pays - declare, compile, reset - and is
    // the row to watch for regressions, while "Compile only" isolates how much of that is the six derivation
    // stages.
    //
    // Compile is the part that grows superlinearly: the transient aliasing plan sweeps the placement list twice
    // per candidate, so its cost climbs faster than linearly while every other stage does not. That growth is
    // easier to read at sizes past anything realistic, which is what `[framegraph-large]` is for. A graph whose
    // resource types do not implement `GetMemoryRequirements` sits out the plan entirely and keeps the whole
    // compile linear, so that is the first difference to check when these rows disagree with a real frame.

    BENCHMARK_ADVANCED("Full frame cycle - 60 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 60);
    };

    BENCHMARK_ADVANCED("Full frame cycle - 80 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 80);
    };

    BENCHMARK_ADVANCED("Full frame cycle - 100 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 100);
    };

    BENCHMARK_ADVANCED("Compile only - 60 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 60);
    };

    BENCHMARK_ADVANCED("Compile only - 80 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 80);
    };

    BENCHMARK_ADVANCED("Compile only - 100 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 100);
    };
}

// Hidden behind `[.]`, so neither a bare run nor `[framegraph]` picks it up - only an explicit
// `benchmarks "[framegraph-large]"` does. These sizes are far past what a scene is expected to produce and cost
// roughly four times the rest of the frame graph benchmarks put together, which is a poor trade for a run whose
// question is usually "did anything regress". They exist to show the shape of the growth rather than a number to
// track: the compile stages climb as roughly n^1.7 here while declare and reset stay linear, so this is where the
// aliasing plan's quadratic sweeps become the whole cost.
TEST_CASE("Frame graph scaling at large graph sizes", "[.][framegraph-large]") {
    BENCHMARK_ADVANCED("Full frame cycle - 200 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 200);
    };

    BENCHMARK_ADVANCED("Full frame cycle - 400 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 400);
    };

    BENCHMARK_ADVANCED("Full frame cycle - 600 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureFullFrameCycle(meter, 600);
    };

    BENCHMARK_ADVANCED("Compile only - 200 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 200);
    };

    BENCHMARK_ADVANCED("Compile only - 400 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 400);
    };

    BENCHMARK_ADVANCED("Compile only - 600 passes")(Catch::Benchmark::Chronometer meter) {
        MeasureCompileOnly(meter, 600);
    };
}
