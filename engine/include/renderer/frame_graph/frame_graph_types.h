#pragma once

#include "vlkypch.h"
#include "core/types/handle.h"
#include "renderer/rhi/barrier_types.h"

#include <atomic>

namespace Vulkyrie {

    /** @brief Identifies a pass, in declaration order. */
    using FrameGraphPassID = Handle<struct FrameGraphPassTag>;

    /** @brief Identifies one version of a resource. */
    using FrameGraphResourceID = Handle<struct FrameGraphResourceTag>;

    /** @brief Identifies the entry backing a resource, shared by all its versions.
     *
     * Spelled as the RHI's `BarrierResourceID` because that is exactly what it is used for outside the graph: it is
     * the id a `ResourceBarrier` carries, and the only thing a backend can correlate a barrier back to. */
    using FrameGraphResourceEntryID = BarrierResourceID;

    static_assert(sizeof(FrameGraphPassID) == sizeof(u32), "FrameGraphPassID size must be equal to sizeof(u32).");
    static_assert(sizeof(FrameGraphResourceID) == sizeof(u32), "FrameGraphResourceID size must be equal to sizeof(u32).");
    static_assert(sizeof(FrameGraphResourceEntryID) == sizeof(u32), "FrameGraphResourceEntryID size must be equal to sizeof(u32).");
    static_assert(std::is_trivially_copyable_v<FrameGraphPassID>, "FrameGraphPassID must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<FrameGraphResourceID>, "FrameGraphResourceID must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<FrameGraphResourceEntryID>, "FrameGraphResourceEntryID must be trivially copyable.");

    /** @brief A resource handle carrying the type it was created with, so a buffer handle cannot be passed where a
     * texture is expected and `FrameGraphResources::Get` cannot reinterpret the wrong type.
     *
     * `TResource` is phantom, so the handle is exactly one `u32`. The templates stop at the API boundary -
     * `Builder::Read`/`Write` strip the handle down to a `FrameGraphResourceID` and tail-call a non-template
     * implementation - so nothing is instantiated per resource type beyond the trivial wrappers.
     *
     * @tparam TResource The resource type this handle refers to. Deliberately unconstrained so the handle stays
     * usable with an incomplete type; `FrameGraphResourceType` is applied at the methods that touch the type. */
    template <typename TResource> struct FrameGraphHandle final {
    public:
        /** @brief The untyped resource node this handle refers to. */
        FrameGraphResourceID ID{};

        /** @brief Checks whether the handle refers to a resource rather than being default-constructed. */
        [[nodiscard]] VE_INLINE constexpr bool IsValid() const noexcept {
            return ID.IsValid();
        }

        friend constexpr bool operator==(FrameGraphHandle, FrameGraphHandle) = default;
    };

    /** @brief Incomplete probe type used only to assert the phantom parameter costs nothing. */
    struct FrameGraphHandleSizeProbe;

    static_assert(sizeof(FrameGraphHandle<FrameGraphHandleSizeProbe>) == sizeof(u32),
                  "FrameGraphHandle must be exactly one u32 - the resource parameter is phantom.");
    static_assert(std::is_trivially_copyable_v<FrameGraphHandle<FrameGraphHandleSizeProbe>>, "FrameGraphHandle must be trivially copyable.");

    namespace detail {

        /** @brief Hands out the next frame-graph resource type id. Monotonic across the process. */
        [[nodiscard]] inline u16 NextFrameGraphTypeID() {
            static std::atomic<u16> counter{ 0 };
            return counter.fetch_add(1, std::memory_order_relaxed);
        }

    } // namespace detail

    /** @brief Returns a stable, process-unique id for a resource type. Used as a Debug-only tag on `ResourceEntry`
     * so `GetResource<T>()` can validate that `T` matches what is actually stored, closing the unchecked-downcast
     * hazard at zero release cost. */
    template <typename T> [[nodiscard]] u16 FrameGraphTypeID() {
        static const u16 id = detail::NextFrameGraphTypeID();
        return id;
    }

    /** @brief One pass's access to one resource version. Lives in graph-level arrays rather than per-pass vectors;
     * a pass refers to its accesses by a `(begin, count)` range. */
    struct ResourceAccess {
    public:
        FrameGraphResourceID Resource{};
        ResourceUsage Usage{};

        friend constexpr bool operator==(const ResourceAccess &, const ResourceAccess &) = default;
    };

    /** @brief Up-front sizing for a `FrameGraph` reused across frames. These are reserve hints, not limits: a frame
     * may exceed any of them. Reserving is what makes the steady state allocation-free - frame 1 grows the arena
     * and the node vectors, every later frame runs off the retained capacity. */
    struct FrameGraphConfig {
    public:
        u32 ExpectedPasses = 64;

        u32 ExpectedResources = 128;

        /** @brief The arena grows by chunking if a frame exceeds it, and keeps the grown capacity across `Reset`,
         * so an undersized value costs allocations only until steady state. */
        size_t InitialArenaBytes = 64 * 1024;
    };

} // namespace Vulkyrie
