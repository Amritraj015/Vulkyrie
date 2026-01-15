#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Core {
    struct NoiseMapSpecification {
        public:
            size_t MapWidth;
            size_t MapHeight;
            f32 Scale;
            i32 Octaves;
            f32 Persistence;
            f32 Lacunarity;
            u32 Seed;
    };

    std::vector<f32> GenerateNoiseMap(const NoiseMapSpecification &specification);
} // namespace Vulkyrie::Core
