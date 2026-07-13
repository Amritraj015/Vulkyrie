#include "physics/systems/contact_solver_system.h"
#include "physics/physics_world.h"
#include "core/utilities.h"

namespace Vulkyrie {

    ContactSolverSystem::ContactSolverSystem(PhysicsWorld &world, Islands &islands, f32 &restitutionVelocityThreshold)
        : _restitutionVelocityThreshold(restitutionVelocityThreshold)
        , _islands(islands)
        , _allContactManifolds(nullptr)
        , _allContactPoints(nullptr)
        , _bodyStore(world.GetBodyComponentStore())
        , _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _colliderStore(world.GetColliderComponentStore())
        , _totalContactPoints(0)
        , _totalContactManifolds(0)
        , _splitImpulseActive(true) {
    }

    void ContactSolverSystem::Initialize(std::vector<ContactManifold> *contactManifolds, std::vector<ContactPoint> *contactPoints) {
        _allContactManifolds = contactManifolds;
        _allContactPoints = contactPoints;
        _totalContactPoints = 0;
        _totalContactManifolds = 0;

        if (_allContactManifolds->empty() || _allContactPoints->empty()) {
            return;
        }

        _contactConstraints.reserve(_allContactManifolds->size());
        _contactPoints.reserve(_allContactPoints->size());

        // For each island of the world
        for (size_t i = 0; i < _islands.GetTotalIslands(); ++i) {
            if (_islands.TotalContactManifolds[i] > 0) {
                initializeForIsland(i);
            }
        }

        warmStart();
    }

    void ContactSolverSystem::StoreImpulses() {
        size_t contactPointIndex = 0;

        for (size_t c = 0; c < _totalContactManifolds; ++c) {
            auto &constraint = _contactConstraints[c];
            auto *manifold = constraint.ExternalContactManifold;

            for (u8 i = 0; i < constraint.TotalContactPoints; ++i) {
                auto &point = _contactPoints[contactPointIndex];
                point.ExternalPoint->SetPenetrationImpulse(point.PenetrationImpulse);

                contactPointIndex++;
            }

            manifold->FrictionImpulseOne = constraint.Friction1Impulse;
            manifold->FrictionImpulseTwo = constraint.Friction2Impulse;
            manifold->FrictionTwistImpulse = constraint.FrictionTwistImpulse;
            manifold->FrictionVectorOne = constraint.FrictionVectorOne;
            manifold->FrictionVectorTwo = constraint.FrictionVectorTwo;
        }
    }

    void ContactSolverSystem::Solve(Timestep timestep) {
        // TODO: Implement this.
        (void)timestep;
    }

    void ContactSolverSystem::Reset() {
        if (nullptr != _allContactPoints && !_allContactPoints->empty()) {
            _allContactPoints->clear();
        }

        if (nullptr != _allContactManifolds && !_allContactManifolds->empty()) {
            _allContactManifolds->clear();
        }
    }

    bool ContactSolverSystem::IsSplitImpulseActive() const {
        // TODO: Implement this.
        return false;
    }

    void ContactSolverSystem::SetSplitImpulseActiveFlag(bool active) {
        // TODO: Implement this.
        (void)active;
    }

    void ContactSolverSystem::initializeForIsland(size_t islandIndex) {
        // TODO: Implement this.
        (void)islandIndex;
    }

    f32 ContactSolverSystem::computeMixedRestitutionFactor(const Material &material1, const Material &material2) const {
        // TODO: Implement this.

        (void)material1;
        (void)material2;
        return f32(0);
    }

    f32 ContactSolverSystem::computeMixedFrictionCoefficient(const Material &material1, const Material &material2) const {
        // TODO: Implement this.

        (void)material1;
        (void)material2;
        return f32(0);
    }

    void ContactSolverSystem::computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldSolver &contactPoint) const {
        VASSERT(glm::length2(contactPoint.Normal) > f32(0.0), "Contact Manifold Normal Length should be greater than 0.");

        const f32 deltaVDotNormal = glm::dot(deltaVelocity, contactPoint.Normal);
        const glm::vec3 normalVelocity = deltaVDotNormal * contactPoint.Normal;
        const glm::vec3 tangentVelocity = deltaVelocity - normalVelocity;
        const f32 lengthTangentVelocity = glm::length(tangentVelocity);

        if (VE_MACHINE_EPSILON < lengthTangentVelocity) {
            contactPoint.FrictionVectorOne = tangentVelocity / lengthTangentVelocity;
        } else {
            contactPoint.FrictionVectorOne = GetOrthogonalUnitVector(contactPoint.Normal);
        }

        contactPoint.FrictionVectorTwo = glm::cross(contactPoint.Normal, contactPoint.FrictionVectorOne);
    }

    void ContactSolverSystem::warmStart() {
        // TODO: Implement this.
    }

} // namespace Vulkyrie
