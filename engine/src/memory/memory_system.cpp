#include "memory/memory_system.h"

#include "memory/memory_tracker.h"

namespace Vulkyrie {

    void MemorySystem::Initialize() {
        VINFO("Memory subsystem initialized (per-subsystem cheap-tier tracking active).");
    }

    void MemorySystem::Shutdown() {
        MemoryTracker::ReportToLog();
    }

} // namespace Vulkyrie
