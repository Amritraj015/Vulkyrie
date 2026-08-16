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

        VE_INLINE void Push(B::Image image) {
            mBuckets[mCurrent].Images.push_back(image);
        }

        VE_INLINE void Push(B::Buffer buffer) {
            mBuckets[mCurrent].Buffers.push_back(buffer);
        }

        VE_INLINE void Push(B::Sampler sampler) {
            mBuckets[mCurrent].Samplers.push_back(sampler);
        }

        VE_INLINE void Push(B::Pipeline pipeline) {
            mBuckets[mCurrent].Pipelines.push_back(pipeline);
        }

        VE_INLINE void Push(B::ShaderModule shaderModule) {
            mBuckets[mCurrent].ShaderModules.push_back(shaderModule);
        }

        VE_INLINE void Flush() {};
        VE_INLINE void Collect(usize currentFrameIndex);

        [[nodiscard]] VE_INLINE usize PendingCount() const noexcept {
            usize count;

            for (const auto &bucket : mBuckets) {
                count += bucket.Images.size() + bucket.Buffers.size() + bucket.Samplers.size() + bucket.Pipelines.size() + bucket.ShaderModules.size();
            }

            return count;
        }

    private:
        struct Bucket final {
            std::vector<typename B::Image> Images;
            std::vector<typename B::Buffer> Buffers;
            std::vector<typename B::Sampler> Samplers;
            std::vector<typename B::Pipeline> Pipelines;
            std::vector<typename B::ShaderModule> ShaderModules;

            VE_INLINE void Clear() const {
                Images.clear();
                Buffers.clear();
                Samplers.clear();
                Pipelines.clear();
                ShaderModules.clear();
            }
        };

        typename B::Context &mContext;
        Bucket mBuckets[B::kFramesInFlight];
        u32 mCurrent;
    };

} // namespace Vulkyrie
