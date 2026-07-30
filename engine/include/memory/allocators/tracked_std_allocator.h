#pragma once

#include <new>
#include <vector>
#include <unordered_map>
#include "memory/memory_scope.h"

namespace Vulkyrie {

    /** @brief An STL-compatible allocator that attributes a container's storage to a fixed subsystem, regardless
     * of which thread or call path grows it.
     *
     * Scope-based attribution (`VE_MEMORY_SCOPE`) already covers most allocations, but it attributes to whoever
     * happens to be on the stack at the moment a container grows, so a container owned by one subsystem but grown
     * from inside another's call stack is charged to the wrong one. Baking the tag into the container's type removes
     * the ambiguity: the storage is charged to `Tag` wherever it is touched from.
     *
     * Allocation still goes through the engine's replaced `operator new`, so these bytes appear in the ordinary
     * per-subsystem heap counters and, when it is on, the deep table - this is a re-attribution, not a separate
     * pool.
     *
     * @tparam T The element type.
     * @tparam Tag The subsystem the storage is charged to. */
    template <typename T, MemoryTag Tag> class TrackedStdAllocator {
    public:
        using value_type = T;

        /** @brief Rebinds to another element type, keeping the tag. Needed by node-based containers, which
         * allocate their node type rather than `T`. */
        template <typename TOther> struct rebind {
            using other = TrackedStdAllocator<TOther, Tag>;
        };

        constexpr TrackedStdAllocator() noexcept = default;

        template <typename TOther> constexpr explicit TrackedStdAllocator(const TrackedStdAllocator<TOther, Tag> &) noexcept {
        }

        /** @brief Allocates storage for `count` objects, charged to `Tag`.
         * @param count Number of objects.
         * @returns Pointer to uninitialized storage.
         * @throws std::bad_alloc on failure. */
        [[nodiscard]] T *allocate(size_t count) {
            // The scope is pushed here rather than at the call site so the tag follows the container, not the
            // caller: this is the whole reason to use a typed allocator over a VE_MEMORY_SCOPE.
            VE_MEMORY_SCOPE(Tag);

            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                return static_cast<T *>(::operator new(count * sizeof(T), std::align_val_t{ alignof(T) }));
            } else {
                return static_cast<T *>(::operator new(count * sizeof(T)));
            }
        }

        /** @brief Releases storage obtained from `allocate`. No scope is needed: the allocation header already
         * records which subsystem to credit.
         * @param pointer The storage to release.
         * @param count Number of objects the storage was allocated for. */
        void deallocate(T *pointer, size_t count) noexcept {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                ::operator delete(pointer, count * sizeof(T), std::align_val_t{ alignof(T) });
            } else {
                ::operator delete(pointer, count * sizeof(T));
            }
        }

        template <typename TOther> bool operator==(const TrackedStdAllocator<TOther, Tag> &) const noexcept {
            return true;
        }
    };

    /** @brief A `std::vector` whose storage is charged to a fixed subsystem. */
    template <typename T, MemoryTag Tag> using TrackedVector = std::vector<T, TrackedStdAllocator<T, Tag>>;

    /** @brief A `std::unordered_map` whose storage is charged to a fixed subsystem. */
    template <typename TKey, typename TValue, MemoryTag Tag>
    using TrackedUnorderedMap = std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<TKey>, TrackedStdAllocator<std::pair<const TKey, TValue>, Tag>>;

    /** @brief A `std::string` whose storage is charged to a fixed subsystem. Note that short strings stay in the
     * inline buffer and never reach the allocator at all. */
    template <MemoryTag Tag> using TrackedString = std::basic_string<char, std::char_traits<char>, TrackedStdAllocator<char, Tag>>;

} // namespace Vulkyrie
