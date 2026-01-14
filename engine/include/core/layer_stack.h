#pragma once

#include "layer.h"
#include "core/logger.h"
#include <typeindex>

namespace Vulkyrie::Core {

    constexpr static u8 MAX_LAYER_OPERATIONS = 10;
    constexpr static u8 MAX_LAYERS = 10;

    /** @brief Represents an operation to be performed on the layer stack. */
    struct LayerOperation {
        public:
            /** @brief The type of operation to perform on the layer stack. */
            enum class OperationType : u8 { PushLayer, PopLayer, PushOverlay, PopOverlay } Type;

            /** @brief The layer to push onto the stack. */
            Scope<Layer> LayerToPush;

            /** @brief The target layer to pop from the stack. */
            std::type_index LayerToPop;

            /** @brief Constructs a LayerOperation for pushing a layer or overlay.
             * @param operation The type of operation (PushLayer or PushOverlay).
             * @param layerToPush The layer to push onto the stack.
             */
            explicit LayerOperation(OperationType operation, Scope<Layer> layerToPush)
                : Type(operation), LayerToPop(typeid(void)), LayerToPush(std::move(layerToPush)) {
            }

            /** @brief Constructs a LayerOperation for popping a layer.
             * @param operation The type of operation (PopLayer).
             * @param layerToPop The type index of the layer to pop.
             */
            explicit LayerOperation(OperationType operation, std::type_index layerToPop) : Type(operation), LayerToPop(layerToPop), LayerToPush(nullptr) {
            }
    };

    /** @brief Manages a stack of layers and overlays for the application. */
    class LayerStack {
        public:
            /** @brief Default constructor for the LayerStack class. */
            LayerStack() : _layerInsertIndex(0) {
                // Reserve space for layer operations to minimize reallocations.
                _layerOperations.reserve(MAX_LAYER_OPERATIONS);

                // Reserve space for layers to minimize reallocations.
                _layers.reserve(MAX_LAYERS);
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
                    _layerOperations.emplace_back(LayerOperation::OperationType::PushLayer, CreateScope<TLayer>(std::forward<TArgs>(args)...));
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
                    _layerOperations.emplace_back(LayerOperation::OperationType::PushOverlay, CreateScope<TLayer>(std::forward<TArgs>(args)...));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot push more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Pops a layer from the layer stack.
             * @tparam TLayer The type of layer to pop.
             * @tparam layerId The ID of the layer to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void PopLayer() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PopLayer, std::type_index(typeid(TLayer)));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot pop more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Pops an overlay from the layer stack.
             * @tparam TLayer The type of overlay to pop.
             * @tparam layerId The ID of the overlay to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void PopOverlay() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PopOverlay, std::type_index(typeid(TLayer)));
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
                            if (MAX_LAYERS >= _layers.size()) {
                                // Get a reference to the layer before moving it.
                                auto &layerRef = *operation.LayerToPush;

                                // Insert the layer at the correct position.
                                _layers.emplace(_layers.begin() + _layerInsertIndex, std::move(operation.LayerToPush));

                                // Increment the layer insert index.
                                _layerInsertIndex++;

                                // Call OnAttach on the layer.
                                layerRef.OnAttach();
                            } else {
                                VERROR("Maximum layers exceeded {}. Cannot push more layers.", MAX_LAYERS);
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PopLayer: {
                            for (auto it = _layers.begin(); it != _layers.begin() + _layerInsertIndex; ++it) {
                                auto *layerPtr = it->get();

                                // Check if this is the layer to remove.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {
                                    // Call OnDetach on the layer.
                                    (*it)->OnDetach();

                                    // Remove the layer from the stack.
                                    _layers.erase(it);

                                    // Decrement the layer insert index.
                                    --_layerInsertIndex;

                                    break;
                                }
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PushOverlay: {
                            if (MAX_LAYERS >= _layers.size()) {
                                // Get a reference to the overlay before moving it.
                                auto &overlayRef = *operation.LayerToPush;

                                // Insert the overlay at the end of the layer stack.
                                _layers.emplace_back(std::move(operation.LayerToPush));

                                // Call OnAttach on the overlay.
                                overlayRef.OnAttach();
                            } else {
                                VERROR("Maximum layers exceeded {}. Cannot push overlay.", MAX_LAYERS);
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PopOverlay: {
                            for (auto it = _layers.begin() + _layerInsertIndex; it != _layers.end(); ++it) {
                                auto *layerPtr = it->get();

                                // Check if this is the overlay to remove.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {
                                    // Call OnDetach on the overlay.
                                    (*it)->OnDetach();

                                    // Remove the overlay from the stack.
                                    _layers.erase(it);

                                    break;
                                }
                            }

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
