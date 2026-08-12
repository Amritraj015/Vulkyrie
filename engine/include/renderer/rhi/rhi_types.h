#pragma once

#include "core/types/handle.h"

namespace Vulkyrie {

    enum class ShaderStage : u8 { Vertex, Fragment, Compute, Task, Mesh, RayGen, Miss, ClosestHit, AnyHit };
    // enum class CullMode : u8 { None, Front, Back };
    // enum class FrontFace : u8 { CounterClockwise, Clockwise };
    // enum class FillMode : u8 { Solid, Wireframe };
    // enum class CompareOp : u8 { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
    // enum class BlendFactor : u8 { Zero, One, SrcColor, InvSrcColor, DstColor, InvDstColor, SrcAlpha, InvSrcAlpha, DstAlpha, InvDstAlpha };
    // enum class BlendOp : u8 { Add, Subtract, RevSubtract, Min, Max };
    // enum class IndexType : u8 { Uint16, Uint32 };
    // enum class LoadOp : u8 { Load, Clear, DontCare };
    // enum class StoreOp : u8 { Store, DontCare, Resolve };

    enum class DeviceStatus : u8 {
        Ok,          // Device OK
        Lost,        // Unrecoverable for this device; recreate everything
        OutOfMemory, // Budget exceeded; evict and retry
        OutOfDate,   // Swapchain no longer matches the surface; recreate swapchain only
        SurfaceLost, // The surface object itself is gone; recreate surface AND swapchain
        Timeout,
    };

    enum class QueueType : u8 {
        Graphics, // graphics + compute + transfer
        Compute,  // async compute
        Transfer, // DMA / upload, often over PCIe without touching the shader cores
        Count,
    };

    enum class Format : u16 {
        Undefined = 0,
        R8Unorm,
        RG8Unorm,
        RGBA8Unorm,
        RGBA8Srgb,
        BGRA8Unorm,
        BGRA8Srgb,
        R16Uint,
        R16Float,
        RG16Float,
        RGBA16Float,
        RG16Snorm,
        R32Uint,
        R32Float,
        RG32Float,
        RGB32Float,
        RGBA32Float,
        R11G11B10Float,
        RGB10A2Unorm,
        RGB9E5Float,
        D16Unorm,
        D32Float,
        D32FloatS8Uint,
        BC1Unorm,
        BC1Srgb,
        BC3Unorm,
        BC3Srgb,
        BC4Unorm,
        BC5Unorm,
        BC6HFloat,
        BC7Unorm,
        BC7Srgb,
    };

    // // ACCESS, NOT LAYOUT.
    // //
    // // The render graph reasons in terms of "what will this pass do with this
    // // resource", and each backend derives its own concrete barrier from that: Vulkan
    // // maps it to VkImageLayout + VkPipelineStageFlags2 + VkAccessFlags2, D3D12 to a
    // // D3D12_BARRIER_LAYOUT + sync/access pair. Exposing Vulkan layouts here would
    // // make the graph — the single largest piece of backend-independent code — quietly
    // // Vulkan-shaped, which is the mistake that forces a rewrite for backend #2.
    // //
    // // These are flags because a resource can legitimately be read two ways at once
    // // (e.g. sampled in one pass and used as an indirect-arg source in another that
    // // shares a barrier batch).
    // enum class ResourceAccess : u32 {
    //     None = 0,
    //     VertexBuffer = 1u << 0,
    //     IndexBuffer = 1u << 1,
    //     UniformRead = 1u << 2,
    //     ShaderRead = 1u << 3,  // SRV / sampled or storage-read
    //     ShaderWrite = 1u << 4, // UAV / storage-write
    //     ColorAttachment = 1u << 5,
    //     DepthRead = 1u << 6,
    //     DepthWrite = 1u << 7,
    //     TransferSrc = 1u << 8,
    //     TransferDst = 1u << 9,
    //     IndirectArgs = 1u << 10,
    //     Present = 1u << 11,
    //     AccelStructRead = 1u << 12,
    //     AccelStructWrite = 1u << 13,
    // };
    // [[nodiscard]] constexpr ResourceAccess operator|(ResourceAccess a, ResourceAccess b) noexcept {
    //     return static_cast<ResourceAccess>(static_cast<u32>(a) | static_cast<u32>(b));
    // }
    //
    // [[nodiscard]] constexpr ResourceAccess operator&(ResourceAccess a, ResourceAccess b) noexcept {
    //     return static_cast<ResourceAccess>(static_cast<u32>(a) & static_cast<u32>(b));
    // }
    //
    // [[nodiscard]] constexpr bool Any(ResourceAccess a) noexcept {
    //     return static_cast<u32>(a) != 0;
    // }
    // // Writes are what force barriers and what break aliasing. Keeping this as a
    // // single predicate means the graph compiler never has to enumerate access bits.
    // [[nodiscard]] constexpr bool IsWrite(ResourceAccess a) noexcept {
    //     constexpr ResourceAccess kWrites = ResourceAccess::ShaderWrite | ResourceAccess::ColorAttachment | ResourceAccess::DepthWrite |
    //                                        ResourceAccess::TransferDst | ResourceAccess::AccelStructWrite;
    //     return Any(a & kWrites);
    // }

    enum class MemoryLocation : u8 {
        DeviceLocal,  // VRAM. Everything the GPU reads more than once.
        HostUpload,   // write-combined, CPU-write / GPU-read. Staging + per-frame constants.
        HostReadback, // cached, GPU-write / CPU-read. Query results, screenshots, GPU feedback.
        DeviceUpload, // resizable BAR: VRAM the CPU can write directly. Best for small hot updates.
    };

