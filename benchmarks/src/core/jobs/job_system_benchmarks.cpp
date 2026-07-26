// What the scheduler costs and whether workers actually buy throughput. Three questions, in order:
// what does one job cost when the job itself does nothing (the floor under every other number
// here), what does a dependency edge add, and does the same fixed workload get faster as the pool
// grows. Run in Release — a Debug number measures the unoptimized build, not the design.
#include "support/benchmark_support.h"

#include "core/jobs/job_system.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <vector>

using namespace Vulkyrie;

namespace {

    /** Batch sizes stay well inside the default 8192-slot pool: a benchmark that exhausts the pool
     * would be measuring the exhaustion-recovery path instead of the scheduler. */
    constexpr u32 kBatchJobs = 1024;
    constexpr u32 kFanWidth = 256;
    constexpr u32 kChainLength = 256;
    constexpr u32 kScalingJobs = 512;

} // namespace

TEST_CASE("Job dispatch overhead", "[jobs]") {
    std::atomic<u64> sink{ 0 };

    // Submit-to-completion latency for a single job: the round trip a caller pays when it cannot
    // batch. Includes the wait-assist path, since the calling thread often runs the job itself.
    BENCHMARK("Run + Wait, one empty job") {
        const JobHandle handle = JobSystem::Run([&sink] { sink.fetch_add(1, std::memory_order_relaxed); });
        JobSystem::Wait(handle);
        return sink.load(std::memory_order_relaxed);
    };

    // Throughput instead of latency: the whole batch is in flight before anything is waited on, so
    // this is the number to divide by kBatchJobs for a per-job scheduling cost.
    BENCHMARK_ADVANCED("Run + Wait, 1024 empty jobs")(Catch::Benchmark::Chronometer meter) {
        std::vector<JobHandle> handles;
        handles.reserve(kBatchJobs);

        meter.measure([&handles, &sink] {
            handles.clear();

            for (u32 i = 0; i < kBatchJobs; ++i) {
                handles.push_back(JobSystem::Run([&sink] { sink.fetch_add(1, std::memory_order_relaxed); }));
            }

            for (const JobHandle &handle : handles) {
                JobSystem::Wait(handle);
            }

            return handles.size();
        });
    };
}

TEST_CASE("Dependency graph shapes", "[jobs]") {
    std::atomic<u64> sink{ 0 };

    // Fan-in: kFanWidth edges onto one join job. Against the equivalent row above, the difference
    // is what AddDependency costs — an edge allocation plus the release cascade at completion.
    BENCHMARK_ADVANCED("Fan-out 256 + join")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&sink] {
            const JobHandle join = JobSystem::Create([] {});

            for (u32 i = 0; i < kFanWidth; ++i) {
                const JobHandle child = JobSystem::Create([&sink] { sink.fetch_add(1, std::memory_order_relaxed); });
                JobSystem::AddDependency(join, child);
                JobSystem::Schedule(child);
            }

            JobSystem::Schedule(join);
            JobSystem::Wait(join);

            return sink.load(std::memory_order_relaxed);
        });
    };

    // The opposite extreme: zero available parallelism, every link waiting on the previous one, so
    // this measures the completion cascade end to end.
    BENCHMARK_ADVANCED("Chain of 256")(Catch::Benchmark::Chronometer meter) {
        std::vector<JobHandle> chain;
        chain.reserve(kChainLength);

        meter.measure([&chain, &sink] {
            chain.clear();

            for (u32 i = 0; i < kChainLength; ++i) {
                chain.push_back(JobSystem::Create([&sink] { sink.fetch_add(1, std::memory_order_relaxed); }));

                if (i > 0) {
                    JobSystem::AddDependency(chain[i], chain[i - 1]);
                }
            }

            // Tail first: nothing can run until the head is released.
            for (u32 i = kChainLength; i-- > 1;) {
                JobSystem::Schedule(chain[i]);
            }

            JobSystem::Schedule(chain[0]);
            JobSystem::Wait(chain[kChainLength - 1]);

            return chain.size();
        });
    };
}

TEST_CASE("Job throughput scales with worker count", "[jobs][scaling]") {
    // The headline measurement: one fixed workload, run against pools from synchronous up to one
    // worker per core. Ideal scaling halves the time at each doubling; where the curve flattens is
    // where per-job overhead starts to dominate the per-job work.
    std::atomic<u64> sink{ 0 };

    for (const u32 workers : Bench::ScalingWorkerCounts()) {
        const Bench::JobSystemScope scope(workers);

        BENCHMARK_ADVANCED("512 jobs of MEDIUM_WORK - " + Bench::WorkerLabel(workers))(Catch::Benchmark::Chronometer meter) {
            std::vector<JobHandle> handles;
            handles.reserve(kScalingJobs);

            meter.measure([&handles, &sink] {
                handles.clear();

                for (u32 i = 0; i < kScalingJobs; ++i) {
                    handles.push_back(JobSystem::Run([&sink, i] { sink.fetch_add(Bench::BusyWork(i, Bench::MEDIUM_WORK), std::memory_order_relaxed); }));
                }

                for (const JobHandle &handle : handles) {
                    JobSystem::Wait(handle);
                }

                return sink.load(std::memory_order_relaxed);
            });
        };
    }
}
