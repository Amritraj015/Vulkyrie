#pragma once

#include "core/time_step.h"
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

            virtual void OnUpdate(Timestep deltaTime) {
            }

            virtual void OnEvent(Vulkyrie::Events::Event &event) {
            }

        protected:
            Application &_application;
    };
} // namespace Vulkyrie::Core
