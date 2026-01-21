#include "core/logger.h"
#include "renderer/open_gl/open_gl_mesh.h"

namespace Vulkyrie::Renderer {
    Ref<Mesh> Mesh::Create(Vulkyrie::Core::GraphicsAPI api, std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<Ref<Texture2D>> &&textures) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for Mesh creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
