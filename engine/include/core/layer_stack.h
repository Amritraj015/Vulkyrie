#pragma once

#include "layer.h"
#include <typeindex>

namespace Vulkyrie::Core {

    constexpr static u8 MAX_LAYER_OPERATIONS = 20;
    constexpr static u8 MAX_LAYERS = 20;

    /** @brief Represents an operation to be performed on the layer stack. */
    struct LayerOperation {
        public:
            /** @brief The type of operation to perform on the layer stack. */
            enum class OperationType : u32 {
                PushLayer,
                PopLayer,
                PushOverlay,
                PopOverlay,
                SuspendLayer,
                ResumeLayer,
            } Type;

            /** @brief The layer to push onto the stack. */
            Scope<Layer> LayerToPush;

            /** @brief The target layer to pop from the stack. */
            std::type_index LayerToPop;

            /** @brief Constructs a LayerOperation for pushing a layer or overlay.
             * @param operation The type of operation (PushLayer or PushOverlay).
             * @param layerToPush The layer to push onto the stack.
             */
            explicit LayerOperation(OperationType operation, Scope<Layer> layerToPush)
                : Type(operation)
                , LayerToPush(std::move(layerToPush))
                , LayerToPop(typeid(void)) {
            }

            /** @brief Constructs a LayerOperation for popping a layer.
             * @param operation The type of operation (PopLayer).
             * @param layerToPop The type index of the layer to pop.
             */
            explicit LayerOperation(OperationType operation, std::type_index layerToPop)
                : Type(operation)
                , LayerToPush(nullptr)
                , LayerToPop(layerToPop) {
            }
    };

    /** @brief Manages a stack of layers and overlays for the application. */
    class LayerStack {
        public:
            /** @brief Default constructor for the LayerStack class. */
            LayerStack()
                : _layerInsertIndex(0) {
                // Reserve space for layer operations to minimize reallocations.
                _layerOperations.reserve(MAX_LAYER_OPERATIONS);

                // Reserve space for layers to minimize reallocations.
                _activeLayers.reserve(MAX_LAYERS);
                _suspendedLayers.reserve(MAX_LAYERS);
            }

            /** @brief Destructor to clean up the layer stack and detach all layers. */
            ~LayerStack() {
                for (auto &layer : _activeLayers) {
                    layer->OnDetached();
                }

                for (auto &layer : _suspendedLayers) {
                    layer->OnDetached();
                }
            }

            /** @brief Queues a layer to be pushed onto the layer stack.
             * @tparam TLayer The type of layer to push.
             * @param args Arguments to forward to the layer's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void QueuePushLayerOperation(TArgs &&...args) {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PushLayer, CreateScope<TLayer>(std::forward<TArgs>(args)...));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot push more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Queues a new overlay to be pushed onto the layer stack.
             * @tparam TLayer The type of overlay to push.
             * @param args Arguments to forward to the overlay's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void QueuePushOverlayOperation(TArgs &&...args) {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PushOverlay, CreateScope<TLayer>(std::forward<TArgs>(args)...));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot push more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Queues a layer to be popped from the layer stack.
             * @tparam TLayer The type of layer to pop.
             * @tparam layerId The ID of the layer to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void QueuePopLayerOperation() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PopLayer, std::type_index(typeid(TLayer)));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot pop more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Queues an overlay to be popped from the layer stack.
             * @tparam TLayer The type of overlay to pop.
             * @tparam layerId The ID of the overlay to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void QueuePopOverlayOperation() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::PopOverlay, std::type_index(typeid(TLayer)));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot pop more layers or overlays this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Queues a layer to be suspended in the layer stack.
             * @tparam TLayer The type of layer to suspend.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void QueueSuspendLayerOperation() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::SuspendLayer, std::type_index(typeid(TLayer)));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot suspend more layers this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Queues a layer to be resumed in the layer stack.
             * @tparam TLayer The type of layer to resume.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void QueueResumeLayerOperation() {
                if (MAX_LAYER_OPERATIONS >= _layerOperations.size()) {
                    _layerOperations.emplace_back(LayerOperation::OperationType::ResumeLayer, std::type_index(typeid(TLayer)));
                } else {
                    VERROR("Maximum layer operations exceeded {}. Cannot resume more layers this frame.", MAX_LAYER_OPERATIONS);
                }
            }

            /** @brief Checks if a layer of the specified type exists in the active or suspended layer stack.
             * @tparam TLayer The type of layer to check for.
             * @returns True if the layer exists, false otherwise.
             */
            template <typename TLayer>
                requires(std::derived_from<TLayer, Layer>)
            bool HasLayer() const {
                // Check active layers.
                for (auto it = _activeLayers.begin(); it != _activeLayers.begin() + _layerInsertIndex; ++it) {
                    auto *layerPtr = it->get();

                    if (std::type_index(typeid(*layerPtr)) == std::type_index(typeid(TLayer))) {
                        return true;
                    }
                }

                // Check suspended layers.
                for (auto it = _suspendedLayers.begin(); it != _suspendedLayers.end(); ++it) {
                    auto *layerPtr = it->get();

                    if (std::type_index(typeid(*layerPtr)) == std::type_index(typeid(TLayer))) {
                        return true;
                    }
                }

                // Layer not found.
                return false;
            }

