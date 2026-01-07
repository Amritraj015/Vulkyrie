#pragma once

#include "core/platform.h"
#include "core/window_props.h"
#include "core/application_config.h"
#include "core/layer_stack.h"
#include "events/application/window_closed_event.h"
#include "events/application/window_created_event.h"
#include "events/application/window_resized_event.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Core {
    class Application {
        public:
            /** @brief Constructs a new Application with the given window properties and configuration.
             * @param windowProps The properties for the application window.
             * @param config The configuration for the application.
             */
            Application(const WindowProps &windowProps, const ApplicationConfig &config);

            /** @brief Destructor to clean up the application and its resources. */
            virtual ~Application();

            /** @brief Starts the application's main loop.
             * @returns Vulkyrie::Core::StatusCode indicating success or failure.
             */
            StatusCode Run();

            /** @brief Stops the application. */
            void Stop();

            /** @brief Pushes a new layer onto the layer stack if the application is running.
             * @tparam TLayer The type of layer to push.
             * @param args Arguments to forward to the layer's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushLayer(TArgs &&...args) {
                if (_running) {
                    _layers.PushLayer<TLayer>(*this, std::forward<TArgs>(args)...);
                } else {
                    VERROR("Cannot push layer while application is not running, it is recommended that you make changes to the layer "
                           "stack after the 'OnInit' lifecycle hook.")
                }
            }

            /** @brief Pops a layer from the layer stack.
             * @tparam TLayer The type of layer to pop.
             * @returns True if the layer was found and removed, false otherwise.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            bool PopLayer() {
                return _layers.PopLayer<TLayer>();
            }

            /** @brief Pushes a new overlay onto the layer stack if the application is running.
             * @tparam TLayer The type of overlay to push.
             * @param args Arguments to forward to the overlay's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushOverlay(TArgs &&...args) {
                if (_running) {
                    _layers.PushOverlay<TLayer>(*this, std::forward<TArgs>(args)...);
                } else {
                    VERROR("Cannot push overlay while application is not running, it is recommended that you make changes to the layer "
                           "stack after the 'OnInit' lifecycle hook.")
                }
            }

            /** @brief Pops an overlay from the layer stack.
             * @tparam TLayer The type of overlay to pop.
             * @returns True if the overlay was found and removed, false otherwise.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            bool PopOverlay() {
                return _layers.PopOverlay<TLayer>();
            }

            /** @brief Gets a layer of the specified type from the layer stack.
             * @tparam TLayer The type of layer to get.
             * @returns A pointer to the layer if found, nullptr otherwise.
             */
            template <typename TLayer>
                requires(std::is_base_of_v<Layer, TLayer>)
            TLayer *GetLayer() {
                return _layers.GetLayer<TLayer>();
            }

            /** @brief Checks if a specific key is currently pressed.
             * @param key The key code to check.
             * @returns True if the key is pressed, false otherwise.
             */
            [[nodiscard]] inline bool IsKeyPressed(const Vulkyrie::Events::KeyCode key) const {
                return _platform->IsKeyPressed(key);
            }

            /** @brief Checks if a specific mouse button is currently pressed.
             * @param button The mouse button to check.
             * @returns True if the mouse button is pressed, false otherwise.
             */
            [[nodiscard]] inline bool IsMouseButtonPressed(Vulkyrie::Events::MouseButton button) const {
                return _platform->IsMouseButtonPressed(button);
            }

            /** @brief Gets the current position of the mouse cursor.
             * @returns A pair of floats representing the X and Y coordinates of the mouse cursor.
             */
            [[nodiscard]] inline std::pair<f32, f32> GetMousePosition() const {
                return _platform->GetMousePosition();
            }

            /** @brief Gets the current X position of the mouse cursor.
             * @returns The X coordinate of the mouse cursor.
             */
            [[nodiscard]] inline f32 GetMouseX() const {
                return _platform->GetMouseX();
            }

            /** @brief Gets the current Y position of the mouse cursor.
             * @returns The Y coordinate of the mouse cursor.
             */
            [[nodiscard]] inline f32 GetMouseY() const {
                return _platform->GetMouseY();
            }

        protected:
            /** @brief Called when the application window is created.
             * @param event The window created event.
             * @returns True if the event was handled successfully, false otherwise.
             */
            virtual bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event);

        private:
            /** @brief The main application window. */
            Ref<Platform> _platform;

            /** @brief Window properties for the application. */
            WindowProps _windowProps;

            /** @brief The application configuration. */
            ApplicationConfig _config;

            /** @brief Indicates whether the application is running. */
            bool _running;

            /** @brief The time of the last frame, used for timestep calculations. */
            f32 _lastFrameTime;

            /** @brief The layers in the stack. */
            LayerStack _layers;

            /** @brief The renderer used for rendering graphics. */
            Ref<Vulkyrie::Renderer::Renderer> _renderer;

            /** @brief Raises an event to be handled by the application or other systems.
             * @param event The event to raise.
             */
            void OnEvent(Vulkyrie::Events::Event &event);

            /** @brief Stops the application and cleans up resources.
             * @param event The window closed event.
             * @returns a `false` value to allow the window close event to propagate further.
             */
            bool OnWindowClosed(Vulkyrie::Events::WindowClosedEvent &event);

            /** @brief Handles window resize events.
             * @param event The window resized event.
             * @returns True if the event was handled successfully, false otherwise.
             */
            bool OnWindowResized(const Vulkyrie::Events::WindowResizedEvent &event);
    };
} // namespace Vulkyrie::Core
