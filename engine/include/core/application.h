#pragma once

#include "window_props.h"
#include "application_config.h"
#include "core/layer.h"

namespace Vulkyrie::Core {
    class Application {
        public:
            Application(const WindowProps &windowProps, const ApplicationConfig &config)
                : windowProps(windowProps), config(config), _layerInsertIndex(0), _running(true) {
            }
            ~Application();

            /** @brief Window properties for the application. */
            WindowProps windowProps;

            /** @brief The application configuration. */
            ApplicationConfig config;

            /** @brief Raises an event to be handled by the application layers.
             * @param event The event to raise.
             */
            void RaiseEvent(Vulkyrie::Events::Event &event) const;

            /** @brief Terminates the application and cleans up resources. */
            void Terminate();

            /** @brief Pushes a new layer onto the layer stack.
             * @param TLayer The type of layer to push.
             * @param args Arguments to forward to the layer's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushLayer(TArgs &&...args) {
                // Create the layer.
                auto layer = std::make_unique<TLayer>(std::forward<TArgs>(args)...);

                // Keep a reference to the layer for OnAttach call after moving.
                TLayer &reference = *layer;

                // Insert the layer at the correct position.
                _layers.emplace(_layers.begin() + _layerInsertIndex, std::move(layer));

                // Increment the layer insert index.
                _layerInsertIndex++;

                // Call OnAttach on the layer.
                reference.OnAttach();
            }

            /** @brief Pops a layer from the layer stack.
             * @param TLayer The type of layer to pop.
             * @returns True if the layer was found and removed, false otherwise.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            bool PopLayer() {
                for (auto it = _layers.begin(); it != _layers.begin() + _layerInsertIndex; ++it) {
                    // Check if this is the layer to remove.
                    if (nullptr != dynamic_cast<TLayer *>(it->get())) {
                        // Call OnDetach on the layer.
                        (*it)->OnDetach();

                        // Remove the layer from the stack.
                        _layers.erase(it);

                        // Decrement the layer insert index.
                        --_layerInsertIndex;

                        // Return true to indicate success.
                        return true;
                    }
                }

                // Layer not found, return false.
                return false;
            }

            /** @brief Pushes a new overlay onto the layer stack.
             * @param TLayer The type of overlay to push.
             * @param args Arguments to forward to the overlay's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushOverlay(TArgs &&...args) {
                // Create the overlay.
                auto overlay = std::make_unique<TLayer>(std::forward<TArgs>(args)...);

                // Keep a reference to the overlay for OnAttach call after moving.
                TLayer &reference = *overlay;

                // Insert the overlay at the end of the layer stack.
                _layers.emplace_back(std::move(overlay));

                // Call OnAttach on the overlay.
                reference.OnAttach();
            }

            /** @brief Pops an overlay from the layer stack.
             * @param TLayer The type of overlay to pop.
             * @returns True if the overlay was found and removed, false otherwise.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            bool PopOverlay() {
                for (auto it = _layers.begin() + _layerInsertIndex; it != _layers.end(); ++it) {
                    // Check if this is the overlay to remove.
                    if (nullptr != dynamic_cast<TLayer *>(it->get())) {

                        // Call OnDetach on the overlay.
                        (*it)->OnDetach();

                        // Remove the overlay from the stack.
                        _layers.erase(it);

                        // Return true to indicate success.
                        return true;
                    }
                }

                // Overlay not found, return false.
                return false;
            }

            /** @brief Gets a layer of the specified type from the layer stack.
             * @param TLayer The type of layer to get.
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

            auto begin() {
                return _layers.begin();
            }
            auto end() {
                return _layers.end();
            }
            auto begin() const {
                return _layers.begin();
            }
            auto end() const {
                return _layers.end();
            }

            auto rbegin() {
                return _layers.rbegin();
            }
            auto rend() {
                return _layers.rend();
            }
            auto rbegin() const {
                return _layers.rbegin();
            }
            auto rend() const {
                return _layers.rend();
            }

        private:
            bool _running;
            /** @brief The layers in the stack. */
            std::vector<std::unique_ptr<Layer>> _layers;

            /** @brief The index at which to insert new layers. Overlays are inserted after this index. */
            size_t _layerInsertIndex;
    };
} // namespace Vulkyrie::Core
