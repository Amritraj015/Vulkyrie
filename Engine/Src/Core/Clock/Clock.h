#pragma once

#include "Defines.h"
#include "Platform/Platform.h"

namespace Vkr
{
    class Clock
    {
    private:
        f64 mStartTime{};
        f64 mElapsed{};
        std::shared_ptr<Platform> mPlatform;

    public:
        explicit Clock(std::shared_ptr<Platform> platform);

        inline f64 GetStartTime() const { return mStartTime; }
        inline f64 GetElapsedTime() const { return mElapsed; }

        // Starts the provided clock. Resets elapsed time.
        void Start();

        // Updates the provided clock. Should be called just before checking elapsed time.
        // Has no effect on non-started clocks.
        void Update();

        // Stops the provided clock. Does not reset elapsed time.
        void Stop();
    };
}
