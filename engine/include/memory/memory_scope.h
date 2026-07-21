#pragma once

// NOTE: This header is force-included via `vlkypch.h`, so it must stay lightweight and
// self-contained: it depends only on `memory_tag.h` + a couple of freestanding std headers, and it
// must NOT pull in `memory_tracker.h`/`<atomic>` (the scope stack only tracks the *current tag*,
// never the byte counters) nor `vlkypch.h` (that would create a circular include).
#include "memory/memory_tag.h"

#include <cstddef>
#include <cstdint>

// Master switch for scope-based attribution. On by default (including release — the cheap tier is
// release-safe). Define to 0 before including the engine to compile `VE_MEMORY_SCOPE` out entirely.
#if !defined(VE_MEMORY_TRACKING)
#define VE_MEMORY_TRACKING 1
#endif

namespace Vulkyrie {

    namespace detail {

        /** @brief Maximum nesting depth of `VE_MEMORY_SCOPE`s on a single thread. */
        inline constexpr std::size_t kMemoryScopeStackCapacity = 32;

        /** @brief Per-thread stack of active memory tags. Trivially/constant-initialized so it is
         * valid before any dynamic initialization runs (allocations from other globals' constructors
         * are still attributable) and allocation-free (the global `operator new` reads it, so it must
         * never itself allocate). */
        struct MemoryScopeStack {
            MemoryTag Tags[kMemoryScopeStackCapacity];
            std::uint32_t Depth;
        };

        inline thread_local constinit MemoryScopeStack tMemoryScopeStack{};

        // Anchor forcing the `global_new_delete.cpp` translation unit to be linked even out of the
        // `engine` static library. The `inline` variable's initializer ODR-uses the function in every
        // TU that sees this header (i.e. everything, via the PCH), so the linker must pull in the
        // object file that defines it — which also carries the `operator new`/`delete` replacements.
        int ForceLinkGlobalNewDelete();
        [[maybe_unused]] inline int gForceLinkGlobalNewDelete = ForceLinkGlobalNewDelete();

    } // namespace detail

    /** @brief Pushes a memory tag onto the calling thread's scope stack.
     * @param tag The subsystem tag to attribute subsequent allocations to.
     */
    inline void PushMemoryTag(MemoryTag tag) {
        auto &stack = detail::tMemoryScopeStack;

        if (stack.Depth < detail::kMemoryScopeStackCapacity) {
            stack.Tags[stack.Depth] = tag;
        }

        // Always increment (even past capacity) so the matching Pop stays balanced.
        ++stack.Depth;
    }

    /** @brief Pops the top memory tag off the calling thread's scope stack. */
    inline void PopMemoryTag() {
        auto &stack = detail::tMemoryScopeStack;

        if (stack.Depth > 0) {
            --stack.Depth;
        }
    }

    /** @brief Returns the memory tag currently in effect on the calling thread.
     * @returns The innermost active tag, or `MemoryTag::Untagged` if no scope is active.
     */
    [[nodiscard]] inline MemoryTag CurrentMemoryTag() {
        const auto &stack = detail::tMemoryScopeStack;
        const std::uint32_t effectiveDepth = stack.Depth <= detail::kMemoryScopeStackCapacity ? stack.Depth : static_cast<std::uint32_t>(detail::kMemoryScopeStackCapacity);

        return effectiveDepth == 0 ? MemoryTag::Untagged : stack.Tags[effectiveDepth - 1];
    }

    /** @brief RAII guard that attributes all allocations made within its lifetime to a subsystem.
     * Prefer the `VE_MEMORY_SCOPE(tag)` macro over constructing this directly. */
    class MemoryScope {
    public:
        /** @brief Pushes `tag` onto the current thread's scope stack.
         * @param tag The subsystem tag to attribute allocations to for this scope's lifetime.
         */
        explicit MemoryScope(MemoryTag tag) {
            PushMemoryTag(tag);
        }

        ~MemoryScope() {
            PopMemoryTag();
        }

        // Non-copyable and non-movable: the guard is strictly tied to its stack frame. (Written out
        // manually rather than via VE_DELETE_MOVE_AND_COPY to keep this header self-contained.)
        MemoryScope(const MemoryScope &) = delete;
        MemoryScope &operator=(const MemoryScope &) = delete;
        MemoryScope(MemoryScope &&) = delete;
        MemoryScope &operator=(MemoryScope &&) = delete;
    };

} // namespace Vulkyrie

#if VE_MEMORY_TRACKING
#define VE_MEMORY_SCOPE_CONCAT(a, b) a##b
#define VE_MEMORY_SCOPE_LINE(tag, line) ::Vulkyrie::MemoryScope VE_MEMORY_SCOPE_CONCAT(veMemoryScope, line)(tag)
#define VE_MEMORY_SCOPE(tag) VE_MEMORY_SCOPE_LINE(tag, __LINE__)
#else
#define VE_MEMORY_SCOPE_CONCAT(a, b) 
#define VE_MEMORY_SCOPE_LINE(tag, line) 
#define VE_MEMORY_SCOPE(tag) 
#endif
