#include "Clock.h"

namespace Vkr
{
    Clock::Clock(const std::shared_ptr<Platform> &platform)
    {
        mPlatform = platform;
    }

    void Clock::Start()
    {
        mStartTime = mPlatform->GetAbsoluteTime();
        mElapsed = 0;
    }

    void Clock::Update()
    {
        if (mStartTime != 0)
        {
            mElapsed = mPlatform->GetAbsoluteTime() - mStartTime;
        }
    }

    void Clock::Stop()
    {
        mStartTime = 0;
    }
}