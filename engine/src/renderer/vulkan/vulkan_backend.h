#pragma once

#include "core/graphics_api.h"
#include "core/types/static_string.h"
#include "renderer/vulkan/vulkan_command_list.h"
#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_pool.h"
#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_types.h"

namespace Vulkyrie {

    struct VulkanBackend final {
        // Types.
        using Context = VulkanContext;
        using Queue = VulkanQueue;
        using CommandList = VulkanCommandList;
        using CommandPool = VulkanPool;
        using Swapchain = VulkanSwapchain;

        // Handles.
        using Image = VulkanImage;
        using Buffer = VulkanBuffer;
        using Sampler = VulkanSampler;
        using Pipeline = VulkanPipeline;
        using ShaderModule = VulkanShaderModule;

        static constexpr StaticString kName = "Vulkan";
        static constexpr GraphicsAPI kType = GraphicsAPI::Vulkan;
        static constexpr u32 kFramesInFlight = 2;
        static constexpr bool kUsesBindlessHeap = true;
        static constexpr bool kHasTimelineSync = true;
        static constexpr bool kHasExplicitBarriers = true;
        static constexpr bool kHasMemoryAliasing = true;
        static constexpr bool kRecordsInParallel = true;
    };

} // namespace Vulkyrie
