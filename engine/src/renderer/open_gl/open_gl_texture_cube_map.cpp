#include "renderer/open_gl/open_gl_texture_cube_map.h"
#include "core/logger.h"
#include "glad/glad.h"
#include "vendor/stb_image.h"

namespace Vulkyrie::Renderer {
    OpenGLTextureCubeMap::OpenGLTextureCubeMap(const std::vector<std::filesystem::path> &faces) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_textureId);

        i32 width = 0, height = 0, channels = 0;

        stbi_set_flip_vertically_on_load(false);

        // Load first face to get dimensions + format
        stbi_uc *data = stbi_load(faces[0].c_str(), &width, &height, &channels, 0);

        if (!data) {
            VERROR("Cubemap tex failed to load at path: {}", faces[0].c_str());
        }

        GLenum internalFormat;
        GLenum dataFormat;

        if (channels == 4) {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        } else {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }

        // Allocate immutable storage for all 6 faces
        glTextureStorage2D(_textureId,
                           1, // mip levels
                           internalFormat,
                           width,
                           height);

        // Upload first face
        glTextureSubImage3D(_textureId,
                            0,
                            0,
                            0,
                            0, // x, y, face index
                            width,
                            height,
                            1,
                            dataFormat,
                            GL_UNSIGNED_BYTE,
                            data);

        stbi_image_free(data);

        // Upload remaining faces
        for (u32 i = 1; i < faces.size(); i++) {
            data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

            if (!data) {
                VERROR("Cubemap tex failed to load at path: {}", faces[i].c_str());
                continue;
            }

            glTextureSubImage3D(_textureId,
                                0,
                                0,
                                0,
                                i, // z = face index
                                width,
                                height,
                                1,
                                dataFormat,
                                GL_UNSIGNED_BYTE,
                                data);

            stbi_image_free(data);
        }

        // Set sampler parameters
        glTextureParameteri(_textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void OpenGLTextureCubeMap::Bind(u32 slot) const {
        glBindTextureUnit(slot, _textureId);
    }

    OpenGLTextureCubeMap::~OpenGLTextureCubeMap() {
        glDeleteTextures(1, &_textureId);
    }
} // namespace Vulkyrie::Renderer
