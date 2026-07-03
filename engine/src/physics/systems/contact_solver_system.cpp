#include "physics/systems/contact_solver_system.h"
#include "physics/physics_world.h"

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

        if (_allContactManifolds->size() || _allContactPoints->size()) {
            return;
        }

        _contactConstraints.reserve(_allContactManifolds->size());
        _contactPoints.reserve(_allContactPoints->size());

        // For each island of the world
        for (size_t i = 0; i < _islands.GetTotalIslands(); ++i) {
            if (_islands.TotalContactManifolds[i] > 0) {
                InitializeForIsland(i);
            }
        }

        warmStart();
    }

    void ContactSolverSystem::InitializeForIsland(size_t islandIndex) {
        // TODO: Implement this.
        (void)islandIndex;
    }

    void ContactSolverSystem::StoreImpulses() {
        // TODO: Implement this.
    }

    void ContactSolverSystem::Solve(Timestep timestep) {
        // TODO: Implement this.
        (void)timestep;
    }

    void ContactSolverSystem::Reset() {
        if (nullptr != _allContactPoints && _allContactPoints->size() > 0) {
            _allContactPoints->clear();
        }

        if (nullptr != _allContactManifolds && _allContactManifolds->size() > 0) {
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
        // TODO: Implement this.

        (void)deltaVelocity;
        (void)contactPoint;
    }

    void ContactSolverSystem::warmStart() {
        // TODO: Implement this.
    }

} // namespace Vulkyrie
