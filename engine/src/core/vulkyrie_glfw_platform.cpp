#include "vulkyrie_glfw_platform.h"
#include "core/logger.h"
#include "events/application/window_closed_event.h"
#include "events/application/window_resized_event.h"
#include "events/mouse/mouse_moved_event.h"
#include "events/mouse/mouse_button_pressed_event.h"
#include "events/mouse/mouse_button_released_event.h"
#include "events/mouse/mouse_scrolled_event.h"
#include "events/keyboard/key_pressed_event.h"
#include "events/keyboard/key_released_event.h"

namespace Vulkyrie {
    /** @brief Converts a GLFW key code to a Vulkyrie key code.
     * @param glfwKeyCode The GLFW key code to convert.
     * @returns The corresponding Vulkyrie key code.
     */
    static constexpr KeyCode ConvertGLFWKeyCodeToVulkyrieKeyCode(int glfwKeyCode) {
        switch (glfwKeyCode) {
            case GLFW_KEY_SPACE:
                return KeyCode::Space;
            case GLFW_KEY_APOSTROPHE:
                return KeyCode::Apostrophe;
            case GLFW_KEY_COMMA:
                return KeyCode::Comma;
            case GLFW_KEY_MINUS:
                return KeyCode::Minus;
            case GLFW_KEY_PERIOD:
                return KeyCode::Period;
            case GLFW_KEY_SLASH:
                return KeyCode::Slash;
            case GLFW_KEY_0:
                return KeyCode::D0;
            case GLFW_KEY_1:
                return KeyCode::D1;
            case GLFW_KEY_2:
                return KeyCode::D2;
            case GLFW_KEY_3:
                return KeyCode::D3;
            case GLFW_KEY_4:
                return KeyCode::D4;
            case GLFW_KEY_5:
                return KeyCode::D5;
            case GLFW_KEY_6:
                return KeyCode::D6;
            case GLFW_KEY_7:
                return KeyCode::D7;
            case GLFW_KEY_8:
                return KeyCode::D8;
            case GLFW_KEY_9:
                return KeyCode::D9;
            case GLFW_KEY_SEMICOLON:
                return KeyCode::Semicolon;
            case GLFW_KEY_EQUAL:
                return KeyCode::Equal;
            case GLFW_KEY_A:
                return KeyCode::A;
            case GLFW_KEY_B:
                return KeyCode::B;
            case GLFW_KEY_C:
                return KeyCode::C;
            case GLFW_KEY_D:
                return KeyCode::D;
            case GLFW_KEY_E:
                return KeyCode::E;
            case GLFW_KEY_F:
                return KeyCode::F;
            case GLFW_KEY_G:
                return KeyCode::G;
            case GLFW_KEY_H:
                return KeyCode::H;
            case GLFW_KEY_I:
                return KeyCode::I;
            case GLFW_KEY_J:
                return KeyCode::J;
            case GLFW_KEY_K:
                return KeyCode::K;
            case GLFW_KEY_L:
                return KeyCode::L;
            case GLFW_KEY_M:
                return KeyCode::M;
            case GLFW_KEY_N:
                return KeyCode::N;
            case GLFW_KEY_O:
                return KeyCode::O;
            case GLFW_KEY_P:
                return KeyCode::P;
            case GLFW_KEY_Q:
                return KeyCode::Q;
            case GLFW_KEY_R:
                return KeyCode::R;
            case GLFW_KEY_S:
                return KeyCode::S;
            case GLFW_KEY_T:
                return KeyCode::T;
            case GLFW_KEY_U:
                return KeyCode::U;
            case GLFW_KEY_V:
                return KeyCode::V;
            case GLFW_KEY_W:
                return KeyCode::W;
            case GLFW_KEY_X:
                return KeyCode::X;
            case GLFW_KEY_Y:
                return KeyCode::Y;
            case GLFW_KEY_Z:
                return KeyCode::Z;
            case GLFW_KEY_LEFT_BRACKET:
                return KeyCode::LeftBracket;
            case GLFW_KEY_BACKSLASH:
                return KeyCode::Backslash;
            case GLFW_KEY_RIGHT_BRACKET:
                return KeyCode::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT:
                return KeyCode::GraveAccent;
            case GLFW_KEY_WORLD_1:
                return KeyCode::World1;
            case GLFW_KEY_WORLD_2:
                return KeyCode::World2;
            case GLFW_KEY_ESCAPE:
                return KeyCode::Escape;
            case GLFW_KEY_ENTER:
                return KeyCode::Enter;
            case GLFW_KEY_TAB:
                return KeyCode::Tab;
            case GLFW_KEY_BACKSPACE:
                return KeyCode::Backspace;
            case GLFW_KEY_INSERT:
                return KeyCode::Insert;
            case GLFW_KEY_DELETE:
                return KeyCode::Delete;
            case GLFW_KEY_RIGHT:
                return KeyCode::Right;
            case GLFW_KEY_LEFT:
                return KeyCode::Left;
            case GLFW_KEY_DOWN:
                return KeyCode::Down;
            case GLFW_KEY_UP:
                return KeyCode::Up;
            case GLFW_KEY_PAGE_UP:
                return KeyCode::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return KeyCode::PageDown;
            case GLFW_KEY_HOME:
                return KeyCode::Home;
            case GLFW_KEY_END:
                return KeyCode::End;
            case GLFW_KEY_CAPS_LOCK:
                return KeyCode::CapsLock;
            case GLFW_KEY_NUM_LOCK:
                return KeyCode::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return KeyCode::PrintScreen;
            case GLFW_KEY_PAUSE:
                return KeyCode::Pause;
            case GLFW_KEY_F1:
                return KeyCode::F1;
            case GLFW_KEY_F2:
                return KeyCode::F2;
            case GLFW_KEY_F3:
                return KeyCode::F3;
            case GLFW_KEY_F4:
                return KeyCode::F4;
            case GLFW_KEY_F5:
                return KeyCode::F5;
            case GLFW_KEY_F6:
                return KeyCode::F6;
            case GLFW_KEY_F7:
                return KeyCode::F7;
            case GLFW_KEY_F8:
                return KeyCode::F8;
            case GLFW_KEY_F9:
                return KeyCode::F9;
            case GLFW_KEY_F10:
                return KeyCode::F10;
            case GLFW_KEY_F11:
                return KeyCode::F11;
            case GLFW_KEY_F12:
                return KeyCode::F12;
            case GLFW_KEY_F13:
                return KeyCode::F13;
            case GLFW_KEY_F14:
                return KeyCode::F14;
            case GLFW_KEY_F15:
                return KeyCode::F15;
            case GLFW_KEY_F16:
                return KeyCode::F16;
            case GLFW_KEY_F17:
                return KeyCode::F17;
            case GLFW_KEY_F18:
                return KeyCode::F18;
            case GLFW_KEY_F19:
                return KeyCode::F19;
            case GLFW_KEY_F20:
                return KeyCode::F20;
            case GLFW_KEY_F21:
                return KeyCode::F21;
            case GLFW_KEY_F22:
                return KeyCode::F22;
            case GLFW_KEY_F23:
                return KeyCode::F23;
            case GLFW_KEY_F24:
                return KeyCode::F24;
            case GLFW_KEY_F25:
                return KeyCode::F25;
            case GLFW_KEY_KP_0:
                return KeyCode::KP0;
            case GLFW_KEY_KP_1:
                return KeyCode::KP1;
            case GLFW_KEY_KP_2:
                return KeyCode::KP2;
            case GLFW_KEY_KP_3:
                return KeyCode::KP3;
            case GLFW_KEY_KP_4:
                return KeyCode::KP4;
            case GLFW_KEY_KP_5:
                return KeyCode::KP5;
            case GLFW_KEY_KP_6:
                return KeyCode::KP6;
            case GLFW_KEY_KP_7:
                return KeyCode::KP7;
            case GLFW_KEY_KP_8:
                return KeyCode::KP8;
            case GLFW_KEY_KP_9:
                return KeyCode::KP9;
            case GLFW_KEY_KP_DECIMAL:
                return KeyCode::KPDecimal;
            case GLFW_KEY_KP_DIVIDE:
                return KeyCode::KPDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return KeyCode::KPMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return KeyCode::KPSubtract;
            case GLFW_KEY_KP_ADD:
                return KeyCode::KPAdd;
            case GLFW_KEY_KP_ENTER:
                return KeyCode::KPEnter;
            case GLFW_KEY_KP_EQUAL:
                return KeyCode::KPEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return KeyCode::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return KeyCode::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return KeyCode::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return KeyCode::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return KeyCode::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return KeyCode::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return KeyCode::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return KeyCode::RightSuper;
            case GLFW_KEY_MENU:
                return KeyCode::Menu;
            default:
                return KeyCode::Unknown;
        }
    }

