#include "memory/memory_scope.h"
#include "memory/memory_tracker.h"

#include "core/asserts.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

// ----------------------------------------------------------------------------------------------
// Linker anchor.
//
// Defined UNCONDITIONALLY (even when the overrides below are compiled out) so the reference in
// `memory_scope.h` always resolves. This is what forces the linker to keep this translation unit —
// and therefore the operator new/delete replacements — when the `engine` static library is linked
// into an executable. See memory_scope.h for the matching declaration + anchor variable.
// ----------------------------------------------------------------------------------------------
namespace Vulkyrie::detail {
    int ForceLinkGlobalNewDelete() {
        return 0;
    }
} // namespace Vulkyrie::detail

#if !defined(VE_MEMORY_DISABLE_GLOBAL_NEW)

// Brings MemoryTag / MemoryTracker / CurrentMemoryTag into scope, and — importantly — Logger /
// LogLevel, which the VASSERT macro references unqualified (this file's code sits at global scope,
// not inside namespace Vulkyrie).
using namespace Vulkyrie;

namespace {

    /** @brief Sentinel written into every allocation header ("VLKY"); validated on free. */
    constexpr std::uint32_t kAllocationMagic = 0x564C4B59U;

    /** @brief Small aligned prefix stored just before every returned payload. Storing `base`
     * explicitly makes free correct regardless of the alignment padding we inserted. */
    struct AllocationHeader {
        void *Base;          ///< The original std::malloc pointer to hand back to std::free.
        std::size_t Size;    ///< The payload size requested by the caller.
        MemoryTag Tag;       ///< The subsystem the allocation was attributed to.
        std::uint32_t Magic; ///< Corruption/foreign-pointer guard.
    };

    static_assert(std::is_trivially_copyable_v<AllocationHeader>);
    static_assert(alignof(AllocationHeader) <= alignof(std::max_align_t));

    [[nodiscard]] std::uintptr_t AlignUp(std::uintptr_t value, std::size_t alignment) {
        const auto mask = static_cast<std::uintptr_t>(alignment) - 1U;
        return (value + mask) & ~mask;
    }

    /** @brief Allocates `size` payload bytes aligned to `alignment`, prefixed with a tracking header.
     * @returns The payload pointer, or nullptr on failure (including size overflow). */
    [[nodiscard]] void *TrackedAlloc(std::size_t size, std::size_t alignment) {
        constexpr std::size_t headerSize = sizeof(AllocationHeader);

        // Guard against size_t overflow of the padded total.
        if (size > SIZE_MAX - headerSize - alignment) {
            return nullptr;
        }

        const std::size_t totalSize = size + headerSize + alignment;
        void *base = std::malloc(totalSize);
        if (nullptr == base) {
            return nullptr;
        }

        const auto baseAddress = reinterpret_cast<std::uintptr_t>(base);
        const std::uintptr_t payloadAddress = AlignUp(baseAddress + headerSize, alignment);

        const AllocationHeader header{ base, size, CurrentMemoryTag(), kAllocationMagic };
        std::memcpy(reinterpret_cast<void *>(payloadAddress - headerSize), &header, headerSize);

        void *payload = reinterpret_cast<void *>(payloadAddress);

        MemoryTracker::OnAllocation(header.Tag, static_cast<i64>(size));

        // The deep table is a debug-tier addition on top of the counters above, never a replacement: attribution
        // stays exact from the header alone when this is compiled out.
#if VE_MEMORY_DEEP_TRACKING
        MemoryTracker::OnAllocationDeep(payload, header.Tag, static_cast<i64>(size));
#endif

        return payload;
    }

