// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/tesseral.h"

#include <variant>

namespace id::engine
{

class StandardPass
{
public:
    StandardPass() = default;

    StandardPass(const StandardPass &) = delete;
    StandardPass(StandardPass &&) = delete;
    StandardPass &operator=(const StandardPass &) = delete;
    StandardPass &operator=(StandardPass &&) = delete;

    CalcMode calc_mode() const;
    void reset();
    void select(CalcMode calc_mode);
    void suspend();

    Tesseral &tesseral();

private:
    struct NoPass
    {
        static constexpr CalcMode CALC_MODE{CalcMode::NONE};
    };

    struct OnePass
    {
        static constexpr CalcMode CALC_MODE{CalcMode::ONE_PASS};
    };

    struct TwoPass
    {
        static constexpr CalcMode CALC_MODE{CalcMode::TWO_PASS};
    };

    struct SynchronousOrbit
    {
        static constexpr CalcMode CALC_MODE{CalcMode::SYNCHRONOUS_ORBIT};
    };

    struct BoundaryTrace
    {
        static constexpr CalcMode CALC_MODE{CalcMode::BOUNDARY_TRACE};
    };

    struct SolidGuess
    {
        static constexpr CalcMode CALC_MODE{CalcMode::SOLID_GUESS};
    };

    struct Diffusion
    {
        static constexpr CalcMode CALC_MODE{CalcMode::DIFFUSION};
    };

    struct TesseralPass
    {
        static constexpr CalcMode CALC_MODE{CalcMode::TESSERAL};

        Tesseral tesseral{};
    };

    struct Orbit
    {
        static constexpr CalcMode CALC_MODE{CalcMode::ORBIT};
    };

    using State = std::variant<NoPass, OnePass, TwoPass, SynchronousOrbit, BoundaryTrace, SolidGuess, Diffusion,
        TesseralPass, Orbit>;

    State m_state{NoPass{}};
};

} // namespace id::engine
