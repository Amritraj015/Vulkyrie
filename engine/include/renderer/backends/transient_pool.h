#pragma once

#include "vlkypch.h"
#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/deletion_queue.h"

namespace Vulkyrie {

    template <RendererBackend B> class TransientPool {
    public:
        explicit TransientPool(B::Context &context, DeletionQueue<B> &deletionQueue) noexcept
            : mContext(context)
            , mDeletionQueue(deletionQueue) {
        }

        VE_DELETE_MOVE_AND_COPY(TransientPool);

        ~TransientPool();

    private:
        B::Context &mContext;
        DeletionQueue<B> &mDeletionQueue;
    };

} // namespace Vulkyrie
