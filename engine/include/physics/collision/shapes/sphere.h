#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Physics {

    class Sphere {
        public:
            Sphere(f32 radius, u32 stacks, u32 sectors)
                : _radius(radius)
                , _stacks(stacks)
                , _sectors(sectors) {

                CreateSphere(radius, stacks, sectors);
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

            void CreateSphere(f32 radius, u32 stacks, u32 sectors) {
                f32 pi = std::numbers::pi;

                // Each vertex has 8 attributes (position, normal, texCoords)
                _vertices.reserve((stacks + 1) * (sectors + 1) * 8);

                for (u32 i = 0; i <= stacks; ++i) {
                    f32 v = (f32)i / stacks;
                    f32 phi = v * pi;

                    for (u32 j = 0; j <= sectors; ++j) {
                        f32 u = (f32)j / sectors;
                        f32 theta = u * 2 * pi;

                        f32 x = radius * sin(phi) * cos(theta);
                        f32 y = radius * cos(phi);
                        f32 z = radius * sin(phi) * sin(theta);

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
                        u32 first = i * (sectors + 1) + j;
                        u32 second = first + sectors + 1;

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

} // namespace Vulkyrie::Physics
