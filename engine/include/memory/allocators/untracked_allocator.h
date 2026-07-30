#pragma once

// NOTE: Deliberately does NOT include `vlkypch.h`. This allocator is what the memory tracker's own
// bookkeeping runs on, so it must stay usable from the lowest layers of the memory subsystem without
// dragging the engine's precompiled header (and its allocations) in behind it.
#include <cstddef>
#include <cstdlib>
#include <new>

namespace Vulkyrie {

    /** @brief An STL-compatible allocator that goes straight to `std::malloc`/`std::free`, bypassing the engine's
     * replaced `operator new`.
     *
     * This exists for exactly one reason: the memory tracker's own storage cannot be allocated through the
     * allocation path it is tracking. A `std::unordered_map` recording live allocations would, with the default
     * allocator, call `operator new` on every insert - which calls back into the tracker, which inserts again.
     * Routing the tracker's containers through this allocator breaks that cycle, and keeps the tracker's own
     * footprint out of the numbers it reports.
     *
     * Consequently, memory obtained here is invisible to per-subsystem accounting. That is the point; do not use
     * it for ordinary engine data. For attributed container storage use `TrackedStdAllocator` instead.
     *
     * @tparam T The element type. */
    template <typename T> class UntrackedAllocator {
    public:
        using value_type = T;

        constexpr UntrackedAllocator() noexcept = default;

        template <typename TOther> constexpr explicit UntrackedAllocator(const UntrackedAllocator<TOther> &) noexcept {
        }

        /** @brief Allocates storage for `count` objects.
         * @param count Number of objects.
         * @returns Pointer to uninitialized storage.
         * @throws std::bad_alloc if the request overflows or `malloc` fails. */
        [[nodiscard]] T *allocate(size_t count) {
            if (count > (static_cast<size_t>(-1) / sizeof(T))) {
                throw std::bad_alloc{};
            }

            const size_t bytes = count * sizeof(T);
            void *memory = nullptr;

            if constexpr (alignof(T) > alignof(std::max_align_t)) {
                // aligned_alloc requires the size to be a multiple of the alignment.
                const size_t alignedBytes = (bytes + alignof(T) - 1) & ~(alignof(T) - 1);
                memory = std::aligned_alloc(alignof(T), alignedBytes);
            } else {
                memory = std::malloc(bytes);
            }

            if (nullptr == memory) {
                throw std::bad_alloc{};
            }

            return static_cast<T *>(memory);
        }

        /** @brief Releases storage obtained from `allocate`.
         * @param pointer The storage to release.
         * @param count Unused; present for allocator-interface conformance. */
        void deallocate(T *pointer, size_t count) noexcept {
            (void)count;
            std::free(pointer);
        }

        template <typename TOther> bool operator==(const UntrackedAllocator<TOther> &) const noexcept {
            return true;
        }
    };

} // namespace Vulkyrie
