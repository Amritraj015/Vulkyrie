#include "renderer/texture_2D.h"
#include "renderer/open_gl/open_gl_texture_2D.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTexture2D>(specification);
            default:
                return nullptr;
        }
    }

    Ref<Texture2D> Texture2D::Create(const std::filesystem::path &path) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTexture2D>(path);
            default:
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
