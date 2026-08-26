#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include <vulkyrie_version.h>
#include <volk.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    namespace {

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

        TrackedVector<VkLayerSettingEXT, MemoryTag::Rendering> getLayerSettings(const ValidationConfig &config) {
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

    } // namespace

    VulkanContext::VulkanContext(const DeviceCreationInfo &info)
        : mDeviceCreationInfo(info)
        , mValidationConfig(createValidationConfig(mDeviceCreationInfo))
        , mCapabilities()
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
        const TrackedVector<const char *, MemoryTag::Rendering> requiredInstanceLayers = getRequiredInstanceLayers();
        const TrackedVector<const char *, MemoryTag::Rendering> requiredInstanceExtensions = getRequiredInstanceExtensions();

        // Make sure that the underlying Vulkan implementation supports the required instance layers and extensions.
        VE_RETURN_ON_FAILURE(validateRequiredInstanceSupport(requiredInstanceLayers, requiredInstanceExtensions));

        const ApplicationInfo &appInfo = mDeviceCreationInfo.ApplicationInfo;

        // Vulkan Application and instance creation info.
        const VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = appInfo.Name.Data(),
            .applicationVersion = VK_MAKE_VERSION(appInfo.Version.Major, appInfo.Version.Minor, appInfo.Version.Patch),
            .pEngineName = kEngineName.Data(),
            .engineVersion = VK_MAKE_VERSION(kEngineMajorVersion, kEngineMinorVersion, kEnginePatchVersion),
            .apiVersion = VK_API_VERSION_1_4,
        };

        VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<u32>(requiredInstanceLayers.size()),
            .ppEnabledLayerNames = requiredInstanceLayers.data(),
            .enabledExtensionCount = static_cast<u32>(requiredInstanceExtensions.size()),
            .ppEnabledExtensionNames = requiredInstanceExtensions.data(),
        };

        // The loader walks instanceCreateInfo.pNext inside vkCreateInstance,
        // so the chain and everything it points at has to live until that call returns.
        // Hence declared here rather than inside the branch below.
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        };

        TrackedVector<VkLayerSettingEXT, MemoryTag::Rendering> layerSettings;

        VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{
            .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
            .pNext = &debugMessengerCreateInfo,
            .settingCount = 0,
            .pSettings = nullptr,
        };

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

    TrackedVector<const char *, MemoryTag::Rendering> VulkanContext::getRequiredInstanceExtensions() {
        u32 requiredExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

        TrackedVector<const char *, MemoryTag::Rendering> requiredInstanceExtensions(glfwExtensions, glfwExtensions + requiredExtensionCount);

        if (mDeviceCreationInfo.EnableRendererValidation) {
            requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return requiredInstanceExtensions;
    }

    TrackedVector<const char *, MemoryTag::Rendering> VulkanContext::getRequiredInstanceLayers() {
        TrackedVector<const char *, MemoryTag::Rendering> requiredInstanceLayers;

        if (mDeviceCreationInfo.EnableRendererValidation) {
            requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        return requiredInstanceLayers;
    }

    // TrackedVector<const char *, MemoryTag::Rendering> VulkanContext::getRequiredDeviceExtensions() {
    //     const TrackedVector<const char *, MemoryTag::Rendering> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    //
    //     return requiredDeviceExtensions;
    // }

    StatusCode VulkanContext::validateRequiredInstanceSupport(const TrackedVector<const char *, MemoryTag::Rendering> &requiredLayers,
                                                              const TrackedVector<const char *, MemoryTag::Rendering> &requiredExtensions) {
        u32 layerCount = 0;
        VE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), StatusCode::FailedToQueryVulkanInstanceLayers);

        TrackedVector<VkLayerProperties, MemoryTag::Rendering> availableLayers(layerCount);
        VE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()), StatusCode::FailedToQueryVulkanInstanceLayers);

        TrackedVector<VkExtensionProperties, MemoryTag::Rendering> availableExtensions;
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

} // namespace Vulkyrie
