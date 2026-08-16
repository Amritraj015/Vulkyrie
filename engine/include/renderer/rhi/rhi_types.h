#pragma once

#include "vlkypch.h"
#include "core/types/application_types.h"
#include "core/types/handle.h"

namespace Vulkyrie {

    inline constexpr u32 INVALID_RENDERER_INDEX = std::numeric_limits<u32>::max();

    // enum class IndexType : u8 { UInt8, UInt16, UInt32 };
    // enum class LoadOp : u8 { Load, Clear, DontCare };
    // enum class StoreOp : u8 { Store, DontCare, Resolve };

    enum class ShaderStage : u8 { Vertex, Fragment, Compute, Task, Mesh, RayGen, Miss, ClosestHit, AnyHit, Count };
    enum class CompareOp : u8 { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

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
    struct RendererStatistics {
        u64 FrameIndex = 0;
        u64 TransientBytesPeak = 0;
        u32 PassesDeclared = 0;
        u32 PassesCulled = 0;
        u32 BarrierBatches = 0;
        u32 DrawCalls = 0;
        u32 PipelineBinds = 0;
        f32 CpuRecordMs = 0.0f;
        f32 GraphCompileMs = 0.0f;
        f32 GpuFrameMs = 0.0f;
        bool PlanCacheHit = false;
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

    // -------------------------------------------------------------------

    struct DeviceCreationInfo {
        DeviceCreationInfo(WindowHandle windowHandle,
                           u32 surfaceWidth = 800,
                           u32 surfaceHeight = 600,
                           u32 maxBuffers = 65536,
                           u32 maxTextures = 262144,
                           u32 maxPipelines = 16384,
                           u32 maxSamplers = 256,
                           bool enableValidation = false,
                           bool preferDiscreteGpu = true,
                           bool enableVSync = false)
            : WindowHandle(windowHandle)
            , SurfaceWidth(surfaceWidth)
            , SurfaceHeight(surfaceHeight)
            , MaxBuffers(maxBuffers)
            , MaxTextures(maxTextures)
            , MaxPipelines(maxPipelines)
            , MaxSamplers(maxSamplers)
            , EnableValidation(enableValidation)
            , PreferDiscreteGpu(preferDiscreteGpu)
            , EnableVSync(enableVSync) {
        }

        WindowHandle WindowHandle;
        u32 SurfaceWidth = 800;
        u32 SurfaceHeight = 600;

        u32 MaxBuffers = 65536;
        u32 MaxTextures = 262144;
        u32 MaxPipelines = 16384;
        u32 MaxSamplers = 256;

        bool EnableValidation = false;
        bool PreferDiscreteGpu = true;
        bool EnableVSync = false;

        // u32 FramesInFlight = 2; // 2 for latency, 3 to hide a spiky CPU frame
        // bool EnableGpuValidation = false;
        // bool EnableCaptureSupport = false; // RenderDoc/PIX marker + capture hooks

        // Handle-table budgets. Fixed at init because SlotMap's lock-free reads
        // require non-reallocating storage; see core/SlotMap.h.
    };

} // namespace Vulkyrie
