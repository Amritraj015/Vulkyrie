#include "renderer/vulkan/vulkan_context.h"
#include "core/utilities.h"
#include "memory/memory_tracker.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include <vulkyrie_version.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    namespace {

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

                usize offset = availableExtensions.size();
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

        void VKAPI_PTR onVmaDeviceMemoryAllocate([[maybe_unused]] VmaAllocator allocator,
                                                 [[maybe_unused]] u32 memoryType,
                                                 [[maybe_unused]] VkDeviceMemory memory,
                                                 VkDeviceSize size,
                                                 [[maybe_unused]] void *userData) {
            MemoryTracker::OnAllocation(MemoryTag::GpuVram, static_cast<i64>(size));
        }

        void VKAPI_PTR onVmaDeviceMemoryFree([[maybe_unused]] VmaAllocator allocator,
                                             [[maybe_unused]] u32 memoryType,
                                             [[maybe_unused]] VkDeviceMemory memory,
                                             VkDeviceSize size,
                                             [[maybe_unused]] void *userData) {
            MemoryTracker::OnFree(MemoryTag::GpuVram, static_cast<i64>(size));
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

        DeviceType ToDeviceType(VkPhysicalDeviceType deviceType) {
            switch (deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    return DeviceType::Other;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return DeviceType::IntegratedGPU;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return DeviceType::DiscreteGPU;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return DeviceType::VirtualGPU;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return DeviceType::CPU;
                default:
                    return DeviceType::Other;
            }
        }

        VendorID ToVendorID(u32 vendorID) {
            switch (vendorID) {
                case 0x1002:
                    return VendorID::AMD;
                case 0x10DE:
                    return VendorID::NVIDIA;
                case 0x8086:
                    return VendorID::Intel;
                case 0x106B:
                    return VendorID::Apple;
                case 0x13B5:
                    return VendorID::ARM;
                case 0x5143:
                    return VendorID::Qualcomm;
                case 0x1010:
                    return VendorID::ImaginationTechnologies;
                case 0x14E4:
                    return VendorID::Broadcom;
                default:
                    return VendorID::Unknown;
            }
        }

        StatusCode getSupportedDeviceExtensions(VkPhysicalDevice mVkDevice, RendererVector<VkExtensionProperties> &extensions) {
            u32 extensionCount = 0;
            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(mVkDevice, nullptr, &extensionCount, nullptr), StatusCode::FailedToQueryVulkanDeviceExtensions);

            extensions.resize(extensionCount);

            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(mVkDevice, nullptr, &extensionCount, extensions.data()),
                        StatusCode::FailedToQueryVulkanDeviceExtensions);

            return StatusCode::Successful;
        }

        // The engine's floor, not a preference list: the frame graph records barriers through
        // synchronization2 and every pass renders through dynamic rendering, both core in 1.3.
        bool isDeviceSuitable(const VulkanDeviceCapabilities &caps) {
            return VK_API_VERSION_1_3 <= caps.EffectiveApiVersion && caps.HasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME) &&
                   kInvalidQueueFamilyIndex != caps.Queues.GraphicsFamily && kInvalidQueueFamilyIndex != caps.Queues.PresentFamily &&
                   caps.Features.DynamicRendering && caps.Features.Synchronization2;
        }

        // Device class dominates; the bonuses only separate devices of the same class, and VRAM
        // only separates devices the bonuses leave tied.
        u64 scoreDevice(const VulkanDeviceCapabilities &caps) {
            u64 score = 0;

            switch (caps.Identity.DeviceType) {
                case DeviceType::DiscreteGPU:
                    score += 1'000'000;
                    break;
                case DeviceType::IntegratedGPU:
                    score += 100'000;
                    break;
                case DeviceType::VirtualGPU:
                    score += 10'000;
                    break;
                case DeviceType::CPU:
                    score += 1'000;
                    break;
                default:
                    break;
            }

            if (caps.Features.Bindless) score += 50'000;
            if (caps.Queues.SupportsAsyncCompute) score += 20'000;
            if (caps.Queues.SupportsAsyncTransfer) score += 20'000;
            if (caps.Features.MeshShader) score += 10'000;
            if (caps.Features.RayTracingPipeline) score += 10'000;

            return score + (caps.Memory.DeviceLocalBytes >> 30);
        }

        // The console sink formats each message into a fixed 2 KB stack buffer, so a capability
        // dump goes out one line per call rather than as one message that would truncate.
        void logCapabilities(const VulkanDeviceCapabilities &caps) {
            const std::string text = caps.ToString();

            for (const auto line : std::views::split(text, '\n')) {
                VINFO("{}", std::string_view(line.begin(), line.end()));
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
        , mVmaAllocator(VK_NULL_HANDLE)
        , mVkSwapchain(VK_NULL_HANDLE)
        , mVkDepthImage(VK_NULL_HANDLE)
        , mVkDepthImageView(VK_NULL_HANDLE)
        , mVmaDepthImageAllocation(VK_NULL_HANDLE)
        , mVkVertexShaderModule(VK_NULL_HANDLE)
        , mVkFragmentShaderModule(VK_NULL_HANDLE)
        , mVkPipelineLayout(VK_NULL_HANDLE)
        , mVkGraphicsPipeline(VK_NULL_HANDLE)
        // , mVkTimelineSemaphore(VK_NULL_HANDLE)
        , mContextCreated(false) {
    }

    VulkanContext::~VulkanContext() {
        // Wait for the device to be idle.
        if (VK_NULL_HANDLE != mVkDevice) {
            vkDeviceWaitIdle(mVkDevice);
        }

        // TODO: Will need to be moved
        {
            // Destroy timeline semaphore.
            // vkDestroySemaphore(mVkDevice, mVkTimelineSemaphore, mHostAllocator.Callbacks());

            for (auto &resources : mFrameResources) {
                if (VK_NULL_HANDLE != resources.ImageAcquiredSemaphore) {
                    vkDestroySemaphore(mVkDevice, resources.ImageAcquiredSemaphore, mHostAllocator.Callbacks());
                }

                if (VK_NULL_HANDLE != resources.CommandPool) {
                    vkDestroyCommandPool(mVkDevice, resources.CommandPool, mHostAllocator.Callbacks());
                }
            }

            // Destroy pipeline layout.
            vkDestroyPipelineLayout(mVkDevice, mVkPipelineLayout, mHostAllocator.Callbacks());

            // Destroy graphics pipeline.
            vkDestroyPipeline(mVkDevice, mVkGraphicsPipeline, mHostAllocator.Callbacks());

            // Destroy shader modules.
            vkDestroyShaderModule(mVkDevice, mVkVertexShaderModule, mHostAllocator.Callbacks());
            vkDestroyShaderModule(mVkDevice, mVkFragmentShaderModule, mHostAllocator.Callbacks());
        }

        // TODO: Will need to be moved
        {
            // Destroy the depth image allocation.
            destroySwapchain();
        }

        // Destroy Vulkan memory allocator instance.
        if (VK_NULL_HANDLE != mVmaAllocator) {
            vmaDestroyAllocator(mVmaAllocator);
        }

        // Destroy logical device.
        if (VK_NULL_HANDLE != mVkDevice) {
            vkDestroyDevice(mVkDevice, mHostAllocator.Callbacks());
        }

        // Destroy surface.
        if (VK_NULL_HANDLE != mVkInstance && VK_NULL_HANDLE != mVkSurface) {
            vkDestroySurfaceKHR(mVkInstance, mVkSurface, mHostAllocator.Callbacks());
        }

        // Destroy instance.
        if (VK_NULL_HANDLE != mVkInstance) {
            vkDestroyInstance(mVkInstance, mHostAllocator.Callbacks());
        }

        // Finally, unload Volk.
        volkFinalize();
    }

    StatusCode VulkanContext::Initialize() {
        // Try to initialize Volk.
        VE_VK_CHECK(volkInitialize(), StatusCode::FailedToInitializeVolk);

        // Create Vulkan instance.
        VE_RETURN_ON_FAILURE(createInstance());

        // Try to create Window surface.
        VE_RETURN_ON_FAILURE(createSurface());

        // Try and select suitable physical device.
        VE_RETURN_ON_FAILURE(selectSuitablePhysicalDevice());

        // Try to create a logical device and get queues.
        VE_RETURN_ON_FAILURE(createLogicalDevice());

        // Try to initialize Vulkan memory allocator.
        VE_RETURN_ON_FAILURE(initializeVulkanMemoryAllocator());

        // Try to create swapchain.
        VE_RETURN_ON_FAILURE(createSwapchain());

        // Try to create shader module and the graphics pipeline.
        VE_RETURN_ON_FAILURE(createShaders());

        // Create synchronization resources.
        VE_RETURN_ON_FAILURE(createSynchronizationResources());

        // Create command pool nad buffer.
        VE_RETURN_ON_FAILURE(createCommandBuffers());

        mContextCreated = true;

        return StatusCode::Successful;
    }

    void VulkanContext::WaitIdle() const {
        vkDeviceWaitIdle(mVkDevice);
    }

    ResourceMemoryRequirements VulkanContext::GetImageMemoryRequirements(const TextureDescriptor &descriptor) const {
        // VkMemoryRequirements2 memoryRequirements2{};
        // memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        //
        // VkImageCreateInfo imageCreateInfo{
        //     .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .imageType = VK_IMAGE_TYPE_2D,
        //     .format = VK_FORMAT_R8G8B8A8_UNORM,
        //     .extent = { .width = 1920, .height = 1080, .depth = 1 },
        //     .mipLevels = 1,
        //     .arrayLayers = 1,
        //     .samples = VK_SAMPLE_COUNT_1_BIT,
        //     .tiling = VK_IMAGE_TILING_OPTIMAL,
        //     .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        //     .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        //     .queueFamilyIndexCount = 0,
        //     .pQueueFamilyIndices = nullptr,
        //     .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // };
        //
        // VkDeviceImageMemoryRequirements imageMemoryRequirements{};
        // imageMemoryRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
        // imageMemoryRequirements.pCreateInfo = &imageCreateInfo;
        // // TODO: This is not correct, needs to be set dynamically.
        // imageMemoryRequirements.planeAspect = VK_IMAGE_ASPECT_NONE;
        //
        // vkGetDeviceImageMemoryRequirements(mVkDevice, &imageMemoryRequirements, &memoryRequirements2);
        //
        // return ResourceMemoryRequirements{
        //     .Size = static_cast<u64>(memoryRequirements2.memoryRequirements.size),
        //     .Alignment = static_cast<u64>(memoryRequirements2.memoryRequirements.alignment),
        //     .MemoryTypeBits = memoryRequirements2.memoryRequirements.memoryTypeBits,
        // };

        return ResourceMemoryRequirements{
            .Size = EstimateTextureBytes(descriptor),
            .Alignment = 256,
            .MemoryTypeBits = 0,
        };
    }

    ResourceMemoryRequirements VulkanContext::GetBufferMemoryRequirements(const BufferDescriptor &descriptor) const {
        // TODO: vkGetDeviceBufferMemoryRequirements, as above.
        return ResourceMemoryRequirements{ .Size = descriptor.Size, .Alignment = 256 };
    }

    VulkanImage VulkanContext::CreateImage(const TextureDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    VulkanBuffer VulkanContext::CreateBuffer(const BufferDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    VulkanSampler VulkanContext::CreateSampler(const SamplerDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    VulkanShaderModule VulkanContext::CreateShaderModule(const ShaderBlob &blob) {
        (void)blob;
        return {};
    }

    VulkanPipeline VulkanContext::CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    VulkanPipeline VulkanContext::CreateComputePipeline(const ComputePipelineDescriptor &descriptor) {
        (void)descriptor;
        return {};
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

#if defined(VE_VK_ENABLE_VALIDATION)

    StatusCode VulkanContext::SetDebugName(StaticString name, VkObjectType objectType, u64 objectHandle) {
        if (VK_NULL_HANDLE == mVkDevice || 0 == objectHandle) {
            return StatusCode::FailedToCreateVulkanObjectDebugName;
        }

        const VkDebugUtilsObjectNameInfoEXT debugObjectNameCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = VK_NULL_HANDLE,
            .objectType = static_cast<VkObjectType>(objectType),
            .objectHandle = objectHandle,
            .pObjectName = name.Data(),
        };

        VE_VK_CHECK(vkSetDebugUtilsObjectNameEXT(mVkDevice, &debugObjectNameCreateInfo), StatusCode::FailedToCreateVulkanObjectDebugName);

        return StatusCode::Successful;
    }

#endif

    StatusCode VulkanContext::createInstance() {
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
        applicationInfo.apiVersion = VULKAN_API_VERSION;

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

        // Initialize instance level functions only.
        // NOTE: The following will not load device specific functions pointers.
        // Those are loaded by volkLoadDevice(mVkDevice) call after logical device creation.
        volkLoadInstanceOnly(mVkInstance);

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createSurface() {
        auto *window = static_cast<GLFWwindow *>(mDeviceCreationInfo.WindowHandle.NativeWindow);
        VE_VK_CHECK(glfwCreateWindowSurface(mVkInstance, window, mHostAllocator.Callbacks(), &mVkSurface), StatusCode::FailedToCreateSurface);

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::selectSuitablePhysicalDevice() {
        u32 deviceCount = 0;
        VE_VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr), StatusCode::FailedToQueryVulkanPhysicalDevices);

        // If no physical devices were found, we can't continue, return an error.
        if (0 == deviceCount) {
            return StatusCode::NoPhysicalDevicesFound;
        }

        RendererVector<VkPhysicalDevice> devices(deviceCount);
        VE_VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, devices.data()), StatusCode::FailedToQueryVulkanPhysicalDevices);

        // One entry per enumerated device, in enumeration order, so capabilities[i] describes
        // devices[i]. A device below the engine's floor still gets an identity-only entry rather
        // than being dropped, so the log can name the GPU that was turned down.
        RendererVector<VulkanDeviceCapabilities> capabilities;
        capabilities.reserve(deviceCount);

        for (const VkPhysicalDevice device : devices) {
            VkPhysicalDeviceProperties baseProperties{};
            vkGetPhysicalDeviceProperties(device, &baseProperties);

            VulkanDeviceCapabilities caps{};

            // deviceName is a fixed 256-byte buffer and the destination is 255; copying it as a C
            // string keeps the result terminated instead of carrying the source's tail padding.
            std::snprintf(caps.Identity.DeviceName, sizeof(caps.Identity.DeviceName), "%s", baseProperties.deviceName);

            caps.Identity.VendorID = ToVendorID(baseProperties.vendorID);
            caps.Identity.DeviceID = baseProperties.deviceID;
            caps.Identity.ApiVersion = baseProperties.apiVersion;
            caps.Identity.DriverVersion = baseProperties.driverVersion;
            caps.Identity.DeviceType = ToDeviceType(baseProperties.deviceType);

            // The instance was created at 1.4, so nothing beyond that is reachable however high the
            // device reports. Every version gate below keys off this, not the raw apiVersion.
            caps.EffectiveApiVersion = std::min(VULKAN_API_VERSION, baseProperties.apiVersion);

            VE_RETURN_ON_FAILURE(getSupportedDeviceExtensions(device, caps.Extensions));

            const auto hasExtension = [&caps](const char *name) { return caps.HasExtension(name); };

            // Below 1.3 the property and feature structures chained below are not valid to pass, and
            // the device could not satisfy isDeviceSuitable anyway. Record the identity and move on.
            if (VK_API_VERSION_1_3 > caps.EffectiveApiVersion) {
                capabilities.push_back(std::move(caps));

                continue;
            }

            const bool supportsVulkan14 = VULKAN_API_VERSION <= caps.EffectiveApiVersion;

            // ----------------------------------------------------------------------------------------------------
            // Query physical device properties.
            // ----------------------------------------------------------------------------------------------------
            VkPhysicalDeviceVulkan11Properties p11{};
            p11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;

            VkPhysicalDeviceVulkan12Properties p12{};
            p12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;

            VkPhysicalDeviceVulkan13Properties p13{};
            p13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;

            VkPhysicalDeviceVulkan14Properties p14{};
            p14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;

            VkPhysicalDeviceMeshShaderPropertiesEXT meshProperties{};
            meshProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;

            VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{};
            rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

            VkPhysicalDeviceProperties2 deviceProperties{};
            deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

            void **propertyNext = &deviceProperties.pNext;

            const auto chainProperty = [&propertyNext](auto &structure) {
                *propertyNext = &structure;
                propertyNext = &structure.pNext;
            };

            chainProperty(p11);
            chainProperty(p12);
            chainProperty(p13);

            if (supportsVulkan14) {
                chainProperty(p14);
            }

            const bool hasMeshExtension = hasExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
            const bool hasRayTracingPipelineExtension = hasExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

            if (hasMeshExtension) {
                chainProperty(meshProperties);
            }

            if (hasRayTracingPipelineExtension) {
                chainProperty(rayTracingProperties);
            }

            vkGetPhysicalDeviceProperties2(device, &deviceProperties);

            const VkPhysicalDeviceProperties &p = deviceProperties.properties;
            const VkPhysicalDeviceLimits &l = p.limits;

            caps.Identity.DriverID = p12.driverID;
            std::snprintf(caps.Identity.DriverInfo, sizeof(caps.Identity.DriverInfo), "%s %s", p12.driverName, p12.driverInfo);

            caps.Limits.MaxTexture1DDim = l.maxImageDimension1D;
            caps.Limits.MaxTexture2DDim = l.maxImageDimension2D;
            caps.Limits.MaxTexture3DDim = l.maxImageDimension3D;
            caps.Limits.MaxTextureCubeDim = l.maxImageDimensionCube;
            caps.Limits.MaxTextureArrayLayers = l.maxImageArrayLayers;

            caps.Limits.MaxColorAttachments = l.maxColorAttachments;
            caps.Limits.MaxViewports = l.maxViewports;
            caps.Limits.MaxViewportDimensions[0] = l.maxViewportDimensions[0];
            caps.Limits.MaxViewportDimensions[1] = l.maxViewportDimensions[1];
            caps.Limits.MaxFramebufferWidth = l.maxFramebufferWidth;
            caps.Limits.MaxFramebufferHeight = l.maxFramebufferHeight;
            caps.Limits.MaxFramebufferLayers = l.maxFramebufferLayers;

            caps.Limits.MaxPushConstantBytes = l.maxPushConstantsSize;

            for (u32 i = 0; i < 3; ++i) {
                caps.Limits.MaxComputeWorkgroupSize[i] = l.maxComputeWorkGroupSize[i];
                caps.Limits.MaxComputeWorkgroupCount[i] = l.maxComputeWorkGroupCount[i];
            }

            caps.Limits.MaxComputeWorkgroupInvocations = l.maxComputeWorkGroupInvocations;

            caps.Limits.MinUniformBufferAlign = l.minUniformBufferOffsetAlignment;
            caps.Limits.MinStorageBufferAlign = l.minStorageBufferOffsetAlignment;
            caps.Limits.MinTexelBufferOffsetAlign = l.minTexelBufferOffsetAlignment;
            caps.Limits.OptimalBufferCopyAlign = l.optimalBufferCopyOffsetAlignment;
            caps.Limits.OptimalBufferCopyRowPitchAlign = l.optimalBufferCopyRowPitchAlignment;
            caps.Limits.MinMemoryMapAlign = l.minMemoryMapAlignment;
            caps.Limits.NonCoherentAtomSize = l.nonCoherentAtomSize;
            caps.Limits.BufferImageGranularity = l.bufferImageGranularity;
            caps.Limits.MaxSamplerAnisotropy = l.maxSamplerAnisotropy;
            caps.Limits.MaxSamplerAllocationCount = l.maxSamplerAllocationCount;

            caps.Limits.MaxUniformBufferRange = l.maxUniformBufferRange;
            caps.Limits.MaxStorageBufferRange = l.maxStorageBufferRange;
            caps.Limits.MaxTexelBufferElements = l.maxTexelBufferElements;
            caps.Limits.MaxMemoryAllocationSize = p11.maxMemoryAllocationSize;

            caps.Limits.MaxVertexInputAttributes = l.maxVertexInputAttributes;
            caps.Limits.MaxVertexInputBindings = l.maxVertexInputBindings;
            caps.Limits.MaxVertexInputAttributeOffset = l.maxVertexInputAttributeOffset;
            caps.Limits.MaxVertexInputBindingStride = l.maxVertexInputBindingStride;

            caps.Limits.FramebufferColorSampleCounts = l.framebufferColorSampleCounts;
            caps.Limits.FramebufferDepthSampleCounts = l.framebufferDepthSampleCounts;

            caps.Limits.TimestampPeriodNs = l.timestampPeriod;
            caps.Limits.TimestampComputeAndGraphics = l.timestampComputeAndGraphics;

            caps.Subgroups.Size = p11.subgroupSize;
            caps.Subgroups.SupportedStages = p11.subgroupSupportedStages;
            caps.Subgroups.SupportedOperations = p11.subgroupSupportedOperations;
            caps.Subgroups.MinSize = p13.minSubgroupSize;
            caps.Subgroups.MaxSize = p13.maxSubgroupSize;

            caps.Descriptors.MaxUpdateAfterBindDescriptors = p12.maxUpdateAfterBindDescriptorsInAllPools;
            caps.Descriptors.MaxUpdateAfterBindSampledImages = p12.maxDescriptorSetUpdateAfterBindSampledImages;
            caps.Descriptors.MaxUpdateAfterBindStorageImages = p12.maxDescriptorSetUpdateAfterBindStorageImages;
            caps.Descriptors.MaxUpdateAfterBindUniformBuffers = p12.maxDescriptorSetUpdateAfterBindUniformBuffers;
            caps.Descriptors.MaxUpdateAfterBindStorageBuffers = p12.maxDescriptorSetUpdateAfterBindStorageBuffers;
            caps.Descriptors.MaxUpdateAfterBindSamplers = p12.maxDescriptorSetUpdateAfterBindSamplers;

            caps.Descriptors.MaxPerStageUpdateAfterBindSamplers = p12.maxPerStageDescriptorUpdateAfterBindSamplers;
            caps.Descriptors.MaxPerStageUpdateAfterBindSampledImages = p12.maxPerStageDescriptorUpdateAfterBindSampledImages;
            caps.Descriptors.MaxPerStageUpdateAfterBindStorageImages = p12.maxPerStageDescriptorUpdateAfterBindStorageImages;
            caps.Descriptors.MaxPerStageUpdateAfterBindUniformBuffers = p12.maxPerStageDescriptorUpdateAfterBindUniformBuffers;
            caps.Descriptors.MaxPerStageUpdateAfterBindStorageBuffers = p12.maxPerStageDescriptorUpdateAfterBindStorageBuffers;
            caps.Descriptors.MaxPerStageUpdateAfterBindResources = p12.maxPerStageUpdateAfterBindResources;

            caps.Descriptors.MaxBindlessTextures =
                std::min(caps.Descriptors.MaxUpdateAfterBindSampledImages, caps.Descriptors.MaxPerStageUpdateAfterBindSampledImages);

            caps.Descriptors.MaxBindlessStorageImages =
                std::min(caps.Descriptors.MaxUpdateAfterBindStorageImages, caps.Descriptors.MaxPerStageUpdateAfterBindStorageImages);

            caps.Descriptors.MaxBindlessBuffers =
                std::min(caps.Descriptors.MaxUpdateAfterBindStorageBuffers, caps.Descriptors.MaxPerStageUpdateAfterBindStorageBuffers);

            caps.Descriptors.MaxBindlessSamplers = std::min(caps.Descriptors.MaxUpdateAfterBindSamplers, caps.Descriptors.MaxPerStageUpdateAfterBindSamplers);

            if (hasMeshExtension) {
                auto &m = caps.MeshShaders;

                m.MaxMeshWorkgroupInvocations = meshProperties.maxMeshWorkGroupInvocations;
                m.MaxMeshOutputVertices = meshProperties.maxMeshOutputVertices;
                m.MaxMeshOutputPrimitives = meshProperties.maxMeshOutputPrimitives;
                m.MaxMeshSharedMemorySize = meshProperties.maxMeshSharedMemorySize;

                m.MaxTaskWorkgroupInvocations = meshProperties.maxTaskWorkGroupInvocations;
                m.MaxTaskPayloadSize = meshProperties.maxTaskPayloadSize;

                for (u32 i = 0; i < 3; ++i) {
                    m.MaxMeshWorkgroupSize[i] = meshProperties.maxMeshWorkGroupSize[i];
                    m.MaxMeshWorkgroupCount[i] = meshProperties.maxMeshWorkGroupCount[i];
                    m.MaxTaskWorkgroupSize[i] = meshProperties.maxTaskWorkGroupSize[i];
                    m.MaxTaskWorkgroupCount[i] = meshProperties.maxTaskWorkGroupCount[i];
                }

                m.MaxPreferredMeshWorkgroupInvocations = meshProperties.maxPreferredMeshWorkGroupInvocations;
                m.MaxPreferredTaskWorkgroupInvocations = meshProperties.maxPreferredTaskWorkGroupInvocations;
                m.PrefersLocalInvocationVertexOutput = meshProperties.prefersLocalInvocationVertexOutput;
                m.PrefersCompactVertexOutput = meshProperties.prefersCompactVertexOutput;
            }

            if (hasRayTracingPipelineExtension) {
                auto &r = caps.RayTracing;

                r.MaxRecursionDepth = rayTracingProperties.maxRayRecursionDepth;
                r.ShaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
                r.ShaderGroupHandleAlignment = rayTracingProperties.shaderGroupHandleAlignment;
                r.ShaderGroupBaseAlignment = rayTracingProperties.shaderGroupBaseAlignment;

                // VUID-vkCmdTraceRaysKHR-width-03641: width * height * depth.
                r.MaxRayDispatchInvocations = rayTracingProperties.maxRayDispatchInvocationCount;

                // VUID-vkCmdTraceRaysKHR-width/height/depth-03638/03639/03640: a
                // separate per-axis cap that 03641 does not subsume.
                for (u32 i = 0; i < 3; ++i) {
                    r.MaxRayDispatchDim[i] = static_cast<u64>(l.maxComputeWorkGroupCount[i]) * static_cast<u64>(l.maxComputeWorkGroupSize[i]);
                }
            }

            // ----------------------------------------------------------------------------------------------------
            // Query physical device features.
            // ----------------------------------------------------------------------------------------------------
            VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

            VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicStateFeatures{};
            dynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
            rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

            VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{};
            rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

            VkPhysicalDeviceVulkan14Features f14{};
            f14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

            VkPhysicalDeviceVulkan13Features f13{};
            f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

            VkPhysicalDeviceVulkan12Features f12{};
            f12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

            VkPhysicalDeviceVulkan11Features f11{};
            f11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

            VkPhysicalDeviceFeatures2 deviceFeatures{};
            deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

            void **featureNext = &deviceFeatures.pNext;

            const auto chainFeature = [&featureNext](auto &structure) {
                *featureNext = &structure;
                featureNext = &structure.pNext;
            };

            chainFeature(f11);
            chainFeature(f12);
            chainFeature(f13);

            if (supportsVulkan14) {
                chainFeature(f14);
            }

            const bool hasAccelerationStructureExtension = hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            const bool hasRayQueryExtension = hasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            const bool hasExtendedDynamicState3Extension = hasExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);

            if (hasMeshExtension) {
                chainFeature(meshFeatures);
            }

            if (hasAccelerationStructureExtension) {
                chainFeature(accelerationStructureFeatures);
            }

            if (hasRayTracingPipelineExtension) {
                chainFeature(rayTracingPipelineFeatures);
            }

            if (hasRayQueryExtension) {
                chainFeature(rayQuery);
            }

            if (hasExtendedDynamicState3Extension) {
                chainFeature(dynamicStateFeatures);
            }

            vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

            const VkPhysicalDeviceFeatures &core = deviceFeatures.features;

            caps.Features.SamplerAnisotropy = core.samplerAnisotropy;
            caps.Features.IndependentBlend = core.independentBlend;
            caps.Features.MultiDrawIndirect = core.multiDrawIndirect;
            caps.Features.ShaderInt16 = core.shaderInt16;
            caps.Features.ShaderInt64 = core.shaderInt64;
            caps.Features.ShaderFloat64 = core.shaderFloat64;
            caps.Features.FragmentStoresAndAtomics = core.fragmentStoresAndAtomics;
            caps.Features.ShaderStorageImageMultisample = core.shaderStorageImageMultisample;
            caps.Features.TextureCompressionBC = core.textureCompressionBC;
            caps.Features.PipelineStatisticsQuery = core.pipelineStatisticsQuery;

            caps.Features.ShaderInt8 = f12.shaderInt8;
            caps.Features.ShaderFloat16 = f12.shaderFloat16;
            caps.Features.DrawIndirectCount = f12.drawIndirectCount;
            caps.Features.HostQueryReset = f12.hostQueryReset;
            caps.Features.TimelineSemaphore = f12.timelineSemaphore;
            caps.Features.BufferDeviceAddress = f12.bufferDeviceAddress;
            caps.Features.ScalarBlockLayout = f12.scalarBlockLayout;
            caps.Features.SamplerFilterMinmax = f12.samplerFilterMinmax;
            caps.Features.StorageBuffer8BitAccess = f12.storageBuffer8BitAccess;
            caps.Features.VulkanMemoryModel = f12.vulkanMemoryModel;

            caps.Features.DynamicRendering = f13.dynamicRendering;
            caps.Features.Synchronization2 = f13.synchronization2;
            caps.Features.Maintenance4 = f13.maintenance4;
            caps.Features.ShaderDemoteToHelperInvocation = f13.shaderDemoteToHelperInvocation;
            caps.Features.SubgroupSizeControl = f13.subgroupSizeControl;
            caps.Features.ComputeFullSubgroups = f13.computeFullSubgroups;

            // Vulkan12Features::descriptorIndexing only signals the promoted minimum,
            // so each bit is recorded separately. Features.Bindless is the aggregate
            // this engine's heap depends on; a caller wanting a narrower heap should
            // test the individual bits instead.
            VulkanDeviceDescriptorCapabilities &di = caps.Descriptors;

            di.DescriptorIndexing = f12.descriptorIndexing;
            di.RuntimeDescriptorArray = f12.runtimeDescriptorArray;
            di.PartiallyBound = f12.descriptorBindingPartiallyBound;
            di.VariableDescriptorCount = f12.descriptorBindingVariableDescriptorCount;
            di.UpdateUnusedWhilePending = f12.descriptorBindingUpdateUnusedWhilePending;

            di.UpdateAfterBindSampledImages = f12.descriptorBindingSampledImageUpdateAfterBind;
            di.UpdateAfterBindStorageImages = f12.descriptorBindingStorageImageUpdateAfterBind;
            di.UpdateAfterBindStorageBuffers = f12.descriptorBindingStorageBufferUpdateAfterBind;
            di.UpdateAfterBindUniformBuffers = f12.descriptorBindingUniformBufferUpdateAfterBind;

            // Not a copy-paste of the line above: VK_DESCRIPTOR_TYPE_SAMPLER has no bit of its own,
            // the spec gates it on descriptorBindingSampledImageUpdateAfterBind.
            di.UpdateAfterBindSamplers = f12.descriptorBindingSampledImageUpdateAfterBind;

            di.SampledImageNonUniformIndexing = f12.shaderSampledImageArrayNonUniformIndexing;
            di.StorageImageNonUniformIndexing = f12.shaderStorageImageArrayNonUniformIndexing;
            di.StorageBufferNonUniformIndexing = f12.shaderStorageBufferArrayNonUniformIndexing;
            di.UniformBufferNonUniformIndexing = f12.shaderUniformBufferArrayNonUniformIndexing;

            caps.Features.Bindless = di.DescriptorIndexing && di.RuntimeDescriptorArray && di.PartiallyBound && di.VariableDescriptorCount &&
                                     di.UpdateUnusedWhilePending && di.UpdateAfterBindSampledImages && di.UpdateAfterBindStorageImages &&
                                     di.UpdateAfterBindStorageBuffers && di.SampledImageNonUniformIndexing && di.StorageImageNonUniformIndexing &&
                                     di.StorageBufferNonUniformIndexing;

            caps.Features.MeshShader = hasMeshExtension && meshFeatures.meshShader;

            caps.Features.RayTracingAccelerationStructure = hasAccelerationStructureExtension && accelerationStructureFeatures.accelerationStructure;

            caps.Features.RayTracingPipeline =
                hasRayTracingPipelineExtension && rayTracingPipelineFeatures.rayTracingPipeline && caps.Features.RayTracingAccelerationStructure;

            caps.Features.RayQuery = hasRayQueryExtension && rayQuery.rayQuery && caps.Features.RayTracingAccelerationStructure;

            caps.Features.ExtendedDynamicState3 = hasExtendedDynamicState3Extension && dynamicStateFeatures.extendedDynamicState3PolygonMode;

            caps.MeshShaders.Supported = caps.Features.MeshShader;
            caps.MeshShaders.TaskShader = caps.Features.MeshShader && meshFeatures.taskShader;
            caps.MeshShaders.MultiviewMeshShader = caps.Features.MeshShader && meshFeatures.multiviewMeshShader;

            caps.RayTracing.AccelerationStructure = caps.Features.RayTracingAccelerationStructure;
            caps.RayTracing.Pipeline = caps.Features.RayTracingPipeline;
            caps.RayTracing.RayQuery = caps.Features.RayQuery;

            // ----------------------------------------------------------------------------------------------------
            // Query Physical device memory properties.
            // ----------------------------------------------------------------------------------------------------
            VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget{};
            memoryBudget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

            VkPhysicalDeviceMemoryProperties2 memoryProperties{};
            memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

            caps.Memory.HasMemoryBudget = hasExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

            if (caps.Memory.HasMemoryBudget) {
                memoryProperties.pNext = &memoryBudget;
            }

            vkGetPhysicalDeviceMemoryProperties2(device, &memoryProperties);

            const VkPhysicalDeviceMemoryProperties &mem = memoryProperties.memoryProperties;

            caps.Memory.Heaps.resize(mem.memoryHeapCount);

            for (u32 i = 0; i < mem.memoryHeapCount; ++i) {
                auto &dst = caps.Memory.Heaps[i];

                dst.Size = mem.memoryHeaps[i].size;
                dst.Flags = mem.memoryHeaps[i].flags;

                if (caps.Memory.HasMemoryBudget) {
                    dst.Budget = memoryBudget.heapBudget[i];
                    dst.Usage = memoryBudget.heapUsage[i];
                }

                if ((dst.Flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
                    caps.Memory.DeviceLocalBytes += dst.Size;
                    caps.Memory.DeviceLocalBudgetBytes += caps.Memory.HasMemoryBudget ? dst.Budget : dst.Size;
                }
            }

            caps.Memory.Types.resize(mem.memoryTypeCount);

            // Several memory types share one heap. Accumulate per heap, not per type,
            // or host-visible size is reported at 3-4x reality on most drivers.
            u32 hostVisibleHeapMask = 0;

            for (u32 i = 0; i < mem.memoryTypeCount; ++i) {
                const auto &type = mem.memoryTypes[i];

                caps.Memory.Types[i].HeapIndex = type.heapIndex;
                caps.Memory.Types[i].Properties = type.propertyFlags;

                if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) hostVisibleHeapMask |= (1u << type.heapIndex);
            }

            for (u32 i = 0; i < mem.memoryHeapCount; ++i) {
                if ((hostVisibleHeapMask & (1u << i)) != 0) caps.Memory.HostVisibleBytes += mem.memoryHeaps[i].size;
            }

            // ----------------------------------------------------------------------------------------------------
            // Query Physical device queue family properties.
            // ----------------------------------------------------------------------------------------------------
            u32 queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, nullptr);

            RendererVector<VkQueueFamilyProperties2> queueProperties(queueFamilyCount);

            for (auto &property : queueProperties) {
                property.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
                property.pNext = nullptr;
            }

            vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, queueProperties.data());

            caps.Queues.Families.resize(queueFamilyCount);

            for (u32 i = 0; i < queueFamilyCount; ++i) {
                const VkQueueFlags flags = queueProperties[i].queueFamilyProperties.queueFlags;
                auto &dst = caps.Queues.Families[i];

                dst.FamilyIndex = i;
                dst.Flags = flags;
                dst.QueueCount = queueProperties[i].queueFamilyProperties.queueCount;
                dst.TimestampValidBits = queueProperties[i].queueFamilyProperties.timestampValidBits;

                dst.SupportsGraphics = (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
                dst.SupportsCompute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;
                dst.SupportsSparseBinding = (flags & VK_QUEUE_SPARSE_BINDING_BIT) != 0;

                // Graphics and compute families support transfer implicitly whether or
                // not VK_QUEUE_TRANSFER_BIT is advertised.
                dst.SupportsTransfer = (flags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0;

                if (VK_NULL_HANDLE != mVkSurface) {
                    VkBool32 presentSupported = VK_FALSE;

                    // The candidate, not mVkPhysicalDevice: selection is what decides that, and it
                    // is still VK_NULL_HANDLE here.
                    if (VK_SUCCESS == vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mVkSurface, &presentSupported)) {
                        dst.SupportsPresent = presentSupported == VK_TRUE;
                    }
                }
            }

            // Prefer dedicated families so async compute/transfer actually lands on a
            // different hardware queue rather than aliasing the graphics family.
            for (const auto &q : caps.Queues.Families) {
                if (q.SupportsGraphics && caps.Queues.GraphicsFamily == kInvalidQueueFamilyIndex) {
                    caps.Queues.GraphicsFamily = q.FamilyIndex;
                }

                if (q.SupportsCompute && !q.SupportsGraphics) {
                    if (caps.Queues.ComputeFamily == kInvalidQueueFamilyIndex || !caps.Queues.HasDedicatedComputeQueue) {
                        caps.Queues.ComputeFamily = q.FamilyIndex;
                    }

                    caps.Queues.HasDedicatedComputeQueue = true;
                }

                if (q.SupportsTransfer && !q.SupportsGraphics && !q.SupportsCompute) {
                    if (caps.Queues.TransferFamily == kInvalidQueueFamilyIndex || !caps.Queues.HasDedicatedTransferQueue) {
                        caps.Queues.TransferFamily = q.FamilyIndex;
                    }

                    caps.Queues.HasDedicatedTransferQueue = true;
                }
            }

            for (const auto &q : caps.Queues.Families) {
                if (caps.Queues.ComputeFamily == kInvalidQueueFamilyIndex && q.SupportsCompute) {
                    caps.Queues.ComputeFamily = q.FamilyIndex;
                }

                if (caps.Queues.TransferFamily == kInvalidQueueFamilyIndex && q.SupportsTransfer) {
                    caps.Queues.TransferFamily = q.FamilyIndex;
                }

                // Prefer a present family that also does graphics, to avoid a queue
                // ownership transfer on every swapchain image. Stated as a direct
                // comparison against the incumbent rather than via a proxy condition.
                if (q.SupportsPresent) {
                    if (caps.Queues.PresentFamily == kInvalidQueueFamilyIndex) {
                        caps.Queues.PresentFamily = q.FamilyIndex;
                    } else {
                        const auto &incumbent = caps.Queues.Families[caps.Queues.PresentFamily];

                        if (q.SupportsGraphics && !incumbent.SupportsGraphics) caps.Queues.PresentFamily = q.FamilyIndex;
                    }
                }
            }

            caps.Queues.SupportsAsyncCompute = caps.Queues.HasDedicatedComputeQueue && caps.Queues.ComputeFamily != caps.Queues.GraphicsFamily;

            caps.Queues.SupportsAsyncTransfer = caps.Queues.HasDedicatedTransferQueue && caps.Queues.TransferFamily != caps.Queues.GraphicsFamily;

            capabilities.push_back(std::move(caps));
        }

        // ----------------------------------------------------------------------------------------------------
        // Report every candidate, then keep the highest scoring suitable one.
        // ----------------------------------------------------------------------------------------------------
        usize selected = capabilities.size();
        u64 selectedScore = 0;

        for (usize i = 0; i < capabilities.size(); ++i) {
            const VulkanDeviceCapabilities &candidate = capabilities[i];

#if defined(VE_DEBUG)
            logCapabilities(candidate);
#endif

            if (!isDeviceSuitable(candidate)) {
                VWARN("Physical device does not meet the engine's requirements, skipping: {}", static_cast<const char *>(candidate.Identity.DeviceName));

                continue;
            }

            const u64 score = scoreDevice(candidate);

            if (selected == capabilities.size() || score > selectedScore) {
                selected = i;
                selectedScore = score;
            }
        }

        if (selected == capabilities.size()) {
            VERROR("None of the {} enumerated physical device(s) meet the engine's requirements.", capabilities.size());

            return StatusCode::NoSuitablePhysicalDeviceFound;
        }

        mVkPhysicalDevice = devices[selected];
        mCapabilities = std::move(capabilities[selected]);
        mCapabilities.Normalize();

        VINFO("Selected physical device: {} (score {})", static_cast<const char *>(mCapabilities.Identity.DeviceName), selectedScore);

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createLogicalDevice() {
        RendererVector<u32> uniqueFamilies;
        f32 priority{ 1.0f };

        const auto addFamily = [&](const u32 familyIndex) {
            if (kInvalidQueueFamilyIndex == familyIndex) {
                return;
            }

            if (std::ranges::find(uniqueFamilies, familyIndex) != uniqueFamilies.end()) {
                return;
            }

            uniqueFamilies.push_back(familyIndex);
        };

        addFamily(mCapabilities.Queues.GraphicsFamily);
        addFamily(mCapabilities.Queues.ComputeFamily);
        addFamily(mCapabilities.Queues.TransferFamily);
        addFamily(mCapabilities.Queues.PresentFamily);

        RendererVector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueFamilies.size());

        for (const auto familyIndex : uniqueFamilies) {
            VkDeviceQueueCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueCount = 1;
            info.queueFamilyIndex = familyIndex;
            info.pQueuePriorities = &priority;

            queueCreateInfos.push_back(info);
        }

        VkPhysicalDeviceVulkan11Features enabledVk11Features{};
        enabledVk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        // Required by SPIR-V's DrawParameters capability, which Slang emits for HLSL-style zero-based SV_VertexID/SV_InstanceID.
        enabledVk11Features.shaderDrawParameters = VK_TRUE;

        VkPhysicalDeviceVulkan12Features enabledVk12Features{};
        enabledVk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        enabledVk12Features.pNext = &enabledVk11Features;
        enabledVk12Features.descriptorIndexing = VK_TRUE;
        enabledVk12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        enabledVk12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        enabledVk12Features.runtimeDescriptorArray = VK_TRUE;
        enabledVk12Features.bufferDeviceAddress = VK_TRUE;
        enabledVk12Features.timelineSemaphore = VK_TRUE;

        VkPhysicalDeviceVulkan13Features enabledVk13Features{};
        enabledVk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        enabledVk13Features.pNext = &enabledVk12Features;
        enabledVk13Features.synchronization2 = VK_TRUE;
        enabledVk13Features.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceFeatures enabledVk10Features{};
        enabledVk10Features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &enabledVk13Features;
        deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<u32>(DeviceRequirements::REQUIRED_EXTENSIONS.size());
        deviceCreateInfo.ppEnabledExtensionNames = DeviceRequirements::REQUIRED_EXTENSIONS.data();
        deviceCreateInfo.pEnabledFeatures = &enabledVk10Features;

        // Create the logical device.
        VE_VK_CHECK(vkCreateDevice(mVkPhysicalDevice, &deviceCreateInfo, mHostAllocator.Callbacks(), &mVkDevice), StatusCode::FailedToCreateLogicalDevice);

        // Load device level function pointers.
        // NOTE: volkLoadDevice is recommended for applications that only use 1 vkDevice: https://github.com/zeux/volk#optimizing-device-calls
        volkLoadDevice(mVkDevice);

        // Create the graphics, compute and transfer queues.
        std::optional<VulkanQueue> graphicsQueue = VulkanQueue::Get(this, QueueType::Graphics, mCapabilities.Queues.GraphicsFamily, 0, &mHostAllocator);
        std::optional<VulkanQueue> computeQueue = VulkanQueue::Get(this, QueueType::Compute, mCapabilities.Queues.ComputeFamily, 0, &mHostAllocator);
        std::optional<VulkanQueue> transferQueue = VulkanQueue::Get(this, QueueType::Transfer, mCapabilities.Queues.TransferFamily, 0, &mHostAllocator);

        if (!graphicsQueue.has_value()) return StatusCode::FailedToGetVulkanGraphicsQueue;
        if (!computeQueue.has_value()) return StatusCode::FailedToGetVulkanComputeQueue;
        if (!transferQueue.has_value()) return StatusCode::FailedToGetVulkanTransferQueue;

        mGraphicsQueue = std::move(graphicsQueue.value());
        mComputeQueue = std::move(computeQueue.value());
        mTransferQueue = std::move(transferQueue.value());

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::initializeVulkanMemoryAllocator() {
        const VmaDeviceMemoryCallbacks deviceMemoryCallbacks{
            .pfnAllocate = &onVmaDeviceMemoryAllocate,
            .pfnFree = &onVmaDeviceMemoryFree,
            .pUserData = nullptr,
        };

        VmaVulkanFunctions vmaFunctionInfo{};
        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCreateInfo.physicalDevice = mVkPhysicalDevice;
        allocatorCreateInfo.device = mVkDevice;
        allocatorCreateInfo.pAllocationCallbacks = mHostAllocator.Callbacks();
        allocatorCreateInfo.pDeviceMemoryCallbacks = &deviceMemoryCallbacks;
        allocatorCreateInfo.pVulkanFunctions = &vmaFunctionInfo;
        allocatorCreateInfo.instance = mVkInstance;
        allocatorCreateInfo.vulkanApiVersion = mCapabilities.Identity.ApiVersion;

        // Try to load Vulkan functions from Volk.
        VE_VK_CHECK(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vmaFunctionInfo), StatusCode::FailedToLoadVulkanMemoryAllocatorFunctionsFromVolk);

        // Try to initialize VMA allocator instance.
        VE_VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &mVmaAllocator), StatusCode::FailedToInitializeVulkanMemoryAllocator);

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createShaders() {
        const std::optional<std::vector<std::byte>> vertexShaderBytes = ReadBytesFromFile("assets/shaders/triangle.vert.spv");
        const std::optional<std::vector<std::byte>> fragmentShaderBytes = ReadBytesFromFile("assets/shaders/triangle.frag.spv");

        if (!vertexShaderBytes.has_value() || !fragmentShaderBytes.has_value()) {
            return StatusCode::FailedToReadSpirvShader;
        }

        // Create Shader modules.
        // Vertex shader module.
        VkShaderModuleCreateInfo vertexShaderModule{};
        vertexShaderModule.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertexShaderModule.codeSize = (*vertexShaderBytes).size();
        vertexShaderModule.pCode = reinterpret_cast<const u32 *>((*vertexShaderBytes).data());
        VE_VK_CHECK(vkCreateShaderModule(mVkDevice, &vertexShaderModule, mHostAllocator.Callbacks(), &mVkVertexShaderModule),
                    StatusCode::FailedToCreateVulkanShaderModule);

        // Fragment shader module.
        VkShaderModuleCreateInfo fragmentShaderModule{};
        fragmentShaderModule.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragmentShaderModule.codeSize = (*fragmentShaderBytes).size();
        fragmentShaderModule.pCode = reinterpret_cast<const u32 *>((*fragmentShaderBytes).data());
        VE_VK_CHECK(vkCreateShaderModule(mVkDevice, &fragmentShaderModule, mHostAllocator.Callbacks(), &mVkFragmentShaderModule),
                    StatusCode::FailedToCreateVulkanShaderModule);

        // Create Pipeline layout.
        VkPipelineLayoutCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineCreateInfo.setLayoutCount = 0;
        pipelineCreateInfo.pushConstantRangeCount = 0;
        VE_VK_CHECK(vkCreatePipelineLayout(mVkDevice, &pipelineCreateInfo, mHostAllocator.Callbacks(), &mVkPipelineLayout),
                    StatusCode::FailedToCreateVulkanPipelineLayout);

        // Create pipeline shader stages.
        VkPipelineShaderStageCreateInfo vertexShaderStage{};
        vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStage.module = mVkVertexShaderModule;
        // Slang always names the SPIR-V entry point "main" regardless of the source function name.
        vertexShaderStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStage{};
        fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStage.module = mVkFragmentShaderModule;
        fragmentShaderStage.pName = "main";

        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertexShaderStage, fragmentShaderStage };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = VK_TRUE;
        depthStencilInfo.depthWriteEnable = VK_TRUE;
        depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilInfo.stencilTestEnable = VK_TRUE;

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = nullptr;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState attachState{};
        attachState.blendEnable = VK_FALSE;
        attachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = &attachState;

        RendererVector<VkDynamicState> dynamicState{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<u32>(dynamicState.size());
        dynamicStateInfo.pDynamicStates = dynamicState.data();

        VkPipelineRenderingCreateInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachmentFormats = &SWAPCHAIN_FORMAT;
        renderInfo.depthAttachmentFormat = DEPTH_FORMAT;

        VkGraphicsPipelineCreateInfo graphicsPipeline{};
        graphicsPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipeline.pNext = &renderInfo;
        graphicsPipeline.stageCount = static_cast<u32>(shaderStages.size());
        graphicsPipeline.pStages = shaderStages.data();
        graphicsPipeline.pVertexInputState = &vertexInputInfo;
        graphicsPipeline.pInputAssemblyState = &inputAssemblyInfo;
        graphicsPipeline.pViewportState = &viewportInfo;
        graphicsPipeline.pRasterizationState = &rasterInfo;
        graphicsPipeline.pMultisampleState = &multisampleInfo;
        graphicsPipeline.pDepthStencilState = &depthStencilInfo;
        graphicsPipeline.pColorBlendState = &blendInfo;
        graphicsPipeline.pDynamicState = &dynamicStateInfo;
        graphicsPipeline.layout = mVkPipelineLayout;
        graphicsPipeline.renderPass = VK_NULL_HANDLE;

        // TODO: Use pipeline cache as the second argument.
        VE_VK_CHECK(vkCreateGraphicsPipelines(mVkDevice, nullptr, 1, &graphicsPipeline, mHostAllocator.Callbacks(), &mVkGraphicsPipeline),
                    StatusCode::FailedToCreateVulkanGraphicsPipeline);

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createSynchronizationResources() {
        // VkSemaphoreTypeCreateInfo timelineSemaphoreTypeInfo{};
        // timelineSemaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        // timelineSemaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        // timelineSemaphoreTypeInfo.initialValue = 2; // TODO: Make this framse in flight count;
        //
        // VkSemaphoreCreateInfo timelineSemaphoreInfo{};
        // timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        // timelineSemaphoreInfo.pNext = &timelineSemaphoreTypeInfo;
        //
        // VE_VK_CHECK(vkCreateSemaphore(mVkDevice, &timelineSemaphoreInfo, mHostAllocator.Callbacks(), &mVkTimelineSemaphore),
        //             StatusCode::FailedToCreateVulkanTimelineSemaphore);

        for (FrameResources &resources : mFrameResources) {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VE_VK_CHECK(vkCreateSemaphore(mVkDevice, &semaphoreInfo, mHostAllocator.Callbacks(), &resources.ImageAcquiredSemaphore),
                        StatusCode::FailedToCreateVulkanImageAcquisitionSemaphore);
        }

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createCommandBuffers() {
        for (FrameResources &resources : mFrameResources) {
            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.queueFamilyIndex = mCapabilities.Queues.GraphicsFamily;

            VE_VK_CHECK(vkCreateCommandPool(mVkDevice, &commandPoolInfo, mHostAllocator.Callbacks(), &resources.CommandPool),
                        StatusCode::FailedToCreateCommandPool);

            VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
            cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufferAllocateInfo.commandPool = resources.CommandPool;
            cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufferAllocateInfo.commandBufferCount = 1;

            VE_VK_CHECK(vkAllocateCommandBuffers(mVkDevice, &cmdBufferAllocateInfo, &resources.CommandBuffer), StatusCode::FailedToAllocateCommandBuffer);
        }

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::createSwapchain() {
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};

        VE_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mVkPhysicalDevice, mVkSurface, &surfaceCapabilities),
                    StatusCode::FailedToQueryPhysicalDeviceSurfaceCapabilities);

        u32 surfaceFormatCount = 0;
        VE_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mVkPhysicalDevice, mVkSurface, &surfaceFormatCount, nullptr),
                    StatusCode::FailedToQueryPhysicalDeviceSurfaceFormats);
        RendererVector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        VE_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mVkPhysicalDevice, mVkSurface, &surfaceFormatCount, surfaceFormats.data()),
                    StatusCode::FailedToQueryPhysicalDeviceSurfaceFormats);

        const bool surfaceFormatSupported = std::ranges::any_of(surfaceFormats, [](const VkSurfaceFormatKHR &surfaceFormat) {
            return SWAPCHAIN_FORMAT == surfaceFormat.format && VK_COLORSPACE_SRGB_NONLINEAR_KHR == surfaceFormat.colorSpace;
        });

        if (!surfaceFormatSupported) {
            VERROR("Surface does not support the required swapchain format/color space combination.");

            return StatusCode::RequiredSwapchainSurfaceFormatNotSupported;
        }

        const u32 swapchainWidth = mDeviceCreationInfo.GraphicsSettings.WindowDimensions.Width;
        const u32 swapchainHeight = mDeviceCreationInfo.GraphicsSettings.WindowDimensions.Height;

        // Try to create swapchain.
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = mVkSurface;
        swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
        swapchainCreateInfo.imageFormat = SWAPCHAIN_FORMAT;
        swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.imageExtent = VkExtent2D{ .width = swapchainWidth, .height = swapchainHeight };
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

        VE_VK_CHECK(vkCreateSwapchainKHR(mVkDevice, &swapchainCreateInfo, mHostAllocator.Callbacks(), &mVkSwapchain),
                    StatusCode::FailedToCreateVulkanSwapchain);

        // Try to get swapchain images.
        u32 swapchainImageCount = 0;
        VE_VK_CHECK(vkGetSwapchainImagesKHR(mVkDevice, mVkSwapchain, &swapchainImageCount, nullptr), StatusCode::FailedToGetVulkanSwapchainImages);
        mVkSwapchainImages.resize(swapchainImageCount);
        VE_VK_CHECK(vkGetSwapchainImagesKHR(mVkDevice, mVkSwapchain, &swapchainImageCount, mVkSwapchainImages.data()),
                    StatusCode::FailedToGetVulkanSwapchainImages);

        // Try to create swapchain image views;
        mVkSwapchainImageViews.resize(swapchainImageCount);
        for (usize i = 0; i < mVkSwapchainImages.size(); ++i) {
            VkImageViewCreateInfo imageView{};
            imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageView.image = mVkSwapchainImages[i];
            imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageView.format = SWAPCHAIN_FORMAT;
            imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageView.subresourceRange.levelCount = 1;
            imageView.subresourceRange.layerCount = 1;

            VE_VK_CHECK(vkCreateImageView(mVkDevice, &imageView, mHostAllocator.Callbacks(), &mVkSwapchainImageViews[i]),
                        StatusCode::FailedToCreateVulkanSwapchainImageView);
        }

        // Try to create render semaphores.
        mVkRenderCompleteSemaphores.resize(mVkSwapchainImages.size());
        for (auto &semaphore : mVkRenderCompleteSemaphores) {
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VE_VK_CHECK(vkCreateSemaphore(mVkDevice, &semaphoreCreateInfo, mHostAllocator.Callbacks(), &semaphore), StatusCode::FailedToCreateVulkanSemaphore);
        }

        // Try to create depth image.
        VkImageCreateInfo depthImageCreateInfo{};
        depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageCreateInfo.format = DEPTH_FORMAT;
        depthImageCreateInfo.extent = { .width = swapchainWidth, .height = swapchainHeight, .depth = 1 };
        depthImageCreateInfo.mipLevels = 1;
        depthImageCreateInfo.arrayLayers = 1;
        depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocationinfo{};
        allocationinfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocationinfo.usage = VMA_MEMORY_USAGE_AUTO;

        VE_VK_CHECK(vmaCreateImage(mVmaAllocator, &depthImageCreateInfo, &allocationinfo, &mVkDepthImage, &mVmaDepthImageAllocation, nullptr),
                    StatusCode::FailedToCreateDepthImage);

        // Try to create depth image view.
        VkImageViewCreateInfo depthImageViewCreateInfo{};
        depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthImageViewCreateInfo.image = mVkDepthImage;
        depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImageViewCreateInfo.format = DEPTH_FORMAT;
        depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewCreateInfo.subresourceRange.levelCount = 1;
        depthImageViewCreateInfo.subresourceRange.layerCount = 1;

        VE_VK_CHECK(vkCreateImageView(mVkDevice, &depthImageViewCreateInfo, mHostAllocator.Callbacks(), &mVkDepthImageView),
                    StatusCode::FailedToCreateDepthImageView);

        return StatusCode::Successful;
    }

    void VulkanContext::destroySwapchain() {
        // If the device hasn't been initialized yet,
        // we can't destroy anything.
        if (VK_NULL_HANDLE == mVkDevice) {
            return;
        }

        // Destroy swapchain image views.
        for (VkImageView swapchainImgView : mVkSwapchainImageViews) {
            vkDestroyImageView(mVkDevice, swapchainImgView, mHostAllocator.Callbacks());
        }

        mVkSwapchainImageViews.clear();

        // Destroy render-complete semaphores.
        for (VkSemaphore &semaphore : mVkRenderCompleteSemaphores) {
            vkDestroySemaphore(mVkDevice, semaphore, mHostAllocator.Callbacks());
        }

        mVkRenderCompleteSemaphores.clear();

        // Destroy the swapchain.
        if (VK_NULL_HANDLE != mVkSwapchain) {
            vkDestroySwapchainKHR(mVkDevice, mVkSwapchain, mHostAllocator.Callbacks());
            mVkSwapchain = nullptr;
        }

        // Destroy swapchain images and views.
        if (VK_NULL_HANDLE != mVkDepthImageView && VK_NULL_HANDLE != mVkDepthImage) {
            vkDestroyImageView(mVkDevice, mVkDepthImageView, mHostAllocator.Callbacks());
            vmaDestroyImage(mVmaAllocator, mVkDepthImage, mVmaDepthImageAllocation);
            mVkDepthImageView = nullptr;
        }
    }

    void VulkanContext::test() {
        const u32 frameResIndex = frameIndex++ % MaxFramesInFlight;
        const u64 signalValue = nextSignalValue++;
        const u64 waitValue = signalValue - MaxFramesInFlight;

        const VkSemaphore graphicsTimeline = mGraphicsQueue.Timeline();

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &graphicsTimeline;
        waitInfo.pValues = &waitValue;
        vkWaitSemaphores(mVkDevice, &waitInfo, UINT64_MAX);

        // now its safe to start recording commands
        FrameResources &res = mFrameResources[frameResIndex];
        vkResetCommandPool(mVkDevice, res.CommandPool, 0);

        // get the resources for this frame
        VkSemaphore imageAcquireSemaphore = mFrameResources[frameResIndex].ImageAcquiredSemaphore;

        u32 imageIndex = 0;
        VkResult acquireResult = vkAcquireNextImageKHR(mVkDevice, mVkSwapchain, UINT64_MAX, imageAcquireSemaphore, VK_NULL_HANDLE, &imageIndex);

        // handle resize and out-of-date images, may need swapchain recreate
        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            requireSwapchainRecreate = true;
            return;
        } else if (acquireResult == VK_SUBOPTIMAL_KHR) {
            // can render this frame, recreate next time around
            requireSwapchainRecreate = true;
        }

        // begin recording commands
        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(res.CommandBuffer, &cmdBeginInfo);

        // transition the color and depth images
        VkImageMemoryBarrier2 barrier1{};
        barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier1.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier1.srcAccessMask = 0;
        barrier1.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier1.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier1.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier1.image = mVkSwapchainImages[imageIndex];
        barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier1.subresourceRange.baseMipLevel = 0;
        barrier1.subresourceRange.levelCount = 1;
        barrier1.subresourceRange.baseArrayLayer = 0;
        barrier1.subresourceRange.layerCount = 1;

        VkImageMemoryBarrier2 barrier2{};
        barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier2.pNext = nullptr;
        barrier2.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        barrier2.srcAccessMask = 0;
        barrier2.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT; // both specified to control memory access at both stages (write)
        barrier2.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier2.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        barrier2.image = mVkDepthImage;
        barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier2.subresourceRange.baseMipLevel = 0;
        barrier2.subresourceRange.levelCount = 1;
        barrier2.subresourceRange.baseArrayLayer = 0;
        barrier2.subresourceRange.layerCount = 1;
        RendererVector<VkImageMemoryBarrier2> layoutBarriers{ barrier1, barrier2 };

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.imageMemoryBarrierCount = static_cast<u32>(layoutBarriers.size());
        depInfo.pImageMemoryBarriers = layoutBarriers.data();
        vkCmdPipelineBarrier2(res.CommandBuffer, &depInfo);

        // setup the attachments (color and depth) and begin rendering (dynamic)
        VkRenderingAttachmentInfo colorAttachInfo{};
        colorAttachInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachInfo.imageView = mVkSwapchainImageViews[imageIndex];
        colorAttachInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // clear the image
        colorAttachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // keep data for presentation
        colorAttachInfo.clearValue.color = { { 0.01f, 0.01f, 0.01f, 1 } };

        VkRenderingAttachmentInfo depthAttachInfo{};
        depthAttachInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachInfo.imageView = mVkDepthImageView;
        depthAttachInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;       // clear the depth data
        depthAttachInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // don't care after rendering
        depthAttachInfo.clearValue.depthStencil = { 1.0f, 0 };

        const u32 swapchainWidth = mDeviceCreationInfo.GraphicsSettings.WindowDimensions.Width;
        const u32 swapchainHeight = mDeviceCreationInfo.GraphicsSettings.WindowDimensions.Height;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = { .offset{ .x = 0, .y = 0 }, .extent{ .width = swapchainWidth, .height = swapchainHeight } };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachInfo;
        renderingInfo.pDepthAttachment = &depthAttachInfo;

        // begin dynamic rendering
        vkCmdBeginRendering(res.CommandBuffer, &renderingInfo);
        {
            // set the viewport and scissor state
            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = static_cast<f32>(swapchainWidth);
            viewport.height = static_cast<f32>(swapchainHeight);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(res.CommandBuffer, 0, 1, &viewport);

            VkRect2D scissor{ .offset{ .x = 0, .y = 0 }, .extent{ .width = swapchainWidth, .height = swapchainHeight } };
            vkCmdSetScissor(res.CommandBuffer, 0, 1, &scissor);

            // draw our triangle
            vkCmdBindPipeline(res.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mVkGraphicsPipeline);
            vkCmdDraw(res.CommandBuffer, 3, 1, 0, 0);
        }
        // end dynamic rendering
        vkCmdEndRendering(res.CommandBuffer);

        // transition the image from color attachment to presentation so we can show it
        VkImageMemoryBarrier2 presentLayoutBarrier{};
        presentLayoutBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        presentLayoutBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        presentLayoutBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        presentLayoutBarrier.dstStageMask =
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT; // presentation engine reads outside the pipeline; this just needs to be ordered last
        presentLayoutBarrier.dstAccessMask = 0;
        presentLayoutBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        presentLayoutBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentLayoutBarrier.image = mVkSwapchainImages[imageIndex];
        presentLayoutBarrier.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        VkDependencyInfo presentDepInfo{};
        presentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        presentDepInfo.imageMemoryBarrierCount = 1;
        presentDepInfo.pImageMemoryBarriers = &presentLayoutBarrier;
        vkCmdPipelineBarrier2(res.CommandBuffer, &presentDepInfo);

        vkEndCommandBuffer(res.CommandBuffer);

        // ensure swapchain image is actually viable to start color output
        VkSemaphoreSubmitInfo imageAcquireWaitInfo{};
        imageAcquireWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        imageAcquireWaitInfo.semaphore = imageAcquireSemaphore;
        imageAcquireWaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; // wait before drawing to image
                                                                                          // signal that the image can be presented

        // render work completion signal
        VkSemaphoreSubmitInfo semSignal1{};
        semSignal1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semSignal1.semaphore = mVkRenderCompleteSemaphores[imageIndex];
        semSignal1.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        // entire frame is completed (timeline)
        VkSemaphoreSubmitInfo semSignal2{};
        semSignal2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        // semSignal2.semaphore = mVkTimelineSemaphore;
        semSignal2.semaphore = graphicsTimeline;
        semSignal2.value = signalValue;
        semSignal2.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        RendererVector<VkSemaphoreSubmitInfo> semaphoreSignals{ semSignal1, semSignal2 };

        VkCommandBufferSubmitInfo cmdSubmitInfo{};
        cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdSubmitInfo.commandBuffer = res.CommandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &imageAcquireWaitInfo; // ensure the image is ready
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = static_cast<u32>(semaphoreSignals.size());
        submitInfo.pSignalSemaphoreInfos = semaphoreSignals.data();

        vkQueueSubmit2(mGraphicsQueue.Handle(), 1, &submitInfo, VK_NULL_HANDLE);

        // present the image
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mVkRenderCompleteSemaphores[imageIndex]; // render work completed semaphore
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mVkSwapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(mGraphicsQueue.Handle(), &presentInfo);
    }

} // namespace Vulkyrie
