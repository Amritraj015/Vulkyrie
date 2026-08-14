#pragma once

#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/deletion_queue.h"
#include "renderer/backends/shader_module_cache.h"

namespace Vulkyrie {

    template <RendererBackend B> class PipelineCache {
    public:
        explicit PipelineCache(B::Context &context, ShaderModuleCache<B> &shaderCache, DeletionQueue<B> &deletionQueue) noexcept
            : mContext(context)
            , mShaderCache(shaderCache)
            , mDeletionQueue(deletionQueue) {
        }

        VE_DELETE_MOVE_AND_COPY(PipelineCache);

        ~PipelineCache() = default;

        [[nodiscard]] VE_INLINE B::Pipeline Get(const GraphicsPipelineDescriptor &descriptor) const;
        [[nodiscard]] VE_INLINE B::Pipeline Get(const ComputePipelineDescriptor &descriptor) const;

        VE_INLINE void PreCompile(std::span<const GraphicsPipelineDescriptor> descriptors);

        [[nodiscard]] VE_INLINE bool LoadFromDisk(const std::filesystem::path path);
        [[nodiscard]] VE_INLINE bool LoadFromDisk(const std::filesystem::path path) const;

        [[nodiscard]] VE_INLINE size_t Size() const noexcept;

    private:
        typename B::Context &mContext;
        ShaderModuleCache<B> &mShaderCache;
        DeletionQueue<B> &mDeletionQueue;
    };

} // namespace Vulkyrie
