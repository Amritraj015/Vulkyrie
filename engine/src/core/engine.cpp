#include "core/engine.h"

namespace Vulkyrie::Core {

    Engine::Engine(EngineConfig &config)
        : _config(config)
        , _audioSystem(CreateRef<Vulkyrie::Audio::AudioSystem>()) {
    }

    void Engine::RunApplication(Vulkyrie::Core::Application &application) {
        _running = true;

        while (_running) {
            application.Run();
        }
    }

} // namespace Vulkyrie::Core
