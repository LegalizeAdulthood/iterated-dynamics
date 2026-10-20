// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/StandardPass.h"

#include "engine/UserData.h"

#include <fmt/format.h>

#include <stdexcept>

namespace id::engine
{

CalcMode StandardPass::calc_mode() const
{
    switch (m_state.index())
    {
    case ONE_PASS:
        return CalcMode::ONE_PASS;

    case TWO_PASS:
        return CalcMode::TWO_PASS;

    case SYNCHRONOUS_ORBIT:
        return CalcMode::SYNCHRONOUS_ORBIT;

    case BOUNDARY_TRACE:
        return CalcMode::BOUNDARY_TRACE;

    case SOLID_GUESS:
        return CalcMode::SOLID_GUESS;

    case DIFFUSION:
        return CalcMode::DIFFUSION;

    case TESSERAL:
        return CalcMode::TESSERAL;

    case ORBIT:
        return CalcMode::ORBIT;

    default:
        return CalcMode::NONE;
    }
}

void StandardPass::reset()
{
    m_state.emplace<NoPass>();
}

StandardPassStatus StandardPass::status() const
{
    StandardPassStatus status;

    switch (m_state.index())
    {
    case ONE_PASS:
    case TWO_PASS:
        status.title = fmt::format("{:d} Pass Mode", g_total_passes);
        break;

    case SYNCHRONOUS_ORBIT:
        status.title = "Synchronous Orbits";
        break;

    case BOUNDARY_TRACE:
        status.title = "Boundary Tracing";
        status.detail = fmt::format("Working on block (y, x) [{:d}, {:d}]...[{:d}, {:d}], at [{:d}, {:d}]",
            g_start_pt.y, g_start_pt.x, g_stop_pt.y, g_stop_pt.x, g_current_row, g_current_column);
        break;

    case SOLID_GUESS:
        status.title = "Solid Guessing";
        break;

    case DIFFUSION:
        status.title = "Diffusion";
        break;

    case TESSERAL:
        status.title = "Tesseral";
        status.detail = fmt::format("Working on block (y, x) [{:d}, {:d}]...[{:d}, {:d}], at [{:d}, {:d}]",
            g_start_pt.y, g_start_pt.x, g_stop_pt.y, g_stop_pt.x, g_current_row, g_current_column);
        break;

    case ORBIT:
        status.title = "Orbits";
        break;

    default:
        return status;
    }

    if (g_user.std_calc_mode == CalcMode::THREE_PASS)
    {
        status.title += " (threepass)";
    }

    if (!status.detail.empty() || m_state.index() == DIFFUSION)
    {
        if (m_state.index() == DIFFUSION)
        {
            const Diffusion &diffusion{std::get<Diffusion>(m_state)};
            if (diffusion.limit() != 0)
            {
                status.progress_percent =
                    100.0F * static_cast<float>(diffusion.counter()) / static_cast<float>(diffusion.limit());
                status.detail = fmt::format("{:2.2f}% done, counter at {:d} of {:d} ({:d} bits)",
                    status.progress_percent, diffusion.counter(), diffusion.limit(), diffusion.bits());
            }
        }
        return status;
    }

    status.detail = fmt::format(
        "Working on block (y, x) [{:d}, {:d}]...[{:d}, {:d}], ", g_start_pt.y, g_start_pt.x, g_stop_pt.y, g_stop_pt.x);
    if (g_total_passes > 1)
    {
        status.detail += fmt::format("pass {:d} of {:d}, ", g_current_pass, g_total_passes);
    }
    status.detail += fmt::format("at row {:d} col {:d}", g_current_row, g_col);
    return status;
}

bool StandardPass::select_pixel()
{
    switch (m_state.index())
    {
    case NO_PASS:
        return false;

    case ONE_PASS:
        return std::get<OnePass>(m_state).select_pixel();

    case TWO_PASS:
        return std::get<TwoPass>(m_state).select_pixel();

    case BOUNDARY_TRACE:
        return std::get<BoundaryTrace>(m_state).select_pixel();

    case SOLID_GUESS:
        return std::get<SolidGuess>(m_state).select_pixel();

    case DIFFUSION:
        return std::get<Diffusion>(m_state).select_pixel();

    case TESSERAL:
        return std::get<Tesseral>(m_state).select_pixel();

    default:
        throw std::logic_error{"The active standard pass does not select pixels"};
    }
}

math::Point2i StandardPass::selected_pixel() const
{
    switch (m_state.index())
    {
    case ONE_PASS:
        return std::get<OnePass>(m_state).selected_pixel();

    case TWO_PASS:
        return std::get<TwoPass>(m_state).selected_pixel();

    case BOUNDARY_TRACE:
        return std::get<BoundaryTrace>(m_state).selected_pixel();

    case SOLID_GUESS:
        return std::get<SolidGuess>(m_state).selected_pixel();

    case DIFFUSION:
        return std::get<Diffusion>(m_state).selected_pixel();

    case TESSERAL:
        return std::get<Tesseral>(m_state).selected_pixel();

    default:
        throw std::logic_error{"The active standard pass has no selected pixel"};
    }
}

void StandardPass::complete_pixel(const int color)
{
    switch (m_state.index())
    {
    case ONE_PASS:
        std::get<OnePass>(m_state).complete_pixel(color);
        break;

    case TWO_PASS:
        std::get<TwoPass>(m_state).complete_pixel(color);
        break;

    case BOUNDARY_TRACE:
        std::get<BoundaryTrace>(m_state).complete_pixel(color);
        break;

    case SOLID_GUESS:
        std::get<SolidGuess>(m_state).complete_pixel(color);
        break;

    case DIFFUSION:
        std::get<Diffusion>(m_state).complete_pixel(color);
        break;

    case TESSERAL:
        std::get<Tesseral>(m_state).complete_pixel(color);
        break;

    default:
        throw std::logic_error{"The active standard pass has no pixel to complete"};
    }
}

void StandardPass::begin_chunk()
{
    switch (m_state.index())
    {
    case ONE_PASS:
        std::get<OnePass>(m_state).begin_chunk();
        break;

    case TWO_PASS:
        std::get<TwoPass>(m_state).begin_chunk();
        break;

    case BOUNDARY_TRACE:
        std::get<BoundaryTrace>(m_state).begin_chunk();
        break;

    case SOLID_GUESS:
        std::get<SolidGuess>(m_state).begin_chunk();
        break;

    case DIFFUSION:
        std::get<Diffusion>(m_state).begin_chunk();
        break;

    case TESSERAL:
        std::get<Tesseral>(m_state).begin_chunk();
        break;

    default:
        throw std::logic_error{"The active standard pass does not select pixels"};
    }
}

bool StandardPass::done() const
{
    switch (m_state.index())
    {
    case NO_PASS:
        return true;

    case ONE_PASS:
        return std::get<OnePass>(m_state).done();

    case TWO_PASS:
        return std::get<TwoPass>(m_state).done();

    case BOUNDARY_TRACE:
        return std::get<BoundaryTrace>(m_state).done();

    case SOLID_GUESS:
        return std::get<SolidGuess>(m_state).done();

    case DIFFUSION:
        return std::get<Diffusion>(m_state).done();

    case TESSERAL:
        return std::get<Tesseral>(m_state).done();

    default:
        throw std::logic_error{"The active standard pass is an orbit algorithm"};
    }
}

bool StandardPass::iterate()
{
    switch (m_state.index())
    {
    case NO_PASS:
        return true;

    case SYNCHRONOUS_ORBIT:
        return std::get<SynchronousOrbit>(m_state).iterate();

    case ORBIT:
        return std::get<StickyOrbits>(m_state).iterate();

    default:
        throw std::logic_error{"Pixel calculation belongs to StandardFractal"};
    }
}

void StandardPass::select(const CalcMode calc_mode)
{
    if (calc_mode == this->calc_mode())
    {
        return;
    }

    switch (calc_mode)
    {
    case CalcMode::ONE_PASS:
        m_state.emplace<OnePass>();
        break;

    case CalcMode::TWO_PASS:
        m_state.emplace<TwoPass>();
        break;

    case CalcMode::SYNCHRONOUS_ORBIT:
        m_state.emplace<SynchronousOrbit>();
        break;

    case CalcMode::BOUNDARY_TRACE:
        m_state.emplace<BoundaryTrace>();
        break;

    case CalcMode::SOLID_GUESS:
        m_state.emplace<SolidGuess>();
        break;

    case CalcMode::DIFFUSION:
        m_state.emplace<Diffusion>();
        break;

    case CalcMode::TESSERAL:
        m_state.emplace<Tesseral>();
        break;

    case CalcMode::ORBIT:
        m_state.emplace<StickyOrbits>();
        break;

    default:
        reset();
        break;
    }
}

void StandardPass::suspend()
{
    if (m_state.index() == ONE_PASS)
    {
        std::get<OnePass>(m_state).suspend();
        return;
    }
    if (m_state.index() == TWO_PASS)
    {
        std::get<TwoPass>(m_state).suspend();
        return;
    }
    if (m_state.index() == SYNCHRONOUS_ORBIT)
    {
        std::get<SynchronousOrbit>(m_state).suspend();
        return;
    }
    if (m_state.index() == BOUNDARY_TRACE)
    {
        std::get<BoundaryTrace>(m_state).suspend();
        return;
    }
    if (m_state.index() == DIFFUSION)
    {
        std::get<Diffusion>(m_state).suspend();
        return;
    }
    if (m_state.index() == SOLID_GUESS)
    {
        std::get<SolidGuess>(m_state).suspend();
        return;
    }
    if (m_state.index() == ORBIT)
    {
        std::get<StickyOrbits>(m_state).suspend();
        return;
    }

    if (m_state.index() != TESSERAL)
    {
        return;
    }
    Tesseral &tesseral{std::get<Tesseral>(m_state)};
    if (!tesseral.done())
    {
        tesseral.suspend();
    }
}

void StandardPass::save_resume() const
{
    if (m_state.index() == ORBIT)
    {
        std::get<StickyOrbits>(m_state).save_resume();
    }
}

void StandardPass::restore_resume(const CalcMode calc_mode)
{
    if (calc_mode == CalcMode::ORBIT)
    {
        select(calc_mode);
        std::get<StickyOrbits>(m_state).restore_resume();
    }
}

} // namespace id::engine
