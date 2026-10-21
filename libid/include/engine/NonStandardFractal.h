// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/Bifurcation.h"
#include "fractals/Diffusion.h"
#include "fractals/popcorn.h"
#include "fractals/TestPoint.h"

#include <variant>

namespace id::engine
{

class NonStandardFractal : public ImageCalculation
{
public:
    enum class Operation
    {
        // TODO: removed later
        LEGACY,
        BIFURCATION,
        POPCORN,
        DIFFUSION,
        TEST_POINT
    };

    NonStandardFractal() = default;
    ~NonStandardFractal();

    Operation select();
    bool done() const;
    void iterate();
    void suspend();
    bool completed() const;
    void finish(bool interrupted);

    // TODO: removed later
    // Slices 47.5-51 replace remaining renderer calls with selected operations.
    void render_legacy();

private:
    enum StateIndex
    {
        NO_OPERATION,
        BIFURCATION,
        POPCORN,
        DIFFUSION,
        TEST_POINT
    };

    struct NoOperation
    {
    };

    void prepare();

    std::variant<NoOperation, fractals::Bifurcation, fractals::Popcorn, fractals::Diffusion, fractals::TestPoint>
        m_state{NoOperation{}};
    bool m_done{true};
};

} // namespace id::engine
