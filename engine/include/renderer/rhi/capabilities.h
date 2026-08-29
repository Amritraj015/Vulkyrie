#pragma once

#include "renderer/rhi/formats.h"
#include "renderer/rhi/rhi_types.h"
#include "vlkypch.h"

namespace Vulkyrie {

    enum class DeviceType : u8 {
        Other,
        IntegratedGPU,
        DiscreteGPU,
        VirtualGPU,
        CPU,
    };

    enum class VendorID : u32 {
        Unknown,
        AMD,
        NVIDIA,
        Intel,
        Apple,
        ARM,
        Qualcomm,
        ImaginationTechnologies,
        Broadcom,
        Mesa,
    };
    //
    // enum class SubgroupOperation : u32 {
    //     None = 0,
    //     Basic = BIT(0),
    //     Vote = BIT(1),
    //     Arithmetic = BIT(2),
    //     Ballot = BIT(3),
    //     Shuffle = BIT(4),
    //     ShuffleRelative = BIT(5),
    //     Clustered = BIT(6),
    //     Quad = BIT(7),
    // };

    /** @brief Returns a display name for a device type.
     * @param deviceType The type to name.
     * @returns A static string; never empty. */
    [[nodiscard]] std::string_view DeviceTypeName(DeviceType deviceType);

    /** @brief Returns a display name for a hardware vendor.
     * @param vendor The vendor to name.
     * @returns A static string; never empty. */
    [[nodiscard]] std::string_view VendorName(VendorID vendor);

    struct DeviceIdentity final {
        char DeviceName[255] = {};
        char DriverInfo[512] = {};
        VendorID VendorID = VendorID::Unknown;
        u32 DeviceID = 0;
        u32 DriverID = 0;
        u32 ApiVersion = 0;
        u32 DriverVersion = 0;
        DeviceType DeviceType = DeviceType::Other;

