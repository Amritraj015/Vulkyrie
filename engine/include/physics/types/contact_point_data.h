#pragma once

namespace Vulkyrie {

    /** @brief Struct representing the data for a contact point between two colliding shapes. This struct is used to store information about the contact normal,
     * contact points on each body, and penetration depth for a single contact point generated during collision detection. */
    struct ContactPointData final {
        public:
            /** @brief The normalized contact normal vector at the contact point, represented in world space.
             * This vector points from the first body towards the second body and is used to determine
             * the direction of the collision response. */
            glm::vec3 WorldSpaceContactNormal;

            /** @brief The contact point on the first body, represented in the local space of the first body. */
            glm::vec3 LocalSpaceContactPointOnBodyOne;

            /** @brief The contact point on the second body, represented in the local space of the second body. */
            glm::vec3 LocalSpaceContactPointOnBodyTwo;

            /** @brief The penetration depth of the contact point, representing
             * how much the two shapes are interpenetrating at this contact point. */
            f32 PenetrationDepth;
    };

} // namespace Vulkyrie
