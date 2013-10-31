#pragma once
#include <chrono>

namespace rs {
	using Clock = std::chrono::steady_clock;
	using Timepoint = typename Clock::time_point;
	using Duration = typename Clock::duration;
}
