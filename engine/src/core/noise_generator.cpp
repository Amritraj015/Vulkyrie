#include "core/noise_generator.h"
#include "glm/gtc/noise.hpp"

namespace Vulkyrie::Core {
    std::vector<f32> GeneratePerlinNoiseMap(const NoiseMapSpecification &specification) {
        // Extract parameters
        const size_t width = specification.MapWidth;
        const size_t height = specification.MapHeight;
        const i32 rawOctaves = specification.Octaves;
        const size_t octaves = static_cast<size_t>(std::max(1, rawOctaves));
        const f32 persistence = std::clamp(specification.Persistence, 0.0f, 1.0f);
        const f32 lacunarity = std::max(specification.Lacunarity, 1.0f);
        const f32 halfWidth = static_cast<f32>(width) / 2.0f;
        const f32 halfHeight = static_cast<f32>(height) / 2.0f;
        const f32 invScale = 1.0f / std::max(specification.Scale, 0.0001f);

        f32 maxNoiseHeight = std::numeric_limits<f32>::lowest();
        f32 minNoiseHeight = std::numeric_limits<f32>::max();

        // Allocate noise map
        std::vector<f32> noiseMap(width * height);

        // Random octave offsets per seed
        std::mt19937 prng(specification.Seed);
        std::uniform_real_distribution<f32> dist(-100000.0f, 100000.0f);
        std::vector<glm::vec2> octaveOffsets(octaves);
        for (size_t i = 0; i < octaves; ++i) {
            octaveOffsets[i] = glm::vec2(dist(prng), dist(prng));
        }

        // Generate noise
        for (size_t y = 0; y < height; ++y) {
            const size_t row = y * width;

            for (size_t x = 0; x < width; ++x) {
                f32 amplitude = 1.0f;
                f32 frequency = 1.0f;
                f32 noiseHeight = 0.0f;

                for (size_t i = 0; i < octaves; ++i) {
                    const f32 sampleX = (static_cast<f32>(x) - halfWidth + octaveOffsets[i].x) * invScale * frequency;
                    const f32 sampleY = (static_cast<f32>(y) - halfHeight + octaveOffsets[i].y) * invScale * frequency;

                    // glm::perlin returns [-1, 1], no extra scaling needed
                    const f32 perlinValue = glm::perlin(glm::vec2(sampleX, sampleY));

                    noiseHeight += perlinValue * amplitude;

                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                minNoiseHeight = std::min(minNoiseHeight, noiseHeight);
                maxNoiseHeight = std::max(maxNoiseHeight, noiseHeight);

                noiseMap[row + x] = noiseHeight;
            }
        }

        // Normalize to [0,1] if range > 0
        const f32 range = maxNoiseHeight - minNoiseHeight;
        if (range > 0.0f) {
            for (f32 &value : noiseMap) {
                value = (value - minNoiseHeight) / range;
            }
        }

        return noiseMap;
    }

} // namespace Vulkyrie::Core
