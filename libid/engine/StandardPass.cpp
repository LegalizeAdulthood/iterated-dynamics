// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StandardPass.h"

#include "engine/diffusion_scan.h"
#include "engine/soi.h"
#include "engine/solid_guess.h"
#include "engine/sticky_orbits.h"

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

bool StandardPass::iterate()
{
    switch (m_state.index())
    {
    case SYNCHRONOUS_ORBIT:
        soi();
        return true;

    case BOUNDARY_TRACE:
        return std::get<BoundaryTrace>(m_state).iterate();

    case SOLID_GUESS:
        if (g_calc_status != CalcStatus::COMPLETED)
        {
            solid_guess();
        }
        return true;

    case DIFFUSION:
        diffusion_scan();
        return true;

    case TESSERAL:
    {
        Tesseral &tesseral{std::get<Tesseral>(m_state)};
        return tesseral.iterate() && tesseral.done();
    }

    case ORBIT:
        sticky_orbits();
        return true;

    case ONE_PASS:
        return std::get<OnePass>(m_state).iterate();

    case TWO_PASS:
        return std::get<TwoPass>(m_state).iterate();

    default:
        return true;
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
        m_state.emplace<Orbit>();
        break;

    default:
        reset();
        break;
    }
}

void StandardPass::suspend()
{
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

} // namespace id::engine
