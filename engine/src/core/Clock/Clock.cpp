#include "Clock.h"
#include "Core/Platform/Platform.h"

namespace Vkr {
	void Clock::Start() {
		mStartTime = Platform::GetAbsoluteTime();
		mElapsed = 0;
	}

	void Clock::Update() {
		if (mStartTime != 0) {
			mElapsed = Platform::GetAbsoluteTime() - mStartTime;
		}
	}

	void Clock::Stop() {
		mStartTime = 0;
	}
}