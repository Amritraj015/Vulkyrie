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
		// Create Vulkan Instance.
		StatusCode statusCode = CreateVulkanInstance(appName);
		RETURN_ON_FAIL(statusCode)

#if defined(_DEBUG)
		// Create Vulkan Debugger.
		CreateVulkanDebugger();
#endif

		// Create Platform specific Vulkan surface.
		statusCode = CreateVulkanSurface();
		RETURN_ON_FAIL(statusCode)

		// Create Logical device.
        statusCode = CreateLogicalDevice();
		RETURN_ON_FAIL(statusCode)

		// Create swapchain.
		statusCode = CreateSwapchain(mFrameBufferWidth, mFrameBufferHeight);
		RETURN_ON_FAIL(statusCode)

		CreateRenderPass();
//		CreateGraphicsPipeline();

        return statusCode;
    }

	StatusCode VulkanRenderer::CreateVulkanInstance(const char *appName) {
		// Information about the application.
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;    // Vulkan Structure Type.
		appInfo.apiVersion = VK_API_VERSION_1_3;               // Vulkan API version to use.
		appInfo.pApplicationName = appName;                    // Application Name.
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // Application Version.
		appInfo.pEngineName = "Vulkyrie";                      // Engine version.
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);      // Engine version.

		// Create vulkan instance.
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Vulkan Instance create info Structure type.
		createInfo.pApplicationInfo = &appInfo;                    // Application creation info.

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

		createInfo.enabledExtensionCount = instanceExtensions.size();   // Enabled extension count.
		createInfo.ppEnabledExtensionNames = instanceExtensions.data(); // Enabled extension names.

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

		createInfo.enabledLayerCount = requiredValidationLayerNames.size();   // Enabled validation layer count.
		createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data(); // Enabled validation layer names.
