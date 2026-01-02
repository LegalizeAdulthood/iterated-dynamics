// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/arg.h"

#include <filesystem>
#include <string>
#include <vector>

namespace id::fractals
{

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
    int op_count{};                        // Total compiled operations
    bool uses_jump{};                      // Whether formula uses jumps
    bool uses_rand{};                      // Whether formula uses rand
    int last_init_op{};                    //
    int load_index{};                      //
    int store_index{};                     //
    int op_index{};                        //
    int var_index{};                       //
    int max_function{};                    //
    int max_ops{};                         //
    int max_args{};                        //
    bool uses_ismand{};                    // true if the formula uses ismand variable
    bool uses_p1{};                        // true if the formula uses p1 variable
    bool uses_p2{};                        // true if the formula uses p2 variable
    bool uses_p3{};                        // true if the formula uses p3 variable
    bool uses_p4{};                        // true if the formula uses p4 variable
    bool uses_p5{};                        // true if the formula uses p5 variable
};

extern CompiledFormula       g_formula;

int frm_get_param_stuff(std::filesystem::path &path, const char *name);
bool parse_formula(std::filesystem::path &path, const std::string &name, bool report_bad_sym);
void init_misc();
void free_work_area();

/// Get current parser state for testing/debugging
std::string get_parser_state();

struct FormulaEntry
{
    std::string name;
    std::string symmetry;
    std::string body;
};

/// Parse formula for testing without executing
bool parse_formula(const FormulaEntry &formula_entry, bool report_bad_sym);

// Reset parser state
void parser_reset();

} // namespace id::fractals
