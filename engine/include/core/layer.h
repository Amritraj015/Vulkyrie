#pragma once

#include "core/time_step.h"
#include "core/utilities/uuid.h"
#include "events/event.h"

namespace Vulkyrie {
    class Application;

    /** @brief Base class representing a layer in the application. */
    class Layer {
    public:
        /** @brief Default destructor for the Layer. */
        virtual ~Layer() = default;

        /** @brief Called when the layer is attached to the application's layer stack. */
        virtual void OnAttached() {
        }

        /** @brief Called when the layer is detached from the application's layer stack. */
        virtual void OnDetached() {
        }

        /** @brief Called when the layer is re-attached to the application's layer stack after being suspended. */
        virtual void OnResumed() {
        }

        /** @brief Called when the layer is suspended (temporarily deactivated). */
        virtual void OnSuspended() {
        }

        /** @brief Called every frame to update the layer.
         * @param deltaTime The time elapsed since the last frame.
         */
        virtual void OnUpdate([[maybe_unused]] Timestep deltaTime) {
        }

        /** @brief Called when an event is dispatched to the layer.
         * @param event The event to handle.
         */
        virtual void OnEvent([[maybe_unused]] Event &event) {
        }

        /** @brief Gets the unique identifier of the layer.
         * @returns The UUID of the layer.
         */
        [[nodiscard]] VE_INLINE const UUID &GetLayerID() const {
            return _id;
        }

    protected:
        /** @brief Unique identifier for the layer. */
        UUID _id{};
    };
} // namespace Vulkyrie
