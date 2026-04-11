#pragma once

#include "physics/components/component_store.h"

namespace Vulkyrie {

    class BodyComponentStore : public ComponentStore {
        public:
            BodyComponentStore() = default;
            ~BodyComponentStore() override = default;
    };

} // namespace Vulkyrie
