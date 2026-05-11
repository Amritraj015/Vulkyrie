#include "physics/constraint/contact_point.h"
#include "core/asserts.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace Vulkyrie {

    ContactPoint::ContactPoint(const ContactPointData &contactPointData)
        : _worldSpaceContactNormal(contactPointData.WorldSpaceContactNormal)
        , _localSpaceContactPointOnBodyOne(contactPointData.LocalSpaceContactPointOnBodyOne)
        , _localSpaceContactPointOnBodyTwo(contactPointData.LocalSpaceContactPointOnBodyTwo)
        , _nextInManifold(nullptr)
        , _previousInManifold(nullptr)
        , _penetrationDepth(contactPointData.PenetrationDepth)
        , _penetrationImpulse(0.0f)
        , _isRestingContact(false)
        , _isObsolete(false) {

        VASSERT(_penetrationDepth > f32(0.0), "Penetration depth should be greater than zero for a valid contact point.");
        VASSERT(glm::length2(_worldSpaceContactNormal) == f32(1.0), "Contact normal should be a normalized vector with length of 1.");
    }

} // namespace Vulkyrie
