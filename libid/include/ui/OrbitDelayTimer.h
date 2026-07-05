// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::ui
{

using OrbitDelayInputPendingFn = bool();

constexpr unsigned long ORBIT_DELAY_UNIT_MICROSECONDS = 100;

void reset_orbit_delay_timer();
bool wait_for_orbit_delay(unsigned long delay_100us, OrbitDelayInputPendingFn &input_pending);

} // namespace id::ui
