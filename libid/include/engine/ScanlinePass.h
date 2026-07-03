// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"

namespace id::engine
{

class ScanlinePass
{
protected:
    int standard_calc(int pass_num, CalcMode calc_mode);
    int stop_row_for_resume() const;

    int m_current_pass{};
    int m_row{};
    int m_col{};
    int m_resume_row{};
    int m_resume_col{};
    bool m_standard_calc_active{};
};

class OnePass : ScanlinePass
{
public:
    bool iterate();
    int run();
};

class TwoPass : ScanlinePass
{
public:
    bool iterate();
    int run();
};

} // namespace id::engine
