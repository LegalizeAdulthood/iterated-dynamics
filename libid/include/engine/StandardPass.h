// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/boundary_trace.h"
#include "engine/calcfrac.h"
#include "engine/Diffusion.h"
#include "engine/one_or_two_pass.h"
#include "engine/solid_guess.h"
#include "engine/tesseral.h"

#include <string>
#include <variant>

namespace id::engine
{

struct StandardPassStatus
{
    std::string title;
    std::string detail;
    float progress_percent{};
};

class StandardPass
{
public:
    StandardPass() = default;

    StandardPass(const StandardPass &) = delete;
    StandardPass(StandardPass &&) = delete;
    StandardPass &operator=(const StandardPass &) = delete;
    StandardPass &operator=(StandardPass &&) = delete;

    CalcMode calc_mode() const;
    bool iterate();
    void reset();
    void select(CalcMode calc_mode);
    StandardPassStatus status() const;
    void suspend();

private:
    enum StateIndex
    {
        NO_PASS,
        ONE_PASS,
        TWO_PASS,
        SYNCHRONOUS_ORBIT,
        BOUNDARY_TRACE,
        SOLID_GUESS,
        DIFFUSION,
        TESSERAL,
        ORBIT,
    };

    struct NoPass
    {
    };

    struct SynchronousOrbit
    {
    };

    struct Orbit
    {
    };

    using State =
        std::variant<NoPass, OnePass, TwoPass, SynchronousOrbit, BoundaryTrace, SolidGuess, Diffusion, Tesseral, Orbit>;

    State m_state{NoPass{}};
};

} // namespace id::engine
