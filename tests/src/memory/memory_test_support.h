#pragma once

// Shared probes for the memory tests. Several build configurations deliberately compile parts of the memory
// subsystem out, and a test that depends on one of them should skip rather than fail.
#include <memory/memory_tracker.h>

#include <cstddef>

namespace Vulkyrie::MemoryTests {

    /** @brief Reports whether the replaced global `operator new` is actually installed.
     *
     * `VE_MEMORY_DISABLE_GLOBAL_NEW` compiles the replacements out so ASan/Valgrind builds do not fight them, and
     * the linker can in principle drop the translation unit that defines them (which the anchor in `memory_scope.h`
     * exists to prevent). Either way the per-subsystem counters and the deep table stop being populated, so this is
     * probed at runtime rather than assumed from a macro: it answers "did the mitigation hold" as well as "is the
     * feature compiled in".
     *
     * @returns True if a heap allocation moves the counters. */
    [[nodiscard]] inline bool GlobalNewOverrideActive() {
        // Cached: the answer cannot change within a process, and the probe itself allocates.
        static const bool active = [] {
            constexpr auto PROBE_TAG = MemoryTag::Untagged;
            constexpr std::size_t PROBE_SIZE = 4096;

            const i64 before = MemoryTracker::TotalAllocated(PROBE_TAG);

            VE_MEMORY_SCOPE(PROBE_TAG);
            auto *block = new std::byte[PROBE_SIZE];
            const bool moved = MemoryTracker::TotalAllocated(PROBE_TAG) > before;
            delete[] block;

            return moved;
        }();

        return active;
    }

} // namespace Vulkyrie::MemoryTests
