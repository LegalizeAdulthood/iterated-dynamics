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
#include "fractals/ifs.h"

#include <cmath>
#include <stdexcept>

using namespace id::fractals;
using namespace id::geometry;

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
    if (is_orbit2d(g_fractal_type))
    {
        start_timer();
        prepare();
        orbit3d_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        Orbit2D &orbit{m_state.emplace<ORBIT_2D>()};
        if (g_resuming)
        {
            orbit.resume();
        }
        m_done = orbit.done();
        update_timer();
        return Operation::ORBIT_2D;
    }
    if (is_orbit3d(g_fractal_type))
    {
        start_timer();
        prepare();
        orbit3d_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        g_display_3d = Display3DMode::MINUS_ONE;
        m_stereo_view.start();
        m_state.emplace<ORBIT_3D>(m_stereo_view.both_eyes());
        m_done = false;
        update_timer();
        return Operation::ORBIT_3D;
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

    case FractalType::IFS:
    case FractalType::IFS_3D:
        start_timer();
        prepare();
        if (g_ifs_definition.empty())
        {
            ifs_load();
        }
        if (g_ifs_dim == IFSDimension::TWO)
        {
            m_state.emplace<IFS_2D>();
            m_done = false;
            update_timer();
            return Operation::IFS_2D;
        }
        g_display_3d = Display3DMode::MINUS_ONE;
        m_stereo_view.start();
        m_state.emplace<IFS_3D>(m_stereo_view.both_eyes());
        m_done = false;
        update_timer();
        return Operation::IFS_3D;

    case FractalType::DYNAMIC:
    case FractalType::MANDEL_CLOUD:
    {
        start_timer();
        prepare();
        dynamic2d_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        Dynamic2D &dynamic{m_state.emplace<DYNAMIC_2D>()};
        if (g_resuming)
        {
            dynamic.resume();
        }
        m_done = dynamic.done();
        update_timer();
        return Operation::DYNAMIC_2D;
    }

    case FractalType::INVERSE_JULIA:
        start_timer();
        prepare();
        orbit3d_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        m_done = m_state.emplace<INVERSE_JULIA>().done();
        update_timer();
        return Operation::INVERSE_JULIA;

    case FractalType::JULIBROT:
        start_timer();
        prepare();
        julibrot_per_image();
        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        set_symmetry(g_symmetry, false);
        m_state.emplace<STANDARD_4D>();
        m_done = false;
        update_timer();
        return Operation::STANDARD_4D;

    default:
        throw std::logic_error("The selected fractal does not use a non-standard calculation operation");
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

bool NonStandardFractal::view_done() const
{
    switch (m_state.index())
    {
    case IFS_3D:
        return std::get<IFS_3D>(m_state).done();

    case ORBIT_3D:
        return std::get<ORBIT_3D>(m_state).done();

    default:
        throw std::logic_error("No stereo calculation operation is selected");
    }
}

void NonStandardFractal::next_view()
{
    if (!view_done() || !m_stereo_view.next_view_required())
    {
        throw std::logic_error("The next stereo view requires a completed first eye");
    }
    if (m_state.index() == ORBIT_3D && bit_set(g_cur_fractal_specific->flags, FractalFlags::INF_CALC))
    {
        orbit3d_per_image();
    }
    m_stereo_view.next_view();
    if (m_state.index() == IFS_3D)
    {
        m_state.emplace<IFS_3D>(m_stereo_view.both_eyes());
    }
    else
    {
        m_state.emplace<ORBIT_3D>(m_stereo_view.both_eyes());
    }
    m_done = false;
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

bool NonStandardFractal::inverse_julia_queue_required() const
{
    return m_state.index() == INVERSE_JULIA && std::get<INVERSE_JULIA>(m_state).queue_required();
}

void NonStandardFractal::complete_inverse_julia_queue(const bool available)
{
    if (!inverse_julia_queue_required())
    {
        throw std::logic_error("Inverse Julia queue storage was not requested");
    }
    std::get<INVERSE_JULIA>(m_state).complete_queue(available);
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

    case IFS_2D:
        if (!m_done)
        {
            IFS2D &ifs{std::get<IFS_2D>(m_state)};
            ifs.iterate();
            m_done = ifs.done();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case IFS_3D:
        if (!m_done)
        {
            IFS3D &ifs{std::get<IFS_3D>(m_state)};
            if (ifs.done())
            {
                throw std::logic_error("The completed IFS view must advance before more iteration");
            }
            ifs.iterate();
            m_done = ifs.done() && !m_stereo_view.next_view_required();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case ORBIT_3D:
        if (!m_done)
        {
            Orbit3D &orbit{std::get<ORBIT_3D>(m_state)};
            if (orbit.done())
            {
                throw std::logic_error("The completed orbit view must advance before more iteration");
            }
            orbit.iterate();
            m_done = orbit.done() && !m_stereo_view.next_view_required();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case ORBIT_2D:
        if (!m_done)
        {
            Orbit2D &orbit{std::get<ORBIT_2D>(m_state)};
            orbit.iterate();
            m_done = orbit.done();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case DYNAMIC_2D:
        if (!m_done)
        {
            Dynamic2D &dynamic{std::get<DYNAMIC_2D>(m_state)};
            dynamic.iterate();
            m_done = dynamic.done();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case INVERSE_JULIA:
        if (!m_done)
        {
            InverseJuliaFractal &inverse{std::get<INVERSE_JULIA>(m_state)};
            inverse.iterate();
            m_done = inverse.done();
            if (m_done)
            {
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        return;

    case STANDARD_4D:
        if (!m_done)
        {
            m_done = !std::get<STANDARD_4D>(m_state).iterate();
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
    case IFS_2D:
    case IFS_3D:
    case ORBIT_3D:
    case INVERSE_JULIA:
    case STANDARD_4D:
        if (!m_done)
        {
            finish(true);
        }
        return;

    case ORBIT_2D:
        if (!m_done)
        {
            std::get<ORBIT_2D>(m_state).suspend();
            finish(true);
        }
        return;

    case DYNAMIC_2D:
        if (!m_done)
        {
            std::get<DYNAMIC_2D>(m_state).suspend();
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
    m_stereo_view.finish();
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

} // namespace id::engine
