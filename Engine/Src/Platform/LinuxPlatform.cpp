#include "LinuxPlatform.h"

#if defined(VPLATFORM_LINUX)
#include "Core/Event/Registrar/EventSystemManager.h"
#include "Core/Event/Keyboard/KeyEvent.h"
#include "Core/Event/Mouse/MouseMovedEvent.h"
#include "Core/Event/Mouse/MouseButtonEvent.h"
#include "Core/Event/Mouse/MouseScrolledEvent.h"
#include "Core/Event/Application/WindowCloseEvent.h"

namespace Vkr
{
    LinuxPlatform::~LinuxPlatform()
    {
        CleanUp();
    }

    StatusCode LinuxPlatform::CreateNewWindow(const char *windowName, i16 x, i16 y, u16 width, u16 height)
    {
        if (mInitialized)
        {
            VWARN("Platform has already been initialized!")
            return StatusCode::PlatformAlreadyInitialized;
        }

        /* Open the connection to the X server */
        // connection = xcb_connect(NULL, NULL);

        // Connect to X
        mpDisplay = XOpenDisplay(nullptr);

        // Turn key repeats off.
        XAutoRepeatOff(mpDisplay);

        // Retrieve the connection from the display.
        mConnection = XGetXCBConnection(mpDisplay);

        if (xcb_connection_has_error(mConnection))
        {
            VFATAL("Failed to connect to X server via XCB.");
            return StatusCode::XcbConnectionHasError;
        }

        /* Get the first screen */
        const xcb_setup_t *setup = xcb_get_setup(mConnection);
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
        mScreen = iter.data;

        // Allocate a XID for the window to be created.
        mWindow = xcb_generate_id(mConnection);

        // Register event types.
        // XCB_CW_BACK_PIXEL = filling then window bg with a single color
        // XCB_CW_EVENT_MASK is required.
        u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        // Values to be sent over XCB (bg color, events)
        u32 values[2] = {mScreen->black_pixel,
                         XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
                             XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
                             XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
                             XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY};

        /* Create the window */
        xcb_create_window(mConnection,                   /* Connection          */
                          XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                          mWindow,                       /* window Id           */
                          mScreen->root,                 /* parent window       */
                          x, y,                          /* x, y                */
                          width, height,                 /* width, height       */
                          10,                            /* border_width        */
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                          mScreen->root_visual,          /* visual              */
                          mask, values);                 /* masks, not used yet */

        // Change the title
        xcb_change_property(
            mConnection,
            XCB_PROP_MODE_REPLACE,
            mWindow,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8, // data should be viewed 8 bits at a time
            strlen(windowName),
            windowName);

        // Tell the server to notify when the window manager
        // attempts to destroy the window.
        xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(
            mConnection,
            0,
            strlen("WM_DELETE_WINDOW"),
            "WM_DELETE_WINDOW");

        xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
            mConnection,
            0,
            strlen("WM_PROTOCOLS"),
            "WM_PROTOCOLS");

        xcb_intern_atom_reply_t *wm_delete_reply = xcb_intern_atom_reply(
            mConnection,
            wm_delete_cookie,
            nullptr);

        xcb_intern_atom_reply_t *wm_protocols_reply = xcb_intern_atom_reply(
            mConnection,
            wm_protocols_cookie,
            nullptr);

        mDeleteWin = wm_delete_reply->atom;
        mProtocols = wm_protocols_reply->atom;

        xcb_change_property(
            mConnection,
            XCB_PROP_MODE_REPLACE,
            mWindow,
            wm_protocols_reply->atom,
            4,
            32,
            1,
            &wm_delete_reply->atom);

        /* Map the window on the screen */
        xcb_map_window(mConnection, mWindow);

        /* Make sure commands are sent before we pause so that the window gets shown */
        i32 stream_result = xcb_flush(mConnection);

        if (stream_result <= 0)
        {
            VFATAL("An error occurred when flushing the stream: %d", stream_result)
            return StatusCode::XcbFlushError;
        }

        mInitialized = true;

