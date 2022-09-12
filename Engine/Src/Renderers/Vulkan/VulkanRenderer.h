#pragma once
#include "Renderers/Renderer.h"
#include "VulkanDevice.h"
#include "QueueFamilyInfo.h"
#include "DeviceRequirements.h"
#include "VulkanSwapchain.h"
#include "ImageInfo.h"
#include "PhysicalDeviceInfo.h"
#include "Platform/Platform.h"

namespace Vkr
{
    class VulkanRenderer : public Renderer
    {
    public:
        explicit VulkanRenderer(const std::shared_ptr<Platform> &platform);
        DESTRUCTOR_LOG(VulkanRenderer);

        StatusCode Initialize(const char *appName) override;
        StatusCode Shutdown() override;
        void OnResize(u16 width, u16 height) override;
        StatusCode BeginFrame(f32 deltaTime) override;
        StatusCode EndFrame(f32 deltaTime) override;


    private:
        std::shared_ptr<Platform> mPlatform; 		// Underlying platform instance.
        VkSurfaceKHR surface{};              		// Vulkan Surface KHR
        VkInstance mInstance{};              		// Vulkan Instance
        VkAllocationCallbacks *mAllocator{}; 		// Custom memory allocator
        VulkanDevice mDevice;                		// Vulkan Devices metadata
        VulkanSwapchain mSwapchain{};        		// Vulkan swapchain metadata.
        u32 mFrameBufferWidth{};             		// The frame-buffer's current width.
        u32 mFrameBufferHeight{};            		// The frame-buffer's current height.
        u32 mImageIndex{};
        u32 mCurrentFrame{};
        [[nodiscard]] i32 FindMemoryIndex(u32 typeFilter, u32 propertyFlags) const;

#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT mDebugMessenger{};
#endif

        StatusCode CreateLogicalDevice();
        StatusCode SelectPhysicalDevice();
        StatusCode PhysicalDeviceMeetsRequirements(VkPhysicalDevice device, const PhysicalDeviceInfo &deviceInfo, QueueFamilyInfo *outQueueFamilyInfo);

        void QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
        StatusCode DestroyDevice();
        bool DetectDepthFormat();

        void CreateSwapchain(u32 width, u32 height);
        void RecreateSwapchain(u32 width, u32 height);
        bool AcquireNextImageIndex(u64 nanoSeconds, VkSemaphore imageAvailableSemaphore, VkFence fence, u32 *outImageIndex);
        void Present(VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);
        void DestroySwapchain();

        void CreateImage(const ImageInfo &imageInfo, VulkanImage *outImage);
        void CreateImageView(VkFormat format, VulkanImage *image, VkImageAspectFlags aspectFlags);
        void DestroyImage(VulkanImage *image);

		void CreateGraphicsPipeline();
		void CreateShaderModule(const std::vector<char> &code);
    };
}