#pragma once

#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/texture_specification.h"

namespace Vulkyrie {

    class FrameGraphTexture {
    public:
        struct Descriptor : public TextureSpecification {};

        /** @brief Materializes the texture from the frame's transient-resource pool.
         * @param context `TransientResources` must point at a `FrameGraphTransientResources`. */
        void Create(const FrameGraphTexture::Descriptor &descriptor, const FrameGraphContext &context);

        /** @brief Returns the texture to the frame's transient-resource pool.
         * @param context `TransientResources` must point at a `FrameGraphTransientResources`. */
        void Destroy(const FrameGraphTexture::Descriptor &descriptor, const FrameGraphContext &context);
    };

} // namespace Vulkyrie
