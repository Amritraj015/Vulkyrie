#include "vlkypch.h"
#include "renderer/renderer_impl.h"
#include "renderer/backend_concepts.h"
#include "renderer/open_gl/open_gl_backend.h"
#include "renderer/frame_graph/frame_graph_concepts.h"
#include "renderer/frame_graph/resources/frame_graph_buffer.h"
#include "renderer/frame_graph/resources/frame_graph_texture.h"

namespace Vulkyrie {

    static_assert(RendererBackend<OpenGLBackend>, "OpenGLBackend does not satisfy RendererBackend concept.");

    // The frame graph is not wired into this backend yet, so nothing here instantiates FrameGraph<OpenGLBackend>.
    // These assert the part that can be checked without one: that the graph-owned resource types still line up with
    // what this backend offers. They are what catches a signature drifting apart from the concept, which would
    // otherwise stay invisible until the first pass is written.
    static_assert(FrameGraphResourceType<FrameGraphTexture<OpenGLBackend>, OpenGLBackend>,
                  "FrameGraphTexture does not satisfy the frame graph resource concept for OpenGLBackend.");
    static_assert(FrameGraphResourceType<FrameGraphBuffer<OpenGLBackend>, OpenGLBackend>,
                  "FrameGraphBuffer does not satisfy the frame graph resource concept for OpenGLBackend.");

    template class RendererImpl<OpenGLBackend>;

    Scope<Renderer> CreateOpenGLRenderer(const DeviceCreationInfo &info) {
        return CreateScope<RendererImpl<OpenGLBackend>>(info);
    }

} // namespace Vulkyrie
