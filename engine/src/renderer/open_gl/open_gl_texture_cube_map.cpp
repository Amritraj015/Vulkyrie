#include "renderer/open_gl/open_gl_texture_cube_map.h"
#include "vlkypch.h"
#include "glad/glad.h"
#include "vendor/stb_image.h"

namespace Vulkyrie::Renderer {
    OpenGLTextureCubeMap::OpenGLTextureCubeMap(std::array<std::filesystem::path, 6> faces)
        : TextureCubeMap(std::move(faces)) {
        // Create texture object
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_textureId);
        if (_textureId == 0) {
            VERROR("Failed to create OpenGL texture for cube map");
            throw std::runtime_error("Failed to create OpenGL texture for cube map");
        }

        stbi_set_flip_vertically_on_load(false);

        i32 width = 0;
        i32 height = 0;
        i32 channels = 0;

        // ---- Load first face to establish format and size ----
        stbi_uc *data = stbi_load(_faces[0].c_str(), &width, &height, &channels, 0);
        if (!data) {
            glDeleteTextures(1, &_textureId);
            VERROR("Failed to load cubemap face: {}", _faces[0].c_str());
            throw std::runtime_error("Failed to load cubemap face: " + _faces[0].string());
        }

        GLenum internalFormat = 0;
        GLenum dataFormat = 0;

        if (channels == 4) {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        } else if (channels == 3) {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        } else {
            stbi_image_free(data);
            glDeleteTextures(1, &_textureId);
            VERROR("Unsupported channel count for cubemap face: {}", _faces[0].c_str());
            throw std::runtime_error("Unsupported channel count for cubemap face: " + _faces[0].string());
        }

        // Allocate immutable storage for all faces
        glTextureStorage2D(_textureId, 1, internalFormat, width, height);

        // Upload first face
        glTextureSubImage3D(_textureId, 0, 0, 0, 0, width, height, 1, dataFormat, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        // ---- Load remaining faces ----
        for (size_t i = 1; i < _faces.size(); ++i) {
            i32 faceWidth = 0;
            i32 faceHeight = 0;
            i32 faceChannels = 0;

            data = stbi_load(_faces[i].c_str(), &faceWidth, &faceHeight, &faceChannels, 0);
            if (!data) {
                glDeleteTextures(1, &_textureId);
                VERROR("Failed to load cubemap face: {}", _faces[i].c_str());
                throw std::runtime_error("Failed to load cubemap face: " + _faces[i].string());
            }

            // Validate dimensions
            if (faceWidth != width || faceHeight != height) {
                stbi_image_free(data);
                glDeleteTextures(1, &_textureId);
                VERROR("Cubemap face size mismatch: {}", _faces[i].c_str());
                throw std::runtime_error("Cubemap face size mismatch: " + _faces[i].string());
            }

            // Validate channel count
            if (faceChannels != channels) {
                stbi_image_free(data);
                glDeleteTextures(1, &_textureId);
                VERROR("Cubemap face channel mismatch: {}", _faces[i].c_str());
                throw std::runtime_error("Cubemap face channel mismatch: " + _faces[i].string());
            }

            // Upload face
            glTextureSubImage3D(_textureId, 0, 0, 0, static_cast<i32>(i), width, height, 1, dataFormat, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }

        // Sampler parameters
        glTextureParameteri(_textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // Mark cubemap as valid
        _isValid = true;
    }

    void OpenGLTextureCubeMap::Bind(u32 slot) const {
        glBindTextureUnit(slot, _textureId);
    }

    OpenGLTextureCubeMap::~OpenGLTextureCubeMap() noexcept {
        if (_textureId != 0) {
            glDeleteTextures(1, &_textureId);
            _textureId = 0;
        }
    }
} // namespace Vulkyrie::Renderer
