#pragma once

#include "renderer/backends/open_gl/open_gl_types.h"
#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    class OpenGLContext {
    public:
        OpenGLImage CreateImage(const ImageDescriptor &descriptor);
        OpenGLBuffer CreateBuffer(const BufferDescriptor &descriptor);
        OpenGLSampler CreateSampler(const SamplerDescriptor &descriptor);
        OpenGLShaderModule CreateShaderModule(const ShaderBlob &blob);
        OpenGLPipeline CreateGraphicsPipeline(const GraphicsPipelineDescriptor &descriptor);
        OpenGLPipeline CreateComputePipeline(const ComputePipelineDescriptor &descriptor);

        bool DestroyImage(OpenGLImage image);
        bool DestroyBuffer(OpenGLBuffer buffer);
        bool DestroySampler(OpenGLSampler sampler);
        bool DestroyShaderModule(OpenGLShaderModule shaderModule);
        bool CreatePipeline(OpenGLPipeline pipeline);

        void WaitIdle() const;
        const DeviceCapabilities &QueryCapabilities() const;
        bool DeviceLost() const;

    private:
    };

} // namespace Vulkyrie
