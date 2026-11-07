// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/save_timer.h>

namespace id::io
{

TimedSave g_resave_flag{};  // tells encoder not to incr filename
int g_save_time_interval{}; // autosave minutes
bool g_started_resaves{};   // but incr on first resave
TimedSave g_timed_save{};   // when doing a timed save

} // namespace id::io
