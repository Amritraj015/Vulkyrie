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
        mPlatform->GetRequiredVulkanExtensions(instanceExtensions);

#if defined(_DEBUG)
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VDEBUG("Required extensions:");

        for (const auto &ext : instanceExtensions)
        {
            VDEBUG("\t%s", ext);
        }
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
                    VDEBUG("\tFound extension: %s", extension.extensionName)
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                VFATAL("Required extension is missing: %s", reqExt);
                return StatusCode::VulkanInstanceExtensionNotFound;
            }
        }

        createInfo.enabledExtensionCount = instanceExtensions.size();
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        // Validation layers.
        std::vector<const char *> requiredValidationLayerNames;

        // If validation should be done, get a list of the required validation layer names
        // and make sure they exist. Validation layers should only be enabled on non-release builds.

#if defined(_DEBUG)
        VDEBUG("Validation layers enabled. Enumerating...");

        // The list of validation layers required.
        requiredValidationLayerNames.reserve(1);
        //        requiredValidationLayerNames.emplace_back("VK_LAYER_KHRONOS_validation");

        u32 availableLayersCount = 0;

        // Obtain a list of available validation layers
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr));
        VkLayerProperties availableLayers[availableLayersCount];
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers));

        // Verify all required layers are available.
        for (const auto &layer : requiredValidationLayerNames)
        {
            VDEBUG("Searching for layer: %s...", layer);
            bool found = false;

            for (u32 i = 0; i < availableLayersCount; ++i)
            {
                if (strcmp(layer, availableLayers[i].layerName) == 0)
                {
                    found = true;
                    VDEBUG("\tFound layer: %s", layer);
                    break;
                }
            }

            if (!found)
            {
                VFATAL("Required validation layer is missing: %s", layer);
                return StatusCode::VulkanRequiredValidationLayersMissing;
            }
        }

        VDEBUG("All required validation layers are present.");
#endif

        createInfo.enabledLayerCount = requiredValidationLayerNames.size();
        createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();

        VK_CHECK(vkCreateInstance(&createInfo, mAllocator, &mInstance))
        VDEBUG("Vulkan instance created successfully.");

        // Debugger
#if defined(_DEBUG)
        VDEBUG("Creating Vulkan debugger...");

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

        VASSERT_MSG(func, "Failed to create debug messenger!");
        VK_CHECK(func(mInstance, &debugCreateInfo, mAllocator, &mDebugMessenger));
        VDEBUG("Vulkan debugger created.");
