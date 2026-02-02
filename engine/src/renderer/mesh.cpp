#include "vlkypch.h"
#include "renderer/open_gl/open_gl_mesh.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<Mesh> Mesh::Create(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for Mesh creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
