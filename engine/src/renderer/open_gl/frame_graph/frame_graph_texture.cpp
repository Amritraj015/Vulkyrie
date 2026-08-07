#include "renderer/frame_graph/resources/frame_graph_texture.h"
#include "renderer/frame_graph/resources/frame_graph_transient_resources.h"

namespace Vulkyrie {

    void FrameGraphTexture::Create(const FrameGraphTexture::Descriptor &descriptor, const FrameGraphContext &context) {
        const auto transientResources = static_cast<FrameGraphTransientResources *>(context.TransientResources);
        transientResources->CreateTexture(descriptor);
    }

    void FrameGraphTexture::Destroy(const FrameGraphTexture::Descriptor &descriptor, const FrameGraphContext &context) {
        const auto transientResources = static_cast<FrameGraphTransientResources *>(context.TransientResources);
        transientResources->DestroyTexture(descriptor);
    }

} // namespace Vulkyrie