        return StatusCode::Successful;
    }

    StatusCode LinuxPlatform::CloseWindow()
    {
        CleanUp();

        return StatusCode::Successful;
    }

    bool LinuxPlatform::PollForEvents()
    {
        xcb_generic_event_t *event;
        bool quit = false;

        while ((event = xcb_poll_for_event(mConnection)))
        {
            if (quit)
            {
                free(event);
                break;
            }

            switch (event->response_type & ~0x80)
            {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                // xcb_key_press_event_t and xcb_key_press_release_t are the sane
                auto *kp = (xcb_key_press_event_t *)event;
                bool pressed = event->response_type == XCB_KEY_PRESS;

                Key key = TranslateKeycode(XkbKeycodeToKeysym(
                    mpDisplay,
                    (KeyCode)kp->detail,
                    0,
                    kp->detail & ShiftMask ? 1 : 0));

                KeyEvent kEvent(key, pressed);
                EventSystemManager::Dispatch(&kEvent, SenderType::Platform);

                break;
            }
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
            {
                auto *bp = (xcb_button_press_event_t *)event;
                bool pressed = event->response_type == XCB_BUTTON_PRESS;
                MouseButton mouseButton = MouseButton::Unknown;

                switch (bp->detail)
                {
                case XCB_BUTTON_INDEX_1:
                    mouseButton = MouseButton::Left;
                    break;
                case XCB_BUTTON_INDEX_2:
                    mouseButton = MouseButton::ScrollWheel;
                    break;
                case XCB_BUTTON_INDEX_3:
                    mouseButton = MouseButton::Right;
                    break;
                case XCB_BUTTON_INDEX_4:
                    mouseButton = MouseButton::ScrollWheelUp;
                    break;
                case XCB_BUTTON_INDEX_5:
                    mouseButton = MouseButton::ScrollWheelDown;
                    break;
                }

                if (mouseButton == MouseButton::ScrollWheelUp || mouseButton == MouseButton::ScrollWheelDown)
                {
                    if (pressed)
                    {
                        MouseScrolledEvent mEvent(mouseButton == MouseButton::ScrollWheelUp, bp->event_x, bp->event_y);
                        EventSystemManager::Dispatch(&mEvent, SenderType::Platform);
                    }
                }
                else
                {
                    MouseButtonEvent mEvent(mouseButton, pressed, bp->event_x, bp->event_y);
                    EventSystemManager::Dispatch(&mEvent, SenderType::Platform);
                }

                break;
            }
            case XCB_MOTION_NOTIFY:
            {
                auto *mv = (xcb_motion_notify_event_t *)event;

                // MouseMovedEvent event(mv->event_x, mv->event_y);
                // EventSystemManager::Dispatch(&event, SenderType::Platform);

                break;
            }
            case XCB_ENTER_NOTIFY:
            case XCB_LEAVE_NOTIFY:
            {
                auto *el = (xcb_leave_notify_event_t *)event;
                bool entered = el->response_type == XCB_ENTER_NOTIFY;

                if (entered)
                {
                    // VINFO("Mouse entered window %u, at coordinates (%u, %u)", el->event, el->event_x, el->event_y)
                }
                else
                {
                    // VINFO("Mouse left window %u, at coordinates (%u, %u)", el->event, el->event_x, el->event_y)
                }

                break;
            }
            case XCB_EXPOSE:
            {
                auto *ex = (xcb_expose_event_t *)event;

                // VINFO("Window %u exposed. Region to be redrawn at location (%u, %u), with dimension (%u, %u)",
                //   ex->window, ex->x, ex->y, ex->width, ex->height)
                break;
            }
            case XCB_CONFIGURE_NOTIFY:
            {
                // VDEBUG("Window modified!!!")
                // TODO: Resizing
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                auto *cm = (xcb_client_message_event_t *)event;

                // Close the window.
                if (cm->data.data32[0] == mDeleteWin)
                {
                    WindowCloseEvent event{};
                    EventSystemManager::Dispatch(&event, SenderType::Platform);
                    quit = true;
                }

                break;
            }
            default:
            {
                /* Unknown event type, ignore it */
                // VINFO("Unknown event: %u", event->response_type);
                break;
            }
            }

            free(event);
        }

        return !quit;
    }

    void LinuxPlatform::AddRequiredVulkanExtensions(std::vector<const char *> &extensions)
    {
        extensions.emplace_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    }

    StatusCode LinuxPlatform::CreateVulkanSurface(VkInstance *instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
    {
        VkXcbSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;	// Vulkan XCB Surface creation structure.
        createInfo.connection = mConnection;								// Connection to the X-server.
        createInfo.window = mWindow;										// A handle to the window.

		// Ensure that the Surface is created successfully.
        VkResult result = vkCreateXcbSurfaceKHR(*instance, &createInfo, allocator, surface);
    	if (result != VK_SUCCESS) {
			return StatusCode::VulkanFailedToCreateXcbSurface;
		}

		return StatusCode::Successful;
	}

    f64 LinuxPlatform::GetAbsoluteTime()
    {
        struct timespec now
        {
        };
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec + now.tv_nsec * 0.000000001;
    }

    void LinuxPlatform::SleepForDuration(u64 ms)
    {
#if _POSIX_C_SOURCE >= 199309L
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000 * 1000;
        nanosleep(&ts, 0);
#else
        if (ms >= 1000)
        {
            sleep(ms / 1000);
        }
        usleep((ms % 1000) * 1000);
#endif
    }

    Key LinuxPlatform::TranslateKeycode(KeySym xKeycode)
    {
        switch (xKeycode)
        {
        case XK_BackSpace:
            return Key::Backspace;
        case XK_Return:
            return Key::Enter;
        case XK_Tab:
        case XK_ISO_Left_Tab:
            return Key::Tab;
        case XK_Caps_Lock:
            return Key::CapsLock;
        case XK_Escape:
            return Key::Escape;
        case XK_space:
            return Key::Space;
        case XK_Shift_L:
            return Key::LeftShift;
        case XK_Shift_R:
            return Key::RightShift;
        case XK_Control_L:
            return Key::LeftControl;
        case XK_Control_R:
            return Key::RightControl;
        case XK_Alt_L:
            return Key::LeftAlt;
        case XK_Alt_R:
            return Key::RightAlt;
        case XK_Super_L:
            return Key::LeftSuper;
        case XK_Super_R:
            return Key::RightSuper;
        case XK_Menu:
            return Key::Menu;

        case XK_a:
        case XK_A:
            return Key::A;
        case XK_b:
        case XK_B:
            return Key::B;
        case XK_c:
        case XK_C:
            return Key::C;
        case XK_d:
        case XK_D:
            return Key::D;
        case XK_e:
        case XK_E:
            return Key::E;
        case XK_f:
        case XK_F:
            return Key::F;
        case XK_g:
        case XK_G:
            return Key::G;
        case XK_h:
        case XK_H:
            return Key::H;
        case XK_i:
        case XK_I:
            return Key::I;
        case XK_j:
        case XK_J:
            return Key::J;
        case XK_k:
        case XK_K:
            return Key::K;
        case XK_l:
        case XK_L:
            return Key::L;
        case XK_m:
        case XK_M:
            return Key::M;
        case XK_n:
        case XK_N:
            return Key::N;
        case XK_o:
        case XK_O:
            return Key::O;
        case XK_p:
        case XK_P:
            return Key::P;
        case XK_q:
        case XK_Q:
            return Key::Q;
        case XK_r:
        case XK_R:
            return Key::R;
        case XK_s:
        case XK_S:
            return Key::S;
        case XK_t:
        case XK_T:
            return Key::T;
        case XK_u:
        case XK_U:
            return Key::U;
        case XK_v:
        case XK_V:
            return Key::V;
        case XK_w:
        case XK_W:
            return Key::W;
        case XK_x:
        case XK_X:
            return Key::X;
        case XK_y:
        case XK_Y:
            return Key::Y;
        case XK_z:
        case XK_Z:
            return Key::Z;

        case XK_1:
        case XK_exclam:
            return Key::D1;
        case XK_2:
        case XK_at:
            return Key::D2;
        case XK_3:
        case XK_numbersign:
            return Key::D3;
        case XK_4:
        case XK_dollar:
            return Key::D4;
        case XK_5:
        case XK_percent:
            return Key::D5;
        case XK_6:
        case XK_asciicircum:
            return Key::D6;
        case XK_7:
        case XK_ampersand:
            return Key::D7;
        case XK_8:
        case XK_asterisk:
            return Key::D8;
        case XK_9:
        case XK_parenleft:
            return Key::D9;
        case XK_0:
        case XK_parenright:
            return Key::D0;
        case XK_minus:
        case XK_underscore:
            return Key::Minus;
        case XK_equal:
        case XK_plus:
            return Key::Equal;

        case XK_bracketleft:
        case XK_braceleft:
            return Key::LeftBracket;
        case XK_bracketright:
        case XK_braceright:
            return Key::RightBracket;
        case XK_backslash:
        case XK_bar:
            return Key::Backslash;
        case XK_semicolon:
        case XK_colon:
            return Key::Semicolon;
        case XK_apostrophe:
            return Key::Apostrophe;
        case XK_comma:
        case XK_less:
            return Key::Comma;
        case XK_period:
        case XK_greater:
            return Key::Period;
        case XK_slash:
        case XK_question:
            return Key::Slash;
        case XK_grave:
        case XK_asciitilde:
            return Key::GraveAccent;

        case XK_F1:
            return Key::F1;
        case XK_F2:
            return Key::F2;
        case XK_F3:
            return Key::F3;
        case XK_F4:
            return Key::F4;
        case XK_F5:
            return Key::F5;
        case XK_F6:
            return Key::F6;
        case XK_F7:
            return Key::F7;
        case XK_F8:
            return Key::F8;
        case XK_F9:
            return Key::F9;
        case XK_F10:
            return Key::F10;
        case XK_F11:
            return Key::F11;
        case XK_F12:
            return Key::F12;
        case XK_F13:
            return Key::F13;
        case XK_F14:
            return Key::F14;
        case XK_F15:
            return Key::F15;
        case XK_F16:
            return Key::F16;
        case XK_F17:
            return Key::F17;
        case XK_F18:
            return Key::F18;
        case XK_F19:
            return Key::F19;
        case XK_F20:
            return Key::F20;
        case XK_F21:
            return Key::F21;
        case XK_F22:
            return Key::F22;
        case XK_F23:
            return Key::F23;
        case XK_F24:
            return Key::F24;
        case XK_F25:
            return Key::F25;

        case XK_Print:
            return Key::PrintScreen;
        case XK_Scroll_Lock:
            return Key::ScrollLock;
        case XK_Pause:
            return Key::Pause;
        case XK_Insert:
            return Key::Insert;
        case XK_Delete:
            return Key::Delete;
        case XK_Home:
            return Key::Home;
        case XK_End:
            return Key::End;
        case XK_Page_Up:
            return Key::PageUp;
        case XK_Page_Down:
            return Key::PageDown;
        case XK_Left:
            return Key::Left;
        case XK_Up:
            return Key::Up;
        case XK_Right:
            return Key::Right;
        case XK_Down:
            return Key::Down;

        case XK_Num_Lock:
            return Key::NumLock;
        case XK_KP_Divide:
            return Key::KPDivide;
        case XK_KP_Multiply:
            return Key::KPMultiply;
        case XK_KP_Subtract:
            return Key::KPSubtract;
        case XK_KP_Add:
            return Key::KPAdd;
        case XK_KP_Enter:
            return Key::KPEnter;
        case XK_KP_Decimal:
            return Key::KPDecimal;
        case XK_KP_0:
        case XK_KP_Insert:
            return Key::KP0;
        case XK_KP_1:
        case XK_KP_End:
            return Key::KP1;
        case XK_KP_2:
        case XK_KP_Down:
            return Key::KP2;
        case XK_KP_3:
        case XK_KP_Page_Down:
            return Key::KP3;
        case XK_KP_4:
        case XK_KP_Left:
            return Key::KP4;
        case XK_KP_5:
        case XK_KP_Begin:
            return Key::KP5;
        case XK_KP_6:
        case XK_KP_Right:
            return Key::KP6;
        case XK_KP_7:
        case XK_KP_Home:
            return Key::KP7;
        case XK_KP_8:
        case XK_KP_Up:
            return Key::KP8;
        case XK_KP_9:
        case XK_KP_Page_Up:
            return Key::KP9;
        case XK_KP_Equal:
            return Key::Equal;

        default:
            return Key::Unknown;
        }
    }

    void LinuxPlatform::CleanUp()
    {
        // Turn key repeats back on since this is global for the OS... just... wow.
        XAutoRepeatOn(mpDisplay);

        // Destroy Window.
        xcb_destroy_window(mConnection, mWindow);

        // Disconnect from the X server.
        // xcb_disconnect(mConnection);

        mInitialized = false;
    }
}

#endif