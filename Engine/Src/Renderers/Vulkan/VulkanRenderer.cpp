#include "VulkanRenderer.h"

namespace Vkr
{
#define LOG_DONE VINFO("\tDone.")

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
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;			// Vulkan Structure Type.
        appInfo.apiVersion = VK_API_VERSION_1_3;					// Vulkan API version to use.
        appInfo.pApplicationName = appName;							// Application Name.
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// Application Version.
        appInfo.pEngineName = "Vulkyrie";							// Engine version.
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);			// Engine version.

        // Create vulkan instance.
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// Vulkan Instance create info Structure type.
        createInfo.pApplicationInfo = &appInfo;						// Application creation info.

		// Required instance extensions.
        std::vector<const char *> instanceExtensions;
        instanceExtensions.reserve(3);
        instanceExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        mPlatform->AddRequiredVulkanExtensions(instanceExtensions);

#if defined(_DEBUG)
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		// Supported extensions count.
        u32 extensionCount = 0;
		// Get supported extension count.
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		// Initialize array to hols a list of supported instance extensions.
		VkExtensionProperties extensions[extensionCount];
		// Populate the array of supported instance extensions.
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

		// Check if required extensions are supported or not.
        for (const auto &reqExt : instanceExtensions)
        {
            VDEBUG("Searching for required instance extension: %s.", reqExt)
            bool found = false;

            for (const auto &extension : extensions)
            {
                if (strcmp(reqExt, extension.extensionName) == 0)
                {
                    found = true;
					VINFO("\tFound.")
                    break;
                }
            }

			// Exit if required extensions are not supported.
            if (!found)
            {
                VFATAL("Required extension is missing: %s", reqExt)
                return StatusCode::VulkanInstanceExtensionNotFound;
            }
        }

        createInfo.enabledExtensionCount = instanceExtensions.size();		// Enabled extension count.
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();		// Enabled extension names.

        // If validation should be done, get a list of the required validation layer names
        // and make sure they exist. Validation layers should only be enabled on non-release builds.

#if defined(_DEBUG)
        // Validation layers.
        // The list of required validation layers for the engine.
        std::vector<const char *> requiredValidationLayerNames;
        requiredValidationLayerNames.reserve(1);
        requiredValidationLayerNames.emplace_back("VK_LAYER_KHRONOS_validation");

		// Available validation layer count.
		u32 availableLayersCount = 0;
        // Get the number of available validation layers.
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr))
        // Initialize an array to hold the validation layers.
		VkLayerProperties availableLayers[availableLayersCount];
		// Obtain a list of available validation layers;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers))

        // Verify all required layers are available.
        for (const auto &layer : requiredValidationLayerNames)
        {
            VDEBUG("Searching for validation layer: %s.", layer)
            bool found = false;

            for (const auto &availableLayer : availableLayers)
            {
                if (strcmp(layer, availableLayer.layerName) == 0)
                {
                    found = true;
                    VINFO("\tFound.")
                    break;
                }
            }

			// Exit if required validation layers are not supported.
            if (!found)
            {
                VFATAL("Required validation layer is missing: %s", layer)
                return StatusCode::VulkanRequiredValidationLayersMissing;
            }
        }

        createInfo.enabledLayerCount = requiredValidationLayerNames.size();			// Enabled validation layer count.
        createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();		// Enabled validation layer names.
#endif

		// Create Vulkan instance and ensure that it is created successfully.
        VDEBUG("Creating Vulkan instance.")
        VK_CHECK(vkCreateInstance(&createInfo, mAllocator, &mInstance))
		LOG_DONE

        // Debugger
#if defined(_DEBUG)
        VDEBUG("Creating Vulkan debugger.")

        u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = logSeverity;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");

        VASSERT_MSG(func, "Failed to create debug messenger!")
        VK_CHECK(func(mInstance, &debugCreateInfo, mAllocator, &mDebugMessenger))
        LOG_DONE
#endif

        // Create Vulkan Surface.
        VDEBUG("Creating Vulkan surface.")
        mPlatform->CreateVulkanSurface(&mInstance, mAllocator, &surface);
        LOG_DONE

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
        VDEBUG("Destroying Swapchain.")
        DestroySwapchain();
		LOG_DONE

        DestroyDevice();

        if (surface != nullptr)
        {
        	VDEBUG("Destroying Vulkan surface.")
            vkDestroySurfaceKHR(mInstance, surface, mAllocator);
            surface = nullptr;
			LOG_DONE
        }

#if defined(_DEBUG)
        if (mDebugMessenger)
        {
            VDEBUG("Destroying Vulkan debugger.")
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
            func(mInstance, mDebugMessenger, mAllocator);
			LOG_DONE
        }
