// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/Bifurcation.h"

#include <variant>

namespace id::engine
{

class NonStandardFractal : public ImageCalculation
{
public:
    NonStandardFractal() = default;
    ~NonStandardFractal();

    bool select();
    bool done() const;
    void iterate();
    void suspend();
    bool completed() const;
    void finish(bool interrupted);

    // TODO: removed later
    // Slices 47.1-51 replace remaining renderer calls with selected operations.
    void render_legacy();

private:
    enum StateIndex
    {
        NO_OPERATION,
        BIFURCATION
    };

    struct NoOperation
    {
    };

    void prepare();

    std::variant<NoOperation, fractals::Bifurcation> m_state{NoOperation{}};
    bool m_done{true};
};

} // namespace id::engine
