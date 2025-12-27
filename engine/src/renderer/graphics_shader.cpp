#include "glad/glad.h"
#include "renderer/graphics_shader.h"
#include "core/logger.h"
#include "core/utilities.h"

namespace Vulkyrie::Renderer {
    GraphicsShader::GraphicsShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath)
        : _vertexShaderPath(vertexShaderPath), _fragmentShaderPath(fragmentShaderPath) {
        _shaderProgram = _oldShaderProgram = Create(vertexShaderPath, fragmentShaderPath);
    }

    GraphicsShader::~GraphicsShader() {
        glDeleteProgram(_shaderProgram);
    }

    void GraphicsShader::Use() const {
        glUseProgram(_shaderProgram);
    }

    void GraphicsShader::setBoolUniform(std::string_view name, bool value) const {
        glUniform1i(glGetUniformLocation(_shaderProgram, name.data()), (int)value);
    }

    void GraphicsShader::setIntUniform(std::string_view name, int value) const {
        glUniform1i(glGetUniformLocation(_shaderProgram, name.data()), value);
    }

    void GraphicsShader::setFloatUniform(std::string_view name, float value) const {
        glUniform1f(glGetUniformLocation(_shaderProgram, name.data()), value);
    }

    u32 GraphicsShader::Reload() {
        // Create a new shader program.
        _shaderProgram = Create(_vertexShaderPath, _fragmentShaderPath);

        // Return old shader if compilation failed
        if (_shaderProgram == 0) return _shaderProgram;

        // Delete the old shader program.
        glDeleteProgram(_oldShaderProgram);

        // Update the old shader program handle.
        _shaderProgram = _shaderProgram;

        // Return the new shader program's handle.
        return _shaderProgram;
    }

    u32 GraphicsShader::Create(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
        // Fetch the vertex shader source code.
        std::string vertexShaderSource = Vulkyrie::Core::ReadTextFromFile(vertexShaderPath);

        // Fetch the fragment shader source code.
        std::string fragmentShaderSource = Vulkyrie::Core::ReadTextFromFile(fragmentShaderPath);

        // Vertex shader
        u32 vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);

        // Compile the vertex shader.
        const char *source = (const char *)vertexShaderSource.c_str();
        glShaderSource(vertexShaderHandle, 1, &source, 0);
        glCompileShader(vertexShaderHandle);

        i32 success = 0;
        glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);

        // Check if the vertex shader compiled successfully.
        if (GL_FALSE == success) {
            // Get the length of the compilation error message.
            i32 maxLength = 0;
            glGetShaderiv(vertexShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the compilation error message.
            char infoLog[maxLength];
            glGetShaderInfoLog(vertexShaderHandle, maxLength, &maxLength, infoLog);

            // At this point we can delete the vertex shader.
            glDeleteShader(vertexShaderHandle);

            // Log an error and return.
            VERROR("Failed to compile vertex shader: %s - Error: %s", _vertexShaderPath.c_str(), infoLog)

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Fragment shader
        u32 fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

        // Compile the fragment shader.
        source = (const char *)fragmentShaderSource.c_str();
        glShaderSource(fragmentShaderHandle, 1, &source, 0);
        glCompileShader(fragmentShaderHandle);

        glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &success);

        if (GL_FALSE == success) {
            // Get the length of the compilation error message.
            i32 maxLength = 0;
            glGetShaderiv(fragmentShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the compilation error message.
            char infoLog[maxLength];
            glGetShaderInfoLog(fragmentShaderHandle, maxLength, &maxLength, infoLog);

            // At this point we can, delete both the vertex and fragment shaders.
            glDeleteShader(vertexShaderHandle);
            glDeleteShader(fragmentShaderHandle);

            // Log an error and return.
            VERROR("Failed to compile fragment shader: %s - Error: %s", _fragmentShaderPath.c_str(), infoLog);

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Attach the vertex and fragment shaders to 
        // the program and then link the program.
        u32 program = glCreateProgram();
        glAttachShader(program, vertexShaderHandle);
        glAttachShader(program, fragmentShaderHandle);
        glLinkProgram(program);

        // Detach Shaders from program.
        // (This is OK to do after the program has been linked)
        glDetachShader(program, vertexShaderHandle);
        glDetachShader(program, fragmentShaderHandle);

        // Delete the shaders from memory.
        // (The shaders are no longer needed at this point, so they can be deleted).
        glDeleteShader(vertexShaderHandle);
        glDeleteShader(fragmentShaderHandle);

        // Get the program's linking status.
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        // If linking failed, then we need to log an error and exit.
        if (GL_FALSE == success) {
            // Get the linking error length.
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the linking error message.
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            // Log the linking error.
            VERROR("An error occurred while linking graphics shader program:", infoLog.data());

            // At this point we can delete the program.
            glDeleteProgram(program);

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Mark the shader as valid.
        _isValid = true;

        // Return the shader program handle.
        return program;
    }
} // namespace Vulkyrie::Renderer