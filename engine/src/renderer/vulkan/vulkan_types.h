#pragma once

#include "memory/allocators/tracked_std_allocator.h"
#include "renderer/rhi/capabilities.h"
#include <limits>
#include <volk.h>

namespace Vulkyrie {

    template <typename T> using RendererVector = TrackedVector<T, MemoryTag::Rendering>;

    struct VulkanImage {};
    struct VulkanBuffer {};
    struct VulkanSampler {};
    struct VulkanPipeline {};
    struct VulkanShaderModule {};

    struct ValidationConfig final {
        // Core correctness — cheap, on by default
        VkBool32 Core = VK_TRUE;         // validate_core
        VkBool32 ThreadSafety = VK_TRUE; // thread_safety — separate cost, exercises parallel command recording

        // Synchronization — moderate cost, directly relevant to frame graph barrier work
        VkBool32 Sync = VK_FALSE;           // validate_sync
        VkBool32 SyncSubmitTime = VK_FALSE; // syncval_submit_time_validation — sub-option, extra cost

        // GPU-assisted — heaviest cost, opt-in only
        VkBool32 GpuAV = VK_FALSE;                 // gpuav_enable
        VkBool32 GpuAVSelectiveShaders = VK_FALSE; // gpuav_select_instrumented_shaders — scope cost to specific shaders

        // Best practices — cheap, informational
        VkBool32 BestPractices = VK_FALSE; // validate_best_practices

        // Shader debug printf — mutually exclusive with gpuav (layer-enforced)
        VkBool32 DebugPrintf = VK_FALSE;
    };

    // -----------------------------------------------------------------------------
    // Physical-device identity
    // -----------------------------------------------------------------------------

    // struct VulkanDeviceIdentity final {
    //     char DeviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE]{};
    //     char DriverInfo[VK_MAX_DRIVER_NAME_SIZE + VK_MAX_DRIVER_INFO_SIZE]{};
    //
    //     u32 VendorID = 0;
    //     u32 DeviceID = 0;
    //     u32 ApiVersion = VK_API_VERSION_1_0;
    //     u32 DriverVersion = 0;
    //
    //     VkDriverId DriverId = VkDriverId{};
    //     VkConformanceVersion ConformanceVersion{};
    //     VkPhysicalDeviceType Type = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    // };

    // -----------------------------------------------------------------------------
    // Memory topology
    // -----------------------------------------------------------------------------

    struct VulkanMemoryHeap final {
        u64 Size = 0;
        u64 Budget = 0;
        u64 Usage = 0;
        VkMemoryHeapFlags Flags = 0;

