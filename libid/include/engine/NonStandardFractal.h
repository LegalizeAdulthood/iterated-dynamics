// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/Bifurcation.h"
#include "fractals/popcorn.h"

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
        POPCORN
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
    // Slices 47.3-51 replace remaining renderer calls with selected operations.
    void render_legacy();

private:
    enum StateIndex
    {
        NO_OPERATION,
        BIFURCATION,
        POPCORN
    };

    struct NoOperation
    {
    };

    void prepare();

    std::variant<NoOperation, fractals::Bifurcation, fractals::Popcorn> m_state{NoOperation{}};
    bool m_done{true};
};

} // namespace id::engine
