#pragma once

#include "glad/glad.h"
#include "renderer/texture_2D.h"

namespace Vulkyrie {
    class OpenGLTexture2D final : public Texture2D {
    public:
        OpenGLTexture2D(const TextureSpecification &specification);
        OpenGLTexture2D(const std::filesystem::path &path);
        ~OpenGLTexture2D();

        [[nodiscard]] inline const TextureSpecification &GetSpecification() const override {
            return _specification;
        }

        [[nodiscard]] inline u32 GetWidth() const override {
            return _width;
        }

        [[nodiscard]] inline u32 GetHeight() const override {
            return _height;
        }

        [[nodiscard]] inline u32 GetTextureID() const override {
            return _textureId;
        }

        [[nodiscard]] inline const std::filesystem::path &GetPath() const override {
            return _path;
        }

        [[nodiscard]] inline std::string_view GetTextureFileName() const override {
            return _fileName;
        }

        void SetData(void *data) override;

        void Bind(u32 slot = 0) const override;

        [[nodiscard]] inline bool IsLoaded() const override {
            return _loaded;
        }

        bool operator==(const Texture &other) const override {
            return _textureId == other.GetTextureID();
        }

    private:
        TextureSpecification _specification;
        std::filesystem::path _path;
        std::string _fileName;
        bool _loaded = false;
        u32 _width, _height;
        u32 _textureId;
        GLenum _imageFormat, _dataFormat;
    };

} // namespace Vulkyrie
