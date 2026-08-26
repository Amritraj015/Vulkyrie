#pragma once

#include <volk.h>

namespace Vulkyrie {

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

} // namespace Vulkyrie
