#pragma once

#include "vlkypch.h"
#include "core/types/application_types.h"
#include "core/types/handle.h"

namespace Vulkyrie {

    inline constexpr u32 INVALID_RENDERER_INDEX = std::numeric_limits<u32>::max();

    // enum class IndexType : u8 { UInt8, UInt16, UInt32 };
    // enum class LoadOp : u8 { Load, Clear, DontCare };
    // enum class StoreOp : u8 { Store, DontCare, Resolve };

    enum class ShaderStage : u32 {
        None = 0,
        Vertex = BIT(0),
        TessellationControl = BIT(1),
        TessellationEvaluation = BIT(2),
        Geometry = BIT(3),
        Fragment = BIT(4),
        Compute = BIT(5),
        Task = BIT(6),
        Mesh = BIT(7),
        RayTracing = BIT(8),
    };

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
        SparseBinding,
        VideoEncode,
        VideoDecode,
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

    struct ShaderBlob {
        std::span<const std::byte> bytes;
        ShaderStage stage = ShaderStage::Compute;
        std::string_view entryPoint = "main";
        u64 contentHash = 0; // computed by the cooker, not at load time
    };

    // -------------------------------------------------------------------

    struct DeviceCreationInfo {
        DeviceCreationInfo(const ApplicationInfo &info,
                           WindowHandle windowHandle,
                           const GraphicsSettings &graphicsSettings = {},
                           u32 maxBuffers = 65536,
                           u32 maxTransientBuffers = 128,
                           u32 maxTextures = 262144,
                           u32 maxTransientTextures = 128,
                           u32 maxPipelines = 16384,
                           u32 maxSamplers = 256,
                           u32 workerCount = 1,
                           bool enableRendererValidation = true,
                           bool enableVSync = false)
            : ApplicationInfo(info)
            , WindowHandle(windowHandle)
            , GraphicsSettings(graphicsSettings)
            , MaxBuffers(maxBuffers)
            , MaxTransientBuffers(maxTransientBuffers)
            , MaxTextures(maxTextures)
            , MaxTransientTextures(maxTransientTextures)
            , MaxPipelines(maxPipelines)
            , MaxSamplers(maxSamplers)
            , WorkerCount(workerCount)
            , EnableRendererValidation(enableRendererValidation)
            , EnableVSync(enableVSync) {
        }

        ApplicationInfo ApplicationInfo;
        WindowHandle WindowHandle;
        GraphicsSettings GraphicsSettings;

        u32 MaxBuffers;
        u32 MaxTransientBuffers;
        u32 MaxTextures;
        u32 MaxTransientTextures;
        u32 MaxPipelines;
        u32 MaxSamplers;

        u32 WorkerCount;

        bool EnableRendererValidation;
        bool EnableVSync;
    };

} // namespace Vulkyrie
