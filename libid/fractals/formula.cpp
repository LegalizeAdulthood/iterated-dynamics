// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/formula.h"

#include "fractals/parser.h"

namespace id::fractals
{

int bad_formula()
{
    //  this is called when a formula is bad, instead of calling
    //     the normal functions which will produce undefined results
    return 1;
}

bool formula_per_image()
{
    const bool result = !parse_formula(g_formula_name, false);
    if (!result)
    {
        debug_trace_close();
    }
    return result; // run_formula() returns true for failure
}

} // namespace id::fractals
