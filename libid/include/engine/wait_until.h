// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

void wait_until(unsigned long wait_time_100us);
void reset_wait_until();
void sleep_orbit_delay(unsigned long delay_100us);
void sleep_ms(long ms);

} // namespace id::engine
