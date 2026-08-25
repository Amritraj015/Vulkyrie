#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "memory/memory_tag.h"
#include "memory/memory_tracker.h"

#include <cstddef>
#include <cstdlib>

namespace Vulkyrie::detail {

    /** @brief Which of the two raw heap paths took a block. Checked on the way back so a pointer freed through
     * the wrong one is caught rather than silently wrecking the other's counters. Debug builds only. */
    enum class BlockOwner : u32 { GlobalNew, HeapAllocator };

    /** @brief Written immediately below every payload the engine's two raw heap paths hand out - the replaced
     * global `operator new` and `HeapAllocator`.
     *
     * Both are given nothing but a pointer on the way back and need the same three answers from it: where the
     * heap's own pointer is, how many bytes the caller asked for, and which subsystem to credit the free to.
     *
     * `alignas` rounds the whole thing to a multiple of the heap's alignment, which is what lets an allocation
     * asking for no more than fundamental alignment reserve no padding at all: `std::malloc` already returns a
     * suitably aligned pointer and stepping over a whole number of alignment units keeps it that way. */
    struct alignas(std::max_align_t) AllocationBlockHeader {
        /** @brief Payload bytes the caller asked for, rather than what was reserved. */
        size_t Size = 0;

        /** @brief Distance from the heap's pointer to the payload. Recorded as an offset rather than as the
         * pointer itself so the header still fits in a single alignment unit. */
        u32 BaseOffset = 0;

        /** @brief Subsystem the bytes are attributed to. */
        MemoryTag Tag = MemoryTag::Untagged;

#if defined(VE_DEBUG)
        /** @brief Catches a foreign or corrupted pointer reaching the free path. */
        u32 Magic = 0;

        /** @brief The path that took the block, and the only one allowed to give it back. */
        BlockOwner Owner = BlockOwner::GlobalNew;
#endif
    };

    static_assert(sizeof(AllocationBlockHeader) % alignof(std::max_align_t) == 0, "AllocationBlockHeader must be a whole number of heap alignment units.");

#if defined(VE_DEBUG)
    inline constexpr u32 kAllocationBlockMagic = 0x564C4B59U; // "VLKY"
#endif

    /** @brief Returns the header belonging to a payload pointer. */
    [[nodiscard]] VE_INLINE AllocationBlockHeader *HeaderOf(void *payload) {
        return reinterpret_cast<AllocationBlockHeader *>(static_cast<std::byte *>(payload) - sizeof(AllocationBlockHeader));
    }

    [[nodiscard]] VE_INLINE const AllocationBlockHeader *HeaderOf(const void *payload) {
        return reinterpret_cast<const AllocationBlockHeader *>(static_cast<const std::byte *>(payload) - sizeof(AllocationBlockHeader));
    }

    /** @brief Takes a headered block from the process heap and reports it to the tracker.
     * @param size Payload bytes; recorded and reported exactly as given.
     * @param alignment Required payload alignment; must be a power of two.
     * @param tag Subsystem the bytes are attributed to.
     * @param owner The path taking the block, which is the only one that may release it.
     * @returns The payload pointer, or nullptr when the heap cannot satisfy the request. */
    [[nodiscard]] VE_INLINE void *AllocateBlock(size_t size, size_t alignment, MemoryTag tag, [[maybe_unused]] BlockOwner owner) {
        VASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Allocation alignment must be a power of two.");

        // The header sits directly below the payload, so the payload can be no less aligned than the header is.
        const size_t effectiveAlignment = std::max(alignment, alignof(AllocationBlockHeader));
        const size_t padding = effectiveAlignment - alignof(AllocationBlockHeader);

        if (size > SIZE_MAX - padding - sizeof(AllocationBlockHeader)) {
            return nullptr;
        }

        // Untracked on purpose: reported below against `tag`, which is the whole point of the header.
        void *base = std::malloc(size + padding + sizeof(AllocationBlockHeader));

        if (nullptr == base) {
            return nullptr;
        }

        const auto belowPayload = reinterpret_cast<uintptr_t>(base) + sizeof(AllocationBlockHeader);
        auto *payload = reinterpret_cast<std::byte *>((belowPayload + effectiveAlignment - 1) & ~(effectiveAlignment - 1));

        const auto offset = static_cast<size_t>(payload - static_cast<std::byte *>(base));
        VASSERT(offset <= UINT32_MAX, "Allocation alignment is too large for the block header to record.");

        AllocationBlockHeader header{ .Size = size, .BaseOffset = static_cast<u32>(offset), .Tag = tag };
#if defined(VE_DEBUG)
        header.Magic = kAllocationBlockMagic;
        header.Owner = owner;
#endif
        std::construct_at(HeaderOf(payload), header);

        MemoryTracker::OnAllocation(tag, static_cast<i64>(size));

#if VE_MEMORY_DEEP_TRACKING
        MemoryTracker::OnAllocationDeep(payload, tag, static_cast<i64>(size));
#endif

        return payload;
    }

    /** @brief Returns a block to the process heap and reports the free to the tracker.
     * @param payload A pointer previously returned by `AllocateBlock`; must not be nullptr.
     * @param owner The path releasing the block, which must be the one that took it.
     * @returns The payload size that was recorded for it, which callers keeping their own counters need. */
    VE_INLINE size_t ReleaseBlock(void *payload, [[maybe_unused]] BlockOwner owner) noexcept {
        AllocationBlockHeader *header = HeaderOf(payload);

#if defined(VE_DEBUG)
        VASSERT(header->Magic == kAllocationBlockMagic, "Freed a pointer the engine did not allocate, or one whose header was overwritten.");
        VASSERT(header->Owner == owner, "Freed a pointer through the wrong path: global delete and HeapAllocator::Free are not interchangeable.");
#endif

        const size_t size = header->Size;
        const MemoryTag tag = header->Tag;
        void *base = static_cast<std::byte *>(payload) - header->BaseOffset;

#if VE_MEMORY_DEEP_TRACKING
        // Before the free, while the pointer is still the one the table was keyed on.
        MemoryTracker::OnFreeDeep(payload);
#endif

        std::destroy_at(header);
        std::free(base);

        MemoryTracker::OnFree(tag, static_cast<i64>(size));

        return size;
    }

    /** @brief Returns the payload size recorded for a block.
     * @param payload A pointer previously returned by `AllocateBlock`; must not be nullptr. */
    [[nodiscard]] VE_INLINE size_t BlockSize(const void *payload) {
#if defined(VE_DEBUG)
        VASSERT(HeaderOf(payload)->Magic == kAllocationBlockMagic, "Asked for the size of a pointer the engine did not allocate.");
#endif

        return HeaderOf(payload)->Size;
    }

} // namespace Vulkyrie::detail
