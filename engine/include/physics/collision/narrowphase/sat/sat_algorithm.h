#pragma once

namespace Vulkyrie {

    class SATAlgorithm final {
        public:
            SATAlgorithm() = default;

            SATAlgorithm(const SATAlgorithm &) = delete;
            SATAlgorithm &operator=(const SATAlgorithm &) = delete;

            SATAlgorithm(SATAlgorithm &&) = delete;
            SATAlgorithm &operator=(SATAlgorithm &&) = delete;

            ~SATAlgorithm() = default;

            bool PerformCollisionCheck();
    };

} // namespace Vulkyrie
