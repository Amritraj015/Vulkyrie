#pragma once

#include "vlkypch.h"
#include "memory/memory_tag.h"

#include <atomic>

namespace Vulkyrie {

    /** @brief Always-on ("cheap tier") per-subsystem counters. Relaxed atomics keep the per-alloc
     * cost to a few nanoseconds so tracking can stay enabled in release builds. */
    struct SubsystemCounters {
        std::atomic<i64> CurrentBytes{ 0 };    ///< Bytes currently live for this subsystem.
        std::atomic<i64> LiveAllocations{ 0 }; ///< Number of live allocations for this subsystem.
        std::atomic<i64> TotalAllocated{ 0 };  ///< Cumulative bytes ever allocated.
        std::atomic<i64> TotalFreed{ 0 };      ///< Cumulative bytes ever freed.
        std::atomic<i64> PeakBytes{ 0 };       ///< High-water mark of `currentBytes`.
    };

    /** @brief The cheap-tier memory tracker: a set of per-`MemoryTag` atomic counters plus reporting.
     *
     * All state lives in constant-initialized static storage (not a lazily-constructed singleton) so
     * the counters are valid before any dynamic initialization — allocations made by other globals'
     * constructors, before `main`, are still counted. The class is a pure static utility.
     */
    class MemoryTracker final {
    public:
        MemoryTracker() = delete;

        /** @brief Records an allocation against a subsystem.
         * @param tag The subsystem the allocation is attributed to.
         * @param size The payload size in bytes.
         */
        static void OnAllocation(MemoryTag tag, i64 size);

        /** @brief Records a free against a subsystem.
         * @param tag The subsystem the original allocation was attributed to.
         * @param size The payload size in bytes (as recorded at allocation time).
         */
        static void OnFree(MemoryTag tag, i64 size);

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
    };

} // namespace Vulkyrie
