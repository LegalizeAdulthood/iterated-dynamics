// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"

namespace id::engine
{

class ScanlinePass
{
public:
    void suspend();

protected:
    bool standard_calc(int pass_num, CalcMode calc_mode);
    int stop_row_for_resume() const;

    int m_current_pass{};
    int m_row{};
    int m_col{};
    bool m_standard_calc_active{};
};

class OnePass : public ScanlinePass
{
public:
    bool iterate();
};

class TwoPass : public ScanlinePass
{
public:
    bool iterate();
};

} // namespace id::engine
