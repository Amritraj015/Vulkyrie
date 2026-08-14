#pragma once

#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/deletion_queue.h"
#include "renderer/backends/shader_compiler.h"

namespace Vulkyrie {

    template <RendererBackend B> class ShaderModuleCache {
    public:
        ShaderModuleCache(typename B::Context &context, ShaderCompiler &compiler, DeletionQueue<B> &deletionQueue) noexcept
            : mContext(context)
            , mCompiler(compiler)
            , mDeletionQueue(deletionQueue) {
        }

        VE_DELETE_MOVE_AND_COPY(ShaderModuleCache);

        ~ShaderModuleCache() = default;

        [[nodiscard]] VE_INLINE B::ShaderModule Get() const;

        [[nodiscard]] VE_INLINE size_t Size() const;

        void Clear();

    private:
        B::Context &mContext;
        ShaderCompiler &mCompiler;
        DeletionQueue<B> &mDeletionQueue;
    };

} // namespace Vulkyrie
