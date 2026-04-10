#pragma once

#include "physics/body/body.h"

namespace Vulkyrie {

    class RigidBody : public Body {
        public:
            virtual ~RigidBody() override = default;
    };

} // namespace Vulkyrie
