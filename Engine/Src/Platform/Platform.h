#pragma once

#include "Defines.h"

namespace Vkr
{
    class Platform
    {
    public:
        Platform(const Platform &) = delete;
        void operator=(Platform const &) = delete;

        virtual ~Platform() = default;

        /** Creates a new Application window.
         * @param windowName Name for the Window.
         * @param x X-coordinate for the window.
         * @param y Y-coordinate for the window.
         * @param width Width of the window to be created.
         * @param height Height of the window to be crated.
         */
        virtual StatusCode CreateNewWindow(const char *windowName, i16 x, i16 y, u16 width, u16 height) = 0;

        /** Closes the application window. */
        virtual StatusCode CloseWindow() = 0;

        /** Polls for events on the platform specific window. */
        virtual bool PollForEvents() = 0;

        /* Gets the absolute time from the underlying platform. */
        virtual f64 GetAbsoluteTime() = 0;

        /** SleepForDuration on the thread for the provided ms. This blocks the main thread.
         * Should only be used for giving time back to the OS for unused update power.
         * Therefore it is not exported.
         * @param duration SleepForDuration duration.
         */
        virtual void SleepForDuration(u64 duration) = 0;

        /** Adds required Vulkan extensions for the underlying platform.
         * @param extensions A std::vector<const char *> where the required extensions will be added.
         */
        virtual void AddRequiredVulkanExtensions(std::vector<const char *> &extensions) = 0;

        /**
         * Creates Vulkan surface.
         * @param renderer A pointer to the vulkan renderer.
         */
        virtual void CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) = 0;

    protected:
        Platform() {}
    };
}
