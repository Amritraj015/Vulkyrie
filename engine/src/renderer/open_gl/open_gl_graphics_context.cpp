#include "renderer/open_gl/open_gl_graphics_context.h"
#include "core/logger.h"
#include "core/application.h"

namespace Vulkyrie::Renderer {
    OpenGLGraphicsContext::OpenGLGraphicsContext(void *windowHandle)
        : _windowHandle(static_cast<GLFWwindow *>(windowHandle)) {}

    Vulkyrie::Core::StatusCode OpenGLGraphicsContext::Initialize() {
        using Vulkyrie::Core::Application;

        // Make the OpenGL context current.
        glfwMakeContextCurrent(_windowHandle);

        // GLAD: load all OpenGL function pointers
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            VFATAL("Failed to initialize GLAD");
            return Vulkyrie::Core::StatusCode::FailedToInitializeGLAD;
        }

        // TODO: This does not belong here. Move it to the renderer initialization.
        glViewport(0, 0, Application::GetSingleton().GetWindowWidth(), Application::GetSingleton().GetWindowHeight());

        return Vulkyrie::Core::StatusCode::Successful;
    };

    void OpenGLGraphicsContext::SwapBuffers() { glfwSwapBuffers(_windowHandle); }
} // namespace Vulkyrie::Renderer
