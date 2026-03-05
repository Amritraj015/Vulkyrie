#include "renderer/frame_graph/resources/frame_graph_buffer.h"
#include "renderer/frame_graph/resources/render_graph_transient_resources.h"

namespace Vulkyrie::Renderer {

    void FrameGraphBuffer::Create(const FrameGraphBuffer::Descriptor &descriptor, void *context) {
        const auto transientResources = static_cast<RenderGraphTransientResources *>(context);
        transientResources->CreateBuffer(descriptor);
    }

    void FrameGraphBuffer::Destroy(const FrameGraphBuffer::Descriptor &descriptor, void *context) {
        const auto transientResources = static_cast<RenderGraphTransientResources *>(context);
        transientResources->CreateBuffer(descriptor);
    }

} // namespace Vulkyrie::Renderer
