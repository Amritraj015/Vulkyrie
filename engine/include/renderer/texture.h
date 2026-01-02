#pragma once

#include "renderer/texture_specification.h"

namespace Vulkyrie::Renderer {
    /** @brief Abstract base class for textures. */
    class Texture {
        public:
            /** @brief Default destructor for the Texture class. */
            virtual ~Texture() = default;

            /** @brief Gets the specification of the texture.
             * @returns The texture specification.
             */
            [[nodiscard]] inline virtual const TextureSpecification &GetSpecification() const = 0;

            /** @brief Gets the width of the texture in pixels.
             * @returns The width of the texture.
             */
            [[nodiscard]] inline virtual u32 GetWidth() const = 0;

            /** @brief Gets the height of the texture in pixels.
             * @returns The height of the texture.
             */
            [[nodiscard]] inline virtual u32 GetHeight() const = 0;

            /** @brief Gets the renderer-specific texture ID.
             * @returns The texture ID.
             */
            [[nodiscard]] inline virtual u32 GetTextureID() const = 0;

            /** @brief Gets the file path of the texture.
             * @returns The file path as a string reference.
             */
            [[nodiscard]] inline virtual const std::filesystem::path &GetPath() const = 0;

            /** @brief Sets the data of the texture.
             * @param data Pointer to the data to set.
             * @param size Size of the data in bytes.
             */
            virtual void SetData(void *data, u32 size) = 0;

            /** @brief Binds the texture to the specified slot.
             * @param slot The texture slot to bind to. Default is 0.
             */
            virtual void Bind(u32 slot = 0) const = 0;

            /** @brief Checks if the texture is loaded.
             * @returns True if the texture is loaded, false otherwise.
             */
            [[nodiscard]] inline virtual bool IsLoaded() const = 0;

            /** @brief Compares this texture with another for equality.
             * @param other The other texture to compare with.
             * @returns True if the textures are equal, false otherwise.
             */
            virtual bool operator==(const Texture &other) const = 0;
    };
} // namespace Vulkyrie::Renderer
