#pragma once

#include "physics/components/component_store.h"

namespace Vulkyrie {

    class RigidBody;

    /** @brief Body type classification used by rigid bodies. Mirrors the runtime Body semantics. */
    enum class BodyType : i32 { STATIC, KINEMATIC, DYNAMIC };

    /** @brief Plain-data component passed to `RigidBodyComponentStore::AddComponent`.
     *
     * All fields are copied into the store's parallel arrays on insertion; callers may construct
     * this struct on the stack and pass by const reference. Note: `WorldPosition` is stored by
     * value (not by reference) to avoid dangling references from temporaries. */
    struct RigidBodyComponent final {
    public:
        /** @brief Non-owning pointer to the RigidBody instance associated with the entity. */
        RigidBody *Body;

        /** @brief Initial body type (static / kinematic / dynamic). */
        BodyType Type;

        /** @brief World-space position of the body's reference point at time of construction. */
        glm::vec3 WorldPosition;

        /** @brief Constructs a RigidBodyComponent with the specified parameters.
         * @param body Non-owning pointer to the RigidBody instance associated with the entity.
         * @param type Initial body type (static / kinematic / dynamic).
         * @param worldPosition World-space position of the body's reference point at time of construction. */
        RigidBodyComponent(RigidBody *body, BodyType type, const glm::vec3 &worldPosition)
            : Body(body)
            , Type(type)
            , WorldPosition(worldPosition) {
        }
    };

    /** @brief Stores rigid body components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that participates as a rigid body in the physics simulation owns exactly one RigidBodyComponent, which
     * binds it to a RigidBody object and tracks all associated physics properties such as mass, velocities, forces, damping,
     * inertia tensors, and constraint solver state. The store maintains the dense active-zone invariant inherited from
     * ComponentStore: active components occupy indices [0, _activeCount) and inactive components occupy [_activeCount, size).
     * Swap operations keep all parallel arrays in sync with _entities at all times, enabling efficient iteration over active
     * bodies during physics updates. */
    class RigidBodyComponentStore final : public ComponentStore {
    public:
        /** @brief Constructs an instance of RigidBodyComponentStore. */
        RigidBodyComponentStore();

        // Delete the copy constructor and copy assignment operator.
        RigidBodyComponentStore(const RigidBodyComponentStore &) = delete;
        RigidBodyComponentStore &operator=(const RigidBodyComponentStore &) = delete;

        // Delete the move constructor and move assignment operator.
        RigidBodyComponentStore(RigidBodyComponentStore &&) = delete;
        RigidBodyComponentStore &operator=(RigidBodyComponentStore &&) = delete;

        /** @brief Destructor for RigidBodyComponentStore. */
        ~RigidBodyComponentStore() override = default;

        /** @brief Adds a RigidBodyComponent to the specified entity. Active components are stored at the front of the
         * vector and inactive ones at the back to maintain dense packing for efficient iteration.
         * @param entity The entity to which the RigidBodyComponent will be added. Must not already have a RigidBodyComponent.
         * @param component The RigidBodyComponent to be added to the entity.
         * @param active Whether the entity is currently active. */
        void AddComponent(Entity entity, const RigidBodyComponent &component, bool active);

