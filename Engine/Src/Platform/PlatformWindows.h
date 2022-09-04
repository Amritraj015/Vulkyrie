#pragma once
#include "Platform.h"
#include "Defines.h"

#if defined(VPLATFORM_WINDOWS)

namespace Vkr{
    class PlatformWindows : public Platform {
    private:
        bool mInitialized = false;
        HINSTANCE mHInstance;
        HWND mHwnd;
        f64 mClockFrequency;
        LARGE_INTEGER mStartTime;

        static LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

    public:
        PlatformWindows();
        DESTRUCTOR_LOG(PlatformWindows)

        PlatformWindows(const PlatformWindows &) = delete;
        void operator=(PlatformWindows const &) = delete;

        StatusCode CreateNewWindow(const char *windowName, i32 x, i32 y, u16 width, u16 height) override;
        StatusCode CloseWindow() override;
        bool PollForEvents() override;
        f64 GetAbsoluteTime() override;
        void SleepForDuration(u64 duration) override;
        void GetRequiredVulkanExtensions(std::vector<const char*> &extensions) override;
        StatusCode CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) override;
    };
}

#endif