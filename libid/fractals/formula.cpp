// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/formula.h"

#include "engine/calcfrac.h"
#include "engine/Inversion.h"
#include "engine/pixel_grid.h"
#include "engine/random_seed.h"
#include "fractals/interpreter.h"
#include "fractals/newton.h"
#include "fractals/parser.h"
#include "io/library.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/debug_flags.h"

#include <fmt/format.h>

using namespace id::engine;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::fractals
{

DebugState s_debug;

// Debug trace utility functions
void debug_trace_init()
{
    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        s_debug.trace_enabled = true;
        const std::filesystem::path path{get_save_path(WriteFile::ROOT, "formula_trace.txt")};
        s_debug.trace_file = std::fopen(path.string().c_str(), "w");
        if (s_debug.trace_file)
        {
            fmt::print(s_debug.trace_file, "Formula Execution Trace\n");
            fmt::print(s_debug.trace_file, "========================\n\n");
        }
    }
}

void debug_trace_close()
{
    if (s_debug.trace_file)
    {
        std::fclose(s_debug.trace_file);
        s_debug.trace_file = nullptr;
    }
    s_debug.trace_enabled = false;
}

void debug_trace_operation(const char* op_name, const Arg* arg1, const Arg* arg2)
{
    if (!s_debug.trace_enabled || !s_debug.trace_file)
    {
        return;
    }

    fmt::print(s_debug.trace_file, "{:04d}: {}{}\n", s_debug.operation_count++,
        std::string(s_debug.indent_level * 2, ' '), op_name);

    if (arg1)
    {
        fmt::print(s_debug.trace_file, "      arg1: ({:.6f}, {:.6f})\n", arg1->d.x, arg1->d.y);
    }
    if (arg2)
    {
        fmt::print(s_debug.trace_file, "      arg2: ({:.6f}, {:.6f})\n", arg2->d.x, arg2->d.y);
    }
}

void debug_trace_stack_state()
{
    if (!s_debug.trace_enabled || !s_debug.trace_file)
    {
        return;
    }

    fmt::print(s_debug.trace_file, "      stack top: ({:.6f}, {:.6f})\n", g_arg1->d.x, g_arg1->d.y);
    std::fflush(s_debug.trace_file);
}

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

int formula_per_pixel()
{
    if (g_formula_name.empty())
    {
        return 1;
    }

    debug_trace_init(); // Initialize tracing
    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(s_debug.trace_file, "\n=== Per-Pixel Initialization ===\n");
        fmt::print(s_debug.trace_file, "Pixel: ({}, {})\n", g_col, g_row);
        fmt::print(s_debug.trace_file, "Pixel coords: ({:.6f}, {:.6f})\n", dx_pixel(), dy_pixel());
    }

    g_overflow = false;
    s_runtime.jump_index = 0;
    s_runtime.op_ptr = 0;
    s_runtime.set_random = false;
    if (s_formula.uses_rand && !s_runtime.randomized)
    {
        random_seed();
    }
    g_store_index = 0;
    g_load_index = 0;
    g_arg1 = s_runtime.stack.data();
    g_arg2 = s_runtime.stack.data();
    g_arg2--;

    s_formula.vars[10].a.d.x = static_cast<double>(g_col);
    s_formula.vars[10].a.d.y = static_cast<double>(g_row);

    if (g_row + g_col & 1)
    {
        s_formula.vars[9].a.d.x = 1.0;
    }
    else
    {
        s_formula.vars[9].a.d.x = 0.0;
    }
    s_formula.vars[9].a.d.y = 0.0;

    if (g_inversion.invert != 0)
    {
        invertz2(&g_old_z);
        s_formula.vars[0].a.d.x = g_old_z.x;
        s_formula.vars[0].a.d.y = g_old_z.y;
    }
    else
    {
        s_formula.vars[0].a.d.x = dx_pixel();
        s_formula.vars[0].a.d.y = engine::dy_pixel();
    }

    if (g_last_init_op)
    {
        g_last_init_op = s_formula.op_count;
    }
    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(s_debug.trace_file, "\nInitialization operations:\n");
        s_debug.operation_count = 0;
    }
    while (s_runtime.op_ptr < g_last_init_op)
    {
        s_formula.fns[s_runtime.op_ptr]();
        s_runtime.op_ptr++;
    }
    s_runtime.init_load_ptr = g_load_index;
    s_runtime.init_store_ptr = g_store_index;
    s_runtime.init_op_ptr = s_runtime.op_ptr;
    // Set old variable for orbits
    engine::g_old_z = s_formula.vars[3].a.d;

    return g_overflow ? 0 : 1;
}

int formula_orbit()
{
    if (g_formula_name.empty() || g_overflow)
    {
        return 1;
    }

    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(s_debug.trace_file, "\n=== Orbit Calculation ===\n");
        fmt::print(s_debug.trace_file, "Input z: ({:.6f}, {:.6f})\n", 
                   g_old_z.x, g_old_z.y);
        s_debug.operation_count = 0;
    }

    g_load_index = s_runtime.init_load_ptr;
    g_store_index = s_runtime.init_store_ptr;
    s_runtime.op_ptr = s_runtime.init_op_ptr;
    s_runtime.jump_index = s_runtime.init_jump_index;
    // Set the random number
    if (s_runtime.set_random || s_runtime.randomized)
    {
        d_random();
    }

    g_arg1 = s_runtime.stack.data();
    g_arg2 = s_runtime.stack.data();
    --g_arg2;
    while (s_runtime.op_ptr < static_cast<int>(s_formula.op_count))
    {
        s_formula.fns[s_runtime.op_ptr]();
        s_runtime.op_ptr++;
    }

    g_new_z = s_formula.vars[3].a.d;
    g_old_z = g_new_z;

    if (s_debug.trace_enabled && s_debug.trace_file)
    {
        fmt::print(s_debug.trace_file, "Output z: ({:.6f}, {:.6f})\n", g_new_z.x, g_new_z.y);
        fmt::print(s_debug.trace_file, "Bailout test: {} (result: {})\n", g_arg1->d.x,
            g_arg1->d.x == 0.0 ? "continue" : "bailout");
        std::fflush(s_debug.trace_file);
    }

    return g_arg1->d.x == 0.0;
}

} // namespace id::fractals
