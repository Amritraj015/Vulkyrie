#include "memory/allocation_block.h"
#include "memory/memory_scope.h"

#include <cstddef>
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

// Brings CurrentMemoryTag and the `detail::` block helpers into scope: this file's code sits at
// global scope, not inside namespace Vulkyrie.
using namespace Vulkyrie;

namespace {

    /** @brief Allocates `size` payload bytes aligned to `alignment`, attributed to the calling thread's
     * innermost memory scope.
     * @returns The payload pointer, or nullptr on failure (including size overflow). */
    [[nodiscard]] void *TrackedAlloc(std::size_t size, std::size_t alignment) {
        return detail::AllocateBlock(size, alignment, CurrentMemoryTag(), detail::BlockOwner::GlobalNew);
    }

    /** @brief Frees a pointer produced by TrackedAlloc and updates the tracker. */
    void TrackedFree(void *pointer) noexcept {
        if (nullptr == pointer) {
            return;
        }

        detail::ReleaseBlock(pointer, detail::BlockOwner::GlobalNew);
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
