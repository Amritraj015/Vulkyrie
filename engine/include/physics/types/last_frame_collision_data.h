#pragma once

namespace Vulkyrie {

    struct LastFrameCollisionData final {
        glm::vec3 GJKSeparatingAxis;

        bool IsValid;
        bool IsObsolete;
        bool WasColliding;
        bool WasUsingGJKAlgorithm;
        bool WasUsingSATAlgorithm;
        bool SATIsAxisFacePolyhedronOne;
        bool SATIsAxisFacePolyhedronTwo;
        u8 SATMinAxisFaceIndex;
        u8 SATMinEdgeOneIndex;
        u8 SATMinEdgeTwoIndex;

        LastFrameCollisionData()
            : GJKSeparatingAxis(glm::vec3(0, 1, 0))
            , IsValid(false)
            , IsObsolete(false)
            , WasColliding(false)
            , WasUsingGJKAlgorithm(false)
            , WasUsingSATAlgorithm(false)
            , SATIsAxisFacePolyhedronOne(false)
            , SATIsAxisFacePolyhedronTwo(false)
            , SATMinAxisFaceIndex(0)
            , SATMinEdgeOneIndex(0)
            , SATMinEdgeTwoIndex(0) {
        }
    };

} // namespace Vulkyrie
