#include "renderer/frame_graph/resources/render_graph_transient_resources.h"
#include <functional>

namespace Vulkyrie::Renderer {

    void RenderGraphTransientResources::CreateTexture(const TextureSpecification &specification) {
        const auto textureHash = std::hash<TextureSpecification>{}(specification);
        auto texture = Texture2D::Create(specification);
    }

    void RenderGraphTransientResources::DestroyTexture(const TextureSpecification &specification) {
    }

    void RenderGraphTransientResources::CreateBuffer() {
    }

    void RenderGraphTransientResources::DestroyBuffer() {
    }

} // namespace Vulkyrie::Renderer