#endif

		// Create Vulkan instance and ensure that it is created successfully.
		VDEBUG("Creating Vulkan instance.")
		VK_CHECK(vkCreateInstance(&createInfo, mAllocator, &mInstance))
		LOG_DONE

		return StatusCode::Successful;
	}

	void VulkanRenderer::CreateVulkanDebugger(){
		VDEBUG("Creating Vulkan debugger.")

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
		LOG_DONE
	}

	StatusCode VulkanRenderer::CreateVulkanSurface(){
		// Create Vulkan Surface.
		VDEBUG("Creating Vulkan surface.")
		StatusCode statusCode = mPlatform->CreateVulkanSurface(&mInstance, mAllocator, &surface);
		RETURN_ON_FAIL(statusCode)
		LOG_DONE

		return StatusCode::Successful;
	}


    StatusCode VulkanRenderer::Shutdown()
    {
        // Destroy in the opposite order of creation.
		DestroyRenderPass();			// Destroy Render pass.
        DestroySwapchain();				// Destroy the swapchain.
		DestroyLogicalDevice();			// Destroy logical device.
		DestroyVulkanSurface();			// Destroy Vulkan Surface.

#if defined(_DEBUG)
		DestroyVulkanDebugger();		// Destroy Vulkan Debugger.
#endif

		DestroyVulkanInstance();		// Destroys Vulkan instance.

        return StatusCode::Successful;
    }

	void VulkanRenderer::DestroyRenderPass(){
		vkDestroyRenderPass(mDevice.logicalDevice, mRenderPass, mAllocator);
		mRenderPass = nullptr;
	}

	void VulkanRenderer::DestroySwapchain()
	{
		VDEBUG("Destroying Swapchain.")
		DestroyImage(&mSwapchain.depthAttachment);

		// Only destroy the views, not the images, since those are owned by the swapchain and are thus
		// destroyed when it is.
		for (u32 i = 0; i < mSwapchain.imageCount; ++i)
		{
			vkDestroyImageView(mDevice.logicalDevice, mSwapchain.views[i], mAllocator);
		}

		vkDestroySwapchainKHR(mDevice.logicalDevice, mSwapchain.handle, mAllocator);
		LOG_DONE
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

	void VulkanRenderer::DestroyLogicalDevice()
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
	}

	void VulkanRenderer::DestroyVulkanSurface(){
		if (surface != nullptr)
		{
			VDEBUG("Destroying Vulkan surface.")
			vkDestroySurfaceKHR(mInstance, surface, mAllocator);
			surface = nullptr;
			LOG_DONE
		}
	}

	void VulkanRenderer::DestroyVulkanDebugger(){
		if (mDebugMessenger)
		{
			VDEBUG("Destroying Vulkan debugger.")
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
			func(mInstance, mDebugMessenger, mAllocator);
			LOG_DONE
		}
	}

	void VulkanRenderer::DestroyVulkanInstance() {
		VDEBUG("Destroying Vulkan instance.")
		vkDestroyInstance(mInstance, mAllocator);
		LOG_DONE
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
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // Queue creation structure.
            info.queueFamilyIndex = index;                           // Queue family index.
            info.queueCount = 1;                                     // Queue count.

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
		deviceFeatures.depthClamp = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;   // Logical device creation structure.
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size(); // Queue create info count.
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();    // Queue create info.
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;             // Device features to enable.
        deviceCreateInfo.enabledExtensionCount = 1;                      // Enabled extensions count.
        const char *extensionNames[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        deviceCreateInfo.ppEnabledExtensionNames = extensionNames; // Enabled extension name.

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
            vkGetPhysicalDeviceProperties(device, &properties); // Get device physical properties.

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features); // Get physical device features.

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory); // Get memory information

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

    StatusCode VulkanRenderer::CreateSwapchain(u32 width, u32 height)
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
        swapchainExtent.width = VCLAMP(swapchainExtent.width, min.width, max.width);
        swapchainExtent.height = VCLAMP(swapchainExtent.height, min.height, max.height);

        u32 imageCount = mDevice.swapchainSupport.capabilities.minImageCount + 1;
        if (mDevice.swapchainSupport.capabilities.maxImageCount > 0 && imageCount > mDevice.swapchainSupport.capabilities.maxImageCount)
        {
            imageCount = mDevice.swapchainSupport.capabilities.maxImageCount;
        }

        // Swapchain create info
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;                                                     // Swapchain surface.
        swapchainCreateInfo.minImageCount = imageCount;                                            // Minimum images in the swapchain.
        swapchainCreateInfo.imageFormat = mSwapchain.imageFormat.format;                           // Swapchain format.
        swapchainCreateInfo.imageColorSpace = mSwapchain.imageFormat.colorSpace;                   // Swapchain color space
        swapchainCreateInfo.imageExtent = swapchainExtent;                                         // Swapchain image extents.
        swapchainCreateInfo.imageArrayLayers = 1;                                                  // Number of extensions used by each image in the chain.
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                      // What attachment image will be used as.
        swapchainCreateInfo.preTransform = mDevice.swapchainSupport.capabilities.currentTransform; // Transform to perform on swapchain images.
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                    // How to handle blending images withe external graphics (e.g. other windows.)
        swapchainCreateInfo.presentMode = presentationMode;                                        // Swapchain presentation mode.
        swapchainCreateInfo.clipped = VK_TRUE;                                                     // Whether to clip parts of images tat are behind other windows.

        // Set up the queue family indices
        if (mDevice.graphicsQueueIndex != mDevice.presentQueueIndex)
        {
            u32 queueFamilyIndices[] = {
                (u32)mDevice.graphicsQueueIndex,
                (u32)mDevice.presentQueueIndex,
            };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling.
            swapchainCreateInfo.queueFamilyIndexCount = 2;                     // Number of queues to share images between.
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;      // Array of queues to share images between.
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
            viewInfo.image = mSwapchain.images[i];                            // Image to create view for.
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                        // TYpe of image for e.g. 1D, 2D, 3D etc.
            viewInfo.format = mSwapchain.imageFormat.format;                  // Format of the image data.
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to view.
            viewInfo.subresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from.
            viewInfo.subresourceRange.levelCount = 1;                         // Number of mipmap levels to view.
            viewInfo.subresourceRange.baseArrayLayer = 0;                     // Start array levels to view from.
            viewInfo.subresourceRange.layerCount = 1;                         // Number of array levels to view.

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

		return StatusCode::Successful;
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
        imageCreateInfo.extent.depth = 1; // TODO: Support configurable depth.
        imageCreateInfo.mipLevels = 4;    // TODO: Support mip mapping
        imageCreateInfo.arrayLayers = 1;  // TODO: Support number of layers in the image.
        imageCreateInfo.format = imageInfo.format;
        imageCreateInfo.tiling = imageInfo.tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = imageInfo.usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // TODO: Configurable sample count.
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO: Configurable sharing mode.

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

    void VulkanRenderer::CreateGraphicsPipeline()
    {
        auto vertexShaderCode = ReadFile("/home/raj/Projects/Vulkyrie/Assets/Shaders/vert.spv");
        auto fragmentShaderCode = ReadFile("/home/raj/Projects/Vulkyrie/Assets/Shaders/frag.spv");

        // Build Shader module to link to the graphics pipeline.
		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		// Create Pipeline.
		// Vertex Stage creation information.
		VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage name.
		vertexShaderCreateInfo.module = vertexShaderModule;						// Shader module to be used by stage.
		vertexShaderCreateInfo.pName = "main";									// Entry point into shader.// 1. Vertex Stage creation information.

		// Fragment stage creation information.
		VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;			// Shader stage name.
		fragmentShaderCreateInfo.module = fragmentShaderModule;					// Shader module to be used by stage.
		fragmentShaderCreateInfo.pName = "main";								// Entry point into shader.

		// Put shader stages creation info into an array.
		// Graphics pipeline creation info requires an array of shader stage creates.
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

		// --- VERTEX INPUT ---
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;	// List of vertex binding descriptions (data spacing/stride information).
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;	// List of vertex attribute descriptions. (data format that we are using and where to bind to/from).

		// --- INPUT ASSEMBLY ---
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
		pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// Primitive type to assemble vertices as.
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;					// Allow overriding of strip" topology to start new primitives.


		// ==================================================================================
		// --- VIEWPORT AND SCISSOR ---
		// Create viewport info struct.
		VkViewport viewport{};
		viewport.x = 0.0f;					// X start coordinate.
		viewport.y = 0.0f;					// Y start coordinate.
		// TODO: COME BACK AND FINISH THIS BASED ON THE UDEMY COURSE.
//		viewport.width = (float)mSwapchain.
//		viewport.height = (float)mSwapchain.
		viewport.minDepth = 0.0f;			// min fame buffer depth.
		viewport.maxDepth = 1.0f;			// max fame buffer depth.

		// Create a scissor info struct.
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };	// Offset to use region from.
		// TODO: COME BACK AND FINISH THIS BASED ON THE UDEMY COURSE.
//		scissor.extent =
		// ==================================================================================

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;


		//==========================================================================================
		// TODO: ENABLE THIS WHEN RESIZING WINDOW.
//		std::vector<VkDynamicState> dynamicStateEnables;
//		dynamicStateEnables.reserve(2);
//		dynamicStateEnables.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
//		dynamicStateEnables.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
//
//		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
//		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//		dynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();
//		dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
		//==========================================================================================

		// --- RASTERIZER ---
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane.
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output.
		rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices.
		rasterizationStateCreateInfo.lineWidth = 1.0f;						// How thick the lines should be when drawn.
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of the tri to cull.
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// Winding to determine which side is the front.
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping.)

		// --- MULTISAMPLING ---
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not.
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of fragments to use per fragment.


		// --- BLENDING ---
		// Blending decides how to blend a new color being written to a fragment. with the old value.

		// Blend attachment state.
		VkPipelineColorBlendAttachmentState colorState{};
		colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorState.blendEnable = VK_TRUE;

		// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
		colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorState.colorBlendOp = VK_BLEND_OP_ADD;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
		// Or to summarize: (new color alpha * new color) + ((1 - new color alpha) * old color)
		colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorState.alphaBlendOp = VK_BLEND_OP_ADD;				// In summary: new alpha = (1 * alpha) + (0 * old alpha)


		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorState;

		// --- PIPELINE LAYOUTS TODO: Apply future descriptor set updates.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		// --- CREATE PIPELINE LAYOUT ---
		VK_CHECK(vkCreatePipelineLayout(mDevice.logicalDevice, &pipelineLayoutCreateInfo, mAllocator, &pipelineLayout))

		// --- DEPTH STENCIL TESTING ---
		// TODO: SET UP DEPTH STENCIL TESTING.

		// --- GRAPHICS PIPELINE CREATION ---
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;													// NUmber of shader stages.
		pipelineCreateInfo.pStages = shaderStages;											// List of shader stages.
		pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;					// All the fixes function pipeline states.
		pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.layout = pipelineLayout;											// Pipeline layout the pipeline should use.
		pipelineCreateInfo.renderPass = mRenderPass;										// Render pass description the pipeline is compatible with.
		pipelineCreateInfo.subpass = 0;														// Subpass of render pass to use with pipeline.

		// Pipeline derivatives - Can create multiple pipeline that can derive from one another for optimization.
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;								// Existing pipeline to derive from.
		pipelineCreateInfo.basePipelineIndex = -1;											// Or index of pipeline being created to derived from. (in case of creating multiple at once.)

		VK_CHECK(vkCreateGraphicsPipelines(mDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, mAllocator, &mGraphicsPipeline))

		// Shader modules are not required after pipeline is created so we can go ahead and destroy them now.
		vkDestroyShaderModule(mDevice.logicalDevice, fragmentShaderModule, mAllocator);
		vkDestroyShaderModule(mDevice.logicalDevice, vertexShaderModule, mAllocator);
    }

	void VulkanRenderer::DestroyGraphicsPipeline(){
		vkDestroyPipeline(mDevice.logicalDevice, mGraphicsPipeline, mAllocator);
	}

	void VulkanRenderer::DestroyPipeline() {
		vkDestroyPipelineLayout(mDevice.logicalDevice, pipelineLayout, mAllocator);
	}

	VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char> &code)
    {
		VkShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();									// Size of code.
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());	// Pointer to code (of type uint32_t type)

		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(mDevice.logicalDevice, &shaderModuleCreateInfo, mAllocator, &shaderModule))

		return shaderModule;
	}

	void VulkanRenderer::CreateRenderPass() {
		u32 attachmentCount = 2;
		VkAttachmentDescription attachmentDescriptions[attachmentCount];

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = mSwapchain.imageFormat.format;				// format to use for attachment.
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling.
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering.
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// Describes what to do with attachment after rendering.
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering.
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering.

		// Framebuffer data will be stored as an image. But image can be given different data layouts.
		// to give optimal use for certain operations.
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass starts.
		colorAttachment.flags = 0;

		attachmentDescriptions[0] = colorAttachment;

		// Attachment reference uses an attachment index that refers to the attachment list passed to renderPassCreateInfo.
		VkAttachmentReference colorAttachmentReference{};
		colorAttachmentReference.attachment = 0;							// Attachment description arry index.
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = mDevice.depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attachmentDescriptions[1] = depthAttachment;

		VkAttachmentReference depthAttachmentReference{};
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Information about a particular subpass the Render pass s using.
		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type sub-pass needs to be bound to.
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentReference;
		subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;

		// Need to determine when layout transitions occur using subpass dependencies.
		std::array<VkSubpassDependency, 2> subpassDependencies{};

		// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYER_COLOR_ATTACHMENT_OPTIMAL
		// Transition must happen after...
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		// but must happen before.
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYER_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// but must happen before.
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags = 0;

		// Create info for render pass.
		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = attachmentCount;
		renderPassCreateInfo.pAttachments = attachmentDescriptions;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = subpassDependencies.size();
		renderPassCreateInfo.pDependencies = subpassDependencies.data();
		renderPassCreateInfo.pNext = nullptr;
		renderPassCreateInfo.flags = 0;

		VK_CHECK(vkCreateRenderPass(mDevice.logicalDevice, &renderPassCreateInfo, mAllocator, &mRenderPass))
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