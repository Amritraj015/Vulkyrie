#pragma once

#include "vlkypch.h"
#include "memory/memory_record.h"
#include "memory/memory_tag.h"

#include <atomic>

namespace Vulkyrie {

    /** @brief Master switch for the deep tier: a per-allocation table recording address, size, tag, thread,
     * timestamp and (optionally) call site.
     *
     * On by default in Debug, off in Release. It costs a hashed insert and erase per allocation, which is fine for
     * a debug session and not for a shipping frame - the cheap counters stay exact either way, so turning this off
     * loses leak detection and hot spots but never loses attribution. */
#if !defined(VE_MEMORY_DEEP_TRACKING)
#if defined(VULKYRIE_DEBUG)
#define VE_MEMORY_DEEP_TRACKING 1
#else
#define VE_MEMORY_DEEP_TRACKING 0
#endif
#endif

    /** @brief Assumed cache-line size, used to keep one subsystem's counters off another's line. Duplicated from
     * `core/jobs/job.h`'s `VE_CACHE_LINE_SIZE` rather than included: the memory subsystem sits below the job
     * system and must not depend on it. */
    inline constexpr std::size_t VE_MEMORY_CACHE_LINE_SIZE = 64;

    /** @brief Always-on ("cheap tier") per-subsystem counters. Relaxed atomics keep the per-alloc
     * cost to a few nanoseconds so tracking can stay enabled in release builds.
     *
     * Cache-line aligned so two subsystems being updated from two threads cannot false-share. Without it the
     * fourteen tags would pack into ~7 lines and a physics worker allocating would invalidate the line a render
     * worker is writing to, which is the worst case for a counter that is meant to be nearly free. */
    struct alignas(VE_MEMORY_CACHE_LINE_SIZE) SubsystemCounters {
        std::atomic<i64> CurrentBytes{ 0 };    ///< Bytes currently live for this subsystem.
        std::atomic<i64> LiveAllocations{ 0 }; ///< Number of live allocations for this subsystem.
        std::atomic<i64> TotalAllocated{ 0 };  ///< Cumulative bytes ever allocated.
        std::atomic<i64> TotalFreed{ 0 };      ///< Cumulative bytes ever freed.
        std::atomic<i64> PeakBytes{ 0 };       ///< High-water mark of `currentBytes`.

        std::atomic<i64> PoolReserved{ 0 }; ///< Bytes reserved by allocator-toolkit pools but not yet served.
        std::atomic<i64> PoolUsed{ 0 };     ///< Bytes currently handed out from within those pools.
        std::atomic<i64> PoolPeakUsed{ 0 }; ///< High-water mark of `PoolUsed`.
    };

    static_assert(sizeof(SubsystemCounters) == VE_MEMORY_CACHE_LINE_SIZE, "SubsystemCounters is expected to occupy exactly one cache line.");

    namespace detail {

        /** @brief The per-subsystem counter array.
         *
         * Lives in the header, as an `inline constinit` variable, purely so the update functions below can be
         * inlined into their callers. They sit on the hot path of every heap allocation and every toolkit
         * allocation, where a non-inlined cross-translation-unit call costs more than the atomic it performs.
         *
         * `constinit` keeps the guarantee that motivated putting these in static storage in the first place: the
         * counters are valid before any dynamic initialization runs, so allocations made by other globals'
         * constructors - before `main` - are still counted.
         *
         * Not part of the public API despite being visible; go through `MemoryTracker`. */
        inline constinit std::array<SubsystemCounters, MemoryTagCount> gSubsystemCounters{};

        /** @brief Returns the counters for a subsystem. */
        [[nodiscard]] VE_INLINE SubsystemCounters &CountersFor(MemoryTag tag) {
            return gSubsystemCounters[static_cast<std::size_t>(tag)];
        }

    } // namespace detail

