#pragma once

#include "memory/memory_record.h"

#include <string>

namespace Vulkyrie {

    /** @brief Master switch for per-allocation callstack capture. Off by default even in Debug: capturing a stack
     * costs hundreds of nanoseconds to microseconds per allocation, which is a different order of overhead from the
     * deep table itself. Turn it on when hunting a specific leak, not as a standing configuration. */
#define VE_MEMORY_CALLSTACKS 0

#if !defined(VE_MEMORY_CALLSTACKS)
#define VE_MEMORY_CALLSTACKS 0
#endif

    /** @brief A captured stack, small enough to live inside an allocation record. */
    struct Callstack {
    public:
        /** @brief Captured return addresses, innermost first. */
        std::array<void *, VE_MEMORY_CALLSTACK_DEPTH> Frames{};

        /** @brief Number of valid entries in `Frames`. */
        std::uint32_t FrameCount = 0;

        /** @brief Order-sensitive hash of the frames, used to group allocations by call site. Zero when nothing
         * was captured. */
        std::uint64_t Hash = 0;
    };

    /** @brief Captures the calling thread's stack, skipping the innermost frames.
     *
     * Returns an empty callstack when `VE_MEMORY_CALLSTACKS` is off, and is safe to call from inside the
     * allocation path: the platform capture routines are invoked under a re-entrancy guard so any allocation they
     * make themselves cannot recurse back into the tracker.
     *
     * @param skipFrames Innermost frames to drop, so the allocator plumbing does not dominate every stack.
     * @returns The captured stack, or an empty one if capture is disabled or unavailable. */
    [[nodiscard]] Callstack CaptureCallstack(std::uint32_t skipFrames = 0);

    /** @brief Resolves a captured stack to human-readable frames.
     *
     * Symbol quality depends on the build: on Linux this needs `-rdynamic` (or a non-stripped binary) to name
     * anything beyond the module, and inlined frames are never recoverable. Addresses are always printed, so an
     * unresolved stack is still usable with `addr2line`.
     *
     * @param callstack The stack to resolve.
     * @returns One newline-separated line per frame; empty when the stack has no frames. */
    [[nodiscard]] std::string FormatCallstack(const Callstack &callstack);

    /** @brief Reports whether callstack capture is compiled in, so callers can explain an empty result rather than
     * silently reporting nothing. */
    [[nodiscard]] constexpr bool CallstacksEnabled() {
        return VE_MEMORY_CALLSTACKS != 0;
    }

} // namespace Vulkyrie
