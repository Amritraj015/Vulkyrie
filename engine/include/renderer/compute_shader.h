#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    class ComputeShader {
        public:
            /** @brief Constructs a ComputeShader object with the compute shader file path. 
             * @param computeShaderPath Path to the compute shader source file.
            */
            explicit ComputeShader(const std::filesystem::path &computeShaderPath);

            /** @brief Destructor to clean up the compute shader program. */
            ~ComputeShader();

            /** @brief Returns whether the compute shader is valid (compiled successfully). */
            [[nodiscard]] inline bool IsValid() const {
                return _isValid;
            }

            /** @brief Returns the compute shader program handle. */
            [[nodiscard]] inline u32 GetShaderProgram() const {
                return _shaderProgram;
            }

            /** @brief Reloads the compute shader from its source file. */
            u32 Reload();

            /** @brief Binds the compute shader program for use. */
            inline void Use() const;
        
        private:
            /** @brief A handle to the compute shader program. */
            u32 _shaderProgram;

            /** @brief Indicates whether the compute shader is valid (compiled successfully). */
            bool _isValid;

            /** @brief Path to the compute shader source file. */
            std::filesystem::path _computeShaderPath;

            /** @brief Creates a compute shader program from the compute shader source file.
             * @param computePath Path to the compute shader source file.
             * @returns The handle of the created compute shader program.
             */
            u32 Create();
    };
}