    /** @brief The memory tracker: per-`MemoryTag` atomic counters, a reserved-pool accounting channel, and an
     * optional per-allocation table for leak detection and hot spots.
     *
     * All state lives in constant-initialized static storage (not a lazily-constructed singleton) so
     * the counters are valid before any dynamic initialization - allocations made by other globals'
     * constructors, before `main`, are still counted. The class is a pure static utility.
     *
     * The tracker's own bookkeeping never routes through the replaced `operator new`: the deep table's storage
     * comes from `UntrackedAllocator`, which is what stops recording an allocation from being one.
     */
    class MemoryTracker final {
    public:
        MemoryTracker() = delete;

        // --- Cheap tier ---------------------------------------------------------------------------------

        /** @brief Records an allocation against a subsystem. Defined here rather than out of line because it runs
         * on every single heap allocation in the process - the call overhead would exceed the work.
         * @param tag The subsystem the allocation is attributed to.
         * @param size The payload size in bytes.
         */
        static VE_INLINE void OnAllocation(MemoryTag tag, i64 size) {
            SubsystemCounters &counters = detail::CountersFor(tag);

            counters.TotalAllocated.fetch_add(size, std::memory_order_relaxed);
            counters.LiveAllocations.fetch_add(1, std::memory_order_relaxed);

            const i64 current = counters.CurrentBytes.fetch_add(size, std::memory_order_relaxed) + size;

            // Plain load before the read-modify-write: once a subsystem settles at its high-water mark the peak
            // stops moving, so the common case is a cheap load and a not-taken branch instead of a second RMW.
            if (current > counters.PeakBytes.load(std::memory_order_relaxed)) {
                counters.PeakBytes.fetch_max(current, std::memory_order_relaxed);
            }
        }

        /** @brief Records a free against a subsystem.
         * @param tag The subsystem the original allocation was attributed to.
         * @param size The payload size in bytes (as recorded at allocation time).
         */
        static VE_INLINE void OnFree(MemoryTag tag, i64 size) {
            SubsystemCounters &counters = detail::CountersFor(tag);

            counters.TotalFreed.fetch_add(size, std::memory_order_relaxed);
            counters.LiveAllocations.fetch_sub(1, std::memory_order_relaxed);
            counters.CurrentBytes.fetch_sub(size, std::memory_order_relaxed);
        }

        /** @brief Logs a per-subsystem summary table (current/peak/live/total) via `VINFO`. Iterates
         * tags in enum order so the report is deterministic across runs. */
        static void ReportToLog();

        /** @brief Returns the bytes currently live for a subsystem. */
        [[nodiscard]] static i64 CurrentBytes(MemoryTag tag);

        /** @brief Returns the high-water mark of live bytes for a subsystem. */
        [[nodiscard]] static i64 PeakBytes(MemoryTag tag);

        /** @brief Returns the number of live allocations for a subsystem. */
        [[nodiscard]] static i64 LiveAllocations(MemoryTag tag);

        /** @brief Returns the cumulative bytes ever allocated for a subsystem. */
        [[nodiscard]] static i64 TotalAllocated(MemoryTag tag);

        /** @brief Returns the cumulative bytes ever freed for a subsystem. */
        [[nodiscard]] static i64 TotalFreed(MemoryTag tag);

        // --- Reserved pools -----------------------------------------------------------------------------
        //
        // Allocator-toolkit pools take one large block up front and sub-allocate from it. Counting that block as
        // ordinary heap would make a subsystem look like it is holding memory it has not handed out, and counting
        // each sub-allocation on top would double-count the same bytes. Pools therefore take their backing memory
        // untracked and report it through this channel instead, so "heap" and "reserved by a pool" stay separable.

        /** @brief Records a pool acquiring backing storage. Cold - once per chunk - but kept alongside the others.
         * @param tag The subsystem the pool belongs to.
         * @param bytes Size of the acquired block. */
        static VE_INLINE void OnPoolReserve(MemoryTag tag, i64 bytes) {
            detail::CountersFor(tag).PoolReserved.fetch_add(bytes, std::memory_order_relaxed);
        }

        /** @brief Records a pool returning backing storage.
         * @param tag The subsystem the pool belongs to.
         * @param bytes Size of the released block. */
        static VE_INLINE void OnPoolRelease(MemoryTag tag, i64 bytes) {
            detail::CountersFor(tag).PoolReserved.fetch_sub(bytes, std::memory_order_relaxed);
        }

