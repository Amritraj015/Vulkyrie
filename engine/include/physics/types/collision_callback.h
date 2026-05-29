#pragma once

#include "physics/types/contact_point.h"
#include "physics/types/contact_pair.h"
#include "physics/types/contact_manifold.h"

namespace Vulkyrie {

    class PhysicsWorld;
    class Collider;
    class Body;

    class CollisionCallback {
    public:
        CollisionCallback() = default;

        // Delete the copy constructor and copy assignment operator.
        CollisionCallback(const CollisionCallback &) = delete;
        CollisionCallback &operator=(const CollisionCallback &) = delete;

        // Delete the move constructor and move assignment operator.
        CollisionCallback(CollisionCallback &&) = delete;
        CollisionCallback &operator=(CollisionCallback &&) = delete;

        /** @brief Default destructor for CollisionCallback. */
        virtual ~CollisionCallback() = default;

        class ContactPair {
        public:
            /** @brief Enumeration representing the type of collision event for a contact pair.
             * This enum is used to classify the state of the contact pair in terms of its collision status across simulation frames. */
            enum class EventType : i32 {
                /** @brief Indicates that the contact pair has just started colliding in the current frame, but was not colliding in the previous frame. This
                 * event is typically used to trigger collision enter events or to perform initialization logic when a new collision occurs. */
                ContactStart,

                /** @brief Indicates that the contact pair is still colliding in the current frame and was also colliding in the previous frame. This event is
                 * typically used to trigger collision stay events or to perform continuous logic while a collision is ongoing. */
                ContactStay,

                /** @brief Indicates that the contact pair has stopped colliding in the current frame, but was colliding in the previous frame. This event is
                 * typically used to trigger collision exit events or to perform cleanup logic when a collision ends. */
                ContactExit,
            };

        private:
            /** @brief Reference to the contact pair data from the collision system. */
            const Vulkyrie::ContactPair &_contactPair;

            /** @brief Reference to the contact manifolds associated with this contact pair. The manifolds contain detailed information about the contact points
             * and their properties, such as contact normals, penetration depths, and friction coefficients. This reference allows the callback to access the
             * contact manifolds for this pair and perform any necessary processing or event dispatch based on the collision data. */
            std::vector<Vulkyrie::ContactPoint> &_contactPoints;

            /** @brief Reference to the physics world that contains this contact pair. */
            PhysicsWorld &_physicsWorld;

            /** @brief True if this is a lost contact pair (contact pair colliding in previous frame but not in current one). */
            bool _isLostContactPair;

        public:
            /** @brief Constructs a ContactPair instance using the provided contact pair data, contact points, physics world reference, and lost contact pair
             * status.
             * @param contactPair The contact pair data from the collision system that contains information about the colliding bodies, colliders, and contact
             * points.
             * @param contactPoints A reference to the vector of contact points associated with this contact pair, which contains detailed information about
             * each contact point generated during collision detection.
             * @param world A reference to the physics world that contains this contact pair, allowing access to other entities and components in the world as
             * needed for processing collision events.
             * @param isLostContactPair A boolean indicating whether this contact pair is a lost contact pair (i.e., it was colliding in the previous frame but
             * is no longer colliding in the current frame). */
            ContactPair(const Vulkyrie::ContactPair &contactPair,
                        std::vector<Vulkyrie::ContactPoint> &contactPoints,
                        PhysicsWorld &world,
                        bool isLostContactPair);

            // Delete the copy constructor and copy assignment operator.
            ContactPair(const ContactPair &contactPair) = default;
            ContactPair &operator=(const ContactPair &contactPair) = delete;

            // Delete the move constructor and move assignment operator.
            ContactPair(ContactPair &&) = delete;
            ContactPair &operator=(ContactPair &&) = delete;

            /** @brief Default destructor for ContactPair. */
            ~ContactPair() = default;

            /** @brief Retrieves the number of contact points generated for this contact pair. This count represents the total number of contact points across
             * all contact manifolds associated with this pair, and it can be used to iterate over the contact points and access their properties for collision
             * response or event dispatch. */
            [[nodiscard]] VE_INLINE size_t GetContactPointsCount() const {
                return _contactPair.ContactPointCount;
            }

