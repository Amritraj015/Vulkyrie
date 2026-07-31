#include "renderer/texture_2D.h"
#include "renderer/open_gl/open_gl_texture_2D.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {

    Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTexture2D>(specification);
            default:
                return nullptr;
        }
    }

    Ref<Texture2D> Texture2D::Create(const std::filesystem::path &path) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLTexture2D>(path);
            default:
                return nullptr;
        }
    }

} // namespace Vulkyrie
