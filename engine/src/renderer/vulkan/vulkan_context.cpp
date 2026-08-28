#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include "memory/allocators/tracked_std_allocator.h"
#include <algorithm>
#include <map>
#include <vulkyrie_version.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    namespace {
        template <typename T> using RendererVector = TrackedVector<T, MemoryTag::Rendering>;

        RendererVector<const char *> getRequiredInstanceExtensions(bool enableValidation) {
            u32 requiredExtensionCount = 0;
            const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

            RendererVector<const char *> requiredInstanceExtensions(glfwExtensions, glfwExtensions + requiredExtensionCount);

            if (enableValidation) {
                requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return requiredInstanceExtensions;
        }

        RendererVector<const char *> getRequiredInstanceLayers(bool enableValidation) {
            RendererVector<const char *> requiredInstanceLayers;

            if (enableValidation) {
                requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
            }

            return requiredInstanceLayers;
        }

        StatusCode validateRequiredInstanceSupport(const RendererVector<const char *> &requiredLayers, const RendererVector<const char *> &requiredExtensions) {
            u32 layerCount = 0;
            VE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), StatusCode::FailedToQueryVulkanInstanceLayers);

            RendererVector<VkLayerProperties> availableLayers(layerCount);
            VE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()), StatusCode::FailedToQueryVulkanInstanceLayers);

            RendererVector<VkExtensionProperties> availableExtensions;
            auto appendExtensionsFor = [&](const char *layerName) {
                u32 extensionCount = 0;
                VE_VK_CHECK(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr), StatusCode::FailedToQueryVulkanInstanceExtensions);

                size_t offset = availableExtensions.size();
                availableExtensions.resize(offset + extensionCount);
                VE_VK_CHECK(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data() + offset),
                            StatusCode::FailedToQueryVulkanInstanceExtensions);

                return StatusCode::Successful;
            };

            VE_RETURN_ON_FAILURE(appendExtensionsFor(nullptr));

            for (const char *requiredLayerName : requiredLayers) {
                bool found = false;

                for (const auto &availableLayer : availableLayers) {
                    if (0 == std::strcmp(availableLayer.layerName, requiredLayerName)) {
                        found = true;

                        VE_RETURN_ON_FAILURE(appendExtensionsFor(requiredLayerName));

                        VINFO("Found required Vulkan instance layer: {}", requiredLayerName);

                        break;
                    }
                }

                if (!found) {
                    VERROR("Could not find Vulkan instance layer: {}", requiredLayerName);

                    return StatusCode::FailedToFindRequiredVulkanInstanceLayer;
                }
            }

            for (const char *requiredExtensionName : requiredExtensions) {
                bool found = false;

                for (const auto &availableExtension : availableExtensions) {
                    if (0 == std::strcmp(availableExtension.extensionName, requiredExtensionName)) {
                        found = true;

                        VINFO("Found required Vulkan instance extension: {}", requiredExtensionName);

                        break;
                    }
                }

                if (!found) {
                    VERROR("Could not find required Vulkan instance extension: {}", requiredExtensionName);

                    return StatusCode::FailedToFindRequiredVulkanInstanceExtension;
                }
            }

            return StatusCode::Successful;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                     [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *data,
                                                     [[maybe_unused]] void *userData) {

            const LogLevel logLevel = severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT     ? LogLevel::Error
                                      : severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? LogLevel::Warn
                                      : severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    ? LogLevel::Info
                                                                                                   : LogLevel::Debug;

            switch (logLevel) {
                case LogLevel::Error:
                    VERROR("[Renderer] {}", data->pMessage);
                    break;
                case LogLevel::Warn:
                    VWARN("[Renderer] {}", data->pMessage);
                    break;
                case LogLevel::Info:
                    VINFO("[Renderer] {}", data->pMessage);
                    break;
                default:
                    VDEBUG("[Renderer] {}", data->pMessage);
                    break;
            }

            return VK_FALSE;
        }

        RendererVector<VkLayerSettingEXT> getLayerSettings(const ValidationConfig &config) {
            return {
                { "VK_LAYER_KHRONOS_validation", "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.Core },
                { "VK_LAYER_KHRONOS_validation", "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.ThreadSafety },
                { "VK_LAYER_KHRONOS_validation", "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.Sync },
                { "VK_LAYER_KHRONOS_validation", "syncval_submit_time_validation", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.SyncSubmitTime },
                { "VK_LAYER_KHRONOS_validation", "gpuav_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.GpuAV },
                { "VK_LAYER_KHRONOS_validation", "gpuav_select_instrumented_shaders", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.GpuAVSelectiveShaders },
                { "VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.BestPractices },
                { "VK_LAYER_KHRONOS_validation", "printf_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &config.DebugPrintf },
            };
        }

        ValidationConfig createValidationConfig(const DeviceCreationInfo info) {
            const ValidationSettings &settings = info.GraphicsSettings.ValidationSettings;

            return {
                .Core = settings.Has("core"),
                .ThreadSafety = settings.Has("thread_safety"),
                .Sync = settings.Has("sync"),
                .SyncSubmitTime = settings.Has("sync_submit_time"),
                .GpuAV = settings.Has("gpuav"),
                .GpuAVSelectiveShaders = settings.Has("gpuav_selective_shaders"),
                .BestPractices = settings.Has("best_practices"),
                .DebugPrintf = settings.Has("debug_printf"),
            };
        }

        std::string_view ToDeviceTypeString(VkPhysicalDeviceType deviceType) {
            switch (deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    return "Other";
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return "Integrated GPU";
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return "Discrete GPU";
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return "Virtual GPU";
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return "CPU";
                default:
                    return "Unknown";
            }
        }

    } // namespace

    VulkanContext::VulkanContext(const DeviceCreationInfo &info)
        : mCapabilities()
        , mDeviceCreationInfo(info)
        , mValidationConfig(createValidationConfig(mDeviceCreationInfo))
        , mVkInstance(VK_NULL_HANDLE)
        , mVkSurface(VK_NULL_HANDLE)
        , mVkPhysicalDevice(VK_NULL_HANDLE)
        , mVkDevice(VK_NULL_HANDLE)
        , mContextCreated(false) {
    }

    VulkanContext::~VulkanContext() {
        // vkDeviceWaitIdle(mVkDevice);

        // Destroy surface.
        vkDestroySurfaceKHR(mVkInstance, mVkSurface, mHostAllocator.Callbacks());

        // Destroy instance.
        vkDestroyInstance(mVkInstance, mHostAllocator.Callbacks());
    }

    StatusCode VulkanContext::Initialize() {
        // Try to initialize Volk.
        VE_VK_CHECK(volkInitialize(), StatusCode::FailedToInitializeVolk);

        // Get required instance layer and extensions.
        const RendererVector<const char *> requiredInstanceLayers = getRequiredInstanceLayers(mDeviceCreationInfo.EnableRendererValidation);
        const RendererVector<const char *> requiredInstanceExtensions = getRequiredInstanceExtensions(mDeviceCreationInfo.EnableRendererValidation);

        // Make sure that the underlying Vulkan implementation supports the required instance layers and extensions.
        VE_RETURN_ON_FAILURE(validateRequiredInstanceSupport(requiredInstanceLayers, requiredInstanceExtensions));

        const ApplicationInfo &appInfo = mDeviceCreationInfo.ApplicationInfo;

        // Vulkan Application and instance creation info.
        VkApplicationInfo applicationInfo{};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = appInfo.Name.Data();
        applicationInfo.applicationVersion = VK_MAKE_VERSION(appInfo.Version.Major, appInfo.Version.Minor, appInfo.Version.Patch);
        applicationInfo.pEngineName = kEngineName.Data();
        applicationInfo.engineVersion = VK_MAKE_VERSION(kEngineMajorVersion, kEngineMinorVersion, kEnginePatchVersion);
        applicationInfo.apiVersion = VK_API_VERSION_1_4;

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = static_cast<u32>(requiredInstanceLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = requiredInstanceLayers.data();
        instanceCreateInfo.enabledExtensionCount = static_cast<u32>(requiredInstanceExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

        // The loader walks instanceCreateInfo.pNext inside vkCreateInstance,
        // so the chain and everything it points at has to live until that call returns.
        // Hence declared here rather than inside the branch below.
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = debugCallback;

        RendererVector<VkLayerSettingEXT> layerSettings;

        VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{};
        layerSettingsCreateInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
        layerSettingsCreateInfo.pNext = &debugMessengerCreateInfo;

        if (mDeviceCreationInfo.EnableRendererValidation) {
            layerSettings = getLayerSettings(mValidationConfig);

            layerSettingsCreateInfo.settingCount = static_cast<u32>(layerSettings.size());
            layerSettingsCreateInfo.pSettings = layerSettings.data();

            instanceCreateInfo.pNext = &layerSettingsCreateInfo;
        }

        // Try to create vulkan instance.
        VE_VK_CHECK(vkCreateInstance(&instanceCreateInfo, mHostAllocator.Callbacks(), &mVkInstance), StatusCode::FailedToCreateVulkanInstance);

        volkLoadInstance(mVkInstance);

        // TODO: Add this to pNext of the device creation into.
        // VkPhysicalDeviceVulkan13Features features13{
        //     .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        //     .pNext = nullptr,
        //     .dynamicRendering = VK_TRUE,
        //     .synchronization2 = VK_TRUE,
        // };

        // Try to create Window surface.
        auto *window = static_cast<GLFWwindow *>(mDeviceCreationInfo.WindowHandle.NativeWindow);
        VE_VK_CHECK(glfwCreateWindowSurface(mVkInstance, window, mHostAllocator.Callbacks(), &mVkSurface), StatusCode::FailedToCreateSurface);

        selectSuitablePhysicalDevice();

        mContextCreated = true;

        return StatusCode::Successful;
    }

    void VulkanContext::WaitIdle() const {
        // TODO: vkDeviceWaitIdle() once a VkDevice exists here.
    }

    ResourceMemoryRequirements VulkanContext::GetImageMemoryRequirements(const TextureDescriptor &descriptor) const {
        // TODO: fill a VkDeviceImageMemoryRequirements around the same VkImageCreateInfo CreateImage builds and
        // call vkGetDeviceImageMemoryRequirements, then return size / alignment / memoryTypeBits verbatim. The
        // estimate below is a placeholder for a backend that cannot yet answer, and is deliberately not what the
        // packer will ship on: it reads low, and a packer that believes it overlaps resources that do not fit.
        // Until then VulkanBackend::kHasMemoryAliasing stays false, so nothing binds at these offsets.
        return ResourceMemoryRequirements{ .Size = EstimateTextureBytes(descriptor), .Alignment = 256 };
    }

    ResourceMemoryRequirements VulkanContext::GetBufferMemoryRequirements(const BufferDescriptor &descriptor) const {
        // TODO: vkGetDeviceBufferMemoryRequirements, as above.
        return ResourceMemoryRequirements{ .Size = descriptor.Size, .Alignment = 256 };
    }

    const DeviceCapabilities &VulkanContext::QueryCapabilities() const {
        return mCapabilities;
    }

    bool VulkanContext::DeviceLost() const {
        return false;
    }

    void VulkanContext::DestroyImage(VulkanImage image) {
        // TODO: vkDestroyImage / free the allocation once images are real.
        (void)image;
    }

    void VulkanContext::DestroyBuffer(VulkanBuffer buffer) {
        // TODO: vkDestroyBuffer / free the allocation once buffers are real.
        (void)buffer;
    }

    void VulkanContext::DestroySampler(VulkanSampler sampler) {
        // TODO: vkDestroySampler.
        (void)sampler;
    }

    void VulkanContext::DestroyShaderModule(VulkanShaderModule shaderModule) {
        // TODO: vkDestroyShaderModule.
        (void)shaderModule;
    }

    void VulkanContext::DestroyPipeline(VulkanPipeline pipeline) {
        // TODO: vkDestroyPipeline.
        (void)pipeline;
    }

    StatusCode VulkanContext::selectSuitablePhysicalDevice() {
        u32 count = 0;
        VE_VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &count, nullptr), StatusCode::FailedToQueryVulkanPhysicalDevices);
        RendererVector<VkPhysicalDevice> devices(count);
        VE_VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &count, devices.data()), StatusCode::FailedToQueryVulkanPhysicalDevices);

        // Required device extensions.
        constexpr std::array<const char *, 1> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        for (const auto &device : devices) {
            // Get physical device properties.
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            // If the physical device does not implement v1.3 or above,
            // we are not going to support it.
            if (VK_API_VERSION_1_3 > deviceProperties.apiVersion) {
                continue;
            }

            // Get supported physical device extensions..
            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr), StatusCode::FailedToQueryVulkanDeviceExtensions);
            RendererVector<VkExtensionProperties> extensions(count);
            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()), StatusCode::FailedToQueryVulkanDeviceExtensions);

            // Make sure all required extensions are found.
            for (const char *re : requiredDeviceExtensions) {
                bool found = false;

                for (const VkExtensionProperties &ae : extensions) {
                    // If found, great! Look for the next required extension.
                    if (0 == std::strcmp(re, ae.extensionName)) {
                        found = true;
                        break;
                    }
                }

                // If not found, then we can't use this device,
                // hence, we'll skip it and look for another one.
                if (!found) {
                    continue;
                }
            }

            // Get Physical device queue family properties.
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
            RendererVector<VkQueueFamilyProperties> queueFamilyProperties(count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilyProperties.data());

            // Get Physical device memory properties.
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

            // Chain feature query structs.
            VkPhysicalDeviceMeshShaderFeaturesEXT mesh{};
            mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{};
            dynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
            dynamicStateFeatures.pNext = &mesh;

            VkPhysicalDeviceVulkan13Features f13{};
            f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            f13.pNext = &dynamicStateFeatures;

            VkPhysicalDeviceVulkan11Features f11{};
            f11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            f11.pNext = &f13;

            VkPhysicalDeviceFeatures2 deviceFeatures{};
            deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures.pNext = &f11;
            vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

            VINFO("**********************************************************************************");
            VINFO("Device ID: {}", deviceProperties.deviceID);
            VINFO("Device Name: {}", deviceProperties.deviceName);
            VINFO("Device Type: {}", ToDeviceTypeString(deviceProperties.deviceType));
            VINFO("API Version: v{}.{}.{}",
                  VK_VERSION_MAJOR(deviceProperties.apiVersion),
                  VK_VERSION_MINOR(deviceProperties.apiVersion),
                  VK_VERSION_PATCH(deviceProperties.apiVersion));
            VINFO("Driver Version: {}", deviceProperties.driverVersion);
            VINFO("Vendor ID: {}", deviceProperties.vendorID);
        }

        return StatusCode::Successful;
    }

} // namespace Vulkyrie