        /** @brief Formats this identity as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };
    //
    // struct DeviceLimits final {
    //     u32 MaxTexture1DDim = 0;
    //     u32 MaxTexture2DDim = 0;
    //     u32 MaxTexture3DDim = 0;
    //     u32 MaxTextureCubeDim = 0;
    //     u32 MaxTextureArrayLayers = 0;
    //
    //     u32 MaxColorAttachments = 0;
    //     u32 MaxViewports = 0;
    //     u32 MaxViewportDimensions[2]{};
    //     u32 MaxFramebufferWidth = 0;
    //     u32 MaxFramebufferHeight = 0;
    //     u32 MaxFramebufferLayers = 0;
    //
    //     u32 MaxPushConstantBytes = 0;
    //
    //     u32 MaxComputeWorkgroupSize[3]{};
    //     u32 MaxComputeWorkgroupCount[3]{};
    //     u32 MaxComputeWorkgroupInvocations = 0;
    //
    //     u64 MinUniformBufferAlign = 0;
    //     u64 MinStorageBufferAlign = 0;
    //     u64 MinTexelBufferOffsetAlign = 0;
    //     u64 OptimalBufferCopyAlign = 0;
    //     u64 OptimalBufferCopyRowPitchAlign = 0;
    //     u64 BufferImageGranularity = 0;
    //     u64 NonCoherentAtomSize = 0;
    //
    //     u64 MaxUniformBufferRange = 0;
    //     u64 MaxStorageBufferRange = 0;
    //     u32 MaxTexelBufferElements = 0;
    //
    //     u32 MaxVertexInputAttributes = 0;
    //     u32 MaxVertexInputBindings = 0;
    //     u32 MaxVertexInputAttributeOffset = 0;
    //     u32 MaxVertexInputBindingStride = 0;
    //
    //     f32 MaxSamplerAnisotropy = 0.0f;
    //     u32 MaxSamplerAllocationCount = 0;
    //
    //     SampleCount ColorSampleCounts = SampleCount::None;
    //     SampleCount DepthSampleCounts = SampleCount::None;
    //
    //     f32 TimestampPeriodNs = 0.0f;
    // };
    //
    // struct DeviceMemoryCapabilities final {
    //     u64 DeviceLocalBytes = 0;
    //     u64 HostVisibleBytes = 0;
    //     u64 DeviceLocalBudgetBytes = 0;
    //     u64 MaxAllocationBytes = 0;
    //
    //     // Budget tracking is available and DeviceLocalBudgetBytes is live rather
    //     // than a static heap size.
    //     bool HasBudgetTracking = false;
    //
    //     // Device-local and host-visible name the same physical memory.
    //     bool UnifiedMemory = false;
    // };
    //
    // struct DeviceQueueCapabilities final {
    //     // A compute/transfer queue exists on a family distinct from graphics.
    //     // Whether work actually overlaps is implementation-defined and must be
    //     // measured, not queried.
    //     bool HasAsyncCompute = false;
    //     bool HasAsyncTransfer = false;
    //     bool HasTimestampsOnAsyncQueues = false;
    // };
    //
    // struct DeviceDescriptorCapabilities final {
    //     bool Indexing = false;
    //     bool RuntimeArray = false;
    //     bool PartiallyBound = false;
    //     bool VariableCount = false;
    //     bool UpdateUnusedWhilePending = false;
    //
    //     bool UpdateAfterBindSampledImages = false;
    //     bool UpdateAfterBindStorageImages = false;
    //     bool UpdateAfterBindStorageBuffers = false;
    //     bool UpdateAfterBindUniformBuffers = false;
    //     bool UpdateAfterBindSamplers = false;
    //
    //     bool SampledImageNonUniformIndexing = false;
    //     bool StorageImageNonUniformIndexing = false;
    //     bool StorageBufferNonUniformIndexing = false;
    //     bool UniformBufferNonUniformIndexing = false;
    //
    //     u32 MaxBindlessTextures = 0;
    //     u32 MaxBindlessStorageImages = 0;
    //     u32 MaxBindlessBuffers = 0;
    //     u32 MaxBindlessSamplers = 0;
    // };
    //
    // struct DeviceSubgroupCapabilities final {
    //     u32 Size = 0;
    //     u32 MinSize = 0;
    //     u32 MaxSize = 0;
    //
    //     ShaderStage SupportedStages = ShaderStage::None;
    //     SubgroupOperation SupportedOperations = SubgroupOperation::None;
    //
    //     bool SizeControl = false;
    //     bool FullSubgroups = false;
    // };
    //
    // struct DeviceMeshShaderCapabilities final {
    //     bool Supported = false;
    //     bool TaskShader = false;
    //
    //     u32 MaxMeshWorkgroupInvocations = 0;
    //     u32 MaxMeshWorkgroupSize[3]{};
    //     u32 MaxMeshWorkgroupCount[3]{};
    //     u32 MaxMeshOutputVertices = 0;
    //     u32 MaxMeshOutputPrimitives = 0;
    //     u32 MaxMeshSharedMemoryBytes = 0;
    //
    //     u32 MaxTaskWorkgroupInvocations = 0;
    //     u32 MaxTaskWorkgroupSize[3]{};
    //     u32 MaxTaskWorkgroupCount[3]{};
    //     u32 MaxTaskPayloadBytes = 0;
    //
    //     u32 PreferredMeshWorkgroupInvocations = 0;
    //     u32 PreferredTaskWorkgroupInvocations = 0;
    //     bool PrefersLocalInvocationVertexOutput = false;
    //     bool PrefersCompactVertexOutput = false;
    // };
    //
    // struct DeviceRayTracingCapabilities final {
    //     bool AccelerationStructure = false;
    //     bool Pipeline = false;
    //     bool RayQuery = false;
    //
    //     u32 MaxRecursionDepth = 0;
    //     u32 ShaderGroupHandleBytes = 0;
    //     u32 ShaderGroupHandleAlign = 0;
    //     u32 ShaderGroupBaseAlign = 0;
    //
    //     u32 MaxDispatchInvocations = 0;
    //     u64 MaxDispatchDim[3]{};
    // };
    //
    // struct DeviceFeaturesSet final {
    //     bool SamplerAnisotropy = false;
    //     bool IndependentBlend = false;
    //     bool MultiDrawIndirect = false;
    //     bool DrawIndirectCount = false;
    //     bool TextureCompressionBC = false;
    //     bool PipelineStatisticsQuery = false;
    //     bool FragmentStoresAndAtomics = false;
    //     bool ShaderStorageImageMultisample = false;
    //
    //     bool ShaderInt8 = false;
    //     bool ShaderInt16 = false;
    //     bool ShaderInt64 = false;
    //     bool ShaderFloat16 = false;
    //     bool ShaderFloat64 = false;
    //
    //     bool Bindless = false;
    //     bool TimelineSemaphore = false;
    //     bool BufferDeviceAddress = false;
    //     bool ScalarBlockLayout = false;
    //     bool HostQueryReset = false;
    //     bool DynamicRendering = false;
    //     bool ExplicitBarriers = false;
    //
    //     bool MeshShader = false;
    //     bool RayTracing = false;
    // };
    //
    struct DeviceCapabilities final {
        DeviceIdentity Identity{};

        /** @brief Formats the whole capability set as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;

        // DeviceLimits Limits;
        // DeviceMeshShaderCapabilities MeshShaders;
        // DeviceRayTracingCapabilities RayTracing;
        // DeviceMemoryCapabilities Memory;
        // DeviceDescriptorCapabilities Descriptors;
        // DeviceSubgroupCapabilities Subgroups;
        // DeviceFeaturesSet Features;
        // DeviceQueueCapabilities Queues;
    };

} // namespace Vulkyrie
