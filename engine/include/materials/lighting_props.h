#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief Represents a material with ambient, diffuse, specular properties and shininess factor. */
    struct LightingProps {
        public:
            /** @brief The ambient color component of the material. */
            const glm::vec4 Ambient;

            /** @brief The diffuse color component of the material. */
            const glm::vec4 Diffuse;

            /** @brief The specular color component of the material. */
            const glm::vec4 Specular;

            /** @brief The shininess factor of the material. */
            const f32 Shininess;
    };

    // Source: http://devernay.free.fr/cours/opengl/materials.html

    /** @brief Predefined material: Emerald - A vibrant green gemstone with high reflectivity */
    static constexpr LightingProps Emerald = {
        .Ambient = glm::vec4(0.0215f, 0.1745f, 0.0215f, 1.0f),
        .Diffuse = glm::vec4(0.07568f, 0.61424f, 0.07568f, 1.0f),
        .Specular = glm::vec4(0.633f, 0.727811f, 0.633f, 1.0f),
        .Shininess = 0.6f,
    };

    /** @brief Predefined material: Jade - A smooth green ornamental stone with subtle sheen */
    static constexpr LightingProps Jade = {
        .Ambient = glm::vec4(0.135f, 0.2225f, 0.1575f, 1.0f),
        .Diffuse = glm::vec4(0.54f, 0.89f, 0.63f, 1.0f),
        .Specular = glm::vec4(0.316228f, 0.316228f, 0.316228f, 1.0f),
        .Shininess = 0.1f,
    };

    /** @brief Predefined material: Obsidian - A dark volcanic glass with moderate reflectivity */
    static constexpr LightingProps Obsidian = {
        .Ambient = glm::vec4(0.05375f, 0.05f, 0.06625f, 1.0f),
        .Diffuse = glm::vec4(0.18275f, 0.17f, 0.22525f, 1.0f),
        .Specular = glm::vec4(0.332741f, 0.328634f, 0.346435f, 1.0f),
        .Shininess = 0.3f,
    };

    /** @brief Predefined material: Pearl - A lustrous white organic gemstone with soft sheen */
    static constexpr LightingProps Pearl = {
        .Ambient = glm::vec4(0.25f, 0.20725f, 0.20725f, 1.0f),
        .Diffuse = glm::vec4(1.0f, 0.829f, 0.829f, 1.0f),
        .Specular = glm::vec4(0.296648f, 0.296648f, 0.296648f, 1.0f),
        .Shininess = 0.088f,
    };

    /** @brief Predefined material: Ruby - A brilliant red gemstone with high shine and reflectivity */
    static constexpr LightingProps Ruby = {
        .Ambient = glm::vec4(0.1745f, 0.01175f, 0.01175f, 1.0f),
        .Diffuse = glm::vec4(0.61424f, 0.04136f, 0.04136f, 1.0f),
        .Specular = glm::vec4(0.727811f, 0.626959f, 0.626959f, 1.0f),
        .Shininess = 0.6f,
    };

    /** @brief Predefined material: Turquoise - A blue-green gemstone with low shininess */
    static constexpr LightingProps Turquoise = {
        .Ambient = glm::vec4(0.1f, 0.18725f, 0.1745f, 1.0f),
        .Diffuse = glm::vec4(0.396f, 0.74151f, 0.69102f, 1.0f),
        .Specular = glm::vec4(0.297254f, 0.30829f, 0.306678f, 1.0f),
        .Shininess = 0.1f,
    };

    /** @brief Predefined material: Brass - A yellowish metal alloy with moderate shine */
    static constexpr LightingProps Brass = {
        .Ambient = glm::vec4(0.329412f, 0.223529f, 0.027451f, 1.0f),
        .Diffuse = glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f),
        .Specular = glm::vec4(0.992157f, 0.941176f, 0.807843f, 1.0f),
        .Shininess = 0.21794872f,
    };

    /** @brief Predefined material: Chrome - A highly reflective metallic surface with mirror-like finish */
    static constexpr LightingProps Chrome = {
        .Ambient = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f),
        .Diffuse = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
        .Specular = glm::vec4(0.774597f, 0.774597f, 0.774597f, 1.0f),
        .Shininess = 0.6f,
    };

    /** @brief Predefined material: Bronze - A brownish metal alloy with warm tones and medium reflectivity */
    static constexpr LightingProps Bronze = {
        .Ambient = glm::vec4(0.2125f, 0.1275f, 0.054f, 1.0f),
        .Diffuse = glm::vec4(0.714f, 0.4284f, 0.18144f, 1.0f),
        .Specular = glm::vec4(0.393548f, 0.271906f, 0.166721f, 1.0f),
        .Shininess = 0.2f,
    };

    /** @brief Predefined material: Polished Silver - A highly reflective silver metal with bright highlights */
    static constexpr LightingProps PolishedSilver = {
        .Ambient = glm::vec4(0.23125f, 0.23125f, 0.23125f, 1.0f),
        .Diffuse = glm::vec4(0.2775f, 0.2775f, 0.2775f, 1.0f),
        .Specular = glm::vec4(0.773911f, 0.773911f, 0.773911f, 1.0f),
        .Shininess = 0.6f,
    };

    /** @brief Predefined material: Copper - A reddish-brown metal with warm tones and low shininess */
    static constexpr LightingProps Copper = {
        .Ambient = glm::vec4(0.19125f, 0.0735f, 0.0225f, 1.0f),
        .Diffuse = glm::vec4(0.7038f, 0.27048f, 0.0828f, 1.0f),
        .Specular = glm::vec4(0.256777f, 0.137622f, 0.086014f, 1.0f),
        .Shininess = 0.1f,
    };

    /** @brief Predefined material: Gold - A precious metal with rich yellow tones and moderate reflectivity */
    static constexpr LightingProps Gold = {
        .Ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f),
        .Diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f),
        .Specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f),
        .Shininess = 0.4f,
    };

    /** @brief Predefined material: Silver - A precious metal with neutral gray tones and good reflectivity */
    static constexpr LightingProps Silver = {
        .Ambient = glm::vec4(0.19225f, 0.19225f, 0.19225f, 1.0f),
        .Diffuse = glm::vec4(0.50754f, 0.50754f, 0.50754f, 1.0f),
        .Specular = glm::vec4(0.508273f, 0.508273f, 0.508273f, 1.0f),
        .Shininess = 0.4f,
    };

    /** @brief Predefined material: Black Plastic - A dark plastic surface with moderate specular highlights */
    static constexpr LightingProps BlackPlastic = {
        .Ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.01f, 0.01f, 0.01f, 1.0f),
        .Specular = glm::vec4(0.50f, 0.50f, 0.50f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: Cyan Plastic - A blue-green plastic with moderate specular highlights */
    static constexpr LightingProps CyanPlastic = {
        .Ambient = glm::vec4(0.0f, 0.1f, 0.06f, 1.0f),
        .Diffuse = glm::vec4(0.0f, 0.50980392f, 0.50980392f, 1.0f),
        .Specular = glm::vec4(0.50196078f, 0.50196078f, 0.50196078f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: Green Plastic - A green plastic surface with moderate specular highlights */
    static constexpr LightingProps GreenPlastic = {
        .Ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.1f, 0.35f, 0.1f, 1.0f),
        .Specular = glm::vec4(0.45f, 0.55f, 0.45f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: Red Plastic - A red plastic surface with moderate specular highlights */
    static constexpr LightingProps RedPlastic = {
        .Ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f),
        .Specular = glm::vec4(0.7f, 0.6f, 0.6f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: White Plastic - A white plastic surface with moderate specular highlights */
    static constexpr LightingProps WhitePlastic = {
        .Ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f),
        .Specular = glm::vec4(0.70f, 0.70f, 0.70f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: Yellow Plastic - A yellow plastic surface with moderate specular highlights */
    static constexpr LightingProps YellowPlastic = {
        .Ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.5f, 0.5f, 0.0f, 1.0f),
        .Specular = glm::vec4(0.60f, 0.60f, 0.50f, 1.0f),
        .Shininess = 0.25f,
    };

    /** @brief Predefined material: Black Rubber - A dark rubber material with low shininess and soft highlights */
    static constexpr LightingProps BlackRubber = {
        .Ambient = glm::vec4(0.02f, 0.02f, 0.02f, 1.0f),
        .Diffuse = glm::vec4(0.01f, 0.01f, 0.01f, 1.0f),
        .Specular = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
        .Shininess = 0.078125f,
    };

    /** @brief Predefined material: Cyan Rubber - A blue-green rubber material with low shininess */
    static constexpr LightingProps CyanRubber = {
        .Ambient = glm::vec4(0.0f, 0.05f, 0.05f, 1.0f),
        .Diffuse = glm::vec4(0.4f, 0.5f, 0.5f, 1.0f),
        .Specular = glm::vec4(0.04f, 0.7f, 0.7f, 1.0f),
        .Shininess = 0.078125f,
    };

    /** @brief Predefined material: Green Rubber - A green rubber material with low shininess */
    static constexpr LightingProps GreenRubber = {
        .Ambient = glm::vec4(0.0f, 0.05f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.4f, 0.5f, 0.4f, 1.0f),
        .Specular = glm::vec4(0.04f, 0.7f, 0.04f, 1.0f),
        .Shininess = 0.078125f,
    };

    /** @brief Predefined material: Red Rubber - A red rubber material with low shininess */
    static constexpr LightingProps RedRubber = {
        .Ambient = glm::vec4(0.05f, 0.0f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.5f, 0.4f, 0.4f, 1.0f),
        .Specular = glm::vec4(0.7f, 0.04f, 0.04f, 1.0f),
        .Shininess = 0.078125f,
    };

    /** @brief Predefined material: White Rubber - A white rubber material with low shininess */
    static constexpr LightingProps WhiteRubber = {
        .Ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f),
        .Diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
        .Specular = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
        .Shininess = 0.078125f,
    };

    /** @brief Predefined material: Yellow Rubber - A yellow rubber material with low shininess */
    static constexpr LightingProps YellowRubber = {
        .Ambient = glm::vec4(0.05f, 0.05f, 0.0f, 1.0f),
        .Diffuse = glm::vec4(0.5f, 0.5f, 0.4f, 1.0f),
        .Specular = glm::vec4(0.7f, 0.7f, 0.04f, 1.0f),
        .Shininess = 0.078125f,
    };

} // namespace Vulkyrie