        /** @brief Retrieves a reference to the RigidBody associated with the specified body entity. The RigidBody represents
         * the runtime physics state and behavior of the entity in the physics simulation. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose RigidBody is to be retrieved.
         * @returns A reference to the RigidBody associated with the specified entity. */
        [[nodiscard]] VE_FORCE_INLINE RigidBody &GetRigidBody(Entity bodyEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return *_rigidBodies[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves a const reference to the RigidBody associated with the specified body entity. The entity must have
         * a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose RigidBody is to be retrieved.
         * @returns A const reference to the RigidBody associated with the specified entity. */
        [[nodiscard]] VE_FORCE_INLINE const RigidBody &GetRigidBody(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return *_rigidBodies[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Checks whether the body associated with the specified entity is allowed to enter a sleeping state. Bodies
         * that can sleep will be deactivated by the physics engine when they remain at rest for a sufficient period, improving
         * performance. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if the body is allowed to sleep, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool CanSleep(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return static_cast<bool>(_canSleepFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Sets whether the body associated with the specified entity is allowed to enter a sleeping state. The entity
         * must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param canSleep True to allow the body to sleep, false to prevent sleeping. */
        VE_FORCE_INLINE void SetCanSleep(Entity bodyEntity, bool canSleep) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _canSleepFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(canSleep);
        }

        /** @brief Checks whether the body associated with the specified entity is currently in a sleeping state. Sleeping
         * bodies are temporarily excluded from physics simulation until awakened by external forces or interactions. The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if the body is currently sleeping, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsSleeping(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return static_cast<bool>(_isSleepingFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Sets whether the body associated with the specified entity is currently in a sleeping state. The entity
         * must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param isSleeping True to mark the body as sleeping, false to mark it as awake. */
        VE_FORCE_INLINE void SetIsSleeping(Entity bodyEntity, bool isSleeping) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _isSleepingFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(isSleeping);
        }

        /** @brief Retrieves the amount of time (in seconds) that the body associated with the specified entity has been
         * in a resting state below the sleep threshold. The physics engine uses this value to determine when a body has been
         * stationary long enough to transition into a sleeping state. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose sleep time is to be retrieved.
         * @returns The sleep time in seconds. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetSleepTime(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _sleepTimes[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the amount of time (in seconds) that the body associated with the specified entity has been in a
         * resting state. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param sleepTime The sleep time in seconds. */
        VE_FORCE_INLINE void SetSleepTime(Entity bodyEntity, f32 sleepTime) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _sleepTimes[_entityToComponentIndex.find(bodyEntity)->second] = sleepTime;
        }

        /** @brief Retrieves the body type of the specified entity. The body type determines how the physics engine treats
         * the body during simulation: STATIC bodies are immovable, KINEMATIC bodies move according to user-defined velocities
         * without being affected by forces, and DYNAMIC bodies respond to all forces and constraints. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose body type is to be retrieved.
         * @returns The BodyType (STATIC, KINEMATIC, or DYNAMIC) of the specified entity. */
        [[nodiscard]] VE_FORCE_INLINE BodyType GetBodyType(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _bodyTypes[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the body type of the specified entity. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param bodyType The new BodyType (STATIC, KINEMATIC, or DYNAMIC) for the entity. */
        VE_FORCE_INLINE void SetBodyType(Entity bodyEntity, BodyType bodyType) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _bodyTypes[_entityToComponentIndex.find(bodyEntity)->second] = bodyType;
        }

        /** @brief Retrieves the linear velocity of the body associated with the specified entity in world space. This represents
         * the rate of change of the body's position per second. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose linear velocity is to be retrieved.
         * @returns A const reference to the linear velocity vector in world space (meters per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLinearVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _linearVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the linear velocity of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param linearVelocity The new linear velocity vector in world space (meters per second). */
        VE_FORCE_INLINE void SetLinearVelocity(Entity bodyEntity, const glm::vec3 &linearVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _linearVelocities[_entityToComponentIndex.find(bodyEntity)->second] = linearVelocity;
        }

        /** @brief Retrieves the angular velocity of the body associated with the specified entity in world space. This represents
         * the rate of change of the body's orientation per second, expressed as a rotation vector where the direction indicates
         * the axis of rotation and the magnitude indicates the rotation speed in radians per second. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose angular velocity is to be retrieved.
         * @returns A const reference to the angular velocity vector in world space (radians per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetAngularVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _angularVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the angular velocity of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param angularVelocity The new angular velocity vector in world space (radians per second). */
        VE_FORCE_INLINE void SetAngularVelocity(Entity bodyEntity, const glm::vec3 &angularVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _angularVelocities[_entityToComponentIndex.find(bodyEntity)->second] = angularVelocity;
        }

        /** @brief Retrieves the accumulated external force currently applied to the body associated with the specified entity.
         * External forces are typically reset each simulation step after being integrated into the body's velocity. The entity
         * must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose external force is to be retrieved.
         * @returns A const reference to the external force vector (Newtons). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetExternalForce(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _externalForces[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the accumulated external force applied to the body associated with the specified entity. The entity must
         * have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param externalForce The external force vector to apply (Newtons). */
        VE_FORCE_INLINE void SetExternalForce(Entity bodyEntity, const glm::vec3 &externalForce) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _externalForces[_entityToComponentIndex.find(bodyEntity)->second] = externalForce;
        }

        /** @brief Retrieves the accumulated external torque currently applied to the body associated with the specified entity.
         * External torques are typically reset each simulation step after being integrated into the body's angular velocity. The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose external torque is to be retrieved.
         * @returns A const reference to the external torque vector (Newton-meters). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetExternalTorque(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _externalTorques[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the accumulated external torque applied to the body associated with the specified entity. The entity must
         * have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param externalTorque The external torque vector to apply (Newton-meters). */
        VE_FORCE_INLINE void SetExternalTorque(Entity bodyEntity, const glm::vec3 &externalTorque) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _externalTorques[_entityToComponentIndex.find(bodyEntity)->second] = externalTorque;
        }

        /** @brief Retrieves the linear damping coefficient of the body associated with the specified entity. Linear damping
         * simulates drag or air resistance by gradually reducing the body's linear velocity over time. Higher values cause the
         * body to slow down more quickly. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose linear damping coefficient is to be retrieved.
         * @returns The linear damping coefficient (typically in the range [0, 1]). */
        [[nodiscard]] VE_FORCE_INLINE f32 GetLinearDamping(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _linearDampings[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the linear damping coefficient of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param linearDamping The new linear damping coefficient (typically in the range [0, 1]). */
        VE_FORCE_INLINE void SetLinearDamping(Entity bodyEntity, f32 linearDamping) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _linearDampings[_entityToComponentIndex.find(bodyEntity)->second] = linearDamping;
        }

        /** @brief Retrieves the angular damping coefficient of the body associated with the specified entity. Angular damping
         * simulates rotational drag by gradually reducing the body's angular velocity over time. Higher values cause the body to
         * stop spinning more quickly. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose angular damping coefficient is to be retrieved.
         * @returns The angular damping coefficient (typically in the range [0, 1]). */
        [[nodiscard]] VE_FORCE_INLINE f32 GetAngularDamping(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _angularDampings[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the angular damping coefficient of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param angularDamping The new angular damping coefficient (typically in the range [0, 1]). */
        VE_FORCE_INLINE void SetAngularDamping(Entity bodyEntity, f32 angularDamping) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _angularDampings[_entityToComponentIndex.find(bodyEntity)->second] = angularDamping;
        }

        /** @brief Retrieves the mass of the body associated with the specified entity, measured in kilograms. The mass determines
         * how the body responds to applied forces according to Newton's second law (F = ma). The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose mass is to be retrieved.
         * @returns The mass in kilograms. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetMass(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _masses[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the mass of the body associated with the specified entity. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param mass The new mass in kilograms. */
        VE_FORCE_INLINE void SetMass(Entity bodyEntity, f32 mass) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _masses[_entityToComponentIndex.find(bodyEntity)->second] = mass;
        }

        /** @brief Retrieves the inverse mass of the body associated with the specified entity. The inverse mass (1/mass) is
         * precomputed for performance and is zero for bodies with infinite mass (immovable bodies). The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose inverse mass is to be retrieved.
         * @returns The inverse mass (1/kilograms), or 0 for immovable bodies. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetInverseMass(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _inverseMasses[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the inverse mass of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param inverseMass The new inverse mass (1/kilograms). Set to 0 for immovable bodies. */
        VE_FORCE_INLINE void SetInverseMass(Entity bodyEntity, f32 inverseMass) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _inverseMasses[_entityToComponentIndex.find(bodyEntity)->second] = inverseMass;
        }

        /** @brief Retrieves the local-space inertia tensor of the body associated with the specified entity. The inertia tensor
         * determines how the body resists rotational acceleration around each principal axis. For convenience, this is stored
         * as a 3-component vector representing the diagonal elements of the inertia tensor matrix (assuming the tensor has been
         * diagonalized to its principal axes). The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose local inertia tensor is to be retrieved.
         * @returns A const reference to the local inertia tensor (kg·m²) stored as a vec3. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLocalInertiaTensor(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _localInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the local-space inertia tensor of the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param inertiaTensorLocal The new local inertia tensor (kg·m²) stored as a vec3. */
        VE_FORCE_INLINE void SetLocalInertiaTensor(Entity bodyEntity, const glm::vec3 &inertiaTensorLocal) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _localInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second] = inertiaTensorLocal;
        }

