// SPDX-License-Identifier: GPL-3.0-only
//

#include "ui/read_ticker.h"

#include <chrono>
#include <ctime>

namespace id::ui
{

namespace
{

using TickerClock = std::chrono::steady_clock;
using TickerDuration = std::chrono::duration<long, std::ratio<1, CLOCKS_PER_SEC>>;

const TickerClock::time_point TICKER_START{TickerClock::now()};

} // namespace

// returns current ticker value
long read_ticker()
{
    return std::chrono::duration_cast<TickerDuration>(TickerClock::now() - TICKER_START).count();
}

} // namespace id::ui
