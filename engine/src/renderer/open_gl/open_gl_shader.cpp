#include "renderer/open_gl/open_gl_shader.h"
#include "core/utilities.h"
#include "core/asserts.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    static constexpr auto INVALID_SHADER_TYPE = 0;

    GLenum ShaderTypeFromString(const std::string_view type) {
        if (type == "vertex") return GL_VERTEX_SHADER;
        if (type == "fragment") return GL_FRAGMENT_SHADER;
        if (type == "geometry") return GL_GEOMETRY_SHADER;
        if (type == "compute") return GL_COMPUTE_SHADER;
        if (type == "tess_control") return GL_TESS_CONTROL_SHADER;
        if (type == "tess_evaluation") return GL_TESS_EVALUATION_SHADER;

        return INVALID_SHADER_TYPE;
    }

    std::unordered_map<GLenum, std::string_view> ExtractShaderStages(const std::string &source) {
        std::unordered_map<GLenum, std::string_view> result;

        constexpr std::string_view tag = "#type";
        size_t pos = 0;

        while (true) {
            size_t tagPos = source.find(tag, pos);
            if (tagPos == std::string_view::npos) break;

            size_t lineEnd = source.find_first_of("\r\n", tagPos);

            VASSERT_EXPR(lineEnd != std::string_view::npos, "Malformed #type line in shader source.");
            // if (lineEnd == std::string_view::npos) throw std::runtime_error("Malformed #type line");

            // Parse shader type
            size_t typeStart = tagPos + tag.size();

            // Skip whitespace to get to the shader type name
            while (typeStart < lineEnd && std::isspace(static_cast<u8>(source[typeStart]))) {
                ++typeStart;
            }

            std::string_view typeName(&source[typeStart], lineEnd - typeStart);
            GLenum shaderType = ShaderTypeFromString(typeName);

            VASSERT_EXPR(shaderType != INVALID_SHADER_TYPE, "Unknown shader type '{}'", typeName);

            // Start of GLSL code (exactly after newline)
            size_t codeStart = source.find_first_not_of("\r\n", lineEnd);
            if (codeStart == std::string_view::npos) break;

            // End at next #type or EOF
            size_t nextTag = source.find(tag, codeStart);
            size_t codeEnd = (nextTag == std::string_view::npos) ? source.size() : nextTag;

            result[shaderType] = std::string_view(source.data() + codeStart, codeEnd - codeStart);

            pos = codeEnd;
        }

        return result;
    }

    OpenGLShader::OpenGLShader(const std::filesystem::path &shaderSourcePath)
        : _shaderSourcePath(shaderSourcePath) {

        VLKY_PROFILE_FUNCTION();

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

    void OpenGLShader::SetFloatUniform(const std::string_view name, const f32 value) const {
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

        if (location == -1) {
            VERROR("Uniform '{}' does not exist in shader program: {}.", name, _shaderProgramID);
        }

        _uniformLocationCache[name] = location;

        return location;
    }

    u32 OpenGLShader::Reload() {
        VLKY_PROFILE_FUNCTION();

        // Create a new shader program.
        const u32 newShaderProgram = LoadAndCompile();

        // Return old shader program handle if compilation failed.
        if (newShaderProgram == 0) {
            // TODO: _isValid is set to false in this case even though the old shader program is still valid. Need to revisit this.
            return _shaderProgramID;
        }

        // Delete the old shader program.
        glDeleteProgram(_shaderProgramID);

        // Clear the uniform location cache since the shader program may have changed
        // and the old uniform locations may no longer be valid.
        _uniformLocationCache.clear();

        // Update the shader program handle.
        _shaderProgramID = newShaderProgram;

        // Return the new shader program's handle.
        return _shaderProgramID;
    }

    u32 OpenGLShader::LoadAndCompile() {
        // Fetch the shader source code.
        const std::string shaderSources = Vulkyrie::Core::ReadTextFromFile(_shaderSourcePath);

        // Split the shader source into its respective stages.
        const auto shaderStages = ExtractShaderStages(shaderSources);

        // Attach the shaders to the program and then link the program.
        u32 program = glCreateProgram();
        i32 success = 0;

        // Store shader IDs so that we can delete them after
        // they are attached and linked with the program.
        std::vector<u32> shaderIds;
        shaderIds.reserve(shaderStages.size());

        for (const auto &[type, source] : shaderStages) {
            if (INVALID_SHADER_TYPE == type) {
                VERROR("Invalid shader type encountered during shader compilation.");

                // Mark the shader as invalid.
                _isValid = false;

                return 0;
            } else {
                // Create the shader object.
                const u32 shaderID = glCreateShader(type);
                const char *src = source.data();
                const i32 srcLength = static_cast<i32>(source.size());

                // Compile the shader.
                glShaderSource(shaderID, 1, &src, &srcLength);
                glCompileShader(shaderID);

                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

                if (GL_FALSE == success) {
                    // Get the length of the compilation error message.
                    i32 logLength = 0;
                    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

                    // Get the compilation error message.
                    std::vector<char> infoLog(logLength);
                    glGetShaderInfoLog(shaderID, logLength, &logLength, &infoLog[0]);

                    // Log an error and return.
                    VERROR("Failed to compile shader: {} - Error: {}", shaderID, infoLog.data());

                    // Detach and delete the current shader since the compilation has failed.
                    glDetachShader(program, shaderID);
                    glDeleteShader(shaderID);

                    // Delete the already added shaders because the compilation has failed.
                    for (const u32 id : shaderIds) {
                        // Detach the shader from the program.
                        glDetachShader(program, id);

                        // Delete the shader from memory.
                        glDeleteShader(id);
                    }

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

            // Delete the shaders since the linking has failed.
            for (const u32 id : shaderIds) {
                // Detach the shader from the program.
                glDetachShader(program, id);

                // Delete the shader from memory.
                glDeleteShader(id);
            }

            // At this point we can delete the program.
            glDeleteProgram(program);

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);

        if (GL_FALSE == success) {
            // Get the validation error length.
            i32 maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // Get the validation error message.
            std::vector<char> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            // Log the validation error.
            VERROR("An error occurred while validating graphics shader program: {}", infoLog.data());

            // Delete the shaders since the validation has failed.
            for (const u32 id : shaderIds) {
                // Detach the shader from the program.
                glDetachShader(program, id);
                // Delete the shader from memory.
                glDeleteShader(id);
            }

            // At this point we can delete the program.
            glDeleteProgram(program);

            // Mark the shader as invalid.
            _isValid = false;

            return 0;
        }

        // Delete the shaders as they are no longer needed.
        for (const u32 shaderID : shaderIds) {
            // Detach the shader from the program.
            glDetachShader(program, shaderID);

            // Delete the shader from memory.
            glDeleteShader(shaderID);
        }

        // Mark the shader as valid.
        _isValid = true;

        // Return the shader program handle.
        return program;
    }
} // namespace Vulkyrie::Renderer
