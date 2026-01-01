// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

namespace id::fractals
{

extern std::filesystem::path g_formula_filename;
extern std::string           g_formula_name;
extern bool                  g_frm_uses_ismand;
extern bool                  g_frm_uses_p1;
extern bool                  g_frm_uses_p2;
extern bool                  g_frm_uses_p3;
extern bool                  g_frm_uses_p4;
extern bool                  g_frm_uses_p5;
extern bool                  g_is_mandelbrot;
extern int                   g_last_init_op;
extern int                   g_load_index;
extern char                  g_max_function;
extern unsigned              g_max_function_args;
extern unsigned              g_max_function_ops;
extern unsigned              g_operation_index;
extern int                   g_store_index;
extern unsigned              g_variable_index;

int frm_get_param_stuff(const char *name);
bool parse_formula(const std::string &name, bool report_bad_sym);
void init_misc();
void free_work_area();

/// Get current parser state for testing/debugging
std::string get_parser_state();

/// Parse formula for testing without executing
bool parse_formula(const std::string &formula_text, std::string &error_msg);

// Reset parser state
void parser_reset();


//////////////////////////////////////////////////

struct DebugState
{
    bool trace_enabled{};
    std::FILE *trace_file{};
    int indent_level{};
    long operation_count{};
};

extern DebugState s_debug;
void debug_trace_init();

} // namespace id::fractals
