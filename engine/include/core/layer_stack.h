#pragma once

#include "layer.h"
#include "core/logger.h"

namespace Vulkyrie::Core {

#define MAX_LAYER_OPERATIONS 10

    /** @brief Represents an operation to be performed on the layer stack. */
    struct LayerOperation {
        public:
            /** @brief The type of operation to perform on the layer stack. */
            enum class OperationType : u8 { PushLayer, PopLayer, PushOverlay } Type;

            /** @brief The layer to push onto the stack.
             * Null if the operation is a Pop.
             */
            Scope<Vulkyrie::Core::Layer> Layer;

            /** @brief The target layer to pop from the stack.
             * Null if the operation is a Push.
             */
            UUID LayerIdToPop;
    };

    /** @brief Manages a stack of layers and overlays for the application. */
    class LayerStack {
        public:
            /** @brief Default constructor for the LayerStack class. */
            LayerStack() : _layerInsertIndex(0) {
                // Reserve space for layer operations to minimize reallocations.
                _layerOperations.reserve(MAX_LAYER_OPERATIONS);
            }

            /** @brief Destructor to clean up the layer stack and detach all layers. */
            ~LayerStack() {
                for (auto &layer : _layers) {
                    layer->OnDetach();
                }
            }

            /** @brief Pushes a new layer onto the layer stack.
             * @tparam TLayer The type of layer to push.
             * @param args Arguments to forward to the layer's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushLayer(TArgs &&...args) {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation{
                        .Type = LayerOperation::OperationType::PushLayer,
                        .Layer = CreateScope<TLayer>(std::forward<TArgs>(args)...),
                        .LayerIdToPop = 0,
                    });
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot push more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Pushes a new overlay onto the layer stack.
             * @tparam TLayer The type of overlay to push.
             * @param args Arguments to forward to the overlay's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushOverlay(TArgs &&...args) {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation{
                        .Type = LayerOperation::OperationType::PushOverlay,
                        .Layer = CreateScope<TLayer>(std::forward<TArgs>(args)...),
                        .LayerIdToPop = 0,
                    });
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot push more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Pops a layer from the layer stack.
             * @tparam layerId The ID of the layer to pop.
             */
            void PopLayer(const UUID &layerId) {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation{
                        .Type = LayerOperation::OperationType::PopLayer,
                        .Layer = nullptr,
                        .LayerIdToPop = layerId,
                    });
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot pop more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Gets a layer of the specified type from the layer stack.
             * @tparam TLayer The type of layer to get.
             * @returns A pointer to the layer if found, nullptr otherwise.
             */
            template <typename TLayer>
                requires(std::is_base_of_v<Layer, TLayer>)
            TLayer *GetLayer() {
                for (const auto &layer : _layers) {
                    if (auto casted = dynamic_cast<TLayer *>(layer.get())) {
                        return casted;
                    }
                }

                return nullptr;
            }

            /** @brief Processes queued operations on the layer stack. */
            inline void ProcessOperations() {
                if (_layerOperations.empty()) {
                    return;
                }

                for (auto &operation : _layerOperations) {
                    switch (operation.Type) {
                        case LayerOperation::OperationType::PushLayer: {
                            // Get a reference to the layer before moving it.
                            auto &layerRef = *operation.Layer;

                            // Insert the layer at the correct position.
                            _layers.emplace(_layers.begin() + _layerInsertIndex, std::move(operation.Layer));

                            // Increment the layer insert index.
                            _layerInsertIndex++;

                            // Call OnAttach on the layer.
                            layerRef.OnAttach();

                            break;
                        }
                        case LayerOperation::OperationType::PopLayer: {
                            const auto layerId = operation.LayerIdToPop;

                            auto it = std::find_if(_layers.begin(), _layers.begin() + _layerInsertIndex, [layerId](const Scope<Layer> &l) {
                                return l.get()->GetLayerID() == layerId;
                            });

                            if (it != _layers.begin() + _layerInsertIndex) {
                                // Call OnDetach on the layer.
                                (*it)->OnDetach();

                                // Remove the layer from the stack.
                                _layers.erase(it);

                                // Decrement the layer insert index.
                                --_layerInsertIndex;
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PushOverlay: {
                            // Get a reference to the overlay before moving it.
                            auto &overlayRef = *operation.Layer;

                            // Insert the overlay at the end of the layer stack.
                            _layers.emplace_back(std::move(operation.Layer));

                            // Call OnAttach on the overlay.
                            overlayRef.OnAttach();

                            break;
                        }
                        default:
                            VERROR("Unknown layer operation type encountered.");
                            break;
                    }
                }

                // Clear the operations after performing them.
                _layerOperations.clear();
            }

            auto begin() {
                return _layers.begin();
            }
            auto end() {
                return _layers.end();
            }
            [[nodiscard]] auto begin() const {
                return _layers.begin();
            }
            [[nodiscard]] auto end() const {
                return _layers.end();
            }

            auto rbegin() {
                return _layers.rbegin();
            }
            auto rend() {
                return _layers.rend();
            }
            [[nodiscard]] auto rbegin() const {
                return _layers.rbegin();
            }
            [[nodiscard]] auto rend() const {
                return _layers.rend();
            }

        private:
            /** @brief The vector of layers in the stack. */
            std::vector<Scope<Layer>> _layers;

            /** @brief The index at which to insert new layers. Overlays are added after this index. */
            u8 _layerInsertIndex = 0;

            /** @brief Operations to be performed on layers in the upcoming render cycle. */
            std::vector<LayerOperation> _layerOperations;
    };
} // namespace Vulkyrie::Core
