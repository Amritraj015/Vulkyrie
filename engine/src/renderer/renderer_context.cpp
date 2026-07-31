#include "renderer/renderer_context.h"
#include "renderer/backends/open_gl/open_gl_renderer_backend.h"
#include "renderer/backends/vulkan/vulkan_renderer_backend.h"

namespace Vulkyrie {

    GraphicsAPI RendererContext::_currentGraphicsApi = GraphicsAPI::None;
    bool RendererContext::_initialized = false;
    RendererBackend *RendererContext::_rendererBackend = nullptr;

    StatusCode RendererContext::Create(GraphicsAPI graphicsApi) {
        if (_initialized) {
            VWARN("Renderer already initialized, call SwitchTo(GraphicsAPI otherGraphicsApi) if you are trying to switch renderer backend.");

            return StatusCode::RendererAlreadyInitialized;
        }

        Dispose();

        _currentGraphicsApi = graphicsApi;

        switch (_currentGraphicsApi) {
            case GraphicsAPI::OpenGL:
                _rendererBackend = new OpenGLRendererBackend(graphicsApi);

                RETURN_ON_FAILURE(_rendererBackend->Initialize());

                _initialized = true;
                break;
            case GraphicsAPI::Vulkan:
                _rendererBackend = new VulkanRendererBackend(graphicsApi);

                RETURN_ON_FAILURE(_rendererBackend->Initialize());

                _initialized = true;

                break;
            default:
                VERROR("Renderer cannot be initialized, invalid Graphics API: {}", GetCurrentGraphicsAPIName());

                _initialized = false;

                return StatusCode::UnsupportedGraphicsAPI;
                break;
        }

        return StatusCode::Successful;
    }

    StatusCode RendererContext::SwitchTo(GraphicsAPI otherGraphicsApi) {
        // We cannot switch to GraphicsAPI::None.
        // So, return an error status code if the user if attempting to do this.
        if (GraphicsAPI::None == otherGraphicsApi) {
            return StatusCode::UnsupportedGraphicsAPI;
        }

        // If the renderer backend:
        // 1. has been initialized and the current backend API is not the same as the one we are trying to switch to or,
        // 2. if the backend has not been initialized,
        // then initialize the appropriate RendererBackend.
        if ((_initialized && _currentGraphicsApi != otherGraphicsApi) || nullptr == _rendererBackend) {
            return Create(otherGraphicsApi);
        }

        // Else we don't need to do anything, just return a successful status code.
        return StatusCode::Successful;
    }

    void RendererContext::Dispose() {
        if (nullptr != _rendererBackend) {
            _initialized = false;

            delete _rendererBackend;
        }
    }
} // namespace Vulkyrie
