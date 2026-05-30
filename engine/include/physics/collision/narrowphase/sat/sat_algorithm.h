#pragma once

namespace Vulkyrie {

    class SATAlgorithm final {
    public:
        /** @brief Default constructor. */
        SATAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        SATAlgorithm(const SATAlgorithm &) = delete;
        SATAlgorithm &operator=(const SATAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        SATAlgorithm(SATAlgorithm &&) = delete;
        SATAlgorithm &operator=(SATAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~SATAlgorithm() = default;

        bool PerformCollisionCheck();
    };

} // namespace Vulkyrie
