// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_fractal.h"

#include "engine/StandardFractal.h"
#include "ui/check_key.h"

using namespace id::engine;

namespace id::ui
{

int standard_fractal()
{
    StandardFractal standard_fractal;
    standard_fractal.resume();

    while (!standard_fractal.done())
    {
        standard_fractal.iterate();
        if (!standard_fractal.done() && check_key())
        {
            standard_fractal.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
