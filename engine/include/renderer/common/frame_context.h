#pragma once

#include "vlkypch.h"
#include "renderer/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class FrameContext {
    public:
        FrameContext(typename B::Context &ctx, u32 slot, u32 workerCount, u64 uploadBytes);

        VE_DELETE_MOVE_AND_COPY(FrameContext);

        ~FrameContext();

        void BeginFrame(u32 frameIndex);
        void EndFrame();

        // [[nodiscard]] typename B::CommandList &AcquireCommandList(u32 workerIndex, QueueType queueType);

    private:
    };

} // namespace Vulkyrie
