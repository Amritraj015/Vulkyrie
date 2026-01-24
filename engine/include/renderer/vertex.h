#pragma once

#include "renderer/buffer_layout.h"

namespace Vulkyrie::Renderer {

    static const BufferLayout VERTEX_LAYOUT({
        { ShaderDataType::Float3, "aPos" },
        { ShaderDataType::Float3, "aNormal" },
        { ShaderDataType::Float2, "aTexCoords" },
        { ShaderDataType::Float3, "aTangent" },
        { ShaderDataType::Float3, "aBitangent" },
    });

    static const BufferLayout WEIGHTED_VERTEX_LAYOUT({
        { ShaderDataType::Float3, "aPos" },
        { ShaderDataType::Float3, "aNormal" },
        { ShaderDataType::Float2, "aTexCoords" },
        { ShaderDataType::Float3, "aTangent" },
        { ShaderDataType::Float3, "aBitangent" },
        { ShaderDataType::Int4, "aBoneIDs" },
        { ShaderDataType::Float4, "aWeights" },
    });

    /** @brief Represents a vertex in 3D space with position, normal, texture coordinates, tangent, and bitangent. */
    struct Vertex {
        public:
            /** @brief Position of the vertex in 3D space. */
            glm::vec3 Position = glm::vec3(0.0f);

            /** @brief Normal vector at the vertex for lighting calculations. */
            glm::vec3 Normal = glm::vec3(0.0f);

            /** @brief Texture coordinates for mapping 2D textures onto the vertex. */
            glm::vec2 TextureCoords = glm::vec2(0.0f);

            /** @brief Tangent vector for normal mapping. */
            glm::vec3 Tangent = glm::vec3(0.0f);

            /** @brief Bitangent vector for normal mapping. */
            glm::vec3 Bitangent = glm::vec3(0.0f);

            /** @brief Gets the buffer layout for the vertex structure.
             * @returns The buffer layout.
             */
            [[nodiscard]] inline static const BufferLayout &GetLayout() {
                return VERTEX_LAYOUT;
            }
    };

    /** @brief Represents a vertex influenced by multiple bones for skeletal animation. */
    struct WeightedVertex : public Vertex {
        public:
            /** @brief IDs of the bones influencing the vertex. */
            glm::ivec4 BoneIDs = glm::ivec4(0);

            /** @brief Weights corresponding to the influence of each bone on the vertex. */
            glm::vec4 Weights = glm::vec4(0.0f);

            /** @brief Gets the buffer layout for the weighted vertex structure.
             * @returns The buffer layout.
             */
            [[nodiscard]] inline static const BufferLayout &GetLayout() {
                return WEIGHTED_VERTEX_LAYOUT;
            }
    };
} // namespace Vulkyrie::Renderer
