#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    /** @brief Base structure for different types of lights. */
    struct Light {
        public:
            /** @brief Virtual destructor for proper cleanup of derived classes. */
            // virtual ~Light() = default;

            /** @brief Intensity of the light's ambient component. */
            glm::vec3 Ambient;

            /** @brief Intensity of the light's diffuse component. */
            glm::vec3 Diffuse;

            /** @brief Intensity of the light's specular component. */
            glm::vec3 Specular;
    };

    /** @brief Structure representing a directional light source. */
    struct DirectionalLight {
        public:
            /** @brief Base light properties. */
            Vulkyrie::Renderer::Light Light;

            /** @brief Direction vector of the light. */
            glm::vec3 Direction;
    };

    /** @brief Structure representing a point light source. */
    struct PointLight {
        public:
            /** @brief Base light properties. */
            Vulkyrie::Renderer::Light Light;

            /** @brief Position of the light in world space. */
            glm::vec3 Position;

            /** @brief Constant term for the attenuation factor. */
            f32 AttenuationConstant;

            /** @brief Linear term for the attenuation factor. */
            f32 AttenuationLinear;

            /** @brief Quadratic term for the attenuation factor. */
            f32 AttenuationQuadratic;
    };

} // namespace Vulkyrie::Renderer
