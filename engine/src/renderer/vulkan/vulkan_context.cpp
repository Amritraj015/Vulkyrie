#include "renderer/vulkan/vulkan_context.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include <vulkyrie_version.h>
#include <volk.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    VulkanContext::VulkanContext(const DeviceCreationInfo &info)
        : mDeviceCreationInfo(info)
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

    std::vector<const char *> VulkanContext::getRequiredInstanceExtensions() {
        u32 requiredExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

        std::vector<const char *> requiredInstanceExtensions(glfwExtensions, glfwExtensions + requiredExtensionCount);
        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return requiredInstanceExtensions;
    }

    std::vector<const char *> VulkanContext::getRequiredInstanceLayers() {
        const std::vector<const char *> requiredInstanceLayers = { "VK_LAYER_KHRONOS_validation" };

        return requiredInstanceLayers;
    }

    std::vector<const char *> VulkanContext::getRequiredDeviceExtensions() {
        const std::vector<const char *> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        return requiredDeviceExtensions;
    }

    StatusCode VulkanContext::validateRequiredExtensions(const std::vector<const char *> &requiredExtensions) {
        // u32 count = 0;
        (void)requiredExtensions;
        return StatusCode::Successful;
    }

    StatusCode VulkanContext::validateRequiredLayers(const std::vector<const char *> &requiredLayers) {
        (void)requiredLayers;

        return StatusCode::Successful;
    }

    StatusCode VulkanContext::Initialize() {
        // Try to initialize Volk.
        VE_VK_CHECK(volkInitialize(), StatusCode::FailedToInitializeVolk);

        // Get required extensions and layers.
        const std::vector<const char *> requiredInstanceExtensions = getRequiredInstanceExtensions();
        const std::vector<const char *> requiredInstanceLayers = getRequiredInstanceLayers();
        const std::vector<const char *> requiredDeviceExtensions = getRequiredDeviceExtensions();

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

        const VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<u32>(requiredInstanceLayers.size()),
            .ppEnabledLayerNames = requiredInstanceLayers.data(),
            .enabledExtensionCount = static_cast<u32>(requiredInstanceExtensions.size()),
            .ppEnabledExtensionNames = requiredInstanceExtensions.data(),
        };

        // TODO: Add this to pNext of the device creation into.
        // VkPhysicalDeviceVulkan13Features features13{
        //     .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        //     .pNext = nullptr,
        //     .dynamicRendering = VK_TRUE,
        //     .synchronization2 = VK_TRUE,
        // };

        // Try to create vulkan instance.
        VE_VK_CHECK(vkCreateInstance(&instanceInfo, mHostAllocator.Callbacks(), &mVkInstance), StatusCode::FailedToCreateVulkanInstance);

        volkLoadInstance(mVkInstance);

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

} // namespace Vulkyrie
