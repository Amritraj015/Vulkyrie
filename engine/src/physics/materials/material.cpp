#include "physics/materials/material.h"

namespace Vulkyrie {

    Material::Material(f32 frictionCoefficient, f32 restitutionCoefficient, f32 density)
        : _frictionCoefficientSquareRoot(std::sqrt(frictionCoefficient))
        , _restitutionCoefficient(restitutionCoefficient)
        , _density(density) {

        VASSERT(frictionCoefficient >= 0.0f, "Friction coefficient must be non-negative.");
        VASSERT(restitutionCoefficient >= 0.0f && restitutionCoefficient <= 1.0f, "Restitution coefficient must be in the range [0, 1].");
        VASSERT(density > 0.0f, "Density must be a positive value.");
    }

} // namespace Vulkyrie