#endif

        // Create Surface
        VDEBUG("Creating Vulkan surface.");
        StatusCode statusCode = mPlatform->CreateVulkanSurface(&mInstance, mAllocator, &surface);
        ENSURE_SUCCESS(statusCode, "Failed to create platform surface!")
        VDEBUG("Vulkan surface created.");

        // Create vulkan device.
        statusCode = CreateDevice();
        ENSURE_SUCCESS(statusCode, "Failed to create logical device!")

        // Create Swapchain.
        CreateSwapchain(mFrameBufferWidth, mFrameBufferHeight);

        return statusCode;
    }

    StatusCode VulkanRenderer::Shutdown()
    {
        // Destroy in the opposite order of creation.

        // Swapchain
        VDEBUG("Destroying Swapchain...");
        DestroySwapchain();

        VDEBUG("Destroying Vulkan device...");
        DestroyDevice();

        VDEBUG("Destroying Vulkan surface...");
        if (surface)
        {
            vkDestroySurfaceKHR(mInstance, surface, mAllocator);
            surface = nullptr;
        }

#if defined(_DEBUG)
        if (mDebugMessenger)
        {
            VDEBUG("Destroying Vulkan debugger...");
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
            func(mInstance, mDebugMessenger, mAllocator);
        }
#endif

        VDEBUG("Destroying Vulkan instance...");
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

    StatusCode VulkanRenderer::CreateDevice()
    {
        StatusCode statusCode = SelectPhysicalDevice();
        ENSURE_SUCCESS(statusCode, "Failed to create device!")

        VINFO("Creating logical device...");
        // NOTE: Do not create additional queues for shared indices.
        bool presentSharesGraphicsQueue = mDevice.graphicsQueueIndex == mDevice.presentQueueIndex;
        bool transferSharesGraphicsQueue = mDevice.graphicsQueueIndex == mDevice.transferQueueIndex;

        u32 indexCount = 1;
        if (!presentSharesGraphicsQueue)
        {
            indexCount++;
        }

        if (!transferSharesGraphicsQueue)
        {
            indexCount++;
        }

        u32 indices[indexCount];
        u8 index = 0;
        indices[index++] = mDevice.graphicsQueueIndex;

        if (!presentSharesGraphicsQueue)
        {
            indices[index++] = mDevice.presentQueueIndex;
        }

        if (!transferSharesGraphicsQueue)
        {
            indices[index++] = mDevice.transferQueueIndex;
        }

        VkDeviceQueueCreateInfo queueCreateInfos[indexCount];
        for (u32 i = 0; i < indexCount; ++i)
        {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = indices[i];
            queueCreateInfos[i].queueCount = 1;

            // TODO: Enable this for a future enhancement.
            // if (indices[i] == mDevice.graphicsQueueIndex) {
            //     queueCreateInfos[i].queueCount = 2;
            // }

            queueCreateInfos[i].flags = 0;
            queueCreateInfos[i].pNext = nullptr;
            f32 queuePriority = 1.0f;
            queueCreateInfos[i].pQueuePriorities = &queuePriority;
        }

        // Request device features.
        // TODO: should be config driven
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE; // Request anisotropy

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = indexCount;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = 1;
        const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        deviceCreateInfo.ppEnabledExtensionNames = &extension_names;

        // Deprecated and ignored, so pass nothing.
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;

        // Create the device.
        VK_CHECK(vkCreateDevice(mDevice.physicalDevice, &deviceCreateInfo, mAllocator, &mDevice.logicalDevice));
        VDEBUG("Logical device created.");

        // Get queues.
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.graphicsQueueIndex, 0, &mDevice.graphicsQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.presentQueueIndex, 0, &mDevice.presentQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.transferQueueIndex, 0, &mDevice.transferQueue);

        VDEBUG("Queues obtained.");
        VDEBUG("Vulkan device created.");
        return statusCode;
    }

    StatusCode VulkanRenderer::SelectPhysicalDevice()
    {
        u32 physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr));

        if (physicalDeviceCount == 0)
        {
            VFATAL("No devices which support Vulkan were found.");
            return StatusCode::NoDevicesWithVulkanSupport;
        }

        VkPhysicalDevice physicalDevices[physicalDeviceCount];
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices));

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

        for (u32 i = 0; i < physicalDeviceCount; ++i)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memory);

            QueueFamilyInfo queueInfo = {};
            StatusCode result = PhysicalDeviceMeetsRequirements(
                physicalDevices[i],
                &properties,
                &features,
                &requirements,
                &queueInfo);

            if (result == StatusCode::Successful)
            {
                VINFO("Selected device: '%s'.", properties.deviceName);
                // GPU type, etc.
                switch (properties.deviceType)
                {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    VINFO("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    VINFO("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    VINFO("GPU type is Discrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    VINFO("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    VINFO("GPU type is CPU.");
                    break;
                }

                VINFO("GPU Driver version: %d.%d.%d",
                      VK_VERSION_MAJOR(properties.driverVersion),
                      VK_VERSION_MINOR(properties.driverVersion),
                      VK_VERSION_PATCH(properties.driverVersion));

                // Vulkan API version.
                VINFO("Vulkan API version: %d.%d.%d",
                      VK_VERSION_MAJOR(properties.apiVersion),
                      VK_VERSION_MINOR(properties.apiVersion),
                      VK_VERSION_PATCH(properties.apiVersion));

                // Memory information
                for (u32 j = 0; j < memory.memoryHeapCount; ++j)
                {
                    f32 memorySizeGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                    if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    {
                        VINFO("Local GPU memory: %.2f GiB", memorySizeGib);
                    }
                    else
                    {
                        VINFO("Shared System memory: %.2f GiB", memorySizeGib);
                    }
                }

                mDevice.physicalDevice = physicalDevices[i];
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
        if (!mDevice.physicalDevice)
        {
            VERROR("No physical devices were found which meet the requirements.");
            return StatusCode::VulkanNoPhysicalDeviceMeetsRequirements;
        }

        VINFO("Physical device selected.");

        return StatusCode::Successful;
    }

    StatusCode VulkanRenderer::PhysicalDeviceMeetsRequirements(
        VkPhysicalDevice device,
        const VkPhysicalDeviceProperties *properties,
        const VkPhysicalDeviceFeatures *features,
        const DeviceRequirements *requirements,
        QueueFamilyInfo *outQueueFamilyInfo)
    {
        // Evaluate device properties to determine if it meets the needs of our application.
        outQueueFamilyInfo->graphicsFamilyIndex = -1;
        outQueueFamilyInfo->presentFamilyIndex = -1;
        outQueueFamilyInfo->computeFamilyIndex = -1;
        outQueueFamilyInfo->transferFamilyIndex = -1;

        // Discrete GPU?
        if (requirements->discreteGpu)
        {
            if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                VINFO("Device is not a discrete GPU, and one is required. Skipping.");
                return StatusCode::DiscreteGpuRequired;
            }
        }

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

        // Look at each queue and see what queues it supports
        VINFO("Graphics | Present | Compute | Transfer | Name");
        u8 minTransferScore = 255;

        for (i32 i = 0; i < queueFamilyCount; ++i)
        {
            u8 currentTransferScore = 0;

            // Graphics queue?
            if (outQueueFamilyInfo->graphicsFamilyIndex == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                outQueueFamilyInfo->graphicsFamilyIndex = i;
                ++currentTransferScore;

                // If also a present queue, this prioritizes grouping of the 2.
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));

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
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
                if (supportsPresent)
                {
                    outQueueFamilyInfo->presentFamilyIndex = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting
                    // purposes.
                    if (outQueueFamilyInfo->presentFamilyIndex != outQueueFamilyInfo->graphicsFamilyIndex)
                    {
                        VWARN("Warning: Different queue index used for present vs graphics: %u.", i);
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
              properties->deviceName);

        if (
            (!requirements->graphics || (requirements->graphics && outQueueFamilyInfo->graphicsFamilyIndex != -1)) &&
            (!requirements->present || (requirements->present && outQueueFamilyInfo->presentFamilyIndex != -1)) &&
            (!requirements->compute || (requirements->compute && outQueueFamilyInfo->computeFamilyIndex != -1)) &&
            (!requirements->transfer || (requirements->transfer && outQueueFamilyInfo->transferFamilyIndex != -1)))
        {
            VINFO("Device meets queue requirements.");
            VTRACE("Graphics Family Index: %i", outQueueFamilyInfo->graphicsFamilyIndex);
            VTRACE("Present Family Index:  %i", outQueueFamilyInfo->presentFamilyIndex);
            VTRACE("Transfer Family Index: %i", outQueueFamilyInfo->transferFamilyIndex);
            VTRACE("Compute Family Index:  %i", outQueueFamilyInfo->computeFamilyIndex);

            // Query swapchain support.
            QuerySwapchainSupport(device);

            if (mDevice.swapchainSupport.formatCount < 1 || mDevice.swapchainSupport.presentationCount < 1)
            {
                mDevice.swapchainSupport.formats = nullptr;
                mDevice.swapchainSupport.presentModes = nullptr;

                VINFO("Required swapchain support not present, skipping device.");
                return StatusCode::RequiredSwapchainNotSupported;
            }

            // Device extensions.
            if (!requirements->deviceExtensionNames.empty())
            {
                u32 availableExtensionCount = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr));

                if (availableExtensionCount != 0)
                {
                    VkExtensionProperties availableExtensions[availableExtensionCount];
                    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions));

                    for (const auto &devExtName : requirements->deviceExtensionNames)
                    {
                        bool found = false;
                        for (u32 j = 0; j < availableExtensionCount; ++j)
                        {
                            if (strcmp(devExtName, availableExtensions[j].extensionName) == 0)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            VINFO("Required extension not found: '%s', skipping device.", devExtName);
                            return StatusCode::VulkanRequiredExtensionNotFound;
                        }
                    }
                }
            }

            // Sampler anisotropy
            if (requirements->samplerAnisotropy && !features->samplerAnisotropy)
            {
                VINFO("Device does not support samplerAnisotropy, skipping.");
                return StatusCode::SamplerAnisotropyNotSupported;
            }

            // Device meets all requirements.
            return StatusCode::Successful;
        }

        return StatusCode::PhysicalDeviceDoesNotMeetRequirements;
    }

    void VulkanRenderer::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
    {
        // Surface capabilities
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &mDevice.swapchainSupport.capabilities));

        // Surface formats
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &mDevice.swapchainSupport.formatCount, nullptr));

        if (mDevice.swapchainSupport.formatCount != 0)
        {
            // mDevice.swapchainSupport.formats.reserve(formatsCount);
            mDevice.swapchainSupport.formats = new VkSurfaceFormatKHR[mDevice.swapchainSupport.formatCount];
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &mDevice.swapchainSupport.formatCount, mDevice.swapchainSupport.formats));
        }

        // Present modes
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &mDevice.swapchainSupport.presentationCount, nullptr));

        if (mDevice.swapchainSupport.presentationCount != 0)
        {
            // mDevice.swapchainSupport.presentModes.reserve(mDevice.swapchainSupport.presentationCount);
            mDevice.swapchainSupport.presentModes = new VkPresentModeKHR[mDevice.swapchainSupport.presentationCount];
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &mDevice.swapchainSupport.presentationCount, mDevice.swapchainSupport.presentModes));
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
        VINFO("Destroying logical device...");
        if (mDevice.logicalDevice != nullptr)
        {
            vkDestroyDevice(mDevice.logicalDevice, mAllocator);
            mDevice.logicalDevice = nullptr;
        }

        // Physical devices are not destroyed.
        VINFO("Releasing physical device resources...");
        mDevice.physicalDevice = nullptr;

        mDevice.swapchainSupport.formats = nullptr;
        mDevice.swapchainSupport.presentModes = nullptr;
        mDevice.swapchainSupport.capabilities = {};

        mDevice.graphicsQueueIndex = -1;
        mDevice.presentQueueIndex = -1;
        mDevice.transferQueueIndex = -1;

        return StatusCode::Successful;
    }

    bool VulkanRenderer::DetectDepthFormat()
    {
        // Format candidates
        const u64 candidateCount = 3;
        VkFormat candidates[3] = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        };

        u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        for (u64 i = 0; i < candidateCount; ++i)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(mDevice.physicalDevice, candidates[i], &properties);

            if ((properties.linearTilingFeatures & flags) == flags)
            {
                mDevice.depthFormat = candidates[i];
                return true;
            }
            else if ((properties.optimalTilingFeatures & flags) == flags)
            {
                mDevice.depthFormat = candidates[i];
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
        for (u32 i = 0; i < mDevice.swapchainSupport.formatCount; ++i)
        {
            // Preferred formats
            if (mDevice.swapchainSupport.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && mDevice.swapchainSupport.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                mSwapchain.imageFormat = mDevice.swapchainSupport.formats[i];
                found = true;
                break;
            }
        }

        if (!found)
        {
            mSwapchain.imageFormat = mDevice.swapchainSupport.formats[0];
        }

        VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;
        for (u32 i = 0; i < mDevice.swapchainSupport.presentationCount; ++i)
        {
            if (mDevice.swapchainSupport.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentationMode = mDevice.swapchainSupport.presentModes[i];
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
        swapchainExtent.width = VCLAMP(swapchainExtent.width, min.width, max.width);
        swapchainExtent.height = VCLAMP(swapchainExtent.height, min.height, max.height);

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
            swapchainCreateInfo.pQueueFamilyIndices = 0;
        }

        swapchainCreateInfo.preTransform = mDevice.swapchainSupport.capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentationMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = 0;

        VK_CHECK(vkCreateSwapchainKHR(mDevice.logicalDevice, &swapchainCreateInfo, mAllocator, &mSwapchain.handle));

        // Start with a zero frame index.
        mCurrentFrame = 0;

        // Images
        mSwapchain.imageCount = 0;

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, nullptr));

        mSwapchain.images = new VkImage[mSwapchain.imageCount];
        mSwapchain.views = new VkImageView[mSwapchain.imageCount];

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, mSwapchain.images));

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

            VK_CHECK(vkCreateImageView(mDevice.logicalDevice, &viewInfo, mAllocator, &mSwapchain.views[i]));
        }

        // Depth resources
        if (!DetectDepthFormat())
        {
            mDevice.depthFormat = VK_FORMAT_UNDEFINED;
            VFATAL("Failed to find a supported format!");
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

        VINFO("Swapchain created successfully.");
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
            VFATAL("Failed to acquire swapchain image!");
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
            VFATAL("Failed to present swap chain image!");
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

        VK_CHECK(vkCreateImage(mDevice.logicalDevice, &imageCreateInfo, mAllocator, &outImage->handle));

        // Query memory requirements.
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(mDevice.logicalDevice, outImage->handle, &memoryRequirements);

        i32 memoryType = FindMemoryIndex(memoryRequirements.memoryTypeBits, memoryFlags);
        if (memoryType == -1)
        {
            VERROR("Required memory type not found. Image not valid.");
        }

        // Allocate memory
        VkMemoryAllocateInfo memoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryType;
        VK_CHECK(vkAllocateMemory(mDevice.logicalDevice, &memoryAllocateInfo, mAllocator, &outImage->memory));

        // Bind the memory
        VK_CHECK(vkBindImageMemory(mDevice.logicalDevice, outImage->handle, outImage->memory, 0)); // TODO: configurable memory offset.

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

        VK_CHECK(vkCreateImageView(mDevice.logicalDevice, &viewCreateInfo, mAllocator, &image->view));
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

        VWARN("Unable to find suitable memory type!");
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
            VERROR(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            VWARN(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            VINFO(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            VTRACE(callbackData->pMessage);
            break;
        }

        return VK_FALSE;
    }
}