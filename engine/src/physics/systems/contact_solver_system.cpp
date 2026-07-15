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

        // Clear the solver data of the previous step (the capacity is kept, so no reallocation happens
        // once the vectors have grown to the size of a typical step).
        _contactConstraints.clear();
        _contactPoints.clear();

        if (_allContactManifolds->empty() || _allContactPoints->empty()) {
            return;
        }

        _contactConstraints.reserve(_allContactManifolds->size());
        _contactPoints.reserve(_allContactPoints->size());

        // Build the internal solver constraints for each island of the world.
        for (size_t i = 0; i < _islands.GetTotalIslands(); ++i) {
            if (_islands.TotalContactManifolds[i] > 0) {
                initializeForIsland(i);
            }
        }

        // Re-apply the impulses accumulated during the previous step to converge faster.
        warmStart();
    }

    void ContactSolverSystem::StoreImpulses() {
        size_t contactPointIndex = 0;

        // Save the accumulated impulses back into the external contacts so the next step can warm
        // start from them.
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
            manifold->FrictionVectorOne = constraint.FrictionVector1;
            manifold->FrictionVectorTwo = constraint.FrictionVector2;
        }
    }

    void ContactSolverSystem::Solve(Timestep timestep) {
        // TODO: Implement this.
        (void)timestep;
    }

    void ContactSolverSystem::Reset() {
        // Clear the external contacts of the step that was just solved (the vectors are owned by the
        // CollisionSystem, which refills them during the next collision detection).
        if (nullptr != _allContactPoints) {
            _allContactPoints->clear();
        }

        if (nullptr != _allContactManifolds) {
            _allContactManifolds->clear();
        }
    }

    void ContactSolverSystem::initializeForIsland(size_t islandIndex) {
        // TODO: Implement this.
        (void)islandIndex;
    }

    void ContactSolverSystem::computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldSolver &contactPoint) const {
        VASSERT(glm::length2(contactPoint.Normal) > f32(0.0), "Contact Manifold Normal Length should be greater than 0.");

        // Compute the velocity difference in the tangential plane of the contact.
        const f32 deltaVDotNormal = glm::dot(deltaVelocity, contactPoint.Normal);
        const glm::vec3 normalVelocity = deltaVDotNormal * contactPoint.Normal;
        const glm::vec3 tangentVelocity = deltaVelocity - normalVelocity;
        const f32 lengthTangentVelocity = glm::length(tangentVelocity);

        // The first friction vector points along the tangential velocity when there is one;
        // otherwise any unit vector orthogonal to the normal works.
        if (VE_MACHINE_EPSILON < lengthTangentVelocity) {
            contactPoint.FrictionVector1 = tangentVelocity / lengthTangentVelocity;
        } else {
            contactPoint.FrictionVector1 = GetOrthogonalUnitVector(contactPoint.Normal);
        }

        // t2 = n x t1, so that (t1, t2, n) forms an orthonormal basis of the contact space.
        contactPoint.FrictionVector2 = glm::cross(contactPoint.Normal, contactPoint.FrictionVector1);
    }

    void ContactSolverSystem::warmStart() {
        size_t contactPointIndex = 0;

        for (size_t c = 0; c < _totalContactManifolds; c++) {
            bool atLeastOneRestingContactPoint = false;
            auto &cc = _contactConstraints[c];
            const size_t indexBody1 = cc.RigidBodyComponentIndexOfBody1;
            const size_t indexBody2 = cc.RigidBodyComponentIndexOfBody2;

            // Both bodies are fixed for the whole manifold, so accumulate the impulses in local
            // copies of the velocities and write them back to the store once per manifold.
            glm::vec3 linearVelocity1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(indexBody1);
            glm::vec3 angularVelocity1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(indexBody1);
            glm::vec3 linearVelocity2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(indexBody2);
            glm::vec3 angularVelocity2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(indexBody2);

            for (u8 i = 0; i < cc.TotalContactPoints; i++) {
                auto &cp = _contactPoints[contactPointIndex];

                if (cp.IsRestingContact) {
                    atLeastOneRestingContactPoint = true;

                    // Re-apply the accumulated penetration impulse P = lambda * n of the previous step.
                    const glm::vec3 impulsePenetration = cp.Normal * cp.PenetrationImpulse;
                    linearVelocity1 -= cc.MassInverseOfBody1 * (impulsePenetration * cc.LinearLockAxisFactorOfBody1);
                    angularVelocity1 -= (cp.i1TimesR1CrossN * cc.AngularLockAxisFactorOfBody1) * cp.PenetrationImpulse;

                    linearVelocity2 += cc.MassInverseOfBody2 * (impulsePenetration * cc.LinearLockAxisFactorOfBody2);
                    angularVelocity2 += (cp.i2TimesR2CrossN * cc.AngularLockAxisFactorOfBody2) * cp.PenetrationImpulse;
                } else {
                    // New contact point: there is no accumulated impulse to re-apply.
                    cp.PenetrationImpulse = f32(0.0);
                }

                contactPointIndex++;
            }

            if (atLeastOneRestingContactPoint) {
                // Project the old friction impulses (expressed with the old friction vectors) onto
                // the new friction vectors.
                const glm::vec3 oldFrictionImpulse = cc.Friction1Impulse * cc.OldFrictionVector1 + cc.Friction2Impulse * cc.OldFrictionVector2;
                cc.Friction1Impulse = glm::dot(oldFrictionImpulse, cc.FrictionVector1);
                cc.Friction2Impulse = glm::dot(oldFrictionImpulse, cc.FrictionVector2);

                // First friction constraint at the center of the contact manifold.
                glm::vec3 angularImpulseBody1 = -cc.r1CrossT1 * cc.Friction1Impulse;
                glm::vec3 linearImpulseBody2 = cc.FrictionVector1 * cc.Friction1Impulse;
                glm::vec3 angularImpulseBody2 = cc.r2CrossT1 * cc.Friction1Impulse;

                linearVelocity1 -= cc.MassInverseOfBody1 * (linearImpulseBody2 * cc.LinearLockAxisFactorOfBody1);
                angularVelocity1 += cc.AngularLockAxisFactorOfBody1 * (cc.InverseInertiaTensorOfBody1 * angularImpulseBody1);

                linearVelocity2 += cc.MassInverseOfBody2 * (linearImpulseBody2 * cc.LinearLockAxisFactorOfBody2);
                angularVelocity2 += cc.AngularLockAxisFactorOfBody2 * (cc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

                // Second friction constraint at the center of the contact manifold.
                angularImpulseBody1 = -cc.r1CrossT2 * cc.Friction2Impulse;
                linearImpulseBody2 = cc.FrictionVector2 * cc.Friction2Impulse;
                angularImpulseBody2 = cc.r2CrossT2 * cc.Friction2Impulse;

                linearVelocity1 -= cc.MassInverseOfBody1 * (linearImpulseBody2 * cc.LinearLockAxisFactorOfBody1);
                angularVelocity1 += cc.AngularLockAxisFactorOfBody1 * (cc.InverseInertiaTensorOfBody1 * angularImpulseBody1);

                linearVelocity2 += cc.MassInverseOfBody2 * (linearImpulseBody2 * cc.LinearLockAxisFactorOfBody2);
                angularVelocity2 += cc.AngularLockAxisFactorOfBody2 * (cc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

                // Twist friction constraint at the center of the contact manifold.
                angularImpulseBody1 = -cc.Normal * cc.FrictionTwistImpulse;
                angularImpulseBody2 = cc.Normal * cc.FrictionTwistImpulse;

                angularVelocity1 += cc.AngularLockAxisFactorOfBody1 * (cc.InverseInertiaTensorOfBody1 * angularImpulseBody1);
                angularVelocity2 += cc.AngularLockAxisFactorOfBody2 * (cc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(indexBody1, linearVelocity1);
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(indexBody1, angularVelocity1);

                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(indexBody2, linearVelocity2);
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(indexBody2, angularVelocity2);
            } else {
                // New contact manifold: there are no accumulated friction impulses to re-apply.
                cc.Friction1Impulse = f32(0.0);
                cc.Friction2Impulse = f32(0.0);
                cc.FrictionTwistImpulse = f32(0.0);
            }
        }
    }

} // namespace Vulkyrie
