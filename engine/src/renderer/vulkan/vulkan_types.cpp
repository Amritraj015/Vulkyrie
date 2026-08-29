#include "renderer/vulkan/vulkan_types.h"

namespace Vulkyrie {

    namespace {

        struct FlagName final {
            u32 Bit;
            std::string_view Name;
        };

        // Decodes a VkFlags bitmask. Bits the table does not name are still reported, as a
        // trailing hex remainder, so a log never silently drops a capability.
        std::string decodeFlags(u32 flags, std::span<const FlagName> names) {
            if (0 == flags) return "None";

            std::string decoded;
            u32 remaining = flags;

            const auto append = [&decoded](std::string_view text) {
                if (!decoded.empty()) decoded += " | ";

                decoded += text;
            };

            for (const FlagName &entry : names) {
                if (0 == (flags & entry.Bit)) continue;

                append(entry.Name);
                remaining &= ~entry.Bit;
            }

            if (0 != remaining) append(std::format("0x{:X}", remaining));

            return decoded;
        }

        constexpr FlagName kQueueFlagNames[] = {
            { VK_QUEUE_GRAPHICS_BIT, "Graphics" },
            { VK_QUEUE_COMPUTE_BIT, "Compute" },
            { VK_QUEUE_TRANSFER_BIT, "Transfer" },
            { VK_QUEUE_SPARSE_BINDING_BIT, "SparseBinding" },
            { VK_QUEUE_PROTECTED_BIT, "Protected" },
            { VK_QUEUE_VIDEO_DECODE_BIT_KHR, "VideoDecode" },
            { VK_QUEUE_VIDEO_ENCODE_BIT_KHR, "VideoEncode" },
            { VK_QUEUE_OPTICAL_FLOW_BIT_NV, "OpticalFlow" },
        };

        constexpr FlagName kMemoryHeapFlagNames[] = {
            { VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, "DeviceLocal" },
            { VK_MEMORY_HEAP_MULTI_INSTANCE_BIT, "MultiInstance" },
        };

        constexpr FlagName kMemoryPropertyFlagNames[] = {
            { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "DeviceLocal" },
            { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "HostVisible" },
            { VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "HostCoherent" },
            { VK_MEMORY_PROPERTY_HOST_CACHED_BIT, "HostCached" },
            { VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, "LazilyAllocated" },
            { VK_MEMORY_PROPERTY_PROTECTED_BIT, "Protected" },
            { VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD, "DeviceCoherentAMD" },
            { VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD, "DeviceUncachedAMD" },
            { VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV, "RdmaCapableNV" },
        };

