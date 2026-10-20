// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"

namespace id::engine
{

class ScanlinePass
{
public:
    void begin_chunk();
    // A selection remains current until complete_pixel() accepts its color.
    bool select_pixel();
    math::Point2i selected_pixel() const;
    void complete_pixel(int color);
    bool done() const;
    void suspend();

protected:
    explicit ScanlinePass(CalcMode calc_mode);

private:
    void start_pass();
    bool select_current_pass_pixel();
    int stop_row_for_resume() const;

    CalcMode m_calc_mode;
    int m_current_pass{};
    int m_row{};
    int m_col{};
    bool m_standard_calc_active{};
    bool m_pixel_selected{};
    bool m_done{};
};

class OnePass : public ScanlinePass
{
public:
    OnePass();
};

class TwoPass : public ScanlinePass
{
public:
    TwoPass();
};

} // namespace id::engine
