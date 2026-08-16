#pragma once

#include "vlkypch.h"
#include "renderer/open_gl/open_gl_types.h"
#include "renderer/rhi/capabilities.h"
#include "renderer/rhi/resource_types.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class OpenGLContext final {
    public:
        explicit OpenGLContext(const DeviceCreationInfo &info);

        VE_DELETE_MOVE_AND_COPY(OpenGLContext);

        ~OpenGLContext() = default;

        [[nodiscard]] OpenGLImage CreateImage(const TextureDescriptor &descriptor);
        [[nodiscard]] OpenGLBuffer CreateBuffer(const BufferDescriptor &descriptor);
        [[nodiscard]] OpenGLSampler CreateSampler(const SamplerDescriptor &descriptor);
        [[nodiscard]] OpenGLShaderModule CreateShaderModule(const ShaderBlob &blob);
        [[nodiscard]] OpenGLPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor);
        [[nodiscard]] OpenGLPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor);

        [[nodiscard]] bool DestroyImage(OpenGLImage image);
        [[nodiscard]] bool DestroyBuffer(OpenGLBuffer buffer);
        [[nodiscard]] bool DestroySampler(OpenGLSampler sampler);
        [[nodiscard]] bool DestroyShaderModule(OpenGLShaderModule shaderModule);
        [[nodiscard]] bool CreatePipeline(OpenGLPipeline pipeline);

        void WaitIdle() const;

        [[nodiscard]] VE_INLINE const DeviceCapabilities &QueryCapabilities() const {
            return mCapabilities;
        }

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept {
            return mContextCreated;
        }

        bool DeviceLost() const;

    private:
        DeviceCapabilities mCapabilities;
        bool mContextCreated;
    };

} // namespace Vulkyrie
