#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    class Shader {
        public:
            /** @brief Virtual destructor for the Shader class. */
            virtual ~Shader() = default;

            /** @brief Returns whether the shader is valid (compiled and linked successfully). */
            [[nodiscard]] inline bool IsValid() const {
                return _isValid;
            }

            /** @brief Returns the shader program ID. */
            [[nodiscard]] inline u32 GetProgramID() const {
                return _shaderProgramID;
            }

            /** @brief Reloads the shader from its source files. */
            virtual u32 Reload() = 0;

            /** @brief Binds the shader program for use. */
            virtual void Use() const = 0;

            /** @brief Sets a boolean uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The boolean value to set.
             */
            virtual void SetBoolUniform(std::string_view name, bool value) const = 0;

            /** @brief Sets an integer uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The integer value to set.
             */
            virtual void SetIntUniform(std::string_view name, int value) const = 0;

            /** @brief Sets a float uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The float value to set.
             */
            virtual void SetFloatUniform(std::string_view name, f32 value) const = 0;

            /** @brief Sets a 2x2 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 2x2 matrix to set.
             */
            virtual void SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const = 0;

            /** @brief Sets a 3x3 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 3x3 matrix to set.
             */
            virtual void SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const = 0;

            /** @brief Sets a 4x4 matrix uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param mat The 4x4 matrix to set.
             */
            virtual void SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const = 0;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param value The vec3 value to set.
             */
            virtual void SetVec3Uniform(const std::string_view name, const glm::vec3 &value) const = 0;

            /** @brief Sets a vec3 uniform variable in the shader program.
             * @param name The name of the uniform variable.
             * @param x The x component of the vec3.
             * @param y The y component of the vec3.
             * @param z The z component of the vec3.
             */
            virtual void SetVec3Uniform(const std::string_view name, f32 x, f32 y, f32 z) const = 0;

            /** @brief Creates a shader program based from the provided source path.
             * @param shaderSourcePath The file path to the shader source code.
             * @returns A reference to the created Shader.
             */
            static Ref<Shader> Create(const std::filesystem::path &shaderSourcePath);

        protected:
            /** @brief The ID of the created shader program. */
            u32 _shaderProgramID;

            /** @brief Indicates whether the shader is valid (compiled and linked successfully). */
            bool _isValid;
    };
} // namespace Vulkyrie
