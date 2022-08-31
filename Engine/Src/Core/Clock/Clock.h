#pragma once

#include "Defines.h"

namespace Vkr
{
    class Clock
    {
    private:
        f64 mStartTime;
        f64 mElapsed;

    public:
        CONSTRUCTOR_LOG(Clock)
        DESTRUCTOR_LOG(Clock)

        // Starts the provided clock. Resets elapsed time.
        void Start();

        // Updates the provided clock. Should be called just before checking elapsed time.
        // Has no effect on non-started clocks.
        void Update();

        // Stops the provided clock. Does not reset elapsed time.
        void Stop();

        inline f64 GetStartTime() const { return mStartTime; }

        inline double GetElapsedTime() const { return mElapsed; }
    };
}
