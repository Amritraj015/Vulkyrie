#include "renderer/open_gl/open_gl_graphics_shader.h"
#include "core/utilities.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    OpenGLGraphicsShader::OpenGLGraphicsShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath)
        : _vertexShaderPath(vertexShaderPath), _fragmentShaderPath(fragmentShaderPath) {
        _shaderProgram = LoadAndCompile(vertexShaderPath, fragmentShaderPath);
    }

    OpenGLGraphicsShader::~OpenGLGraphicsShader() {
        glDeleteProgram(_shaderProgram);
    }

    void OpenGLGraphicsShader::Use() const {
        glUseProgram(_shaderProgram);
    }

    void OpenGLGraphicsShader::SetBoolUniform(const std::string_view name, const bool value) const {
        glUniform1i(GetUniformLocation(name.data()), static_cast<int>(value));
    }

    void OpenGLGraphicsShader::SetIntUniform(const std::string_view name, const int value) const {
        glUniform1i(GetUniformLocation(name.data()), value);
    }

    void OpenGLGraphicsShader::SetFloatUniform(const std::string_view name, const float value) const {
        glUniform1f(GetUniformLocation(name.data()), value);
    }

    void OpenGLGraphicsShader::SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLGraphicsShader::SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLGraphicsShader::SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLGraphicsShader::SetVec3Uniform(const std::string_view name, const glm::vec3 &value) const {
        glUniform3fv(GetUniformLocation(name.data()), 1, &value[0]);
    }

    void OpenGLGraphicsShader::SetVec3Uniform(const std::string_view name, f32 x, f32 y, f32 z) const {
        glUniform3f(GetUniformLocation(name.data()), x, y, z);
    }

    i32 OpenGLGraphicsShader::GetUniformLocation(const std::string &name) const {
        if (auto it = _uniformLocationCache.find(name); it != _uniformLocationCache.end()) {
            return it->second;
        }

        const i32 location = glGetUniformLocation(_shaderProgram, name.c_str());

#if defined(VULKYRIE_DEBUG)
        if (location == -1) {
            VERROR("Uniform '{}' does not exist in shader program: {}.", name, _shaderProgram);
        }
#endif

        _uniformLocationCache[name] = location;

        return location;
    }

    u32 OpenGLGraphicsShader::Reload() {
        // Create a new shader program.
        const u32 newShaderProgram = LoadAndCompile(_vertexShaderPath, _fragmentShaderPath);

        // Return old shader program handle if compilation failed.
        if (newShaderProgram == 0) return _shaderProgram;

        // Delete the old shader program.
        glDeleteProgram(_shaderProgram);

        // Update the shader program handle.
        _shaderProgram = newShaderProgram;

        // Return the new shader program's handle.
        return _shaderProgram;
    }

    u32 OpenGLGraphicsShader::LoadAndCompile(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
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
