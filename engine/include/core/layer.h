#pragma once

#include "core/time_step.h"
#include "events/event.h"

namespace Vulkyrie::Core {
    class Application;

    class Layer {
        public:
            explicit Layer(const Application &application, const std::string_view name = "Layer")
                : _application(application), _layerName(name) {};
            virtual ~Layer() = default;

            virtual void OnAttach() {
            }

            const char *GetName() const {
                return _layerName.data();
            }

            virtual void OnDetach() {
            }

            virtual void OnUpdate(Timestep deltaTime) {
            }

            virtual void OnEvent(Vulkyrie::Events::Event &event) {
            }

        protected:
            const Application &_application;
            std::string _layerName;
    };
} // namespace Vulkyrie::Core
