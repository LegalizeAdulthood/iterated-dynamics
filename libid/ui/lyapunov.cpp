// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/lyapunov.h"

#include "fractals/lyapunov.h"
#include "ui/LyapunovController.h"

using namespace id::fractals;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 makes the outer render context own this calculation and controller.
int lyapunov_fractal()
{
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
