#include "generic_window.h"
#include "core/logger.h"
#include "events/application/window_close_event.h"
#include "events/application/window_resize_event.h"
#include "events/mouse/mouse_moved_event.h"
#include "events/mouse/mouse_button_pressed_event.h"
#include "events/mouse/mouse_button_released_event.h"
#include "events/mouse/mouse_scrolled_event.h"
#include "events/keyboard/key_pressed_event.h"
#include "events/keyboard/key_released_event.h"

// TODO: Remove these.
#include "renderer/graphics_shader.h"
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"


namespace Vulkyrie::Core {
    /** @brief Converts a GLFW key code to a Vulkyrie key code.
     * @param glfwKeyCode The GLFW key code to convert.
     * @returns The corresponding Vulkyrie key code.
     */
    inline Vulkyrie::Events::KeyCode ConvertGLFWKeyCodeToVulkyrieKeyCode(int glfwKeyCode);

    /** @brief Converts a GLFW mouse button to a Vulkyrie mouse button.
     * @param glfwMouseButton The GLFW mouse button to convert.
     * @returns The corresponding Vulkyrie mouse button.
     */
    inline Vulkyrie::Events::MouseButton ConvertGLFWMouseButtonToVulkyrieMouseButton(int glfwMouseButton);

    /** @brief Converts GLFW modifier flags to Vulkyrie key modifiers.
     * @param glfwMods The GLFW modifier flags.
     * @returns The corresponding Vulkyrie key modifiers.
     */
    inline Vulkyrie::Events::KeyModifier GetModifiersFromGLFW(int glfwMods);

