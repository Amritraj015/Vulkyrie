#include "core/application.h"

namespace Vulkyrie::Core {
    Application::~Application() {
        for (auto& layer : _layers) {
            layer->OnDetach();
        }
    }

    void Application::RaiseEvent(Vulkyrie::Events::Event &event) const {
        for (auto it = _layers.rbegin(); it != _layers.rend(); ++it) {
            // If the event has been handled, stop propagating.
            if (event.handled) {
                break;
            }

            // Else, pass the event to the layer.
            (*it)->OnEvent(event);
        }
    }
} // namespace Vulkyrie::Core