// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StandardPass.h"

#include <cassert>
#include <type_traits>

namespace id::engine
{

namespace
{

template <typename State>
CalcMode state_calc_mode(const State &)
{
    return State::CALC_MODE;
}

} // namespace

CalcMode StandardPass::calc_mode() const
{
    return std::visit([](const auto &state) { return state_calc_mode(state); }, m_state);
}

void StandardPass::reset()
{
    m_state.emplace<NoPass>();
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
        m_state.emplace<TesseralPass>();
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
    std::visit(
        [](auto &state)
        {
            using State = std::decay_t<decltype(state)>;
            if constexpr (std::is_same_v<State, TesseralPass>)
            {
                if (!state.tesseral.done())
                {
                    state.tesseral.suspend();
                }
            }
        },
        m_state);
}

Tesseral &StandardPass::tesseral()
{
    assert(std::holds_alternative<TesseralPass>(m_state));
    return std::get<TesseralPass>(m_state).tesseral;
}

} // namespace id::engine
