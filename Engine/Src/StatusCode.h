#pragma once

namespace Vkr
{
    enum class StatusCode
    {
        Successful,                                  // Operation Successful.
        InvalidApplicationInstance,                  // Invalid application instance for initialization.
        PlatformAlreadyInitialized,                  // Platform is already initialized.
        EventAlreadyRegistered,                      // Event listener already registered.
        ClientAppInitializationFailed,               // Failed to initialize client application.
        XcbConnectionHasError,                       // Platform Linux - XCB Connection has errors.
        XcbFlushError,                               // Platform Linux - XCB Flush failed.
        WindowRegistrationFailed,                    // Window registration failed.
        WindowCreationFailed,                        // Window creation failed.
        AppNotInitialized,                           // Application not initialized
        AppAlreadyInitialized,                       // Application already initialized.
        VulkanRequiredValidationLayersMissing,       // Required vulkan validation layer(s) missing.
        VulkanInstanceExtensionNotFound,             // Vulkan - instance extension not found.
        VulkanXcbSurfaceCreationFailed,              // Vulkan - XCB surface creation failed.
        VulkanWin32SurfaceCreationFailed,            // Vulkan - Win32 surface creation failed.
        VulkanNoDevicesWithVulkanSupport,            // Vulkan - No device could be found that supports Vulkan
        VulkanDiscreteGpuRequired,                   // Vulkan - Discrete GPU Required.
        VulkanPhysicalDeviceDoesNotMeetRequirements, // Vulkan - Physical device does not meet requirements.
        VulkanSamplerAnisotropyNotSupported,         // Vulkan - Sampler Anisotropy is not supported.
        VulkanRequiredSwapchainNotSupported,         // Vulkan - Required swapchain not supported
        VulkanRequiredExtensionNotFound,             // Vulkan - Required extension not found
        VulkanNoPhysicalDeviceMeetsRequirements      // Vulkan - No physical device meets requirements
    };
}
