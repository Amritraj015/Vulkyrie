#include "core/noise_generator.h"
#include "glm/gtc/noise.hpp"

namespace Vulkyrie::Core {
    std::vector<f32> GeneratePerlinNoiseMap(const NoiseMapSpecification &specification) {
        // Extract specification parameters.
        const size_t width = specification.MapWidth;
        const size_t height = specification.MapHeight;
        const i32 octaves = specification.Octaves;
        const f32 persistence = specification.Persistence;
        const f32 lacunarity = specification.Lacunarity;
        const f32 seedOffset = static_cast<f32>(specification.Seed);
        f32 maxNoiseHeight = std::numeric_limits<f32>::lowest();
        f32 minNoiseHeight = std::numeric_limits<f32>::max();

        // Allocate the noise map.
        std::vector<f32> noiseMap(width * height);

        // Pre-compute inverse scale for faster multiplication (division is slower than multiplication).
        const f32 invScale = 1.0f / std::max(specification.Scale, 0.0001f);

        // Generate the noise map using Perlin noise.
        for (size_t y = 0; y < height; ++y) {
            const size_t row = y * width;
            // Calculate scaled y-coordinate once per row for performance.
            const f32 yCoord = static_cast<f32>(y) * invScale;

            for (size_t x = 0; x < width; ++x) {
                // Calculate scaled x-coordinate.
                const f32 xCoord = static_cast<f32>(x) * invScale;
                f32 amplitude = 1.0f;
                f32 frequency = 1.0f;
                f32 noiseHeight = 0.0f;

                // Combine multiple octaves of Perlin noise for more natural-looking terrain.
                for (i32 o = 0; o < octaves; ++o) {
                    // Perlin noise returns values in the range [-1, 1], so we scale it accordingly.
                    const f32 perlinValue = glm::perlin(glm::vec2((xCoord * frequency) + seedOffset, (yCoord * frequency) + seedOffset)) * 2.0f - 1.0f;

                    noiseHeight += perlinValue * amplitude;

                    // Update amplitude and frequency for the next octave.
                    // Higher frequencies add finer details, lower amplitudes reduce their influence.
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                // Track the maximum and minimum noise heights for normalization.
                maxNoiseHeight = std::max(maxNoiseHeight, noiseHeight);
                minNoiseHeight = std::min(minNoiseHeight, noiseHeight);

                // Store the noise height in the noise map.
                noiseMap[row + x] = noiseHeight;
            }
        }

        // Normalize the noise map to the range [0, 1].
        const f32 range = maxNoiseHeight - minNoiseHeight;

        // Only normalize if there's variation in the data (avoid division by zero).
        if (range > 0.0f) {
            for (f32 &value : noiseMap) {
                value = (value - minNoiseHeight) / range;
            }
        }

        return noiseMap;
    }

} // namespace Vulkyrie::Core
