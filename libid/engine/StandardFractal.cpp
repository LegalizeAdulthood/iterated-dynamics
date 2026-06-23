// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StandardFractal.h"

#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/LogicalScreen.h"
#include "engine/resume.h"
#include "misc/ValueSaver.h"

namespace id::engine
{

void perform_work_list();

namespace
{

int timer_work_list()
{
    perform_work_list();
    return 0;
}

CalcMode followup_calc_mode()
{
    return g_logical_screen.x_dots >= 640 ? CalcMode::TWO_PASS : CalcMode::ONE_PASS;
}

} // namespace

void StandardFractal::iterate()
{
    if (done())
    {
        return;
    }

    if (g_std_calc_mode == CalcMode::THREE_PASS) // convoluted 'g' + '2' hybrid
    {
        misc::ValueSaver saved_calc_mode{g_std_calc_mode};
        if (!g_resuming || g_three_pass)
        {
            g_std_calc_mode = CalcMode::SOLID_GUESS;
            g_three_pass = true;
            engine_timer(timer_work_list);
            if (g_calc_status == CalcStatus::COMPLETED)
            {
                g_std_calc_mode = followup_calc_mode();
                engine_timer(timer_work_list);
                g_three_pass = false;
            }
        }
        else // resuming '2' pass
        {
            g_std_calc_mode = followup_calc_mode();
            engine_timer(timer_work_list);
        }
    }
    else // main case, much nicer!
    {
        g_three_pass = false;
        engine_timer(timer_work_list);
    }

    m_phase = Phase::COMPLETE;
}

} // namespace id::engine
