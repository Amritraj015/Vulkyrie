#pragma once

#include "renderer/backends/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class DeletionQueue {
    public:
        explicit DeletionQueue(typename B::Context &context) noexcept
            : mContext(context)
            , mCurrent(0) {
        }

        VE_DELETE_MOVE_AND_COPY(DeletionQueue);

        ~DeletionQueue() {
            Flush();
        }

        VE_INLINE void Push(B::Image);
        VE_INLINE void Push(B::Buffer);
        VE_INLINE void Push(B::Sampler);
        VE_INLINE void Push(B::Pipeline);
        VE_INLINE void Push(B::ShaderModule);

        VE_INLINE void Flush() {};
        VE_INLINE void Collect(size_t currentFrameIndex);

        [[nodiscard]] VE_INLINE size_t PendingCount() const noexcept;

    private:
        struct Bucket {};

        typename B::Context &mContext;
        Bucket mBuckets[B::kFramesInFlight];
        u32 mCurrent;
    };

} // namespace Vulkyrie
