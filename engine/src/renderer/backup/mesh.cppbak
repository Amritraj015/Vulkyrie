#include "vlkypch.h"
#include "renderer/open_gl/open_gl_mesh.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {

    Ref<Mesh> Mesh::Create(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for Mesh creation!");
                return nullptr;
        }
    }

} // namespace Vulkyrie
