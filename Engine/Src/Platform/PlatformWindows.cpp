#include "PlatformWindows.h"

#if defined(VPLATFORM_WINDOWS)

#include "Core/Input/Key.h"
#include "Core/Input/MouseButton.h"
#include "Core/Event/Registrar/EventSystemManager.h"
#include "Core/Event/Keyboard/KeyEvent.h"
#include "Core/Event/Mouse/MouseMovedEvent.h"
#include "Core/Event/Mouse/MouseButtonEvent.h"
#include "Core/Event/Mouse/MouseScrolledEvent.h"

#include <vulkan/vulkan_win32.h>

namespace Vkr{
    PlatformWindows::PlatformWindows() {

    }

    StatusCode PlatformWindows::CreateNewWindow(const char *windowName, i32 x, i32 y, u16 width, u16 height) {
        mHInstance = GetModuleHandleA(0);

        // Setup and register window class.
        HICON icon = LoadIcon(mHInstance, IDI_APPLICATION);
        WNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_DBLCLKS; // Get double-clicks
        wc.lpfnWndProc = PlatformWindows::ProcessMessage;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = mHInstance;
        wc.hIcon = icon;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // NULL; // Manage the cursor manually
        wc.hbrBackground = nullptr;                  // Transparent
        wc.lpszClassName = "vulkyrie_window_class";

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
            return StatusCode::WindowRegistrationFailed;
        }

        // Create window
        u32 clientX = x;
        u32 clientY = y;
        u32 clientWidth = width;
        u32 clientHeight = height;

        u32 windowX = clientX;
        u32 windowY = clientY;
        u32 windowWidth = clientWidth;
        u32 windowHeight = clientHeight;

        u32 windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
        u32 windowExStyle = WS_EX_APPWINDOW;

        windowStyle |= WS_MAXIMIZEBOX;
        windowStyle |= WS_MINIMIZEBOX;
        windowStyle |= WS_THICKFRAME;

        // Obtain the size of the border.
        RECT borderRect = {0, 0, 0, 0};
        AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

        // In this case, the border rectangle is negative.
        windowX += borderRect.left;
        windowY += borderRect.top;

        // Grow by the size of the OS border.
        windowWidth += borderRect.right - borderRect.left;
        windowHeight += borderRect.bottom - borderRect.top;

        HWND handle = CreateWindowExA(
                windowExStyle, "vulkyrie_window_class", windowName,
                windowStyle, windowX, windowY, windowWidth, windowHeight,
                0, 0, mHInstance, 0);

        if (handle == nullptr)
        {
            MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
            VFATAL("Window creation failed!");
            return StatusCode::WindowCreationFailed;
        }

        mHwnd = handle;

        // Show the window
        bool shouldActivate = true; // TODO: if the window should not accept input, this should be false.
        i32 showWindowCommandFlags = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
        // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
        // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
        ShowWindow(mHwnd, showWindowCommandFlags);

        // Clock setup
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        mClockFrequency = 1.0 / (f64)frequency.QuadPart;
        QueryPerformanceCounter(&mStartTime);

        return StatusCode::Successful;
    }

    StatusCode PlatformWindows::CloseWindow() {
        if (mHwnd != nullptr)
        {
            DestroyWindow(mHwnd);
            mHwnd = nullptr;
        }

        return StatusCode::Successful;
    }

    bool PlatformWindows::PollForEvents() {
        MSG message;

        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        return true;
    }

    void PlatformWindows::SleepForDuration(u64 duration) {
        Sleep(duration);
    }

    f64 PlatformWindows::GetAbsoluteTime() {
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);
        return (f64)nowTime.QuadPart * mClockFrequency;
    }

    StatusCode PlatformWindows::CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface) {

        VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        createInfo.hinstance = mHInstance;
        createInfo.hwnd = mHwnd;

        VkResult result = vkCreateWin32SurfaceKHR(*instance, &createInfo, allocator, surface);
        if (result != VK_SUCCESS)
        {
            VFATAL("Vulkan Win32 surface creation failed.");
            return StatusCode::VulkanWin32SurfaceCreationFailed;
        }

        return StatusCode::Successful;
    }

    void PlatformWindows::GetRequiredVulkanExtensions(std::vector<const char *> &extensions) {
        extensions.emplace_back("VK_KHR_win32_surface");
    }

    LRESULT CALLBACK PlatformWindows::ProcessMessage(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param)
    {
        switch (msg)
        {
            case WM_ERASEBKGND:
                // Notify the OS that erasing will be handled by the application to prevent flicker.
                return 1;
            case WM_CLOSE:
                // TODO: Fire an event for the application to quit.
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_SIZE:
            {
                // Get the updated size.
                // RECT r;
                // GetClientRect(hwnd, &r);
                // u32 width = r.right - r.left;
                // u32 height = r.bottom - r.top;

                // TODO: Fire an event for window resize.

                break;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                // Key pressed/released
                bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                Key key = (Key)w_param;

                KeyEvent kEvent(key, pressed);
                EventSystemManager::Dispatch(&kEvent, SenderType::Platform);

                break;
            }
            case WM_MOUSEMOVE:
            {
                // Mouse move
                i32 xPosition = GET_X_LPARAM(l_param);
                i32 yPosition = GET_Y_LPARAM(l_param);

                // MouseMovedEvent event(xPosition,yPosition);
                // EventSystemManager::Dispatch(&event, SenderType::Platform);

                break;
            }
            case WM_MOUSEWHEEL:
            {
                i32 zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
                i32 xPosition = GET_X_LPARAM(l_param);
                i32 yPosition = GET_Y_LPARAM(l_param);

                if (zDelta != 0)
                {
                    MouseScrolledEvent mEvent(zDelta > 0, xPosition, yPosition);
                    EventSystemManager::Dispatch(&mEvent, SenderType::Platform);
                }

                break;
            }
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            {
                bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
                MouseButton mouseButton = MouseButton::Unknown;

                i32 xPosition = GET_X_LPARAM(l_param);
                i32 yPosition = GET_Y_LPARAM(l_param);

                switch (msg)
                {
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                        mouseButton = MouseButton::Left;
                        break;
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                        mouseButton = MouseButton::ScrollWheel;
                        break;
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                        mouseButton = MouseButton::Right;
                        break;
                }

                // Pass over to the event subsystem.
                MouseButtonEvent mEvent(mouseButton, pressed, xPosition, yPosition);
                EventSystemManager::Dispatch(&mEvent, SenderType::Platform);

                break;
            }
        }

        return DefWindowProcA(hwnd, msg, w_param, l_param);
    }
}

#endif