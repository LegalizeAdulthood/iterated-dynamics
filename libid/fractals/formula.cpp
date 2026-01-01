// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/formula.h"

#include "engine/calcfrac.h"
#include "engine/Inversion.h"
#include "engine/pixel_grid.h"
#include "fractals/interpreter.h"
#include "fractals/newton.h"
#include "fractals/parser.h"
#include "math/fixed_pt.h"

#include <fmt/format.h>

using namespace id::engine;
using namespace id::math;
using namespace id::misc;

namespace id::fractals
{

std::filesystem::path g_formula_filename; // file to find formulas in
std::string g_formula_name;               // Name of the Formula (if not empty)
bool g_frm_is_mandelbrot{true};           // true if the formula is a mandelbrot type
bool g_frm_uses_ismand{};                 // true if the formula uses ismand variable
bool g_frm_uses_p1{};                     // true if the formula uses p1 variable
bool g_frm_uses_p2{};                     // true if the formula uses p2 variable
bool g_frm_uses_p3{};                     // true if the formula uses p3 variable
bool g_frm_uses_p4{};                     // true if the formula uses p4 variable
bool g_frm_uses_p5{};                     // true if the formula uses p5 variable

int bad_formula()
{
    //  this is called when a formula is bad, instead of calling
    //     the normal functions which will produce undefined results
    return 1;
}

bool formula_per_image()
{
    const bool result = !parse_formula(g_formula_filename, g_formula_name, false);
    if (!result)
    {
        debug_trace_close();
    }
    return result; // run_formula() returns true for failure
}

int formula_per_pixel()
{
    if (g_formula_name.empty())
    {
        return 1;
    }

    g_runtime.per_pixel_begin();

    g_overflow = false;
    g_runtime.jump_index = 0;
    g_runtime.op_index = 0;
    g_runtime.set_random = false;
    if (g_formula.uses_rand && !g_runtime.randomized)
    {
        random_seed();
    }
    g_runtime.store_index = 0;
    g_runtime.load_index = 0;
    g_arg1 = g_runtime.stack.data();
    g_arg2 = g_runtime.stack.data();
    g_arg2--;

    g_formula.vars[10].a.d.x = static_cast<double>(g_col);
    g_formula.vars[10].a.d.y = static_cast<double>(g_row);

    if (g_row + g_col & 1)
    {
        g_formula.vars[9].a.d.x = 1.0;
    }
    else
    {
        g_formula.vars[9].a.d.x = 0.0;
    }
    g_formula.vars[9].a.d.y = 0.0;

    if (g_inversion.invert != 0)
    {
        invertz2(&g_old_z);
        g_formula.vars[0].a.d.x = g_old_z.x;
        g_formula.vars[0].a.d.y = g_old_z.y;
    }
    else
    {
        g_formula.vars[0].a.d.x = dx_pixel();
        g_formula.vars[0].a.d.y = dy_pixel();
    }

    if (g_formula.last_init_op)
    {
        g_formula.last_init_op = g_formula.op_count;
    }
    g_runtime.per_pixel_init();
    while (g_runtime.op_index < g_formula.last_init_op)
    {
        g_formula.fns[g_runtime.op_index]();
        g_runtime.op_index++;
    }
    g_runtime.init_load_index = g_runtime.load_index;
    g_runtime.init_store_index = g_runtime.store_index;
    g_runtime.init_op_index = g_runtime.op_index;
    // Set old variable for orbits
    g_old_z = g_formula.vars[3].a.d;

    return g_overflow ? 0 : 1;
}

int formula_orbit()
{
    if (g_formula_name.empty() || g_overflow)
    {
        return 1;
    }

    g_runtime.orbit_begin();

    g_runtime.load_index = g_runtime.init_load_index;
    g_runtime.store_index = g_runtime.init_store_index;
    g_runtime.op_index = g_runtime.init_op_index;
    g_runtime.jump_index = g_runtime.init_jump_index;
    // Set the random number
    if (g_runtime.set_random || g_runtime.randomized)
    {
        d_random();
    }

    g_arg1 = g_runtime.stack.data();
    g_arg2 = g_runtime.stack.data();
    --g_arg2;
    while (g_runtime.op_index < static_cast<int>(g_formula.op_count))
    {
        g_formula.fns[g_runtime.op_index]();
        g_runtime.op_index++;
    }

    g_new_z = g_formula.vars[3].a.d;
    g_old_z = g_new_z;

    g_runtime.orbit_end();

    return g_arg1->d.x == 0.0;
}

} // namespace id::fractals