            /** @brief Retrieves the contact point at the specified index for this contact pair.
             * @param index The index of the contact point to retrieve (must be 0 >= index < GetContactPointsCount()).
             * @returns A ContactPoint object representing the contact point at the specified index for this contact pair.
             * */
            [[nodiscard]] VE_INLINE ContactPoint GetContactPoint(size_t index) const {
                VASSERT(index < GetContactPointsCount(), "Contact point index out of bounds in CollisionCallback::ContactPair::GetContactPoint.");

                return ContactPoint(_contactPoints[_contactPair.ContactPointIndex + index]);
            }

            /** @brief Retrieves the first body involved in this contact pair.
             * @returns A reference to the first Body involved in this contact pair. */
            [[nodiscard]] Body &GetBodyOne() const;

            /** @brief Retrieves the second body involved in this contact pair.
             * @returns A reference to the second Body involved in this contact pair. */
            [[nodiscard]] Body &GetBodyTwo() const;

            /** @brief Retrieves the first collider involved in this contact pair.
             * @returns A reference to the first Collider involved in this contact pair. */
            [[nodiscard]] Collider &GetColliderOne() const;

            /** @brief Retrieves the second collider involved in this contact pair.
             * @returns A reference to the second Collider involved in this contact pair. */
            [[nodiscard]] Collider &GetColliderTwo() const;

            /** @brief Retrieves the type of collision event for this contact pair, which indicates whether the pair has just started colliding, is still
             * colliding, or has stopped colliding in the current frame compared to the previous frame.
             * @returns The EventType representing the type of collision event for this contact pair. */
            [[nodiscard]] VE_INLINE EventType GetEventType() const {
                if (_isLostContactPair)
                    return EventType::ContactExit;
                else if (_contactPair.CollidingInPreviousFrame)
                    return EventType::ContactStay;

                return EventType::ContactStart;
            }
        };

        class Data final {
        public:
            Data(std::vector<Vulkyrie::ContactPair> &contactPairs,
                 std::vector<Vulkyrie::ContactManifold> &contactManifolds,
                 std::vector<Vulkyrie::ContactPoint> &contactPoints,
                 std::vector<Vulkyrie::ContactPair> &lostContactPairs,
                 PhysicsWorld &world);

            // Delete the copy constructor and copy assignment operator.
            Data(const Data &) = delete;
            Data &operator=(const Data &) = delete;

            // Delete the move constructor and move assignment operator.
            Data(Data &&) = delete;
            Data &operator=(Data &&) = delete;

            /** @brief Default destructor for Data. */
            ~Data() = default;

            /** @brief Retrieves the total number of contact pairs in the collision data, which includes both current contact pairs (pairs that are colliding in
             * the current frame) and lost contact pairs (pairs that were colliding in the previous frame but are no longer colliding in the current frame).
             * This count can be used to iterate over all contact pairs and access their details for processing collision events or performing collision
             * response. */
            [[nodiscard]] VE_INLINE size_t GetContactPairCount() const {
                return _contactPairIndices.size() + _lostContactPairIndices.size();
            }

            /** @brief Retrieves the contact pair at the specified index from the collision data. The index should be in the range of valid contact pairs, which
             * includes both current contact pairs and lost contact pairs (contact pairs that were colliding in the previous frame but are no longer colliding
             * in the current frame).
             * @param index The index of the contact pair to retrieve (must be 0 >= index < GetContactPairCount()).
             * @returns A CollisionCallback::ContactPair object representing the contact pair at the specified index from the collision data. */
            [[nodiscard]] CollisionCallback::ContactPair GetContactPair(size_t index) const;

        private:
            std::vector<Vulkyrie::ContactPair> &_contactPairs;
            std::vector<Vulkyrie::ContactManifold> &_contactManifolds;
            std::vector<Vulkyrie::ContactPoint> &_contactPoints;
            std::vector<Vulkyrie::ContactPair> &_lostContactPairs;
            std::vector<size_t> _contactPairIndices;
            std::vector<size_t> _lostContactPairIndices;
            PhysicsWorld &_physicsWorld;
        };

        virtual void OnCollision(const Data &collisionData) = 0;
    };

} // namespace Vulkyrie
