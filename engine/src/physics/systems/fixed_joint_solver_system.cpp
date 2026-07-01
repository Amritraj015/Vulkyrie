#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/physics_world.h"
#include "core/asserts.h"
#include "core/utilities.h"

namespace Vulkyrie {

    FixedJointSolverSystem::FixedJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _fixedJointStore(world.GetFixedJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void FixedJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _fixedJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");

            _fixedJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex));
            _fixedJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex));

            const glm::quat &orientationBodyOne = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &orientationBodyTwo = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            const glm::vec3 rOneWorld = orientationBodyOne * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 rTwoWorld = orientationBodyTwo * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _fixedJointStore.SetR1WorldAtIndex(i, rOneWorld);
            _fixedJointStore.SetR2WorldAtIndex(i, rTwoWorld);

            const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(rOneWorld);
            const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(rTwoWorld);

            const f32 bodyOneInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 bodyTwoInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 totalInverseMass = bodyOneInverseMass + bodyTwoInverseMass;

            const glm::mat3 &inertiaTensorBodyOne = _fixedJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _fixedJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::mat3 massMatrix = glm::mat3(glm::vec3(totalInverseMass, 0, 0), //
                                                   glm::vec3(0, totalInverseMass, 0), //
                                                   glm::vec3(0, 0, totalInverseMass)  //
                                                   ) +
                                         skewSymmetricMatrixU1 * inertiaTensorBodyOne * glm::transpose(skewSymmetricMatrixU1) +
                                         skewSymmetricMatrixU2 * inertiaTensorBodyTwo * glm::transpose(skewSymmetricMatrixU2);

            _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat3(0));
            const f32 massMatrixDeterminant = glm::determinant(massMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat3(massMatrix, massMatrixDeterminant));
                }
            }

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            _fixedJointStore.SetTranslationBiasAtIndex(i, glm::vec3(0));
            if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                _fixedJointStore.SetTranslationBiasAtIndex(i, biasFactor * (x2 + rTwoWorld - x1 - rOneWorld));
            }

            const glm::mat3 inverseMassRotationMatrix = inertiaTensorBodyOne + inertiaTensorBodyTwo;
            _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, inverseMassRotationMatrix);

            const f32 massMatrixRotationDeterminant = glm::determinant(_fixedJointStore.GetInverseMassRotationMatrixAtIndex(i));
            if (VE_MACHINE_EPSILON < std::abs(massMatrixRotationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(inverseMassRotationMatrix, massMatrixRotationDeterminant));
                }
            }

            _fixedJointStore.SetRotationBiasAtIndex(i, glm::vec3(0));

            if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                const glm::quat &initialOrientationDifferenceInverse = _fixedJointStore.GetInitialOrientationDifferenceInverseAtIndex(i);
                const glm::quat qError = orientationBodyTwo * initialOrientationDifferenceInverse * glm::inverse(orientationBodyOne);

                _fixedJointStore.SetRotationBiasAtIndex(i, biasFactor * f32(2.0) * glm::vec3(qError.x, qError.y, qError.z));
            }

            if (!_enableWarmStartup) {
                _fixedJointStore.SetImpulseTranslationAtIndex(i, glm::vec3(0));
                _fixedJointStore.SetImpulseRotationAtIndex(i, glm::vec3(0));
            }
        }
    }

    void FixedJointSolverSystem::WarmStart() {
        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {

            const Entity jointEntity = _fixedJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);
        }
    }

    void FixedJointSolverSystem::SolveVelocityConstraint() {
        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {

            const Entity jointEntity = _fixedJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);
        }
    }

    void FixedJointSolverSystem::SolvePositionConstraint() {
        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _fixedJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);
        }
    }

} // namespace Vulkyrie
