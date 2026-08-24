#pragma once

#include "core/platform.h"
#include "core/types/application_types.h"
#include "core/layer_stack.h"
#include "events/application/window_created_event.h"
#include "events/application/window_resized_event.h"
#include "renderer/renderer.h"

namespace Vulkyrie {

    /** @brief The Application class represents the main application and manages the application lifecycle,
     * including window creation, event handling, and layer management. */
    class Application {
    public:
        /** @brief Constructs a new Application with the provided settings.
         * @param appSettings The settings for the application to be created.
         */
        explicit Application(const ApplicationSettings &appSettings);

        /** @brief Destructor to clean up the application and its resources. */
        virtual ~Application() = default;

        /** @brief Gets the singleton instance of the application.
         * @returns A reference to the application instance.
         */
        [[nodiscard]] VE_INLINE static Application &GetSingleton() {
            return *sInstance;
        }

        /** @brief Pushes a new layer onto the layer stack if the application is running.
         * @tparam TLayer The type of layer to push.
         * @param args Arguments to forward to the layer's constructor.
         */
        template <typename TLayer, typename... TArgs>
            requires std::derived_from<TLayer, Layer>
        void PushLayer(TArgs &&...args) {
            mLayers.QueuePushLayerOperation<TLayer>(std::forward<TArgs>(args)...);
        }

        /** @brief Pushes a new overlay onto the layer stack if the application is running.
         * @tparam TLayer The type of overlay to push.
         * @param args Arguments to forward to the overlay's constructor.
         */
        template <typename TLayer, typename... TArgs>
            requires std::derived_from<TLayer, Layer>
        void PushOverlay(TArgs &&...args) {
            mLayers.QueuePushOverlayOperation<TLayer>(std::forward<TArgs>(args)...);
        }

        /** @brief Pops a layer from the layer stack.
         * @tparam TLayer The type of layer to pop.
         * @tparam layerId The ID of the layer to pop.
         */
        template <typename TLayer>
            requires std::derived_from<TLayer, Layer>
        void PopLayer() {
            return mLayers.QueuePopLayerOperation<TLayer>();
        }

        /** @brief Pops an overlay from the layer stack.
         * @tparam TLayer The type of overlay to pop.
         * @tparam layerId The ID of the overlay to pop.
         */
        template <typename TLayer>
            requires std::derived_from<TLayer, Layer>
        void PopOverlay() {
            return mLayers.QueuePopOverlayOperation<TLayer>();
        }

        /** @brief Suspends a layer in the layer stack.
         * @tparam TLayer The type of layer to suspend.
         */
        template <typename TLayer>
            requires(std::derived_from<TLayer, Layer>)
        void SuspendLayer() {
            return mLayers.QueueSuspendLayerOperation<TLayer>();
        }

        /** @brief Resumes a suspended layer in the layer stack.
         * @tparam TLayer The type of layer to resume.
         */
        template <typename TLayer>
            requires(std::derived_from<TLayer, Layer>)
        void ResumeLayer() {
            return mLayers.QueueResumeLayerOperation<TLayer>();
        }

        /** @brief Checks if a layer of the specified type exists in the active or suspended layer stack.
         * @tparam TLayer The type of layer to check for.
         * @returns True if the layer exists, false otherwise.
         */
        template <typename TLayer>
            requires std::derived_from<TLayer, Layer>
        [[nodiscard]] bool HasLayer() const {
            return mLayers.HasLayer<TLayer>();
        }

        /** @brief Gets a layer of the specified type from the layer stack.
         * @tparam TLayer The type of layer to get.
         * @returns A pointer to the layer if found, nullptr otherwise.
         */
        template <typename TLayer>
            requires(std::derived_from<TLayer, Layer>)
        [[nodiscard]] const TLayer *GetLayer() {
            return mLayers.GetActiveLayer<TLayer>();
        }

        /** @brief Gets the native window handle.
         * @returns A pointer to the native window.
         */
        [[nodiscard]] VE_INLINE void *GetWindowHandle() const {
            return mPlatform->GetWindowHandle();
        }

        /** @brief Gets the dimensions of the application window.
         * @returns The dimensions of the window in pixels.
         */
        [[nodiscard]] VE_INLINE Extent2D GetWindowDimensions() const {
            return mAppSettings.GraphicsSettings.WindowDimensions;
        }

        /** @brief Gets the current time in seconds since the application started.
         * @returns The current time in seconds.
         */
        [[nodiscard]] VE_INLINE f32 GetTime() const {
            return mPlatform->GetTime();
        }

        /** @brief Sets whether the application should capture the mouse when the window is focused.
         * @param capture True to capture the mouse on focus, false to not capture.
         */
        void VE_INLINE CaptureMouseOnFocus(bool capture) const {
            mPlatform->CaptureMouseOnFocus(capture);
        }

        /** @brief Starts the application's main loop.
         * @returns StatusCode indicating success or failure.
         */
        [[nodiscard]] StatusCode Run();

        /** @brief Stops the application. */
        void Stop();

    protected:
        /** @brief Called when the application window is created.
         * @param event The window created event.
         * @returns True if the event was handled successfully, false otherwise.
         */
        virtual bool OnInit(WindowCreatedEvent &event);

    private:
        /** @brief The singleton instance of the application. */
        static Application *sInstance;

        /** @brief Settings for this application. */
        ApplicationSettings mAppSettings;

        /** @brief The main application window. */
        Scope<Platform> mPlatform;

        /** @brief The application renderer. */
        Scope<Renderer> mRenderer;

        /** @brief Indicates whether the application is running. */
        bool mRunning;

        /** @brief The layers in the stack. */
        LayerStack mLayers;

        /** @brief Raises an event to be handled by the application or other systems.
         * @param event The event to raise.
         */
        void OnEvent(Event &event);

        /** @brief Handles window resize events.
         * @param event The window resized event.
         * @returns True if the event was handled successfully, false otherwise.
         */
        bool OnWindowResized(const WindowResizedEvent &event);
    };

} // namespace Vulkyrie
