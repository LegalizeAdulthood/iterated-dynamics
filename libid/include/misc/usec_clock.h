// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// TODO: replace with <chrono> clock.
using uclock_t = unsigned long;
uclock_t usec_clock();
void restart_uclock();
