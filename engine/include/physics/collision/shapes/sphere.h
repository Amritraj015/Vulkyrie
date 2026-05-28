#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    class Sphere final {
    public:
        Sphere(f32 radius, u32 stacks, u32 sectors)
            : _radius(radius)
            , _stacks(stacks)
            , _sectors(sectors) {

            createSphere(radius, stacks, sectors);
        }

        [[nodiscard]] f32 GetRadius() const {
            return _radius;
        }

        [[nodiscard]] u32 GetStacks() const {
            return _stacks;
        }

        [[nodiscard]] u32 GetSectors() const {
            return _sectors;
        }

        [[nodiscard]] const std::vector<f32> &GetVertices() const {
            return _vertices;
        }

        [[nodiscard]] const std::vector<u32> &GetIndices() const {
            return _indices;
        }

    private:
        f32 _radius;
        u32 _stacks;
        u32 _sectors;

        std::vector<f32> _vertices;
        std::vector<u32> _indices;

        void createSphere(f32 radius, u32 stacks, u32 sectors) {
            const f32 pi = std::numbers::pi_v<f32>;
            const f32 sectorsAsFloat = static_cast<f32>(sectors);
            const f32 stacksAsFloat = static_cast<f32>(stacks);

            // Each vertex has 8 attributes (position, normal, texCoords)
            _vertices.reserve((stacks + 1) * (sectors + 1) * 8);

            for (u32 i = 0; i <= stacks; ++i) {
                const f32 v = (f32)i / stacksAsFloat;
                const f32 phi = v * pi;

                for (u32 j = 0; j <= sectors; ++j) {
                    const f32 u = (f32)j / sectorsAsFloat;
                    const f32 theta = u * 2 * pi;

                    const f32 x = radius * std::sin(phi) * std::cos(theta);
                    const f32 y = radius * std::cos(phi);
                    const f32 z = radius * std::sin(phi) * std::sin(theta);

                    _vertices.push_back(x);          // x
                    _vertices.push_back(y);          // y
                    _vertices.push_back(z);          // z
                    _vertices.push_back(x / radius); // nx
                    _vertices.push_back(y / radius); // ny
                    _vertices.push_back(z / radius); // nz
                    _vertices.push_back(u);          // u
                    _vertices.push_back(v);          // v
                }
            }

            // Each stack and sector contributes 6 indices (2 triangles)
            _indices.reserve(stacks * sectors * 6);

            for (u32 i = 0; i < stacks; ++i) {
                for (u32 j = 0; j < sectors; ++j) {
                    const u32 first = i * (sectors + 1) + j;
                    const u32 second = first + sectors + 1;

                    _indices.push_back(first);
                    _indices.push_back(second);
                    _indices.push_back(first + 1);
                    _indices.push_back(second);
                    _indices.push_back(second + 1);
                    _indices.push_back(first + 1);
                }
            }
        }
    };

} // namespace Vulkyrie