#endif

        VDEBUG("Destroying Vulkan instance.")
        vkDestroyInstance(mInstance, mAllocator);
		LOG_DONE

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

        VDEBUG("Creating logical device.")
        // NOTE: Do not create additional queues for shared indices.
        // This is why std::set is being used on the following line.
        std::unordered_set<i32> queueFamilyIndices = {mDevice.graphicsQueueIndex, mDevice.presentQueueIndex, mDevice.transferQueueIndex};
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(queueFamilyIndices.size());

        for (const auto &index : queueFamilyIndices)
        {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;				// Queue creation structure.
            info.queueFamilyIndex = index;											// Queue family index.
            info.queueCount = 1;													// Queue count.

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
        deviceFeatures.samplerAnisotropy = VK_TRUE; 								// Request anisotropy

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;				// Logical device creation structure.
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();			// Queue create info count.
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();				// Queue create info.
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;						// Device features to enable.
        deviceCreateInfo.enabledExtensionCount = 1;									// Enabled extensions count.
        const char *extensionNames[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        deviceCreateInfo.ppEnabledExtensionNames = extensionNames;					// Enabled extension name.

        // Deprecated and ignored, so pass nothing.
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;

        // Create logical device.
        VK_CHECK(vkCreateDevice(mDevice.physicalDevice, &deviceCreateInfo, mAllocator, &mDevice.logicalDevice))
        LOG_DONE

        // Get handles to queues.
		VDEBUG("Obtaining handles to queues.")
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.graphicsQueueIndex, 0, &mDevice.graphicsQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.presentQueueIndex, 0, &mDevice.presentQueue);
        vkGetDeviceQueue(mDevice.logicalDevice, mDevice.transferQueueIndex, 0, &mDevice.transferQueue);
		LOG_DONE

        return statusCode;
    }

    StatusCode VulkanRenderer::SelectPhysicalDevice()
    {
		// Initialize a variable to hold number of available physical devices that support Vulkan.
        u32 physicalDeviceCount = 0;
		// Get the number of physical devices that support Vulkan.
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr))

		// If there are no physical devices that support Vulkan, then exit with failure.
        if (physicalDeviceCount == 0)
        {
            VFATAL("No devices which support Vulkan were found.")
            return StatusCode::VulkanNoDevicesWithVulkanSupport;
        }

		// Else, initialize an array to hold the list of physical devices.
        VkPhysicalDevice physicalDevices[physicalDeviceCount];
		// Get the list of supported physical devices.
        VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices))

        // TODO: These requirements should probably be driven by engine
        // configuration.
        DeviceRequirements requirements{};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = true;
        requirements.samplerAnisotropy = true;
        requirements.discreteGpu = true;
        requirements.deviceExtensionNames.reserve(1);
        requirements.deviceExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		// Try to select the most appropriate device.
        for (const auto &device : physicalDevices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);			// Get device physical properties.

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);					// Get physical device features.

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory);	// Get memory information

            QueueFamilyInfo outQueueFamilyInfo{};
			PhysicalDeviceInfo deviceInfo{};
			deviceInfo.features = &features;
			deviceInfo.properties = &properties;
			deviceInfo.requirements = &requirements;

			// Check if physical device meets requirements.
            StatusCode statusCode = PhysicalDeviceMeetsRequirements(device, deviceInfo, &outQueueFamilyInfo);

            if (statusCode == StatusCode::Successful)
            {
#if defined(_DEBUG)
                VDEBUG("Selected device: %s", properties.deviceName)

                // GPU type, etc.
                switch (properties.deviceType)
                {
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
						VINFO("\tGPU type: Integrated")
						break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
						VINFO("\tGPU type: Discrete")
						break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
						VINFO("\tGPU type: Virtual")
						break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU:
						VINFO("\tGPU type: CPU")
						break;
					default:
						VINFO("\tGPU type: Unknown")
						break;
                }

                VINFO("\tGPU Driver version: %d.%d.%d", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion))
                VINFO("\tVulkan API version: %d.%d.%d", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion))

                // Memory information
                for (u32 j = 0; j < memory.memoryHeapCount; ++j)
                {
                    f32 memorySizeGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                    if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    {
                        VINFO("\tLocal GPU memory: %.2f GiB", memorySizeGib)
                    }
                    else
                    {
                        VINFO("\tShared System memory: %.2f GiB", memorySizeGib)
                    }
                }