    constexpr static int shiftModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::Shift);
    constexpr static int controlModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::Control);
    constexpr static int altModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::Alt);
    constexpr static int superModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::Super);
    constexpr static int capsLockModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::CapsLock);
    constexpr static int numLockModifierAsInt = std::to_underlying(Vulkyrie::Events::KeyModifier::NumLock);

    GenericWindow::GenericWindow(const Vulkyrie::Core::Application &application) : Window(application) {};

    Vulkyrie::Core::StatusCode GenericWindow::Create() {
        // Set GLFW error callback.
        glfwSetErrorCallback([](int errorCode, const char *description) {
            VERROR("GLFW Error {}: {}", errorCode, description);
        });

        // GLFW: initialize and configure
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // GLFW window creation
        _window = glfwCreateWindow(appRef.windowProps.width, appRef.windowProps.height, appRef.windowProps.title, nullptr, nullptr);

        // Check if window creation failed.
        if (nullptr == _window) {
            VFATAL("Failed to create GLFW window");

            // Terminate GLFW.
            glfwTerminate();

            // Return an error code.
            return Vulkyrie::Core::StatusCode::FailedToCreateWindow;
        }

        // Set the window user pointer to this instance.
        glfwSetWindowUserPointer(_window, (void *)&appRef);

        // Make the OpenGL context current.
        glfwMakeContextCurrent(_window);

        // Enable/Disable VSync.
        glfwSwapInterval(appRef.windowProps.vsync ? 1 : 0);

        // Set window event callbacks.
        glfwSetFramebufferSizeCallback(_window, [](GLFWwindow *window, int width, int height) {
            // Reset the height and width of the viewport.
            glViewport(0, 0, width, height);

            // Create the window resize event.
            Vulkyrie::Events::WindowResizeEvent event(width, height);

            // Get the window user pointer.
            const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            app.RaiseEvent(event);
        });

        glfwSetWindowCloseCallback(_window, [](GLFWwindow *window) {
            Vulkyrie::Events::WindowCloseEvent event;

            // Get the window user pointer.
            const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            app.RaiseEvent(event);
        });

        glfwSetKeyCallback(_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            const Vulkyrie::Events::KeyCode code = ConvertGLFWKeyCodeToVulkyrieKeyCode(key);

            switch (action) {
                case GLFW_PRESS: {
                    const Vulkyrie::Events::KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    Vulkyrie::Events::KeyPressedEvent event(code, modifiers);

                    // Get the window user pointer.
                    const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    app.RaiseEvent(event);

                    // TODO: Remove this.
                    if (key == GLFW_KEY_ESCAPE) {
                        glfwSetWindowShouldClose(window, true);
                    }

                    break;
                }
                case GLFW_RELEASE: {
                    Vulkyrie::Events::KeyReleasedEvent event(code);

                    // Get the window user pointer.
                    const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    app.RaiseEvent(event);

                    break;
                }
                case GLFW_REPEAT: {
                    Vulkyrie::Events::KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    Vulkyrie::Events::KeyPressedEvent event(code, modifiers, true);

                    // Get the window user pointer.
                    const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    app.RaiseEvent(event);

                    break;
                }
                default:
                    break;
            }
        });

        // glfwSetCharCallback(_window, [](GLFWwindow *window, unsigned int codepoint) {
        //     Vulkyrie::Events::KeyCode keycode = ConvertGLFWKeyCodeToVulkyrieKeyCode(codepoint);
        //     Vulkyrie::Events::KeyCharEvent event(keycode);

        //     // Get the window user pointer.
        //     Vulkyrie::Core::Application& app = *(Vulkyrie::Core::Application *)glfwGetWindowUserPointer(window);

        //     // Dispatch the event.
        //     app.RaiseEvent(event);
        // });

        glfwSetMouseButtonCallback(_window, [](GLFWwindow *window, int button, int action, int mods) {
            Vulkyrie::Events::MouseButton mouseButton = ConvertGLFWMouseButtonToVulkyrieMouseButton(button);

            switch (action) {
                case GLFW_PRESS: {
                    Vulkyrie::Events::KeyModifier modifiers = GetModifiersFromGLFW(mods);
                    Vulkyrie::Events::MouseButtonPressedEvent event(mouseButton, modifiers);

                    // Get the window user pointer.
                    const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    app.RaiseEvent(event);

                    break;
                }
                case GLFW_RELEASE: {
                    Vulkyrie::Events::MouseButtonReleasedEvent event(mouseButton);

                    // Get the window user pointer.
                    const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

                    // Dispatch the event.
                    app.RaiseEvent(event);

                    break;
                }
            }
        });

        glfwSetScrollCallback(_window, [](GLFWwindow *window, double offsetX, double offsetY) {
            Vulkyrie::Events::MouseScrolledEvent event(offsetX, offsetY);

            // Get the window user pointer.
            const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            app.RaiseEvent(event);
        });

        glfwSetCursorPosCallback(_window, [](GLFWwindow *window, double positionX, double positionY) {
            Vulkyrie::Events::MouseMovedEvent event(positionX, positionY);

            // Get the window user pointer.
            const Vulkyrie::Core::Application &app = *static_cast<Vulkyrie::Core::Application *>(glfwGetWindowUserPointer(window));

            // Dispatch the event.
            app.RaiseEvent(event);
        });

        // GLAD: load all OpenGL function pointers
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            VFATAL("Failed to initialize GLAD");

            return Vulkyrie::Core::StatusCode::FailedToInitializeGLAD;
        }

        // set the viewport
        glViewport(0, 0, appRef.windowProps.width, appRef.windowProps.height);

        // int nrAttributes;
        // glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);

        // VINFO("Total vertex attributes allowed: {}", nrAttributes)

        // -------------------------------------------------------------------------------
        // Load shaders.
        Vulkyrie::Renderer::GraphicsShader graphicsShader(
            "assets/shaders/triangle.vert.glsl",
            "assets/shaders/triangle.frag.glsl"
        );

        // Check if shader program creation failed.
        if (!graphicsShader.IsValid()) {
            // Log a fatal error.
            VFATAL("Failed to create graphics shader");

            // Return an error code to represent failure to compile shader program.
            return Vulkyrie::Core::StatusCode::FailedToCompileShaderProgram;
        }

        constexpr f32 vertices[] = {
            // positions         // colors
            // 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
            // -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
            // 0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // top

            // positions          // colors           // texture coords
            0.5f, 0.5f,  0.0f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
            0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
            -0.5f, 0.5f,  0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
        };

        unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };

        u32 vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // i32 vertexColorLocation = glGetUniformLocation(graphicsShader.GetShaderProgram(), "ourColor");
        // -------------------------------------------------------------------------------


        // -------------------------------------------------------------------------------
        // Generate Textures.
        stbi_set_flip_vertically_on_load(true);

        i32 width, height, nrChannels;
        const u8 *image = stbi_load("assets/textures/wall.jpg", &width, &height, &nrChannels, 0);

        i32 aWidth, aHeight, anrChannels;
        const u8 *aImage = stbi_load("assets/textures/awesomeface.png", &aWidth, &aHeight, &anrChannels, 0);

        if (!image) {
            VERROR("Failed to load texture: assets/textures/container.jpg");

            return Vulkyrie::Core::StatusCode::FailedToCreateWindow;
        }

        // Create the texture handle.
        u32 textureHandle, aTextureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &aTextureHandle);
        glBindTexture(GL_TEXTURE_2D, aTextureHandle);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, aWidth, aHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, aImage);
        glGenerateMipmap(GL_TEXTURE_2D);

        graphicsShader.Use();                                                // don't forget to activate the shader before setting uniforms!
        glUniform1i(glGetUniformLocation(graphicsShader.GetShaderProgram(), "texture1"), 0); // set it manually
        graphicsShader.SetIntUniform("texture2", 1);                                // or with shader class

        stbi_image_free((void*)image);
        stbi_image_free((void*)aImage);
        // -------------------------------------------------------------------------------

        // Game/Render loop.
        while (!glfwWindowShouldClose(_window)) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT); // clear the color buffer

            // bind Texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureHandle);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, aTextureHandle);

            // render container
            // Use the graphics shader program.
            // graphicsShader.Use();
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // float timeValue = glfwGetTime();
            // float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
            // float redValue = (cos(timeValue) / 2.0f) + 0.1f;
            // float blueValue = (sin(timeValue) / 2.0f) + 1.3f;
            // glUniform4f(vertexColorLocation, redValue, greenValue, blueValue, 1.0f);

            // glBindVertexArray(vao);
            // glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(_window);

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);

        return Vulkyrie::Core::StatusCode::Successful;
    }

    void GenericWindow::ToggleWireframeMode(bool enable) {
        if (enable) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    Vulkyrie::Core::StatusCode GenericWindow::Close() {
        // glfw: terminate, clearing all previously allocated GLFW resources.
        glfwDestroyWindow(_window);
        glfwTerminate();

        return Vulkyrie::Core::StatusCode::Successful;
    }

    inline Vulkyrie::Events::KeyModifier GetModifiersFromGLFW(int glfwMods) {
        i32 modifiers = 0;

        if (glfwMods & GLFW_MOD_SHIFT) {
            modifiers |= shiftModifierAsInt;
        }

        if (glfwMods & GLFW_MOD_CONTROL) {
            modifiers |= controlModifierAsInt;
        }

        if (glfwMods & GLFW_MOD_ALT) {
            modifiers |= altModifierAsInt;
        }

        if (glfwMods & GLFW_MOD_SUPER) {
            modifiers |= superModifierAsInt;
        }

        if (glfwMods & GLFW_MOD_CAPS_LOCK) {
            modifiers |= capsLockModifierAsInt;
        }

        if (glfwMods & GLFW_MOD_NUM_LOCK) {
            modifiers |= numLockModifierAsInt;
        }

        return static_cast<Vulkyrie::Events::KeyModifier>(modifiers);
    }

    inline Vulkyrie::Events::MouseButton ConvertGLFWMouseButtonToVulkyrieMouseButton(int glfwMouseButton) {
        switch (glfwMouseButton) {
            case GLFW_MOUSE_BUTTON_1:
                return Vulkyrie::Events::MouseButton::MouseButton1;
            case GLFW_MOUSE_BUTTON_2:
                return Vulkyrie::Events::MouseButton::MouseButton2;
            case GLFW_MOUSE_BUTTON_3:
                return Vulkyrie::Events::MouseButton::MouseButton3;
            case GLFW_MOUSE_BUTTON_4:
                return Vulkyrie::Events::MouseButton::MouseButton4;
            case GLFW_MOUSE_BUTTON_5:
                return Vulkyrie::Events::MouseButton::MouseButton5;
            case GLFW_MOUSE_BUTTON_6:
                return Vulkyrie::Events::MouseButton::MouseButton6;
            case GLFW_MOUSE_BUTTON_7:
                return Vulkyrie::Events::MouseButton::MouseButton7;
            case GLFW_MOUSE_BUTTON_8:
                return Vulkyrie::Events::MouseButton::MouseButton8;
            default:
                return Vulkyrie::Events::MouseButton::Unknown;
        }
    }

    inline Vulkyrie::Events::KeyCode ConvertGLFWKeyCodeToVulkyrieKeyCode(int glfwKeyCode) {
        switch (glfwKeyCode) {
            case GLFW_KEY_SPACE:
                return Vulkyrie::Events::KeyCode::Space;
            case GLFW_KEY_APOSTROPHE:
                return Vulkyrie::Events::KeyCode::Apostrophe;
            case GLFW_KEY_COMMA:
                return Vulkyrie::Events::KeyCode::Comma;
            case GLFW_KEY_MINUS:
                return Vulkyrie::Events::KeyCode::Minus;
            case GLFW_KEY_PERIOD:
                return Vulkyrie::Events::KeyCode::Period;
            case GLFW_KEY_SLASH:
                return Vulkyrie::Events::KeyCode::Slash;
            case GLFW_KEY_0:
                return Vulkyrie::Events::KeyCode::D0;
            case GLFW_KEY_1:
                return Vulkyrie::Events::KeyCode::D1;
            case GLFW_KEY_2:
                return Vulkyrie::Events::KeyCode::D2;
            case GLFW_KEY_3:
                return Vulkyrie::Events::KeyCode::D3;
            case GLFW_KEY_4:
                return Vulkyrie::Events::KeyCode::D4;
            case GLFW_KEY_5:
                return Vulkyrie::Events::KeyCode::D5;
            case GLFW_KEY_6:
                return Vulkyrie::Events::KeyCode::D6;
            case GLFW_KEY_7:
                return Vulkyrie::Events::KeyCode::D7;
            case GLFW_KEY_8:
                return Vulkyrie::Events::KeyCode::D8;
            case GLFW_KEY_9:
                return Vulkyrie::Events::KeyCode::D9;
            case GLFW_KEY_SEMICOLON:
                return Vulkyrie::Events::KeyCode::Semicolon;
            case GLFW_KEY_EQUAL:
                return Vulkyrie::Events::KeyCode::Equal;
            case GLFW_KEY_A:
                return Vulkyrie::Events::KeyCode::A;
            case GLFW_KEY_B:
                return Vulkyrie::Events::KeyCode::B;
            case GLFW_KEY_C:
                return Vulkyrie::Events::KeyCode::C;
            case GLFW_KEY_D:
                return Vulkyrie::Events::KeyCode::D;
            case GLFW_KEY_E:
                return Vulkyrie::Events::KeyCode::E;
            case GLFW_KEY_F:
                return Vulkyrie::Events::KeyCode::F;
            case GLFW_KEY_G:
                return Vulkyrie::Events::KeyCode::G;
            case GLFW_KEY_H:
                return Vulkyrie::Events::KeyCode::H;
            case GLFW_KEY_I:
                return Vulkyrie::Events::KeyCode::I;
            case GLFW_KEY_J:
                return Vulkyrie::Events::KeyCode::J;
            case GLFW_KEY_K:
                return Vulkyrie::Events::KeyCode::K;
            case GLFW_KEY_L:
                return Vulkyrie::Events::KeyCode::L;
            case GLFW_KEY_M:
                return Vulkyrie::Events::KeyCode::M;
            case GLFW_KEY_N:
                return Vulkyrie::Events::KeyCode::N;
            case GLFW_KEY_O:
                return Vulkyrie::Events::KeyCode::O;
            case GLFW_KEY_P:
                return Vulkyrie::Events::KeyCode::P;
            case GLFW_KEY_Q:
                return Vulkyrie::Events::KeyCode::Q;
            case GLFW_KEY_R:
                return Vulkyrie::Events::KeyCode::R;
            case GLFW_KEY_S:
                return Vulkyrie::Events::KeyCode::S;
            case GLFW_KEY_T:
                return Vulkyrie::Events::KeyCode::T;
            case GLFW_KEY_U:
                return Vulkyrie::Events::KeyCode::U;
            case GLFW_KEY_V:
                return Vulkyrie::Events::KeyCode::V;
            case GLFW_KEY_W:
                return Vulkyrie::Events::KeyCode::W;
            case GLFW_KEY_X:
                return Vulkyrie::Events::KeyCode::X;
            case GLFW_KEY_Y:
                return Vulkyrie::Events::KeyCode::Y;
            case GLFW_KEY_Z:
                return Vulkyrie::Events::KeyCode::Z;
            case GLFW_KEY_LEFT_BRACKET:
                return Vulkyrie::Events::KeyCode::LeftBracket;
            case GLFW_KEY_BACKSLASH:
                return Vulkyrie::Events::KeyCode::Backslash;
            case GLFW_KEY_RIGHT_BRACKET:
                return Vulkyrie::Events::KeyCode::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT:
                return Vulkyrie::Events::KeyCode::GraveAccent;
            case GLFW_KEY_WORLD_1:
                return Vulkyrie::Events::KeyCode::World1;
            case GLFW_KEY_WORLD_2:
                return Vulkyrie::Events::KeyCode::World2;
            case GLFW_KEY_ESCAPE:
                return Vulkyrie::Events::KeyCode::Escape;
            case GLFW_KEY_ENTER:
                return Vulkyrie::Events::KeyCode::Enter;
            case GLFW_KEY_TAB:
                return Vulkyrie::Events::KeyCode::Tab;
            case GLFW_KEY_BACKSPACE:
                return Vulkyrie::Events::KeyCode::Backspace;
            case GLFW_KEY_INSERT:
                return Vulkyrie::Events::KeyCode::Insert;
            case GLFW_KEY_DELETE:
                return Vulkyrie::Events::KeyCode::Delete;
            case GLFW_KEY_RIGHT:
                return Vulkyrie::Events::KeyCode::Right;
            case GLFW_KEY_LEFT:
                return Vulkyrie::Events::KeyCode::Left;
            case GLFW_KEY_DOWN:
                return Vulkyrie::Events::KeyCode::Down;
            case GLFW_KEY_UP:
                return Vulkyrie::Events::KeyCode::Up;
            case GLFW_KEY_PAGE_UP:
                return Vulkyrie::Events::KeyCode::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return Vulkyrie::Events::KeyCode::PageDown;
            case GLFW_KEY_HOME:
                return Vulkyrie::Events::KeyCode::Home;
            case GLFW_KEY_END:
                return Vulkyrie::Events::KeyCode::End;
            case GLFW_KEY_CAPS_LOCK:
                return Vulkyrie::Events::KeyCode::CapsLock;
            case GLFW_KEY_NUM_LOCK:
                return Vulkyrie::Events::KeyCode::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return Vulkyrie::Events::KeyCode::PrintScreen;
            case GLFW_KEY_PAUSE:
                return Vulkyrie::Events::KeyCode::Pause;
            case GLFW_KEY_F1:
                return Vulkyrie::Events::KeyCode::F1;
            case GLFW_KEY_F2:
                return Vulkyrie::Events::KeyCode::F2;
            case GLFW_KEY_F3:
                return Vulkyrie::Events::KeyCode::F3;
            case GLFW_KEY_F4:
                return Vulkyrie::Events::KeyCode::F4;
            case GLFW_KEY_F5:
                return Vulkyrie::Events::KeyCode::F5;
            case GLFW_KEY_F6:
                return Vulkyrie::Events::KeyCode::F6;
            case GLFW_KEY_F7:
                return Vulkyrie::Events::KeyCode::F7;
            case GLFW_KEY_F8:
                return Vulkyrie::Events::KeyCode::F8;
            case GLFW_KEY_F9:
                return Vulkyrie::Events::KeyCode::F9;
            case GLFW_KEY_F10:
                return Vulkyrie::Events::KeyCode::F10;
            case GLFW_KEY_F11:
                return Vulkyrie::Events::KeyCode::F11;
            case GLFW_KEY_F12:
                return Vulkyrie::Events::KeyCode::F12;
            case GLFW_KEY_F13:
                return Vulkyrie::Events::KeyCode::F13;
            case GLFW_KEY_F14:
                return Vulkyrie::Events::KeyCode::F14;
            case GLFW_KEY_F15:
                return Vulkyrie::Events::KeyCode::F15;
            case GLFW_KEY_F16:
                return Vulkyrie::Events::KeyCode::F16;
            case GLFW_KEY_F17:
                return Vulkyrie::Events::KeyCode::F17;
            case GLFW_KEY_F18:
                return Vulkyrie::Events::KeyCode::F18;
            case GLFW_KEY_F19:
                return Vulkyrie::Events::KeyCode::F19;
            case GLFW_KEY_F20:
                return Vulkyrie::Events::KeyCode::F20;
            case GLFW_KEY_F21:
                return Vulkyrie::Events::KeyCode::F21;
            case GLFW_KEY_F22:
                return Vulkyrie::Events::KeyCode::F22;
            case GLFW_KEY_F23:
                return Vulkyrie::Events::KeyCode::F23;
            case GLFW_KEY_F24:
                return Vulkyrie::Events::KeyCode::F24;
            case GLFW_KEY_F25:
                return Vulkyrie::Events::KeyCode::F25;
            case GLFW_KEY_KP_0:
                return Vulkyrie::Events::KeyCode::KP0;
            case GLFW_KEY_KP_1:
                return Vulkyrie::Events::KeyCode::KP1;
            case GLFW_KEY_KP_2:
                return Vulkyrie::Events::KeyCode::KP2;
            case GLFW_KEY_KP_3:
                return Vulkyrie::Events::KeyCode::KP3;
            case GLFW_KEY_KP_4:
                return Vulkyrie::Events::KeyCode::KP4;
            case GLFW_KEY_KP_5:
                return Vulkyrie::Events::KeyCode::KP5;
            case GLFW_KEY_KP_6:
                return Vulkyrie::Events::KeyCode::KP6;
            case GLFW_KEY_KP_7:
                return Vulkyrie::Events::KeyCode::KP7;
            case GLFW_KEY_KP_8:
                return Vulkyrie::Events::KeyCode::KP8;
            case GLFW_KEY_KP_9:
                return Vulkyrie::Events::KeyCode::KP9;
            case GLFW_KEY_KP_DECIMAL:
                return Vulkyrie::Events::KeyCode::KPDecimal;
            case GLFW_KEY_KP_DIVIDE:
                return Vulkyrie::Events::KeyCode::KPDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return Vulkyrie::Events::KeyCode::KPMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return Vulkyrie::Events::KeyCode::KPSubtract;
            case GLFW_KEY_KP_ADD:
                return Vulkyrie::Events::KeyCode::KPAdd;
            case GLFW_KEY_KP_ENTER:
                return Vulkyrie::Events::KeyCode::KPEnter;
            case GLFW_KEY_KP_EQUAL:
                return Vulkyrie::Events::KeyCode::KPEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return Vulkyrie::Events::KeyCode::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return Vulkyrie::Events::KeyCode::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return Vulkyrie::Events::KeyCode::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return Vulkyrie::Events::KeyCode::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return Vulkyrie::Events::KeyCode::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return Vulkyrie::Events::KeyCode::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return Vulkyrie::Events::KeyCode::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return Vulkyrie::Events::KeyCode::RightSuper;
            case GLFW_KEY_MENU:
                return Vulkyrie::Events::KeyCode::Menu;
            default:
                return Vulkyrie::Events::KeyCode::Unknown;
        }
    }
} // namespace Vulkyrie::Core
