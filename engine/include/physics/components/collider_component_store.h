#pragma once

#include "core/entity.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    class ColliderComponentStore : public ComponentStore {
        public:
            ColliderComponentStore() = default;
            ~ColliderComponentStore() override = default;

        protected:
            void swapComponents(size_t indexA, size_t indexB) override;
            void removeLastComponentAndEntity() override;

        private:
    };

} // namespace Vulkyrie
