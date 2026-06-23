// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/fractalp.h"

namespace id::engine
{

class StandardFractal
{
public:
    void resume();

    void suspend();

    bool done() const;

    void iterate();

private:
    enum class Phase
    {
        START,
        WORK_LIST,
        COMPLETE
    };

    enum class AfterWorkList
    {
        COMPLETE,
        START_THREE_PASS_FOLLOWUP
    };

    void complete();
    void finish_work_list();
    void pop_work_list_front();
    void restore_dispatch();
    void run_current_work_item();
    void run_current_work_item_mode();
    void start_next_pass();
    void start_timer();
    void start_work_list();
    void update_timer();

    fractals::FractalDispatch m_saved_dispatch{};
    CalcMode m_requested_calc_mode{};
    AfterWorkList m_after_work_list{AfterWorkList::COMPLETE};
    Phase m_phase{Phase::START};
    bool m_dispatch_saved{};
    bool m_timer_started{};
    bool m_work_list_started{};
};

} // namespace id::engine