    /** @brief Frees a pointer produced by TrackedAlloc and updates the tracker. */
    void TrackedFree(void *pointer) noexcept {
        if (nullptr == pointer) {
            return;
        }

        const auto payloadAddress = reinterpret_cast<std::uintptr_t>(pointer);
        AllocationHeader header{};
        std::memcpy(&header, reinterpret_cast<const void *>(payloadAddress - sizeof(AllocationHeader)), sizeof(AllocationHeader));

        VASSERT(header.Magic == kAllocationMagic, "operator delete: corrupted or foreign allocation header");

#if VE_MEMORY_DEEP_TRACKING
        // Before the free, while the pointer is still the one the table was keyed on.
        MemoryTracker::OnFreeDeep(pointer);
#endif

        MemoryTracker::OnFree(header.Tag, static_cast<i64>(header.Size));
        std::free(header.Base);
    }

    /** @brief Throwing allocation with the standard new_handler retry loop. */
    [[nodiscard]] void *AllocateThrowing(std::size_t size, std::size_t alignment) {
        for (;;) {
            if (void *pointer = TrackedAlloc(size, alignment)) {
                return pointer;
            }

            const std::new_handler handler = std::get_new_handler();
            if (nullptr == handler) {
                throw std::bad_alloc();
            }
            handler();
        }
    }

    /** @brief Non-throwing allocation; returns nullptr on failure. */
    [[nodiscard]] void *AllocateNoThrow(std::size_t size, std::size_t alignment) noexcept {
        for (;;) {
            if (void *pointer = TrackedAlloc(size, alignment)) {
                return pointer;
            }

            const std::new_handler handler = std::get_new_handler();
            if (nullptr == handler) {
                return nullptr;
            }

            // A new_handler is permitted to throw std::bad_alloc; treat that as failure here.
            try {
                handler();
            } catch (...) {
                return nullptr;
            }
        }
    }

    constexpr std::size_t kDefaultAlignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

} // namespace

// ----------------------------------------------------------------------------------------------
// Replacement operator new (throwing / nothrow, single / array, default / over-aligned).
// ----------------------------------------------------------------------------------------------
void *operator new(std::size_t size) {
    return AllocateThrowing(size, kDefaultAlignment);
}

void *operator new[](std::size_t size) {
    return AllocateThrowing(size, kDefaultAlignment);
}

void *operator new(std::size_t size, std::align_val_t alignment) {
    return AllocateThrowing(size, static_cast<std::size_t>(alignment));
}

void *operator new[](std::size_t size, std::align_val_t alignment) {
    return AllocateThrowing(size, static_cast<std::size_t>(alignment));
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    return AllocateNoThrow(size, kDefaultAlignment);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    return AllocateNoThrow(size, kDefaultAlignment);
}

void *operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    return AllocateNoThrow(size, static_cast<std::size_t>(alignment));
}

void *operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    return AllocateNoThrow(size, static_cast<std::size_t>(alignment));
}

// ----------------------------------------------------------------------------------------------
// Replacement operator delete (all plain / sized / over-aligned / nothrow forms). Every form
// resolves to TrackedFree — the size and alignment arguments are redundant given the header.
// ----------------------------------------------------------------------------------------------
void operator delete(void *pointer) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer) noexcept {
    TrackedFree(pointer);
}

void operator delete(void *pointer, std::size_t) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer, std::size_t) noexcept {
    TrackedFree(pointer);
}

void operator delete(void *pointer, std::align_val_t) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer, std::align_val_t) noexcept {
    TrackedFree(pointer);
}

void operator delete(void *pointer, std::size_t, std::align_val_t) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer, std::size_t, std::align_val_t) noexcept {
    TrackedFree(pointer);
}

void operator delete(void *pointer, const std::nothrow_t &) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer, const std::nothrow_t &) noexcept {
    TrackedFree(pointer);
}

void operator delete(void *pointer, std::align_val_t, const std::nothrow_t &) noexcept {
    TrackedFree(pointer);
}

void operator delete[](void *pointer, std::align_val_t, const std::nothrow_t &) noexcept {
    TrackedFree(pointer);
}

#endif // VE_MEMORY_DISABLE_GLOBAL_NEW