    enum class HeapUsage : u32 {
        None = 0,
        Buffers = 1u << 0,
        Textures = 1u << 1,      // non-render-target textures
        RenderTargets = 1u << 2, // color/depth attachments, and MSAA targets
    };

    [[nodiscard]] constexpr HeapUsage operator|(HeapUsage a, HeapUsage b) noexcept {
        return static_cast<HeapUsage>(static_cast<u32>(a) | static_cast<u32>(b));
    }

    using BufferHandle = GenerationalHandle<struct BufferTag>;
    using TextureHandle = GenerationalHandle<struct TextureTag>;
    using SamplerHandle = GenerationalHandle<struct SamplerTag>;
    using PipelineHandle = GenerationalHandle<struct PipelineTag>;
    using QueryHeapHandle = GenerationalHandle<struct QueryHeapTag>;
    using ShaderHandle = GenerationalHandle<struct ShaderTag>;
    using HeapHandle = GenerationalHandle<struct HeapTag>;

    static_assert(sizeof(BufferHandle) == sizeof(u32));
    static_assert(sizeof(TextureHandle) == sizeof(u32));
    static_assert(sizeof(SamplerHandle) == sizeof(u32));
    static_assert(sizeof(PipelineHandle) == sizeof(u32));
    static_assert(sizeof(QueryHeapHandle) == sizeof(u32));
    static_assert(sizeof(ShaderHandle) == sizeof(u32));
    static_assert(sizeof(HeapHandle) == sizeof(u32));
    static_assert(std::is_trivially_copyable_v<BufferHandle>);
    static_assert(std::is_trivially_copyable_v<TextureHandle>);
    static_assert(std::is_trivially_copyable_v<SamplerHandle>);
    static_assert(std::is_trivially_copyable_v<PipelineHandle>);
    static_assert(std::is_trivially_copyable_v<QueryHeapHandle>);
    static_assert(std::is_trivially_copyable_v<ShaderHandle>);
    static_assert(std::is_trivially_copyable_v<HeapHandle>);

    using BindlessIndex = Handle<struct BindlessTag>;

    static_assert(sizeof(BindlessIndex) == sizeof(u32));
    static_assert(std::is_trivially_copyable_v<BindlessIndex>);

    // -------------------------------------------------------------------
    // TODO: Finish these up.
    struct BufferDescriptor;
    struct ImageDescriptor;
    struct SamplerDescriptor;
    struct GraphicsPipelineDescriptor;
    struct ComputePipelineDescriptor;
    struct RendererStatistics;

    struct HeapDescriptor {
        std::string_view DebugName;
        u64 SizeBytes = 0;
        MemoryLocation Location = MemoryLocation::DeviceLocal;
        HeapUsage Usage = HeapUsage::RenderTargets;
    };

    struct ResourceFootprint {
        u64 SizeBytes = 0;
        u64 Alignment = 0;
        u32 MemoryTypeBits = std::numeric_limits<u32>::max(); // Vulkan: heap must satisfy these
    };

    struct ShaderBlob {
        std::span<const std::byte> bytes;
        ShaderStage stage = ShaderStage::Compute;
        std::string_view entryPoint = "main";
        u64 contentHash = 0; // computed by the cooker, not at load time
    };

    struct TextureSubresource {
        u32 BaseMip = 0;
        u32 MipCount = std::numeric_limits<u32>::max();
        u32 BaseLayer = 0;
        u32 LayerCount = std::numeric_limits<u32>::max();
    };

    struct TimelineValue {
        u64 Value = 0;
        QueueType Queue = QueueType::Graphics;

        [[nodiscard]] constexpr bool operator<=(const TimelineValue &o) const noexcept {
            return Value <= o.Value;
        }
    };

    // -------------------------------------------------------------------

    struct DeviceCreationInfo {
        void *NativeWindow = nullptr;
        void *NativeDisplay = nullptr;

        u32 MaxBuffers = 65536;
        u32 MaxTextures = 262144;
        u32 MaxPipelines = 16384;
        u32 MaxSamplers = 256;

        bool EnableValidation = false;
        bool PreferDiscreteGpu = true;
        bool VSync = false;

        // u32 FramesInFlight = 2; // 2 for latency, 3 to hide a spiky CPU frame
        // bool EnableGpuValidation = false;
        // bool EnableCaptureSupport = false; // RenderDoc/PIX marker + capture hooks

        // Handle-table budgets. Fixed at init because SlotMap's lock-free reads
        // require non-reallocating storage; see core/SlotMap.h.
    };

    struct DeviceCapabilities {
        u64 VRAMBudgetBytes = 0;
        u32 MaxBindlessTextures = 0;
        u32 MaxBindlessBuffers = 0;
        u32 SubgroupSize = 0;
        f32 TimestampPeriodNs = 0.0f;
        bool MeshShaders = false;
        bool RayTracing = false;
        bool AsyncCompute = false;
        bool DrawIndirectCount = false; // load the draw count from a GPU buffer
        bool ResizableBar = false;
        bool Int16Shader = false;
        bool WaveIntrinsics = false;
        bool ConservativeRaster = false;
    };

    struct MemoryStats {
        u64 deviceLocalUsed = 0;
        u64 deviceLocalBudget = 0;
        u64 hostVisibleUsed = 0;
        u64 allocationCount = 0;
        u64 blockCount = 0;
    };

    // struct Viewport {
    //     f32 X = 0;
    //     f32 Y = 0;
    //     f32 Width = 0;
    //     f32 Height = 0;
    //     f32 MinDepth = 0.0f;
    //     f32 MaxDepth = 1.0f;
    // };

} // namespace Vulkyrie
