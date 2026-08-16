#pragma once

#include "renderer/backends/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class DeletionQueue {
    public:
        explicit DeletionQueue(typename B::Context &context, const DeviceCreationInfo &info) noexcept
            : mContext(context)
            , mCurrent(0) {

            const usize minSize = 100;
            const usize maxSize = 1000;

            for (auto &b : mBuckets) {
                b.Images.reserve(std::clamp(usize(info.MaxTextures / 2), minSize, maxSize));
                b.Buffers.reserve(std::clamp(usize(info.MaxBuffers / 2), minSize, maxSize));
                b.Samplers.reserve(std::clamp(usize(info.MaxSamplers / 2), minSize, maxSize));
                b.Pipelines.reserve(std::clamp(usize(info.MaxPipelines / 2), minSize, maxSize));
                b.ShaderModules.reserve(std::clamp(usize(info.MaxPipelines / 2), minSize, maxSize));
            }
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

        VE_INLINE void Flush() {
            for (auto &b : mBuckets) {
                b.Clear();
            }
        }

        // VE_INLINE void Collect(usize currentFrameIndex) {
        //     // TODO: destroy bucket (completedFrameIndex % kFramesInFlight) via mCtx.
        //     mCurrent = static_cast<u32>((currentFrameIndex + 1) % B::kFramesInFlight);
        // }

        [[nodiscard]] VE_INLINE usize PendingCount() const noexcept {
            usize count;

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

            VE_INLINE void Clear() {
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