        /** @brief Formats this heap as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    struct VulkanMemoryType final {
        u32 HeapIndex = 0;
        VkMemoryPropertyFlags Properties = 0;

        /** @brief Formats this memory type as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    struct VulkanDeviceMemoryCapabilities final {
        RendererVector<VulkanMemoryHeap> Heaps;
        RendererVector<VulkanMemoryType> Types;

        u64 DeviceLocalBytes = 0;
        u64 HostVisibleBytes = 0;
        u64 DeviceLocalBudgetBytes = 0;

        // Budget/Usage are transient. Re-query per frame for residency decisions.
        bool HasMemoryBudget = false;

        /** @brief Formats the memory topology, including every heap and type, as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Queue topology
    // -----------------------------------------------------------------------------

    struct VulkanQueueFamilyCapabilities final {
        u32 FamilyIndex = 0;
        u32 QueueCount = 0;
        u32 TimestampValidBits = 0;
        VkQueueFlags Flags = 0;

        bool SupportsGraphics = false;
        bool SupportsCompute = false;
        bool SupportsTransfer = false;
        bool SupportsSparseBinding = false;
        bool SupportsPresent = false;

        /** @brief Formats this queue family as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    inline constexpr u32 kInvalidQueueFamilyIndex = std::numeric_limits<u32>::max();
    inline constexpr u32 kInvalidQueueIndex = std::numeric_limits<u32>::max();

    struct VulkanDeviceQueueCapabilities final {
        RendererVector<VulkanQueueFamilyCapabilities> Families;

        u32 GraphicsFamily = kInvalidQueueFamilyIndex;
        u32 ComputeFamily = kInvalidQueueFamilyIndex;
        u32 TransferFamily = kInvalidQueueFamilyIndex;
        u32 PresentFamily = kInvalidQueueFamilyIndex;

        bool HasDedicatedComputeQueue = false;
        bool HasDedicatedTransferQueue = false;

        // "Async" here means a distinct queue family, not a guarantee of
        // concurrent execution -- that is implementation-defined and must be
        // measured, not queried.
        bool SupportsAsyncCompute = false;
        bool SupportsAsyncTransfer = false;

        /** @brief Formats the queue topology, including every family, as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Device limits
    // -----------------------------------------------------------------------------

    struct VulkanDeviceLimits final {
        u32 MaxTexture1DDim = 0;
        u32 MaxTexture2DDim = 0;
        u32 MaxTexture3DDim = 0;
        u32 MaxTextureCubeDim = 0;
        u32 MaxTextureArrayLayers = 0;

        u32 MaxColorAttachments = 0;
        u32 MaxViewports = 0;
        u32 MaxViewportDimensions[2]{};
        u32 MaxFramebufferWidth = 0;
        u32 MaxFramebufferHeight = 0;
        u32 MaxFramebufferLayers = 0;

        u32 MaxPushConstantBytes = 0;

        u32 MaxComputeWorkgroupSize[3]{};
        u32 MaxComputeWorkgroupCount[3]{};
        u32 MaxComputeWorkgroupInvocations = 0;

        u64 MinUniformBufferAlign = 0;
        u64 MinStorageBufferAlign = 0;
        u64 MinTexelBufferOffsetAlign = 0;
        u64 OptimalBufferCopyAlign = 0;
        u64 OptimalBufferCopyRowPitchAlign = 0;
        u64 MinMemoryMapAlign = 0;
        u64 NonCoherentAtomSize = 0;

        // Needed to compute safe offsets when aliasing buffers and images in one
        // allocation, e.g. the frame graph's transient resource pool.
        u64 BufferImageGranularity = 0;

        f32 MaxSamplerAnisotropy = 0.0f;
        u32 MaxSamplerAllocationCount = 0;

        u64 MaxUniformBufferRange = 0;
        u64 MaxStorageBufferRange = 0;
        u64 MaxMemoryAllocationSize = 0;
        u32 MaxTexelBufferElements = 0;

        u32 MaxVertexInputAttributes = 0;
        u32 MaxVertexInputBindings = 0;
        u32 MaxVertexInputAttributeOffset = 0;
        u32 MaxVertexInputBindingStride = 0;

        VkSampleCountFlags FramebufferColorSampleCounts = 0;
        VkSampleCountFlags FramebufferDepthSampleCounts = 0;

        // Nanoseconds per timestamp tick. Valid-bit counts are per queue family.
        f32 TimestampPeriodNs = 0.0f;
        bool TimestampComputeAndGraphics = false;

        /** @brief Formats the device limits as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Descriptor/bindless limits
    // -----------------------------------------------------------------------------

    struct VulkanDeviceDescriptorCapabilities final {
        // Individual descriptor-indexing bits. A heap that only needs bindless
        // sampled images should test those bits rather than the aggregate.
        bool DescriptorIndexing = false;
        bool RuntimeDescriptorArray = false;
        bool PartiallyBound = false;
        bool VariableDescriptorCount = false;
        bool UpdateUnusedWhilePending = false;

        bool UpdateAfterBindSampledImages = false;
        bool UpdateAfterBindStorageImages = false;
        bool UpdateAfterBindStorageBuffers = false;
        bool UpdateAfterBindUniformBuffers = false;
        bool UpdateAfterBindSamplers = false;

        bool SampledImageNonUniformIndexing = false;
        bool StorageImageNonUniformIndexing = false;
        bool StorageBufferNonUniformIndexing = false;
        bool UniformBufferNonUniformIndexing = false;

        u32 MaxUpdateAfterBindDescriptors = 0;
        u32 MaxUpdateAfterBindSampledImages = 0;
        u32 MaxUpdateAfterBindStorageImages = 0;
        u32 MaxUpdateAfterBindUniformBuffers = 0;
        u32 MaxUpdateAfterBindStorageBuffers = 0;
        u32 MaxUpdateAfterBindSamplers = 0;

        u32 MaxPerStageUpdateAfterBindSamplers = 0;
        u32 MaxPerStageUpdateAfterBindSampledImages = 0;
        u32 MaxPerStageUpdateAfterBindStorageImages = 0;
        u32 MaxPerStageUpdateAfterBindUniformBuffers = 0;
        u32 MaxPerStageUpdateAfterBindStorageBuffers = 0;
        u32 MaxPerStageUpdateAfterBindResources = 0;

        // min(per-set, per-stage). The per-stage cap is the binding one on several
        // drivers, so a per-set-only heap size is not achievable in a shader.
        u32 MaxBindlessTextures = 0;
        u32 MaxBindlessStorageImages = 0;
        u32 MaxBindlessBuffers = 0;
        u32 MaxBindlessSamplers = 0;

        /** @brief Formats the descriptor and bindless limits as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Subgroups
    // -----------------------------------------------------------------------------

    struct VulkanDeviceSubgroupCapabilities final {
        u32 Size = 0;
        u32 MinSize = 0;
        u32 MaxSize = 0;
        VkShaderStageFlags SupportedStages = 0;
        VkSubgroupFeatureFlags SupportedOperations = 0;

        /** @brief Formats the subgroup capabilities as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Mesh shader
    // -----------------------------------------------------------------------------

    struct VulkanDeviceMeshShaderCapabilities final {
        bool Supported = false;
        bool TaskShader = false;
        bool MultiviewMeshShader = false;

        u32 MaxMeshWorkgroupInvocations = 0;
        u32 MaxMeshWorkgroupSize[3]{};
        u32 MaxMeshWorkgroupCount[3]{};
        u32 MaxMeshOutputVertices = 0;
        u32 MaxMeshOutputPrimitives = 0;
        u32 MaxMeshSharedMemorySize = 0;

        u32 MaxTaskWorkgroupInvocations = 0;
        u32 MaxTaskWorkgroupSize[3]{};
        u32 MaxTaskWorkgroupCount[3]{};
        u32 MaxTaskPayloadSize = 0;

        u32 MaxPreferredMeshWorkgroupInvocations = 0;
        u32 MaxPreferredTaskWorkgroupInvocations = 0;
        bool PrefersLocalInvocationVertexOutput = false;
        bool PrefersCompactVertexOutput = false;

        /** @brief Formats the mesh shader capabilities as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Ray tracing
    // -----------------------------------------------------------------------------

    struct VulkanDeviceRayTracingCapabilities final {
        bool AccelerationStructure = false;
        bool Pipeline = false;
        bool RayQuery = false;

        u32 MaxRecursionDepth = 0;
        u32 ShaderGroupHandleSize = 0;
        u32 ShaderGroupHandleAlignment = 0;
        u32 ShaderGroupBaseAlignment = 0;

        // width * height * depth passed to vkCmdTraceRaysKHR must not exceed this.
        // Per-dimension caps derive from the compute workgroup limits.
        u32 MaxRayDispatchInvocations = 0;
        u64 MaxRayDispatchDim[3]{};

        /** @brief Formats the ray tracing capabilities as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Feature set
    // -----------------------------------------------------------------------------

    struct VulkanDeviceFeatureSet final {
        bool SamplerAnisotropy = false;
        bool IndependentBlend = false;
        bool MultiDrawIndirect = false;
        bool ShaderInt16 = false;
        bool ShaderInt64 = false;
        bool ShaderFloat64 = false;
        bool FragmentStoresAndAtomics = false;
        bool ShaderStorageImageMultisample = false;
        bool TextureCompressionBC = false;
        bool PipelineStatisticsQuery = false;

        bool ShaderInt8 = false;
        bool ShaderFloat16 = false;
        bool DrawIndirectCount = false;
        bool HostQueryReset = false;
        bool TimelineSemaphore = false;
        bool BufferDeviceAddress = false;
        bool ScalarBlockLayout = false;
        bool SamplerFilterMinmax = false;
        bool StorageBuffer8BitAccess = false;
        bool VulkanMemoryModel = false;

        bool DynamicRendering = false;
        bool Synchronization2 = false;
        bool Maintenance4 = false;
        bool ShaderDemoteToHelperInvocation = false;
        bool SubgroupSizeControl = false;
        bool ComputeFullSubgroups = false;

        // Every bit the bindless heap actually depends on, ANDed together.
        bool Bindless = false;

        bool MeshShader = false;
        bool RayTracingAccelerationStructure = false;
        bool RayTracingPipeline = false;
        bool RayQuery = false;
        bool ExtendedDynamicState3 = false;

        /** @brief Formats the feature set as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;
    };

    // -----------------------------------------------------------------------------
    // Complete normalized capability set
    // -----------------------------------------------------------------------------

    struct VulkanDeviceCapabilities final {
        DeviceIdentity Identity;
        VulkanDeviceLimits Limits;
        VulkanDeviceMeshShaderCapabilities MeshShaders;
        VulkanDeviceMemoryCapabilities Memory;
        VulkanDeviceDescriptorCapabilities Descriptors;
        VulkanDeviceQueueCapabilities Queues;
        VulkanDeviceRayTracingCapabilities RayTracing;
        VulkanDeviceFeatureSet Features;
        VulkanDeviceSubgroupCapabilities Subgroups;

        // Owns the name storage. Do not replace with RendererVector<const char*>
        // pointing at a temporary enumeration buffer.
        RendererVector<VkExtensionProperties> Extensions;

        // min(instance apiVersion, device apiVersion).
        u32 EffectiveApiVersion = VK_API_VERSION_1_0;

        [[nodiscard]] bool HasExtension(const char *name) const {
            for (const auto &extension : Extensions) {
                if (std::strcmp(extension.extensionName, name) == 0) return true;
            }

            return false;
        }

        /** @brief Formats every section of this capability set as a human-readable block.
         * @returns One `key: value` pair per line, with no trailing newline. */
        [[nodiscard]] std::string ToString() const;

