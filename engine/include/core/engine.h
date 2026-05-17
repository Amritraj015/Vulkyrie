#pragma once

#include "audio/audio_system.h"
#include "core/application.h"
#include "renderer/renderer.h"

namespace Vulkyrie {
    struct EngineConfig {
    public:
        std::filesystem::path WorkingDirectory;
        std::filesystem::path AssetsDirectory;
    };

    class Engine final {
    public:
        Engine(EngineConfig &config);
        ~Engine() = default;

        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;

        Engine(Engine &&) = delete;
        Engine &operator=(Engine &&) = delete;

        void RunApplication(Application &application);

        // /** @brief Gets the native window handle.
        //  * @returns A pointer to the native window.
        //  */
        // [[nodiscard]] inline void *GetWindowHandle() const {
        //     return _platform->GetWindowHandle();
        // }
        //
        // /** @brief Gets the width of the application window.
        //  * @returns The width of the window in pixels.
        //  */
        // [[nodiscard]] inline u32 GetWindowWidth() const {
        //     return _windowProps.Width;
        // }
        //
        // /** @brief Gets the height of the application window.
        //  * @returns The height of the window in pixels.
        //  */
        // [[nodiscard]] inline u32 GetWindowHeight() const {
        //     return _windowProps.Height;
        // }

        /** @brief Gets the current time in seconds since the application started.
         * @returns The current time in seconds.
         */
        [[nodiscard]] f32 GetTime() const {
            return _platform->GetTime();
        }

    private:
        bool _running;
        EngineConfig _config;

        Ref<Platform> _platform;
        Ref<AudioSystem> _audioSystem;
        Ref<Renderer> _renderer;
    };
} // namespace Vulkyrie
