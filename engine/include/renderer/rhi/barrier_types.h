#pragma once

#include "vlkypch.h"
#include "core/types/handle.h"

namespace Vulkyrie {

    /** @brief Identifies the resource a barrier applies to, in whatever numbering the barrier's producer uses.
     *
     * The frame graph passes its resource-entry ids through here. A backend uses this only to correlate a barrier
     * with a resource it has already been handed - it is not an index into any backend-owned storage. */
    using BarrierResourceID = Handle<struct BarrierResourceTag>;

    /** @brief How a pass touches a resource, in terms a backend can turn into a barrier. The masks are opaque to
     * the frame graph; a Vulkan backend maps them to `VkPipelineStageFlags2` / `VkAccessFlags2` / `VkImageLayout`. */
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

    /** @brief A single transition the backend must emit before a pass runs. The frame graph batches these so that
     * execution makes one `EmitBarriers` call per pass rather than one per resource. */
    struct ResourceBarrier {
    public:
        BarrierResourceID Entry{};

        /** @brief The usage the resource is currently in.
         *
         * When `AliasingTransition` is set this instead carries the union of the stage and access masks the
         * previous occupants of these bytes were left in - the scope the discard has to wait on. `Layout` is left
         * zero there, because the contents being transitioned from are not this resource's. */
        ResourceUsage Before{};

        /** @brief The usage the upcoming pass requires. */
        ResourceUsage After{};

        /** @brief Whether this is the first use of a transient that took over storage already given to a resource
         * whose lifetime has ended.
         *
         * The backend must treat the contents as undefined - a Vulkan backend transitions from
         * `VK_IMAGE_LAYOUT_UNDEFINED` rather than from `Before.Layout`, which both discards the previous occupant's
         * data and lets the driver skip decompressing it. Ignoring the flag and transitioning from a layout the
         * memory was never in is a validation error and, worse, can preserve garbage. */
        bool AliasingTransition = false;
    };

    /** @brief The size and alignment a transient resource needs from an aliasing allocator. Resource types opt in
     * by providing `GetMemoryRequirements(descriptor)`. */
    struct ResourceMemoryRequirements {
    public:
        u64 Size = 0;

        /** @brief Must be a power of two. */
        u64 Alignment = 1;
    };

    /** @brief Where a transient aliasing plan put a resource inside a shared storage block. Handed to resource
     * types that opt into the placed `Acquire` overload.
     *
     * Passed by value: sixteen trivially-copyable bytes travel in registers, where a reference would force the
     * placement into memory on a call the execute path makes once per acquired resource. */
    struct ResourcePlacement {
    public:
        /** @brief Byte offset into the shared storage block. Only meaningful when `IsAliased` is set. */
        u64 Offset = 0;

        /** @brief Whether the plan placed this resource at all. False for imported resources, for types that report
         * no memory requirements, for resources no surviving pass touches, and on backends that cannot bind two
         * resources to one allocation (`B::kHasMemoryAliasing == false`) - in each case the resource type should
         * allocate exactly as it would without a plan. */
        bool IsAliased = false;
    };

    static_assert(std::is_trivially_copyable_v<ResourcePlacement>, "ResourcePlacement must be trivially copyable so it can be passed in registers.");

} // namespace Vulkyrie
