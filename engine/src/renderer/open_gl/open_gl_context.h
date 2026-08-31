#pragma once

#include "vlkypch.h"
#include "renderer/open_gl/open_gl_types.h"
#include "renderer/rhi/barrier_types.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class OpenGLContext final {
    public:
        explicit OpenGLContext(const DeviceCreationInfo &info);

        VE_DELETE_MOVE_AND_COPY(OpenGLContext);

        ~OpenGLContext() = default;

        StatusCode Initialize();

        [[nodiscard]] OpenGLImage CreateImage(const TextureDescriptor &descriptor);
        [[nodiscard]] OpenGLBuffer CreateBuffer(const BufferDescriptor &descriptor);
        [[nodiscard]] OpenGLSampler CreateSampler(const SamplerDescriptor &descriptor);
        [[nodiscard]] OpenGLShaderModule CreateShaderModule(const ShaderBlob &blob);
        [[nodiscard]] OpenGLPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor);
        [[nodiscard]] OpenGLPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor);

        void DestroyImage(OpenGLImage image);
        void DestroyBuffer(OpenGLBuffer buffer);
        void DestroySampler(OpenGLSampler sampler);
        void DestroyShaderModule(OpenGLShaderModule shaderModule);
        void DestroyPipeline(OpenGLPipeline pipeline);

        /** @brief Estimates an image's storage. GL exposes no allocator query and cannot alias two textures onto
         * one allocation, so this only ever feeds the frame graph's aliasing report.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] ResourceMemoryRequirements GetImageMemoryRequirements(const TextureDescriptor &descriptor) const;

        /** @brief Reports a buffer's storage. Exact, unlike the image estimate.
         * @param descriptor The descriptor to size. */
        [[nodiscard]] ResourceMemoryRequirements GetBufferMemoryRequirements(const BufferDescriptor &descriptor) const;

        void WaitIdle() const;

        [[nodiscard]] VE_INLINE const DeviceCapabilities &QueryCapabilities() const {
            return mCapabilities;
        }

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContextCreated;
        }

        bool DeviceLost() const;

        // TODO: remove this.
        void test() {};

    private:
        DeviceCreationInfo mDeviceCreationInfo;
        DeviceCapabilities mCapabilities;
        bool mContextCreated;
    };

} // namespace Vulkyrie
