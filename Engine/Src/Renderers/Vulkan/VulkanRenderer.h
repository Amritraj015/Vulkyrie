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

		VkRenderPass mRenderPass{};					// Render Pass.
		VkPipelineLayout pipelineLayout{};			// Pipeline layout.
		VkPipeline mGraphicsPipeline{};				// Graphics pipeline.

		u32 mFrameBufferWidth{};             		// The frame-buffer's current width.
        u32 mFrameBufferHeight{};            		// The frame-buffer's current height.
        u32 mImageIndex{};
        u32 mCurrentFrame{};
        [[nodiscard]] i32 FindMemoryIndex(u32 typeFilter, u32 propertyFlags) const;

#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT mDebugMessenger{};

		// Creates a Debugger for Vulkan.
		void CreateVulkanDebugger();

		// Destroys Vulkan debugger.
		void DestroyVulkanDebugger();
#endif

		/** Creates Vulkan instance.
		 * @param appName - Name of the Application .
		 * */
		StatusCode CreateVulkanInstance(const char *appName);
		// Destroys Vulkan instance.
		void DestroyVulkanInstance();

		// Creates Vulkan Surface.
		StatusCode CreateVulkanSurface();
		// Destroys Vulkan Surface.
		void DestroyVulkanSurface();

		// Creates a Vulkan Logical device.
        StatusCode CreateLogicalDevice();
		// Select a physical device. (GPU)
        StatusCode SelectPhysicalDevice();
        StatusCode PhysicalDeviceMeetsRequirements(VkPhysicalDevice device, const PhysicalDeviceInfo &deviceInfo, QueueFamilyInfo *outQueueFamilyInfo);
        void QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
        bool DetectDepthFormat();
		// Destroy the Vulkan logical device.
        void DestroyLogicalDevice();


		// Creates Swapchain.
		StatusCode CreateSwapchain(u32 width, u32 height);
        void RecreateSwapchain(u32 width, u32 height);
        bool AcquireNextImageIndex(u64 nanoSeconds, VkSemaphore imageAvailableSemaphore, VkFence fence, u32 *outImageIndex);
        void Present(VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);
		// Destroys Swapchain.
		void DestroySwapchain();




        void CreateImage(const ImageInfo &imageInfo, VulkanImage *outImage);
        void CreateImageView(VkFormat format, VulkanImage *image, VkImageAspectFlags aspectFlags);
        void DestroyImage(VulkanImage *image);

		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char> &code);
		void DestroyPipeline();

		void CreateRenderPass();
		void DestroyRenderPass();
		void DestroyGraphicsPipeline();
    };
}