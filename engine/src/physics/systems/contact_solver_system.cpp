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
        , _splitImpulseActive(true) {
    }

    void ContactSolverSystem::Initialize(std::vector<ContactManifold> *contactManifolds, std::vector<ContactPoint> *contactPoints) {
        _allContactManifolds = contactManifolds;
        _allContactPoints = contactPoints;

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
        for (size_t c = 0; c < _contactConstraints.size(); ++c) {
            ContactManifoldConstraint &constraint = _contactConstraints[c];
            ContactManifold *manifold = constraint.ExternalContactManifold;

            for (u8 i = 0; i < constraint.TotalContactPoints; ++i) {
                ContactPointConstraint &point = _contactPoints[contactPointIndex];
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
        size_t contactPointIndex = 0;

        const f32 beta = _splitImpulseActive ? BETA_SPLIT_IMPULSE : BETA;

        // For each contact manifold: solve every penetration constraint (one per contact point),
        // then the three friction constraints applied at the center of the manifold.
        for (size_t c = 0; c < _contactConstraints.size(); c++) {
            ContactManifoldConstraint &cmc = _contactConstraints[c];

            const size_t rigidBody1Index = cmc.RigidBodyComponentIndexOfBody1;
            const size_t rigidBody2Index = cmc.RigidBodyComponentIndexOfBody2;

            // Sum of the penetration impulses applied below; it bounds the friction impulses of the
            // manifold (Coulomb friction cone).
            f32 sumPenetrationImpulse = 0.0f;

            // Work on local copies of the constrained velocities and write them back once the whole
            // manifold has been solved. Every constraint below still sees the impulses applied by
            // the previous one (Gauss-Seidel) because the copies are updated in place.
            glm::vec3 v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(rigidBody1Index);
            glm::vec3 w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(rigidBody1Index);
            glm::vec3 v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(rigidBody2Index);
            glm::vec3 w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(rigidBody2Index);

            // Same for the split (pseudo) velocities used by the split-impulse position correction.
            glm::vec3 v1Split(0.0f);
            glm::vec3 w1Split(0.0f);
            glm::vec3 v2Split(0.0f);
            glm::vec3 w2Split(0.0f);

            if (_splitImpulseActive) {
                v1Split = _rigidBodyStore.GetSplitLinearVelocityAtIndex(rigidBody1Index);
                w1Split = _rigidBodyStore.GetSplitAngularVelocityAtIndex(rigidBody1Index);
                v2Split = _rigidBodyStore.GetSplitLinearVelocityAtIndex(rigidBody2Index);
                w2Split = _rigidBodyStore.GetSplitAngularVelocityAtIndex(rigidBody2Index);
            }

            for (u8 i = 0; i < cmc.TotalContactPoints; i++) {
                ContactPointConstraint &cpc = _contactPoints[contactPointIndex];

                // --------- Penetration constraint --------- //

                // Compute J*v: the relative velocity at the contact point along the contact normal.
                const glm::vec3 deltaV = v2 + glm::cross(w2, cpc.R2) - v1 - glm::cross(w1, cpc.R1);
                const f32 Jv = glm::dot(deltaV, cpc.Normal);

                // Baumgarte bias: drives the residual penetration (beyond the tolerated slop) to
                // zero by adding a corrective term to the velocity constraint.
                f32 biasPenetrationDepth = 0.0f;
                if (cpc.PenetrationDepth > SLOP) {
                    biasPenetrationDepth = -(beta / timestep.GetSeconds()) * std::max(f32(0.0), cpc.PenetrationDepth - SLOP);
                }
                const f32 b = biasPenetrationDepth + cpc.RestitutionBias;

                // Compute the Lagrange multiplier increment. With split impulses the position
                // correction is solved separately below, so only the restitution bias applies here;
                // otherwise the Baumgarte term is folded into the velocity constraint.
                f32 deltaLambda = _splitImpulseActive ? -(Jv + cpc.RestitutionBias) * cpc.InversePenetrationMass //
                                                      : -(Jv + b) * cpc.InversePenetrationMass;

                // Clamp the accumulated impulse to stay non-negative (contacts push, never pull) and
                // apply only the resulting increment.
                const f32 lambdaTemp = cpc.PenetrationImpulse;
                cpc.PenetrationImpulse = std::max(cpc.PenetrationImpulse + deltaLambda, f32(0.0));
                deltaLambda = cpc.PenetrationImpulse - lambdaTemp;

                // Apply the impulse P = lambda * n to both bodies.
                const glm::vec3 linearImpulse = cpc.Normal * deltaLambda;
                v1 -= cmc.MassInverseOfBody1 * (linearImpulse * cmc.LinearLockAxisFactorOfBody1);
                w1 -= (cpc.i1TimesR1CrossN * cmc.AngularLockAxisFactorOfBody1) * deltaLambda;

                v2 += cmc.MassInverseOfBody2 * (linearImpulse * cmc.LinearLockAxisFactorOfBody2);
                w2 += (cpc.i2TimesR2CrossN * cmc.AngularLockAxisFactorOfBody2) * deltaLambda;

                sumPenetrationImpulse += cpc.PenetrationImpulse;

                // --------- Split impulse (position correction) --------- //

                if (_splitImpulseActive) {
                    // Solve the same penetration constraint against the pseudo velocities with the
                    // Baumgarte bias, so the position correction adds no momentum to the real ones.
                    const glm::vec3 deltaVSplit = v2Split + glm::cross(w2Split, cpc.R2) - v1Split - glm::cross(w1Split, cpc.R1);
                    const f32 JvSplit = glm::dot(deltaVSplit, cpc.Normal);

                    f32 deltaLambdaSplit = -(JvSplit + biasPenetrationDepth) * cpc.InversePenetrationMass;

                    const f32 lambdaTempSplit = cpc.PenetrationSplitImpulse;
                    cpc.PenetrationSplitImpulse = std::max(cpc.PenetrationSplitImpulse + deltaLambdaSplit, f32(0.0));
                    deltaLambdaSplit = cpc.PenetrationSplitImpulse - lambdaTempSplit;

                    // Apply the impulse P = lambda * n to the pseudo velocities of both bodies.
                    const glm::vec3 splitImpulse = cpc.Normal * deltaLambdaSplit;
                    v1Split -= cmc.MassInverseOfBody1 * (splitImpulse * cmc.LinearLockAxisFactorOfBody1);
                    w1Split -= (cpc.i1TimesR1CrossN * cmc.AngularLockAxisFactorOfBody1) * deltaLambdaSplit;

                    v2Split += cmc.MassInverseOfBody2 * (splitImpulse * cmc.LinearLockAxisFactorOfBody2);
                    w2Split += (cpc.i2TimesR2CrossN * cmc.AngularLockAxisFactorOfBody2) * deltaLambdaSplit;
                }

                contactPointIndex++;
            }

            // The three friction constraints below share the same Coulomb cone limit derived from
            // the penetration impulses of the manifold.
            const f32 frictionLimit = cmc.FrictionCoefficient * sumPenetrationImpulse;

            // ------ First friction constraint at the center of the contact manifold ------ //

            // Compute J*v: the relative velocity at the friction point along the first friction vector.
            glm::vec3 deltaV = v2 + glm::cross(w2, cmc.r2Friction) - v1 - glm::cross(w1, cmc.r1Friction);
            f32 Jv = glm::dot(deltaV, cmc.FrictionVector1);

            // Compute the impulse increment and clamp the accumulated impulse to the friction cone.
            f32 deltaLambda = -Jv * cmc.InverseFriction1Mass;
            f32 lambdaTemp = cmc.Friction1Impulse;
            cmc.Friction1Impulse = std::max(-frictionLimit, std::min(cmc.Friction1Impulse + deltaLambda, frictionLimit));
            deltaLambda = cmc.Friction1Impulse - lambdaTemp;

            // Compute the impulse P = J^T * lambda and apply it to both bodies.
            glm::vec3 angularImpulseBody1 = -cmc.r1CrossT1 * deltaLambda;
            glm::vec3 linearImpulseBody2 = cmc.FrictionVector1 * deltaLambda;
            glm::vec3 angularImpulseBody2 = cmc.r2CrossT1 * deltaLambda;

            v1 -= cmc.MassInverseOfBody1 * (linearImpulseBody2 * cmc.LinearLockAxisFactorOfBody1);
            w1 += cmc.AngularLockAxisFactorOfBody1 * (cmc.InverseInertiaTensorOfBody1 * angularImpulseBody1);

            v2 += cmc.MassInverseOfBody2 * (linearImpulseBody2 * cmc.LinearLockAxisFactorOfBody2);
            w2 += cmc.AngularLockAxisFactorOfBody2 * (cmc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

            // ------ Second friction constraint at the center of the contact manifold ----- //

            // Recompute J*v with the velocities updated by the first friction constraint, this time
            // along the second friction vector.
            deltaV = v2 + glm::cross(w2, cmc.r2Friction) - v1 - glm::cross(w1, cmc.r1Friction);
            Jv = glm::dot(deltaV, cmc.FrictionVector2);

            deltaLambda = -Jv * cmc.InverseFriction2Mass;
            lambdaTemp = cmc.Friction2Impulse;
            cmc.Friction2Impulse = std::max(-frictionLimit, std::min(cmc.Friction2Impulse + deltaLambda, frictionLimit));
            deltaLambda = cmc.Friction2Impulse - lambdaTemp;

            // Compute the impulse P = J^T * lambda and apply it to both bodies.
            angularImpulseBody1 = -cmc.r1CrossT2 * deltaLambda;
            linearImpulseBody2 = cmc.FrictionVector2 * deltaLambda;
            angularImpulseBody2 = cmc.r2CrossT2 * deltaLambda;

            v1 -= cmc.MassInverseOfBody1 * (linearImpulseBody2 * cmc.LinearLockAxisFactorOfBody1);
            w1 += cmc.AngularLockAxisFactorOfBody1 * (cmc.InverseInertiaTensorOfBody1 * angularImpulseBody1);

            v2 += cmc.MassInverseOfBody2 * (linearImpulseBody2 * cmc.LinearLockAxisFactorOfBody2);
            w2 += cmc.AngularLockAxisFactorOfBody2 * (cmc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

            // ------ Twist friction constraint at the center of the contact manifold ------ //

            // Compute J*v: the relative angular velocity around the contact normal.
            Jv = glm::dot(w2 - w1, cmc.Normal);

            deltaLambda = -Jv * cmc.InverseTwistFrictionMass;
            lambdaTemp = cmc.FrictionTwistImpulse;
            cmc.FrictionTwistImpulse = std::max(-frictionLimit, std::min(cmc.FrictionTwistImpulse + deltaLambda, frictionLimit));
            deltaLambda = cmc.FrictionTwistImpulse - lambdaTemp;

            // Apply the purely angular impulse P = lambda * n around the manifold normal.
            angularImpulseBody2 = cmc.Normal * deltaLambda;
            w1 -= cmc.AngularLockAxisFactorOfBody1 * (cmc.InverseInertiaTensorOfBody1 * angularImpulseBody2);
            w2 += cmc.AngularLockAxisFactorOfBody2 * (cmc.InverseInertiaTensorOfBody2 * angularImpulseBody2);

            // Write the updated velocities of the manifold's bodies back to the store.
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(rigidBody1Index, v1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(rigidBody1Index, w1);

            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(rigidBody2Index, v2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(rigidBody2Index, w2);

            if (_splitImpulseActive) {
                _rigidBodyStore.SetSplitLinearVelocityAtIndex(rigidBody1Index, v1Split);
                _rigidBodyStore.SetSplitAngularVelocityAtIndex(rigidBody1Index, w1Split);

                _rigidBodyStore.SetSplitLinearVelocityAtIndex(rigidBody2Index, v2Split);
                _rigidBodyStore.SetSplitAngularVelocityAtIndex(rigidBody2Index, w2Split);
            }
        }
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
        VASSERT(_islands.TotalBodiesInIsland[islandIndex] > 0, "Total bodies in island at this index must be greater than 0.");
        VASSERT(_islands.TotalContactManifolds[islandIndex] > 0, "Total contact manifolds at this index must be greater than 0.");

        const size_t contactManifoldsIndex = _islands.ContactManifoldIndices[islandIndex];
        const size_t totalContactManifolds = _islands.TotalContactManifolds[islandIndex];

        // For each contact manifold of the island, build one manifold constraint (friction, applied
        // at the center of the manifold) and one point constraint per contact point (penetration).
        for (size_t m = contactManifoldsIndex; m < contactManifoldsIndex + totalContactManifolds; m++) {
            ContactManifold &externalManifold = (*_allContactManifolds)[m];

            VASSERT(externalManifold.ContactPointCount > 0, "Contact point count must be greater than 0.");

            const size_t rigidBodyIndex1 = _rigidBodyStore.GetEntityIndex(externalManifold.BodyOneEntity);
            const size_t rigidBodyIndex2 = _rigidBodyStore.GetEntityIndex(externalManifold.BodyTwoEntity);

            const size_t collider1Index = _colliderStore.GetEntityIndex(externalManifold.ColliderOneEntity);
            const size_t collider2Index = _colliderStore.GetEntityIndex(externalManifold.ColliderTwoEntity);

            // World-space centers of mass of the two bodies; every lever arm below is measured from these.
            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(rigidBodyIndex1);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(rigidBodyIndex2);

            // The materials are per-collider constants, so the mixed friction/restitution
            // coefficients are computed once per manifold rather than once per contact point.
            const Material &material1 = _colliderStore.GetMaterialAtIndex(collider1Index);
            const Material &material2 = _colliderStore.GetMaterialAtIndex(collider2Index);
            const f32 restitutionFactor = computeMixedRestitutionFactor(material1, material2);

            // Snapshot the per-body data that is shared by every contact point of the manifold.
            ContactManifoldConstraint &cmc = _contactConstraints.emplace_back();
            cmc.RigidBodyComponentIndexOfBody1 = rigidBodyIndex1;
            cmc.RigidBodyComponentIndexOfBody2 = rigidBodyIndex2;
            cmc.InverseInertiaTensorOfBody1 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(rigidBodyIndex1);
            cmc.InverseInertiaTensorOfBody2 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(rigidBodyIndex2);
            cmc.MassInverseOfBody1 = _rigidBodyStore.GetInverseMassAtIndex(rigidBodyIndex1);
            cmc.MassInverseOfBody2 = _rigidBodyStore.GetInverseMassAtIndex(rigidBodyIndex2);
            cmc.LinearLockAxisFactorOfBody1 = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(rigidBodyIndex1);
            cmc.LinearLockAxisFactorOfBody2 = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(rigidBodyIndex2);
            cmc.AngularLockAxisFactorOfBody1 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(rigidBodyIndex1);
            cmc.AngularLockAxisFactorOfBody2 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(rigidBodyIndex2);
            cmc.TotalContactPoints = externalManifold.ContactPointCount;
            cmc.FrictionCoefficient = computeMixedFrictionCoefficient(material1, material2);
            cmc.ExternalContactManifold = &externalManifold;

            // Accumulated over the contact points below, then averaged/normalized after the loop.
            cmc.Normal = glm::vec3(0.0f);
            cmc.FrictionPointBody1 = glm::vec3(0.0f);
            cmc.FrictionPointBody2 = glm::vec3(0.0f);

            // First term of every effective-mass computation below.
            const f32 inverseMassSum = cmc.MassInverseOfBody1 + cmc.MassInverseOfBody2;

            // Velocities of the bodies at the beginning of the step (needed for the restitution bias
            // and the initial friction directions).
            const glm::vec3 &v1 = _rigidBodyStore.GetLinearVelocityAtIndex(rigidBodyIndex1);
            const glm::vec3 &w1 = _rigidBodyStore.GetAngularVelocityAtIndex(rigidBodyIndex1);
            const glm::vec3 &v2 = _rigidBodyStore.GetLinearVelocityAtIndex(rigidBodyIndex2);
            const glm::vec3 &w2 = _rigidBodyStore.GetAngularVelocityAtIndex(rigidBodyIndex2);

            const TransformComponent &collider1LocalToWorldTransform = _colliderStore.GetLocalToWorldTransformAtIndex(collider1Index);
            const TransformComponent &collider2LocalToWorldTransform = _colliderStore.GetLocalToWorldTransformAtIndex(collider2Index);

            const size_t contactPointsStartIndex = externalManifold.ContactPointIndex;
            const size_t nbContactPoints = externalManifold.ContactPointCount;

            for (size_t c = contactPointsStartIndex; c < contactPointsStartIndex + nbContactPoints; c++) {
                ContactPoint &externalContact = (*_allContactPoints)[c];

                ContactPointConstraint &cpc = _contactPoints.emplace_back();
                cpc.ExternalPoint = &externalContact;
                cpc.Normal = externalContact.GetWorldSpaceContactNormal();

                // Bring the contact points into world space and compute the lever arms r1/r2
                // relative to the centers of mass.
                const glm::vec3 p1 = collider1LocalToWorldTransform * externalContact.GetLocalSpaceContactPointOnBodyOne();
                const glm::vec3 p2 = collider2LocalToWorldTransform * externalContact.GetLocalSpaceContactPointOnBodyTwo();
                cpc.R1 = p1 - x1;
                cpc.R2 = p2 - x2;

                cpc.PenetrationDepth = externalContact.GetPenetrationDepth();

                // Remember whether the point already existed during the previous step (warm starting
                // only re-applies impulses of resting contacts), then mark it as resting for the next step.
                cpc.IsRestingContact = externalContact.IsRestingContact();
                externalContact.SetIsRestingContact(true);

                // Start from the impulse accumulated during the previous step (used by warm starting).
                cpc.PenetrationImpulse = externalContact.GetPenetrationImpulse();
                cpc.PenetrationSplitImpulse = 0.0f;

                // Accumulate the world-space contact points; their average is the point where the
                // friction constraints of the manifold are applied.
                cmc.FrictionPointBody1 += p1;
                cmc.FrictionPointBody2 += p2;

                // Relative velocity of the two bodies at the contact point:
                // deltaV = v2 + w2 x r2 - v1 - w1 x r1
                const glm::vec3 deltaV = v2 + glm::cross(w2, cpc.R2) - v1 - glm::cross(w1, cpc.R1);

                // Precompute I^-1 * (r x n) for both bodies; the velocity solver applies these on
                // every iteration.
                const glm::vec3 r1CrossN = glm::cross(cpc.R1, cpc.Normal);
                const glm::vec3 r2CrossN = glm::cross(cpc.R2, cpc.Normal);
                cpc.i1TimesR1CrossN = cmc.InverseInertiaTensorOfBody1 * r1CrossN;
                cpc.i2TimesR2CrossN = cmc.InverseInertiaTensorOfBody2 * r2CrossN;

                // Effective mass of the penetration constraint:
                // K = m1^-1 + m2^-1 + ((I1^-1 (r1 x n)) x r1) . n + ((I2^-1 (r2 x n)) x r2) . n
                const f32 massPenetration = inverseMassSum                                                  //
                                            + glm::dot(glm::cross(cpc.i1TimesR1CrossN, cpc.R1), cpc.Normal) //
                                            + glm::dot(glm::cross(cpc.i2TimesR2CrossN, cpc.R2), cpc.Normal);
                cpc.InversePenetrationMass = massPenetration > f32(0.0) ? f32(1.0) / massPenetration : f32(0.0);

                // The restitution bias is computed here instead of inside the solver because it must
                // use the relative velocity at the beginning of the step. Contacts approaching slower
                // than the threshold are treated as resting and get no bounce.
                cpc.RestitutionBias = f32(0.0);
                const f32 deltaVDotN = glm::dot(deltaV, cpc.Normal);
                if (deltaVDotN < -_restitutionVelocityThreshold) {
                    cpc.RestitutionBias = restitutionFactor * deltaVDotN;
                }

                cmc.Normal += cpc.Normal;
            }

            // Average the contact points to get the friction application point of the manifold and
            // its lever arms relative to the centers of mass.
            cmc.FrictionPointBody1 /= static_cast<f32>(cmc.TotalContactPoints);
            cmc.FrictionPointBody2 /= static_cast<f32>(cmc.TotalContactPoints);
            cmc.r1Friction = cmc.FrictionPointBody1 - x1;
            cmc.r2Friction = cmc.FrictionPointBody2 - x2;

            // Keep the previous friction basis so warm starting can project the accumulated friction
            // impulses onto the new friction vectors.
            cmc.OldFrictionVector1 = externalManifold.FrictionVectorOne;
            cmc.OldFrictionVector2 = externalManifold.FrictionVectorTwo;

            // Start from the friction impulses accumulated during the previous step (used by warm starting).
            cmc.Friction1Impulse = externalManifold.FrictionImpulseOne;
            cmc.Friction2Impulse = externalManifold.FrictionImpulseTwo;
            cmc.FrictionTwistImpulse = externalManifold.FrictionTwistImpulse;

            // Average normal of the manifold; the friction and twist constraints act around it.
            cmc.Normal = glm::normalize(cmc.Normal);

            // Relative velocity of the two bodies at the friction point, used to align the first
            // friction vector with the tangential sliding direction:
            // deltaVFrictionPoint = v2 + w2 x r2Friction - v1 - w1 x r1Friction
            const glm::vec3 deltaVFrictionPoint = v2 + glm::cross(w2, cmc.r2Friction) - v1 - glm::cross(w1, cmc.r1Friction);
            computeFrictionVectors(deltaVFrictionPoint, cmc);

            // Effective masses of the two tangential friction constraints and of the twist friction
            // constraint (same K formula as the penetration constraint, along t1, t2 and n).
            cmc.r1CrossT1 = glm::cross(cmc.r1Friction, cmc.FrictionVector1);
            cmc.r1CrossT2 = glm::cross(cmc.r1Friction, cmc.FrictionVector2);
            cmc.r2CrossT1 = glm::cross(cmc.r2Friction, cmc.FrictionVector1);
            cmc.r2CrossT2 = glm::cross(cmc.r2Friction, cmc.FrictionVector2);

            const f32 friction1Mass = inverseMassSum +
                                      glm::dot(glm::cross(cmc.InverseInertiaTensorOfBody1 * cmc.r1CrossT1, cmc.r1Friction), cmc.FrictionVector1) +
                                      glm::dot(glm::cross(cmc.InverseInertiaTensorOfBody2 * cmc.r2CrossT1, cmc.r2Friction), cmc.FrictionVector1);

            const f32 friction2Mass = inverseMassSum +
                                      glm::dot(glm::cross(cmc.InverseInertiaTensorOfBody1 * cmc.r1CrossT2, cmc.r1Friction), cmc.FrictionVector2) +
                                      glm::dot(glm::cross(cmc.InverseInertiaTensorOfBody2 * cmc.r2CrossT2, cmc.r2Friction), cmc.FrictionVector2);

            const f32 frictionTwistMass = glm::dot(cmc.Normal, cmc.InverseInertiaTensorOfBody1 * cmc.Normal) //
                                          + glm::dot(cmc.Normal, cmc.InverseInertiaTensorOfBody2 * cmc.Normal);

            // A zero effective mass means both bodies are immovable along the constraint direction;
            // store an inverse of zero so the solver applies no impulse instead of dividing by zero.
            cmc.InverseFriction1Mass = friction1Mass > f32(0.0) ? f32(1.0) / friction1Mass : f32(0.0);
            cmc.InverseFriction2Mass = friction2Mass > f32(0.0) ? f32(1.0) / friction2Mass : f32(0.0);
            cmc.InverseTwistFrictionMass = frictionTwistMass > f32(0.0) ? f32(1.0) / frictionTwistMass : f32(0.0);
        }
    }

    void ContactSolverSystem::computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldConstraint &contactManifold) const {
        VASSERT(glm::length2(contactManifold.Normal) > f32(0.0), "Contact Manifold Normal Length should be greater than 0.");

        // Compute the velocity difference in the tangential plane of the contact.
        const f32 deltaVDotNormal = glm::dot(deltaVelocity, contactManifold.Normal);
        const glm::vec3 normalVelocity = deltaVDotNormal * contactManifold.Normal;
        const glm::vec3 tangentVelocity = deltaVelocity - normalVelocity;
        const f32 lengthTangentVelocity = glm::length(tangentVelocity);

        // The first friction vector points along the tangential velocity when there is one;
        // otherwise any unit vector orthogonal to the normal works.
        if (VE_MACHINE_EPSILON < lengthTangentVelocity) {
            contactManifold.FrictionVector1 = tangentVelocity / lengthTangentVelocity;
        } else {
            contactManifold.FrictionVector1 = GetOrthogonalUnitVector(contactManifold.Normal);
        }

        // t2 = n x t1, so that (t1, t2, n) forms an orthonormal basis of the contact space.
        contactManifold.FrictionVector2 = glm::cross(contactManifold.Normal, contactManifold.FrictionVector1);
    }

    void ContactSolverSystem::warmStart() {
        size_t contactPointIndex = 0;

        for (size_t c = 0; c < _contactConstraints.size(); c++) {
            bool atLeastOneRestingContactPoint = false;
            ContactManifoldConstraint &cc = _contactConstraints[c];
            const size_t indexBody1 = cc.RigidBodyComponentIndexOfBody1;
            const size_t indexBody2 = cc.RigidBodyComponentIndexOfBody2;

            // Both bodies are fixed for the whole manifold, so accumulate the impulses in local
            // copies of the velocities and write them back to the store once per manifold.
            glm::vec3 linearVelocity1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(indexBody1);
            glm::vec3 angularVelocity1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(indexBody1);
            glm::vec3 linearVelocity2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(indexBody2);
            glm::vec3 angularVelocity2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(indexBody2);

            for (u8 i = 0; i < cc.TotalContactPoints; i++) {
                ContactPointConstraint &cp = _contactPoints[contactPointIndex];

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