    /** @brief Converts a GLFW mouse button to a Vulkyrie mouse button.
     * @param glfwMouseButton The GLFW mouse button to convert.
     * @returns The corresponding Vulkyrie mouse button.
     */
    static constexpr MouseButton ConvertGLFWMouseButtonToVulkyrieMouseButton(int glfwMouseButton) {
        switch (glfwMouseButton) {
            case GLFW_MOUSE_BUTTON_1:
                return MouseButton::MouseButton1;
            case GLFW_MOUSE_BUTTON_2:
                return MouseButton::MouseButton2;
            case GLFW_MOUSE_BUTTON_3:
                return MouseButton::MouseButton3;
            case GLFW_MOUSE_BUTTON_4:
                return MouseButton::MouseButton4;
            case GLFW_MOUSE_BUTTON_5:
                return MouseButton::MouseButton5;
            case GLFW_MOUSE_BUTTON_6:
                return MouseButton::MouseButton6;
            case GLFW_MOUSE_BUTTON_7:
                return MouseButton::MouseButton7;
            case GLFW_MOUSE_BUTTON_8:
                return MouseButton::MouseButton8;
            default:
                return MouseButton::Unknown;
        }
    }

    static constexpr u8 shiftKeyModifier = std::to_underlying(KeyModifier::Shift);
    static constexpr u8 controlKeyModifier = std::to_underlying(KeyModifier::Control);
    static constexpr u8 altKeyModifier = std::to_underlying(KeyModifier::Alt);
    static constexpr u8 superKeyModifier = std::to_underlying(KeyModifier::Super);
    static constexpr u8 capsLockKeyModifier = std::to_underlying(KeyModifier::CapsLock);
    static constexpr u8 numLockKeyModifier = std::to_underlying(KeyModifier::NumLock);

