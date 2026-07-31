#include "renderer/backends/open_gl/open_gl_renderer_backend.h"
#include "renderer/open_gl/open_gl_frame_buffer.h"
#include "renderer/open_gl/open_gl_index_buffer.h"
#include "renderer/open_gl/open_gl_shader.h"
#include "renderer/open_gl/open_gl_vertex_array.h"
#include "renderer/open_gl/open_gl_vertex_buffer.h"
#include "renderer/renderer_context.h"
#include "core/application.h"

namespace Vulkyrie {

    OpenGLRendererBackend::OpenGLRendererBackend(GraphicsAPI graphicsApi)
        : RendererBackend(graphicsApi)
        , _windowHandle(static_cast<GLFWwindow *>(Application::GetSingleton().GetWindowHandle())) {
    }

    StatusCode OpenGLRendererBackend::Initialize() {
        // Make the OpenGL context current.
        glfwMakeContextCurrent(_windowHandle);

        // GLAD: load all OpenGL function pointers
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            VFATAL("Failed to initialize GLAD");
            return StatusCode::FailedToInitializeGLAD;
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
        VINFO("API              | {}", RendererContext::GetCurrentGraphicsAPIName());
        VINFO("Version          | {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
        VINFO("Vendor           | {}", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
        VINFO("Renderer         | {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
        VINFO("*****************************************************************************************");
#endif

        glViewport(0, 0, Application::GetSingleton().GetWindowWidth(), Application::GetSingleton().GetWindowHeight());

        _indexBuffer = new OpenGLIndexBuffer();
        // _frameBuffer = new OpenGLFrameBuffer();
        // _shader = new OpenGLShader();
        // _texture = new OpenGLTexture();
        // _cubeMapTexture = new OpenGLCubeMapTexture();
        // _vertexArray = new OpenGLVertexArray();
        // _vertexBuffer = new OpenGLVertexBuffer();

        return StatusCode::Successful;
    }

    OpenGLRendererBackend::~OpenGLRendererBackend() {
        for (const auto &resource : _bufferResources) {
            glDeleteBuffers(1, &resource.BufferID);
        }

        if (nullptr != _indexBuffer) {
            delete _indexBuffer;
        }

        if (nullptr != _frameBuffer) {
            delete _frameBuffer;
        }

        if (nullptr != _shader) {
            delete _shader;
        }

        if (nullptr != _texture) {
            delete _texture;
        }

        if (nullptr != _cubeMapTexture) {
            delete _cubeMapTexture;
        }

        if (nullptr != _vertexArray) {
            delete _vertexArray;
        }

        if (nullptr != _vertexBuffer) {
            delete _vertexBuffer;
        }
    }

    void OpenGLRendererBackend::SwapBuffers() {
        glfwSwapBuffers(_windowHandle);
    }

    BufferHandle OpenGLRendererBackend::CreateBuffer(std::span<f32> data) {
        return CreateBuffer(data.size() * sizeof(f32), data);
    }

    BufferHandle OpenGLRendererBackend::CreateBuffer(size_t size, std::span<f32> data) {
        u32 index;

        if (!_freeIndices.empty()) {
            index = static_cast<u32>(_freeIndices.back());
            _freeIndices.pop_back();
        } else {
            index = static_cast<u32>(_bufferResources.size());
            _bufferResources.emplace_back();
        }

        OpenGLBufferResource &resource = _bufferResources[index];
        resource.Generation++;

        // If size is greater than 0, we can use the provided data. Otherwise, we need to create a dynamic storage buffer that can be mapped for writing.
        glCreateBuffers(1, &resource.BufferID);
        glNamedBufferStorage(resource.BufferID, size, data.data(), size > 0 ? GL_NONE : GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);

        return BufferHandle(index, resource.Generation);
    }

    void OpenGLRendererBackend::SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) {
        glNamedBufferSubData(_bufferResources[handle.Index()].BufferID, startIndex * sizeof(f32), data.size() * sizeof(f32), data.data());
    }

    void OpenGLRendererBackend::DestroyBuffer(const BufferHandle &handle) {
        glDeleteBuffers(1, &_bufferResources[handle.Index()].BufferID);
    }

} // namespace Vulkyrie
