#include "renderer/vulkan/vulkan_context.h"
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

        StatusCode getSupportedDeviceExtensions(VkPhysicalDevice device, RendererVector<VkExtensionProperties> &extensions) {
            u32 extensionCount = 0;
            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr), StatusCode::FailedToQueryVulkanDeviceExtensions);

            extensions.resize(extensionCount);

            VE_VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data()),
                        StatusCode::FailedToQueryVulkanDeviceExtensions);

            return StatusCode::Successful;
        }

        // The engine's floor, not a preference list: the frame graph records barriers through
        // synchronization2 and every pass renders through dynamic rendering, both core in 1.3.
        bool isDeviceSuitable(const VulkanDeviceCapabilities &caps) {
            return VK_API_VERSION_1_3 <= caps.EffectiveApiVersion && caps.HasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME) &&
                   kInvalidQueueFamily != caps.Queues.GraphicsFamily && kInvalidQueueFamily != caps.Queues.PresentFamily && caps.Features.DynamicRendering &&
                   caps.Features.Synchronization2;
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
        , mGraphicsQueue()
        , mTransferQueue()
        , mComputeQueue()
        , mPresentQueue()
        , mContextCreated(false) {
    }

    VulkanContext::~VulkanContext() {
        if (VK_NULL_HANDLE != mVkDevice) {
            vkDeviceWaitIdle(mVkDevice);
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

        // Initialize instance level functions only.
        // NOTE: The following will not load device specific functions pointers.
        // Those are loaded by volkLoadDevice(mVkDevice) call after logical device creation.
        volkLoadInstanceOnly(mVkInstance);

        // Try to create Window surface.
        auto *window = static_cast<GLFWwindow *>(mDeviceCreationInfo.WindowHandle.NativeWindow);
        VE_VK_CHECK(glfwCreateWindowSurface(mVkInstance, window, mHostAllocator.Callbacks(), &mVkSurface), StatusCode::FailedToCreateSurface);

        VE_RETURN_ON_FAILURE(selectSuitablePhysicalDevice());
        VE_RETURN_ON_FAILURE(createLogicalDevice());

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
            caps.EffectiveApiVersion = std::min(VK_API_VERSION_1_4, baseProperties.apiVersion);

            VE_RETURN_ON_FAILURE(getSupportedDeviceExtensions(device, caps.Extensions));

            const auto hasExtension = [&caps](const char *name) { return caps.HasExtension(name); };

            // Below 1.3 the property and feature structures chained below are not valid to pass, and
            // the device could not satisfy isDeviceSuitable anyway. Record the identity and move on.
            if (VK_API_VERSION_1_3 > caps.EffectiveApiVersion) {
                capabilities.push_back(std::move(caps));

                continue;
            }

            const bool supportsVulkan14 = VK_API_VERSION_1_4 <= caps.EffectiveApiVersion;

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
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            RendererVector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueProperties.data());

            caps.Queues.Families.resize(queueFamilyCount);

            for (u32 i = 0; i < queueFamilyCount; ++i) {
                const VkQueueFlags flags = queueProperties[i].queueFlags;
                auto &dst = caps.Queues.Families[i];

                dst.FamilyIndex = i;
                dst.Flags = flags;
                dst.QueueCount = queueProperties[i].queueCount;
                dst.TimestampValidBits = queueProperties[i].timestampValidBits;

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
                if (q.SupportsGraphics && caps.Queues.GraphicsFamily == kInvalidQueueFamily) {
                    caps.Queues.GraphicsFamily = q.FamilyIndex;
                }

                if (q.SupportsCompute && !q.SupportsGraphics) {
                    if (caps.Queues.ComputeFamily == kInvalidQueueFamily || !caps.Queues.HasDedicatedComputeQueue) {
                        caps.Queues.ComputeFamily = q.FamilyIndex;
                    }

                    caps.Queues.HasDedicatedComputeQueue = true;
                }

                if (q.SupportsTransfer && !q.SupportsGraphics && !q.SupportsCompute) {
                    if (caps.Queues.TransferFamily == kInvalidQueueFamily || !caps.Queues.HasDedicatedTransferQueue) {
                        caps.Queues.TransferFamily = q.FamilyIndex;
                    }

                    caps.Queues.HasDedicatedTransferQueue = true;
                }
            }

            for (const auto &q : caps.Queues.Families) {
                if (caps.Queues.ComputeFamily == kInvalidQueueFamily && q.SupportsCompute) {
                    caps.Queues.ComputeFamily = q.FamilyIndex;
                }

                if (caps.Queues.TransferFamily == kInvalidQueueFamily && q.SupportsTransfer) {
                    caps.Queues.TransferFamily = q.FamilyIndex;
                }

                // Prefer a present family that also does graphics, to avoid a queue
                // ownership transfer on every swapchain image. Stated as a direct
                // comparison against the incumbent rather than via a proxy condition.
                if (q.SupportsPresent) {
                    if (caps.Queues.PresentFamily == kInvalidQueueFamily) {
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
            if (kInvalidQueueFamily == familyIndex) {
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

        VkPhysicalDeviceVulkan12Features enabledVk12Features{};
        enabledVk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        enabledVk12Features.descriptorIndexing = true;
        enabledVk12Features.shaderSampledImageArrayNonUniformIndexing = true;
        enabledVk12Features.descriptorBindingVariableDescriptorCount = true;
        enabledVk12Features.runtimeDescriptorArray = true;
        enabledVk12Features.bufferDeviceAddress = true;

        VkPhysicalDeviceVulkan13Features enabledVk13Features{};
        enabledVk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        enabledVk13Features.pNext = &enabledVk12Features;
        enabledVk13Features.synchronization2 = true;
        enabledVk13Features.dynamicRendering = true;

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
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE;
        VkQueue transferQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        vkGetDeviceQueue(mVkDevice, mCapabilities.Queues.GraphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(mVkDevice, mCapabilities.Queues.ComputeFamily, 0, &computeQueue);
        vkGetDeviceQueue(mVkDevice, mCapabilities.Queues.TransferFamily, 0, &transferQueue);
        vkGetDeviceQueue(mVkDevice, mCapabilities.Queues.PresentFamily, 0, &presentQueue);

        mGraphicsQueue = VulkanQueue{ this, graphicsQueue, mCapabilities.Queues.GraphicsFamily, QueueType::Graphics };
        mTransferQueue = VulkanQueue{ this, computeQueue, mCapabilities.Queues.ComputeFamily, QueueType::Compute };
        mComputeQueue = VulkanQueue{ this, transferQueue, mCapabilities.Queues.TransferFamily, QueueType::Transfer };
        mPresentQueue = VulkanQueue{ this, presentQueue, mCapabilities.Queues.PresentFamily, QueueType::Present };

        return StatusCode::Successful;
    }

} // namespace Vulkyrie
