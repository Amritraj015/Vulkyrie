#pragma once

#include "vlkypch.h"
#include "core/entity_manager.h"
#include "physics/components/ball_and_socket_joint_component_store.h"
#include "physics/components/fixed_joint_component_store.h"
#include "physics/components/hinge_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/slider_joint_component_store.h"
#include "physics/components/transform_component_store.h"
#include "physics/physics_context.h"
#include "physics/physics_world_settings.h"
#include "physics/systems/collision_system.h"
#include "physics/systems/constraint_solver_system.h"
#include "physics/systems/contact_solver_system.h"
#include "physics/systems/dynamics_system.h"
#include "physics/types/islands.h"
#include <glm/gtx/string_cast.hpp>

namespace Vulkyrie {

    class Joint;
    struct JointData;

    enum class ContactsPositionCorrectionTechnique : i32 {
        BaumgarteContacts,
        SplitImpulses,
    };

    class PhysicsWorld {
    public:
        explicit PhysicsWorld(const PhysicsWorldSettings &settings);

        VE_DELETE_MOVE_AND_COPY(PhysicsWorld);

        ~PhysicsWorld();

        VE_INLINE void SetWarmStartupFlag(bool enableWarmStartup) {
            _enableWarmStartup = enableWarmStartup;
        }

        [[nodiscard]] VE_INLINE const PhysicsWorldSettings &GetSettings() const {
            return _settings;
        }

        [[nodiscard]] VE_INLINE EntityManager &GetEntityManager() {
            return _entityManager;
        }

        [[nodiscard]] VE_INLINE BodyComponentStore &GetBodyComponentStore() {
            return _bodyStore;
        }

        [[nodiscard]] VE_INLINE RigidBodyComponentStore &GetRigidBodyComponentStore() {
            return _rigidBodyStore;
        }

        [[nodiscard]] VE_INLINE ColliderComponentStore &GetColliderComponentStore() {
            return _colliderStore;
        }

        [[nodiscard]] VE_INLINE TransformComponentStore &GetTransformComponentStore() {
            return _transformStore;
        }

        [[nodiscard]] VE_INLINE BallAndSocketJointComponentStore &GetBallAndSocketJointComponentStore() {
            return _basJointStore;
        }

        [[nodiscard]] VE_INLINE FixedJointComponentStore &GetFixedJointComponentStore() {
            return _fixedJointStore;
        }

        [[nodiscard]] VE_INLINE HingeJointComponentStore &GetHingeJointComponentStore() {
            return _hingeJointStore;
        }

        [[nodiscard]] VE_INLINE SliderJointComponentStore &GetSliderJointComponentStore() {
            return _sliderJointStore;
        }

        [[nodiscard]] VE_INLINE JointComponentStore &GetJointComponentStore() {
            return _jointStore;
        }

        [[nodiscard]] VE_INLINE CollisionSystem &GetCollisionSystem() {
            return _collisionSystem;
        }

        [[nodiscard]] VE_INLINE ConstraintSolverSystem &GetConstraintSolverSystem() {
            return _constraintSolverSystem;
        }

        [[nodiscard]] VE_INLINE const std::string &GetWorldName() const {
            return _settings.Name;
        }

        [[nodiscard]] VE_INLINE u16 GetVelocitySolverIterations() const {
            return _settings.VelocitySolverIterations;
        }

        VE_INLINE void SetVelocitySolverIterations(u16 iterations) {
            VTRACE("PhysicsWorld: {} - Updating VelocitySolverIterations from {} to {}.", GetWorldName(), _settings.VelocitySolverIterations, iterations);

            _settings.VelocitySolverIterations = iterations;
        }

        [[nodiscard]] VE_INLINE u16 GetPositionSolverIterations() const {
            return _settings.PositionSolverIterations;
        }

        VE_INLINE void SetPositionSolverIterations(u16 iterations) {
            VTRACE("PhysicsWorld: {} - Updating PositionSolverIterations from {} to {}.", GetWorldName(), _settings.PositionSolverIterations, iterations);

            _settings.PositionSolverIterations = iterations;
        }

        VE_INLINE void SetContactsPositionCorrectionTechnique(ContactsPositionCorrectionTechnique technique) {
            switch (technique) {
                case ContactsPositionCorrectionTechnique::BaumgarteContacts:
                    _contactSolverSystem.SetSplitImpulseActiveFlag(false);
                    break;
                case ContactsPositionCorrectionTechnique::SplitImpulses:
                    _contactSolverSystem.SetSplitImpulseActiveFlag(true);
                    break;
            }
        }

        [[nodiscard]] VE_INLINE bool IsGravityEnabled() const {
            return _gravityEnabled;
        }

        VE_INLINE void SetGravityEnabled(bool enabled) {
            VTRACE("PhysicsWorld: {} - Updating GravityEnabled from {} to {}.", GetWorldName(), _gravityEnabled, enabled);

            _gravityEnabled = enabled;
        }

        [[nodiscard]] VE_INLINE const glm::vec3 &GetGravity() const {
            return _settings.Gravity;
        }

        VE_INLINE void SetGravity(glm::vec3 gravity) {
            VTRACE("PhysicsWorld: {} - Updating Gravity from {} to {}.", GetWorldName(), glm::to_string(_settings.Gravity), glm::to_string(gravity));

            _settings.Gravity = gravity;
        }

        [[nodiscard]] VE_INLINE bool IsSleepingEnabled() const {
            return _settings.EnableSleeping;
        }

        void SetSleepingEnabled(bool enabled);

