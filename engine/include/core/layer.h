#pragma once

#include "core/time_step.h"
#include "events/event.h"

namespace Vulkyrie::Core {
    class Layer {
        public:
            explicit Layer(const std::string_view name = "Layer") : _layerName(name) {};
            virtual ~Layer() = default;

            virtual void OnAttach() {
            }

            virtual void OnDetach() {
            }

            virtual void OnUpdate(Timestep deltaTime) {
            }

            virtual void OnRender() {
            }

            virtual void OnEvent(Vulkyrie::Events::Event &event) {
            }

        protected:
            std::string _layerName;
    };
} // namespace Vulkyrie::Core
