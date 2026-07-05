// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/non_standard_fractal.h"

#include "engine/calcfrac.h"
#include "ui/check_key.h"

using namespace id::engine;

namespace id::ui
{

int non_standard_fractal()
{
    NonStandardFractal non_standard_fractal;
    non_standard_fractal.iterate();

    non_standard_fractal.finish(check_key());

    return non_standard_fractal.completed() ? 0 : -1;
}

} // namespace id::ui
