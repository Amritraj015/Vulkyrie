#include "generic_platform.h"
#include "events/application/window_close.h"
#include "core/logger.h"

namespace Vulkyrie::Platform {
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void window_close_callback(GLFWwindow* window);
    void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void window_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    void window_scroll_callback(GLFWwindow* window, double offsetX, double offsetY);
    void window_mouse_move_callback(GLFWwindow* window, double positionX, double positionY);

    const char *vertexShaderSource = "#version 330 core\n"
                                     "layout(location = 0) in vec3 aPos;\n"
                                     "void main()\n"
                                     "{\n"
                                     "    gl_Position = vec4(aPos, 1.0);\n"
                                     "}\n\0";

    const char *fragmentShaderSource = "#version 330 core\n"
                                       "out vec4 FragColor;\n"
                                       "void main()\n"
                                       "{\n"
                                       "    FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
                                       "}\n\0";

    GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f, // bottom left
        0.5f,  -0.5f, 0.0f, // bottom right
        0.0f,  0.5f,  0.0f  // top
    };

    Vulkyrie::Core::StatusCode GenericPlatform::CreateNewWindow(Vulkyrie::Core::VulkyrieWindowProps props) {
        // glfw: initialize and configure
        // ------------------------------
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // glfw window creation
        // --------------------
        window = glfwCreateWindow(props.width, props.height, props.title, nullptr, nullptr);

        if (nullptr == window) {
            VFATAL("Failed to create GLFW window");
            glfwTerminate();

            return Vulkyrie::Core::StatusCode::FailedToCreateWindow;
        }

        glfwMakeContextCurrent(window);

        // Set window event callbacks.
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetWindowCloseCallback(window, window_close_callback);
        glfwSetKeyCallback(window, window_key_callback);
        glfwSetMouseButtonCallback(window, window_mouse_button_callback);
        glfwSetScrollCallback(window, window_scroll_callback);
        glfwSetCursorPosCallback(window, window_mouse_move_callback);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            VFATAL("Failed to initialize GLAD");
            return Vulkyrie::Core::StatusCode::FailedToCreateWindow;
        }

        gladLoadGL();                    // load OpenGL functions
        glViewport(0, 0, props.width, props.height); // set the viewport

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDetachShader(shaderProgram, vertexShader);
        glDetachShader(shaderProgram, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glClearColor(0.07f, 0.13f, 0.17f, 1.0f); // set clear color
        glClear(GL_COLOR_BUFFER_BIT);            // clear the color buffer
        glfwSwapBuffers(window);

        // render loop
        // -----------
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.07f, 0.13f, 0.17f, 1.0f); // set clear color
            glClear(GL_COLOR_BUFFER_BIT);            // clear the color buffer

            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);

            glDrawArrays(GL_TRIANGLES, 0, 3);
            glfwSwapBuffers(window);

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwPollEvents();
        }

        return Vulkyrie::Core::StatusCode::Successful;
    }

    Vulkyrie::Core::StatusCode GenericPlatform::CloseWindow() {
        // glfw: terminate, clearing all previously allocated GLFW resources.
        // ------------------------------------------------------------------
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);

        glfwDestroyWindow(window);
        glfwTerminate();

        return Vulkyrie::Core::StatusCode::Successful;
    }

    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    // ---------------------------------------------------------------------------------------------
    void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        glViewport(0, 0, width, height);
    }

    void window_close_callback(GLFWwindow* window) {
        Vulkyrie::Events::WindowCloseEvent event;
        // VTRACE("Window close event triggered");
    }

    void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        switch (action) {
            case GLFW_PRESS:
            {
                // VTRACE("Key Pressed: {}", key);

                // std::cout << "Key Pressed: " << key << std::endl;

                if (key == GLFW_KEY_ESCAPE) {
                    glfwSetWindowShouldClose(window, true);
                }
                break;
            }
            case GLFW_RELEASE:
            {
                // VTRACE("Key Released: {}", key);
                break;
            }
            case GLFW_REPEAT:
            {
                // VTRACE("Key Repeated: {}", key);
                break;
            }
            default:
                break;
        }
    }

    void window_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        switch (action) {
            case GLFW_PRESS:
                // VTRACE("Mouse Button Pressed: {}", button);
                break;
            case GLFW_RELEASE:
                // VTRACE("Mouse Button Released: {}", button);
                break;
            default:
                break;
        }
    }

    void window_scroll_callback(GLFWwindow* window, double offsetX, double offsetY) {
        // VTRACE("Mouse Scrolled - X offset: {}, Y offset: {}", offsetX, offsetY);
    }

    void window_mouse_move_callback(GLFWwindow* window, double positionX, double positionY) {
        // VTRACE("Mouse Moved to Position - X: {}, Y: {}", positionX, positionY);
    }
} // namespace Vulkyrie
