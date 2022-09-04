#pragma once
#include "Platform.h"

#if defined(VPLATFORM_LINUX)
#include "Core/Input/Key.h"

namespace Vkr
{
    class LinuxPlatform : public Platform
    {
    private:
        bool mInitialized = false;
        Display *mpDisplay;
        xcb_connection_t *mConnection;
        xcb_window_t mWindow;
        xcb_screen_t *mScreen;
        xcb_atom_t mProtocols;
        xcb_atom_t mDeleteWin;

        static Key TranslateKeycode(KeySym x_keycode);

    public:
        LinuxPlatform();
        DESTRUCTOR_LOG(LinuxPlatform)

        LinuxPlatform(const LinuxPlatform &) = delete;
        void operator=(LinuxPlatform const &) = delete;

        StatusCode CreateWindow(const char *windowName, i32 x, i32 y, u16 width, u16 height) override;
        StatusCode CloseWindow() override;
        bool PollForEvents() override;
        f64 GetAbsoluteTime() override;
        void Sleep(u64 duration) override;
        void AddRequiredVulkanExtensions(std::vector<const char *> &extensions) override;
        StatusCode CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) override;
    };
}

#endif