#pragma once

#include "defines.h"

namespace Vulkyrie::Renderer {
    class GraphicsShader {
        public:
            /** @brief Constructs a Shader object with vertex and fragment shader file paths. 
             * @param vertexShaderPath Path to the vertex shader source file.
             * @param fragmentShaderPath Path to the fragment shader source file.
            */
            explicit GraphicsShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

            /** @brief Destructor to clean up the graphics shader program. */
            ~GraphicsShader();

            /** @brief Returns whether the shader is valid (compiled and linked successfully). */
            [[nodiscard]] inline bool IsValid() const {
                return _isValid;
            }

            /** @brief Returns the shader program handle. */
            [[nodiscard]] inline u32 GetShaderProgram() const {
                return _shaderProgram;
            }

            /** @brief Reloads the shader from its source files. */
            u32 Reload();

            /** @brief Binds the shader program for use. */
            void Use() const;

            /** @brief Sets a boolean uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The boolean value to set.
             */
            void setBoolUniform(std::string_view name, bool value) const;

            /** @brief Sets an integer uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The integer value to set.
             */
            void setIntUniform(std::string_view name, int value) const;

            /** @brief Sets a float uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The float value to set.
             */
            void setFloatUniform(std::string_view name, float value) const;

        private:
            /** @brief A handle to the shader program. */
            u32 _shaderProgram;

            /** @brief Indicates whether the shader is valid (compiled and linked successfully). */
            bool _isValid;

            /** @brief Path to the vertex shader source file. */
            std::filesystem::path _vertexShaderPath;

            /** @brief Path to the fragment shader source file. */
            std::filesystem::path _fragmentShaderPath;

            /** @brief Creates a shader program from vertex and fragment shader source files.
             * @param vertexPath Path to the vertex shader source file.
             * @param fragmentPath Path to the fragment shader source file.
             * @returns The handle of the created shader program.
             */
            u32 Create(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);
    };
}