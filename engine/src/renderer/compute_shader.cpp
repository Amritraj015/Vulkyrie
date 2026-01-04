#include "core/logger.h"
#include "renderer/compute_shader.h"
#include "renderer/open_gl/open_gl_compute_shader.h"

namespace Vulkyrie::Renderer {
    Ref<ComputeShader> Create(Vulkyrie::Core::GraphicsAPI api, const std::filesystem::path &computeShaderPath) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<Vulkyrie::Renderer::OpenGLComputeShader>(computeShaderPath);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for ComputeShader creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie::Renderer
