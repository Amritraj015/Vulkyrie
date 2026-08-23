#include "renderer/vulkan/vulkan_context.h"
#include "core/constants.h"
#include "renderer/vulkan/vulkan_utilities.h"
#include <volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    VulkanContext::VulkanContext(const DeviceCreationInfo &info)
        : mDeviceCreationInfo(info)
        , mCapabilities()
        , mContextCreated(false) {
    }

    VulkanContext::~VulkanContext() {
        vkDeviceWaitIdle(mVkDevice);

        vkDestroyInstance(mVkInstance, nullptr);
    }

    StatusCode VulkanContext::Initialize() {

        VE_VK_CHECK(volkInitialize(), "Failed to initialize Volk.", StatusCode::FailedToInitializeVolk);

        const ApplicationInfo &appInfo = mDeviceCreationInfo.ApplicationInfo;

        // -----------------------------------------------------------------------------------------------------------
        // TODO: These are probably not correct. check them.
        u32 glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        u32 enabledLayerCount = 0;
        std::vector<const char *> enabledLayers;
        // -----------------------------------------------------------------------------------------------------------

        const VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = appInfo.Name.Data(),
            .applicationVersion = VK_MAKE_VERSION(appInfo.MajorVersion, appInfo.MinorVersion, appInfo.PatchVersion),
            .pEngineName = VE_K_ENGINE_NAME.Data(),
            .engineVersion = VK_MAKE_VERSION(VE_K_ENGINE_MAJOR_VERSION, VE_K_ENGINE_MINOR_VERSION, VE_K_ENGINE_PATCH_VERSION),
            .apiVersion = VK_API_VERSION_1_4,
        };

        // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR ;
        // VK_INSTANCE_CREATE_FLAG_BITS_MAX_ENUM ;

        const VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = enabledLayerCount,
            .ppEnabledLayerNames = enabledLayers.data(),
            .enabledExtensionCount = static_cast<u32>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        // TODO: Use Engine's general purpose allocators here instead of "nullptr".
        VE_VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &mVkInstance), "Failed to create Vulkan instance.", StatusCode::FailedToCreateVulkanInstance);

        volkLoadInstance(mVkInstance);

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
