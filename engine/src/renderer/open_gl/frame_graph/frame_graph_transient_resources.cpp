#include "renderer/frame_graph/resources/frame_graph_transient_resources.h"

namespace Vulkyrie {

    void FrameGraphTransientResources::CreateTexture(const TextureSpecification &specification) {
        // const auto textureHash = std::hash<TextureSpecification>{}(specification);
        auto texture = Texture2D::Create(specification);
    }

    void FrameGraphTransientResources::DestroyTexture([[maybe_unused]] const TextureSpecification &specification) {
    }

    // void FrameGraphTransientResources::CreateBuffer([[maybe_unused]] size_t size) {
    // }
    //
    // void FrameGraphTransientResources::DestroyBuffer() {
    // }

} // namespace Vulkyrie
