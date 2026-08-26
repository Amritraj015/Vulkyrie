#include "support/benchmark_support.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <renderer/frame_graph/frame_graph.h>

#include "renderer/support/mock_backend.h"

#include <string>
#include <vector>

using namespace Vulkyrie;

namespace {

    // Not MockBackend: that one cannot record in parallel, so RecordParallel against it degrades to Record, and
    // its command list allocates a string per barrier batch - which would make these rows a measurement of the mock
    // rather than of the graph.
    using Backend = RendererTests::MockParallelBackend;

    /** @brief A resource type with a POD descriptor and no callbacks, so a benchmark measures the graph rather than
     * the backend. Reports memory requirements so the aliasing planner is exercised too. */
    struct BenchTexture {
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

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor, const Device<Backend> &device) const {
            (void)device;

            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }
    };

    using BenchTextureHandle = FrameGraphHandle<BenchTexture>;

    // The planner is only fed by types that satisfy this, and the concept is opt-in: a signature that drifts out
    // of date silently drops the resource from the plan rather than failing to compile. Asserted so a benchmark
    // measuring "compile including the aliasing plan" cannot quietly stop doing so.
    static_assert(HasMemoryRequirements<BenchTexture, Backend>, "BenchTexture must feed the aliasing planner, or the compile benchmarks stop measuring it.");

    /** @brief The device and frame a benchmark's graph executes against. Nothing here is measured - the mock
     * backend does no work - but the graph needs a typed context to run at all. */
    struct BenchDevice {
    public:
        DeviceCreationInfo Info{ ApplicationInfo{ "FrameGraphBenchmarks", { 1, 0, 0 } }, WindowHandle{}, {}, 64, 64, 16, 16 };
        Device<Backend> Dev{ Info };

        // One command list per worker, because RecordParallel asserts there are at least as many as the job system
        // has workers - two passes sharing a list would race.
        FrameContext<Backend> Frame{ Dev.Context(), 0, JobSystem::WorkerCount(), 0 };
        FrameGraphContext<Backend> Context{ Dev, Frame };
    };

    /** @brief Pass data for a body doing a fixed amount of busy work. Pass bodies are function pointers, so what
     * they accumulate into travels here rather than in a capture. */
    struct WorkPassData {
    public:
        BenchTextureHandle Output;
        u64 *Sink = nullptr;
        u32 Pass = 0;
    };

    /** @brief Pass data for a body that only marks that it ran. */
    struct SinkPassData {
    public:
        u64 *Sink = nullptr;
    };

    /** @brief Busy-work body, kept out of line so every pass shares one function pointer. */
    void RunBusyWork(const WorkPassData &data, FrameGraphPassContext<Backend> &) {
        *data.Sink += Bench::BusyWork(data.Pass, Bench::TINY_WORK);
    }

    /** @brief Minimal body: proves the pass ran without measuring anything but the dispatch. */
    template <typename TPassData> void BumpSink(const TPassData &data, FrameGraphPassContext<Backend> &) {
        ++*data.Sink;
    }

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
    void DeclareSyntheticGraph(FrameGraph<Backend> &graph, u64 &sink, u32 passCount = PASS_COUNT) {
        std::vector<BenchTextureHandle> outputs;
        outputs.reserve(passCount);

        for (u32 pass = 0; pass < passCount; ++pass) {
            graph.AddPass<WorkPassData>(
                "Pass",
                [&outputs, &sink, pass](FrameGraph<Backend>::Builder &builder, WorkPassData &data) {
                    for (u32 read = 0; read < READS_PER_PASS && read < pass; ++read) {
                        (void)builder.Read(outputs[pass - read - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Target", BenchTexture::Descriptor{ 1920, 1080 });
                    data.Sink = &sink;
                    data.Pass = pass;
                    outputs.push_back(data.Output);
                },
                &RunBusyWork);
        }

        graph.AddPass<SinkPassData>(
            "Present",
            [&outputs, &sink](FrameGraph<Backend>::Builder &builder, SinkPassData &data) {
                (void)builder.Read(outputs.back());
                data.Sink = &sink;
                builder.MarkSideEffect();
            },
            &BumpSink<SinkPassData>);
    }

    /** @brief Declares a graph where half the passes feed the present chain and half are dead ends, so the cull
     * worklist actually has work to propagate rather than terminating immediately.
     * @param graph The graph to populate.
     * @param sink Accumulator the pass bodies write to.
     * @param passCount Live passes to declare; the same number of dead ones is declared alongside them. */
    void DeclareHalfDeadGraph(FrameGraph<Backend> &graph, u64 &sink, u32 passCount = PASS_COUNT) {
        std::vector<BenchTextureHandle> live;
        live.reserve(passCount);

        for (u32 pass = 0; pass < passCount; ++pass) {
            graph.AddPass<WorkPassData>(
                "Live",
                [&live, &sink, pass](FrameGraph<Backend>::Builder &builder, WorkPassData &data) {
                    if (pass > 0) {
                        (void)builder.Read(live[pass - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Live", BenchTexture::Descriptor{ 512, 512 });
                    data.Sink = &sink;
                    live.push_back(data.Output);
                },
                &BumpSink<WorkPassData>);

            graph.AddPass<WorkPassData>(
                "Dead",
                [&sink](FrameGraph<Backend>::Builder &builder, WorkPassData &data) {
                    data.Output = builder.Create<BenchTexture>("Dead", BenchTexture::Descriptor{ 512, 512 });
                    data.Sink = &sink;
                },
                &BumpSink<WorkPassData>);
        }

        graph.AddPass<SinkPassData>(
            "Present",
            [&live, &sink](FrameGraph<Backend>::Builder &builder, SinkPassData &data) {
                (void)builder.Read(live.back());
                data.Sink = &sink;
                builder.MarkSideEffect();
            },
            &BumpSink<SinkPassData>);
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
     * @param device The device the graphs acquire against.
     * @returns The declared, uncompiled graphs, one per measured run. */
    template <typename TDeclare>
    [[nodiscard]] std::vector<Scope<FrameGraph<Backend>>> DeclaredGraphPool(u32 count, u32 passCount, TDeclare &&declare, u64 &sink, Device<Backend> &device) {
        std::vector<Scope<FrameGraph<Backend>>> pool;
        pool.reserve(count);

        for (u32 i = 0; i < count; ++i) {
            Scope<FrameGraph<Backend>> graph = CreateScope<FrameGraph<Backend>>(device, ConfigFor(passCount));
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
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, ConfigFor(passCount) };
        u64 sink = 0;

        // A warm-up frame so the arena and every node array reach their high-water mark before anything is timed;
        // the steady state is the state the engine actually runs in.
        DeclareSyntheticGraph(graph, sink, passCount);
        graph.Compile();
        graph.Reset();

        meter.measure([&graph, &sink, &device, passCount] {
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

        BenchDevice device;

        std::vector<Scope<FrameGraph<Backend>>> pool = DeclaredGraphPool(
            static_cast<u32>(meter.runs()),
            passCount,
            [passCount](FrameGraph<Backend> &graph, u64 &s) { DeclareSyntheticGraph(graph, s, passCount); },
            sink,
            device.Dev);

        meter.measure([&pool, &device](int run) {
            FrameGraph<Backend> &graph = *pool[static_cast<size_t>(run)];
            graph.Compile();

            return graph.GetExecutionOrder().size();
        });
    }

} // namespace

TEST_CASE("Frame graph construction", "[framegraph]") {
    BENCHMARK_ADVANCED("Declare 41 passes into a warm graph")(Catch::Benchmark::Chronometer meter) {
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
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
        // The cold path, for comparison: every buffer and the arena grow from nothing. The device is built outside
        // the measured body so only the graph's own growth is timed.
        BenchDevice device;

        meter.measure([&device] {
            FrameGraph<Backend> graph{ device.Dev };
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
        BenchDevice device;

        std::vector<Scope<FrameGraph<Backend>>> pool = DeclaredGraphPool(
            static_cast<u32>(meter.runs()), PASS_COUNT, [](FrameGraph<Backend> &graph, u64 &s) { DeclareSyntheticGraph(graph, s); }, sink, device.Dev);

        meter.measure([&pool, &device](int run) {
            FrameGraph<Backend> &graph = *pool[static_cast<size_t>(run)];
            graph.Compile();

            return graph.GetExecutionOrder().size();
        });
    };

    BENCHMARK_ADVANCED("Declare + compile a full frame")(Catch::Benchmark::Chronometer meter) {
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;

        DeclareSyntheticGraph(graph, sink);
        graph.Compile();
        graph.Reset();

        meter.measure([&graph, &sink, &device] {
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
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;

        DeclareSyntheticGraph(graph, sink);
        graph.Compile();

        meter.measure([&graph, &sink, &device] {
            graph.Execute(device.Frame);
            return sink;
        });
    };

    BENCHMARK_ADVANCED("Record + Submit (serial)")(Catch::Benchmark::Chronometer meter) {
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        u64 sink = 0;

        DeclareSyntheticGraph(graph, sink);
        graph.Compile();

        meter.measure([&graph, &sink, &device] {
            graph.Record(device.Frame);
            graph.Submit(device.Frame);
            return sink;
        });
    };

    BENCHMARK_ADVANCED("Record + Submit (parallel)")(Catch::Benchmark::Chronometer meter) {
        // The pass bodies here are deliberately tiny, so this row mostly reports the fan-out overhead rather than
        // a speed-up. It is the baseline the number to beat once passes record real command buffers.
        BenchDevice device;
        FrameGraph<Backend> graph{ device.Dev, FrameGraphConfig{ .ExpectedPasses = 64, .ExpectedResources = 128, .InitialArenaBytes = 64 * 1024 } };
        std::atomic<u64> sink{ 0 };

        std::vector<BenchTextureHandle> outputs;
        outputs.reserve(PASS_COUNT);

        // Concurrent bodies accumulate into an atomic, reached through the pass data like every other body.
        struct AtomicPassData {
            BenchTextureHandle Output;
            std::atomic<u64> *Sink = nullptr;
            u32 Pass = 0;
        };

        for (u32 pass = 0; pass < PASS_COUNT; ++pass) {
            graph.AddPass<AtomicPassData>(
                "Pass",
                [&outputs, &sink, pass](FrameGraph<Backend>::Builder &builder, AtomicPassData &data) {
                    for (u32 read = 0; read < READS_PER_PASS && read < pass; ++read) {
                        (void)builder.Read(outputs[pass - read - 1]);
                    }

                    data.Output = builder.Create<BenchTexture>("Target", BenchTexture::Descriptor{ 1920, 1080 });
                    data.Sink = &sink;
                    data.Pass = pass;
                    outputs.push_back(data.Output);
                },
                [](const AtomicPassData &data, FrameGraphPassContext<Backend> &) {
                    data.Sink->fetch_add(Bench::BusyWork(data.Pass, Bench::SMALL_WORK), std::memory_order_relaxed);
                });
        }

        graph.AddPass<AtomicPassData>(
            "Present",
            [&outputs, &sink](FrameGraph<Backend>::Builder &builder, AtomicPassData &data) {
                (void)builder.Read(outputs.back());
                data.Sink = &sink;
                builder.MarkSideEffect();
            },
            [](const AtomicPassData &data, FrameGraphPassContext<Backend> &) { data.Sink->fetch_add(1, std::memory_order_relaxed); });

        graph.Compile();

        meter.measure([&graph, &sink, &device] {
            graph.RecordParallel(device.Frame);
            graph.Submit(device.Frame);
            return sink.load(std::memory_order_relaxed);
        });
    };
}

TEST_CASE("Frame graph culling", "[framegraph]") {
    BENCHMARK_ADVANCED("Compile a graph that is half dead")(Catch::Benchmark::Chronometer meter) {
        u64 sink = 0;

        // Twice PASS_COUNT passes are declared, so the graph is sized for the live and dead halves together.
        BenchDevice device;

        std::vector<Scope<FrameGraph<Backend>>> pool = DeclaredGraphPool(
            static_cast<u32>(meter.runs()), PASS_COUNT * 2, [](FrameGraph<Backend> &graph, u64 &s) { DeclareHalfDeadGraph(graph, s); }, sink, device.Dev);

        meter.measure([&pool, &device](int run) {
            FrameGraph<Backend> &graph = *pool[static_cast<size_t>(run)];
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
