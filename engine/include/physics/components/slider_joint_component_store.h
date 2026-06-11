#pragma once

#include "vlkypch.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    class SliderJointComponentStore : public ComponentStore {
    public:
        SliderJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(SliderJointComponentStore);

        ~SliderJointComponentStore() override = default;

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
    };

} // namespace Vulkyrie
