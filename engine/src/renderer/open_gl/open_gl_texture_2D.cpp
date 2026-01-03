#include "open_gl_texture_2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

namespace Vulkyrie::Renderer {
    namespace Utils {
        static GLenum VulkyrieImageFormatToOpenGLDataFormat(TextureImageFormat format) {
            switch (format) {
                case TextureImageFormat::RGB8:
                    return GL_RGB;
                case TextureImageFormat::RGBA8:
                    return GL_RGBA;
                case TextureImageFormat::R8:
                case TextureImageFormat::RGBA32F:
                case TextureImageFormat::None:
                    break;
            }

            return 0;
        }

        static GLenum VulkyrieImageFormatToOpenGLInternalFormat(TextureImageFormat format) {
            switch (format) {
                case TextureImageFormat::RGB8:
                    return GL_RGB8;
                case TextureImageFormat::RGBA8:
                    return GL_RGBA8;
                case TextureImageFormat::R8:
                case TextureImageFormat::RGBA32F:
                case TextureImageFormat::None:
                    break;
            }

            return 0;
        }
    } // namespace Utils

    OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification &specification)
        : _specification(specification), _width(_specification.Width), _height(_specification.Height) {

        _imageFormat = Utils::VulkyrieImageFormatToOpenGLInternalFormat(_specification.Format);
        _dataFormat = Utils::VulkyrieImageFormatToOpenGLDataFormat(_specification.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &_textureId);
        glTextureStorage2D(_textureId, 1, _imageFormat, _width, _height);

        glTextureParameteri(_textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // if (_specification.GenerateMips) {
        //     glGenerateTextureMipmap(_textureId);
        // }
    }

    OpenGLTexture2D::OpenGLTexture2D(const std::filesystem::path &path) : _path(path) {
        int width, height, channels;

        stbi_set_flip_vertically_on_load(1);

        stbi_uc *data = nullptr;
        {
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        }

        if (data) {
            _loaded = true;
            _width = width;
            _height = height;

            GLenum internalFormat = 0, dataFormat = 0;

            if (channels == 4) {
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
            } else if (channels == 3) {
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
            }

            _imageFormat = internalFormat;
            _dataFormat = dataFormat;

            glCreateTextures(GL_TEXTURE_2D, 1, &_textureId);
            glTextureStorage2D(_textureId, 1, internalFormat, _width, _height);

            glTextureParameteri(_textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTextureParameteri(_textureId, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(_textureId, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTextureSubImage2D(_textureId, 0, 0, 0, _width, _height, dataFormat, GL_UNSIGNED_BYTE, data);

            // If mipmaps are to be generated.
            // glGenerateTextureMipmap(_textureId);

            stbi_image_free(data);
        }
    }

    OpenGLTexture2D::~OpenGLTexture2D() {
        glDeleteTextures(1, &_textureId);
    }

    void OpenGLTexture2D::SetData(void *data, u32 size) {
        // u32 bpp = _dataFormat == GL_RGBA ? 4 : 3;
        glTextureSubImage2D(_textureId, 0, 0, 0, _width, _height, _dataFormat, GL_UNSIGNED_BYTE, data);
    }

    void OpenGLTexture2D::Bind(u32 slot) const {
        glBindTextureUnit(slot, _textureId);
    }
} // namespace Vulkyrie::Renderer
