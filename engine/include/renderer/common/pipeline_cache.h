#pragma once

#include "renderer/backend_concepts.h"
#include "renderer/common/deletion_queue.h"
#include "renderer/common/shader_module_cache.h"

namespace Vulkyrie {

    template <RendererBackend B> class PipelineCache {
    public:
        explicit PipelineCache(B::Context &context, ShaderModuleCache<B> &shaderCache, DeletionQueue<B> &deletionQueue) noexcept
            : mContext(context)
            , mShaderCache(shaderCache)
            , mDeletionQueue(deletionQueue) {
        }

        VE_DELETE_MOVE_AND_COPY(PipelineCache);

        ~PipelineCache() {
            for (auto &[hash, pipeline] : mPipelines) {
                mDeletionQueue.Push(pipeline);
            }
        }

        [[nodiscard]] VE_INLINE B::Pipeline Get(const GraphicsPipelineDescriptor &descriptor) {
            const u64 hash = HashDescriptor(descriptor);

            if (auto it = mPipelines.find(hash); it != mPipelines.end()) {
                return it->second;
            }

            auto pipeline = mContext.CreateGraphicsPipeline(descriptor);
            mPipelines.emplace(hash, pipeline);

            return pipeline;
        }

        [[nodiscard]] VE_INLINE B::Pipeline Get(const ComputePipelineDescriptor &descriptor) {
            const u64 hash = HashDescriptor(descriptor);

            if (auto it = mPipelines.find(hash); it != mPipelines.end()) {
                return it->second;
            }

            auto pipeline = mContext.CreateComputePipeline(descriptor);
            mPipelines.emplace(hash, pipeline);

            return pipeline;
        }

        VE_INLINE void PreCompile(std::span<const GraphicsPipelineDescriptor> descriptors);

        [[nodiscard]] VE_INLINE bool LoadFromDisk(const std::filesystem::path path);
        [[nodiscard]] VE_INLINE bool SaveToDisk(const std::filesystem::path path) const;

        [[nodiscard]] VE_INLINE size_t Size() const noexcept;

    private:
        typename B::Context &mContext;
        ShaderModuleCache<B> &mShaderCache;
        DeletionQueue<B> &mDeletionQueue;
        std::unordered_map<u64, typename B::Pipeline> mPipelines;
        std::unordered_map<u64, std::vector<u64>> mShaderToPipelines;
    };

} // namespace Vulkyrie
