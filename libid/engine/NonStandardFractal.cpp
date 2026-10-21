// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/NonStandardFractal.h"

#include "engine/calc_frac_init.h"
#include "engine/fractals.h"
#include "engine/LogicalScreen.h"
#include "engine/resume.h"
#include "fractals/bif_may.h"
#include "fractals/fractalp.h"
#include "fractals/frasetup.h"

#include <cmath>
#include <stdexcept>

using namespace id::fractals;

namespace id::engine
{

void set_symmetry(SymmetryType sym, bool use_list);

NonStandardFractal::~NonStandardFractal()
{
    if (started())
    {
        finish(true);
    }
}

void NonStandardFractal::prepare()
{
    g_dispatch.init_calc_type(*g_cur_fractal_specific);
    g_symmetry = g_cur_fractal_specific->symmetry;
    g_plot = g_put_color;
    g_begin_pt = {};
    g_start_pt = {};
    g_i_start_pt = {};
    g_stop_pt = {g_logical_screen.x_dots - 1, g_logical_screen.y_dots - 1};
    g_i_stop_pt = g_stop_pt;
    g_calc_status = CalcStatus::IN_PROGRESS;
    g_distance_estimator = 0;
}

NonStandardFractal::Operation NonStandardFractal::select()
{
    if (!started())
    {
        throw std::logic_error("Non-standard image setup has not started");
    }
    if (!m_done)
    {
        throw std::logic_error("Non-standard calculation is still active");
    }
    switch (g_fractal_type)
    {
    case FractalType::BIFURCATION:
    case FractalType::BIF_LAMBDA:
    case FractalType::BIF_PLUS_SIN_PI:
    case FractalType::BIF_EQ_SIN_PI:
    case FractalType::BIF_STEWART:
    case FractalType::BIF_MAY:
        prepare();
        if (g_fractal_type == FractalType::BIF_MAY)
        {
            set_bifurc_may_beta(g_params[2]);
        }
        m_state.emplace<BIFURCATION>();
        if (g_resuming)
        {
            std::get<BIFURCATION>(m_state).resume();
        }
        m_done = false;
        return Operation::BIFURCATION;

    case FractalType::POPCORN:
    case FractalType::POPCORN_JUL:
        start_timer();
        prepare();
        julia_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        m_state.emplace<POPCORN>();
        std::get<POPCORN>(m_state).resume();
        m_done = std::get<POPCORN>(m_state).done();
        update_timer();
        return Operation::POPCORN;

    case FractalType::DIFFUSION:
        start_timer();
        prepare();
        m_state.emplace<DIFFUSION>();
        m_done = false;
        update_timer();
        return Operation::DIFFUSION;

    default:
        return Operation::LEGACY;
    }
}

bool NonStandardFractal::done() const
{
    return m_done;
}

void NonStandardFractal::iterate()
{
    switch (m_state.index())
    {
    case BIFURCATION:
        if (!m_done)
        {
            start_timer();
            m_done = !std::get<BIFURCATION>(m_state).iterate();
            update_timer();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case POPCORN:
        if (!m_done)
        {
            Popcorn &popcorn{std::get<POPCORN>(m_state)};
            popcorn.iterate();
            m_done = popcorn.done();
        }
        return;

    case DIFFUSION:
        if (!m_done)
        {
            m_done = !std::get<DIFFUSION>(m_state).iterate();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    default:
        throw std::logic_error("No non-standard calculation operation is selected");
    }
}

void NonStandardFractal::suspend()
{
    switch (m_state.index())
    {
    case BIFURCATION:
        if (!m_done)
        {
            std::get<BIFURCATION>(m_state).suspend();
            finish(true);
        }
        return;

    case POPCORN:
        if (!m_done)
        {
            std::get<POPCORN>(m_state).suspend();
            finish(true);
        }
        return;

    case DIFFUSION:
        if (!m_done)
        {
            std::get<DIFFUSION>(m_state).suspend();
            finish(true);
        }
        return;

    default:
        throw std::logic_error("No non-standard calculation operation is selected");
    }
}

bool NonStandardFractal::completed() const
{
    return g_calc_status == CalcStatus::COMPLETED;
}

void NonStandardFractal::finish(const bool interrupted)
{
    if (g_calc_status == CalcStatus::IN_PROGRESS)
    {
        g_calc_status = interrupted ? CalcStatus::NON_RESUMABLE : CalcStatus::COMPLETED;
    }
    m_state.emplace<NO_OPERATION>();
    m_done = true;
    ImageCalculation::finish();
}

// TODO: removed later
// Slices 47.4-51 replace remaining renderer calls with selected operations.
void NonStandardFractal::render_legacy()
{
    start_timer();
    prepare();
    if (per_image())
    {
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        const CalcType calc_type{g_dispatch.calc_type()};
        calc_type();
    }
    update_timer();
}

} // namespace id::engine
