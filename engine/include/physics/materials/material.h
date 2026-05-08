#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief The Material class represents the physical properties of a material, including its friction coefficient, restitution coefficient, and density.
     * The friction coefficient determines how much resistance there is to sliding motion between two surfaces in contact, while the restitution coefficient
     * determines how much kinetic energy is conserved during a collision. The density is a measure of how much mass is contained in a given volume of the
     * material. These properties are crucial for accurate physics simulations, as they affect how objects interact with each other and respond to forces. The
     * Material class provides getter and setter methods for each of these properties, allowing for dynamic adjustments during runtime. */
    class Material final {
        public:
            /** @brief Constructs a Material with the specified friction coefficient, restitution coefficient, and density.
             * @param frictionCoefficient The friction coefficient of the material, which determines how much resistance there is to sliding motion between two
             * surfaces in contact. Must be a non-negative value.
             * @param restitutionCoefficient The restitution coefficient of the material, which determines how much kinetic energy is conserved during a
             * collision. It is a value between 0 and 1, where 0 represents a perfectly inelastic collision and 1 represents a perfectly elastic collision.
             * @param density The density of the material, which is a measure of how much mass is contained in a given volume of the material. Must be a
             * positive value. */
            Material(f32 frictionCoefficient, f32 restitutionCoefficient, f32 density = 1.0f);

            /** @brief Destructor for the Material class. */
            ~Material() = default;

            /** @brief Gets the square root of the friction coefficient of the material. Storing the square root allows for more efficient calculations during
             * collision response, as it avoids the need to compute square roots repeatedly when calculating friction forces. */
            [[nodiscard]] VE_FORCE_INLINE f32 GetFrictionCoefficientSquareRoot() const {
                return _frictionCoefficientSquareRoot;
            }

            /** @brief Gets the friction coefficient of the material. The friction coefficient determines how much resistance there is to sliding motion between
             * two surfaces in contact. The friction coefficient must be a non-negative value, as negative friction does not make physical sense. This property
             * is crucial for accurate physics simulations, as it affects how objects interact with each other and respond to forces. */
            [[nodiscard]] VE_FORCE_INLINE f32 GetFrictionCoefficient() const {
                return _frictionCoefficientSquareRoot * _frictionCoefficientSquareRoot;
            }

            /** @brief Sets the friction coefficient of the material. The friction coefficient determines how much resistance there is to sliding motion between
             * two surfaces in contact. The friction coefficient must be a non-negative value, as negative friction does not make physical sense. This property
             * is crucial for accurate physics simulations, as it affects how objects interact with each other and respond to forces. */
            VE_FORCE_INLINE void SetFrictionCoefficient(f32 frictionCoefficient) {
                VASSERT(frictionCoefficient >= 0.0f, "Friction coefficient must be non-negative.");

                _frictionCoefficientSquareRoot = std::sqrt(frictionCoefficient);
            }

            /** @brief Gets the restitution coefficient of the material. The restitution coefficient determines how much kinetic energy is conserved during a
             * collision. It is a value between 0 and 1, where 0 represents a perfectly inelastic collision and 1 represents a perfectly elastic collision. The
             * restitution coefficient is used to calculate the resulting velocities of colliding bodies based on their masses and initial velocities. */
            [[nodiscard]] VE_FORCE_INLINE f32 GetRestitutionCoefficient() const {
                return _restitutionCoefficient;
            }

            /** @brief Sets the restitution coefficient of the material. The restitution coefficient determines how much kinetic energy is conserved during a
             * collision. It is a value between 0 and 1, where 0 represents a perfectly inelastic collision and 1 represents a perfectly elastic
             * collision. The restitution coefficient is used to calculate the resulting velocities of colliding bodies based on their masses and
             * initial velocities. */
            VE_FORCE_INLINE void SetRestitutionCoefficient(f32 restitutionCoefficient) {
                VASSERT(restitutionCoefficient >= 0.0f && restitutionCoefficient <= 1.0f, "Restitution coefficient must be in the range [0, 1].");

                _restitutionCoefficient = restitutionCoefficient;
            }

            /** @brief Gets the density of the material. The density is a measure of how much mass is contained in a given volume of the material.
             * A higher density indicates a heavier material, while a lower density indicates a lighter material. */
            [[nodiscard]] VE_FORCE_INLINE f32 GetDensity() const {
                return _density;
            }

            /** @brief Sets the density of the material. The density must be a positive value, as it represents the mass per unit volume of the material. A
             * higher density indicates a heavier material, while a lower density indicates a lighter material. This property is crucial for accurate physics
             * simulations, as it affects how objects interact with each other and respond to forces. */
            VE_FORCE_INLINE void SetDensity(f32 density) {
                VASSERT(density > 0.0f, "Density must be a positive value.");

                _density = density;
            }

        private:
            /** @brief The square root of the friction coefficient of the material. Storing the square root allows for more efficient calculations during
             * collision response, as it avoids the need to compute square roots repeatedly when calculating friction forces. */
            f32 _frictionCoefficientSquareRoot;

            /** @brief The restitution coefficient of the material, which determines the elasticity of collisions.
             * It is a value between 0 and 1, where 0 represents a perfectly inelastic collision and 1 represents a perfectly elastic
             * collision. The restitution coefficient is used to calculate the resulting velocities of colliding bodies based on their
             * masses and initial velocities. */
            f32 _restitutionCoefficient;

            /** @brief The density of the material. */
            f32 _density;
    };

} // namespace Vulkyrie
