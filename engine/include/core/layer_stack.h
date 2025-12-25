#pragma once

#include "defines.h"
#include "core/layer.h"

namespace Vulkyrie::Core {
    class LayerStack {
        public:
            LayerStack() = default;
            ~LayerStack();

            void PushLayer(Layer *layer);
            void PushOverlay(Layer *overlay);
            void PopLayer(Layer *layer);
            void PopOverlay(Layer *overlay);

            auto begin() { return _layers.begin(); }
            auto end() { return _layers.end(); }
            auto begin() const { return _layers.begin(); }
            auto end() const { return _layers.end(); }

            auto rbegin() { return _layers.rbegin(); }
            auto rend() { return _layers.rend(); }
            auto rbegin() const { return _layers.rbegin(); }
            auto rend() const { return _layers.rend(); }

        private:
            std::vector<Layer *> _layers;
            size_t _layerInsertIndex = 0;
    };
} // namespace Vulkyrie::Core