        /** @brief Records bytes handed out from within a pool. Runs on every toolkit allocation - a frame graph
         * rebuild alone calls it well over a hundred times - so it is defined here to inline into the allocator.
         * @param tag The subsystem the pool belongs to.
         * @param bytes Bytes served to the caller. */
        static VE_INLINE void OnPoolAllocate(MemoryTag tag, i64 bytes) {
            SubsystemCounters &counters = detail::CountersFor(tag);
            const i64 used = counters.PoolUsed.fetch_add(bytes, std::memory_order_relaxed) + bytes;

            // See OnAllocation: a plain load skips the second read-modify-write once the peak has settled.
            if (used > counters.PoolPeakUsed.load(std::memory_order_relaxed)) {
                counters.PoolPeakUsed.fetch_max(used, std::memory_order_relaxed);
            }
        }

        /** @brief Records bytes returned to a pool.
         * @param tag The subsystem the pool belongs to.
         * @param bytes Bytes reclaimed. */
        static VE_INLINE void OnPoolFree(MemoryTag tag, i64 bytes) {
            detail::CountersFor(tag).PoolUsed.fetch_sub(bytes, std::memory_order_relaxed);
        }

        /** @brief Returns the bytes a subsystem's pools hold as backing storage. */
        [[nodiscard]] static i64 PoolReservedBytes(MemoryTag tag);

        /** @brief Returns the bytes currently served from within a subsystem's pools. */
        [[nodiscard]] static i64 PoolUsedBytes(MemoryTag tag);

        /** @brief Returns the high-water mark of bytes served from a subsystem's pools. */
        [[nodiscard]] static i64 PoolPeakUsedBytes(MemoryTag tag);

        // --- Deep tier ----------------------------------------------------------------------------------

        /** @brief Reports whether the deep table is compiled in. Defined out of line so every caller agrees,
         * whatever each translation unit happened to define `VE_MEMORY_DEEP_TRACKING` to. */
        [[nodiscard]] constexpr static VE_INLINE bool DeepTrackingEnabled() {
#if VE_MEMORY_DEEP_TRACKING
            return true;
#else
            return false;
#endif
        }

        /** @brief Records an allocation in the deep table. No-op when deep tracking is off.
         * @param address The payload pointer handed to the caller.
         * @param tag The subsystem the allocation is attributed to.
         * @param size The payload size in bytes. */
        static void OnAllocationDeep(void *address, MemoryTag tag, i64 size);

        /** @brief Removes an allocation from the deep table. No-op when deep tracking is off, and tolerant of
         * pointers it never saw - anything allocated before the table first came up.
         * @param address The payload pointer being freed. */
        static void OnFreeDeep(void *address);

        /** @brief Returns every allocation currently outstanding. Ordering is unspecified; sort by whatever the
         * caller cares about. Uses untracked storage, so collecting a report does not perturb the numbers. */
        [[nodiscard]] static UntrackedVector<AllocationRecord> LiveAllocationRecords();

        /** @brief Returns totals for everything currently outstanding, without materialising the records. */
        [[nodiscard]] static LeakSummary CollectLeakSummary();

        /** @brief Returns the call sites with the most bytes still outstanding, largest first.
         *
         * Requires `VE_MEMORY_CALLSTACKS`; without it every allocation shares one null site and the result is
         * empty, because grouping by "unknown" would only restate the per-tag counters.
         *
         * @param limit Maximum sites to return. */
        [[nodiscard]] static UntrackedVector<AllocationSite> TopAllocationSites(std::size_t limit);

        /** @brief Logs everything still outstanding: totals, a per-subsystem breakdown, and the worst call sites.
         *
         * "Outstanding" is not the same as "leaked" when called from `MemorySystem::Shutdown`, which runs before
         * static destruction - the logger, the profiler and every other global still hold live allocations at that
         * point. It is a leak report when called around a scope that is expected to balance.
         *
         * @param maxSites Maximum call sites to list. */
        static void ReportOutstandingToLog(std::size_t maxSites = 16);
    };

} // namespace Vulkyrie
