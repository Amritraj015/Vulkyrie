#include "glad/glad.h"
#include "defines.h"
#include "core/utilities.h"
#include "renderer/compute_shader.h"

namespace Vulkyrie::Renderer {
    ComputeShader::ComputeShader(const std::filesystem::path &computeShaderPath) : _computeShaderPath(computeShaderPath), _isValid(false) {
        _shaderProgram = Create();
    }

    ComputeShader::~ComputeShader() {
        glDeleteProgram(_shaderProgram);
    }

    inline void ComputeShader::Use() const {
        glUseProgram(_shaderProgram);
    }

    u32 ComputeShader::Reload() {
        // Create a new compute shader program.
        u32 newShaderProgram = Create();

        // Return old shader handle if compilation failed
        if (newShaderProgram == 0) return _shaderProgram;

        // Delete the old compute shader program.
        glDeleteProgram(_shaderProgram);

        // Update the shader program handle.
        _shaderProgram = newShaderProgram;

        // Return the new compute shader program's handle.
        return _shaderProgram;
    }

    u32 ComputeShader::Create() {
        // Read the compute shader source code from file.
        std::string shaderSource = Vulkyrie::Core::ReadTextFromFile(_computeShaderPath);

        // Create and compile the compute shader.
        u32 shaderHandle = glCreateShader(GL_COMPUTE_SHADER);
        const char *source = (const char *)shaderSource.c_str();
        glShaderSource(shaderHandle, 1, &source, 0);
        glCompileShader(shaderHandle);

        // Check for compilation errors.
        i32 success = 0;
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);

        // If compilation failed, log the error and return 0.
        if (GL_FALSE == success) {
            // Get the length of the compilation error message.
            i32 maxLength = 0;
            glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the compilation error message.
            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(shaderHandle, maxLength, &maxLength, &infoLog[0]);

            // Delete the shader as it is no longer needed.
            glDeleteShader(shaderHandle);

            // Log an error and return.
            VERROR("Failed to compile compile shader: {} - Error: {}", _computeShaderPath.c_str(), infoLog.data());

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Create the shader program and link the compute shader.
        u32 program = glCreateProgram();
        glAttachShader(program, shaderHandle);
        glLinkProgram(program);

        // Check for linking errors.
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        // If linking failed, log the error and return 0.
        if (GL_FALSE == success) {
            // Get the length of the linking error message.
            i32 maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the linking error message.
            std::vector<char> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            // Delete the program and shader as they are no longer needed.
            glDeleteProgram(program);
            glDeleteShader(shaderHandle);

            // Log an error and return.
            VERROR("An error occurred while linking compute shader program: {}", infoLog.data());

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Detach and delete the shader from the program.
        glDetachShader(program, shaderHandle);
        glDeleteShader(shaderHandle);

        // Mark the shader as valid.
        _isValid = true;

        // Return the shader program handle.
        return program;
    }
} // namespace Vulkyrie::Renderer
