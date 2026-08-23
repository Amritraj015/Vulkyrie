#pragma once

#include "vlkypch.h"
#include "core/types/handle.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    /** @brief Identifies the resource a barrier applies to, in whatever numbering the barrier's producer uses.
     *
     * The frame graph passes its resource-entry ids through here. A backend uses this only to correlate a barrier
     * with a resource it has already been handed - it is not an index into any backend-owned storage. */
    using BarrierResourceID = Handle<struct BarrierResourceTag>;

    /** @brief Pipeline stages an access can happen in.
     *
     * Engine-side and backend-agnostic: these are the stages the engine models, not a mirror of any one API's bits.
     * A backend maps them to its own - Vulkan to `VkPipelineStageFlags2`, which is 64 bits wide and sparsely
     * populated, so a dense engine enum both compresses better and survives that API's next expansion. The
     * underlying type is an implementation detail; widening it if the engine ever outgrows 32 stages changes no
     * call site. */
    enum class PipelineStage : u32 {
        None = 0,
        DrawIndirect = BIT(0),
        VertexInput = BIT(1),
        VertexShader = BIT(2),
        TessellationControlShader = BIT(3),
        TessellationEvaluationShader = BIT(4),
        GeometryShader = BIT(5),
        FragmentShader = BIT(6),
        EarlyFragmentTests = BIT(7),
        LateFragmentTests = BIT(8),
        ColorAttachmentOutput = BIT(9),
        ComputeShader = BIT(10),
        TaskShader = BIT(11),
        MeshShader = BIT(12),
        RayTracingShader = BIT(13),
        AccelerationStructureBuild = BIT(14),
        Copy = BIT(15),
        Blit = BIT(16),
        Resolve = BIT(17),
        Clear = BIT(18),
        Host = BIT(19),
        AllGraphics = BIT(20),
        AllCommands = BIT(21),
    };

    [[nodiscard]] VE_INLINE constexpr PipelineStage operator|(PipelineStage a, PipelineStage b) noexcept {
        return static_cast<PipelineStage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    VE_INLINE constexpr PipelineStage &operator|=(PipelineStage &a, PipelineStage b) noexcept {
        return a = a | b;
    }

    [[nodiscard]] VE_INLINE constexpr PipelineStage operator&(PipelineStage a, PipelineStage b) noexcept {
        return static_cast<PipelineStage>(static_cast<u32>(a) & static_cast<u32>(b));
    }

    [[nodiscard]] VE_INLINE constexpr bool HasFlag(PipelineStage a, PipelineStage b) noexcept {
        return 0 != (static_cast<u32>(a) & static_cast<u32>(b));
    }

    /** @brief How memory is touched in those stages. Separate from `PipelineStage` for the same reason the two are
     * separate in every explicit API: the stage says when, the access says what kind of hazard it can form. */
    enum class AccessFlags : u32 {
        None = 0,
        IndirectCommandRead = BIT(0),
        IndexRead = BIT(1),
        VertexAttributeRead = BIT(2),
        UniformRead = BIT(3),
        InputAttachmentRead = BIT(4),
        ShaderSampledRead = BIT(5),
        ShaderStorageRead = BIT(6),
        ShaderStorageWrite = BIT(7),
        ColorAttachmentRead = BIT(8),
        ColorAttachmentWrite = BIT(9),
        DepthStencilAttachmentRead = BIT(10),
        DepthStencilAttachmentWrite = BIT(11),
        TransferRead = BIT(12),
        TransferWrite = BIT(13),
        HostRead = BIT(14),
        HostWrite = BIT(15),
        AccelerationStructureRead = BIT(16),
        AccelerationStructureWrite = BIT(17),
        MemoryRead = BIT(18),
        MemoryWrite = BIT(19),
    };

    [[nodiscard]] VE_INLINE constexpr AccessFlags operator|(AccessFlags a, AccessFlags b) noexcept {
        return static_cast<AccessFlags>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    VE_INLINE constexpr AccessFlags &operator|=(AccessFlags &a, AccessFlags b) noexcept {
        return a = a | b;
    }

    [[nodiscard]] VE_INLINE constexpr AccessFlags operator&(AccessFlags a, AccessFlags b) noexcept {
        return static_cast<AccessFlags>(static_cast<u32>(a) & static_cast<u32>(b));
    }

    [[nodiscard]] VE_INLINE constexpr bool HasFlag(AccessFlags a, AccessFlags b) noexcept {
        return 0 != (static_cast<u32>(a) & static_cast<u32>(b));
    }

    /** @brief The layout an image must be in for an access. Not a flag set - an image is in exactly one layout.
     *
     * `Undefined` doubles as "no layout requirement", which is what a buffer access and an unspecified usage both
     * carry, and as the layout a discard transitions from. */
    enum class ImageLayout : u8 {
        Undefined = 0,
        General,
        ColorAttachment,
        DepthStencilAttachment,
        DepthStencilReadOnly,
        ShaderReadOnly,
        TransferSource,
        TransferDestination,
        Present,
    };

    /** @brief How a pass touches a resource, in terms a backend can turn into a barrier. */
    struct ResourceUsage {
    public:
        /** @brief The stages that touch the resource. */
        PipelineStage Stages = PipelineStage::None;

        /** @brief The kinds of access those stages make. */
        AccessFlags Access = AccessFlags::None;

        /** @brief The layout an image must be in; `Undefined` for buffers and for an unspecified usage. */
        ImageLayout Layout = ImageLayout::Undefined;

        /** @brief The queue the access happens on.
         *
         * Really a property of the pass rather than of one access - a pass runs on exactly one queue - which is why
         * two accesses in a pass disagreeing about it is an assertion rather than something to merge. */
        QueueType Queue = QueueType::Graphics;

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
         * `Undefined` there, because the contents being transitioned from are not this resource's. */
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

    /** @brief What a transient resource needs from an aliasing allocator, as the driver reports it.
     *
     * Not a CPU-side estimate. A packer that guesses low places two resources overlapping when their real extents
     * do not fit, so this has to come from the backend: `vkGetDeviceImageMemoryRequirements` /
     * `vkGetDeviceBufferMemoryRequirements` answer it from a descriptor without creating anything. */
    struct ResourceMemoryRequirements {
    public:
        u64 Size = 0;

        /** @brief Must be a power of two. */
        u64 Alignment = 1;

        /** @brief Which memory types can back this resource, one bit per device memory type.
         *
         * Two resources may only share an allocation if their masks intersect, because the allocation is made from
         * one memory type. All-ones means "no constraint", which is what a backend without the notion reports. */
        u32 MemoryTypeBits = std::numeric_limits<u32>::max();
    };

    /** @brief Where a transient aliasing plan put a resource. Handed to resource types that opt into the placed
     * `Acquire` overload.
     *
     * Passed by value: trivially-copyable and small enough to travel in registers, where a reference would force it
     * into memory on a call the execute path makes once per acquired resource. */
    struct ResourcePlacement {
    public:
        /** @brief Byte offset into the block. Only meaningful when `IsAliased` is set. */
        u64 Offset = 0;

        /** @brief Which block the offset is into. The plan packs one block per distinct memory-type mask, since
         * resources that cannot be backed by the same memory type cannot share bytes. */
        u32 BlockIndex = 0;

        /** @brief Whether the plan placed this resource at all. False for imported resources, for types that report
         * no memory requirements, for resources no surviving pass touches, and on backends that cannot bind two
         * resources to one allocation (`B::kHasMemoryAliasing == false`) - in each case the resource type should
         * allocate exactly as it would without a plan. */
        bool IsAliased = false;
    };

    static_assert(std::is_trivially_copyable_v<ResourcePlacement>, "ResourcePlacement must be trivially copyable so it can be passed in registers.");

} // namespace Vulkyrie
