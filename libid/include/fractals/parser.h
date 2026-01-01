// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/arg.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

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

int frm_get_param_stuff(std::filesystem::path &path, const char *name);
bool parse_formula(std::filesystem::path &path, const std::string &name, bool report_bad_sym);
void init_misc();
void free_work_area();

/// Get current parser state for testing/debugging
std::string get_parser_state();

/// Parse formula for testing without executing
bool parse_formula(const std::string &formula_text, std::string &error_msg);

// Reset parser state
void parser_reset();


//////////////////////////////////////////////////

enum class JumpControlType
{
    NONE = 0,
    IF = 1,
    ELSE_IF = 2,
    ELSE = 3,
    END_IF = 4
};

struct ConstArg
{
    const char *s;
    int len;
    math::Arg a;
};

struct JumpPtrs
{
    int jump_op_ptr;
    int jump_lod_ptr;
    int jump_sto_ptr;
};

struct JumpControl
{
    JumpControlType type;
    JumpPtrs ptrs;
    int dest_jump_index;
};

using Function = void();
using FunctionPtr = Function *;

struct PendingOp
{
    FunctionPtr f;
    int p;
};

struct CompiledFormula
{
    std::string formula;                   // Source text
    std::vector<FunctionPtr> fns;          // Compiled operations (bytecode)
    std::vector<math::Arg *> load;         // Load table
    std::vector<math::Arg *> store;        // Store table
    std::vector<ConstArg> vars;            // All constants/variables
    std::vector<JumpControl> jump_control; // Jump control structure
    std::vector<PendingOp> ops;            // Pending operations (used during compilation)
    unsigned int op_count{};               // Total compiled operations
    bool uses_jump{};                      // Whether formula uses jumps
    bool uses_rand{};                      // Whether formula uses rand
};

extern CompiledFormula g_formula;

} // namespace id::fractals
