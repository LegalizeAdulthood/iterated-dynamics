// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::io
{

enum class TimedSave
{
    NONE = 0,
    STARTED = 1,
    FINAL = 2,
};

extern TimedSave             g_resave_flag;
extern int                   g_save_time_interval;
extern bool                  g_started_resaves;
extern TimedSave             g_timed_save;

} // namespace id::io
