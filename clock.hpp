#pragma once
#include <chrono>
#include <iosfwd>

namespace rs {
	using Clock = std::chrono::steady_clock;
	using Timepoint = typename Clock::time_point;
	using Duration = typename Clock::duration;
	using Hours = std::chrono::hours;
	using Minutes = std::chrono::minutes;
	using Seconds = std::chrono::seconds;
	using Milliseconds = std::chrono::milliseconds;
	using Microseconds = std::chrono::microseconds;
	using Nanoseconds = std::chrono::nanoseconds;

	std::ostream& operator << (std::ostream& os, const Timepoint& t);
	std::ostream& operator << (std::ostream& os, const Duration& t);
}
