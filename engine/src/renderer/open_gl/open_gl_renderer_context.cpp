#include "core/application.h"
#include "renderer/renderer.h"
#include "renderer/open_gl/open_gl_renderer_context.h"

namespace Vulkyrie::Renderer {
    OpenGLRendererContext::OpenGLRendererContext(void *windowHandle)
        : _windowHandle(static_cast<GLFWwindow *>(windowHandle)) {
    }

    Vulkyrie::Core::StatusCode OpenGLRendererContext::Initialize() {
        using Vulkyrie::Core::Application;

        // Make the OpenGL context current.
        glfwMakeContextCurrent(_windowHandle);

        // GLAD: load all OpenGL function pointers
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            VFATAL("Failed to initialize GLAD");
            return Vulkyrie::Core::StatusCode::FailedToInitializeGLAD;
        }

#if defined(VULKYRIE_DEBUG)
        i32 contextFlags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

        if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            glDebugMessageCallback(
                [](GLenum source,
                   GLenum type,
                   GLuint id,
                   GLenum severity,
                   [[maybe_unused]] GLsizei length,
                   const GLchar *message,
                   [[maybe_unused]] const void *userParam) {
                    std::string_view sourceView;
                    switch (source) {
                        case GL_DEBUG_SOURCE_API:
                            sourceView = "API";
                            break;
                        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                            sourceView = "Window System";
                            break;
                        case GL_DEBUG_SOURCE_SHADER_COMPILER:
                            sourceView = "Shader Compiler";
                            break;
                        case GL_DEBUG_SOURCE_THIRD_PARTY:
                            sourceView = "Third Party";
                            break;
                        case GL_DEBUG_SOURCE_APPLICATION:
                            sourceView = "Application";
                            break;
                        case GL_DEBUG_SOURCE_OTHER:
                        default:
                            sourceView = "Other";
                            break;
                    }

                    std::string_view typeView;
                    switch (type) {
                        case GL_DEBUG_TYPE_ERROR:
                            typeView = "Error";
                            break;
                        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                            typeView = "Deprecated Behavior";
                            break;
                        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                            typeView = "Undefined Behavior";
                            break;
                        case GL_DEBUG_TYPE_PORTABILITY:
                            typeView = "Portability";
                            break;
                        case GL_DEBUG_TYPE_PERFORMANCE:
                            typeView = "Performance";
                            break;
                        case GL_DEBUG_TYPE_MARKER:
                            typeView = "Marker";
                            break;
                        case GL_DEBUG_TYPE_PUSH_GROUP:
                            typeView = "Push Group";
                            break;
                        case GL_DEBUG_TYPE_POP_GROUP:
                            typeView = "Pop Group";
                            break;
                        case GL_DEBUG_TYPE_OTHER:
                        default:
                            typeView = "Other";
                            break;
                    }

                    switch (severity) {
                        case GL_DEBUG_SEVERITY_HIGH:
                            VERROR("[OpenGLRenderer] Source: {}, Type: {}, ID: {}, Severity: High: {}", sourceView, typeView, id, message);
                            break;
                        case GL_DEBUG_SEVERITY_MEDIUM:
                            VWARN("[OpenGLRenderer] Source: {}, Type: {}, ID: {}, Severity: Medium: {}", sourceView, typeView, id, message);
                            break;
                        case GL_DEBUG_SEVERITY_LOW:
                            VDEBUG("[OpenGLRenderer] Source: {}, Type: {}, ID: {}, Severity: Low: {}", sourceView, typeView, id, message);
                            break;
                        case GL_DEBUG_SEVERITY_NOTIFICATION:
                        default:
                            VTRACE("[OpenGLRenderer] Source: {}, Type: {}, ID: {}, Severity: Notification: {}", sourceView, typeView, id, message);
                            break;
                    }
                },
                nullptr);

            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }

        VINFO("Renderer Info");
        VINFO("*****************************************************************************************");
        VINFO("API              | {}", Vulkyrie::Renderer::GetCurrentGraphicsAPIName());
        VINFO("Version          | {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
        VINFO("Vendor           | {}", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
        VINFO("Renderer         | {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
        VINFO("*****************************************************************************************");
#endif

        // TODO: This does not belong here. Move it to the renderer initialization.
        glViewport(0, 0, Application::GetSingleton().GetWindowWidth(), Application::GetSingleton().GetWindowHeight());

        return Vulkyrie::Core::StatusCode::Successful;
    }

    void OpenGLRendererContext::SwapBuffers() {
        glfwSwapBuffers(_windowHandle);
    }

    BufferHandle OpenGLRendererContext::CreateBuffer(std::span<f32> data) {
        return CreateBuffer(data.size() * sizeof(f32), data);
    }

    BufferHandle OpenGLRendererContext::CreateBuffer(size_t size, std::span<f32> data) {
        size_t index;

        if (!_freeIndices.empty()) {
            index = _freeIndices.back();
            _freeIndices.pop_back();
        } else {
            index = _bufferResources.size();
            _bufferResources.emplace_back();
        }

        OpenGLBufferResource &resource = _bufferResources[index];
        resource.Generation++;

        // If size is greater than 0, we can use the provided data. Otherwise, we need to create a dynamic storage buffer that can be mapped for writing.
        glCreateBuffers(1, &resource.BufferID);
        glNamedBufferStorage(resource.BufferID, size, data.data(), size > 0 ? GL_NONE : GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);

        return BufferHandle{ index, resource.Generation };
    }

    void OpenGLRendererContext::SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) {
        glNamedBufferSubData(_bufferResources[handle.Index].BufferID, startIndex * sizeof(f32), data.size() * sizeof(f32), data.data());
    }

    void OpenGLRendererContext::DestroyBuffer(const BufferHandle &handle) {
        glDeleteBuffers(1, &_bufferResources[handle.Index].BufferID);
    }

    OpenGLRendererContext::~OpenGLRendererContext() {
        for (const auto &resource : _bufferResources) {
            glDeleteBuffers(1, &resource.BufferID);
        }
    }

} // namespace Vulkyrie::Renderer
