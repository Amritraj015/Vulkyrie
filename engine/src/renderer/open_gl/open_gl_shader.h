#pragma once

#include "renderer/shader.h"

namespace Vulkyrie::Renderer {
    class OpenGLShader final : public Shader {
        public:
            /** @brief Constructs an OpenGL shader program by compiling the provided shader source file.
             * @param shaderSourcePath Path to the shader source file.
             */
            explicit OpenGLShader(const std::filesystem::path &shaderSourcePath);

            /** @brief Destructor to clean up the shader program. */
            ~OpenGLShader();

            /** @brief Reloads the shader from its source file. */
            u32 Reload() override;

            /** @brief Binds the shader program for use. */
            void Use() const override;

            /** @brief Sets a boolean uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The boolean value to set.
             */
            void SetBoolUniform(std::string_view name, bool value) const override;

            /** @brief Sets an integer uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The integer value to set.
             */
            void SetIntUniform(std::string_view name, int value) const override;

            /** @brief Sets a float uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The float value to set.
             */
            void SetFloatUniform(std::string_view name, f32 value) const override;

            /** @brief Sets a 2x2 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 2x2 matrix to set.
             */
            void SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const override;

            /** @brief Sets a 3x3 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 3x3 matrix to set.
             */
            void SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const override;

            /** @brief Sets a 4x4 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 4x4 matrix to set.
             */
            void SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const override;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The vec3 value to set.
             */
            void SetVec3Uniform(const std::string_view name, const glm::vec3 &value) const override;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param x The x component of the vec3.
             * @param y The y component of the vec3.
             * @param z The z component of the vec3.
             */
            void SetVec3Uniform(const std::string_view name, f32 x, f32 y, f32 z) const override;

        private:
            /** @brief Cache for uniform variable locations. */
            mutable std::unordered_map<std::string, i32> _uniformLocationCache;

            /** @brief Path to the shader source file. */
            std::filesystem::path _shaderSourcePath;

            /** @brief Creates and compiles a shader program from the shader source file.
             * @returns The ID of the created shader program.
             */
            u32 LoadAndCompile();

            /** @brief Retrieves the location of a uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @returns The location of the uniform variable.
             */
            i32 GetUniformLocation(const std::string &name) const;
    };
} // namespace Vulkyrie::Renderer
