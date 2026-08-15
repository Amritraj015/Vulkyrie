#include "renderer/backends/open_gl/open_gl_context.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Vulkyrie {

    OpenGLContext::OpenGLContext(const DeviceCreationInfo &info)
        : mCapabilities()
        , mContextCreated(false) {

        auto *window = static_cast<GLFWwindow *>(info.WindowHandle.NativeWindow);

        // Make the OpenGL context current.
        glfwMakeContextCurrent(window);

        // Check if the OpenGL context is current.
        if (glfwGetCurrentContext() != window) {
            VFATAL("Could not make OpenGL context current.");

            mContextCreated = false;

            return;
        }

        // GLAD: load all OpenGL function pointers
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            VFATAL("Failed to initialize GLAD");

            glfwMakeContextCurrent(nullptr);

            mContextCreated = false;

            return;
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
        VINFO("API              | OpenGL");
        VINFO("Version          | {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
        VINFO("Vendor           | {}", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
        VINFO("Renderer         | {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
        VINFO("*****************************************************************************************");
#endif

        glViewport(0, 0, info.SurfaceWidth, info.SurfaceHeight);

        mContextCreated = true;
    }

    OpenGLImage CreateImage(const ImageDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    OpenGLBuffer CreateBuffer(const BufferDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    OpenGLSampler CreateSampler(const SamplerDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    OpenGLShaderModule CreateShaderModule(const ShaderBlob &blob) {
        (void)blob;
        return {};
    }

    OpenGLPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    OpenGLPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor) {
        (void)descriptor;
        return {};
    }

    bool DestroyImage(OpenGLImage image) {
        (void)image;
        return true;
    }

    bool DestroyBuffer(OpenGLBuffer buffer) {
        (void)buffer;
        return true;
    }

    bool DestroySampler(OpenGLSampler sampler) {
        (void)sampler;
        return true;
    }

    bool DestroyShaderModule(OpenGLShaderModule shaderModule) {
        (void)shaderModule;
        return true;
    }

    bool CreatePipeline(OpenGLPipeline pipeline) {
        (void)pipeline;
        return true;
    }

    void OpenGLContext::WaitIdle() const {
        glFinish();
    }

    bool OpenGLContext::DeviceLost() const {
        return false;
    }

} // namespace Vulkyrie