            /** @brief Gets an active/attached layer of the specified type from the layer stack.
             * @tparam TLayer The type of layer to get.
             * @returns A pointer to the layer if found, nullptr otherwise.
             */
            template <typename TLayer>
                requires(std::derived_from<TLayer, Layer>)
            const TLayer *GetActiveLayer() {
                for (const auto &layer : _activeLayers) {
                    if (auto casted = dynamic_cast<TLayer *>(layer.get())) {
                        return casted;
                    }
                }

                return nullptr;
            }

            /** @brief Processes queued operations on the layer stack. */
            inline void ProcessQueuedOperations() {
                if (_layerOperations.empty()) {
                    return;
                }

                for (auto &operation : _layerOperations) {
                    switch (operation.Type) {
                        case LayerOperation::OperationType::PushLayer: {
                            if (MAX_LAYERS >= _activeLayers.size()) {
                                // Get a reference to the layer before moving it.
                                auto &layerRef = *operation.LayerToPush;

                                // Insert the layer at the correct position.
                                _activeLayers.emplace(_activeLayers.begin() + _layerInsertIndex, std::move(operation.LayerToPush));

                                // Increment the layer insert index.
                                _layerInsertIndex++;

                                // Call OnAttach on the layer.
                                layerRef.OnAttached();
                            } else {
                                VERROR("Maximum layers exceeded {}. Cannot push more layers.", MAX_LAYERS);
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PopLayer: {
                            for (auto it = _activeLayers.begin(); it != _activeLayers.begin() + _layerInsertIndex; ++it) {
                                auto *layerPtr = it->get();

                                // Check if this is the layer to remove.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {
                                    // Call OnDetach on the layer.
                                    (*it)->OnDetached();

                                    // Remove the layer from the stack.
                                    _activeLayers.erase(it);

                                    // Decrement the layer insert index.
                                    --_layerInsertIndex;

                                    break;
                                }
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PushOverlay: {
                            if (MAX_LAYERS >= _activeLayers.size()) {
                                // Get a reference to the overlay before moving it.
                                auto &overlayRef = *operation.LayerToPush;

                                // Insert the overlay at the end of the layer stack.
                                _activeLayers.emplace_back(std::move(operation.LayerToPush));

                                // Call OnAttach on the overlay.
                                overlayRef.OnAttached();
                            } else {
                                VERROR("Maximum layers exceeded {}. Cannot push overlay.", MAX_LAYERS);
                            }

                            break;
                        }
                        case LayerOperation::OperationType::PopOverlay: {
                            for (auto it = _activeLayers.begin() + _layerInsertIndex; it != _activeLayers.end(); ++it) {
                                auto *layerPtr = it->get();

                                // Check if this is the overlay to remove.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {
                                    // Call OnDetach on the overlay.
                                    (*it)->OnDetached();

                                    // Remove the overlay from the stack.
                                    _activeLayers.erase(it);

                                    break;
                                }
                            }

                            break;
                        }
                        case LayerOperation::OperationType::SuspendLayer: {
                            for (auto it = _activeLayers.begin(); it != _activeLayers.begin() + _layerInsertIndex; ++it) {
                                auto *layerPtr = it->get();

                                // Check if this is the layer to suspend.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {

                                    // Call OnSuspended on the layer.
                                    (*it)->OnSuspended();

                                    // Move the layer to the suspended layers list.
                                    _suspendedLayers.emplace_back(std::move(*it));

                                    // Remove the layer from the active layers stack.
                                    _activeLayers.erase(it);

                                    // Decrement the layer insert index.
                                    --_layerInsertIndex;

                                    break;
                                }
                            }

                            break;
                        }
                        case LayerOperation::OperationType::ResumeLayer: {
                            for (auto it = _suspendedLayers.begin(); it != _suspendedLayers.end(); ++it) {
                                auto *layerPtr = it->get();
                                // Check if this is the layer to resume.
                                if (std::type_index(typeid(*layerPtr)) == operation.LayerToPop) {
                                    // Get a reference to the layer before moving it.
                                    auto &layerRef = **it;

                                    // Insert the layer back into the active layers stack at the correct position.
                                    _activeLayers.emplace(_activeLayers.begin() + _layerInsertIndex, std::move(*it));

                                    // Increment the layer insert index.
                                    ++_layerInsertIndex;

                                    // Call OnResumed on the layer.
                                    layerRef.OnResumed();

                                    // Remove the layer from the suspended layers list.
                                    _suspendedLayers.erase(it);

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
                return _activeLayers.begin();
            }
            auto end() {
                return _activeLayers.end();
            }
            [[nodiscard]] auto begin() const {
                return _activeLayers.begin();
            }
            [[nodiscard]] auto end() const {
                return _activeLayers.end();
            }

            auto rbegin() {
                return _activeLayers.rbegin();
            }
            auto rend() {
                return _activeLayers.rend();
            }
            [[nodiscard]] auto rbegin() const {
                return _activeLayers.rbegin();
            }
            [[nodiscard]] auto rend() const {
                return _activeLayers.rend();
            }

        private:
            /** @brief List of layers in the stack. */
            std::vector<Scope<Layer>> _activeLayers;

            /** @brief List of suspended layers. */
            std::vector<Scope<Layer>> _suspendedLayers;

            /** @brief Operations to be performed on layers in the upcoming render cycle. */
            std::vector<LayerOperation> _layerOperations;

            /** @brief The index at which to insert new layers. Overlays are added after this index. */
            u8 _layerInsertIndex = 0;
    };
} // namespace Vulkyrie::Core
