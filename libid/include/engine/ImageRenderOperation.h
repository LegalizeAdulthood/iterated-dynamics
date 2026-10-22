// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/NonStandardFractal.h"
#include "engine/StandardFractal.h"
#include "fractals/lyapunov.h"

#include <variant>

namespace id::engine
{

class ImageRenderOperation
{
public:
    enum class Renderer
    {
        STANDARD,
        LYAPUNOV,
        NON_STANDARD
    };

    Renderer select();
    StandardFractal &standard();
    fractals::Lyapunov &lyapunov();
    NonStandardFractal &non_standard();
    void finish(bool interrupted);
    void abort();

private:
    enum StateIndex
    {
        NO_OPERATION,
        STANDARD,
        LYAPUNOV,
        NON_STANDARD
    };

    std::variant<std::monostate, StandardFractal, fractals::Lyapunov, NonStandardFractal> m_state;
};

} // namespace id::engine
