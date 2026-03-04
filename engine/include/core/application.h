#pragma once

#include "core/platform.h"
#include "core/window_props.h"
#include "core/layer_stack.h"
#include "events/application/window_closed_event.h"
#include "events/application/window_created_event.h"
#include "events/application/window_resized_event.h"

namespace Vulkyrie::Core {

    /** @brief The Application class represents the main application and manages the application lifecycle,
     * including window creation, event handling, and layer management. */
    class Application {
        public:
            /** @brief Constructs a new Application with the given window properties and configuration.
             * @param windowProps The properties for the application window.
             */
            Application(WindowProps windowProps);

            /** @brief Destructor to clean up the application and its resources. */
            virtual ~Application() = default;

            /** @brief Gets the singleton instance of the application.
             * @returns A reference to the application instance.
             */
            [[nodiscard]] inline static Application &GetSingleton() {
                return *_instance;
            }

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
                _layers.QueuePushLayerOperation<TLayer>(std::forward<TArgs>(args)...);
            }

            /** @brief Pushes a new overlay onto the layer stack if the application is running.
             * @tparam TLayer The type of overlay to push.
             * @param args Arguments to forward to the overlay's constructor.
             */
            template <typename TLayer, typename... TArgs>
                requires std::derived_from<TLayer, Layer>
            void PushOverlay(TArgs &&...args) {
                _layers.QueuePushOverlayOperation<TLayer>(std::forward<TArgs>(args)...);
            }

            /** @brief Pops a layer from the layer stack.
             * @tparam TLayer The type of layer to pop.
             * @tparam layerId The ID of the layer to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void PopLayer() {
                return _layers.QueuePopLayerOperation<TLayer>();
            }

            /** @brief Pops an overlay from the layer stack.
             * @tparam TLayer The type of overlay to pop.
             * @tparam layerId The ID of the overlay to pop.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            void PopOverlay() {
                return _layers.QueuePopOverlayOperation<TLayer>();
            }

            /** @brief Suspends a layer in the layer stack.
             * @tparam TLayer The type of layer to suspend.
             */
            template <typename TLayer>
                requires(std::derived_from<TLayer, Layer>)
            void SuspendLayer() {
                return _layers.QueueSuspendLayerOperation<TLayer>();
            }

            /** @brief Resumes a suspended layer in the layer stack.
             * @tparam TLayer The type of layer to resume.
             */
            template <typename TLayer>
                requires(std::derived_from<TLayer, Layer>)
            void ResumeLayer() {
                return _layers.QueueResumeLayerOperation<TLayer>();
            }

            /** @brief Checks if a layer of the specified type exists in the active or suspended layer stack.
             * @tparam TLayer The type of layer to check for.
             * @returns True if the layer exists, false otherwise.
             */
            template <typename TLayer>
                requires std::derived_from<TLayer, Layer>
            [[nodiscard]] bool HasLayer() const {
                return _layers.HasLayer<TLayer>();
            }

            /** @brief Gets a layer of the specified type from the layer stack.
             * @tparam TLayer The type of layer to get.
             * @returns A pointer to the layer if found, nullptr otherwise.
             */
            template <typename TLayer>
                requires(std::derived_from<TLayer, Layer>)
            [[nodiscard]] const TLayer *GetLayer() {
                return _layers.GetActiveLayer<TLayer>();
            }

            /** @brief Gets the native window handle.
             * @returns A pointer to the native window.
             */
            [[nodiscard]] inline void *GetWindowHandle() const {
                return _platform->GetWindowHandle();
            }

            /** @brief Gets the width of the application window.
             * @returns The width of the window in pixels.
             */
            [[nodiscard]] inline u32 GetWindowWidth() const {
                return _windowProps.Width;
            }

            /** @brief Gets the height of the application window.
             * @returns The height of the window in pixels.
             */
            [[nodiscard]] inline u32 GetWindowHeight() const {
                return _windowProps.Height;
            }

            /** @brief Gets the current time in seconds since the application started.
             * @returns The current time in seconds.
             */
            [[nodiscard]] f32 GetTime() const {
                return _platform->GetTime();
            }

            /** @brief Sets whether the application should capture the mouse when the window is focused.
             * @param capture True to capture the mouse on focus, false to not capture.
             */
            void CaptureMouseOnFocus(bool capture) const {
                _platform->CaptureMouseOnFocus(capture);
            }

        protected:
            /** @brief Called when the application window is created.
             * @param event The window created event.
             * @returns True if the event was handled successfully, false otherwise.
             */
            virtual bool OnInit(Vulkyrie::Events::WindowCreatedEvent &event);

        private:
            /** @brief The singleton instance of the application. */
            static Application *_instance;

            /** @brief The main application window. */
            Ref<Platform> _platform;

            /** @brief Window properties for the application. */
            WindowProps _windowProps;

            /** @brief Indicates whether the application is running. */
            bool _running;

            /** @brief The layers in the stack. */
            LayerStack _layers;

            /** @brief Raises an event to be handled by the application or other systems.
             * @param event The event to raise.
             */
            void OnEvent(Vulkyrie::Events::Event &event);

            /** @brief Handles window resize events.
             * @param event The window resized event.
             * @returns True if the event was handled successfully, false otherwise.
             */
            bool OnWindowResized(const Vulkyrie::Events::WindowResizedEvent &event);
    };

} // namespace Vulkyrie::Core
