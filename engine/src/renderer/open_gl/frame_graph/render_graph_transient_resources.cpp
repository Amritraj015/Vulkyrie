#include "renderer/frame_graph/resources/render_graph_transient_resources.h"
#include <functional>

namespace Vulkyrie {

    void RenderGraphTransientResources::CreateTexture(const TextureSpecification &specification) {
        // const auto textureHash = std::hash<TextureSpecification>{}(specification);
        auto texture = Texture2D::Create(specification);
    }

    void RenderGraphTransientResources::DestroyTexture([[maybe_unused]] const TextureSpecification &specification) {
    }

    // void RenderGraphTransientResources::CreateBuffer([[maybe_unused]] size_t size) {
    // }
    //
    // void RenderGraphTransientResources::DestroyBuffer() {
    // }

} // namespace Vulkyrie
