// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/NonStandardFractal.h"

#include "engine/calc_frac_init.h"
#include "engine/fractals.h"
#include "engine/LogicalScreen.h"
#include "engine/resume.h"
#include "engine/VideoInfo.h"
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

    case FractalType::TEST:
    {
        start_timer();
        prepare();
        TestPoint &test{m_state.emplace<TEST_POINT>()};
        if (g_resuming)
        {
            test.resume();
        }
        const bool standalone{test.start()};
        m_done = standalone || test.done();
        if (m_done)
        {
            if (!standalone)
            {
                test.finish();
            }
            g_calc_status = CalcStatus::COMPLETED;
        }
        update_timer();
        return Operation::TEST_POINT;
    }

    case FractalType::ANT:
        start_timer();
        prepare();
        m_done = m_state.emplace<ANT>().done();
        if (m_done)
        {
            g_calc_status = CalcStatus::COMPLETED;
        }
        update_timer();
        return Operation::ANT;

    case FractalType::CELLULAR:
        start_timer();
        prepare();
        if (!g_resuming)
        {
            g_cellular_next_screen = false;
        }
        m_state.emplace<CELLULAR>();
        m_done = false;
        update_timer();
        return Operation::CELLULAR;

    case FractalType::PLASMA:
    {
        start_timer();
        prepare();
        if (g_colors < 4)
        {
            throw std::runtime_error("Plasma Clouds requires 4 or more color video");
        }
        Plasma &plasma{m_state.emplace<PLASMA>()};
        m_done = false;
        if (!plasma.potential_storage_required())
        {
            plasma.init(false);
        }
        update_timer();
        return Operation::PLASMA;
    }

    case FractalType::L_SYSTEM:
    {
        start_timer();
        prepare();
        if (!lsystem_loaded())
        {
            lsystem_load();
        }
        LSystem &system{m_state.emplace<L_SYSTEM>()};
        system.start();
        m_done = system.done() || system.interrupted();
        if (m_done)
        {
            g_calc_status = CalcStatus::COMPLETED;
        }
        update_timer();
        return Operation::L_SYSTEM;
    }

    case FractalType::FROTHY_BASIN:
    {
        start_timer();
        prepare();
        FrothyBasin &froth{m_state.emplace<FROTHY_BASIN>()};
        froth.per_image();
        froth.resume();
        m_done = froth.done();
        update_timer();
        return Operation::FROTHY_BASIN;
    }

    default:
        return Operation::LEGACY;
    }
}

void NonStandardFractal::select_ant_on_image()
{
    if (started() || m_state.index() != NO_OPERATION || g_fractal_type != FractalType::ANT)
    {
        throw std::logic_error("Ant drawing requires an existing image and no active operation");
    }
    m_done = m_state.emplace<ANT>().done();
}

bool NonStandardFractal::done() const
{
    return m_done;
}

bool NonStandardFractal::preparing() const
{
    if (m_state.index() != CELLULAR)
    {
        throw std::logic_error("No cellular calculation operation is selected");
    }
    return std::get<CELLULAR>(m_state).preparing();
}

bool NonStandardFractal::sizing() const
{
    if (m_state.index() != L_SYSTEM)
    {
        throw std::logic_error("No L-system calculation operation is selected");
    }
    return std::get<L_SYSTEM>(m_state).sizing();
}

std::string NonStandardFractal::prepare_palette()
{
    if (m_state.index() != FROTHY_BASIN)
    {
        throw std::logic_error("No FrothyBasin calculation operation is selected");
    }
    return std::get<FROTHY_BASIN>(m_state).set_froth_palette();
}

bool NonStandardFractal::consume_orbit_overlay_displayed()
{
    if (m_state.index() != FROTHY_BASIN)
    {
        throw std::logic_error("No FrothyBasin calculation operation is selected");
    }
    return std::get<FROTHY_BASIN>(m_state).consume_orbit_overlay_displayed();
}

void NonStandardFractal::scrub_overlay()
{
    if (m_state.index() != FROTHY_BASIN)
    {
        throw std::logic_error("No FrothyBasin calculation operation is selected");
    }
    std::get<FROTHY_BASIN>(m_state).scrub_overlay();
}

bool NonStandardFractal::potential_storage_required() const
{
    return m_state.index() == PLASMA && std::get<PLASMA>(m_state).potential_storage_required();
}

void NonStandardFractal::complete_potential_storage(const bool available)
{
    if (!potential_storage_required())
    {
        throw std::logic_error("Potential storage was not requested");
    }
    std::get<PLASMA>(m_state).init(available);
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

    case TEST_POINT:
        if (!m_done)
        {
            TestPoint &test{std::get<TEST_POINT>(m_state)};
            test.iterate();
            m_done = test.done();
            if (m_done)
            {
                test.finish();
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case ANT:
        if (!m_done)
        {
            Ant &ant{std::get<ANT>(m_state)};
            ant.iterate();
            m_done = ant.done();
            if (m_done && started())
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case CELLULAR:
        if (!m_done)
        {
            m_done = !std::get<CELLULAR>(m_state).iterate();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case PLASMA:
        if (!m_done)
        {
            Plasma &plasma{std::get<PLASMA>(m_state)};
            plasma.iterate();
            m_done = plasma.done();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case L_SYSTEM:
        if (!m_done)
        {
            LSystem &system{std::get<L_SYSTEM>(m_state)};
            system.iterate();
            m_done = system.done() || system.interrupted();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case FROTHY_BASIN:
        if (!m_done)
        {
            FrothyBasin &froth{std::get<FROTHY_BASIN>(m_state)};
            froth.iterate();
            m_done = froth.done();
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

bool NonStandardFractal::consume_batch_complete()
{
    if (m_state.index() != ANT)
    {
        throw std::logic_error("No Ant calculation operation is selected");
    }
    return std::get<ANT>(m_state).consume_batch_complete();
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

    case TEST_POINT:
        if (!m_done)
        {
            std::get<TEST_POINT>(m_state).suspend();
            m_done = true;
            finish(true);
        }
        return;

    case ANT:
    case PLASMA:
        if (!m_done)
        {
            finish(true);
        }
        return;

    case CELLULAR:
        if (!m_done)
        {
            std::get<CELLULAR>(m_state).suspend();
            finish(true);
        }
        return;

    case L_SYSTEM:
        if (!m_done)
        {
            std::get<L_SYSTEM>(m_state).suspend();
            finish(true);
        }
        return;

    case FROTHY_BASIN:
        if (!m_done)
        {
            std::get<FROTHY_BASIN>(m_state).suspend();
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
    if (m_state.index() == TEST_POINT && !m_done)
    {
        std::get<TEST_POINT>(m_state).finish();
    }
    if (started() && g_calc_status == CalcStatus::IN_PROGRESS)
    {
        g_calc_status = interrupted ? CalcStatus::NON_RESUMABLE : CalcStatus::COMPLETED;
    }
    m_state.emplace<NO_OPERATION>();
    m_done = true;
    if (started())
    {
        ImageCalculation::finish();
    }
}

// TODO: removed later
// Slices 48-51 replace remaining renderer calls with selected operations.
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