    /** @brief Converts GLFW modifier flags to Vulkyrie key modifiers.
     * @param glfwMods The GLFW modifier flags.
     * @returns The corresponding Vulkyrie key modifiers.
     */
    static constexpr KeyModifier GetModifiersFromGLFW(int glfwMods) {
        u8 modifiers = 0U;

        if (glfwMods & GLFW_MOD_SHIFT) {
            modifiers |= shiftKeyModifier;
        }

        if (glfwMods & GLFW_MOD_CONTROL) {
            modifiers |= controlKeyModifier;
        }

        if (glfwMods & GLFW_MOD_ALT) {
            modifiers |= altKeyModifier;
        }

        if (glfwMods & GLFW_MOD_SUPER) {
            modifiers |= superKeyModifier;
        }

        if (glfwMods & GLFW_MOD_CAPS_LOCK) {
            modifiers |= capsLockKeyModifier;
        }

        if (glfwMods & GLFW_MOD_NUM_LOCK) {
            modifiers |= numLockKeyModifier;
        }

        return static_cast<KeyModifier>(modifiers);
    }

    VulkyrieGLFWPlatform::VulkyrieGLFWPlatform(const WindowProps &windowProps, const EventCallbackFn &eventCallbackFn)
        : Platform(windowProps, eventCallbackFn)
        , _window(nullptr) {};

    VulkyrieGLFWPlatform::~VulkyrieGLFWPlatform() {
        glfwDestroyWindow(_window);
        glfwTerminate();
    }

    StatusCode VulkyrieGLFWPlatform::CreateWindow() {
        // GLFW: initialize and configure
        glfwInit();

        // Set GLFW error callback.
        glfwSetErrorCallback([](int errorCode, const char *description) { VERROR("GLFW Error {}: {}", errorCode, description); });

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

        if (_windowProps.GraphicsAPI == GraphicsAPI::OpenGL) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(VULKYRIE_DEBUG)
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        }

        // GLFW window creation
        _window = glfwCreateWindow(_windowProps.Width, _windowProps.Height, _windowProps.Title.c_str(), nullptr, nullptr);

        // Check if window creation failed.
        if (nullptr == _window) {
            VFATAL("Failed to create GLFW window");

            // Terminate GLFW.
            glfwTerminate();

            // Return an error code.
            return StatusCode::FailedToCreateWindow;
        }

        // Set the window user pointer to this instance.
        glfwSetWindowUserPointer(_window, &_eventCallbackFn);

