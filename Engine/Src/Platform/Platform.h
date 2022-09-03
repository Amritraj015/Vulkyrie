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

        /** Initializes the Platform instance.
         * @param windowName Name for the Window.
         * @param x X-coordinate for the window.
         * @param y Y-coordinate for the window.
         * @param width Width of the window to be created.
         * @param height Height of the window to be crated.
         */
        virtual StatusCode CreateWindow(const char *windowName, i32 x, i32 y, u16 width, u16 height) = 0;

        /** Terminates the Platform instance. */
        virtual StatusCode CloseWindow() = 0;

        /** Polls for events on the platform specific window. */
        virtual bool PollForEvents() = 0;

        /* Gets the absolute time from the underlying platform. */
        virtual f64 GetAbsoluteTime() = 0;

        /** Sleep on the thread for the provided ms. This blocks the main thread.
         * Should only be used for giving time back to the OS for unused update power.
         * Therefore it is not exported.
         * @param duration Sleep duration.
         */
        virtual void Sleep(u64 duration) = 0;

        /**
         * Creates Vulkan surface.
         * @param renderer A pointer to the vulkan renderer.
         */
        virtual StatusCode CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) = 0;

    protected:
        Platform() {}
    };
}
