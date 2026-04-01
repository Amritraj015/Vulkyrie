#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Physics {

    class AABB {
        public:
            AABB(const glm::vec3 &minCoordinates, const glm::vec3 &maxCoordinates)
                : _minCoordinates(minCoordinates)
                , _maxCoordinates(maxCoordinates) {
            }

            [[nodiscard]] glm::vec3 GetCenter() const {
                return (_minCoordinates + _maxCoordinates) * 0.5f;
            }

            void Encapsulate(const glm::vec3 &point) {
                _minCoordinates = glm::min(_minCoordinates, point);
                _maxCoordinates = glm::max(_maxCoordinates, point);
            }

            void Inflate(const glm::vec3 &inflation) {
                _minCoordinates -= inflation;
                _maxCoordinates += inflation;
            }

            [[nodiscard]] const glm::vec3 &GetMin() const {
                return _minCoordinates;
            }

            void SetMin(const glm::vec3 &min) {
                _minCoordinates = min;
            }

            [[nodiscard]] const glm::vec3 &GetMax() const {
                return _maxCoordinates;
            }

            void SetMax(const glm::vec3 &max) {
                _maxCoordinates = max;
            }

        private:
            glm::vec3 _minCoordinates;
            glm::vec3 _maxCoordinates;
    };

} // namespace Vulkyrie::Physics
