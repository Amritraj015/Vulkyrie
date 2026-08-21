#pragma once

#include "vlkypch.h"
#include "renderer/rhi/barrier_types.h"

namespace Vulkyrie {

    class OpenGLCommandList {
    public:
        void Begin();
        void End();

        /** @brief No-op: GL has no barrier objects carrying layouts, which is what `kHasExplicitBarriers == false`
         * records. The frame graph still computes the transitions; this backend simply has nothing to emit them to.
         * @param barriers The batch, ignored. */
        VE_INLINE void EmitBarriers(std::span<const ResourceBarrier> barriers) {
            (void)barriers;
        }
    };

} // namespace Vulkyrie
