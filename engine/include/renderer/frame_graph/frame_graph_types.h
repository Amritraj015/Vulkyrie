#pragma once

#include "vlkypch.h"

#include <atomic>
#include <limits>

namespace Vulkyrie {

    /** @brief A strongly-typed 32-bit index. `Tag` is phantom - it appears in no data member - so the struct is
     * exactly one `u32` and register-passed, while still making a `FrameGraphPassID` and a `FrameGraphResourceID`
     * mutually unassignable. */
    template <typename Tag> struct TypedIndex {
    public:
        constexpr TypedIndex() noexcept = default;

        constexpr explicit TypedIndex(u32 value) noexcept
            : _value(value) {
        }

        [[nodiscard]] VE_INLINE explicit constexpr operator u32() const noexcept {
            return _value;
        }

        /** @brief Returns the raw value, for use as an array subscript. */
        [[nodiscard]] VE_INLINE constexpr u32 Get() const noexcept {
            return _value;
        }

        /** @brief Checks whether the index refers to an element rather than being the invalid sentinel. */
        [[nodiscard]] VE_INLINE constexpr bool IsValid() const noexcept {
            return _value != INVALID_INDEX;
        }

        friend constexpr auto operator<=>(TypedIndex, TypedIndex) = default;

    private:
        static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

        u32 _value = INVALID_INDEX;
    };

    /** @brief Identifies a pass, in declaration order. */
    using FrameGraphPassID = TypedIndex<struct FrameGraphPassTag>;

    /** @brief Identifies one version of a resource. */
    using FrameGraphResourceID = TypedIndex<struct FrameGraphResourceTag>;

    /** @brief Identifies the entry backing a resource, shared by all its versions. */
    using FrameGraphResourceEntryID = TypedIndex<struct FrameGraphResourceEntryTag>;

    static_assert(sizeof(FrameGraphPassID) == sizeof(u32), "FrameGraphPassID size must be equal to sizeof(u32).");
    static_assert(sizeof(FrameGraphResourceID) == sizeof(u32), "FrameGraphResourceID size must be equal to sizeof(u32).");
    static_assert(sizeof(FrameGraphResourceEntryID) == sizeof(u32), "FrameGraphResourceEntryID size must be equal to sizeof(u32).");
    static_assert(std::is_trivially_copyable_v<FrameGraphPassID>, "FrameGraphPassID must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<FrameGraphResourceID>, "FrameGraphResourceID must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<FrameGraphResourceEntryID>, "FrameGraphResourceEntryID must be trivially copyable.");

    /** @brief A resource handle carrying the type it was created with, so a buffer handle cannot be passed where a
     * texture is expected and `FrameGraph::GetResource` cannot reinterpret the wrong type.
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

    /** @brief How a pass touches a resource, in terms a backend can turn into a barrier. The masks are opaque to
     * the graph; a Vulkan backend maps them to `VkPipelineStageFlags2` / `VkAccessFlags2` / `VkImageLayout`. */
    struct ResourceUsage {
    public:
        /** @brief Backend-defined pipeline stage mask. */
        u32 Stages = 0;

        /** @brief Backend-defined access mask. */
        u32 Access = 0;

        /** @brief Backend-defined image layout. */
        u16 Layout = 0;

        /** @brief Backend-defined queue type (graphics / async-compute / transfer). */
        u16 QueueType = 0;

        friend constexpr bool operator==(const ResourceUsage &, const ResourceUsage &) = default;
    };

    static_assert(sizeof(ResourceUsage) == 12, "ResourceUsage is expected to pack into 12 bytes.");
    static_assert(std::is_trivially_copyable_v<ResourceUsage>, "ResourceUsage must be trivially copyable.");

    /** @brief One pass's access to one resource version. Lives in graph-level arrays rather than per-pass vectors;
     * a pass refers to its accesses by a `(begin, count)` range. */
    struct ResourceAccess {
    public:
        FrameGraphResourceID Resource{};
        ResourceUsage Usage{};

        friend constexpr bool operator==(const ResourceAccess &, const ResourceAccess &) = default;
    };

    /** @brief A single transition the backend must emit before a pass runs. `Compile` batches these into a
     * graph-level array with a per-pass range, so execution makes one `EmitBarriers` call per pass rather than one
     * per resource. */
    struct ResourceBarrier {
    public:
        FrameGraphResourceEntryID Entry{};

        /** @brief The usage the resource is currently in.
         *
         * When `AliasingTransition` is set this instead carries the union of the stage and access masks the
         * previous occupants of these bytes were left in - the scope the discard has to wait on. `Layout` is left
         * zero there, because the contents being transitioned from are not this resource's. */
        ResourceUsage Before{};

        /** @brief The usage the upcoming pass requires. */
        ResourceUsage After{};

        /** @brief Whether this is the first use of a transient that took over storage the aliasing plan had already
         * given to a resource whose lifetime has ended.
         *
         * The backend must treat the contents as undefined - a Vulkan backend transitions from
         * `VK_IMAGE_LAYOUT_UNDEFINED` rather than from `Before.Layout`, which both discards the previous occupant's
         * data and lets the driver skip decompressing it. Ignoring the flag and transitioning from a layout the
         * memory was never in is a validation error and, worse, can preserve garbage. */
        bool AliasingTransition = false;
    };

    /** @brief The size and alignment a transient resource needs from the aliasing allocator. Resource types opt in
     * by providing `GetMemoryRequirements(descriptor)`; see `HasMemoryRequirements`. */
    struct ResourceMemoryRequirements {
    public:
        u64 Size = 0;

        /** @brief Must be a power of two. */
        u64 Alignment = 1;
    };

    /** @brief Where the transient aliasing plan put a resource inside the graph's shared transient storage. Handed
     * to resource types that opt into the placed `Create` overload; see `HasPlacedCreate`.
     *
     * Passed by value: sixteen trivially-copyable bytes travel in registers, where a reference would force the
     * placement into memory on a call the execute path makes once per created resource. */
    struct ResourcePlacement {
    public:
        /** @brief Byte offset into the transient storage block. Only meaningful when `IsAliased` is set. */
        u64 Offset = 0;

        /** @brief Whether the plan placed this resource at all. False for imported resources, for types that report
         * no memory requirements, and for resources no surviving pass touches - in each case the resource type
         * should allocate exactly as it would without a plan. */
        bool IsAliased = false;
    };

    static_assert(std::is_trivially_copyable_v<ResourcePlacement>, "ResourcePlacement must be trivially copyable so it can be passed in registers.");

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

    struct FrameGraphContext;

    /** @brief Backend hook invoked once per pass with that pass's batched transitions. */
    using BarrierEmitFn = void (*)(const FrameGraphContext &context, std::span<const ResourceBarrier> barriers);

    /** @brief Everything a pass or a resource type needs at execute time. */
    struct FrameGraphContext {
    public:
        /** @brief The backend render context (command buffer, device, GL state wrapper, ...). */
        void *RenderContext = nullptr;

        /** @brief The transient-resource pool that materializes and releases graph-owned resources. */
        void *TransientResources = nullptr;

        /** @brief Barrier hook; when null the graph computes transitions but emits nothing (the OpenGL path). */
        BarrierEmitFn EmitBarriers = nullptr;
    };

} // namespace Vulkyrie