        constexpr FlagName kShaderStageFlagNames[] = {
            { VK_SHADER_STAGE_VERTEX_BIT, "Vertex" },
            { VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TessControl" },
            { VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TessEval" },
            { VK_SHADER_STAGE_GEOMETRY_BIT, "Geometry" },
            { VK_SHADER_STAGE_FRAGMENT_BIT, "Fragment" },
            { VK_SHADER_STAGE_COMPUTE_BIT, "Compute" },
            { VK_SHADER_STAGE_TASK_BIT_EXT, "Task" },
            { VK_SHADER_STAGE_MESH_BIT_EXT, "Mesh" },
            { VK_SHADER_STAGE_RAYGEN_BIT_KHR, "RayGen" },
            { VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "AnyHit" },
            { VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "ClosestHit" },
            { VK_SHADER_STAGE_MISS_BIT_KHR, "Miss" },
            { VK_SHADER_STAGE_INTERSECTION_BIT_KHR, "Intersection" },
            { VK_SHADER_STAGE_CALLABLE_BIT_KHR, "Callable" },
        };

        constexpr FlagName kSubgroupFeatureFlagNames[] = {
            { VK_SUBGROUP_FEATURE_BASIC_BIT, "Basic" },
            { VK_SUBGROUP_FEATURE_VOTE_BIT, "Vote" },
            { VK_SUBGROUP_FEATURE_ARITHMETIC_BIT, "Arithmetic" },
            { VK_SUBGROUP_FEATURE_BALLOT_BIT, "Ballot" },
            { VK_SUBGROUP_FEATURE_SHUFFLE_BIT, "Shuffle" },
            { VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT, "ShuffleRelative" },
            { VK_SUBGROUP_FEATURE_CLUSTERED_BIT, "Clustered" },
            { VK_SUBGROUP_FEATURE_QUAD_BIT, "Quad" },
            { VK_SUBGROUP_FEATURE_ROTATE_BIT, "Rotate" },
            { VK_SUBGROUP_FEATURE_ROTATE_CLUSTERED_BIT, "RotateClustered" },
            { VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV, "PartitionedNV" },
        };

        constexpr FlagName kSampleCountFlagNames[] = {
            { VK_SAMPLE_COUNT_1_BIT, "1" },   { VK_SAMPLE_COUNT_2_BIT, "2" },   { VK_SAMPLE_COUNT_4_BIT, "4" },  { VK_SAMPLE_COUNT_8_BIT, "8" },
            { VK_SAMPLE_COUNT_16_BIT, "16" }, { VK_SAMPLE_COUNT_32_BIT, "32" }, { VK_SAMPLE_COUNT_64_BIT, "64" },
        };

        std::string formatBytes(u64 bytes) {
            constexpr std::string_view units[] = { "B", "KiB", "MiB", "GiB", "TiB" };

            auto scaled = static_cast<f64>(bytes);
            usize unit = 0;

            while (scaled >= 1024.0 && unit + 1 < std::size(units)) {
                scaled /= 1024.0;
                ++unit;
            }

            return 0 == unit ? std::format("{} B", bytes) : std::format("{:.2f} {}", scaled, units[unit]);
        }

        std::string formatQueueFamily(u32 family) {
            return kInvalidQueueFamily == family ? std::string("none") : std::format("{}", family);
        }

        template <typename T> std::string formatTriple(const T (&values)[3]) {
            return std::format("[{}, {}, {}]", values[0], values[1], values[2]);
        }

        // Indents an already-formatted nested block so a list of heaps, types or queue families
        // stays visually attached to its parent section.
        std::string indent(const std::string &block) {
            std::string indented = "  ";

            for (const char character : block) {
                indented += character;

                if ('\n' == character) indented += "  ";
            }

            return indented;
        }

    } // namespace

    std::string VulkanMemoryHeap::ToString() const {
        return std::format("size {}, budget {}, usage {}, flags {}",
                           formatBytes(Size),
                           formatBytes(Budget),
                           formatBytes(Usage),
                           decodeFlags(Flags, kMemoryHeapFlagNames));
    }

    std::string VulkanMemoryType::ToString() const {
        return std::format("heap {}, properties {}", HeapIndex, decodeFlags(Properties, kMemoryPropertyFlagNames));
    }

    std::string VulkanDeviceMemoryCapabilities::ToString() const {
        std::string text = "Memory:\n";

        text += std::format("  Device local   : {} ({} budget)\n", formatBytes(DeviceLocalBytes), formatBytes(DeviceLocalBudgetBytes));
        text += std::format("  Host visible   : {}\n", formatBytes(HostVisibleBytes));
        text += std::format("  Budget queries : {}\n", HasMemoryBudget);

        for (usize i = 0; i < Heaps.size(); ++i) {
            text += indent(std::format("Heap {:<2}        : {}\n", i, Heaps[i].ToString()));
        }

        for (usize i = 0; i < Types.size(); ++i) {
            text += indent(std::format("Type {:<2}        : {}\n", i, Types[i].ToString()));
        }

        // The loops above always leave a trailing newline behind; heaps and types are never empty
        // on a device that reached this point.
        if (!text.empty() && '\n' == text.back()) text.pop_back();

        return text;
    }

    std::string VulkanQueueFamilyCapabilities::ToString() const {
        return std::format("{} queue(s), {} timestamp bits, flags {}{}",
                           QueueCount,
                           TimestampValidBits,
                           decodeFlags(Flags, kQueueFlagNames),
                           SupportsPresent ? " | Present" : "");
    }

    std::string VulkanDeviceQueueCapabilities::ToString() const {
        std::string text = "Queues:\n";

        text += std::format("  Graphics family: {}\n", formatQueueFamily(GraphicsFamily));
        text += std::format("  Compute family : {} (dedicated {})\n", formatQueueFamily(ComputeFamily), HasDedicatedComputeQueue);
        text += std::format("  Transfer family: {} (dedicated {})\n", formatQueueFamily(TransferFamily), HasDedicatedTransferQueue);
        text += std::format("  Present family : {}\n", formatQueueFamily(PresentFamily));
        text += std::format("  Async          : compute {}, transfer {}\n", SupportsAsyncCompute, SupportsAsyncTransfer);

        for (const VulkanQueueFamilyCapabilities &family : Families) {
            text += indent(std::format("Family {:<2}      : {}\n", family.FamilyIndex, family.ToString()));
        }

        if (!text.empty() && '\n' == text.back()) text.pop_back();

        return text;
    }

    std::string VulkanDeviceLimits::ToString() const {
        std::string text = "Limits:\n";

        text += std::format("  Texture dims   : 1D {}, 2D {}, 3D {}, cube {}, {} array layers\n",
                            MaxTexture1DDim,
                            MaxTexture2DDim,
                            MaxTexture3DDim,
                            MaxTextureCubeDim,
                            MaxTextureArrayLayers);

        text += std::format("  Framebuffer    : {} x {} x {} layers, {} color attachments\n",
                            MaxFramebufferWidth,
                            MaxFramebufferHeight,
                            MaxFramebufferLayers,
                            MaxColorAttachments);

        text += std::format("  Viewports      : {}, max dimensions [{}, {}]\n", MaxViewports, MaxViewportDimensions[0], MaxViewportDimensions[1]);
        text += std::format("  Push constants : {} B\n", MaxPushConstantBytes);

        text += std::format("  Compute groups : size {}, count {}, {} invocations\n",
                            formatTriple(MaxComputeWorkgroupSize),
                            formatTriple(MaxComputeWorkgroupCount),
                            MaxComputeWorkgroupInvocations);

        text += std::format("  Buffer align   : uniform {} B, storage {} B, texel {} B\n",
                            MinUniformBufferAlign,
                            MinStorageBufferAlign,
                            MinTexelBufferOffsetAlign);

        text += std::format("  Copy align     : offset {} B, row pitch {} B, memory map {} B, non-coherent atom {} B\n",
                            OptimalBufferCopyAlign,
                            OptimalBufferCopyRowPitchAlign,
                            MinMemoryMapAlign,
                            NonCoherentAtomSize);

        text += std::format("  Buffer/image granularity: {} B\n", BufferImageGranularity);

        text += std::format("  Ranges         : uniform {}, storage {}, {} texel elements, {} max allocation\n",
                            formatBytes(MaxUniformBufferRange),
                            formatBytes(MaxStorageBufferRange),
                            MaxTexelBufferElements,
                            formatBytes(MaxMemoryAllocationSize));

        text += std::format("  Vertex input   : {} attributes, {} bindings, max offset {}, max stride {}\n",
                            MaxVertexInputAttributes,
                            MaxVertexInputBindings,
                            MaxVertexInputAttributeOffset,
                            MaxVertexInputBindingStride);

        text += std::format("  Samplers       : max anisotropy {:.1f}, {} allocations\n", MaxSamplerAnisotropy, MaxSamplerAllocationCount);

        text += std::format("  Sample counts  : color {}, depth {}\n",
                            decodeFlags(FramebufferColorSampleCounts, kSampleCountFlagNames),
                            decodeFlags(FramebufferDepthSampleCounts, kSampleCountFlagNames));

        text += std::format("  Timestamps     : {:.2f} ns/tick, compute+graphics {}", TimestampPeriodNs, TimestampComputeAndGraphics);

        return text;
    }

    std::string VulkanDeviceDescriptorCapabilities::ToString() const {
        std::string text = "Descriptors:\n";

        text += std::format("  Indexing       : indexing {}, runtime array {}, partially bound {}, variable count {}, update unused {}\n",
                            DescriptorIndexing,
                            RuntimeDescriptorArray,
                            PartiallyBound,
                            VariableDescriptorCount,
                            UpdateUnusedWhilePending);

        text += std::format("  Update after bind: sampled {}, storage image {}, storage buffer {}, uniform {}, sampler {}\n",
                            UpdateAfterBindSampledImages,
                            UpdateAfterBindStorageImages,
                            UpdateAfterBindStorageBuffers,
                            UpdateAfterBindUniformBuffers,
                            UpdateAfterBindSamplers);

        text += std::format("  Non-uniform    : sampled {}, storage image {}, storage buffer {}, uniform buffer {}\n",
                            SampledImageNonUniformIndexing,
                            StorageImageNonUniformIndexing,
                            StorageBufferNonUniformIndexing,
                            UniformBufferNonUniformIndexing);

        text += std::format("  Per-set caps   : {} total, sampled {}, storage image {}, uniform {}, storage buffer {}, sampler {}\n",
                            MaxUpdateAfterBindDescriptors,
                            MaxUpdateAfterBindSampledImages,
                            MaxUpdateAfterBindStorageImages,
                            MaxUpdateAfterBindUniformBuffers,
                            MaxUpdateAfterBindStorageBuffers,
                            MaxUpdateAfterBindSamplers);

        text += std::format("  Per-stage caps : {} resources, sampled {}, storage image {}, uniform {}, storage buffer {}, sampler {}\n",
                            MaxPerStageUpdateAfterBindResources,
                            MaxPerStageUpdateAfterBindSampledImages,
                            MaxPerStageUpdateAfterBindStorageImages,
                            MaxPerStageUpdateAfterBindUniformBuffers,
                            MaxPerStageUpdateAfterBindStorageBuffers,
                            MaxPerStageUpdateAfterBindSamplers);

        text += std::format("  Bindless heap  : {} textures, {} storage images, {} buffers, {} samplers",
                            MaxBindlessTextures,
                            MaxBindlessStorageImages,
                            MaxBindlessBuffers,
                            MaxBindlessSamplers);

        return text;
    }

    std::string VulkanDeviceSubgroupCapabilities::ToString() const {
        std::string text = "Subgroups:\n";

        text += std::format("  Size           : {} (min {}, max {})\n", Size, MinSize, MaxSize);
        text += std::format("  Stages         : {}\n", decodeFlags(SupportedStages, kShaderStageFlagNames));
        text += std::format("  Operations     : {}", decodeFlags(SupportedOperations, kSubgroupFeatureFlagNames));

        return text;
    }

    std::string VulkanDeviceMeshShaderCapabilities::ToString() const {
        std::string text = "Mesh shaders:\n";

        text += std::format("  Supported      : {} (task {}, multiview {})\n", Supported, TaskShader, MultiviewMeshShader);

        if (!Supported) return text.substr(0, text.size() - 1);

        text += std::format("  Mesh groups    : size {}, count {}, {} invocations\n",
                            formatTriple(MaxMeshWorkgroupSize),
                            formatTriple(MaxMeshWorkgroupCount),
                            MaxMeshWorkgroupInvocations);

        text += std::format("  Mesh output    : {} vertices, {} primitives, {} shared memory\n",
                            MaxMeshOutputVertices,
                            MaxMeshOutputPrimitives,
                            formatBytes(MaxMeshSharedMemorySize));

        text += std::format("  Task groups    : size {}, count {}, {} invocations, {} payload\n",
                            formatTriple(MaxTaskWorkgroupSize),
                            formatTriple(MaxTaskWorkgroupCount),
                            MaxTaskWorkgroupInvocations,
                            formatBytes(MaxTaskPayloadSize));

        text += std::format("  Preferred      : mesh {} invocations, task {} invocations, local vertex output {}, compact vertex output {}",
                            MaxPreferredMeshWorkgroupInvocations,
                            MaxPreferredTaskWorkgroupInvocations,
                            PrefersLocalInvocationVertexOutput,
                            PrefersCompactVertexOutput);

        return text;
    }

    std::string VulkanDeviceRayTracingCapabilities::ToString() const {
        std::string text = "Ray tracing:\n";

        text += std::format("  Supported      : acceleration structure {}, pipeline {}, ray query {}\n", AccelerationStructure, Pipeline, RayQuery);

        if (!Pipeline) return text.substr(0, text.size() - 1);

        text += std::format("  Recursion      : {} levels deep\n", MaxRecursionDepth);

        text += std::format("  Shader groups  : handle {} B, handle align {} B, base align {} B\n",
                            ShaderGroupHandleSize,
                            ShaderGroupHandleAlignment,
                            ShaderGroupBaseAlignment);

        text += std::format("  Dispatch       : {} invocations, dims {}", MaxRayDispatchInvocations, formatTriple(MaxRayDispatchDim));

        return text;
    }

    std::string VulkanDeviceFeatureSet::ToString() const {
        std::string text = "Features:\n";

        text += std::format("  Core           : sampler anisotropy {}, independent blend {}, multi draw indirect {}, BC compression {}\n",
                            SamplerAnisotropy,
                            IndependentBlend,
                            MultiDrawIndirect,
                            TextureCompressionBC);

        text += std::format("  Shader types   : int8 {}, int16 {}, int64 {}, float16 {}, float64 {}\n",
                            ShaderInt8,
                            ShaderInt16,
                            ShaderInt64,
                            ShaderFloat16,
                            ShaderFloat64);

        text += std::format("  Shader ops     : fragment stores/atomics {}, storage image multisample {}, demote to helper {}, pipeline stats query {}\n",
                            FragmentStoresAndAtomics,
                            ShaderStorageImageMultisample,
                            ShaderDemoteToHelperInvocation,
                            PipelineStatisticsQuery);

        text += std::format("  Vulkan 1.2     : draw indirect count {}, host query reset {}, timeline semaphore {}, buffer device address {}\n",
                            DrawIndirectCount,
                            HostQueryReset,
                            TimelineSemaphore,
                            BufferDeviceAddress);

        text += std::format("                   scalar block layout {}, sampler filter minmax {}, 8-bit storage {}, memory model {}\n",
                            ScalarBlockLayout,
                            SamplerFilterMinmax,
                            StorageBuffer8BitAccess,
                            VulkanMemoryModel);

        text += std::format("  Vulkan 1.3     : dynamic rendering {}, synchronization2 {}, maintenance4 {}, subgroup size control {}, full subgroups {}\n",
                            DynamicRendering,
                            Synchronization2,
                            Maintenance4,
                            SubgroupSizeControl,
                            ComputeFullSubgroups);

        text += std::format("  Bindless       : {}\n", Bindless);

        text += std::format("  Extensions     : mesh shader {}, acceleration structure {}, ray tracing pipeline {}, ray query {}, dynamic state3 {}",
                            MeshShader,
                            RayTracingAccelerationStructure,
                            RayTracingPipeline,
                            RayQuery,
                            ExtendedDynamicState3);

        return text;
    }

    std::string VulkanDeviceCapabilities::ToString() const {
        std::string text = Identity.ToString();

        text += std::format("\nEffective API  : {}.{}.{}\n",
                            VK_API_VERSION_MAJOR(EffectiveApiVersion),
                            VK_API_VERSION_MINOR(EffectiveApiVersion),
                            VK_API_VERSION_PATCH(EffectiveApiVersion));

        text += std::format("Extensions     : {} supported\n", Extensions.size());

        text += Queues.ToString() + "\n";
        text += Memory.ToString() + "\n";
        text += Features.ToString() + "\n";
        text += Descriptors.ToString() + "\n";
        text += Subgroups.ToString() + "\n";
        text += MeshShaders.ToString() + "\n";
        text += RayTracing.ToString() + "\n";
        text += Limits.ToString();

        return text;
    }

    void VulkanDeviceCapabilities::Normalize() {
        mCapabilities.Identity = Identity;
    }

} // namespace Vulkyrie
