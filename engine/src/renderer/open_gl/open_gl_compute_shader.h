#pragma once

#include "renderer/compute_shader.h"

namespace Vulkyrie::Renderer {
    class OpenGLComputeShader final : public ComputeShader {
        public:
            /** @brief Constructs a ComputeShader object with the compute shader file path.
             * @param computeShaderPath Path to the compute shader source file.
             */
            explicit OpenGLComputeShader(const std::filesystem::path &computeShaderPath);

            /** @brief Destructor to clean up the compute shader program. */
            ~OpenGLComputeShader();

            /** @brief Reloads the compute shader from its source file. */
            virtual u32 Reload() override;

            /** @brief Binds the compute shader program for use. */
            virtual inline void Use() const override;

        private:
            /** @brief Path to the compute shader source file. */
            std::filesystem::path _computeShaderPath;

            /** @brief Loads and compiles the compute shader from the source file.
             * @returns The handle to the compiled compute shader program.
             */
            u32 LoadAndCompile();
    };
} // namespace Vulkyrie::Renderer