        /** @brief Retrieves the inverse of the local-space inertia tensor of the body associated with the specified entity.
         * This is precomputed for performance. Components are zero for axes with infinite rotational inertia. The entity must
         * have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose inverse local inertia tensor is to be retrieved.
         * @returns A const reference to the inverse local inertia tensor (1/(kg·m²)) stored as a vec3. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetInverseLocalInertiaTensor(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _inverseLocalInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the inverse of the world-space inertia tensor of the body associated with the specified entity.
         * This is computed by rotating the local inverse inertia tensor into world space using the body's current orientation,
         * and is updated each simulation step. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose inverse world inertia tensor is to be retrieved.
         * @returns A const reference to the inverse world inertia tensor (1/(kg·m²)) as a 3x3 matrix. */
        [[nodiscard]] VE_FORCE_INLINE const glm::mat3 &GetInverseWorldInertiaTensor(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _inverseWorldInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the inverse of the world-space inertia tensor of the body associated with the specified entity. The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param inertiaTensor The new inverse world inertia tensor (1/(kg·m²)) as a 3x3 matrix. */
        VE_FORCE_INLINE void SetInverseWorldInertiaTensor(Entity bodyEntity, const glm::mat3 &inertiaTensor) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _inverseWorldInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second] = inertiaTensor;
        }

