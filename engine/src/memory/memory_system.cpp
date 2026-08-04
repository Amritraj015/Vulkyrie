#include "memory/memory_system.h"

#include "memory/callstack.h"
#include "memory/memory_tracker.h"

namespace Vulkyrie {

    void MemorySystem::Initialize() {
        if constexpr (MemoryTracker::DeepTrackingEnabled()) {
            VINFO("Memory subsystem initialized (cheap-tier counters + deep per-allocation tracking{}).",
                  CallstacksEnabled() ? " with callstacks" : ", callstacks off");
        } else {
            VINFO("Memory subsystem initialized (per-subsystem cheap-tier tracking active).");
        }
    }

    void MemorySystem::Shutdown() {
        MemoryTracker::ReportToLog();

        // TODO: Runs before static destruction, so what is still outstanding here includes every global that has not
        // been torn down yet. Informational rather than a leak gate; see ReportOutstandingToLog.
        if constexpr (MemoryTracker::DeepTrackingEnabled()) {
            MemoryTracker::ReportOutstandingToLog();
        }
    }

} // namespace Vulkyrie
