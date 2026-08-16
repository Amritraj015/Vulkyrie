#pragma once

#include "renderer/backends/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class UploadRing final {
    public:
        UploadRing(typename B::Context &ctx, usize bytesPerFrame)
            : pContext(ctx)
            , mBytesPerFrame(bytesPerFrame) {
        }

        VE_DELETE_MOVE_AND_COPY(UploadRing);

        ~UploadRing();

    private:
        typename B::Context *pContext = nullptr;
        typename B::Buffer mBuffer{};
        u8 *pMapped = nullptr;
        usize mBytesPerFrame = 0;
        usize mFrameBase = 0;
        usize mHighWater = 0;
        std::atomic<usize> mCursor = 0;
    };

} // namespace Vulkyrie
