#pragma once

#include "events/event.h"

namespace Vulkyrie::Layer {
    class Layer {
        public:
            virtual ~Layer() = default;

            virtual void OnAttach() {
            }
            virtual void OnDetach() {
            }
            virtual void OnUpdate(float deltaTime) {
            }
            virtual void OnEvent(Vulkyrie::Events::Event &event) {
            }
    };
} // namespace Vulkyrie::Layer
