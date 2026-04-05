#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    /** @brief Base structure for different types of lights. */
    struct Light {
        public:
            /** @brief Intensity of the light's ambient component. */
            glm::vec3 Ambient;

            /** @brief Intensity of the light's diffuse component. */
            glm::vec3 Diffuse;

            /** @brief Intensity of the light's specular component. */
            glm::vec3 Specular;

            /** @brief Constructor to initialize all properties of the light.
             * @param ambient Ambient light intensity.
             * @param diffuse Diffuse light intensity.
             * @param specular Specular light intensity. */
            Light(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular) : Ambient(ambient), Diffuse(diffuse), Specular(specular) {
            }
    };

    /** @brief Structure representing a directional light source. */
    struct DirectionalLight : public Light {
        public:
            /** @brief Direction vector of the light. */
            glm::vec3 Direction;

            /** @brief Constructor to initialize all properties of the directional light.
             * @param ambient Ambient light intensity.
             * @param diffuse Diffuse light intensity.
             * @param specular Specular light intensity.
             * @param direction Direction vector of the light. */
            DirectionalLight(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, glm::vec3 direction)
                : Light(ambient, diffuse, specular), Direction(direction) {
            }
    };

    /** @brief Structure representing a point light source. */
    struct PointLight : public Light {
        public:
            /** @brief Position of the light in world space. */
            glm::vec3 Position;

            /** @brief Constant term for the attenuation factor. */
            f32 AttenuationConstant;

            /** @brief Linear term for the attenuation factor. */
            f32 AttenuationLinear;

            /** @brief Quadratic term for the attenuation factor. */
            f32 AttenuationQuadratic;

            /** @brief Constructor to initialize all properties of the point light.
             * @param ambient Ambient light intensity.
             * @param diffuse Diffuse light intensity.
             * @param specular Specular light intensity.
             * @param position Position of the light in world space.
             * @param attenuationConstant Constant term for attenuation.
             * @param attenuationLinear Linear term for attenuation.
             * @param attenuationQuadratic Quadratic term for attenuation. */
            PointLight(glm::vec3 ambient,
                       glm::vec3 diffuse,
                       glm::vec3 specular,
                       glm::vec3 position,
                       f32 attenuationConstant,
                       f32 attenuationLinear,
                       f32 attenuationQuadratic)
                : Light(ambient, diffuse, specular), Position(position), AttenuationConstant(attenuationConstant), AttenuationLinear(attenuationLinear),
                  AttenuationQuadratic(attenuationQuadratic) {
            }
    };

    /** @brief Structure representing a spotlight source. */
    struct SpotLight : public PointLight {
        public:
            /** @brief Direction vector of the spotlight. */
            glm::vec3 Direction;

            /** @brief Inner cutoff angle (in radians) for the spotlight's cone. */
            f32 CutoffInner;

            /** @brief Outer cutoff angle (in radians) for the spotlight's cone. */
            f32 CutoffOuter;

            /** @brief Constructor to initialize all properties of the spotlight.
             * @param ambient Ambient light intensity.
             * @param diffuse Diffuse light intensity.
             * @param specular Specular light intensity.
             * @param position Position of the spotlight in world space.
             * @param attenuationConstant Constant term for attenuation.
             * @param attenuationLinear Linear term for attenuation.
             * @param attenuationQuadratic Quadratic term for attenuation.
             * @param direction Direction vector of the spotlight.
             * @param cutoffInner Inner cutoff angle (in radians).
             * @param cutoffOuter Outer cutoff angle (in radians). */
            SpotLight(glm::vec3 ambient,
                      glm::vec3 diffuse,
                      glm::vec3 specular,
                      glm::vec3 position,
                      f32 attenuationConstant,
                      f32 attenuationLinear,
                      f32 attenuationQuadratic,
                      glm::vec3 direction,
                      f32 cutoffInner,
                      f32 cutoffOuter)
                : PointLight(ambient, diffuse, specular, position, attenuationConstant, attenuationLinear, attenuationQuadratic), Direction(direction),
                  CutoffInner(cutoffInner), CutoffOuter(cutoffOuter) {
            }
    };

} // namespace Vulkyrie
