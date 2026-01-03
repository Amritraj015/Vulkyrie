#pragma once

namespace Vulkyrie::Materials {
    struct Material {
        public:
            const glm::vec4 Ambient;
            const glm::vec4 Diffuse;
            const glm::vec4 Specular;
            const float Shininess;
    };
} // namespace Vulkyrie::Materials
