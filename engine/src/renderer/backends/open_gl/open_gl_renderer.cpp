#include "vlkypch.h"
#include "renderer/renderer_impl.h"
#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/open_gl/open_gl_backend.h"

namespace Vulkyrie {

    static_assert(RendererBackend<OpenGLBackend>, "OpenGLBackend does not satisfy RendererBackend concept.");

    template class RendererImpl<OpenGLBackend>;

    Scope<Renderer> CreateOpenGLRenderer(const DeviceCreationInfo &info) {
        return CreateScope<RendererImpl<OpenGLBackend>>(info);
    }

} // namespace Vulkyrie
