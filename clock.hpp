#pragma once

#include <chrono>
using Clock = std::chrono::steady_clock;
using Timepoint = typename Clock::time_point;
using Duration = typename Clock::duration;
