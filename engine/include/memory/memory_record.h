#pragma once

// NOTE: Kept free of `vlkypch.h` for the same reason as `untracked_allocator.h` - these types are used by the
// tracker's own bookkeeping, below the level where the engine's vocabulary header is appropriate.
#include "memory/allocators/untracked_allocator.h"
#include "memory/memory_tag.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Vulkyrie {

    /** @brief Maximum stack frames captured per allocation when `VE_MEMORY_CALLSTACKS` is on. Deep enough to get
     * past the allocator plumbing and into engine code, shallow enough that the per-allocation record stays small. */
    inline constexpr std::uint32_t VE_MEMORY_CALLSTACK_DEPTH = 16;

    /** @brief A container the tracker can fill without allocating through the path it is tracking. */
    template <typename T> using UntrackedVector = std::vector<T, UntrackedAllocator<T>>;

    /** @brief One live allocation, as recorded by the deep tier. */
    struct AllocationRecord {
    public:
        /** @brief The payload address handed to the caller. */
        void *Address = nullptr;

        /** @brief Payload size in bytes, excluding the tracking header. */
        std::size_t Size = 0;

        /** @brief Subsystem the allocation was attributed to. */
        MemoryTag Tag = MemoryTag::Untagged;

        /** @brief Hash of the allocating thread's id, for spotting cross-thread ownership. */
        std::uint32_t ThreadId = 0;

        /** @brief Nanoseconds since the tracker's first recorded allocation. Makes it possible to tell a leak that
         * has been outstanding since startup from one that appeared during a particular frame. */
        std::uint64_t TimestampNanos = 0;

        /** @brief Hash identifying the call site, or 0 when callstack capture is disabled. */
        std::uint64_t SiteHash = 0;
    };

    /** @brief Aggregated statistics for one allocation call site, identified by its captured callstack. */
    struct AllocationSite {
    public:
        /** @brief Hash identifying this site. */
        std::uint64_t Hash = 0;

        /** @brief Captured return addresses, outermost last. */
        std::array<void *, VE_MEMORY_CALLSTACK_DEPTH> Frames{};

        /** @brief Number of valid entries in `Frames`. */
        std::uint32_t FrameCount = 0;

        /** @brief Subsystem the site's first allocation was attributed to. */
        MemoryTag Tag = MemoryTag::Untagged;

        /** @brief Bytes currently outstanding from this site - the number that matters for a leak hunt. */
        std::int64_t LiveBytes = 0;

        /** @brief Allocations currently outstanding from this site. */
        std::int64_t LiveCount = 0;

        /** @brief Bytes ever allocated from this site - the number that matters for churn. */
        std::int64_t TotalBytes = 0;

        /** @brief Allocations ever made from this site. */
        std::int64_t TotalCount = 0;
    };

    /** @brief Totals for everything still outstanding in the deep table. */
    struct LeakSummary {
    public:
        /** @brief Number of allocations still live. */
        std::int64_t Count = 0;

        /** @brief Bytes still live. */
        std::int64_t Bytes = 0;

        /** @brief Per-subsystem live bytes, indexed by `MemoryTag`. */
        std::array<std::int64_t, MemoryTagCount> BytesByTag{};
    };

} // namespace Vulkyrie
