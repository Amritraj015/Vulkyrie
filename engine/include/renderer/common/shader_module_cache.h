#pragma once

#include "renderer/backend_concepts.h"
#include "renderer/common/deletion_queue.h"
#include "renderer/shaders/shader_compiler.h"

namespace Vulkyrie {

    template <RendererBackend B> class ShaderModuleCache {
    public:
        ShaderModuleCache(typename B::Context &context, ShaderCompiler &compiler, DeletionQueue<B> &deletionQueue) noexcept
            : mContext(context)
            , mCompiler(compiler)
            , mDeletionQueue(deletionQueue) {
        }

        VE_DELETE_MOVE_AND_COPY(ShaderModuleCache);

        ~ShaderModuleCache() {
            Clear();
        }

        // [[nodiscard]] VE_INLINE B::ShaderModule Get() const {
        // }

        void Invalidate(u64 sourceHash) {
            for (auto it = mModules.begin(); it != mModules.end();) {
                if (it->first.SourceHash == sourceHash) {
                    mDeletionQueue->Push(it->second);
                    it = mModules.erase(it);
                } else {
                    ++it;
                }
            }
        }

        typename B::ShaderModule Insert(const ShaderKey &shaderKey, ShaderBlob &blob) {
            auto shaderModule = mContext.CreateShaderModule(blob);
            mModules.emplace(shaderKey, shaderModule);

            return shaderModule;
        }

        [[nodiscard]] VE_INLINE usize Size() const {
            return mModules.size();
        }

        VE_INLINE void Clear() {
            for (auto &[k, m] : mModules) {
                mDeletionQueue.Push(m);
            }

            mModules.clear();
        }

    private:
        B::Context &mContext;
        ShaderCompiler &mCompiler;
        DeletionQueue<B> &mDeletionQueue;
        std::unordered_map<ShaderKey, typename B::ShaderModule, ShaderKeyHasher> mModules;
    };

} // namespace Vulkyrie
