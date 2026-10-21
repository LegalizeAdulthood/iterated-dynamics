// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/Ant.h"
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
        TEST_POINT,
        ANT
    };

    NonStandardFractal() = default;
    ~NonStandardFractal();

    Operation select();
    void select_ant_on_image();
    bool done() const;
    void iterate();
    bool consume_batch_complete();
    void suspend();
    bool completed() const;
    void finish(bool interrupted);

    // TODO: removed later
    // Slices 47.6-51 replace remaining renderer calls with selected operations.
    void render_legacy();

private:
    enum StateIndex
    {
        NO_OPERATION,
        BIFURCATION,
        POPCORN,
        DIFFUSION,
        TEST_POINT,
        ANT
    };

    struct NoOperation
    {
    };

    void prepare();

    std::variant<NoOperation, fractals::Bifurcation, fractals::Popcorn, fractals::Diffusion, fractals::TestPoint,
        fractals::Ant>
        m_state{NoOperation{}};
    bool m_done{true};
};

} // namespace id::engine
