#pragma once

#include "physics/types/contact_point_data.h"

namespace Vulkyrie {

    /** @brief The ContactPoint class represents a single contact point generated during collision detection between two shapes. It stores information about the
     * contact normal, contact points on each body, penetration depth, and other properties related to the contact. */
    class ContactPoint final {
    public:
        /** @brief Constructs a ContactPoint instance using the provided ContactPointData.
         * @param contactPointData The data used to initialize the ContactPoint. */
        explicit ContactPoint(const ContactPointData &contactPointData);

        // Delete the copy constructor and copy assignment operator to prevent copying of ContactPoint instances,
        ContactPoint(const ContactPoint &) = delete;
        ContactPoint &operator=(const ContactPoint &) = delete;

        // Delete the move constructor and move assignment operator to prevent moving of ContactPoint instances,
        ContactPoint(ContactPoint &&) = delete;
        ContactPoint &operator=(ContactPoint &&) = delete;

        /** @brief Default destructor for ContactPoint. */
        ~ContactPoint() = default;

        /** @brief Retrieves the contact normal at this contact point, represented in world space. The contact normal is a normalized vector that points
         * from the first body towards the second body at the point of contact, and it is used to determine the direction of the collision response.
         * @return The contact normal at this contact point, represented in world space. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetWorldSpaceContactNormal() const {
            return _worldSpaceContactNormal;
        }

        /** @brief Retrieves the contact point on the first body, represented in the local space of the first body.
         * @return The contact point on the first body, represented in the local space of the first body. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLocalSpaceContactPointOnBodyOne() const {
            return _localSpaceContactPointOnBodyOne;
        }

        /** @brief Retrieves the contact point on the second body, represented in the local space of the second body.
         * @return The contact point on the second body, represented in the local space of the second body. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLocalSpaceContactPointOnBodyTwo() const {
            return _localSpaceContactPointOnBodyTwo;
        }

        /** @brief Retrieves the penetration depth for this contact point.
         * @return The penetration depth for this contact point. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetPenetrationDepth() const {
            return _penetrationDepth;
        }

        /** @brief Sets the penetration impulse for this contact point. The penetration impulse represents the corrective impulse that should be applied to
         * resolve the interpenetration between the two colliding bodies at this contact point.
         * @param penetrationDepth The penetration depth to set for this contact point. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetPenetrationImpulse() const {
            return _penetrationImpulse;
        }

        /** @brief Sets the penetration impulse for this contact point. The penetration impulse represents the corrective impulse that should be applied to
         * resolve the interpenetration between the two colliding bodies at this contact point.
         * @param penetrationImpulse The penetration impulse to set for this contact point. */
        VE_FORCE_INLINE void setPenetrationImpulse(f32 penetrationImpulse) {
            _penetrationImpulse = penetrationImpulse;
        }

        /** @brief Checks whether this contact point is a resting contact. A resting contact is a contact that has existed for multiple frames and is likely
         * to persist in future frames, which can be used to optimize collision response by treating it differently from new contacts. Resting contacts are
         * typically those that occur when two objects are in sustained contact, such as an object resting on the ground or two objects sliding against each
         * other. Identifying resting contacts can help improve the stability of the physics simulation by allowing the engine to apply different response
         * strategies for contacts that are expected to persist over time.
         * @return True if this contact point is a resting contact, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsRestingContact() const {
            return _isRestingContact;
        }

        /** @brief Sets whether this contact point is a resting contact. A resting contact is a contact that has existed for multiple frames and is likely
         * to persist in future frames, which can be used to optimize collision response by treating it differently from new contacts. Setting a contact
         * point as a resting contact can help improve the stability of the physics simulation by allowing the engine to apply different response strategies
         * for contacts that are expected to persist over time.
         * @param isRestingContact True to set this contact point as a resting contact, false otherwise. */
        VE_FORCE_INLINE void setIsRestingContact(bool isRestingContact) {
            _isRestingContact = isRestingContact;
        }

    private:
        /** @brief The contact normal at this contact point, represented in world space. The contact normal is a normalized vector that points from the
         * first body towards the second body at the point of contact, and it is used to determine the direction of the collision response. */
        glm::vec3 _worldSpaceContactNormal;

        /** @brief The contact point on the first body, represented in the local space of the first body. */
        glm::vec3 _localSpaceContactPointOnBodyOne;

        /** @brief The contact point on the second body, represented in the local space of the second body. */
        glm::vec3 _localSpaceContactPointOnBodyTwo;

        /** @brief The penetration depth for this contact point, representing how much the two shapes are interpenetrating at this contact point. */
        f32 _penetrationDepth;

        /** @brief The penetration impulse for this contact point. The penetration impulse represents the corrective impulse that should be applied to
         * resolve the interpenetration between the two colliding bodies at this contact point. */
        f32 _penetrationImpulse;

        /** @brief A flag indicating whether this contact point is a resting contact. A resting contact is a contact that has existed for multiple frames
         * and is likely to persist in future frames, which can be used to optimize collision response by treating it differently from new contacts. Resting
         * contacts are typically those that occur when two objects are in sustained contact, such as an object resting on the ground or two objects sliding
         * against each other. */
        bool _isRestingContact;

        /** @brief A flag indicating whether this contact point is obsolete and should be removed from the contact manifold. */
        // bool _isObsolete;
    };

} // namespace Vulkyrie
