// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/non_standard_fractal.h"

#include "engine/calcfrac.h"
#include "ui/check_key.h"

using namespace id::engine;

namespace id::ui
{

int non_standard_fractal()
{
    NonStandardFractal non_standard_fractal;
    non_standard_fractal.iterate();

    if (check_key())
    {
        if (g_calc_status == CalcStatus::IN_PROGRESS)  // calctype didn't set this itself,
        {
            g_calc_status = CalcStatus::NON_RESUMABLE; // so mark it interrupted, non-resumable
        }
    }
    else if (g_calc_status == CalcStatus::IN_PROGRESS)
    {
        g_calc_status = CalcStatus::COMPLETED; // no key, so assume it completed
    }

    return g_calc_status == CalcStatus::COMPLETED ? 0 : -1;
}

} // namespace id::ui
