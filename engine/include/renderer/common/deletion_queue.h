#pragma once

#include "renderer/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class DeletionQueue {
    public:
        explicit DeletionQueue(typename B::Context &context, const DeviceCreationInfo &info) noexcept
            : mContext(context)
            , mCurrentSlot(0)
            , mWorkerCount(info.WorkerCount) {

            const usize minSize = 100;
            const usize maxSize = 1000;

            for (auto &b : mBuckets) {
                b.Images.reserve(std::clamp(usize(info.MaxTextures), minSize, maxSize));
                b.Buffers.reserve(std::clamp(usize(info.MaxBuffers), minSize, maxSize));
                b.Samplers.reserve(std::clamp(usize(info.MaxSamplers), minSize, maxSize));
                b.Pipelines.reserve(std::clamp(usize(info.MaxPipelines), minSize, maxSize));
                b.ShaderModules.reserve(std::clamp(usize(info.MaxPipelines), minSize, maxSize));
            }
        }

        VE_DELETE_MOVE_AND_COPY(DeletionQueue);

        ~DeletionQueue() {
            Flush();
        }

        VE_INLINE void Push(B::Image image, u32 workerIndex = 0) {
            at(mCurrentSlot, workerIndex).Images.push_back(image);
        }

        VE_INLINE void Push(B::Buffer buffer, u32 workerIndex = 0) {
            at(mCurrentSlot, workerIndex).Buffers.push_back(buffer);
        }

        VE_INLINE void Push(B::Sampler sampler, u32 workerIndex = 0) {
            at(mCurrentSlot, workerIndex).Samplers.push_back(sampler);
        }

        VE_INLINE void Push(B::Pipeline pipeline, u32 workerIndex = 0) {
            at(mCurrentSlot, workerIndex).Pipelines.push_back(pipeline);
        }

        VE_INLINE void Push(B::ShaderModule shaderModule, u32 workerIndex = 0) {
            at(mCurrentSlot, workerIndex).ShaderModules.push_back(shaderModule);
        }

        VE_INLINE void Flush() {
            if (!mContext.ContextCreated()) {
                return;
            }

            for (auto &b : mBuckets) {
                b.DestroyAll(mContext);
            }
        }

        VE_INLINE void Collect(u64 completedFrameIndex) {
            const u64 slot = completedFrameIndex % B::kFramesInFlight;

            for (u32 w = 0; w < mWorkerCount; ++w) {
                at(slot, w).DestroyAll(mContext);
            }

            mCurrentSlot = (completedFrameIndex + 1) % B::kFramesInFlight;
        }

        [[nodiscard]] VE_INLINE usize PendingCount() const noexcept {
            usize count = 0;

            for (const auto &b : mBuckets) {
                count += b.Images.size() + b.Buffers.size() + b.Samplers.size() + b.Pipelines.size() + b.ShaderModules.size();
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

            VE_INLINE void DestroyAll(typename B::Context &context) {
                for (auto &r : Images) context.DestroyImage(r);
                for (auto &r : Buffers) context.DestroyBuffer(r);
                for (auto &r : Samplers) context.DestroySampler(r);
                for (auto &r : Pipelines) context.DestroyPipeline(r);
                for (auto &r : ShaderModules) context.DestroyShaderModule(r);

                Images.clear();
                Buffers.clear();
                Samplers.clear();
                Pipelines.clear();
                ShaderModules.clear();
            }
        };

        typename B::Context &mContext;
        Bucket mBuckets[B::kFramesInFlight];
        u64 mCurrentSlot;
        u32 mWorkerCount;

        [[nodiscard]] VE_INLINE Bucket &at(u64 slot, u32 worker) noexcept {
            return mBuckets[slot * mWorkerCount + worker];
        }
    };

} // namespace Vulkyrie
