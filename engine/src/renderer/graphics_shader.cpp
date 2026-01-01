#include "glad/glad.h"
#include "vlkypch.h"
#include "renderer/graphics_shader.h"
#include "core/utilities.h"

namespace Vulkyrie::Renderer {
    GraphicsShader::GraphicsShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath)
        : _vertexShaderPath(vertexShaderPath), _fragmentShaderPath(fragmentShaderPath), _isValid(false) {
        _shaderProgram = Create(vertexShaderPath, fragmentShaderPath);
    }

    GraphicsShader::~GraphicsShader() {
        glDeleteProgram(_shaderProgram);
    }

    void GraphicsShader::Use() const {
        glUseProgram(_shaderProgram);
    }

    void GraphicsShader::SetBoolUniform(const std::string_view name, const bool value) const {
        glUniform1i(glGetUniformLocation(_shaderProgram, name.data()), static_cast<int>(value));
    }

    void GraphicsShader::SetIntUniform(const std::string_view name, const int value) const {
        glUniform1i(glGetUniformLocation(_shaderProgram, name.data()), value);
    }

    void GraphicsShader::SetFloatUniform(const std::string_view name, const float value) const {
        glUniform1f(glGetUniformLocation(_shaderProgram, name.data()), value);
    }

    void GraphicsShader::SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(_shaderProgram, name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void GraphicsShader::SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(_shaderProgram, name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void GraphicsShader::SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    u32 GraphicsShader::Reload() {
        // Create a new shader program.
        const u32 newShaderProgram = Create(_vertexShaderPath, _fragmentShaderPath);

        // Return old shader program handle if compilation failed.
        if (newShaderProgram == 0) return _shaderProgram;

        // Delete the old shader program.
        glDeleteProgram(_shaderProgram);

        // Update the shader program handle.
        _shaderProgram = newShaderProgram;

        // Return the new shader program's handle.
        return _shaderProgram;
    }

    u32 GraphicsShader::Create(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
        // Fetch the vertex shader source code.
        const std::string vertexShaderSource = Vulkyrie::Core::ReadTextFromFile(vertexShaderPath);

        // Fetch the fragment shader source code.
        const std::string fragmentShaderSource = Vulkyrie::Core::ReadTextFromFile(fragmentShaderPath);

        // Vertex shader
        const u32 vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);

        // Compile the vertex shader.
        const char *source = vertexShaderSource.c_str();
        glShaderSource(vertexShaderHandle, 1, &source, nullptr);
        glCompileShader(vertexShaderHandle);

        i32 success = 0;
        glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);

        // Check if the vertex shader compiled successfully.
        if (GL_FALSE == success) {
            // Get the length of the compilation error message.
            i32 maxLength = 0;
            glGetShaderiv(vertexShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the compilation error message.
            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(vertexShaderHandle, maxLength, &maxLength, &infoLog[0]);

            // At this point we can delete the vertex shader.
            glDeleteShader(vertexShaderHandle);

            // Log an error and return.
            VERROR("Failed to compile vertex shader: {} - Error: {}", _vertexShaderPath.c_str(), infoLog.data());

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Fragment shader
        const u32 fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

        // Compile the fragment shader.
        source = fragmentShaderSource.c_str();
        glShaderSource(fragmentShaderHandle, 1, &source, nullptr);
        glCompileShader(fragmentShaderHandle);

        glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &success);

        if (GL_FALSE == success) {
            // Get the length of the compilation error message.
            i32 maxLength = 0;
            glGetShaderiv(fragmentShaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the compilation error message.
            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(fragmentShaderHandle, maxLength, &maxLength, &infoLog[0]);

            // At this point we can, delete both the vertex and fragment shaders.
            glDeleteShader(vertexShaderHandle);
            glDeleteShader(fragmentShaderHandle);

            // Log an error and return.
            VERROR("Failed to compile fragment shader: {} - Error: {}", _fragmentShaderPath.c_str(), infoLog.data());

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
            i32 maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the linking error message.
            std::vector<char> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            // Log the linking error.
            VERROR("An error occurred while linking graphics shader program: {}", infoLog.data());

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
