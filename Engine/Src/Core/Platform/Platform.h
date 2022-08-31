#pragma once

#include "Defines.h"
#include "Core/Renderers/Vulkan/VulkanRenderer.h"

namespace Vkr
{
    class Platform
    {
    private:
        CONSTRUCTOR_LOG(Platform)

        DESTRUCTOR_LOG(Platform)

        /* Singleton Platform instance. */
        static Platform sInstance;

        bool mInitialized = false;
        Display *mpDisplay;
        xcb_connection_t *mConnection;
        xcb_window_t mWindow;
        xcb_screen_t *mScreen;
        xcb_atom_t mProtocols;
        xcb_atom_t mDeleteWin;

    public:
        Platform(const Platform &) = delete;

        void operator=(Platform const &) = delete;

        /* Initializes the Platform instance. */
        static StatusCode Initialize(const char *windowName, i32 x, i32 y, u16 width, u16 height);

        /* Terminates the Platform instance. */
        static StatusCode Terminate();

        /* Polls for events on the platform specific window. */
        static bool PollEvents();

        /**
         * Writes a message on the console
         * @param message The message to write to the console.
         * @param color Color code for the message.
         */
        static void ConsoleWrite(const char *message, u8 color);

        /**
         * Writes an error message on the console
         * @param message The error message to write to the console.
         * @param color Color code for the error message.
         */
        static void ConsoleWriteError(const char *message, u8 color);

        /* Gets the absolute time from the underlying platform. */
        static f64 GetAbsoluteTime();

        /* Sleep on the thread for the provided ms. This blocks the main thread.
         Should only be used for giving time back to the OS for unused update power.
         Therefore it is not exported. */
        static void Sleep(u64 duration);

        /**
         * Creates Vulkan surface.
         * @param renderer A pointer to the vulkan renderer.
         */
        static StatusCode CreateVulkanSurface(VulkanRenderer *renderer);
    };
}
