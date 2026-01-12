#include "renderer/open_gl/open_gl_shader.h"
#include "core/utilities.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    static constexpr auto INVALID_SHADER_TYPE = 0;

    GLenum ShaderTypeFromString(const std::string_view type) {
        if (type == "vertex") return GL_VERTEX_SHADER;
        if (type == "fragment") return GL_FRAGMENT_SHADER;
        if (type == "geometry") return GL_GEOMETRY_SHADER;
        if (type == "compute") return GL_COMPUTE_SHADER;

        return INVALID_SHADER_TYPE;
    }

    std::unordered_map<GLenum, std::string_view> ExtractShaderStages(const std::string &source) {
        std::unordered_map<GLenum, std::string_view> shaderSources;

        const char *typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0); // Start of shader type declaration line
        while (pos != std::string::npos) {
            size_t eol = source.find_first_of("\r\n", pos); // End of shader type declaration line

            size_t begin = pos + typeTokenLength + 1; // Start of shader type name (after "#type " keyword)
            std::string type = source.substr(begin, eol - begin);
            
            // Trim whitespace from type string
            type.erase(0, type.find_first_not_of(" \t"));
            type.erase(type.find_last_not_of(" \t") + 1);

            size_t nextLinePos = source.find_first_not_of("\r\n", eol); // Start of shader code after shader type declaration line
            pos = source.find(typeToken, nextLinePos);                  // Start of next shader type declaration line

            // Calculate the end position (exclude trailing whitespace before next #type)
            size_t endPos;
            if (pos == std::string::npos) {
                endPos = source.size();
            } else {
                // Find the last non-whitespace character before the next #type token
                endPos = source.find_last_not_of(" \t\r\n", pos - 1) + 1;
            }

            shaderSources[ShaderTypeFromString(type)] = std::string_view(source.c_str() + nextLinePos, endPos - nextLinePos);
        }

        return shaderSources;
    }

    OpenGLShader::OpenGLShader(const std::filesystem::path &shaderSourcePath) : _shaderSourcePath(shaderSourcePath) {
        _shaderProgramID = LoadAndCompile();
    }

    OpenGLShader::~OpenGLShader() {
        glDeleteProgram(_shaderProgramID);
    }

    void OpenGLShader::Use() const {
        glUseProgram(_shaderProgramID);
    }

    void OpenGLShader::SetBoolUniform(const std::string_view name, const bool value) const {
        glUniform1i(GetUniformLocation(name.data()), static_cast<int>(value));
    }

    void OpenGLShader::SetIntUniform(const std::string_view name, const int value) const {
        glUniform1i(GetUniformLocation(name.data()), value);
    }

    void OpenGLShader::SetFloatUniform(const std::string_view name, const float value) const {
        glUniform1f(GetUniformLocation(name.data()), value);
    }

    void OpenGLShader::SetMat2Uniform(const std::string_view name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLShader::SetMat3Uniform(const std::string_view name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLShader::SetMat4Uniform(const std::string_view name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(GetUniformLocation(name.data()), 1, GL_FALSE, &mat[0][0]);
    }

    void OpenGLShader::SetVec3Uniform(const std::string_view name, const glm::vec3 &value) const {
        glUniform3fv(GetUniformLocation(name.data()), 1, &value[0]);
    }

    void OpenGLShader::SetVec3Uniform(const std::string_view name, f32 x, f32 y, f32 z) const {
        glUniform3f(GetUniformLocation(name.data()), x, y, z);
    }

    i32 OpenGLShader::GetUniformLocation(const std::string &name) const {
        if (auto it = _uniformLocationCache.find(name); it != _uniformLocationCache.end()) {
            return it->second;
        }

        const i32 location = glGetUniformLocation(_shaderProgramID, name.c_str());

#if defined(VULKYRIE_DEBUG)
        if (location == -1) {
            VERROR("Uniform '{}' does not exist in shader program: {}.", name, _shaderProgramID);
        }
#endif

        _uniformLocationCache[name] = location;

        return location;
    }

    u32 OpenGLShader::Reload() {
        // Create a new shader program.
        const u32 newShaderProgram = LoadAndCompile();

        // Return old shader program handle if compilation failed.
        if (newShaderProgram == 0) return _shaderProgramID;

        // Delete the old shader program.
        glDeleteProgram(_shaderProgramID);

        // Update the shader program handle.
        _shaderProgramID = newShaderProgram;

        // Return the new shader program's handle.
        return _shaderProgramID;
    }

    u32 OpenGLShader::LoadAndCompile() {
        // Fetch the vertex shader source code.
        const std::string shaderSource = Vulkyrie::Core::ReadTextFromFile(_shaderSourcePath);

        // Split the shader source into its respective stages.
        const auto shaderStages = ExtractShaderStages(shaderSource);

        // Attach the vertex and fragment shaders to
        // the program and then link the program.
        u32 program = glCreateProgram();
        i32 success = 0;

        // Store shader IDs so that we can delete them after
        // they are attached and linked with the program.
        std::vector<u32> shaderIds(shaderStages.size());

        for (const auto &[type, source] : shaderStages) {
            if (INVALID_SHADER_TYPE == type) {
                VERROR("Invalid shader type encountered during shader compilation.");

                // Mark the shader as invalid.
                _isValid = false;

                return 0;
            } else {
                // Fragment shader
                const u32 shaderID = glCreateShader(type);
                const char *shaderSource = source.data();

            std::cout << "Compiling shader source: " << shaderSource << "\n";


                // Compile the fragment shader.
                glShaderSource(shaderID, 1, &shaderSource, nullptr);
                glCompileShader(shaderID);

                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

                if (GL_FALSE == success) {
                    // Get the length of the compilation error message.
                    i32 maxLength = 0;
                    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

                    // Get the compilation error message.
                    std::vector<char> infoLog(maxLength);
                    glGetShaderInfoLog(shaderID, maxLength, &maxLength, &infoLog[0]);

                    // Delete the shaders because the compilation has failed.
                    for (const u32 shaderID : shaderIds) {
                        // Detach the shader from the program.
                        glDetachShader(program, shaderID);

                        // Delete the shader from memory.
                        glDeleteShader(shaderID);
                    }

                    // Log an error and return.
                    VERROR("Failed to compile fragment shader: {} - Error: {}", shaderID, infoLog.data());

                    // Mark the shader as invalid.
                    _isValid = false;

                    return 0;
                }

                // Attach the compiled shader to the program.
                glAttachShader(program, shaderID);

                // Store the shader ID for later deletion.
                shaderIds.push_back(shaderID);
            }
        }

        // Link the shader program.
        glLinkProgram(program);

        // Delete the shaders as they are no longer needed.
        for (const u32 shaderID : shaderIds) {
            // Detach the shader from the program.
            glDetachShader(program, shaderID);

            // Delete the shader from memory.
            glDeleteShader(shaderID);
        }

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
