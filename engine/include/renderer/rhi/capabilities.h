#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    struct DeviceCapabilities final {
        char DeviceName[256]{};
        char DriverInfo[256]{};
        u32 VendorId = 0;
        u32 DeviceId = 0;
        u64 DeviceLocalMemoryBytes = 0;
        u64 HostVisibleMemoryBytes = 0;
        u32 MaxTexture2DDim = 0;
        u32 MaxTextureArrayLayers = 0;
        u32 MaxColorAttachments = 0;
        u32 MaxBoundDescriptors = 0; // bindless heap capacity
        u32 MaxPushConstantBytes = 0;
        u32 MaxComputeWorkgroup[3]{};
        u64 MinUniformBufferAlign = 0;
        u64 MinStorageBufferAlign = 0;
        u64 OptimalBufferCopyAlign = 0;
        u32 TimestampValidBits = 0;
        f32 TimestampPeriodNs = 0.0f;

        // Descriptor indexing adequate for the bindless heap. On a backend with
        // kUsesBindlessHeap this is a PRECONDITION, not a branch: device selection
        // rejects adapters that report false, and Renderer::Create falls through
        // to the next backend.
        bool DescriptorIndexingSupported = false;

        // Genuinely per-device. Branch on these; never assume from the backend.
        bool HasDedicatedComputeQueue = false;
        bool HasDedicatedTransferQueue = false;
        bool SupportsIndirectCount = false;
        bool SupportsMeshShaders = false;
        bool SupportsDynamicRendering = false;
        bool SupportsHostQueryReset = false;

        // u64 VRAMBudgetBytes = 0;
        // u32 MaxBindlessTextures = 0;
        // u32 MaxBindlessBuffers = 0;
        // u32 SubgroupSize = 0;
        // f32 TimestampPeriodNs = 0.0f;
        // bool MeshShaders = false;
        // bool RayTracing = false;
        // bool AsyncCompute = false;
        // bool DrawIndirectCount = false; // load the draw count from a GPU buffer
        // bool ResizableBar = false;
        // bool Int16Shader = false;
        // bool WaveIntrinsics = false;
        // bool ConservativeRaster = false;
    };

} // namespace Vulkyrie
