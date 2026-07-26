// Whether ParallelFor is worth using, and at what grain. Every parallel row here is directly
// comparable to the serial row above it: same items, same per-item work, same arithmetic — only
// the dispatch differs. A parallel row slower than the serial one means the chunk overhead is
// bigger than the chunk's work, which is exactly what the grain sweep is for.
#include "support/benchmark_support.h"

#include "core/jobs/parallel_for.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <string>

using namespace Vulkyrie;

namespace {

    constexpr u32 kItems = 10000;

    /** Grain sizes spanning the whole range of behaviour at kItems: 1 saturates the 1024-chunk cap
     * (maximum parallelism, maximum overhead), 4096 collapses to three chunks. */
    constexpr std::array<u32, 4> kGrainSizes{ 1U, 32U, 256U, 4096U };

    /** @brief The workload every row in this file runs, as a plain serial loop.
     * @returns The accumulated result, which the caller must consume.
     */
    [[nodiscard]] u64 SerialWorkload() {
        u64 accumulator = 0;

        for (u32 i = 0; i < kItems; ++i) {
            accumulator += Bench::BusyWork(i, Bench::SMALL_WORK);
        }

        return accumulator;
    }

    /** @brief The same workload through ParallelForRange. Each chunk accumulates locally and folds
     * once at the end, so the shared atomic is touched per chunk rather than per item — otherwise
     * the benchmark would be measuring contention on the counter, not the loop.
     * @param grainSize Target items per chunk.
     * @returns The accumulated result, which the caller must consume.
     */
    [[nodiscard]] u64 ParallelWorkload(u32 grainSize) {
        std::atomic<u64> accumulator{ 0 };

        ParallelForRange(kItems, grainSize, [&accumulator](u32 /*chunkIndex*/, u32 begin, u32 end) {
            u64 local = 0;

            for (u32 i = begin; i < end; ++i) {
                local += Bench::BusyWork(i, Bench::SMALL_WORK);
            }

            accumulator.fetch_add(local, std::memory_order_relaxed);
        });

        return accumulator.load(std::memory_order_relaxed);
    }

} // namespace

TEST_CASE("ParallelFor against a serial loop", "[jobs][parallel-for]") {
    BENCHMARK("Serial loop, 10000 items") {
        return SerialWorkload();
    };

    for (const u32 grain : kGrainSizes) {
        BENCHMARK("ParallelForRange, grain " + std::to_string(grain)) {
            return ParallelWorkload(grain);
        };
    }
}

TEST_CASE("ParallelFor scales with worker count", "[jobs][parallel-for][scaling]") {
    // Same workload, same grain, one row per pool size. The synchronous row is the serial baseline
    // measured through the ParallelFor machinery, so the gap between it and the serial loop above
    // is pure partitioning overhead.
    constexpr u32 kGrain = 64;

    for (const u32 workers : Bench::ScalingWorkerCounts()) {
        const Bench::JobSystemScope scope(workers);

        BENCHMARK("10000 items of SMALL_WORK - " + Bench::WorkerLabel(workers)) {
            return ParallelWorkload(kGrain);
        };
    }
}