        /** @brief Sets the inverse of the local-space inertia tensor of the body associated with the specified entity. The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param inertiaTensorLocalInverse The new inverse local inertia tensor (1/(kg·m²)) stored as a vec3. */
        VE_FORCE_INLINE void SetInverseLocalInertiaTensor(Entity bodyEntity, const glm::vec3 &inertiaTensorLocalInverse) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _inverseLocalInertiaTensors[_entityToComponentIndex.find(bodyEntity)->second] = inertiaTensorLocalInverse;
        }

        /** @brief Retrieves the constrained linear velocity of the body associated with the specified entity. This velocity is
         * updated by the constraint solver during physics simulation and may differ from the unconstrained velocity due to
         * contact, joint, and other constraint forces. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose constrained linear velocity is to be retrieved.
         * @returns A const reference to the constrained linear velocity vector (meters per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetConstrainedLinearVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _constrainedLinearVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the constrained angular velocity of the body associated with the specified entity. This velocity is
         * updated by the constraint solver during physics simulation and may differ from the unconstrained velocity due to
         * contact, joint, and other constraint forces. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose constrained angular velocity is to be retrieved.
         * @returns A const reference to the constrained angular velocity vector (radians per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetConstrainedAngularVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _constrainedAngularVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the split linear velocity of the body associated with the specified entity. Split velocities are
         * used by certain constraint solvers to separate positional correction from velocity-level solving, improving stability
         * and preventing artificial energy gain. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose split linear velocity is to be retrieved.
         * @returns A const reference to the split linear velocity vector (meters per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetSplitLinearVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _splitLinearVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the split angular velocity of the body associated with the specified entity. Split velocities are
         * used by certain constraint solvers to separate positional correction from velocity-level solving. The entity must have
         * a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose split angular velocity is to be retrieved.
         * @returns A const reference to the split angular velocity vector (radians per second). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetSplitAngularVelocity(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _splitAngularVelocities[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the constrained position of the body associated with the specified entity.
         * The constrained position may be modified by certain constraint solvers during iterative solving to correct positional
         * drift. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose constrained position is to be retrieved.
         * @returns A mutable reference to the constrained position vector. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 &GetConstrainedPosition(Entity bodyEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _constrainedPositions[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves a mutable reference to the constrained orientation quaternion of the body associated with the
         * specified entity. The constrained orientation may be modified by certain constraint solvers during iterative solving
         * to correct rotational drift. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose constrained orientation is to be retrieved.
         * @returns A mutable reference to the constrained orientation quaternion. */
        [[nodiscard]] VE_FORCE_INLINE glm::quat &GetConstrainedOrientation(Entity bodyEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _constrainedOrientations[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the center of mass of the body associated with the specified entity in local (body) space. The
         * center of mass is the point at which the body's mass is considered to be concentrated for purposes of linear motion.
         * The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose local center of mass is to be retrieved.
         * @returns A const reference to the local center of mass position vector. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLocalCenterOfMass(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _localCenterOfMasses[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the center of mass of the body associated with the specified entity in world space. This is
         * computed by transforming the local center of mass by the body's current position and orientation, and is updated each
         * simulation step. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose world center of mass is to be retrieved.
         * @returns A const reference to the world center of mass position vector. */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetWorldCenterOfMass(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _worldCenterOfMasses[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Checks whether gravity is enabled for the body associated with the specified entity. When gravity is enabled,
         * the physics engine applies the global gravity force to the body each simulation step. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if gravity is enabled for the body, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsGravityEnabled(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return static_cast<bool>(_gravityEnabledFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Checks whether the body associated with the specified entity has already been assigned to a simulation island
         * during the current constraint solving step. Islands are groups of interconnected bodies that are solved together to
         * improve performance and allow independent sleeping of disconnected body groups. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be checked.
         * @returns True if the body is already assigned to an island, false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsInIsland(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return static_cast<bool>(_isInIslandFlags[_entityToComponentIndex.find(bodyEntity)->second]);
        }

        /** @brief Retrieves the linear axis locking factors for the body associated with the specified entity. These factors
         * allow selective constraint of linear motion along individual axes: 1.0 indicates free movement along that axis, while
         * 0.0 completely locks motion. This enables effects like sliding along a rail or movement restricted to a plane. The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose linear axis locking factors are to be retrieved.
         * @returns A const reference to the linear axis locking factors (1 = free, 0 = locked). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetLinearLockAxisFactor(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _linearLockAxisFactors[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Retrieves the angular axis locking factors for the body associated with the specified entity. These factors
         * allow selective constraint of rotational motion around individual axes: 1.0 indicates free rotation around that axis,
         * while 0.0 completely locks rotation. This enables effects like a door hinge (rotation around one axis only). The
         * entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity whose angular axis locking factors are to be retrieved.
         * @returns A const reference to the angular axis locking factors (1 = free, 0 = locked). */
        [[nodiscard]] VE_FORCE_INLINE const glm::vec3 &GetAngularLockAxisFactor(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _angularLockAxisFactors[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Sets the constrained linear velocity of the body associated with the specified entity. This is typically
         * called by the constraint solver during physics simulation. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param constrainedLinearVelocity The new constrained linear velocity vector (meters per second). */
        VE_FORCE_INLINE void SetConstrainedLinearVelocity(Entity bodyEntity, const glm::vec3 &constrainedLinearVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _constrainedLinearVelocities[_entityToComponentIndex.find(bodyEntity)->second] = constrainedLinearVelocity;
        }

        /** @brief Sets the constrained angular velocity of the body associated with the specified entity. This is typically
         * called by the constraint solver during physics simulation. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param constrainedAngularVelocity The new constrained angular velocity vector (radians per second). */
        VE_FORCE_INLINE void SetConstrainedAngularVelocity(Entity bodyEntity, const glm::vec3 &constrainedAngularVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _constrainedAngularVelocities[_entityToComponentIndex.find(bodyEntity)->second] = constrainedAngularVelocity;
        }

        /** @brief Sets the split linear velocity of the body associated with the specified entity. This is typically called by
         * constraint solvers that use velocity splitting for positional correction. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param splitLinearVelocity The new split linear velocity vector (meters per second). */
        VE_FORCE_INLINE void SetSplitLinearVelocity(Entity bodyEntity, const glm::vec3 &splitLinearVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _splitLinearVelocities[_entityToComponentIndex.find(bodyEntity)->second] = splitLinearVelocity;
        }

        /** @brief Sets the split angular velocity of the body associated with the specified entity. This is typically called by
         * constraint solvers that use velocity splitting for positional correction. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param splitAngularVelocity The new split angular velocity vector (radians per second). */
        VE_FORCE_INLINE void SetSplitAngularVelocity(Entity bodyEntity, const glm::vec3 &splitAngularVelocity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _splitAngularVelocities[_entityToComponentIndex.find(bodyEntity)->second] = splitAngularVelocity;
        }

        /** @brief Sets the constrained position of the body associated with the specified entity. This is typically modified by
         * constraint solvers during iterative solving to correct positional drift. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param constrainedPosition The new constrained position vector. */
        VE_FORCE_INLINE void SetConstrainedPosition(Entity bodyEntity, const glm::vec3 &constrainedPosition) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _constrainedPositions[_entityToComponentIndex.find(bodyEntity)->second] = constrainedPosition;
        }

        /** @brief Sets the constrained orientation of the body associated with the specified entity. This is typically modified
         * by constraint solvers during iterative solving to correct rotational drift. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param constrainedOrientation The new constrained orientation quaternion. */
        VE_FORCE_INLINE void SetConstrainedOrientation(Entity bodyEntity, const glm::quat &constrainedOrientation) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _constrainedOrientations[_entityToComponentIndex.find(bodyEntity)->second] = constrainedOrientation;
        }

        /** @brief Sets the center of mass of the body associated with the specified entity in local (body) space. The entity
         * must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param centerOfMassLocal The new local center of mass position vector. */
        VE_FORCE_INLINE void SetLocalCenterOfMass(Entity bodyEntity, const glm::vec3 &centerOfMassLocal) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _localCenterOfMasses[_entityToComponentIndex.find(bodyEntity)->second] = centerOfMassLocal;
        }

        /** @brief Sets the center of mass of the body associated with the specified entity in world space. This is typically
         * recomputed each simulation step by transforming the local center of mass. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity to be updated.
         * @param centerOfMassWorld The new world center of mass position vector. */
        VE_FORCE_INLINE void SetWorldCenterOfMass(Entity bodyEntity, const glm::vec3 &centerOfMassWorld) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _worldCenterOfMasses[_entityToComponentIndex.find(bodyEntity)->second] = centerOfMassWorld;
        }

        /** @brief Sets whether gravity is enabled for the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param isGravityEnabled True to enable gravity for the body, false to disable it. */
        VE_FORCE_INLINE void SetGravityEnabled(Entity bodyEntity, bool isGravityEnabled) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _gravityEnabledFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(isGravityEnabled);
        }

        /** @brief Sets whether the body associated with the specified entity has been assigned to a simulation island. This is
         * typically updated during the constraint solving phase. The entity must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param isInIsland True to mark the body as assigned to an island, false otherwise. */
        VE_FORCE_INLINE void SetInIsland(Entity bodyEntity, bool isInIsland) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _isInIslandFlags[_entityToComponentIndex.find(bodyEntity)->second] = static_cast<u8>(isInIsland);
        }

        /** @brief Sets the linear axis locking factors for the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param linearLockAxisFactor The new linear axis locking factors (1 = free, 0 = locked). */
        VE_FORCE_INLINE void SetLinearLockAxisFactor(Entity bodyEntity, const glm::vec3 &linearLockAxisFactor) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _linearLockAxisFactors[_entityToComponentIndex.find(bodyEntity)->second] = linearLockAxisFactor;
        }

        /** @brief Sets the angular axis locking factors for the body associated with the specified entity. The entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity to be updated.
         * @param rotationTranslationFactor The new angular axis locking factors (1 = free, 0 = locked). */
        VE_FORCE_INLINE void SetAngularLockAxisFactor(Entity bodyEntity, const glm::vec3 &rotationTranslationFactor) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _angularLockAxisFactors[_entityToComponentIndex.find(bodyEntity)->second] = rotationTranslationFactor;
        }

        /** @brief Retrieves the list of joint entities attached to the specified body entity. Joints constrain the motion of two
         * bodies relative to each other, such as hinges, sliders, or fixed connections. The entity must have a RigidBodyComponent
         * associated with it.
         * @param bodyEntity The entity whose joint list is to be retrieved.
         * @returns A const reference to the vector of joint entities attached to the specified body. */
        [[nodiscard]] VE_FORCE_INLINE const std::vector<Entity> &GetJoints(Entity bodyEntity) const {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            return _joints[_entityToComponentIndex.find(bodyEntity)->second];
        }

        /** @brief Appends the specified joint entity to the joint list of the given body entity. The body entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity that owns the body.
         * @param jointEntity The joint entity to associate with this body. */
        VE_FORCE_INLINE void AddJointToBody(Entity bodyEntity, Entity jointEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _joints[_entityToComponentIndex.find(bodyEntity)->second].push_back(jointEntity);
        }

        /** @brief Removes the specified joint entity from the joint list of the given body entity. Uses swap-erase so joint
         * order within the list is not preserved. The body entity must have a RigidBodyComponent associated with it, and the
         * joint entity must exist in the body's joint list.
         * @param bodyEntity The entity that owns the body.
         * @param jointEntity The joint entity to remove. */
        VE_FORCE_INLINE void RemoveJointFromBody(Entity bodyEntity, Entity jointEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            std::vector<Entity> &joints = _joints[_entityToComponentIndex.find(bodyEntity)->second];
            auto it = std::find(joints.begin(), joints.end(), jointEntity);

            if (it != joints.end()) {
                *it = joints.back();
                joints.pop_back();
            } else {
                VASSERT(false, "Joint entity not found in body's joint list.");
            }
        }

        /** @brief Appends the specified contact pair index to the contact pair list of the given body entity. Contact pairs
         * represent active collisions between two bodies that require constraint solving. The body entity must have a
         * RigidBodyComponent associated with it.
         * @param bodyEntity The entity that owns the body.
         * @param contactPairIndex The index of the contact pair to add. */
        VE_FORCE_INLINE void AddContactPair(Entity bodyEntity, u32 contactPairIndex) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _contactPairs[_entityToComponentIndex.find(bodyEntity)->second].push_back(contactPairIndex);
        }

        /** @brief Removes all contact pair indices from the contact pair list of the given body entity. This is typically called
         * at the beginning of each simulation step to clear stale contact information before collision detection. The body entity
         * must have a RigidBodyComponent associated with it.
         * @param bodyEntity The entity that owns the body. */
        VE_FORCE_INLINE void RemoveAllContactPairs(Entity bodyEntity) {
            VASSERT(HasComponent(bodyEntity), "Entity does not have a RigidBodyComponent.");

            _contactPairs[_entityToComponentIndex.find(bodyEntity)->second].clear();
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Parallel array of RigidBody pointers (non-owning). */
        std::vector<RigidBody *> _rigidBodies;

        /** @brief Parallel array of flags indicating whether each body can enter a sleeping state. */
        std::vector<u8> _canSleepFlags;

        /** @brief Parallel array of flags indicating whether each body is currently sleeping. */
        std::vector<u8> _isSleepingFlags;

        /** @brief Parallel array of sleep timers tracking how long each body has been at rest (seconds). */
        std::vector<f32> _sleepTimes;

        /** @brief Parallel array of body types (STATIC, KINEMATIC, or DYNAMIC). */
        std::vector<BodyType> _bodyTypes;

        /** @brief Parallel array of linear velocities in world space (m/s). */
        std::vector<glm::vec3> _linearVelocities;

        /** @brief Parallel array of angular velocities in world space (rad/s). */
        std::vector<glm::vec3> _angularVelocities;

        /** @brief Parallel array of accumulated external forces (N). */
        std::vector<glm::vec3> _externalForces;

        /** @brief Parallel array of accumulated external torques (N·m). */
        std::vector<glm::vec3> _externalTorques;

        /** @brief Parallel array of linear damping coefficients. */
        std::vector<f32> _linearDampings;

        /** @brief Parallel array of angular damping coefficients. */
        std::vector<f32> _angularDampings;

        /** @brief Parallel array of body masses (kg). */
        std::vector<f32> _masses;

        /** @brief Parallel array of inverse masses (1/kg), zero for immovable bodies. */
        std::vector<f32> _inverseMasses;

        /** @brief Parallel array of local-space inertia tensors stored as diagonal vectors (kg·m²). */
        std::vector<glm::vec3> _localInertiaTensors;

        /** @brief Parallel array of inverse local-space inertia tensors stored as diagonal vectors (1/(kg·m²)). */
        std::vector<glm::vec3> _inverseLocalInertiaTensors;

        /** @brief Parallel array of inverse world-space inertia tensors as 3x3 matrices (1/(kg·m²)). */
        std::vector<glm::mat3> _inverseWorldInertiaTensors;

        /** @brief Parallel array of constraint-solver-modified linear velocities (m/s). */
        std::vector<glm::vec3> _constrainedLinearVelocities;

        /** @brief Parallel array of constraint-solver-modified angular velocities (rad/s). */
        std::vector<glm::vec3> _constrainedAngularVelocities;

        /** @brief Parallel array of split linear velocities for positional correction (m/s). */
        std::vector<glm::vec3> _splitLinearVelocities;

        /** @brief Parallel array of split angular velocities for positional correction (rad/s). */
        std::vector<glm::vec3> _splitAngularVelocities;

        /** @brief Parallel array of constraint-solver-modified positions. */
        std::vector<glm::vec3> _constrainedPositions;

        /** @brief Parallel array of constraint-solver-modified orientation quaternions. */
        std::vector<glm::quat> _constrainedOrientations;

        /** @brief Parallel array of centers of mass in local (body) space. */
        std::vector<glm::vec3> _localCenterOfMasses;

        /** @brief Parallel array of centers of mass in world space. */
        std::vector<glm::vec3> _worldCenterOfMasses;

        /** @brief Parallel array of flags indicating whether gravity is enabled for each body. */
        std::vector<u8> _gravityEnabledFlags;

        /** @brief Parallel array of flags indicating whether each body has been assigned to a simulation island. */
        std::vector<u8> _isInIslandFlags;

        /** @brief Parallel array of joint entity lists, one per body. */
        std::vector<std::vector<Entity>> _joints;

        /** @brief Parallel array of contact pair index lists, one per body. */
        std::vector<std::vector<u32>> _contactPairs;

        /** @brief Parallel array of linear axis locking factors (1 = free, 0 = locked). */
        std::vector<glm::vec3> _linearLockAxisFactors;

        /** @brief Parallel array of angular axis locking factors (1 = free, 0 = locked). */
        std::vector<glm::vec3> _angularLockAxisFactors;
    };

} // namespace Vulkyrie
