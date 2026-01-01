// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/formula.h"

#include "fractals/interpreter.h"
#include "fractals/parser.h"
#include "io/library.h"
#include "math/arg.h"
#include "misc/debug_flags.h"

#include <fmt/format.h>

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

} // namespace id::fractals
