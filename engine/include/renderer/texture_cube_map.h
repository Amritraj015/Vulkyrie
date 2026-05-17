#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    /** @brief Abstract base class for a cube map texture. */
    class TextureCubeMap {
    public:
        virtual ~TextureCubeMap() noexcept = default;

        /** @brief Creates a cube map texture from the specified file paths.
         * @param faces The file paths for the cube map faces.
         * @return A reference to the created TextureCubeMap.
         */
        static Ref<TextureCubeMap> Create(std::array<std::filesystem::path, 6> faces);

        /** @brief Gets the renderer-specific texture ID.
         * @returns The texture ID.
         */
        [[nodiscard]] inline u32 GetTextureID() const {
            return _textureId;
        }

        /** @brief Checks if the texture is valid.
         * @returns True if the texture is valid, false otherwise.
         */
        [[nodiscard]] bool IsValid() const {
            return _isValid;
        }

        /** @brief Gets the file paths for the cube map faces.
         * @returns The array of file paths for the cube map faces.
         */
        [[nodiscard]] const std::array<std::filesystem::path, 6> &GetFaces() const {
            return _faces;
        }

        /** @brief Binds the cube map texture to the specified slot.
         * @param slot The texture slot to bind to. Default is 0.
         */
        virtual void Bind(u32 slot = 0) const = 0;

    protected:
        /** @brief Constructs a TextureCubeMap with the specified file paths.
         * @param faces The file paths for the cube map faces.
         */
        TextureCubeMap(std::array<std::filesystem::path, 6> faces)
            : _textureId(0)
            , _isValid(false)
            , _faces(std::move(faces)) {
        }

        /** @brief The renderer-specific texture ID. */
        u32 _textureId;

        /** @brief Indicates whether the texture is valid. */
        bool _isValid;

        /** @brief The file paths for the cube map faces. */
        std::array<std::filesystem::path, 6> _faces;
    };
} // namespace Vulkyrie
