#pragma once

#include "vlkypch.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/deletion_queue.h"
#include "renderer/common/pipeline_cache.h"
#include "renderer/common/shader_module_cache.h"
#include "renderer/common/transient_pool.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/shaders/shader_compiler.h"

namespace Vulkyrie {

    template <RendererBackend B> class Device {
    public:
        explicit Device(const DeviceCreationInfo &info)
            : mShaderCompiler()
            , mContext(CreateScope<typename B::Context>(info))
            , mDeletionQueue(CreateScope<DeletionQueue<B>>(*mContext, info))
            , mTransients(CreateScope<TransientPool<B>>(*mContext, *mDeletionQueue))
            , mShaders(CreateScope<ShaderModuleCache<B>>(*mContext, mShaderCompiler, *mDeletionQueue))
            , mPipelines(CreateScope<PipelineCache<B>>(*mContext, *mShaders, *mDeletionQueue)) {
        }

        VE_DELETE_MOVE_AND_COPY(Device);

        ~Device() {
            if (nullptr != mContext && mContext->ContextCreated()) {
                mContext->WaitIdle();
            }
        }

        [[nodiscard]] VE_INLINE typename B::Context &Context() noexcept {
            return *mContext;
        };

        [[nodiscard]] VE_INLINE const typename B::Context &Context() const noexcept {
            return *mContext;
        };

        [[nodiscard]] VE_INLINE typename B::Queue &GetGraphicsQueue() noexcept {
            return mContext->GetGraphicsQueue();
        }

        [[nodiscard]] VE_INLINE typename B::Queue &GetTransferQueue() noexcept {
            return mContext->GetTransferQueue();
        }

        [[nodiscard]] VE_INLINE typename B::Queue &GetComputeQueue() noexcept {
            return mContext->GetComputeQueue();
        }

        [[nodiscard]] VE_INLINE auto &GetHeap() noexcept
            requires(B::kUsesBindlessHeap)
        {
            return mContext->GetHeap();
        }

        [[nodiscard]] VE_INLINE ShaderModuleCache<B> &GetShaders() noexcept {
            return *mShaders;
        }

        [[nodiscard]] VE_INLINE PipelineCache<B> &GetPipelines() noexcept {
            return *mPipelines;
        }

        [[nodiscard]] VE_INLINE TransientPool<B> &GetTransients() noexcept {
            return *mTransients;
        }

        [[nodiscard]] VE_INLINE DeletionQueue<B> &GetDeletionQueue() noexcept {
            return *mDeletionQueue;
        }

        [[nodiscard]] VE_INLINE const DeviceCapabilities &QueryCapabilities() const noexcept {
            return mContext->QueryCapabilities();
        }

        VE_INLINE void WaitIdle() const {
            mContext->WaitIdle();
        }

        [[nodiscard]] VE_INLINE bool DeviceLost() const noexcept {
            return mContext->DeviceLost();
        }

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContext->ContextCreated();
        }

    private:
        ShaderCompiler mShaderCompiler;
        Scope<typename B::Context> mContext;
        Scope<DeletionQueue<B>> mDeletionQueue;
        Scope<TransientPool<B>> mTransients;
        Scope<ShaderModuleCache<B>> mShaders;
        Scope<PipelineCache<B>> mPipelines;
    };

} // namespace Vulkyrie
