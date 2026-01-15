#include "core/noise_generator.h"
#include "glm/gtc/noise.hpp"

namespace Vulkyrie::Core {
    std::vector<f32> GenerateNoiseMap(const NoiseMapSpecification &specification) {
        const size_t width = specification.MapWidth;
        const size_t height = specification.MapHeight;
        const i32 octaves = specification.Octaves;
        const f32 persistence = specification.Persistence;
        const f32 lacunarity = specification.Lacunarity;
        const f32 seedOffset = static_cast<f32>(specification.Seed);

        std::vector<f32> noiseMap(width * height);

        const f32 invScale = 1.0f / std::max(specification.Scale, 0.0001f);

        // Generate the noise map using Perlin noise.
        for (size_t y = 0; y < height; ++y) {
            const size_t row = y * width;
            const f32 yCoord = static_cast<f32>(y) * invScale;

            for (size_t x = 0; x < width; ++x) {
                const f32 xCoord = static_cast<f32>(x) * invScale;
                f32 amplitude = 1.0f;
                f32 frequency = 1.0f;
                f32 noiseHeight = 0.0f;

                // Combine multiple octaves of Perlin noise.
                for (i32 o = 0; o < octaves; ++o) {
                    // Perlin noise returns values in the range [-1, 1], so we scale it accordingly.
                    const f32 perlinValue = glm::perlin(glm::vec2(
                        (xCoord * frequency) + seedOffset, 
                        (yCoord * frequency) + seedOffset
                    )) * 2.0f - 1.0f;
                    
                    noiseHeight += perlinValue * amplitude;

                    // Update amplitude and frequency for the next octave.
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                // Store the noise height in the noise map.
                noiseMap[row + x] = noiseHeight;
            }
        }

        // Normalize the noise map to the range [0, 1].
        const auto [minIt, maxIt] = std::minmax_element(noiseMap.begin(), noiseMap.end());
        const f32 minNoiseHeight = *minIt;
        const f32 range = *maxIt - minNoiseHeight;
        
        if (range > 0.0f) {
            for (f32 &value : noiseMap) {
                value = (value - minNoiseHeight) / range;
            }
        }

        return noiseMap;
    }

} // namespace Vulkyrie::Core
