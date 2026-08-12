#pragma once

#include "vlkypch.h"
#include "renderer/backends/backend_concepts.h"
#include "renderer/backends/deletion_queue.h"
#include "renderer/backends/pipeline_cache.h"
#include "renderer/backends/shader_compiler.h"
#include "renderer/backends/shader_module_cache.h"
#include "renderer/backends/transient_pool.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    template <RendererBackend B> class Device {
    public:
        explicit Device(const DeviceCreationInfo &info);

        VE_DELETE_MOVE_AND_COPY(Device);

        ~Device() {
            if (nullptr != mContext) {
                mContext->WaitIdle();
            }
        }

        [[nodiscard]] typename B::Context &Context() noexcept {
            return *mContext;
        };

        [[nodiscard]] const typename B::Context &Context() const noexcept {
            return *mContext;
        };

        [[nodiscard]] typename B::Queue &GetGraphicsQueue() noexcept {
            return mContext->GetGraphicsQueue();
        }

        [[nodiscard]] typename B::Queue &GetTransferQueue() noexcept {
            return mContext->GetTransferQueue();
        }

        [[nodiscard]] typename B::Queue &GetComputeQueue() noexcept {
            return mContext->GetComputeQueue();
        }

        [[nodiscard]] auto &GetHeap() noexcept
            requires(B::kUsesBindlessHeap)
        {
            return mContext->GetHeap();
        }

        [[nodiscard]] ShaderModuleCache<B> &GetShaders() noexcept {
            return mShaders;
        }

        [[nodiscard]] PipelineCache<B> &GetPipelines() noexcept {
            return mPipelines;
        }

        [[nodiscard]] TransientPool<B> &GetTransients() noexcept {
            return mPipelines;
        }

        [[nodiscard]] DeletionQueue<B> &GetDeletionQueue() noexcept {
            return mDeletionQueue;
        }

        [[nodiscard]] const DeviceCapabilities &QueryCapabilities() const noexcept {
            return mContext->QueryCapabilities();
        }

        void WaitIdle() const {
            mContext->WaitIdle();
        }

        [[nodiscard]] bool DeviceLost() const noexcept {
            return mContext->DeviceLost();
        }

    private:
        Scope<typename B::Context> mContext;
        Scope<DeletionQueue<B>> mDeletionQueue;
        Scope<TransientPool<B>> mTransients;
        Scope<ShaderModuleCache<B>> mShaders;
        Scope<PipelineCache<B>> mPipelines;
        ShaderCompiler mShaderCompiler;
    };

} // namespace Vulkyrie