        /** @brief Projects the backend-specific fields onto the backend-agnostic `DeviceCapabilities`.
         *
         * Must run before `ToDeviceCapabilities`, which only hands back what this wrote. */
        void Normalize();

        [[nodiscard]] const DeviceCapabilities &ToDeviceCapabilities() const {
            return mCapabilities;
        }

    private:
        DeviceCapabilities mCapabilities;
    };

    struct DeviceRequirements final {
        u32 MinimumApiVersion = VK_API_VERSION_1_3;

        bool RequireBindless = true;
        bool RequireDynamicRendering = true;
        bool RequireSynchronization2 = true;
        bool RequireIndirectCount = true;
        bool RequireTimelineSemaphore = true;
        bool RequireBufferDeviceAddress = true;
        bool RequirePresent = true;

        u32 MinimumBindlessTextures = 65536;
        u32 MinimumBindlessBuffers = 65536;

        // Checked during selection and passed verbatim to
        // VkDeviceCreateInfo::ppEnabledExtensionNames, so the two cannot drift.
        // A missing entry rejects the candidate here instead of failing
        // vkCreateDevice with VK_ERROR_EXTENSION_NOT_PRESENT after the other
        // candidates have already been discarded.
        //
        // VK_KHR_swapchain is a device extension and is not implied by
        // vkGetPhysicalDeviceSurfaceSupportKHR, which comes from the
        // VK_KHR_surface instance extension. Clear this list for headless.
        static constexpr std::array<const char *, 1> REQUIRED_EXTENSIONS{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

} // namespace Vulkyrie
