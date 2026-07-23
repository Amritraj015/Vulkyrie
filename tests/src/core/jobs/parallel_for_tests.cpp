#include "core/jobs/parallel_for.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace Vulkyrie;

// The partition contract is pure and machine-independent: provable at compile time.
static_assert(ChunkCountFor(0, 16) == 0);
static_assert(ChunkCountFor(1, 16) == 1);
static_assert(ChunkCountFor(10, 100) == 1, "grainSize > count yields a single chunk");
static_assert(ChunkCountFor(1000, 10) == 100);
static_assert(ChunkCountFor(1'000'000, 10) == MAX_PARALLEL_FOR_CHUNKS, "chunk count is clamped");
static_assert(ChunkCountFor(100, 0) == 100, "grainSize 0 is treated as 1");
static_assert(ChunkRangeFor(10, 3, 0) == IndexRange{ 0, 4 });
static_assert(ChunkRangeFor(10, 3, 1) == IndexRange{ 4, 7 });
static_assert(ChunkRangeFor(10, 3, 2) == IndexRange{ 7, 10 });

namespace {

    /** The deterministic workload the multi-worker-count test replays: per-chunk buffers keyed by
     * chunk index, merged in ascending chunk order. */
    [[nodiscard]] std::vector<u64> RunDeterministicWorkload() {
        constexpr u32 kCount = 5000;
        constexpr u32 kGrain = 16;

        const u32 chunkCount = ChunkCountFor(kCount, kGrain);
        ChunkedOutput<u64> output(chunkCount);

        ParallelForRange(kCount, kGrain, [&output](u32 chunkIndex, u32 begin, u32 end) {
            std::vector<u64> &items = output.Chunk(chunkIndex);
            for (u32 i = begin; i < end; ++i) {
                items.push_back((static_cast<u64>(i) * 2654435761ULL) ^ chunkIndex);
            }
        });

        std::vector<u64> merged;
        output.MergeInto(merged);
        return merged;
    }

} // namespace

TEST_CASE("Partition chunks tile [0, count) exactly once with no gap or overlap", "[jobs]") {
    constexpr std::array<std::array<u32, 2>, 7> kCases{ { { 1, 16 }, { 7, 3 }, { 100, 1 }, { 1000, 7 }, { 5000, 16 }, { 37, 100 }, { 100000, 10 } } };

    for (const auto &[count, grain] : kCases) {
        const u32 chunkCount = ChunkCountFor(count, grain);
        REQUIRE(chunkCount >= 1);
        REQUIRE(chunkCount <= MAX_PARALLEL_FOR_CHUNKS);

        std::vector<u32> hits(count, 0);
        u32 expectedBegin = 0;
        for (u32 chunk = 0; chunk < chunkCount; ++chunk) {
            const IndexRange range = ChunkRangeFor(count, chunkCount, chunk);
            REQUIRE(range.Begin == expectedBegin); // Contiguous.
            REQUIRE(range.Begin <= range.End);
            REQUIRE(range.End <= count);
            for (u32 i = range.Begin; i < range.End; ++i) {
                ++hits[i];
            }
            expectedBegin = range.End;
        }
        REQUIRE(expectedBegin == count); // Full coverage.

        u32 mismatches = 0;
        for (const u32 hit : hits) {
            if (hit != 1) {
                ++mismatches;
            }
        }
        REQUIRE(mismatches == 0);
    }
}

TEST_CASE("ParallelForRange with count 0 never invokes the body", "[jobs]") {
    std::atomic<u32> calls{ 0 };
    ParallelForRange(0, 16, [&calls](u32, u32, u32) { calls.fetch_add(1, std::memory_order_relaxed); });
    REQUIRE(calls.load(std::memory_order_relaxed) == 0);
}

TEST_CASE("ParallelForRange with grainSize > count runs one chunk covering everything", "[jobs]") {
    std::atomic<u32> calls{ 0 };
    std::atomic<u32> observedChunk{ 99 };
    std::atomic<u32> observedBegin{ 99 };
    std::atomic<u32> observedEnd{ 0 };

    ParallelForRange(10, 100, [&](u32 chunkIndex, u32 begin, u32 end) {
        calls.fetch_add(1, std::memory_order_relaxed);
        observedChunk.store(chunkIndex, std::memory_order_relaxed);
        observedBegin.store(begin, std::memory_order_relaxed);
        observedEnd.store(end, std::memory_order_relaxed);
    });

    REQUIRE(calls.load(std::memory_order_relaxed) == 1);
    REQUIRE(observedChunk.load(std::memory_order_relaxed) == 0);
    REQUIRE(observedBegin.load(std::memory_order_relaxed) == 0);
    REQUIRE(observedEnd.load(std::memory_order_relaxed) == 10);
}

TEST_CASE("ParallelFor visits every index exactly once", "[jobs]") {
    constexpr u32 kCount = 100000;

    auto counts = std::make_unique<std::atomic<u32>[]>(kCount);
    ParallelFor(kCount, 64, [&counts](u32 index) { counts[index].fetch_add(1, std::memory_order_relaxed); });

    u32 mismatches = 0;
    for (u32 i = 0; i < kCount; ++i) {
        if (counts[i].load(std::memory_order_relaxed) != 1) {
            ++mismatches;
        }
    }
    REQUIRE(mismatches == 0);
}

TEST_CASE("ParallelForRange output is byte-identical for any worker count", "[jobs]") {
    // Baseline under the listener's default (auto) worker count.
    const std::vector<u64> baseline = RunDeterministicWorkload();
    REQUIRE(baseline.size() == 5000);

    const u32 autoWorkers = std::max(std::thread::hardware_concurrency(), 2U) - 1U;
    const std::array<u32, 4> workerCounts{ 0U, 1U, 2U, autoWorkers };

    for (const u32 workers : workerCounts) {
        JobSystem::Shutdown();

        if (workers > 0) {
            JobSystemConfig config{};
            config.WorkerCount = workers;
            config.PinToCores = false;
            JobSystem::Initialize(config);
        }
        // workers == 0: stay uninitialized so the first use lazily bootstraps the fully
        // synchronous zero-worker instance — the inline debug path must produce the same bytes.

        REQUIRE(RunDeterministicWorkload() == baseline);
    }

    // Restore the listener's configuration for the tests that follow.
    JobSystem::Shutdown();
    JobSystemConfig config{};
    config.PinToCores = false;
    JobSystem::Initialize(config);
}