        [[nodiscard]] VE_INLINE f32 GetSleepLinearVelocity() const {
            return _settings.DefaultSleepLinearVelocity;
        }

        VE_INLINE void SetSleepLinearVelocity(f32 velocity) {
            if (velocity >= 0.0f) {
                VTRACE("PhysicsWorld: {} - Updating SleepLinearVelocity from {} to {}.", GetWorldName(), _settings.DefaultSleepLinearVelocity, velocity);

                _settings.DefaultSleepLinearVelocity = velocity;
                _sleepLinearVelocitySquared = velocity * velocity;
            }
        }

        [[nodiscard]] VE_INLINE f32 GetSleepAngularVelocity() const {
            return _settings.DefaultSleepAngularVelocity;
        }

        VE_INLINE void SetSleepAngularVelocity(f32 velocity) {
            if (velocity >= 0.0f) {
                VTRACE("PhysicsWorld: {} - Updating SleepAngularVelocity from {} to {}.", GetWorldName(), _settings.DefaultSleepAngularVelocity, velocity);

                _settings.DefaultSleepAngularVelocity = velocity;
                _sleepAngularVelocitySquared = velocity * velocity;
            }
        }

        [[nodiscard]] VE_INLINE f32 GetTimeToSleep() const {
            return _settings.TimeToSleep;
        }

        VE_INLINE void SetTimeToSleep(f32 timeToSleep) {
            if (timeToSleep >= 0.0f) {
                VTRACE("PhysicsWorld: {} - Updating TimeToSleep from {} to {}.", GetWorldName(), _settings.TimeToSleep, timeToSleep);

                _settings.TimeToSleep = timeToSleep;
            }
        }

        [[nodiscard]] VE_INLINE EventListener *GetEventListener() const {
            return _eventListener;
        }

        VE_INLINE void SetEventListener(EventListener *eventListener) {
            _eventListener = eventListener;
        }

        // [[nodiscard]] VE_INLINE bool IsDebugRenderingEnabled() const {
        //     return _enableDebugRendering;
        // }

        [[nodiscard]] VE_INLINE AABB GetWorldAABB(const Collider &collider) const {
            if (collider.GetBroadPhaseID() == AABB_TREE_NULL_NODE) {
                return AABB();
            }

            return _collisionSystem.GetWorldAABB(&collider);
        }

        VE_INLINE void TestOverlap(Body &bodyOne, Body &bodyTwo) {
            _collisionSystem.TestOverlap(bodyOne, bodyTwo);
        }

        VE_INLINE void TestOverlap(Body &body, OverlapCallback &callback) {
            _collisionSystem.TestOverlap(body, callback);
        }

        VE_INLINE void TestOverlap(OverlapCallback &callback) {
            _collisionSystem.TestOverlap(callback);
        }

        VE_INLINE void TestCollision(Body &bodyOne, Body &bodyTwo, CollisionCallback &callback) {
            _collisionSystem.TestCollision(bodyOne, bodyTwo, callback);
        }

        VE_INLINE void TestCollision(Body &body, CollisionCallback &callback) {
            _collisionSystem.TestCollision(body, callback);
        }

        VE_INLINE void TestCollision(CollisionCallback &callback) {
            _collisionSystem.TestCollision(callback);
        }

        [[nodiscard]] VE_INLINE RigidBody &GetRigidBody(size_t index) {
            VASSERT(index < _rigidBodies.size(), "Rigid Body index out of bounds");

            return *_rigidBodies[index];
        }

        [[nodiscard]] VE_INLINE const RigidBody &GetRigidBody(size_t index) const {
            VASSERT(index < _rigidBodies.size(), "Rigid Body index out of bounds");

            return *_rigidBodies[index];
        }

        void Update(Timestep timestep);

        RigidBody &CreateRigidBody(const TransformComponent &transform);
        void DestroyRigidBody(RigidBody &body);

        Joint &CreateJoint(const JointData &jointInfo);
        void DestroyJoint(const Joint &joint);

        void SetActiveStatusForBody(Entity entity, bool active);

    private:
        PhysicsWorldSettings _settings;
        PhysicsContext _context;
        EntityManager _entityManager;

        BodyComponentStore _bodyStore;
        RigidBodyComponentStore _rigidBodyStore;
        ColliderComponentStore _colliderStore;
        TransformComponentStore _transformStore;
        BallAndSocketJointComponentStore _basJointStore;
        FixedJointComponentStore _fixedJointStore;
        HingeJointComponentStore _hingeJointStore;
        SliderJointComponentStore _sliderJointStore;
        JointComponentStore _jointStore;

        CollisionSystem _collisionSystem;
        ConstraintSolverSystem _constraintSolverSystem;
        DynamicsSystem _dynamicsSystem;
        ContactSolverSystem _contactSolverSystem;

        Islands _islands;

        std::vector<RigidBody *> _rigidBodies;
        std::vector<size_t> _processContactPairsOrderIslands;

        EventListener *_eventListener;

        f32 _sleepLinearVelocitySquared;
        f32 _sleepAngularVelocitySquared;

        bool _gravityEnabled;
        bool _enableWarmStartup;
        // bool _enableDebugRendering;

        void setJointStatus(Entity jointEntity, bool enabled);
        void enableDisableJoints();

        void solveContactsAndConstraints(Timestep timestep);
        void solvePositionCorrection();
        void createIslands();
        void updateSleepingBodies(Timestep timeStep);
        void addJointToBodies(Entity bodyOne, Entity bodyTwo, Entity joint);
        void updateBodiesInverseWorldInertiaTensors();
    };

} // namespace Vulkyrie
