#include "core/layer_stack.h"

namespace Vulkyrie::Core {
    LayerStack::~LayerStack() {
        for (Layer* layer : _layers) {
            layer->OnDetach();
            delete layer;
        }
    }

    void LayerStack::PushLayer(Layer* layer) {
        _layers.emplace(_layers.begin() + _layerInsertIndex, layer);
        _layerInsertIndex++;
        layer->OnAttach();
    }

    void LayerStack::PushOverlay(Layer* overlay) {
        _layers.emplace_back(overlay);
        overlay->OnAttach();
    }

    void LayerStack::PopLayer(Layer* layer) {
        auto it = std::find(_layers.begin(), _layers.begin() + _layerInsertIndex, layer);
        if (it != _layers.begin() + _layerInsertIndex) {
            layer->OnDetach();
            _layers.erase(it);
            _layerInsertIndex--;
        }
    }

    void LayerStack::PopOverlay(Layer* overlay) {
        auto it = std::find(_layers.begin() + _layerInsertIndex, _layers.end(), overlay);
        if (it != _layers.end()) {
            overlay->OnDetach();
            _layers.erase(it);
        }
    }
} // namespace Vulkyrie::Core