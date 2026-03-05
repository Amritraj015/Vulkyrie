#include "renderer/frame_graph/resources/frame_graph_texture.h"
#include "renderer/frame_graph/resources/render_graph_transient_resources.h"

namespace Vulkyrie::Renderer {

    void FrameGraphTexture::Create(const FrameGraphTexture::Descriptor &descriptor, void *context) {
        const auto transientResources = static_cast<RenderGraphTransientResources *>(context);
        transientResources->CreateTexture(descriptor);
    }

    void FrameGraphTexture::Destroy(const FrameGraphTexture::Descriptor &descriptor, void *context) {
        const auto transientResources = static_cast<RenderGraphTransientResources *>(context);
        transientResources->DestroyTexture(descriptor);
    }

} // namespace Vulkyrie::Renderer
