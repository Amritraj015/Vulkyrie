#pragma once

// Scaffolding shared by every benchmark in this binary, regardless of which engine subsystem it
// measures: a workload primitive the optimizer cannot delete, a scoped job-system reconfiguration
// for scaling sweeps, and the worker-count sweep itself. Anything a second subsystem would also
// want belongs here rather than in one benchmark file.
#include "core/jobs/job_system.h"

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

namespace Vulkyrie::Bench {

    /** @brief Worker count meaning "run the workload on the calling thread with no worker pool" —
     * the serial baseline every scaling number is measured against. Passed to `JobSystemScope`. */
    inline constexpr u32 SYNCHRONOUS = 0;

    /** @brief Per-item workload sizes, in rounds of `BusyWork`. Named rather than spelled out at
     * each call site so numbers from different subsystems are comparable; the serial baseline in
     * each benchmark reports what a round actually costs on the machine at hand. */
    inline constexpr u32 TINY_WORK = 8;
    inline constexpr u32 SMALL_WORK = 64;
    inline constexpr u32 MEDIUM_WORK = 512;

    /** @brief A deterministic unit of CPU work: `rounds` iterations of an LCG step mixed with a
     * shift. Each iteration depends on the previous one, so it cannot be vectorized, unrolled into
     * nothing, or hoisted out of a loop — and because the result is returned, a caller that feeds
     * it back into the benchmark's return value keeps the whole thing from being optimized away.
     * @param seed Starting value; pass the loop index so different items do different work.
     * @param rounds How much work to do (see `TINY_WORK` and friends).
     * @returns The mixed value; the caller must consume it.
     */
    [[nodiscard]] inline u64 BusyWork(u64 seed, u32 rounds) {
        u64 value = seed;

        for (u32 i = 0; i < rounds; ++i) {
            value = (value * 6364136223846793005ULL) + 1442695040888963407ULL;
            value ^= value >> 33U;
        }

        return value;
    }

    /** @brief Rebuilds the job system with a chosen worker count for the lifetime of the scope and
     * restores the run listener's configuration on destruction. Declare one before a `BENCHMARK`
     * (or around a sweep) to measure a specific pool size.
     *
     * Restoration is RAII rather than a trailing call because a benchmark that throws would
     * otherwise leave every later benchmark measuring the wrong pool.
     */
    class JobSystemScope final {
    public:
        /** @brief Brings the job system up with exactly `workerCount` workers.
         * @param workerCount Workers to spawn, or `SYNCHRONOUS` for no pool at all. (`Initialize`
         * cannot express "zero workers" — 0 means "choose automatically" — so the synchronous case
         * leaves the system down and lets the first `Create` lazily bootstrap the inline instance.)
         * @param pinToCores Whether to pin workers to cores. Off by default, matching the listener:
         * pinning steadies numbers on an idle machine and distorts them on a busy one.
         */
        explicit JobSystemScope(u32 workerCount, bool pinToCores = false) {
            JobSystem::Shutdown();

            if (SYNCHRONOUS == workerCount) {
                return;
            }

            JobSystemConfig config{};
            config.WorkerCount = workerCount;
            config.PinToCores = pinToCores;
            JobSystem::Initialize(config);
        }

        /** @brief Brings the job system up with a fully specified configuration, for benchmarks
         * that need to move pool sizes rather than the worker count.
         * @param config The configuration to run under; `WorkerCount` 0 keeps its usual "choose
         * automatically" meaning here, unlike the worker-count constructor.
         */
        explicit JobSystemScope(const JobSystemConfig &config) {
            JobSystem::Shutdown();
            JobSystem::Initialize(config);
        }

        VE_DELETE_MOVE_AND_COPY(JobSystemScope);

        ~JobSystemScope() {
            JobSystem::Shutdown();

            JobSystemConfig config{};
            config.PinToCores = false;
            JobSystem::Initialize(config);
        }
    };

    /** @brief The worker counts a scaling benchmark sweeps: synchronous, then 1, 2, 4, 8, ... up to
     * the machine's core count, with the core count itself always included. Duplicates are dropped
     * so a small machine does not measure the same pool twice.
     * @returns Worker counts in ascending order, starting with `SYNCHRONOUS`.
     */
    [[nodiscard]] inline std::vector<u32> ScalingWorkerCounts() {
        const u32 cores = std::max(std::thread::hardware_concurrency(), 2U);

        std::vector<u32> counts{ SYNCHRONOUS };

        for (u32 workers = 1; workers < cores; workers *= 2U) {
            counts.push_back(workers);
        }

        // The interesting configuration is "the whole machine": one worker per core minus the main
        // thread, which participates as worker 0.
        counts.push_back(cores - 1U);

        std::sort(counts.begin(), counts.end());
        counts.erase(std::unique(counts.begin(), counts.end()), counts.end());

        return counts;
    }

    /** @brief Renders a worker count for a benchmark name.
     * @param workerCount The count to describe.
     * @returns `"synchronous"` for `SYNCHRONOUS`, otherwise e.g. `"4 workers"`.
     */
    [[nodiscard]] inline std::string WorkerLabel(u32 workerCount) {
        if (SYNCHRONOUS == workerCount) {
            return "synchronous";
        }

        return std::to_string(workerCount) + (1U == workerCount ? " worker" : " workers");
    }

} // namespace Vulkyrie::Bench