#endif

                mDevice.physicalDevice = device;
                mDevice.graphicsQueueIndex = outQueueFamilyInfo.graphicsFamilyIndex;
                mDevice.presentQueueIndex = outQueueFamilyInfo.presentFamilyIndex;
                mDevice.transferQueueIndex = outQueueFamilyInfo.transferFamilyIndex;
                // NOTE: set compute index here if needed.

                // Keep a copy of properties, features and memory info for later use.
                mDevice.properties = properties;
                mDevice.features = features;
                mDevice.memory = memory;

				// We've found and selected a suitable device,
				// Stop iterating over physical devices.
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

    StatusCode VulkanRenderer::PhysicalDeviceMeetsRequirements(VkPhysicalDevice device, const PhysicalDeviceInfo &deviceInfo, QueueFamilyInfo *outQueueFamilyInfo)
    {
        // Discrete GPU?
        if (deviceInfo.requirements->discreteGpu)
        {
            if (deviceInfo.properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                VINFO("Device is not a discrete GPU, and one is required. Skipping.")
                return StatusCode::VulkanDiscreteGpuRequired;
            }
        }

		// Initialize variable to hold queue family count.
        u32 queueFamilyCount = 0;
		// Get queue family count.
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        // Initialize queue supported families array.
		VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        // Get supported queue families.
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
        // This should only happen if there is a queue that supports graphics but NOT present.
        if (outQueueFamilyInfo->presentFamilyIndex == -1)
        {
            for (i32 i = 0; i < queueFamilyCount; ++i)
            {
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent))
                if (supportsPresent)
                {
                    outQueueFamilyInfo->presentFamilyIndex = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting purposes.
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
			  deviceInfo.properties->deviceName)

        if (
				(!deviceInfo.requirements->graphics || (deviceInfo.requirements->graphics && outQueueFamilyInfo->graphicsFamilyIndex != -1)) &&
				(!deviceInfo.requirements->present || (deviceInfo.requirements->present && outQueueFamilyInfo->presentFamilyIndex != -1)) &&
				(!deviceInfo.requirements->compute || (deviceInfo.requirements->compute && outQueueFamilyInfo->computeFamilyIndex != -1)) &&
				(!deviceInfo.requirements->transfer || (deviceInfo.requirements->transfer && outQueueFamilyInfo->transferFamilyIndex != -1)))
        {
            VINFO("Device meets queue requirements.")
            VINFO("\tGraphics Family Index: %i", outQueueFamilyInfo->graphicsFamilyIndex)
            VINFO("\tPresent Family Index: %i", outQueueFamilyInfo->presentFamilyIndex)
            VINFO("\tTransfer Family Index: %i", outQueueFamilyInfo->transferFamilyIndex)
            VINFO("\tCompute Family Index: %i", outQueueFamilyInfo->computeFamilyIndex)

            // Query swapchain support.
            QuerySwapchainSupport(device);

            if (mDevice.swapchainSupport.formats.empty() || mDevice.swapchainSupport.presentModes.empty())
            {
                VINFO("Required swapchain support not present, skipping device.")
                return StatusCode::VulkanRequiredSwapchainNotSupported;
            }

            // Device extensions.
            if (!deviceInfo.requirements->deviceExtensionNames.empty())
            {
                u32 availableExtensionCount = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr))

                if (availableExtensionCount != 0)
                {
                    VkExtensionProperties availableExtensions[availableExtensionCount];
                    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions))

                    for (const auto &devExtName : deviceInfo.requirements->deviceExtensionNames)
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
            if (deviceInfo.requirements->samplerAnisotropy && !deviceInfo.features->samplerAnisotropy)
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

        // VINFO("Destroying command pools.");
        // vkDestroyCommandPool(
        //     mDevice.logicalDevice,
        //     mDevice.graphics_command_pool,
        //     mAllocator);

        // Destroy logical device
        if (mDevice.logicalDevice != nullptr)
        {
        	VDEBUG("Destroying logical device.")
            vkDestroyDevice(mDevice.logicalDevice, mAllocator);
            mDevice.logicalDevice = nullptr;
			LOG_DONE
        }

        // Physical devices are not destroyed.
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
		VDEBUG("Creating swapchain.")
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

        u32 imageCount = mDevice.swapchainSupport.capabilities.minImageCount + 1;
        if (mDevice.swapchainSupport.capabilities.maxImageCount > 0 && imageCount > mDevice.swapchainSupport.capabilities.maxImageCount)
        {
			imageCount = mDevice.swapchainSupport.capabilities.maxImageCount;
        }

        // Swapchain create info
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;														// Swapchain surface.
        swapchainCreateInfo.minImageCount = imageCount;												// Minimum images in the swapchain.
        swapchainCreateInfo.imageFormat = mSwapchain.imageFormat.format;							// Swapchain format.
        swapchainCreateInfo.imageColorSpace = mSwapchain.imageFormat.colorSpace;					// Swapchain color space
        swapchainCreateInfo.imageExtent = swapchainExtent;											// Swapchain image extents.
        swapchainCreateInfo.imageArrayLayers = 1;													// Number of extensions used by each image in the chain.
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// What attachment image will be used as.
		swapchainCreateInfo.preTransform = mDevice.swapchainSupport.capabilities.currentTransform;	// Transform to perform on swapchain images.
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// How to handle blending images withe external graphics (e.g. other windows.)
		swapchainCreateInfo.presentMode = presentationMode;											// Swapchain presentation mode.
		swapchainCreateInfo.clipped = VK_TRUE;														// Whether to clip parts of images tat are behind other windows.

		// Set up the queue family indices
        if (mDevice.graphicsQueueIndex != mDevice.presentQueueIndex)
        {
            u32 queueFamilyIndices[] = {
                (u32)mDevice.graphicsQueueIndex,
                (u32)mDevice.presentQueueIndex,
            };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;						// Image share handling.
            swapchainCreateInfo.queueFamilyIndexCount = 2;											// Number of queues to share images between.
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;							// Array of queues to share images between.
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;
            swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        }

		// If old swapchain has been destroyed and this one replaces it,
		// then link the old one to quickly hand over responsibilities.
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(mDevice.logicalDevice, &swapchainCreateInfo, mAllocator, &mSwapchain.handle))

        // Start with a zero frame index.
        mCurrentFrame = 0;

        // Images
        mSwapchain.imageCount = 0;

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, nullptr))

        mSwapchain.images.resize(mSwapchain.imageCount);
        mSwapchain.views.resize(mSwapchain.imageCount);

        VK_CHECK(vkGetSwapchainImagesKHR(mDevice.logicalDevice, mSwapchain.handle, &mSwapchain.imageCount, mSwapchain.images.data()))

        // Views
        for (u32 i = 0; i < mSwapchain.imageCount; ++i)
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = mSwapchain.images[i];										// Image to create view for.
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;									// TYpe of image for e.g. 1D, 2D, 3D etc.
            viewInfo.format = mSwapchain.imageFormat.format;							// Format of the image data.
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;			// Which aspect of image to view.
            viewInfo.subresourceRange.baseMipLevel = 0;									// Start mipmap level to view from.
            viewInfo.subresourceRange.levelCount = 1;									// Number of mipmap levels to view.
            viewInfo.subresourceRange.baseArrayLayer = 0;								// Start array levels to view from.
            viewInfo.subresourceRange.layerCount = 1;									// Number of array levels to view.

            VK_CHECK(vkCreateImageView(mDevice.logicalDevice, &viewInfo, mAllocator, &mSwapchain.views[i]))
        }

        // Depth resources
        if (!DetectDepthFormat())
        {
            mDevice.depthFormat = VK_FORMAT_UNDEFINED;
            VFATAL("Failed to find a supported format!")
        }

        // Create depth image and its view.
		ImageInfo imageInfo{};
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.width = swapchainExtent.width;
		imageInfo.height = swapchainExtent.height;
		imageInfo.format = mDevice.depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		imageInfo.createView = true;
		imageInfo.viewAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        CreateImage(imageInfo, &mSwapchain.depthAttachment);

        LOG_DONE
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

    void VulkanRenderer::CreateImage(const ImageInfo &imageInfo, VulkanImage *outImage)
    {
        // Copy params
        outImage->width = imageInfo.width;
        outImage->height = imageInfo.height;

        // Creation info.
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = imageInfo.width;
        imageCreateInfo.extent.height = imageInfo.height;
        imageCreateInfo.extent.depth = 1; 								// TODO: Support configurable depth.
        imageCreateInfo.mipLevels = 4;    								// TODO: Support mip mapping
        imageCreateInfo.arrayLayers = 1;  								// TODO: Support number of layers in the image.
        imageCreateInfo.format = imageInfo.format;
        imageCreateInfo.tiling = imageInfo.tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = imageInfo.usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;         		// TODO: Configurable sample count.
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 		// TODO: Configurable sharing mode.

        VK_CHECK(vkCreateImage(mDevice.logicalDevice, &imageCreateInfo, mAllocator, &outImage->handle))

        // Query memory requirements.
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(mDevice.logicalDevice, outImage->handle, &memoryRequirements);

        i32 memoryType = FindMemoryIndex(memoryRequirements.memoryTypeBits, imageInfo.memoryFlags);
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
        if (imageInfo.createView)
        {
            outImage->view = nullptr;
            CreateImageView(imageInfo.format, outImage, imageInfo.viewAspectFlags);
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

	void VulkanRenderer::CreateGraphicsPipeline() {
		auto vertexShaderCode = readFile("/");
		auto fragmentShaderCode = readFile("/");

		// Build Shader module to link to the graphics pipeline.

	}

	void VulkanRenderer::CreateShaderModule(const std::vector<char> &code) {

	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
        void *userData)
    {
        switch (messageSeverity)
        {
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
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
				break;
		}

        return VK_FALSE;
    }
}