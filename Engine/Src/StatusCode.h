#pragma once

namespace Vkr
{
    enum class StatusCode
    {
        Successful,                            // Operation Successful.
        InvalidApplicationInstance,            // Invalid application instance for initialization.
        PlatformAlreadyInitialized,            // Platform is already initialized.
        EventAlreadyRegistered,                // Event listener already registered.
        ClientAppInitializationFailed,         // Failed to initialize client application.
        XcbConnectionHasError,                 // Platform Linux - XCB Connection has errors.
        XcbFlushError,                         // Platform Linux - XCB Flush failed.
        WindowRegistrationFailed,              // Window registration failed.
        WindowCreationFailed,                  // Window creation failed.
        AppNotInitialized,                     // Application not initialized
        AppAlreadyInitialized,                 // Application already initialized.
        VulkanRequiredValidationLayersMissing, // Required vulkan validation layer(s) missing.
        VulkanInstanceExtensionNotFound,       // Vulkan instance extension not found.
        VulkanXcbSurfaceCreationFailed,        // Vulkan XCB surface creation failed.
        VulkanWin32SurfaceCreationFailed,      // Vulkan Win32 surface creation failed.
        NoDevicesWithVulkanSupport,            // No device could be found that supports Vulkan
        DiscreteGpuRequired,
        PhysicalDeviceDoesNotMeetRequirements,
        SamplerAnisotropyNotSupported,
        RequiredSwapchainNotSupported,
        VulkanRequiredExtensionNotFound,
        VulkanNoPhysicalDeviceMeetsRequirements
    };
}
