#pragma once

#include "core/entity.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    class ColliderComponentStore : public ComponentStore {
        public:
            ColliderComponentStore() = default;
            ~ColliderComponentStore() override = default;

        private:
    };

} // namespace Vulkyrie
