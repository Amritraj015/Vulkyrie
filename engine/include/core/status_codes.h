#pragma once

#include <cstdint>

#define VE_RETURN_ON_FAILURE(expr)                                                                                                                             \
    do {                                                                                                                                                       \
        Vulkyrie::StatusCode _s = (expr);                                                                                                                      \
        if (Vulkyrie::StatusCode::Successful != _s) return _s;                                                                                                 \
    } while (false)

namespace Vulkyrie {
    /** @brief This `enum` defines status codes to represent various operation results. */
    enum class StatusCode : int32_t {
        Successful = 0,
        InvalidApplication,
        FailedToInitializeLogger,
        UnsupportedLoggerType,
        FailedToInitializeGLFW,
        FailedToCreateWindow,
        UnsupportedGraphicsAPI,
        RendererAlreadyInitialized,
        FailedToInitializeRendererContext,
        FailedToInitializeGLAD,
        FailedToCompileShaderProgram,
        JobSystemAlreadyInitialized,
        FailedToInitializeVolk,
        FailedToCreateVulkanInstance,
        FailedToQueryVulkanInstanceLayers,
        FailedToFindRequiredVulkanInstanceLayer,
        FailedToQueryVulkanInstanceExtensions,
        FailedToFindRequiredVulkanInstanceExtension,
        NoPhysicalDevicesFound,
        NoSuitablePhysicalDeviceFound,
        FailedToQueryVulkanDeviceExtensions,
        FailedToFindRequiredVulkanDeviceExtension,
        FailedToCreateSurface,
        FailedToQueryVulkanPhysicalDevices,
        FailedToQueryVulkanLogicalDevices,
        FailedToCreateLogicalDevice,
        FailedToGetGraphicsQueue,
        FailedToGetTransferQueue,
        FailedToGetComputeQueue,
        FailedToGetPresentQueue,
        FailedToLoadVulkanMemoryAllocatorFunctionsFromVolk,
        FailedToInitializeVulkanMemoryAllocator,
        FailedToQueryPhysicalDeviceSurfaceCapabilities,
        FailedToQueryPhysicalDeviceSurfaceFormats,
        RequiredSwapchainSurfaceFormatNotSupported,
        FailedToCreateVulkanSwapchain,
        FailedToGetVulkanSwapchainImages,
        FailedToCreateVulkanSwapchainImageView,
        FailedToCreateVulkanSemaphore,
        FailedToCreateDepthImage,
        FailedToCreateDepthImageView,

    };
} // namespace Vulkyrie
