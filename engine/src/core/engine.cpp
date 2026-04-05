#include "core/engine.h"

namespace Vulkyrie {

    Engine::Engine(EngineConfig &config)
        : _config(config)
        , _audioSystem(CreateRef<AudioSystem>()) {
    }

    void Engine::RunApplication(Application &application) {
        _running = true;

        while (_running) {
            application.Run();
        }
    }

} // namespace Vulkyrie
