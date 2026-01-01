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

struct DebugState
{
    bool trace_enabled{};
    std::FILE *trace_file{};
    int indent_level{};
    long operation_count{};
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
    std::string formula;                     // Source text
    std::vector<FunctionPtr> fns;            // Compiled operations (bytecode)
    std::vector<math::Arg *> load;                 // Load table
    std::vector<math::Arg *> store;                // Store table
    std::vector<ConstArg> vars;              // All constants/variables
    std::vector<JumpControl> jump_control;   // Jump control structure
    std::vector<PendingOp> ops;              // Pending operations (used during compilation)
    unsigned int op_count{};                 // Total compiled operations
    bool uses_jump{};                        // Whether formula uses jumps
    bool uses_rand{};                        // Whether formula uses rand
};

struct RuntimeState
{
    std::array<math::Arg, 20> stack{};
    int op_ptr{};
    int jump_index{};

    int init_op_ptr{};
    int init_jump_index{};
    int init_load_ptr{};
    int init_store_ptr{};

    bool set_random{};
    bool randomized{};
    unsigned long rand_num{};
    long rand_x{};
    long rand_y{};
};

constexpr int BIT_SHIFT{16};

extern CompiledFormula s_formula;
extern DebugState s_debug;
extern RuntimeState s_runtime;
void debug_trace_init();
void random_seed();
void d_stk_srand();
void d_random();

void d_stk_lod_dup();
void d_stk_lod_sqr();
void d_stk_lod_sqr2();
void d_stk_lod_dbl();
void d_stk_sqr0();
void d_stk_sqr3();
void d_stk_sub();
void d_stk_real();
void d_stk_imag();
void d_stk_neg();
void d_stk_div();
void d_stk_mod();
void stk_sto();
void stk_lod();
void d_stk_lt();
void d_stk_gt();
void d_stk_lte();
void d_stk_gte();
void d_stk_eq();
void d_stk_ne();
void d_stk_or();
void d_stk_and();
void d_stk_fn1();
void stk_clr();
void d_stk_fn2();
void d_stk_fn3();
void d_stk_fn4();
void end_init();
void stk_jump();
void d_stk_jump_on_false();
void d_stk_jump_on_true();
void stk_jump_label();

#define LAST_SQR (s_formula.vars[4].a)

extern const std::array<const char *, 19> VARIABLES;

} // namespace id::fractals
