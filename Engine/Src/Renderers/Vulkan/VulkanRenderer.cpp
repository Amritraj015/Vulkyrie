#include "VulkanRenderer.h"

namespace Vkr
{
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
        void *userData);

    VulkanRenderer::VulkanRenderer(const std::shared_ptr<Platform> &platform)
    {
        mPlatform = platform;
    }

    StatusCode VulkanRenderer::Initialize(const char *appName)
    {
        // Information about the application.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.pApplicationName = appName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vulkyrie";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        // Create vulkan instance.
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char *> instanceExtensions;
        instanceExtensions.reserve(3);
        instanceExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        mPlatform->AddRequiredVulkanExtensions(instanceExtensions);

#if defined(_DEBUG)
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        // Get all supported extensions.
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        VkExtensionProperties extensions[extensionCount];
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

        for (const auto &reqExt : instanceExtensions)
        {
            VDEBUG("Searching for required extension: %s", reqExt)
            bool found = false;

            for (const auto &extension : extensions)
            {
                if (strcmp(reqExt, extension.extensionName) == 0)
                {
                    VINFO("\tFound extension: %s", extension.extensionName)
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                VFATAL("Required extension is missing: %s", reqExt)
                return StatusCode::VulkanInstanceExtensionNotFound;
            }
        }

        createInfo.enabledExtensionCount = instanceExtensions.size();
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        // If validation should be done, get a list of the required validation layer names
        // and make sure they exist. Validation layers should only be enabled on non-release builds.

#if defined(_DEBUG)
        VDEBUG("Validation layers enabled. Enumerating...")

        // Validation layers.
        // The list of validation layers required.
        std::vector<const char *> requiredValidationLayerNames;
        requiredValidationLayerNames.reserve(1);
        requiredValidationLayerNames.emplace_back("VK_LAYER_KHRONOS_validation");
        u32 availableLayersCount = 0;

        // Obtain a list of available validation layers
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr))
        VkLayerProperties availableLayers[availableLayersCount];
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers))

        // Verify all required layers are available.
        for (const auto &layer : requiredValidationLayerNames)
        {
            VDEBUG("Searching for layer: %s...", layer)
            bool found = false;

            for (const auto &availableLayer : availableLayers)
            {
                if (strcmp(layer, availableLayer.layerName) == 0)
                {
                    found = true;
                    VINFO("\tFound layer: %s", layer)
                    break;
                }
            }

            if (!found)
            {
                VFATAL("Required validation layer is missing: %s", layer)
                return StatusCode::VulkanRequiredValidationLayersMissing;
            }
        }

        createInfo.enabledLayerCount = requiredValidationLayerNames.size();
        createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();

        VDEBUG("All required validation layers are present.")
#endif

        VK_CHECK(vkCreateInstance(&createInfo, mAllocator, &mInstance))
        VDEBUG("Vulkan instance created successfully.")

        // Debugger
#if defined(_DEBUG)
        VDEBUG("Creating Vulkan debugger...")

        u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = logSeverity;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");

        VASSERT_MSG(func, "Failed to create debug messenger!")
        VK_CHECK(func(mInstance, &debugCreateInfo, mAllocator, &mDebugMessenger))
        VINFO("\tVulkan debugger created.")
