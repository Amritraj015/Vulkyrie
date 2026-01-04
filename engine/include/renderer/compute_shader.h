#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"

namespace Vulkyrie::Renderer {
    class ComputeShader {
        public:
            /** @brief Returns whether the compute shader is valid (compiled successfully). */
            [[nodiscard]] inline bool IsValid() const {
                return _isValid;
            }

            /** @brief Returns the compute shader program handle. */
            [[nodiscard]] inline u32 GetShaderProgram() const {
                return _shaderProgram;
            }

            /** @brief Reloads the compute shader from its source file. */
            virtual u32 Reload() = 0;

            /** @brief Binds the compute shader program for use. */
            virtual inline void Use() const = 0;

            /** @brief Creates a compute shader based on the specified graphics API and shader file path.
             * @param api The graphics API to use.
             * @param computeShaderPath The file path to the compute shader source code.
             * @returns A reference to the created ComputeShader object.
             */
            Ref<ComputeShader> Create(Vulkyrie::Core::GraphicsAPI api, const std::filesystem::path &computeShaderPath);

        protected:
            /** @brief A handle to the compute shader program. */
            u32 _shaderProgram;

            /** @brief Indicates whether the compute shader is valid (compiled successfully). */
            bool _isValid;
    };
} // namespace Vulkyrie::Renderer