        // Set window event callbacks.
        glfwSetFramebufferSizeCallback(_window, [](GLFWwindow *window, int width, int height) {
            // Reset the height and width of the viewport.
            glViewport(0, 0, width, height);

            // Create the window resize event.
            WindowResizedEvent event(width, height);

            // Get the window user pointer.
            const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            callbackFn(event);
        });

        glfwSetWindowCloseCallback(_window, [](GLFWwindow *window) {
            WindowClosedEvent event;

            // Get the window user pointer.
            const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            callbackFn(event);
        });

        glfwSetKeyCallback(_window, [](GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action, int mods) {
            const KeyCode code = ConvertGLFWKeyCodeToVulkyrieKeyCode(key);

            switch (action) {
                case GLFW_PRESS: {
                    const KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    KeyPressedEvent event(code, modifiers);

                    // Get the window user pointer.
                    const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    callbackFn(event);

                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(code);

                    // Get the window user pointer.
                    const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    callbackFn(event);

                    break;
                }
                case GLFW_REPEAT: {
                    const KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    KeyPressedEvent event(code, modifiers, true);

                    // Get the window user pointer.
                    const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    callbackFn(event);

                    break;
                }
                default:
                    break;
            }
        });

        // glfwSetCharCallback(_window, [](GLFWwindow *window, unsigned int codepoint) {
        //     KeyCode keycode = ConvertGLFWKeyCodeToVulkyrieKeyCode(codepoint);
        //     KeyCharEvent event(keycode);

        //     // Get the window user pointer.
        //     Application& app = *(Application *)glfwGetWindowUserPointer(window);

        //     // Dispatch the event.
        //     app.RaiseEvent(event);
        // });

        glfwSetMouseButtonCallback(_window, [](GLFWwindow *window, int button, int action, int mods) {
            const MouseButton mouseButton = ConvertGLFWMouseButtonToVulkyrieMouseButton(button);

            switch (action) {
                case GLFW_PRESS: {
                    KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    MouseButtonPressedEvent event(mouseButton, modifiers);

                    // Get the window user pointer.
                    const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    callbackFn(event);

                    break;
                }
                case GLFW_RELEASE: {
                    MouseButtonReleasedEvent event(mouseButton);

                    // Get the window user pointer.
                    const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    callbackFn(event);

                    break;
                }
                default:;
            }
        });

        glfwSetScrollCallback(_window, [](GLFWwindow *window, f64 offsetX, f64 offsetY) {
            MouseScrolledEvent event(offsetX, offsetY);

            // Get the window user pointer.
            const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            callbackFn(event);
        });

        glfwSetCursorPosCallback(_window, [](GLFWwindow *window, const f64 positionX, const f64 positionY) {
            MouseMovedEvent event(positionX, positionY);

            // Get the window user pointer.
            const EventCallbackFn &callbackFn = *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            callbackFn(event);
        });

        // Enable/Disable VSync based on the window settigns.
        // SetVSync(_windowProps.EnableVSync);

        // Return success.
        return StatusCode::Successful;
    }

    void VulkyrieGLFWPlatform::SetVSync(bool enable) {
        glfwSwapInterval(static_cast<i32>(enable));
    }

    void VulkyrieGLFWPlatform::CaptureMouseOnFocus(bool enable) {
        if (enable) {
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    StatusCode VulkyrieGLFWPlatform::CloseWindow() {
        // glfw: terminate, clearing all previously allocated GLFW resources.
        // glfwDestroyWindow(_window);
        // glfwTerminate();

        return StatusCode::Successful;
    }

    // static constexpr GLenum GetOpenGLDataTypeFromShaderDataType(ShaderDataType type) noexcept {
    //     switch (type) {
    //         case ShaderDataType::Float:
    //         case ShaderDataType::Float2:
    //         case ShaderDataType::Float3:
    //         case ShaderDataType::Float4:
    //         case ShaderDataType::Mat3:
    //         case ShaderDataType::Mat4:
    //             return GL_FLOAT;
    //         case ShaderDataType::Int:
    //         case ShaderDataType::Int2:
    //         case ShaderDataType::Int3:
    //         case ShaderDataType::Int4:
    //             return GL_INT;
    //         case ShaderDataType::Bool:
    //             return GL_BOOL;
    //         default:
    //             return GL_INVALID_ENUM;
    //     }
    // }
} // namespace Vulkyrie
