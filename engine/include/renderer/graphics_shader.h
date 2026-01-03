#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    class GraphicsShader {
        public:
            /** @brief Default constructor for the GraphicsShader class. */
            GraphicsShader() = default;

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
            void SetBoolUniform(std::string_view name, bool value) const;

            /** @brief Sets an integer uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The integer value to set.
             */
            void SetIntUniform(std::string_view name, int value) const;

            /** @brief Sets a float uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The float value to set.
             */
            void SetFloatUniform(std::string_view name, float value) const;

            /** @brief Sets a 2x2 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 2x2 matrix to set.
             */
            void SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const;

            /** @brief Sets a 3x3 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 3x3 matrix to set.
             */
            void SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const;

            /** @brief Sets a 4x4 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 4x4 matrix to set.
             */
            void SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The vec3 value to set.
             */
            void SetVec3Uniform(const std::string_view name, const glm::vec3 &value) const;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param x The x component of the vec3.
             * @param y The y component of the vec3.
             * @param z The z component of the vec3.
             */
            void SetVec3Uniform(const std::string_view name, f32 x, f32 y, f32 z) const;

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
} // namespace Vulkyrie::Renderer