#endif

        // Create Surface
        VDEBUG("Creating Vulkan surface.")
        mPlatform->CreateVulkanSurface(&mInstance, mAllocator, &surface);
        VDEBUG("Vulkan surface created.")

        // Create vulkan device.
        StatusCode statusCode = CreateLogicalDevice();
        ENSURE_SUCCESS(statusCode, "Failed to create logical device!")

        // Create Swapchain.
        CreateSwapchain(mFrameBufferWidth, mFrameBufferHeight);

        return statusCode;
    }

    StatusCode VulkanRenderer::Shutdown()
    {
        // Destroy in the opposite order of creation.

        // Swapchain
        VDEBUG("Destroying Swapchain...")
        DestroySwapchain();

        VDEBUG("Destroying Vulkan device...")
        DestroyDevice();

        VDEBUG("Destroying Vulkan surface...")
        if (surface)
        {
            vkDestroySurfaceKHR(mInstance, surface, mAllocator);
            surface = nullptr;
        }

#if defined(_DEBUG)
        if (mDebugMessenger)
        {
            VDEBUG("Destroying Vulkan debugger...")
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
            func(mInstance, mDebugMessenger, mAllocator);
        }
#endif

        VDEBUG("Destroying Vulkan instance...")
        vkDestroyInstance(mInstance, mAllocator);

        return StatusCode::Successful;
    }

    void VulkanRenderer::OnResize(u16 width, u16 height)
    {
    }

    StatusCode VulkanRenderer::BeginFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode VulkanRenderer::EndFrame(f32 deltaTime)
    {
        return StatusCode::Successful;
    }

    StatusCode VulkanRenderer::CreateLogicalDevice()
    {
        StatusCode statusCode = SelectPhysicalDevice();
        ENSURE_SUCCESS(statusCode, "Failed to create device!")

        VINFO("Creating logical device...")
        // NOTE: Do not create additional queues for shared indices.
        // This is why std::set is being used on the following line.
        std::unordered_set<i32> queueFamilyIndices = {mDevice.graphicsQueueIndex, mDevice.presentQueueIndex, mDevice.transferQueueIndex};
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(queueFamilyIndices.size());

        for (const auto &index : queueFamilyIndices)
        {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = index;
            info.queueCount = 1;

            // TODO: Enable this for a future enhancement.
            // if (indices[i] == mDevice.graphicsQueueIndex) {
            //     info.queueCount = 2;
            // }

            info.flags = 0;
            info.pNext = nullptr;
            f32 queuePriority = 1.0f;
            info.pQueuePriorities = &queuePriority;

            queueCreateInfos.emplace_back(info);
        }

        // Request device features.
        // TODO: should be config driven
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE; // Request anisotropy

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = 1;
        const char *extensionNames[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        deviceCreateInfo.ppEnabledExtensionNames = extensionNames;

        // Deprecated and ignored, so pass nothing.
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;

        // Create logical device.
        VK_CHECK(vkCreateDevice(mDevice.physicalDevice, &deviceCreateInfo, mAllocator, &mDevice.logicalDevice))
        VDEBUG("Logical device created.")

        // Get queues.
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.graphicsQueueIndex, 0, &mDevice.graphicsQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.presentQueueIndex, 0, &mDevice.presentQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.transferQueueIndex, 0, &mDevice.transferQueue);

        VDEBUG("Queues obtained.")
        return statusCode;
    }

    StatusCode VulkanRenderer::SelectPhysicalDevice()
    {
        u32 physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr))

        if (physicalDeviceCount == 0)
        {
            VFATAL("No devices which support Vulkan were found.")
            return StatusCode::VulkanNoDevicesWithVulkanSupport;
        }

        VkPhysicalDevice physicalDevices[physicalDeviceCount];
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices))

        // TODO: These requirements should probably be driven by engine
        // configuration.
        DeviceRequirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = true;
        requirements.samplerAnisotropy = true;
        requirements.discreteGpu = true;
        requirements.deviceExtensionNames.reserve(1);
        requirements.deviceExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        for (const auto &device : physicalDevices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory);

            QueueFamilyInfo queueInfo = {};
            StatusCode statusCode = PhysicalDeviceMeetsRequirements(device, &properties, &features, &requirements, &queueInfo);

            if (statusCode == StatusCode::Successful)
            {
                VINFO("| Selected device:\t\t | %s \t|", properties.deviceName)

                // GPU type, etc.
                switch (properties.deviceType)
                {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    VINFO("| GPU type:\t\t\t | Unknown \t\t\t|")
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    VINFO("| GPU type:\t\t\t | Integrated \t\t\t|")
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    VINFO("| GPU type:\t\t\t | Discrete \t\t\t|")
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    VINFO("| GPU type:\t\t\t | Virtual \t\t\t|")
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    VINFO("| GPU type:\t\t\t | CPU \t\t\t|")
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
                    VINFO("| GPU type:\t\t\t | Unknown \t\t\t|")
                    break;
                }

                VINFO("| GPU Driver version:\t\t | %d.%d.%d \t\t\t|",
                      VK_VERSION_MAJOR(properties.driverVersion),
                      VK_VERSION_MINOR(properties.driverVersion),
                      VK_VERSION_PATCH(properties.driverVersion))

                // Vulkan API version.
                VINFO("| Vulkan API version:\t\t | %d.%d.%d \t\t\t|",
                      VK_VERSION_MAJOR(properties.apiVersion),
                      VK_VERSION_MINOR(properties.apiVersion),
                      VK_VERSION_PATCH(properties.apiVersion))

                // Memory information
                for (u32 j = 0; j < memory.memoryHeapCount; ++j)
                {
                    f32 memorySizeGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                    if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    {
                        VINFO("| Local GPU memory:\t\t | %.2f GiB \t\t\t|", memorySizeGib)
                    }
                    else
                    {
                        VINFO("| Shared System memory:\t | %.2f GiB \t\t\t|", memorySizeGib)
                    }
                }

                mDevice.physicalDevice = device;
                mDevice.graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
                mDevice.presentQueueIndex = queueInfo.presentFamilyIndex;
                mDevice.transferQueueIndex = queueInfo.transferFamilyIndex;
                // NOTE: set compute index here if needed.

                // Keep a copy of properties, features and memory info for later use.
                mDevice.properties = properties;
                mDevice.features = features;
                mDevice.memory = memory;

                break;
            }
        }

        // Ensure a device was selected
        if (mDevice.physicalDevice == nullptr)
        {
            VERROR("No physical devices were found which meet the requirements.")
            return StatusCode::VulkanNoPhysicalDeviceMeetsRequirements;
        }

        return StatusCode::Successful;
    }

    StatusCode VulkanRenderer::PhysicalDeviceMeetsRequirements(
        VkPhysicalDevice device,
        const VkPhysicalDeviceProperties *properties,
        const VkPhysicalDeviceFeatures *features,
        const DeviceRequirements *requirements,
        QueueFamilyInfo *outQueueFamilyInfo)
    {
        // Discrete GPU?
        if (requirements->discreteGpu)
        {
            if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                VINFO("Device is not a discrete GPU, and one is required. Skipping.")
                return StatusCode::VulkanDiscreteGpuRequired;
            }
        }

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

        // Look at each queue and see what queues it supports
        VINFO("Graphics | Present | Compute | Transfer | Name")
        u8 minTransferScore = 255;

        for (i32 i = 0; i < queueFamilyCount; ++i)
        {
            u8 currentTransferScore = 0;

            // Graphics queue?
            if (outQueueFamilyInfo->graphicsFamilyIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                outQueueFamilyInfo->graphicsFamilyIndex = i;
                ++currentTransferScore;

                // If also a presentation queue, this prioritizes grouping of the 2.
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent))

                if (supportsPresent)
                {
                    outQueueFamilyInfo->presentFamilyIndex = i;
                    ++currentTransferScore;
                }
            }

            // Compute queue?
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                outQueueFamilyInfo->computeFamilyIndex = i;
                ++currentTransferScore;
            }

            // Transfer queue?
            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                // Take the index if it is the current lowest. This increases the
                // likelihood that it is a dedicated transfer queue.
                if (currentTransferScore <= minTransferScore)
                {
                    minTransferScore = currentTransferScore;
                    outQueueFamilyInfo->transferFamilyIndex = i;
                }
            }
        }

        // If a present queue hasn't been found, iterate again and take the first one.
        // This should only happen if there is a queue that supports graphics but NOT
        // present.
        if (outQueueFamilyInfo->presentFamilyIndex == -1)
        {
            for (i32 i = 0; i < queueFamilyCount; ++i)
            {
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent))
                if (supportsPresent)
                {
                    outQueueFamilyInfo->presentFamilyIndex = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting
                    // purposes.
                    if (outQueueFamilyInfo->presentFamilyIndex != outQueueFamilyInfo->graphicsFamilyIndex)
                    {
                        VWARN("Warning: Different queue index used for present vs graphics: %u.", i)
                    }
                    break;
                }
            }
        }

        // Print out some info about the device
        VINFO("       %d |       %d |       %d |        %d | %s",
              outQueueFamilyInfo->graphicsFamilyIndex != -1,
              outQueueFamilyInfo->presentFamilyIndex != -1,
              outQueueFamilyInfo->computeFamilyIndex != -1,
              outQueueFamilyInfo->transferFamilyIndex != -1,
              properties->deviceName)

        if (
            (!requirements->graphics || (requirements->graphics && outQueueFamilyInfo->graphicsFamilyIndex != -1)) &&
            (!requirements->present || (requirements->present && outQueueFamilyInfo->presentFamilyIndex != -1)) &&
            (!requirements->compute || (requirements->compute && outQueueFamilyInfo->computeFamilyIndex != -1)) &&
            (!requirements->transfer || (requirements->transfer && outQueueFamilyInfo->transferFamilyIndex != -1)))
        {
            VINFO("| Device meets queue requirements.\t|")
            VTRACE("| Graphics Family Index:\t| %i \t|", outQueueFamilyInfo->graphicsFamilyIndex)
            VTRACE("| Present Family Index:\t| %i \t|", outQueueFamilyInfo->presentFamilyIndex)
            VTRACE("| Transfer Family Index:\t| %i \t|", outQueueFamilyInfo->transferFamilyIndex)
            VTRACE("| Compute Family Index:\t| %i \t|", outQueueFamilyInfo->computeFamilyIndex)

            // Query swapchain support.
            QuerySwapchainSupport(device);

            if (mDevice.swapchainSupport.formats.empty() || mDevice.swapchainSupport.presentModes.empty())
            {
                VINFO("Required swapchain support not present, skipping device.")
                return StatusCode::VulkanRequiredSwapchainNotSupported;
            }

            // Device extensions.
            if (!requirements->deviceExtensionNames.empty())
            {
                u32 availableExtensionCount = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr))

                if (availableExtensionCount != 0)
                {
                    VkExtensionProperties availableExtensions[availableExtensionCount];
                    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions))

                    for (const auto &devExtName : requirements->deviceExtensionNames)
                    {
                        bool found = false;

                        for (const auto &extension : availableExtensions)
                        {
                            if (strcmp(devExtName, extension.extensionName) == 0)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            VINFO("Required extension not found: '%s', skipping device.", devExtName)
                            return StatusCode::VulkanRequiredExtensionNotFound;
                        }
                    }
                }
            }

            // Sampler anisotropy
            if (requirements->samplerAnisotropy && !features->samplerAnisotropy)
            {
                VINFO("Device does not support samplerAnisotropy, skipping.")
                return StatusCode::VulkanSamplerAnisotropyNotSupported;
            }

            // Device meets all requirements.
            return StatusCode::Successful;
        }

        return StatusCode::VulkanPhysicalDeviceDoesNotMeetRequirements;
    }

    void VulkanRenderer::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
    {
        // Surface capabilities
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &mDevice.swapchainSupport.capabilities))

        // Surface formats
        u32 formatCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr))

        if (formatCount != 0)
        {
            mDevice.swapchainSupport.formats.resize(formatCount);
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, mDevice.swapchainSupport.formats.data()))
        }

        // Present modes
        u32 presentationCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, nullptr))

        if (presentationCount != 0)
        {
            mDevice.swapchainSupport.presentModes.resize(presentationCount);
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount, mDevice.swapchainSupport.presentModes.data()))
        }
    }

    StatusCode VulkanRenderer::DestroyDevice()
    {
        // Unset queues
        mDevice.graphicsQueue = nullptr;
        mDevice.presentQueue = nullptr;
        mDevice.transferQueue = nullptr;

        // VINFO("Destroying command pools...");
        // vkDestroyCommandPool(
        //     mDevice.logicalDevice,
        //     mDevice.graphics_command_pool,
        //     mAllocator);

        // Destroy logical device
        VINFO("Destroying logical device...")
        if (mDevice.logicalDevice != nullptr)
        {
            vkDestroyDevice(mDevice.logicalDevice, mAllocator);
            mDevice.logicalDevice = nullptr;
        }

        // Physical devices are not destroyed.
        VINFO("Releasing physical device resources...")
        mDevice.physicalDevice = nullptr;

        mDevice.swapchainSupport.capabilities = {};

        mDevice.graphicsQueueIndex = -1;
        mDevice.presentQueueIndex = -1;
        mDevice.transferQueueIndex = -1;

        return StatusCode::Successful;
    }

    bool VulkanRenderer::DetectDepthFormat()
    {
        // Format candidates
        VkFormat candidates[3] = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        };

        u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        for (const auto &format : candidates)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(mDevice.physicalDevice, format, &properties);

            if ((properties.linearTilingFeatures & flags) == flags)
            {
                mDevice.depthFormat = format;
                return true;
            }
            else if ((properties.optimalTilingFeatures & flags) == flags)
            {
                mDevice.depthFormat = format;
                return true;
            }
        }

        return false;
    }

    void VulkanRenderer::CreateSwapchain(u32 width, u32 height)
    {
        VkExtent2D swapchainExtent = {width, height};
        mSwapchain.maxFramesInFlight = 2;

        // Choose a swap surface format.
        bool found = false;
        for (const auto &f : mDevice.swapchainSupport.formats)
        {
            // Preferred formats
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                mSwapchain.imageFormat = f;
                found = true;
                break;
            }
        }

        if (!found)
        {
            mSwapchain.imageFormat = mDevice.swapchainSupport.formats[0];
        }

        VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto &p : mDevice.swapchainSupport.presentModes)
        {
            if (p == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentationMode = p;
                break;
            }
        }

        // Re-query swapchain support.
        QuerySwapchainSupport(mDevice.physicalDevice);

        // Swapchain extent
        if (mDevice.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX)
        {
            swapchainExtent = mDevice.swapchainSupport.capabilities.currentExtent;
        }

        // Clamp to the value allowed by the GPU.
        VkExtent2D min = mDevice.swapchainSupport.capabilities.minImageExtent;
        VkExtent2D max = mDevice.swapchainSupport.capabilities.maxImageExtent;
        swapchainExtent.width = VCLAMP(swapchainExtent.width, min.width, max.width)
		swapchainExtent.height = VCLAMP(swapchainExtent.height, min.height, max.height)

        u32 image_count = mDevice.swapchainSupport.capabilities.minImageCount + 1;
        if (mDevice.swapchainSupport.capabilities.maxImageCount > 0 && image_count > mDevice.swapchainSupport.capabilities.maxImageCount)
        {
            image_count = mDevice.swapchainSupport.capabilities.maxImageCount;
        }

        // Swapchain create info
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = image_count;
        swapchainCreateInfo.imageFormat = mSwapchain.imageFormat.format;
        swapchainCreateInfo.imageColorSpace = mSwapchain.imageFormat.colorSpace;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Set up the queue family indices
        if (mDevice.graphicsQueueIndex != mDevice.presentQueueIndex)
        {
            u32 queueFamilyIndices[] = {
                (u32)mDevice.graphicsQueueIndex,
                (u32)mDevice.presentQueueIndex,
            };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;
            swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        swapchainCreateInfo.preTransform = mDevice.swapchainSupport.capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentationMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = nullptr;

        VK_CHECK(vkCreateSwapchainKHR(mDevice.logicalDevice, &swapchainCreateInfo, mAllocator, &mSwapchain.handle))

        // Start with a zero frame index.
        mCurrentFrame = 0;

        // Images
        mSwapchain.imageCount = 0;

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, nullptr))

        mSwapchain.images = new VkImage[mSwapchain.imageCount];
        mSwapchain.views = new VkImageView[mSwapchain.imageCount];

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, mSwapchain.images))

        // Views
        for (u32 i = 0; i < mSwapchain.imageCount; ++i)
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = mSwapchain.images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = mSwapchain.imageFormat.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(mDevice.logicalDevice, &viewInfo, mAllocator, &mSwapchain.views[i]))
        }

        // Depth resources
        if (!DetectDepthFormat())
        {
            mDevice.depthFormat = VK_FORMAT_UNDEFINED;
            VFATAL("Failed to find a supported format!")
        }

        // Create depth image and its view.
        CreateImage(
            VK_IMAGE_TYPE_2D,
            swapchainExtent.width,
            swapchainExtent.height,
            mDevice.depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            &mSwapchain.depthAttachment);

        VINFO("Swapchain created successfully.")
    }

    void VulkanRenderer::RecreateSwapchain(u32 width, u32 height)
    {
        DestroySwapchain();
        CreateSwapchain(width, height);
    }

    bool VulkanRenderer::AcquireNextImageIndex(u64 nanoSeconds, VkSemaphore imageAvailableSemaphore, VkFence fence, u32 *outImageIndex)
    {
        VkResult result = vkAcquireNextImageKHR(
            mDevice.logicalDevice,
            mSwapchain.handle,
            nanoSeconds,
            imageAvailableSemaphore,
            fence,
            outImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Trigger swapchain recreation, then boot out of the render loop.
            RecreateSwapchain(mFrameBufferWidth, mFrameBufferHeight);
            return false;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            VFATAL("Failed to acquire swapchain image!")
            return false;
        }

        return true;
    }

    void VulkanRenderer::Present(VkSemaphore renderCompleteSemaphore, u32 presentImageIndex)
    {
        // Return the image to the swapchain for presentation.
        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mSwapchain.handle;
        presentInfo.pImageIndices = &presentImageIndex;
        presentInfo.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(mDevice.presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            // Swapchain is out of date, suboptimal or a frame-buffer resize has occurred. Trigger swapchain recreation.
            RecreateSwapchain(mFrameBufferWidth, mFrameBufferHeight);
        }
        else if (result != VK_SUCCESS)
        {
            VFATAL("Failed to present swap chain image!")
        }
    }

    void VulkanRenderer::DestroySwapchain()
    {
        DestroyImage(&mSwapchain.depthAttachment);

        // Only destroy the views, not the images, since those are owned by the swapchain and are thus
        // destroyed when it is.
        for (u32 i = 0; i < mSwapchain.imageCount; ++i)
        {
            vkDestroyImageView(mDevice.logicalDevice, mSwapchain.views[i], mAllocator);
        }

        vkDestroySwapchainKHR(mDevice.logicalDevice, mSwapchain.handle, mAllocator);
    }

    void VulkanRenderer::CreateImage(
        VkImageType imageType,
        u32 width,
        u32 height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags,
        bool createView,
        VkImageAspectFlags viewAspectFlags,
        VulkanImage *outImage)
    {

        // Copy params
        outImage->width = width;
        outImage->height = height;

        // Creation info.
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1; // TODO: Support configurable depth.
        imageCreateInfo.mipLevels = 4;    // TODO: Support mip mapping
        imageCreateInfo.arrayLayers = 1;  // TODO: Support number of layers in the image.
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // TODO: Configurable sample count.
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO: Configurable sharing mode.

        VK_CHECK(vkCreateImage(mDevice.logicalDevice, &imageCreateInfo, mAllocator, &outImage->handle))

        // Query memory requirements.
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(mDevice.logicalDevice, outImage->handle, &memoryRequirements);

        i32 memoryType = FindMemoryIndex(memoryRequirements.memoryTypeBits, memoryFlags);
        if (memoryType == -1)
        {
            VERROR("Required memory type not found. Image not valid.")
        }

        // Allocate memory
        VkMemoryAllocateInfo memoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryType;
        VK_CHECK(vkAllocateMemory(mDevice.logicalDevice, &memoryAllocateInfo, mAllocator, &outImage->memory))

        // Bind the memory
        VK_CHECK(vkBindImageMemory(mDevice.logicalDevice, outImage->handle, outImage->memory, 0)) // TODO: configurable memory offset.

        // Create view
        if (createView)
        {
            outImage->view = nullptr;
            CreateImageView(format, outImage, viewAspectFlags);
        }
    }

    void VulkanRenderer::CreateImageView(VkFormat format, VulkanImage *image, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = image->handle;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // TODO: Make configurable.
        viewCreateInfo.format = format;
        viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

        // TODO: Make configurable
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(mDevice.logicalDevice, &viewCreateInfo, mAllocator, &image->view))
    }

    void VulkanRenderer::DestroyImage(VulkanImage *image)
    {
        if (image->view)
        {
            vkDestroyImageView(mDevice.logicalDevice, image->view, mAllocator);
            image->view = nullptr;
        }
        if (image->memory)
        {
            vkFreeMemory(mDevice.logicalDevice, image->memory, mAllocator);
            image->memory = nullptr;
        }
        if (image->handle)
        {
            vkDestroyImage(mDevice.logicalDevice, image->handle, mAllocator);
            image->handle = nullptr;
        }
    }

    i32 VulkanRenderer::FindMemoryIndex(u32 typeFilter, u32 propertyFlags) const
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(mDevice.physicalDevice, &memoryProperties);

        for (i32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            // Check each memory type to see if its bit is set to 1.
            if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            {
                return i;
            }
        }

        VWARN("Unable to find suitable memory type!")
        return -1;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
        void *userData)
    {
        switch (messageSeverity)
        {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            VERROR(callbackData->pMessage)
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            VWARN(callbackData->pMessage)
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            VINFO(callbackData->pMessage)
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            VTRACE(callbackData->pMessage)
            break;
        }

        return VK_FALSE;
    }
}