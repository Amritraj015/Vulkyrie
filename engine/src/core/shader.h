#pragma once

#include "defines.h"

namespace Vulkyrie::Core {
    class Shader {
    public:
        Shader(const char* vertexPath, const char* fragmentPath);
        ~Shader();

        void use() const;
        [[nodiscard]] inline GLuint getID() const;

    private:
        GLuint ID;
        void checkCompileErrors(GLuint shader, const std::string& type);
    };
}
