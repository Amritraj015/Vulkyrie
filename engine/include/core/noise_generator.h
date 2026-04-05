#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief Specification for generating a Perlin noise map. */
    struct NoiseMapSpecification {
        public:
            /** @brief Width of the noise map. */
            size_t MapWidth;

            /** @brief Height of the noise map. */
            size_t MapHeight;

            /** @brief Scale of the noise. Higher values zoom out, lower values zoom in. */
            f32 Scale;

            /** @brief Number of octaves to combine for the noise generation. */
            i32 Octaves;

            /** @brief Persistence value for the noise generation. */
            f32 Persistence;

            /** @brief Lacunarity value for the noise generation. */
            f32 Lacunarity;

            /** @brief Seed value for the noise generation. */
            u32 Seed;

            /** @brief Offset for the noise map generation. */
            glm::vec2 Offset;
    };

    /** @brief Generates a Perlin noise map based on the provided specification.
     * @param specification The specifications for generating the noise map.
     * @return A vector containing the generated Perlin noise values.
     */
    std::vector<f32> GeneratePerlinNoiseMap(const NoiseMapSpecification &specification);
} // namespace Vulkyrie
