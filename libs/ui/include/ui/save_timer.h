// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::ui
{

class SaveTimer
{
public:
    // Ticks use the same units as read_ticker().
    void start(int interval_minutes, long now);
    void stop();
    bool running() const;
    bool auto_save_needed(long now);

private:
    long m_save_start{};
    long m_save_ticks{};
    int m_finish_row{-1};
};

void start_save_timer();
void stop_save_timer();
bool auto_save_needed();

} // namespace id::ui
