#include "memory/memory_tracker.h"

namespace Vulkyrie {

    namespace {

        // Constant-initialized static storage: zeroed at load time, before any dynamic init runs, so
        // allocations from other globals' constructors are counted. Reached only through the (uninlined)
        // MemoryTracker methods below, never through the overridden operator new — no recursion.
        constinit std::array<SubsystemCounters, MemoryTagCount> gCounters{};

        [[nodiscard]] SubsystemCounters &CountersFor(MemoryTag tag) {
            return gCounters[static_cast<std::size_t>(tag)];
        }

    } // namespace

    void MemoryTracker::OnAllocation(MemoryTag tag, i64 size) {
        SubsystemCounters &counters = CountersFor(tag);

        counters.TotalAllocated.fetch_add(size, std::memory_order_relaxed);
        counters.LiveAllocations.fetch_add(1, std::memory_order_relaxed);

        const i64 current = counters.CurrentBytes.fetch_add(size, std::memory_order_relaxed) + size;

        // Raise the high-water mark with a relaxed CAS loop.
        i64 previousPeak = counters.PeakBytes.load(std::memory_order_relaxed);
        while (current > previousPeak && !counters.PeakBytes.compare_exchange_weak(previousPeak, current, std::memory_order_relaxed)) {
            // `previousPeak` is refreshed by compare_exchange_weak on failure; retry.
        }
    }

    void MemoryTracker::OnFree(MemoryTag tag, i64 size) {
        SubsystemCounters &counters = CountersFor(tag);

        counters.TotalFreed.fetch_add(size, std::memory_order_relaxed);
        counters.LiveAllocations.fetch_sub(1, std::memory_order_relaxed);
        counters.CurrentBytes.fetch_sub(size, std::memory_order_relaxed);
    }

    i64 MemoryTracker::CurrentBytes(MemoryTag tag) {
        return CountersFor(tag).CurrentBytes.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::PeakBytes(MemoryTag tag) {
        return CountersFor(tag).PeakBytes.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::LiveAllocations(MemoryTag tag) {
        return CountersFor(tag).LiveAllocations.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::TotalAllocated(MemoryTag tag) {
        return CountersFor(tag).TotalAllocated.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::TotalFreed(MemoryTag tag) {
        return CountersFor(tag).TotalFreed.load(std::memory_order_relaxed);
    }

    void MemoryTracker::ReportToLog() {
        // Guard the whole body so release builds (where VINFO compiles out) don't warn on unused locals.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_INFO_LEVEL_LOG
        VINFO("================= Memory report (per subsystem) =================");
        VINFO("{:<12}{:>14}{:>14}{:>10}{:>16}{:>16}", "Subsystem", "Current(B)", "Peak(B)", "Live", "TotalAlloc(B)", "TotalFreed(B)");

        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            const auto tag = static_cast<MemoryTag>(index);
            VINFO("{:<12}{:>14}{:>14}{:>10}{:>16}{:>16}",
                  MemoryTagName(tag),
                  CurrentBytes(tag),
                  PeakBytes(tag),
                  LiveAllocations(tag),
                  TotalAllocated(tag),
                  TotalFreed(tag));
        }
        VINFO("================================================================");
#endif
    }

} // namespace Vulkyrie
