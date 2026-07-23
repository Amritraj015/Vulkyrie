#include "core/jobs/job_system.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <numeric>
#include <vector>

using namespace Vulkyrie;

TEST_CASE("JobSystem runs every submitted job exactly once and Wait blocks until completion", "[jobs]") {
    constexpr u32 kJobCount = 512;

    auto executionCounts = std::make_unique<std::atomic<u32>[]>(kJobCount);
    std::vector<JobHandle> handles;
    handles.reserve(kJobCount);

    for (u32 i = 0; i < kJobCount; ++i) {
        handles.push_back(JobSystem::Run([&executionCounts, i] { executionCounts[i].fetch_add(1, std::memory_order_relaxed); }));
    }

    for (const JobHandle &handle : handles) {
        JobSystem::Wait(handle);
        REQUIRE(JobSystem::IsComplete(handle));
    }

    u32 mismatches = 0;
    for (u32 i = 0; i < kJobCount; ++i) {
        if (executionCounts[i].load(std::memory_order_relaxed) != 1) {
            ++mismatches;
        }
    }
    REQUIRE(mismatches == 0);
}

TEST_CASE("IsComplete treats invalid and stale handles as finished", "[jobs]") {
    REQUIRE(JobSystem::IsComplete(JobHandle{}));

    const JobHandle handle = JobSystem::Run([] {});
    JobSystem::Wait(handle);
    REQUIRE(JobSystem::IsComplete(handle));
}

TEST_CASE("A job can create and wait on another job without deadlocking", "[jobs]") {
    std::atomic<bool> innerRan{ false };
    std::atomic<bool> innerFinishedFirst{ false };

    const JobHandle outer = JobSystem::Run([&innerRan, &innerFinishedFirst] {
        const JobHandle inner = JobSystem::Run([&innerRan] { innerRan.store(true, std::memory_order_release); });
        JobSystem::Wait(inner);
        innerFinishedFirst.store(innerRan.load(std::memory_order_acquire), std::memory_order_relaxed);
    });

    JobSystem::Wait(outer);
    REQUIRE(innerRan.load(std::memory_order_relaxed));
    REQUIRE(innerFinishedFirst.load(std::memory_order_relaxed));
}

TEST_CASE("Job payloads up to the inline capacity are stored and invoked correctly", "[jobs]") {
    // 48 bytes of data + an 8-byte reference = 56 bytes: close to JOB_PAYLOAD_CAPACITY without
    // exceeding it (a larger capture is a compile error by design).
    std::array<u8, 48> data{};
    std::iota(data.begin(), data.end(), u8{ 1 });
    const u64 expected = std::accumulate(data.begin(), data.end(), u64{ 0 });

    std::atomic<u64> sum{ 0 };
    const JobHandle handle = JobSystem::Run([data, &sum] {
        u64 total = 0;
        for (const u8 value : data) {
            total += value;
        }
        sum.store(total, std::memory_order_relaxed);
    });

    JobSystem::Wait(handle);
    REQUIRE(sum.load(std::memory_order_relaxed) == expected);
}

TEST_CASE("Non-trivially-destructible captures are destroyed after execution", "[jobs]") {
    auto shared = CreateRef<i32>(42);
    std::atomic<i32> seen{ 0 };

    const JobHandle handle = JobSystem::Run([shared, &seen] { seen.store(*shared, std::memory_order_relaxed); });
    JobSystem::Wait(handle);

    REQUIRE(seen.load(std::memory_order_relaxed) == 42);
    // Wait acquires the job's Finished store, which the payload destruction happens-before: the
    // job's copy of the shared_ptr is gone again.
    REQUIRE(shared.use_count() == 1);
}

TEST_CASE("CurrentWorkerIndex is in range inside jobs and WorkerCount includes the main thread", "[jobs]") {
    REQUIRE(JobSystem::IsInitialized());
    REQUIRE(JobSystem::WorkerCount() >= 1);
    REQUIRE(JobSystem::CurrentWorkerIndex() == 0); // The test runner is the main thread.

    std::atomic<u32> observedIndex{ INVALID_WORKER_INDEX };
    const JobHandle handle = JobSystem::Run([&observedIndex] { observedIndex.store(JobSystem::CurrentWorkerIndex(), std::memory_order_relaxed); });
    JobSystem::Wait(handle);

    REQUIRE(observedIndex.load(std::memory_order_relaxed) < JobSystem::WorkerCount());
}

TEST_CASE("Stress: thousands of small jobs under steal pressure each run exactly once", "[jobs]") {
    constexpr u32 kWaves = 6;
    constexpr u32 kJobsPerWave = 4000;

    std::atomic<u64> executed{ 0 };
    auto executionCounts = std::make_unique<std::atomic<u32>[]>(kJobsPerWave);
    std::vector<JobHandle> handles;
    handles.reserve(kJobsPerWave);

    for (u32 wave = 0; wave < kWaves; ++wave) {
        for (u32 i = 0; i < kJobsPerWave; ++i) {
            executionCounts[i].store(0, std::memory_order_relaxed);
        }
        handles.clear();

        for (u32 i = 0; i < kJobsPerWave; ++i) {
            handles.push_back(JobSystem::Run([&executed, &executionCounts, i] {
                executionCounts[i].fetch_add(1, std::memory_order_relaxed);
                executed.fetch_add(1, std::memory_order_relaxed);
            }));
        }

        for (const JobHandle &handle : handles) {
            JobSystem::Wait(handle);
        }

        u32 mismatches = 0;
        for (u32 i = 0; i < kJobsPerWave; ++i) {
            if (executionCounts[i].load(std::memory_order_relaxed) != 1) {
                ++mismatches;
            }
        }
        REQUIRE(mismatches == 0);
    }

    REQUIRE(executed.load(std::memory_order_relaxed) == static_cast<u64>(kWaves) * kJobsPerWave);
}
