#pragma once

namespace Vulkyrie {

    struct LastFrameCollisionInfo {
        public:
            bool IsValid;
            bool IsObsolete;
            bool WasColliding;
            bool WasUsingGJKAlgorithm;
            bool WasUsingSATAlgorithm;

            glm::vec3 GJKSeparatingAxis;

            LastFrameCollisionInfo()
                : IsValid(false)
                , IsObsolete(false)
                , WasColliding(false)
                , WasUsingGJKAlgorithm(false)
                , WasUsingSATAlgorithm(false)
                , GJKSeparatingAxis(glm::vec3(0, 1, 0)) {
                // , satIsAxisFacePolyhedron1(false)
                // , satIsAxisFacePolyhedron2(false)
                // , satMinAxisFaceIndex(0)
                // , satMinEdge1Index(0)
                // , satMinEdge2Index(0) {
            }
    };

} // namespace Vulkyrie
