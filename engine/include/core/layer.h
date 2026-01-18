#pragma once

#include "core/time_step.h"
#include "core/uuid.h"
#include "events/event.h"

namespace Vulkyrie::Core {
    class Application;

    class Layer {
        public:
            explicit Layer(Application &application) : _application(application) {};
            virtual ~Layer() = default;

            virtual void OnAttach() {
            }

            virtual void OnDetach() {
            }

            virtual void OnUpdate(const Timestep deltaTime) {
            }

            virtual void OnEvent(Vulkyrie::Events::Event &event) {
            }

            [[nodiscard]] inline const UUID &GetLayerID() const {
                return _id;
            }

        protected:
            Application &_application;
            UUID _id;
    };
} // namespace Vulkyrie::Core
