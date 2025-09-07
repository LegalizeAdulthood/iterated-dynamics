// SPDX-License-Identifier: GPL-3.0-only
//
/* (C) 1990, Mark C. Peterson
   All rights reserved.

   Code may be used in any program provided the author is credited
    either during program execution or in the documentation.  Source
    code may be distributed only in combination with public domain or
    shareware source code.  Source code may be modified provided the
    copyright notice and this message is left unchanged and all
    modifications are clearly documented.

    I would appreciate a copy of any work which incorporates this code,
    however this is optional.

    Mark C. Peterson
*/
#include "fractals/parser.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/convert_center_mag.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/jiim.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/newton.h"
#include "io/library.h"
#include "math/arg.h"
#include "math/cmplx.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "math/rand15.h"
#include "math/sign.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/file_item.h"
#include "ui/stop_msg.h"

#include <config/string_case_compare.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <vector>

using namespace id;

namespace
{

using Function = void();
using FunctionPtr = Function *;

enum
{
    MAX_OPS = 250,
    MAX_ARGS = 100,
    MAX_JUMPS = 200 // size of JUMP_CONTROL array
};

enum class JumpControlType
{
    NONE = 0,
    IF = 1,
    ELSE_IF = 2,
    ELSE = 3,
    END_IF = 4
};

// token_type definitions
enum class FormulaTokenType
{
    NOT_A_TOKEN = 0,
    PARENS = 1,
    PARAM_VARIABLE = 2,
    USER_NAMED_VARIABLE = 3,
    PREDEFINED_VARIABLE = 4,
    REAL_CONSTANT = 5,
    COMPLEX_CONSTANT = 6,
    FUNCTION = 7,
    PARAM_FUNCTION = 8,
    FLOW_CONTROL = 9,
    OPERATOR = 10,
    END_OF_FORMULA = 11
};
inline int operator+(FormulaTokenType value)
{
    return static_cast<int>(value);
}

// token IDs
enum class TokenId
{
    NONE = 0,                   // no relevant id for token
    END_OF_FILE = 1,            // begin error condition ids
    ILLEGAL_CHARACTER = 2,      //
    ILLEGAL_VARIABLE_NAME = 3,  //
    TOKEN_TOO_LONG = 4,         //
    FUNC_USED_AS_VAR = 5,       //
    JUMP_MISSING_BOOLEAN = 6,   //
    JUMP_WITH_ILLEGAL_CHAR = 7, //
    UNDEFINED_FUNCTION = 8,     //
    ILLEGAL_OPERATOR = 9,       //
    ILL_FORMED_CONSTANT = 10,   // end error condition ids
    OPEN_PARENS = 1,            //
    CLOSE_PARENS = -1,          //
    FUNC_SIN = 0,               // these ids must match up with the entries in s_func_list
    FUNC_SINH = 1,              //
    FUNC_COS = 2,               //
    FUNC_COSH = 3,              //
    FUNC_SQR = 4,               //
    FUNC_LOG = 5,               //
    FUNC_EXP = 6,               //
    FUNC_ABS = 7,               //
    FUNC_CONJ = 8,              //
    FUNC_REAL = 9,              //
    FUNC_IMAG = 10,             //
    FUNC_FN1 = 11,              //
    FUNC_FN2 = 12,              //
    FUNC_FN3 = 13,              //
    FUNC_FN4 = 14,              //
    FUNC_FLIP = 15,             //
    FUNC_TAN = 16,              //
    FUNC_TANH = 17,             //
    FUNC_COTAN = 18,            //
    FUNC_COTANH = 19,           //
    FUNC_COSXX = 20,            //
    FUNC_SRAND = 21,            //
    FUNC_ASIN = 22,             //
    FUNC_ASINH = 23,            //
    FUNC_ACOS = 24,             //
    FUNC_ACOSH = 25,            //
    FUNC_ATAN = 26,             //
    FUNC_ATANH = 27,            //
    FUNC_SQRT = 28,             //
    FUNC_CABS = 29,             //
    FUNC_FLOOR = 30,            //
    FUNC_CEIL = 31,             //
    FUNC_TRUNC = 32,            //
    FUNC_ROUND = 33,            // end of function name ids
    OP_COMMA = 0,               // these ids must match up with the entries in s_op_list
    OP_NOT_EQUAL = 1,           //
    OP_ASSIGN = 2,              //
    OP_EQUAL = 3,               //
    OP_LT = 4,                  //
    OP_LE = 5,                  //
    OP_GT = 6,                  //
    OP_GE = 7,                  //
    OP_MODULUS = 8,             //
    OP_OR = 9,                  //
    OP_AND = 10,                //
    OP_COLON = 11,              //
    OP_PLUS = 12,               //
    OP_MINUS = 13,              //
    OP_MULTIPLY = 14,           //
    OP_DIVIDE = 15,             //
    OP_POWER = 16,              // end of operator name ids
    VAR_PIXEL = 0,              // these ids must match up with the entries in s_variables
    VAR_P1 = 1,                 //
    VAR_P2 = 2,                 //
    VAR_Z = 3,                  //
    VAR_LAST_SQR = 4,           //
    VAR_PI = 5,                 //
    VAR_E = 6,                  //
    VAR_RAND = 7,               //
    VAR_P3 = 8,                 //
    VAR_WHITE_SQUARE = 9,       //
    VAR_SCREEN_PIX = 10,        //
    VAR_SCREEN_MAX = 11,        //
    VAR_MAX_ITER = 12,          //
    VAR_IS_MANDEL = 13,         //
    VAR_CENTER = 14,            //
    VAR_MAG_X_MAG = 15,         //
    VAR_ROTATION_SKEW = 16,     //
    VAR_P4 = 17,                //
    VAR_P5 = 18,                // end of predefined variable name ids
    JUMP_IF = 1,                // these ids must match up with the entries in s_jump_list, offset by one
    JUMP_ELSE_IF = 2,            //
    JUMP_ELSE = 3,              //
    JUMP_END_IF = 4,             // end of jump list name ids
};
inline int operator+(TokenId value)
{
    return static_cast<int>(value);
}

enum class ParseError
{
    NONE = -1,
    SHOULD_BE_ARGUMENT = 0,
    SHOULD_BE_OPERATOR = 1,
    NEED_A_MATCHING_OPEN_PARENS = 2,
    NEED_MORE_CLOSE_PARENS = 3,
    UNDEFINED_OPERATOR = 4,
    UNDEFINED_FUNCTION = 5,
    TABLE_OVERFLOW = 6,
    NO_MATCH_RIGHT_PAREN = 7,
    NO_LEFT_BRACKET_FIRST_LINE = 8,
    UNEXPECTED_EOF = 9,
    INVALID_SYM_USING_NOSYM = 10,
    FORMULA_TOO_LARGE = 11,
    INSUFFICIENT_MEM_FOR_TYPE_FORMULA = 12,
    COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED = 13,
    JUMP_NOT_FIRST = 14,
    NO_CHAR_AFTER_THIS_JUMP = 15,
    JUMP_NEEDS_BOOLEAN = 16,
    ENDIF_REQUIRED_AFTER_ELSE = 17,
    ENDIF_WITH_NO_IF = 18,
    MISPLACED_ELSE_OR_ELSEIF = 19,
    UNMATCHED_IF_IN_INIT_SECTION = 20,
    IF_WITH_NO_ENDIF = 21,
    ERROR_IN_PARSING_JUMP_STATEMENTS = 22,
    TOO_MANY_JUMPS = 23,
    FORMULA_NAME_TOO_LARGE = 24,
    ILLEGAL_ASSIGNMENT = 25,
    ILLEGAL_VAR_NAME = 26,
    INVALID_CONST = 27,
    ILLEGAL_CHAR = 28,
    NESTING_TO_DEEP = 29,
    UNMATCHED_MODULUS = 30,
    FUNC_USED_AS_VAR = 31,
    NO_NEG_AFTER_EXPONENT = 32,
    TOKEN_TOO_LONG = 33,
    SECOND_COLON = 34,
    INVALID_CALL_TO_PARSE_ERRS = 35
};
inline int operator+(ParseError value)
{
    return static_cast<int>(value);
}

struct PendingOp
{
    FunctionPtr f;
    int p;
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

struct Token
{
    char str[80];
    FormulaTokenType type;
    TokenId id;
    DComplex constant;
};

struct FunctList
{
    const char *s;
    FunctionPtr *ptr;
};

struct SymmetryName
{
    const char *s;
    SymmetryType n;
};

struct ErrorData
{
    long start_pos;
    long error_pos;
    ParseError error_number;
};

constexpr int BIT_SHIFT{16};

struct ConstArg
{
    const char *s;
    int len;
    Arg a;
};

} // namespace

// forward declarations
static bool frm_prescan(std::FILE *open_file);
static void parser_allocate();
static void d_stk_srand();
static void d_stk_add();
static void d_stk_sub();
static void d_stk_real();
static void d_stk_imag();
static void d_stk_neg();
static void d_stk_div();
static void d_stk_mod();
static void d_stk_lt();
static void d_stk_gt();
static void d_stk_lte();
static void d_stk_gte();
static void d_stk_eq();
static void d_stk_ne();
static void d_stk_or();
static void d_stk_and();
static void end_init();
static void d_stk_jump_on_false();
static void d_stk_jump_on_true();

unsigned int g_max_function_ops{MAX_OPS};
unsigned int g_max_function_args{MAX_ARGS};
int g_store_index{};
int g_load_index{};
bool g_is_mandelbrot{true};
unsigned int g_operation_index{};
unsigned int g_variable_index{};
int g_last_init_op{};
bool g_frm_uses_p1{};
bool g_frm_uses_p2{};
bool g_frm_uses_p3{};
bool g_frm_uses_p4{};
bool g_frm_uses_p5{};
bool g_frm_uses_ismand{};
char g_max_function{};

static std::vector<JumpControl> s_jump_control;
static int s_jump_index{};
static std::array<Arg, 20> s_stack{};
static std::vector<Arg *> s_load;
static int s_op_ptr{};
static std::vector<FunctionPtr> s_fns;
static std::vector<ConstArg> s_vars;
#define LAST_SQR (s_vars[4].a)
static unsigned int s_op_count{};
static int s_init_load_ptr{};
static int s_init_store_ptr{};
static int s_init_op_ptr{};
static bool s_uses_jump{};
static std::vector<Arg *> s_store;
static unsigned long s_num_ops{};
static unsigned long s_num_loads{};
static unsigned long s_num_stores{};
static unsigned long s_num_jumps{};
static int s_init_jump_index{};
static std::vector<PendingOp> s_op;
static unsigned int s_n{};
static unsigned int s_next_op{};
static unsigned int s_init_n{};
static int s_paren{};
static bool s_expecting_arg{};
static bool s_set_random{};
static bool s_randomized{};
static unsigned long s_rand_num{};
static long s_rand_x{};
static long s_rand_y{};
static unsigned int s_chars_in_formula{};
static constexpr std::array<const char *, 4> JUMP_LIST
{
    "if",
    "elseif",
    "else",
    "endif"
};
static std::string s_formula;
static std::array<ErrorData, 3> s_errors{};

static FunctionPtr s_srand{d_stk_srand};
static FunctionPtr s_abs{d_stk_abs};
static FunctionPtr s_sqr{d_stk_sqr};
static FunctionPtr s_add{d_stk_add};
static FunctionPtr s_sub{d_stk_sub};
static FunctionPtr s_conj{d_stk_conj};
static FunctionPtr s_floor{d_stk_floor};
static FunctionPtr s_ceil{d_stk_ceil};
static FunctionPtr s_trunc{d_stk_trunc};
static FunctionPtr s_round{d_stk_round};
static FunctionPtr s_zero{d_stk_zero};
static FunctionPtr s_one{d_stk_one};
static FunctionPtr s_real{d_stk_real};
static FunctionPtr s_imag{d_stk_imag};
static FunctionPtr s_neg{d_stk_neg};
static FunctionPtr s_mul{d_stk_mul};
static FunctionPtr s_div{d_stk_div};
static FunctionPtr s_mod{d_stk_mod};
static FunctionPtr s_flip{d_stk_flip};
static FunctionPtr s_sin{d_stk_sin};
static FunctionPtr s_tan{d_stk_tan};
static FunctionPtr s_tanh{d_stk_tanh};
static FunctionPtr s_cotan{d_stk_cotan};
static FunctionPtr s_cotanh{d_stk_cotanh};
static FunctionPtr s_sinh{d_stk_sinh};
static FunctionPtr s_cos{d_stk_cos};
static FunctionPtr s_cosxx{d_stk_cosxx};
static FunctionPtr s_cosh{d_stk_cosh};
static FunctionPtr s_asin{d_stk_asin};
static FunctionPtr s_asinh{d_stk_asinh};
static FunctionPtr s_acos{d_stk_acos};
static FunctionPtr s_acosh{d_stk_acosh};
static FunctionPtr s_atan{d_stk_atan};
static FunctionPtr s_atanh{d_stk_atanh};
static FunctionPtr s_sqrt{d_stk_sqrt};
static FunctionPtr s_cabs{d_stk_cabs};
static FunctionPtr s_lt{d_stk_lt};
static FunctionPtr s_gt{d_stk_gt};
static FunctionPtr s_lte{d_stk_lte};
static FunctionPtr s_gte{d_stk_gte};
static FunctionPtr s_eq{d_stk_eq};
static FunctionPtr s_ne{d_stk_ne};
static FunctionPtr s_or{d_stk_or};
static FunctionPtr s_and{d_stk_and};
static FunctionPtr s_log{d_stk_log};
static FunctionPtr s_exp{d_stk_exp};
static FunctionPtr s_pwr{d_stk_pwr};
static FunctionPtr s_jump_on_false{d_stk_jump_on_false};
static FunctionPtr s_jump_on_true{d_stk_jump_on_true};
static FunctionPtr s_trig0{d_stk_sin};
static FunctionPtr s_trig1{d_stk_sqr};
static FunctionPtr s_trig2{d_stk_sinh};
static FunctionPtr s_trig3{d_stk_cosh};
static constexpr std::array<FunctList, 34> FUNC_LIST
{
    FunctList{"sin",   &s_sin},
    FunctList{"sinh",  &s_sinh},
    FunctList{"cos",   &s_cos},
    FunctList{"cosh",  &s_cosh},
    FunctList{"sqr",   &s_sqr},
    FunctList{"log",   &s_log},
    FunctList{"exp",   &s_exp},
    FunctList{"abs",   &s_abs},
    FunctList{"conj",  &s_conj},
    FunctList{"real",  &s_real},
    FunctList{"imag",  &s_imag},
    FunctList{"fn1",   &s_trig0},
    FunctList{"fn2",   &s_trig1},
    FunctList{"fn3",   &s_trig2},
    FunctList{"fn4",   &s_trig3},
    FunctList{"flip",  &s_flip},
    FunctList{"tan",   &s_tan},
    FunctList{"tanh",  &s_tanh},
    FunctList{"cotan", &s_cotan},
    FunctList{"cotanh", &s_cotanh},
    FunctList{"cosxx", &s_cosxx},
    FunctList{"srand", &s_srand},
    FunctList{"asin",  &s_asin},
    FunctList{"asinh", &s_asinh},
    FunctList{"acos",  &s_acos},
    FunctList{"acosh", &s_acosh},
    FunctList{"atan",  &s_atan},
    FunctList{"atanh", &s_atanh},
    FunctList{"sqrt",  &s_sqrt},
    FunctList{"cabs",  &s_cabs},
    FunctList{"floor", &s_floor},
    FunctList{"ceil",  &s_ceil},
    FunctList{"trunc", &s_trunc},
    FunctList{"round", &s_round},
};
static std::array<const char *, 17> s_op_list
{
    ",",    //  0
    "!=",   //  1
    "=",    //  2
    "==",   //  3
    "<",    //  4
    "<=",   //  5
    ">",    //  6
    ">=",   //  7
    "|",    //  8
    "||",   //  9
    "&&",   // 10
    ":",    // 11
    "+",    // 12
    "-",    // 13
    "*",    // 14
    "/",    // 15
    "^"     // 16
};
static constexpr std::array<const char *, 19> VARIABLES
{
    "pixel",        // v[0]
    "p1",           // v[1]
    "p2",           // v[2]
    "z",            // v[3]
    "LastSqr",      // v[4]
    "pi",           // v[5]
    "e",            // v[6]
    "rand",         // v[7]
    "p3",           // v[8]
    "whitesq",      // v[9]
    "scrnpix",      // v[10]
    "scrnmax",      // v[11]
    "maxit",        // v[12]
    "ismand",       // v[13]
    "center",       // v[14]
    "magxmag",      // v[15]
    "rotskew",      // v[16]
    "p4",           // v[17]
    "p5"            // v[18]
};
static constexpr std::array<SymmetryName, 14> SYMMETRY_NAMES
{
    SymmetryName{ "NOSYM",         SymmetryType::NONE },
    SymmetryName{ "XAXIS_NOPARM",  SymmetryType::X_AXIS_NO_PARAM },
    SymmetryName{ "XAXIS",         SymmetryType::X_AXIS },
    SymmetryName{ "YAXIS_NOPARM",  SymmetryType::Y_AXIS_NO_PARAM },
    SymmetryName{ "YAXIS",         SymmetryType::Y_AXIS },
    SymmetryName{ "XYAXIS_NOPARM", SymmetryType::XY_AXIS_NO_PARAM },
    SymmetryName{ "XYAXIS",        SymmetryType::XY_AXIS },
    SymmetryName{ "ORIGIN_NOPARM", SymmetryType::ORIGIN_NO_PARAM },
    SymmetryName{ "ORIGIN",        SymmetryType::ORIGIN },
    SymmetryName{ "PI_SYM_NOPARM", SymmetryType::PI_SYM_NO_PARAM },
    SymmetryName{ "PI_SYM",        SymmetryType::PI_SYM },
    SymmetryName{ "XAXIS_NOIMAG",  SymmetryType::X_AXIS_NO_IMAG },
    SymmetryName{ "XAXIS_NOREAL",  SymmetryType::X_AXIS_NO_REAL },
    SymmetryName{ "NOPLOT",        SymmetryType::NO_PLOT },
};

static void push_jump(JumpControlType type)
{
    JumpControl value{};
    value.type = type;
    s_jump_control.push_back(value);
    ++s_jump_index;
}

#define CASE_TERMINATOR case',':\
                        case '\n':\
                        case '(':\
                        case ')':\
                        case '!':\
                        case '=':\
                        case '<':\
                        case '>':\
                        case '|':\
                        case '&':\
                        case '}':\
                        case ':':\
                        case '+':\
                        case '-':\
                        case '*':\
                        case '/':\
                        case '^'

#define CASE_ALPHA      case 'a':\
                        case 'b':\
                        case 'c':\
                        case 'd':\
                        case 'e':\
                        case 'f':\
                        case 'g':\
                        case 'h':\
                        case 'i':\
                        case 'j':\
                        case 'k':\
                        case 'l':\
                        case 'm':\
                        case 'n':\
                        case 'o':\
                        case 'p':\
                        case 'q':\
                        case 'r':\
                        case 's':\
                        case 't':\
                        case 'u':\
                        case 'v':\
                        case 'w':\
                        case 'x':\
                        case 'y':\
                        case 'z'

#define CASE_NUM        case '0':\
                        case '1':\
                        case '2':\
                        case '3':\
                        case '4':\
                        case '5':\
                        case '6':\
                        case '7':\
                        case '8':\
                        case '9'

// MAX_STORES must be even to make Unix alignment work

#define MAX_STORES ((g_max_function_ops/4)*2)  // at most only half the ops can be stores
#define MAX_LOADS  ((unsigned)(g_max_function_ops*.8))  // and 80% can be loads

static bool check_denom(double denom)
{
    if (std::abs(denom) <= DBL_MIN)
    {
        g_overflow = true;
        return true;
    }
    return false;
}

static const char *parse_error_text(ParseError which)
{
    // the entries in this array need to correspond to the ParseError enum values
    static constexpr const char *const MESSAGES[]{
        "Should be an Argument",
        "Should be an Operator",
        "')' needs a matching '('",
        "Need more ')'",
        "Undefined Operator",
        "Undefined Function",
        "Table overflow",
        "Didn't find matching ')' in symmetry declaration",
        "No '{' found on first line",
        "Unexpected EOF!",
        "Symmetry below is invalid, will use NOSYM",
        "Formula is too large",
        "Insufficient memory to run fractal type 'formula'",
        "Could not open file where formula located",
        "No characters may precede jump instruction",
        "No characters may follow this jump instruction",
        "Jump instruction missing required (boolean argument)",
        R"msg(Next jump after "else" must be "endif")msg",
        R"msg("endif" has no matching "if")msg",
        R"msg(Misplaced "else" or "elseif()")msg",
        R"msg("if ()" in initialization has no matching "endif")msg",
        R"msg("if ()" has no matching "endif")msg",
        "Error in parsing jump statements",
        "Formula has too many jump commands",
        "Formula name has too many characters",
        "Only variables are allowed to the left of assignment",
        "Illegal variable name",
        "Invalid constant expression",
        "This character not supported by parser",
        "Nesting of parentheses exceeds maximum depth",
        R"msg(Unmatched modulus operator "|" in this expression)msg",
        "Can't use function name as variable",
        "Negative exponent must be enclosed in parens",
        "Variable or constant exceeds 32 character limit",
        R"msg(Only one ":" permitted in a formula)msg",
        "Invalid ParseErrs code",
    };
    constexpr int last_err = std::size(MESSAGES) - 1;
    if (+which > last_err)
    {
        which = static_cast<ParseError>(last_err);
    }
    return MESSAGES[+which];
}

/* use the following when only float functions are implemented to
   get MP math and Integer math */

static unsigned long new_random_num()
{
    s_rand_num = ((s_rand_num << 15) + RAND15()) ^ s_rand_num;
    return s_rand_num;
}

static void d_random()
{
    /* Use the same algorithm as for fixed math so that they will generate
           the same fractals when the srand() function is used. */
    long x = new_random_num() >> (32 - BIT_SHIFT);
    long y = new_random_num() >> (32 - BIT_SHIFT);
    s_vars[7].a.d.x = ((double)x / (1L << BIT_SHIFT));
    s_vars[7].a.d.y = ((double)y / (1L << BIT_SHIFT));
}

static void set_random()
{
    if (!s_set_random)
    {
        s_rand_num = s_rand_x ^ s_rand_y;
    }

    const unsigned int seed = (unsigned) s_rand_num ^ (unsigned) (s_rand_num >> 16);
    std::srand(seed);
    s_set_random = true;

    // Clear out the seed
    new_random_num();
    new_random_num();
    new_random_num();
}

static void random_seed()
{
    std::time_t now;

    // Use the current time to randomize the random number sequence.
    std::time(&now);
    std::srand((unsigned int)now);

    new_random_num();
    new_random_num();
    new_random_num();
    s_randomized = true;
}

static void d_stk_srand()
{
    s_rand_x = (long)(g_arg1->d.x * (1L << BIT_SHIFT));
    s_rand_y = (long)(g_arg1->d.y * (1L << BIT_SHIFT));
    set_random();
    d_random();
    g_arg1->d = s_vars[7].a.d;
}

static void d_stk_lod_dup()
{
    g_arg1 += 2;
    g_arg2 += 2;
    *g_arg1 = *s_load[g_load_index];
    *g_arg2 = *g_arg1;
    g_load_index += 2;
}

static void d_stk_lod_sqr()
{
    g_arg1++;
    g_arg2++;
    g_arg1->d.y = s_load[g_load_index]->d.x * s_load[g_load_index]->d.y * 2.0;
    g_arg1->d.x = (s_load[g_load_index]->d.x * s_load[g_load_index]->d.x) - (s_load[g_load_index]->d.y * s_load[g_load_index]->d.y);
    g_load_index++;
}

static void d_stk_lod_sqr2()
{
    g_arg1++;
    g_arg2++;
    LAST_SQR.d.x = s_load[g_load_index]->d.x * s_load[g_load_index]->d.x;
    LAST_SQR.d.y = s_load[g_load_index]->d.y * s_load[g_load_index]->d.y;
    g_arg1->d.y = s_load[g_load_index]->d.x * s_load[g_load_index]->d.y * 2.0;
    g_arg1->d.x = LAST_SQR.d.x - LAST_SQR.d.y;
    LAST_SQR.d.x += LAST_SQR.d.y;
    LAST_SQR.d.y = 0;
    g_load_index++;
}

static void d_stk_lod_dbl()
{
    g_arg1++;
    g_arg2++;
    g_arg1->d.x = s_load[g_load_index]->d.x * 2.0;
    g_arg1->d.y = s_load[g_load_index]->d.y * 2.0;
    g_load_index++;
}

static void d_stk_sqr0()
{
    LAST_SQR.d.y = g_arg1->d.y * g_arg1->d.y; // use LastSqr as temp storage
    g_arg1->d.y = g_arg1->d.x * g_arg1->d.y * 2.0;
    g_arg1->d.x = g_arg1->d.x * g_arg1->d.x - LAST_SQR.d.y;
}

static void d_stk_sqr3()
{
    g_arg1->d.x = g_arg1->d.x * g_arg1->d.x;
}

void d_stk_abs()
{
    g_arg1->d.x = std::abs(g_arg1->d.x);
    g_arg1->d.y = std::abs(g_arg1->d.y);
}

void d_stk_sqr()
{
    LAST_SQR.d.x = g_arg1->d.x * g_arg1->d.x;
    LAST_SQR.d.y = g_arg1->d.y * g_arg1->d.y;
    g_arg1->d.y = g_arg1->d.x * g_arg1->d.y * 2.0;
    g_arg1->d.x = LAST_SQR.d.x - LAST_SQR.d.y;
    LAST_SQR.d.x += LAST_SQR.d.y;
    LAST_SQR.d.y = 0;
}

static void d_stk_add()
{
    g_arg2->d.x += g_arg1->d.x;
    g_arg2->d.y += g_arg1->d.y;
    g_arg1--;
    g_arg2--;
}

static void d_stk_sub()
{
    g_arg2->d.x -= g_arg1->d.x;
    g_arg2->d.y -= g_arg1->d.y;
    g_arg1--;
    g_arg2--;
}

void d_stk_conj()
{
    g_arg1->d.y = -g_arg1->d.y;
}

void d_stk_floor()
{
    g_arg1->d.x = floor(g_arg1->d.x);
    g_arg1->d.y = floor(g_arg1->d.y);
}

void d_stk_ceil()
{
    g_arg1->d.x = ceil(g_arg1->d.x);
    g_arg1->d.y = ceil(g_arg1->d.y);
}

void d_stk_trunc()
{
    g_arg1->d.x = (int)(g_arg1->d.x);
    g_arg1->d.y = (int)(g_arg1->d.y);
}

void d_stk_round()
{
    g_arg1->d.x = floor(g_arg1->d.x+.5);
    g_arg1->d.y = floor(g_arg1->d.y+.5);
}

void d_stk_zero()
{
    g_arg1->d.x = 0.0;
    g_arg1->d.y = g_arg1->d.x;
}

void d_stk_one()
{
    g_arg1->d.x = 1.0;
    g_arg1->d.y = 0.0;
}

static void d_stk_real()
{
    g_arg1->d.y = 0.0;
}

static void d_stk_imag()
{
    g_arg1->d.x = g_arg1->d.y;
    g_arg1->d.y = 0.0;
}

static void d_stk_neg()
{
    g_arg1->d.x = -g_arg1->d.x;
    g_arg1->d.y = -g_arg1->d.y;
}

void d_stk_mul()
{
    fpu_cmplx_mul(&g_arg2->d, &g_arg1->d, &g_arg2->d);
    g_arg1--;
    g_arg2--;
}

static void d_stk_div()
{
    fpu_cmplx_div(&g_arg2->d, &g_arg1->d, &g_arg2->d);
    g_arg1--;
    g_arg2--;
}

static void d_stk_mod()
{
    g_arg1->d.x = (g_arg1->d.x * g_arg1->d.x) + (g_arg1->d.y * g_arg1->d.y);
    g_arg1->d.y = 0.0;
}

static void stk_sto()
{
    assert(s_store[g_store_index] != nullptr);
    *s_store[g_store_index++] = *g_arg1;
}

static void stk_lod()
{
    g_arg1++;
    g_arg2++;
    *g_arg1 = *s_load[g_load_index++];
}

static void stk_clr()
{
    s_stack[0] = *g_arg1;
    g_arg1 = s_stack.data();
    g_arg2 = s_stack.data();
    g_arg2--;
}

void d_stk_flip()
{
    double t = g_arg1->d.x;
    g_arg1->d.x = g_arg1->d.y;
    g_arg1->d.y = t;
}

void d_stk_sin()
{
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;

    sin_cos(g_arg1->d.x, &sin_x, &cos_x);
    sinh_cosh(g_arg1->d.y, &sinh_y, &cosh_y);
    g_arg1->d.x = sin_x*cosh_y;
    g_arg1->d.y = cos_x*sinh_y;
}

/* The following functions are supported by both the parser and for fn
   variable replacement.
*/
void d_stk_tan()
{
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.x, &sin_x, &cos_x);
    sinh_cosh(g_arg1->d.y, &sinh_y, &cosh_y);
    double denom = cos_x + cosh_y;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sin_x/denom;
    g_arg1->d.y = sinh_y/denom;
}

void d_stk_tanh()
{
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.y, &sin_y, &cos_y);
    sinh_cosh(g_arg1->d.x, &sinh_x, &cosh_x);
    double denom = cosh_x + cos_y;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sinh_x/denom;
    g_arg1->d.y = sin_y/denom;
}

void d_stk_cotan()
{
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.x, &sin_x, &cos_x);
    sinh_cosh(g_arg1->d.y, &sinh_y, &cosh_y);
    double denom = cosh_y - cos_x;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sin_x/denom;
    g_arg1->d.y = -sinh_y/denom;
}

void d_stk_cotanh()
{
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;
    g_arg1->d.x *= 2;
    g_arg1->d.y *= 2;
    sin_cos(g_arg1->d.y, &sin_y, &cos_y);
    sinh_cosh(g_arg1->d.x, &sinh_x, &cosh_x);
    double denom = cosh_x - cos_y;
    if (check_denom(denom))
    {
        return;
    }
    g_arg1->d.x = sinh_x/denom;
    g_arg1->d.y = -sin_y/denom;
}

/* The following functions are not directly used by the parser - support
   for the parser was not provided because the existing parser language
   represents these quite easily. They are used for fn variable support
   in miscres.c but are placed here because they follow the pattern of
   the other parser functions.
*/

void d_stk_recip()
{
    double mod = g_arg1->d.x * g_arg1->d.x + g_arg1->d.y * g_arg1->d.y;
    if (check_denom(mod))
    {
        return;
    }
    g_arg1->d.x =  g_arg1->d.x/mod;
    g_arg1->d.y = -g_arg1->d.y/mod;
}

void stk_ident()
{
    // do nothing - the function Z
}

void d_stk_sinh()
{
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;

    sin_cos(g_arg1->d.y, &sin_y, &cos_y);
    sinh_cosh(g_arg1->d.x, &sinh_x, &cosh_x);
    g_arg1->d.x = sinh_x*cos_y;
    g_arg1->d.y = cosh_x*sin_y;
}

void d_stk_cos()
{
    double sin_x;
    double cos_x;
    double sinh_y;
    double cosh_y;

    sin_cos(g_arg1->d.x, &sin_x, &cos_x);
    sinh_cosh(g_arg1->d.y, &sinh_y, &cosh_y);
    g_arg1->d.x = cos_x*cosh_y;
    g_arg1->d.y = -sin_x*sinh_y;
}

// Bogus version of cos, to replicate bug which was in regular cos till v16:

void d_stk_cosxx()
{
    d_stk_cos();
    g_arg1->d.y = -g_arg1->d.y;
}

void d_stk_cosh()
{
    double sin_y;
    double cos_y;
    double sinh_x;
    double cosh_x;

    sin_cos(g_arg1->d.y, &sin_y, &cos_y);
    sinh_cosh(g_arg1->d.x, &sinh_x, &cosh_x);
    g_arg1->d.x = cosh_x*cos_y;
    g_arg1->d.y = sinh_x*sin_y;
}

void d_stk_asin()
{
    asin_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_asinh()
{
    asinh_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_acos()
{
    acos_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_acosh()
{
    acosh_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_atan()
{
    atan_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_atanh()
{
    atanh_z(g_arg1->d, &(g_arg1->d));
}

void d_stk_sqrt()
{
    g_arg1->d = complex_sqrt_float(g_arg1->d.x, g_arg1->d.y);
}

void d_stk_cabs()
{
    g_arg1->d.x = std::sqrt(sqr(g_arg1->d.x)+sqr(g_arg1->d.y));
    g_arg1->d.y = 0.0;
}

static void d_stk_lt()
{
    g_arg2->d.x = (double)(g_arg2->d.x < g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_gt()
{
    g_arg2->d.x = (double)(g_arg2->d.x > g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_lte()
{
    g_arg2->d.x = (double)(g_arg2->d.x <= g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_gte()
{
    g_arg2->d.x = (double)(g_arg2->d.x >= g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_eq()
{
    g_arg2->d.x = (double)(g_arg2->d.x == g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_ne()
{
    g_arg2->d.x = (double)(g_arg2->d.x != g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_or()
{
    g_arg2->d.x = (double)(g_arg2->d.x || g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

static void d_stk_and()
{
    g_arg2->d.x = (double)(g_arg2->d.x && g_arg1->d.x);
    g_arg2->d.y = 0.0;
    g_arg1--;
    g_arg2--;
}

void d_stk_log()
{
    fpu_cmplx_log(&g_arg1->d, &g_arg1->d);
}

void d_stk_exp()
{
    fpu_cmplx_exp(&g_arg1->d, &g_arg1->d);
}

void d_stk_pwr()
{
    g_arg2->d = complex_power(g_arg2->d, g_arg1->d);
    g_arg1--;
    g_arg2--;
}

static void end_init()
{
    g_last_init_op = s_op_ptr;
    s_init_jump_index = s_jump_index;
}

static void stk_jump()
{
    s_op_ptr =  s_jump_control[s_jump_index].ptrs.jump_op_ptr;
    g_load_index = s_jump_control[s_jump_index].ptrs.jump_lod_ptr;
    g_store_index = s_jump_control[s_jump_index].ptrs.jump_sto_ptr;
    s_jump_index = s_jump_control[s_jump_index].dest_jump_index;
}

static void d_stk_jump_on_false()
{
    if (g_arg1->d.x == 0)
    {
        stk_jump();
    }
    else
    {
        s_jump_index++;
    }
}

static void d_stk_jump_on_true()
{
    if (g_arg1->d.x)
    {
        stk_jump();
    }
    else
    {
        s_jump_index++;
    }
}

static void stk_jump_label()
{
    s_jump_index++;
}

static unsigned int skip_white_space(const char *str)
{
    unsigned n;
    bool done = false;
    for (n = 0; !done; n++)
    {
        switch (str[n])
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            break;
        default:
            done = true;
        }
    }
    return n - 1;
}

// detect if constant is part of a (a,b) construct
static bool is_const_pair(const char *str)
{
    int n;
    bool answer = false;
    // skip past first number
    for (n = 0; std::isdigit(str[n]) || str[n] == '.'; n++)
    {
    }
    if (str[n] == ',')
    {
        int j = n + skip_white_space(&str[n + 1]) + 1;
        if (std::isdigit(str[j])
            || (str[j] == '-' && (std::isdigit(str[j+1]) || str[j+1] == '.'))
            || str[j] == '.')
        {
            answer = true;
        }
    }
    return answer;
}

static ConstArg *is_const(const char *str, int len)
{
    // next line enforces variable vs constant naming convention
    for (unsigned n = 0U; n < g_variable_index; n++)
    {
        if (s_vars[n].len == len)
        {
            if (id::string_case_equal(s_vars[n].s, str, len))
            {
                if (n == 1)          // The formula uses 'p1'.
                {
                    g_frm_uses_p1 = true;
                }
                if (n == 2)          // The formula uses 'p2'.
                {
                    g_frm_uses_p2 = true;
                }
                if (n == 7)          // The formula uses 'rand'.
                {
                    random_seed();
                }
                if (n == 8)          // The formula uses 'p3'.
                {
                    g_frm_uses_p3 = true;
                }
                if (n == 13)          // The formula uses 'ismand'.
                {
                    g_frm_uses_ismand = true;
                }
                if (n == 17)          // The formula uses 'p4'.
                {
                    g_frm_uses_p4 = true;
                }
                if (n == 18)          // The formula uses 'p5'.
                {
                    g_frm_uses_p5 = true;
                }
                if (!is_const_pair(str))
                {
                    return &s_vars[n];
                }
            }
        }
    }
    s_vars[g_variable_index].s = str;
    s_vars[g_variable_index].len = len;
    s_vars[g_variable_index].a.d.y = 0.0;
    s_vars[g_variable_index].a.d.x = s_vars[g_variable_index].a.d.y;

    if (std::isdigit(str[0])
        || (str[0] == '-' && (std::isdigit(str[1]) || str[1] == '.'))
        || str[0] == '.')
    {
        DComplex z;
        assert(g_operation_index > 0);
        assert(g_operation_index == s_op.size());
        if (s_op.back().f == s_neg)
        {
            s_op.pop_back();
            g_operation_index--;
            str = str - 1;
            s_init_n--;
            s_vars[g_variable_index].len++;
        }
        unsigned n;
        for (n = 1; std::isdigit(str[n]) || str[n] == '.'; n++)
        {
        }
        if (str[n] == ',')
        {
            unsigned j = n + skip_white_space(&str[n+1]) + 1;
            if (std::isdigit(str[j])
                || (str[j] == '-' && (std::isdigit(str[j+1]) || str[j+1] == '.'))
                || str[j] == '.')
            {
                z.y = std::atof(&str[j]);
                for (; std::isdigit(str[j]) || str[j] == '.' || str[j] == '-'; j++)
                {
                }
                s_vars[g_variable_index].len = j;
            }
            else
            {
                z.y = 0.0;
            }
        }
        else
        {
            z.y = 0.0;
        }
        z.x = std::atof(str);
        s_vars[g_variable_index].a.d = z;
        s_vars[g_variable_index].s = str;
    }
    return &s_vars[g_variable_index++];
}

/* return values
    0 - Not a jump
    1 - if
    2 - elseif
    3 - else
    4 - endif
*/
static JumpControlType is_jump(const char *str, int len)
{
    for (int i = 0; i < static_cast<int>(JUMP_LIST.size()); i++)
    {
        if ((int) std::strlen(JUMP_LIST[i]) == len && id::string_case_equal(JUMP_LIST[i], str, len))
        {
            return static_cast<JumpControlType>(i + 1);
        }
    }
    return JumpControlType::NONE;
}

static void not_a_funct()
{
}

static void funct_not_found()
{
}

// determine if s names a function and if so which one
static int which_fn(const char *s, int len)
{
    int out;
    if (len != 3)
    {
        out = 0;
    }
    else if (!id::string_case_equal(s, "fn", 2))
    {
        out = 0;
    }
    else
    {
        out = std::atoi(s+2);
    }
    if (out < 1 || out > 4)
    {
        out = 0;
    }
    return out;
}

static FunctionPtr is_func(const char *str, int len)
{
    unsigned n = skip_white_space(&str[len]);
    if (str[len+n] == '(')
    {
        for (n = 0; n < static_cast<unsigned>(FUNC_LIST.size()); n++)
        {
            if (id::string_case_equal(FUNC_LIST[n].s, str, len))
            {
                // count function variables
                int funct_num = which_fn(str, len);
                if (funct_num != 0)
                {
                    if (funct_num > g_max_function)
                    {
                        g_max_function = (char)funct_num;
                    }
                }
                return *FUNC_LIST[n].ptr;
            }
        }
        return funct_not_found;
    }
    return not_a_funct;
}

static void sort_precedence()
{
    int this_op = s_next_op++;
    while (s_op[this_op].p > s_op[s_next_op].p && s_next_op < g_operation_index)
    {
        sort_precedence();
    }
    if (s_op_ptr > static_cast<int>(s_fns.size()))
    {
        throw std::runtime_error(
            "OpPtr (" + std::to_string(s_op_ptr) + ") exceeds size of f[] (" + std::to_string(s_fns.size()) + ")");
    }
    s_fns.push_back(s_op[this_op].f);
    ++s_op_ptr;
}

static void push_pending_op(FunctionPtr f, int p)
{
    s_op.push_back(PendingOp{f, p});
    ++g_operation_index;
    assert(g_operation_index == s_op.size());
}

static bool parse_formula_text(const std::string &text)
{
    int mod_flag = 999;
    int len;
    int equals = 0;
    std::array<int, 20> mods{};
    int m_d_stk = 0;
    double x_ctr;
    double y_ctr;
    double x_mag_factor;
    double rotation;
    double skew;
    LDouble magnification;
    s_set_random = false;
    s_randomized = false;
    s_uses_jump = false;
    s_jump_index = 0;
    s_jump_control.clear();

    s_add = d_stk_add;
    s_sub = d_stk_sub;
    s_neg = d_stk_neg;
    s_mul = d_stk_mul;
    s_sin = d_stk_sin;
    s_sinh = d_stk_sinh;
    s_lt = d_stk_lt;
    s_lte = d_stk_lte;
    s_mod = d_stk_mod;
    s_sqr = d_stk_sqr;
    s_cos = d_stk_cos;
    s_cosh = d_stk_cosh;
    s_log = d_stk_log;
    s_exp = d_stk_exp;
    s_pwr = d_stk_pwr;
    s_div = d_stk_div;
    s_abs = d_stk_abs;
    s_real = d_stk_real;
    s_imag = d_stk_imag;
    s_conj = d_stk_conj;
    s_trig0 = g_d_trig0;
    s_trig1 = g_d_trig1;
    s_trig2 = g_d_trig2;
    s_trig3 = g_d_trig3;
    s_flip = d_stk_flip;
    s_tan = d_stk_tan;
    s_tanh = d_stk_tanh;
    s_cotan = d_stk_cotan;
    s_cotanh = d_stk_cotanh;
    s_cosxx = d_stk_cosxx;
    s_gt = d_stk_gt;
    s_gte = d_stk_gte;
    s_eq = d_stk_eq;
    s_ne = d_stk_ne;
    s_and = d_stk_and;
    s_or = d_stk_or;
    s_srand = d_stk_srand;
    s_asin = d_stk_asin;
    s_asinh = d_stk_asinh;
    s_acos = d_stk_acos;
    s_acosh = d_stk_acosh;
    s_atan = d_stk_atan;
    s_atanh = d_stk_atanh;
    s_cabs = d_stk_cabs;
    s_sqrt = d_stk_sqrt;
    s_zero = d_stk_zero;
    s_floor = d_stk_floor;
    s_ceil = d_stk_ceil;
    s_trunc = d_stk_trunc;
    s_round = d_stk_round;
    s_jump_on_true = d_stk_jump_on_true;
    s_jump_on_false = d_stk_jump_on_false;
    s_one = d_stk_one;
    g_max_function = 0;
    for (g_variable_index = 0; g_variable_index < static_cast<unsigned>(VARIABLES.size()); g_variable_index++)
    {
        s_vars[g_variable_index].s = VARIABLES[g_variable_index];
        s_vars[g_variable_index].len = (int) std::strlen(VARIABLES[g_variable_index]);
    }
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
    double const_pi = std::atan(1.0) * 4;
    double const_e = std::exp(1.0);
    s_vars[7].a.d.y = 0.0;
    s_vars[7].a.d.x = s_vars[7].a.d.y;
    s_vars[11].a.d.x = (double)g_logical_screen_x_dots;
    s_vars[11].a.d.y = (double)g_logical_screen_y_dots;
    s_vars[12].a.d.x = (double)g_max_iterations;
    s_vars[12].a.d.y = 0;
    s_vars[13].a.d.x = g_is_mandelbrot ? 1.0 : 0.0;
    s_vars[13].a.d.y = 0;
    s_vars[14].a.d.x = x_ctr;
    s_vars[14].a.d.y = y_ctr;
    s_vars[15].a.d.x = (double)magnification;
    s_vars[15].a.d.y = x_mag_factor;
    s_vars[16].a.d.x = rotation;
    s_vars[16].a.d.y = skew;

    s_vars[1].a.d.x = g_params[0];
    s_vars[1].a.d.y = g_params[1];
    s_vars[2].a.d.x = g_params[2];
    s_vars[2].a.d.y = g_params[3];
    s_vars[5].a.d.x = const_pi;
    s_vars[5].a.d.y = 0.0;
    s_vars[6].a.d.x = const_e;
    s_vars[6].a.d.y = 0.0;
    s_vars[8].a.d.x = g_params[4];
    s_vars[8].a.d.y = g_params[5];
    s_vars[17].a.d.x = g_params[6];
    s_vars[17].a.d.y = g_params[7];
    s_vars[18].a.d.x = g_params[8];
    s_vars[18].a.d.y = g_params[9];

    g_operation_index = 0;
    s_op.clear();
    g_store_index = 0;
    g_load_index = 0;
    s_op_ptr = 0;
    s_paren = 0;
    g_last_init_op = 0;
    s_expecting_arg = true;
    for (s_n = 0; text[s_n]; s_n++)
    {
        if (!text[s_n])
        {
            break;
        }
        s_init_n = s_n;
        switch (text[s_n])
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;
        case '(':
            s_paren++;
            break;
        case ')':
            s_paren--;
            break;
        case '|':
            if (text[s_n+1] == '|')
            {
                s_expecting_arg = true;
                s_n++;
                push_pending_op(s_or, 7 - (s_paren + equals) * 15);
            }
            else if (mod_flag == s_paren-1)
            {
                s_paren--;
                mod_flag = mods[--m_d_stk];
            }
            else
            {
                assert(m_d_stk < mods.size());
                mods[m_d_stk++] = mod_flag;
                push_pending_op(s_mod, 2 - (s_paren + equals) * 15);
                mod_flag = s_paren++;
            }
            break;
        case ',':
        case ';':
            if (!s_expecting_arg)
            {
                s_expecting_arg = true;
                push_pending_op(nullptr, 15);
                push_pending_op(stk_clr, -30000);
                s_paren = 0;
                equals = 0;
            }
            break;
        case ':':
            s_expecting_arg = true;
            push_pending_op(nullptr, 15);
            push_pending_op(end_init, -30000);
            s_paren = 0;
            equals = 0;
            g_last_init_op = 10000;
            break;
        case '+':
            s_expecting_arg = true;
            push_pending_op(s_add, 4 - (s_paren + equals)*15);
            break;
        case '-':
            if (s_expecting_arg)
            {
                push_pending_op(s_neg, 2 - (s_paren + equals)*15);
            }
            else
            {
                push_pending_op(s_sub, 4 - (s_paren + equals)*15);
                s_expecting_arg = true;
            }
            break;
        case '&':
            s_expecting_arg = true;
            s_n++;
            push_pending_op(s_and, 7 - (s_paren + equals)*15);
            break;
        case '!':
            s_expecting_arg = true;
            s_n++;
            push_pending_op(s_ne, 6 - (s_paren + equals)*15);
            break;
        case '<':
            s_expecting_arg = true;
            {
                FunctionPtr fn;
                if (text[s_n + 1] == '=')
                {
                    s_n++;
                    fn = s_lte;
                }
                else
                {
                    fn = s_lt;
                }
                push_pending_op(fn, 6 - (s_paren + equals) * 15);
            }
            break;
        case '>':
            s_expecting_arg = true;
            {
                FunctionPtr fn;
                if (text[s_n + 1] == '=')
                {
                    s_n++;
                    fn = s_gte;
                }
                else
                {
                    fn = s_gt;
                }
                push_pending_op(fn, 6 - (s_paren + equals) * 15);
            }
            break;
        case '*':
            s_expecting_arg = true;
            push_pending_op(s_mul, 3 - (s_paren + equals)*15);
            break;
        case '/':
            s_expecting_arg = true;
            push_pending_op(s_div, 3 - (s_paren + equals)*15);
            break;
        case '^':
            s_expecting_arg = true;
            push_pending_op(s_pwr, 2 - (s_paren + equals)*15);
            break;
        case '=':
            s_expecting_arg = true;
            if (text[s_n+1] == '=')
            {
                s_n++;
                push_pending_op(s_eq, 6 - (s_paren + equals)*15);
            }
            else
            {
                s_op[g_operation_index-1].f = stk_sto;
                s_op[g_operation_index-1].p = 5 - (s_paren + equals)*15;
                s_store[g_store_index++] = s_load[--g_load_index];
                equals++;
            }
            break;
        default:
            while (std::isalnum(text[s_n+1]) || text[s_n+1] == '.' || text[s_n+1] == '_')
            {
                s_n++;
            }
            len = (s_n+1)-s_init_n;
            s_expecting_arg = false;
            if (const JumpControlType type = is_jump(&text[s_init_n], len); type != JumpControlType::NONE)
            {
                s_uses_jump = true;
                switch (type)
                {
                case JumpControlType::IF:
                    s_expecting_arg = true;
                    push_jump(JumpControlType::IF);
                    push_pending_op(s_jump_on_false, 1);
                    break;
                case JumpControlType::ELSE_IF:
                    s_expecting_arg = true;
                    push_jump(JumpControlType::ELSE_IF);
                    push_jump(JumpControlType::ELSE_IF);
                    push_pending_op(stk_jump, 1);
                    push_pending_op(nullptr, 15);
                    push_pending_op(stk_clr, -30000);
                    push_pending_op(s_jump_on_false, 1);
                    break;
                case JumpControlType::ELSE:
                    push_jump(JumpControlType::ELSE);
                    push_pending_op(stk_jump, 1);
                    break;
                case JumpControlType::END_IF:
                    push_jump(JumpControlType::END_IF);
                    push_pending_op(stk_jump_label, 1);
                    break;
                default:
                    break;
                }
            }
            else
            {
                if (const FunctionPtr fn = is_func(&text[s_init_n], len); fn != not_a_funct)
                {
                    push_pending_op(fn,  1 - (s_paren + equals)*15);
                    s_expecting_arg = true;
                }
                else
                {
                    ConstArg *c = is_const(&text[s_init_n], len);
                    s_load[g_load_index++] = &(c->a);
                    push_pending_op(stk_lod, 1 - (s_paren + equals)*15);
                    s_n = s_init_n + c->len - 1;
                }
            }
            break;
        }
    }
    push_pending_op(nullptr, 16);
    s_next_op = 0;
    s_op_count = g_operation_index;
    while (s_next_op < g_operation_index)
    {
        if (s_op[s_next_op].f)
        {
            sort_precedence();
        }
        else
        {
            s_next_op++;
            s_op_count--;
        }
    }
    return false;
}

int formula_orbit()
{
    if (g_formula_name.empty() || g_overflow)
    {
        return 1;
    }

    g_load_index = s_init_load_ptr;
    g_store_index = s_init_store_ptr;
    s_op_ptr = s_init_op_ptr;
    s_jump_index = s_init_jump_index;
    // Set the random number
    if (s_set_random || s_randomized)
    {
        d_random();
    }

    g_arg1 = s_stack.data();
    g_arg2 = s_stack.data();
    --g_arg2;
    while (s_op_ptr < (int) s_op_count)
    {
        s_fns[s_op_ptr]();
        s_op_ptr++;
    }

    g_new_z = s_vars[3].a.d;
    g_old_z = g_new_z;
    return g_arg1->d.x == 0.0;
}

int formula_per_pixel()
{
    if (g_formula_name.empty())
    {
        return 1;
    }
    g_overflow = false;
    s_jump_index = 0;
    s_op_ptr = 0;
    g_store_index = 0;
    g_load_index = 0;
    g_arg1 = s_stack.data();
    g_arg2 = s_stack.data();
    g_arg2--;

    s_vars[10].a.d.x = (double)g_col;
    s_vars[10].a.d.y = (double) g_row;

    if ((g_row + g_col) & 1)
    {
        s_vars[9].a.d.x = 1.0;
    }
    else
    {
        s_vars[9].a.d.x = 0.0;
    }
    s_vars[9].a.d.y = 0.0;

    {
        if (g_invert != 0)
        {
            invertz2(&g_old_z);
            s_vars[0].a.d.x = g_old_z.x;
            s_vars[0].a.d.y = g_old_z.y;
        }
        else
        {
            s_vars[0].a.d.x = g_dx_pixel();
            s_vars[0].a.d.y = g_dy_pixel();
        }
    }

    if (g_last_init_op)
    {
        g_last_init_op = s_op_count;
    }
    while (s_op_ptr < g_last_init_op)
    {
        s_fns[s_op_ptr]();
        s_op_ptr++;
    }
    s_init_load_ptr = g_load_index;
    s_init_store_ptr = g_store_index;
    s_init_op_ptr = s_op_ptr;
    // Set old variable for orbits
    g_old_z = s_vars[3].a.d;

    return g_overflow ? 0 : 1;
}

static int fill_if_group(int endif_index, JumpPtrs *jump_data)
{
    int i   = endif_index;
    int ljp = endif_index; // ljp means "last jump processed"
    while (i > 0)
    {
        i--;
        switch (s_jump_control[i].type)
        {
        case JumpControlType::IF:    //if (); this concludes processing of this group
            s_jump_control[i].ptrs = jump_data[ljp];
            s_jump_control[i].dest_jump_index = ljp + 1;
            return i;
        case JumpControlType::ELSE_IF:    //elseif* (2 jumps, the 'else' and the 'if')
            // first, the "if" part
            s_jump_control[i].ptrs = jump_data[ljp];
            s_jump_control[i].dest_jump_index = ljp + 1;

            // then, the else part
            i--; //fall through to "else" is intentional
        case JumpControlType::ELSE:
            s_jump_control[i].ptrs = jump_data[endif_index];
            s_jump_control[i].dest_jump_index = endif_index + 1;
            ljp = i;
            break;
        case JumpControlType::END_IF:    //endif
            i = fill_if_group(i, jump_data);
            break;
        default:
            break;
        }
    }
    return -1; // should never get here
}

static bool fill_jump_struct()
{
    // Completes all entries in jump structure. Returns true on error.
    // On entry, jump_index is the number of jump functions in the formula
    int i = 0;
    int load_count = 0;
    int store_count = 0;
    bool check_for_else = false;
    FunctionPtr jump_func = nullptr;
    bool find_new_func = true;

    std::vector<JumpPtrs> jump_data;

    for (s_op_ptr = 0; s_op_ptr < (int) s_op_count; s_op_ptr++)
    {
        if (find_new_func)
        {
            if (i < static_cast<int>(s_jump_control.size()))
            {
                switch (s_jump_control[i].type)
                {
                case JumpControlType::IF:
                    jump_func = s_jump_on_false;
                    break;
                case JumpControlType::ELSE_IF:
                    check_for_else = !check_for_else;
                    if (check_for_else)
                    {
                        jump_func = stk_jump;
                    }
                    else
                    {
                        jump_func = s_jump_on_false;
                    }
                    break;
                case JumpControlType::ELSE:
                    jump_func = stk_jump;
                    break;
                case JumpControlType::END_IF:
                    jump_func = stk_jump_label;
                    break;
                default:
                    break;
                }
            }
            find_new_func = false;
        }
        if (*(s_fns[s_op_ptr]) == stk_lod)
        {
            load_count++;
        }
        else if (*(s_fns[s_op_ptr]) == stk_sto)
        {
            store_count++;
        }
        else if (*(s_fns[s_op_ptr]) == jump_func)
        {
            JumpPtrs value{};
            value.jump_op_ptr = s_op_ptr;
            value.jump_lod_ptr = load_count;
            value.jump_sto_ptr = store_count;
            jump_data.push_back(value);
            i++;
            find_new_func = true;
        }
    }

    // Following for safety only; all should always be false
    if (i != s_jump_index
        || s_jump_control[i - 1].type != JumpControlType::END_IF
        || s_jump_control[0].type != JumpControlType::IF)
    {
        return true;
    }

    while (i > 0)
    {
        i--;
        i = fill_if_group(i, jump_data.data());
    }
    return i < 0;
}

static int frm_get_char(std::FILE *open_file)
{
    int c;
    bool done = false;
    bool line_wrap = false;
    while (!done)
    {
        c = std::getc(open_file);
        switch (c)
        {
        case '\r':
        case ' ' :
        case '\t' :
            break;
        case '\\':
            line_wrap = true;
            break;
        case ';' :
            while ((c = std::getc(open_file)) != '\n' && c != EOF)
            {
            }
            if (c == EOF)
            {
                done = true;
            }
        case '\n' :
            if (!line_wrap)
            {
                done = true;
            }
            line_wrap = false;
            break;
        default:
            done = true;
            break;
        }
    }
    return std::tolower(c);
}

// This function also gets flow control info

static void get_func_info(Token *tok)
{
    for (int i = 0; i < static_cast<int>(FUNC_LIST.size()); i++)
    {
        if (!std::strcmp(FUNC_LIST[i].s, tok->str))
        {
            tok->id = static_cast<TokenId>(i);
            if (tok->id >= TokenId::FUNC_FN1 && tok->id <= TokenId::FUNC_FN4)
            {
                tok->type = FormulaTokenType::PARAM_FUNCTION;
            }
            else
            {
                tok->type = FormulaTokenType::FUNCTION;
            }
            return;
        }
    }

    for (int i = 0; i < static_cast<int>(JUMP_LIST.size()); i++) // pick up flow control
    {
        if (std::strcmp(JUMP_LIST[i], tok->str) == 0)
        {
            tok->type = FormulaTokenType::FLOW_CONTROL;
            tok->id   = static_cast<TokenId>(i + 1);
            return;
        }
    }
    tok->type = FormulaTokenType::NOT_A_TOKEN;
    tok->id   = TokenId::UNDEFINED_FUNCTION;
}

static void get_var_info(Token *tok)
{
    for (int i = 0; i < static_cast<int>(VARIABLES.size()); i++)
    {
        if (!std::strcmp(VARIABLES[i], tok->str))
        {
            tok->id = static_cast<TokenId>(i);
            switch (tok->id)
            {
            case TokenId::VAR_P1:
            case TokenId::VAR_P2:
            case TokenId::VAR_P3:
            case TokenId::VAR_IS_MANDEL:
            case TokenId::VAR_P4:
            case TokenId::VAR_P5:
                tok->type = FormulaTokenType::PARAM_VARIABLE;
                break;
            default:
                tok->type = FormulaTokenType::PREDEFINED_VARIABLE;
                break;
            }
            return;
        }
    }
    tok->type = FormulaTokenType::USER_NAMED_VARIABLE;
    tok->id   = TokenId::NONE;
}

// fills in token structure where numeric constant is indicated
/* Note - this function will be called twice to fill in the components
        of a complex constant. See is_complex_constant() below. */

// returns 1 on success, 0 on NOT_A_TOKEN
static bool frm_get_constant(std::FILE *open_file, Token *tok)
{
    int i = 1;
    bool getting_base = true;
    long file_pos = std::ftell(open_file);
    bool got_decimal_already = false;
    bool done = false;
    tok->constant.x = 0.0;          //initialize values to 0
    tok->constant.y = 0.0;
    if (tok->str[0] == '.')
    {
        got_decimal_already = true;
    }
    while (!done)
    {
        switch (int c = frm_get_char(open_file); c)
        {
        case EOF:
            tok->str[i] = (char) 0;
            tok->type = FormulaTokenType::NOT_A_TOKEN;
            tok->id   = TokenId::END_OF_FILE;
            return false;
        CASE_NUM:
            tok->str[i++] = (char) c;
            file_pos = std::ftell(open_file);
            break;
        case '.':
            if (got_decimal_already || !getting_base)
            {
                tok->str[i++] = (char) c;
                tok->str[i++] = (char) 0;
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            tok->str[i++] = (char) c;
            got_decimal_already = true;
            file_pos = std::ftell(open_file);
            break;
        default :
            if (c == 'e' && getting_base && (std::isdigit(tok->str[i-1]) || (tok->str[i-1] == '.' && i > 1)))
            {
                tok->str[i++] = (char) c;
                getting_base = false;
                got_decimal_already = false;
                file_pos = std::ftell(open_file);
                c = frm_get_char(open_file);
                if (c == '-' || c == '+')
                {
                    tok->str[i++] = (char) c;
                    file_pos = std::ftell(open_file);
                }
                else
                {
                    std::fseek(open_file, file_pos, SEEK_SET);
                }
            }
            else if (std::isalpha(c) || c == '_')
            {
                tok->str[i++] = (char) c;
                tok->str[i++] = (char) 0;
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            else if (tok->str[i-1] == 'e' || (tok->str[i-1] == '.' && i == 1))
            {
                tok->str[i++] = (char) c;
                tok->str[i++] = (char) 0;
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                tok->str[i++] = (char) 0;
                done = true;
            }
            break;
        }
        if (i == 33 && tok->str[32])
        {
            tok->str[33] = (char) 0;
            tok->type = FormulaTokenType::NOT_A_TOKEN;
            tok->id = TokenId::TOKEN_TOO_LONG;
            return false;
        }
    }    // end of while loop. Now fill in the value
    tok->constant.x = std::atof(tok->str);
    tok->type = FormulaTokenType::REAL_CONSTANT;
    tok->id   = TokenId::NONE;
    return true;
}

static void is_complex_constant(std::FILE *open_file, Token *tok)
{
    assert(tok->str[0] == '(');
    Token temp_tok;
    int sign_value = 1;
    bool done = false;
    bool getting_real = true;
    std::FILE * debug_token = nullptr;
    tok->str[1] = (char) 0;  // so we can concatenate later

    long file_pos = std::ftell(open_file);
    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        const std::filesystem::path path{id::io::get_save_path(id::io::WriteFile::ROOT, "frmconst.txt")};
        assert(!path.empty());
        debug_token = std::fopen(path.string().c_str(), "at");
    }

    while (!done)
    {
        int c = frm_get_char(open_file);
        switch (c)
        {
CASE_NUM :
        case '.':
            if (debug_token != nullptr)
            {
                fmt::print(debug_token, "Set temp_tok.token_str[0] to {:c}\n", c);
            }
            temp_tok.str[0] = (char) c;
            break;
        case '-' :
            if (debug_token != nullptr)
            {
                fmt::print(debug_token,  "First char is a minus\n");
            }
            sign_value = -1;
            c = frm_get_char(open_file);
            if (c == '.' || std::isdigit(c))
            {
                if (debug_token != nullptr)
                {
                    fmt::print(debug_token, "Set temp_tok.token_str[0] to {:c}\n", c);
                }
                temp_tok.str[0] = (char) c;
            }
            else
            {
                if (debug_token != nullptr)
                {
                    fmt::print(debug_token,  "First char not a . or NUM\n");
                }
                done = true;
            }
            break;
        default:
            if (debug_token != nullptr)
            {
                fmt::print(debug_token,  "First char not a . or NUM\n");
            }
            done = true;
            break;
        }
        if (debug_token != nullptr)
        {
            fmt::print(debug_token, "Calling frmgetconstant unless done is true; done is {:s}\n",
                done ? "true" : "false");
        }
        if (!done && frm_get_constant(open_file, &temp_tok))
        {
            c = frm_get_char(open_file);
            if (debug_token != nullptr)
            {
                fmt::print(debug_token, "frmgetconstant returned 1; next token is {:c}\n", c);
            }
            if (getting_real && c == ',')   // we have the real part now
            {
                if (sign_value == -1)
                {
                    std::strcat(tok->str, "-");
                }
                std::strcat(tok->str, temp_tok.str);
                std::strcat(tok->str, ",");
                tok->constant.x = temp_tok.constant.x * sign_value;
                getting_real = false;
                sign_value = 1;
            }
            else if (!getting_real && c == ')') // we have the complex part
            {
                if (sign_value == -1)
                {
                    std::strcat(tok->str, "-");
                }
                std::strcat(tok->str, temp_tok.str);
                std::strcat(tok->str, ")");
                tok->constant.y = temp_tok.constant.x * sign_value;
                tok->type = tok->constant.y ? FormulaTokenType::COMPLEX_CONSTANT : FormulaTokenType::REAL_CONSTANT;
                tok->id   = TokenId::NONE;
                if (debug_token != nullptr)
                {
                    fmt::print(debug_token, "Exiting with type set to {:d}\n",
                        +(tok->constant.y ? FormulaTokenType::COMPLEX_CONSTANT : FormulaTokenType::REAL_CONSTANT));
                    std::fclose(debug_token);
                }
                return;
            }
            else
            {
                done = true;
            }
        }
        else
        {
            done = true;
        }
    }
    std::fseek(open_file, file_pos, SEEK_SET);
    tok->str[1] = (char) 0;
    tok->constant.x = 0.0;
    tok->constant.y = tok->constant.x;
    tok->type = FormulaTokenType::PARENS;
    tok->id = TokenId::OPEN_PARENS;
    if (debug_token != nullptr)
    {
        fmt::print(debug_token,  "Exiting with ID set to OPEN_PARENS\n");
        std::fclose(debug_token);
    }
}

static bool frm_get_alpha(std::FILE *open_file, Token *tok)
{
    int c;
    int i = 1;
    bool var_name_too_long = false;
    long last_file_pos = std::ftell(open_file);
    while ((c = frm_get_char(open_file)) != EOF)
    {
        long file_pos = std::ftell(open_file);
        switch (c)
        {
CASE_ALPHA:
CASE_NUM:
        case '_':
            if (i < 79)
            {
                tok->str[i++] = (char) c;
            }
            else
            {
                tok->str[i] = (char) 0;
            }
            if (i == 33)
            {
                var_name_too_long = true;
            }
            last_file_pos = file_pos;
            break;
        default:
            if (c == '.')       // illegal character in variable or func name
            {
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::ILLEGAL_VARIABLE_NAME;
                tok->str[i++] = '.';
                tok->str[i] = (char) 0;
                return false;
            }
            if (var_name_too_long)
            {
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::TOKEN_TOO_LONG;
                tok->str[i] = (char) 0;
                std::fseek(open_file, last_file_pos, SEEK_SET);
                return false;
            }
            tok->str[i] = (char) 0;
            std::fseek(open_file, last_file_pos, SEEK_SET);
            get_func_info(tok);
            if (c == '(') //getfuncinfo() correctly filled structure
            {
                if (tok->type == FormulaTokenType::NOT_A_TOKEN)
                {
                    return false;
                }
                if (tok->type == FormulaTokenType::FLOW_CONTROL &&
                    (tok->id == TokenId::ILLEGAL_VARIABLE_NAME || tok->id == TokenId::TOKEN_TOO_LONG))
                {
                    tok->type = FormulaTokenType::NOT_A_TOKEN;
                    tok->id   = TokenId::JUMP_WITH_ILLEGAL_CHAR;
                    return false;
                }
                return true;
            }
            //can't use function names as variables
            if (tok->type == FormulaTokenType::FUNCTION || tok->type == FormulaTokenType::PARAM_FUNCTION)
            {
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::FUNC_USED_AS_VAR;
                return false;
            }
            if (tok->type == FormulaTokenType::FLOW_CONTROL &&
                (tok->id == TokenId::END_OF_FILE || tok->id == TokenId::ILLEGAL_CHARACTER))
            {
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::JUMP_MISSING_BOOLEAN;
                return false;
            }
            if (tok->type == FormulaTokenType::FLOW_CONTROL &&
                (tok->id == TokenId::ILLEGAL_VARIABLE_NAME || tok->id == TokenId::TOKEN_TOO_LONG))
            {
                if (c == ',' || c == '\n' || c == ':')
                {
                    return true;
                }
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::JUMP_WITH_ILLEGAL_CHAR;
                return false;
            }
            get_var_info(tok);
            return true;
        }
    }
    tok->str[0] = (char) 0;
    tok->type = FormulaTokenType::NOT_A_TOKEN;
    tok->id   = TokenId::END_OF_FILE;
    return false;
}

static void frm_get_eos(std::FILE *open_file, Token *this_token)
{
    long last_file_pos = std::ftell(open_file);
    int c;

    for (c = frm_get_char(open_file); (c == '\n' || c == ',' || c == ':'); c = frm_get_char(open_file))
    {
        if (c == ':')
        {
            this_token->str[0] = ':';
        }
        last_file_pos = std::ftell(open_file);
    }
    if (c == '}')
    {
        this_token->str[0] = '}';
        this_token->type = FormulaTokenType::END_OF_FORMULA;
        this_token->id   = TokenId::NONE;
    }
    else
    {
        std::fseek(open_file, last_file_pos, SEEK_SET);
        if (this_token->str[0] == '\n')
        {
            this_token->str[0] = ',';
        }
    }
}

/*frmgettoken fills token structure; returns 1 on success and 0 on
  NOT_A_TOKEN and END_OF_FORMULA
*/
static bool frm_get_token(std::FILE *open_file, Token *this_token)
{
    int i = 1;
    long file_pos;

    switch (int c = frm_get_char(open_file); c)
    {
CASE_NUM:
    case '.':
        this_token->str[0] = (char) c;
        return frm_get_constant(open_file, this_token);
CASE_ALPHA:
    case '_':
        this_token->str[0] = (char) c;
        return frm_get_alpha(open_file, this_token);
CASE_TERMINATOR:
        this_token->type = FormulaTokenType::OPERATOR; // this may be changed below
        this_token->str[0] = (char) c;
        file_pos = std::ftell(open_file);
        if (c == '<' || c == '>' || c == '=')
        {
            c = frm_get_char(open_file);
            if (c == '=')
            {
                this_token->str[i++] = (char) c;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
            }
        }
        else if (c == '!')
        {
            c = frm_get_char(open_file);
            if (c == '=')
            {
                this_token->str[i++] = (char) c;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                this_token->str[1] = (char) 0;
                this_token->type = FormulaTokenType::NOT_A_TOKEN;
                this_token->id = TokenId::ILLEGAL_OPERATOR;
                return false;
            }
        }
        else if (c == '|')
        {
            c = frm_get_char(open_file);
            if (c == '|')
            {
                this_token->str[i++] = (char) c;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
            }
        }
        else if (c == '&')
        {
            c = frm_get_char(open_file);
            if (c == '&')
            {
                this_token->str[i++] = (char) c;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                this_token->str[1] = (char) 0;
                this_token->type = FormulaTokenType::NOT_A_TOKEN;
                this_token->id = TokenId::ILLEGAL_OPERATOR;
                return false;
            }
        }
        else if (this_token->str[0] == '}')
        {
            this_token->type = FormulaTokenType::END_OF_FORMULA;
            this_token->id   = TokenId::NONE;
        }
        else if (this_token->str[0] == '\n'
            || this_token->str[0] == ','
            || this_token->str[0] == ':')
        {
            frm_get_eos(open_file, this_token);
        }
        else if (this_token->str[0] == ')')
        {
            this_token->type = FormulaTokenType::PARENS;
            this_token->id = TokenId::CLOSE_PARENS;
        }
        else if (this_token->str[0] == '(')
        {
            /* the following function will set token_type to PARENS and
               token_id to OPEN_PARENS if this is not the start of a
               complex constant */
            is_complex_constant(open_file, this_token);
            return true;
        }
        this_token->str[i] = (char) 0;
        if (this_token->type == FormulaTokenType::OPERATOR)
        {
            for (int j = 0; j < static_cast<int>(s_op_list.size()); j++)
            {
                if (std::strcmp(s_op_list[j], this_token->str) == 0)
                {
                    this_token->id = static_cast<TokenId>(j);
                    break;
                }
            }
        }
        return this_token->str[0] != '}';
    case EOF:
        this_token->str[0] = (char) 0;
        this_token->type = FormulaTokenType::NOT_A_TOKEN;
        this_token->id = TokenId::END_OF_FILE;
        return false;
    default:
        this_token->str[0] = (char) c;
        this_token->str[1] = (char) 0;
        this_token->type = FormulaTokenType::NOT_A_TOKEN;
        this_token->id = TokenId::ILLEGAL_CHARACTER;
        return false;
    }
}

int frm_get_param_stuff(const char *name)
{
    std::FILE *debug_token = nullptr;
    int c;
    Token current_token;
    std::FILE * entry_file = nullptr;
    g_frm_uses_p1 = false;
    g_frm_uses_p2 = false;
    g_frm_uses_p3 = false;
    g_frm_uses_ismand = false;
    g_max_function = 0;
    g_frm_uses_p4 = false;
    g_frm_uses_p5 = false;

    if (g_formula_name.empty())
    {
        return 0;  //  and don't reset the pointers
    }
    if (find_file_item(g_formula_filename, name, &entry_file, ItemType::FORMULA))
    {
        stop_msg(parse_error_text(ParseError::COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
        return 0;
    }
    while ((c = frm_get_char(entry_file)) != '{' && c != EOF)
    {
    }
    if (c != '{')
    {
        stop_msg(parse_error_text(ParseError::UNEXPECTED_EOF));
        std::fclose(entry_file);
        return 0;
    }

    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        const std::filesystem::path path{id::io::get_save_path(id::io::WriteFile::ROOT, "frmtokens.txt")};
        assert(!path.empty());
        debug_token = std::fopen(path.string().c_str(), "at");
        if (debug_token != nullptr)
        {
            fmt::print(debug_token, "{:s}\n", name);
        }
    }
    while (frm_get_token(entry_file, &current_token))
    {
        if (debug_token != nullptr)
        {
            fmt::print(debug_token,
                "{:s}\n"
                "token_type is {:d}\n"
                "token_id is {:d}\n",
                current_token.str,   //
                +current_token.type, //
                +current_token.id);
            if (current_token.type == FormulaTokenType::REAL_CONSTANT || current_token.type == FormulaTokenType::COMPLEX_CONSTANT)
            {
                fmt::print(debug_token,
                    "Real value is {:f}\n"
                    "Imag value is {:f}\n",
                    current_token.constant.x,
                    current_token.constant.y);
            }
            fmt::print(debug_token, "\n");
        }
        switch (current_token.type)
        {
        case FormulaTokenType::PARAM_VARIABLE:
            if (current_token.id == TokenId::VAR_P1)
            {
                g_frm_uses_p1 = true;
            }
            else if (current_token.id == TokenId::VAR_P2)
            {
                g_frm_uses_p2 = true;
            }
            else if (current_token.id == TokenId::VAR_P3)
            {
                g_frm_uses_p3 = true;
            }
            else if (current_token.id == TokenId::VAR_IS_MANDEL)
            {
                g_frm_uses_ismand = true;
            }
            else if (current_token.id == TokenId::VAR_P4)
            {
                g_frm_uses_p4 = true;
            }
            else if (current_token.id == TokenId::VAR_P5)
            {
                g_frm_uses_p5 = true;
            }
            break;
        case FormulaTokenType::PARAM_FUNCTION:
            if ((+current_token.id - 10) > g_max_function)
            {
                g_max_function = (char)(+current_token.id - 10);
            }
            break;
        default:
            break;
        }
    }
    std::fclose(entry_file);
    if (debug_token)
    {
        std::fclose(debug_token);
    }
    if (current_token.type != FormulaTokenType::END_OF_FORMULA)
    {
        g_frm_uses_p1 = false;
        g_frm_uses_p2 = false;
        g_frm_uses_p3 = false;
        g_frm_uses_ismand = false;
        g_max_function = 0;
        g_frm_uses_p4 = false;
        g_frm_uses_p5 = false;
        return 0;
    }
    return 1;
}

/* frm_check_name_and_sym():
     error checking to the open brace on the first line; return true
     on success, and false if errors are found which should cause the
     formula not to be executed
*/
static bool frm_check_name_and_sym(std::FILE * open_file, bool report_bad_sym)
{
    long file_pos = std::ftell(open_file);
    int c;
    bool at_end_of_name = false;

    // first, test name
    int i = 0;
    bool done = false;
    while (!done)
    {
        c = std::getc(open_file);
        switch (c)
        {
        case EOF:
            stop_msg(parse_error_text(ParseError::UNEXPECTED_EOF));
            return false;
        case '\r':
        case '\n':
            stop_msg(parse_error_text(ParseError::NO_LEFT_BRACKET_FIRST_LINE));
            return false;
        case ' ':
        case '\t':
            at_end_of_name = true;
            break;
        case '(':
        case '{':
            done = true;
            break;
        default :
            if (!at_end_of_name)
            {
                i++;
            }
            break;
        }
    }

    if (i > ITEM_NAME_LEN)
    {
        int j;
        int k = (int) std::strlen(parse_error_text(ParseError::FORMULA_NAME_TOO_LARGE));
        char msg_buff[100];
        std::strcpy(msg_buff, parse_error_text(ParseError::FORMULA_NAME_TOO_LARGE));
        std::strcat(msg_buff, ":\n   ");
        std::fseek(open_file, file_pos, SEEK_SET);
        for (j = 0; j < i && j < 25; j++)
        {
            msg_buff[j+k+2] = (char) std::getc(open_file);
        }
        msg_buff[j+k+2] = (char) 0;
        stop_msg(StopMsgFlags::FIXED_FONT, msg_buff);
        return false;
    }
    // get symmetry
    g_symmetry = SymmetryType::NONE;
    if (c == '(')
    {
        char sym_buf[20];
        done = false;
        i = 0;
        while (!done)
        {
            c = std::getc(open_file);
            switch (c)
            {
            case EOF:
                stop_msg(parse_error_text(ParseError::UNEXPECTED_EOF));
                return false;
            case '\r':
            case '\n':
                stop_msg(StopMsgFlags::FIXED_FONT, parse_error_text(ParseError::NO_LEFT_BRACKET_FIRST_LINE));
                return false;
            case '{':
                stop_msg(StopMsgFlags::FIXED_FONT, parse_error_text(ParseError::NO_MATCH_RIGHT_PAREN));
                return false;
            case ' ':
            case '\t':
                break;
            case ')':
                done = true;
                break;
            default :
                if (i < 19)
                {
                    sym_buf[i++] = (char) std::toupper(c);
                }
                break;
            }
        }
        sym_buf[i] = (char) 0;
        constexpr int num_names{SYMMETRY_NAMES.size()};
        for (i = 0; i < num_names; i++)
        {
            if (id::string_case_equal(SYMMETRY_NAMES[i].s, sym_buf))
            {
                g_symmetry = SYMMETRY_NAMES[i].n;
                break;
            }
        }
        if (i == num_names && report_bad_sym)
        {
            std::string msg_buff{parse_error_text(ParseError::INVALID_SYM_USING_NOSYM)};
            msg_buff += ":\n   ";
            msg_buff += sym_buf;
            stop_msg(StopMsgFlags::FIXED_FONT, msg_buff);
        }
    }
    if (c != '{')
    {
        done = false;
        while (!done)
        {
            c = std::getc(open_file);
            switch (c)
            {
            case EOF:
                stop_msg(StopMsgFlags::FIXED_FONT, parse_error_text(ParseError::UNEXPECTED_EOF));
                return false;
            case '\r':
            case '\n':
                stop_msg(StopMsgFlags::FIXED_FONT, parse_error_text(ParseError::NO_LEFT_BRACKET_FIRST_LINE));
                return false;
            case '{':
                done = true;
                break;
            default :
                break;
            }
        }
    }
    return true;
}

/* This function sets the
    symmetry and converts a formula into a string  with no spaces,
    and one comma after each expression except where the ':' is placed
    and except the final expression in the formula. The open file passed
    as an argument is open in "rb" mode and is positioned at the first
    letter of the name of the formula to be prepared. This function
    is called from run_formula() below.
*/
static std::string prepare_formula(std::FILE *file, bool report_bad_sym)
{
    const long file_pos{std::ftell(file)};

    // Test for a repeat
    if (!frm_check_name_and_sym(file, report_bad_sym))
    {
        std::fseek(file, file_pos, SEEK_SET);
        return {};
    }
    if (!frm_prescan(file))
    {
        std::fseek(file, file_pos, SEEK_SET);
        return {};
    }

    if (s_chars_in_formula > 8190)
    {
        std::fseek(file, file_pos, SEEK_SET);
        return {};
    }

    std::FILE *debug_fp{};
    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        const std::filesystem::path path{id::io::get_save_path(id::io::WriteFile::ROOT, "debugfrm.txt")};
        assert(!path.empty());
        debug_fp = std::fopen(path.string().c_str(), "at");
        if (debug_fp != nullptr)
        {
            fmt::print(debug_fp, "{:s}\n", g_formula_name);
            if (g_symmetry != SymmetryType::NONE)
            {
                const auto it = std::find_if(std::begin(SYMMETRY_NAMES), std::end(SYMMETRY_NAMES),
                    [](const SymmetryName &item) { return item.n == g_symmetry; });
                if (it != std::end(SYMMETRY_NAMES))
                {
                    fmt::print(debug_fp, "{:s}\n", it->s);
                }
            }
        }
    }

    std::string formula_text;

    bool done{};

    // skip opening end-of-lines
    Token temp_tok;
    while (!done)
    {
        frm_get_token(file, &temp_tok);
        if (temp_tok.type == FormulaTokenType::NOT_A_TOKEN)
        {
            stop_msg(StopMsgFlags::FIXED_FONT, "Unexpected token error in PrepareFormula\n");
            std::fseek(file, file_pos, SEEK_SET);
            if (debug_fp != nullptr)
            {
                std::fclose(debug_fp);
            }
            return {};
        }
        if (temp_tok.type == FormulaTokenType::END_OF_FORMULA)
        {
            stop_msg(StopMsgFlags::FIXED_FONT, "Formula has no executable instructions\n");
            std::fseek(file, file_pos, SEEK_SET);
            if (debug_fp != nullptr)
            {
                std::fclose(debug_fp);
            }
            return {};
        }
        if (temp_tok.str[0] != ',')
        {
            formula_text += temp_tok.str;
            done = true;
        }
    }

    done = false;
    while (!done)
    {
        frm_get_token(file, &temp_tok);
        switch (temp_tok.type)
        {
        case FormulaTokenType::NOT_A_TOKEN:
            stop_msg(StopMsgFlags::FIXED_FONT, "Unexpected token error in prepare_formula\n");
            std::fseek(file, file_pos, SEEK_SET);
            if (debug_fp != nullptr)
            {
                std::fclose(debug_fp);
            }
            return {};
        case FormulaTokenType::END_OF_FORMULA:
            done = true;
            std::fseek(file, file_pos, SEEK_SET);
            break;
        default:
            formula_text += temp_tok.str;
            break;
        }
    }

    if (debug_fp != nullptr)
    {
        if (!formula_text.empty())
        {
            fmt::print(debug_fp, "   {:s}\n", formula_text);
        }
        std::fclose(debug_fp);
    }

    return formula_text;
}

int bad_formula()
{
    //  this is called when a formula is bad, instead of calling
    //     the normal functions which will produce undefined results
    return 1;
}

//  returns true if an error occurred
bool run_formula(const std::string &name, bool report_bad_sym)
{
    //  first set the pointers so they point to a fn which always returns 1
    assert(g_cur_fractal_specific == get_fractal_specific(FractalType::FORMULA));
    g_cur_fractal_specific->per_pixel = bad_formula;
    g_cur_fractal_specific->orbit_calc = bad_formula;

    if (g_formula_name.empty())
    {
        return true;  //  and don't reset the pointers
    }

    std::FILE *entry_file{};
    if (find_file_item(g_formula_filename, name, &entry_file, ItemType::FORMULA))
    {
        stop_msg(parse_error_text(ParseError::COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
        return true;
    }

    s_formula = prepare_formula(entry_file, report_bad_sym);
    std::fclose(entry_file);

    if (!s_formula.empty())  //  No errors while making string
    {
        parser_allocate();  //  parse_formula_text() will test if this alloc worked
        if (parse_formula_text(s_formula))
        {
            return true;   //  parse failed, don't change fn pointers
        }
        if (s_uses_jump && fill_jump_struct())
        {
            stop_msg(parse_error_text(ParseError::ERROR_IN_PARSING_JUMP_STATEMENTS));
            return true;
        }

        // all parses succeeded so set the pointers back to good functions
        g_cur_fractal_specific->per_pixel = formula_per_pixel;
        g_cur_fractal_specific->orbit_calc = formula_orbit;
        return false;
    }
    return true; // error in making string
}

bool formula_per_image()
{
    return !run_formula(g_formula_name, false); // run_formula() returns true for failure
}

void init_misc()
{
    static Arg arg_first;
    static Arg arg_second;
    if (s_vars.empty())
    {
        s_vars.resize(5);
    }
    g_arg1 = &arg_first;
    g_arg2 = &arg_second; // needed by all the ?stk* functions
    g_frm_uses_p1 = false;
    g_frm_uses_p2 = false;
    g_frm_uses_p3 = false;
    s_uses_jump = false;
    g_frm_uses_ismand = false;
    g_frm_uses_p4 = false;
    g_frm_uses_p5 = false;
}

static void parser_allocate()
{
    free_work_area();
    g_max_function_ops = 2300;
    g_max_function_args = (unsigned)(g_max_function_ops/2.5);

    s_fns.reserve(g_max_function_ops);
    s_store.resize(MAX_STORES);
    s_load.resize(MAX_LOADS);
    s_vars.resize(g_max_function_args);

    if (!parse_formula_text(s_formula))
    {
        // per Chuck Ebbert, fudge these up a little
        g_max_function_ops = g_operation_index + 4;
        g_max_function_args = g_variable_index + 4;
    }
    g_frm_uses_p1 = false;
    g_frm_uses_p2 = false;
    g_frm_uses_p3 = false;
    g_frm_uses_p4 = false;
    g_frm_uses_p5 = false;
}

void free_work_area()
{
    s_store.clear();
    s_load.clear();
    s_vars.clear();
    s_fns.clear();
}

static void frm_error(std::FILE * open_file, long begin_frm)
{
    Token tok;
    int chars_to_error = 0;
    int chars_in_error = 0;
    char msg_buff[900];
    std::strcpy(msg_buff, "\n");

    for (int j = 0; j < 3 && s_errors[j].start_pos; j++)
    {
        const bool initialization_error = s_errors[j].error_number == ParseError::SECOND_COLON;
        std::fseek(open_file, begin_frm, SEEK_SET);
        int line_number = 1;
        while (std::ftell(open_file) != s_errors[j].error_pos)
        {
            int i = std::fgetc(open_file);
            if (i == '\n')
            {
                line_number++;
            }
            else if (i == EOF || i == '}')
            {
                stop_msg("Unexpected EOF or end-of-formula in error function.\n");
                std::fseek(open_file, s_errors[j].error_pos, SEEK_SET);
                frm_get_token(open_file, &tok); //reset file to end of error token
                return;
            }
        }
        std::strcat(msg_buff,
            fmt::format("Error({:d}) at line {:d}:  {:s}\n  ", //
                +s_errors[j].error_number, line_number, parse_error_text(s_errors[j].error_number))
                .c_str());
        int i = (int) std::strlen(msg_buff);
        std::fseek(open_file, s_errors[j].start_pos, SEEK_SET);
        int token_count = 0;
        int statement_len = token_count;
        bool done = false;
        while (!done)
        {
            long file_pos = std::ftell(open_file);
            if (file_pos == s_errors[j].error_pos)
            {
                chars_to_error = statement_len;
                frm_get_token(open_file, &tok);
                chars_in_error = (int) std::strlen(tok.str);
                statement_len += chars_in_error;
                token_count++;
            }
            else
            {
                frm_get_token(open_file, &tok);
                statement_len += (int) std::strlen(tok.str);
                token_count++;
            }
            if (tok.type == FormulaTokenType::END_OF_FORMULA ||
                (tok.type == FormulaTokenType::OPERATOR && (tok.id == TokenId::OP_COMMA || tok.id == TokenId::OP_COLON)) ||
                (tok.type == FormulaTokenType::NOT_A_TOKEN && tok.id == TokenId::END_OF_FILE))
            {
                done = true;
                if (token_count > 1 && !initialization_error)
                {
                    token_count--;
                }
            }
        }
        std::fseek(open_file, s_errors[j].start_pos, SEEK_SET);
        if (chars_in_error < 74)
        {
            while (chars_to_error + chars_in_error > 74)
            {
                frm_get_token(open_file, &tok);
                chars_to_error -= (int) std::strlen(tok.str);
                token_count--;
            }
        }
        else
        {
            std::fseek(open_file, s_errors[j].error_pos, SEEK_SET);
            chars_to_error = 0;
            token_count = 1;
        }
        while ((int) std::strlen(&msg_buff[i]) <=74 && token_count--)
        {
            frm_get_token(open_file, &tok);
            std::strcat(msg_buff, tok.str);
        }
        std::fseek(open_file, s_errors[j].error_pos, SEEK_SET);
        frm_get_token(open_file, &tok);
        if ((int) std::strlen(&msg_buff[i]) > 74)
        {
            msg_buff[i + 74] = (char) 0;
        }
        std::strcat(msg_buff, "\n");
        i = (int) std::strlen(msg_buff);
        while (chars_to_error-- > -2)
        {
            std::strcat(msg_buff, " ");
        }
        if (s_errors[j].error_number == ParseError::TOKEN_TOO_LONG)
        {
            chars_in_error = 33;
        }
        while (chars_in_error-- && (int) std::strlen(&msg_buff[i]) <=74)
        {
            std::strcat(msg_buff, "^");
        }
        std::strcat(msg_buff, "\n");
    }
    stop_msg(StopMsgFlags::FIXED_FONT, msg_buff);
}

/*frm_prescan() takes an open file with the file pointer positioned at
  the beginning of the relevant formula, and parses the formula, token
  by token, for syntax errors. The function also accumulates data for
  memory allocation to be done later.

  The function returns 1 if success, and 0 if errors are found.
*/
static bool frm_prescan(std::FILE * open_file)
{
    long file_pos{};
    bool done{};
    Token this_token{};
    int errors_found{};
    bool expecting_arg{true};
    bool new_statement{true};
    bool assignment_ok{true};
    bool already_got_colon{};
    unsigned long else_has_been_used{};
    unsigned long waiting_for_mod{};
    int waiting_for_endif{};
    constexpr int MAX_PARENS{sizeof(long) * 8};

    s_num_jumps = 0UL;
    s_num_stores = 0UL;
    s_num_loads = 0UL;
    s_num_ops = 0UL;
    s_chars_in_formula = 0U;
    s_uses_jump = false;
    s_paren = 0;

    long statement_pos{std::ftell(open_file)};
    long orig_pos{statement_pos};
    for (ErrorData &error : s_errors)
    {
        error.start_pos    = 0L;
        error.error_pos    = 0L;
        error.error_number = ParseError::NONE;
    }
    const auto record_error = [&](ParseError err)
    {
        if (errors_found == 0 || s_errors[errors_found - 1].start_pos != statement_pos)
        {
            s_errors[errors_found].start_pos = statement_pos;
            s_errors[errors_found].error_pos = file_pos;
            s_errors[errors_found++].error_number = err;
        }
    };

    while (!done)
    {
        file_pos = std::ftell(open_file);
        frm_get_token(open_file, &this_token);
        s_chars_in_formula += (int) std::strlen(this_token.str);
        switch (this_token.type)
        {
        case FormulaTokenType::NOT_A_TOKEN:
            assignment_ok = false;
            switch (this_token.id)
            {
            case TokenId::END_OF_FILE:
                stop_msg(parse_error_text(ParseError::UNEXPECTED_EOF));
                std::fseek(open_file, orig_pos, SEEK_SET);
                return false;
            case TokenId::ILLEGAL_CHARACTER:
                record_error(ParseError::ILLEGAL_CHAR);
                break;
            case TokenId::ILLEGAL_VARIABLE_NAME:
                record_error(ParseError::ILLEGAL_VAR_NAME);
                break;
            case TokenId::TOKEN_TOO_LONG:
                record_error(ParseError::TOKEN_TOO_LONG);
                break;
            case TokenId::FUNC_USED_AS_VAR:
                record_error(ParseError::FUNC_USED_AS_VAR);
                break;
            case TokenId::JUMP_MISSING_BOOLEAN:
                record_error(ParseError::JUMP_NEEDS_BOOLEAN);
                break;
            case TokenId::JUMP_WITH_ILLEGAL_CHAR:
                record_error(ParseError::NO_CHAR_AFTER_THIS_JUMP);
                break;
            case TokenId::UNDEFINED_FUNCTION:
                record_error(ParseError::UNDEFINED_FUNCTION);
                break;
            case TokenId::ILLEGAL_OPERATOR:
                record_error(ParseError::UNDEFINED_OPERATOR);
                break;
            case TokenId::ILL_FORMED_CONSTANT:
                record_error(ParseError::INVALID_CONST);
                break;
            default:
                stop_msg("Unexpected arrival at default case in prescan()");
                std::fseek(open_file, orig_pos, SEEK_SET);
                return false;
            }
            break;
        case FormulaTokenType::PARENS:
            assignment_ok = false;
            new_statement = false;
            switch (this_token.id)
            {
            case TokenId::OPEN_PARENS:
                if (++s_paren > MAX_PARENS)
                {
                    record_error(ParseError::NESTING_TO_DEEP);
                }
                else if (!expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_OPERATOR);
                }
                waiting_for_mod = waiting_for_mod << 1;
                break;
            case TokenId::CLOSE_PARENS:
                if (s_paren)
                {
                    s_paren--;
                }
                else
                {
                    record_error(ParseError::NEED_A_MATCHING_OPEN_PARENS);
                    s_paren = 0;
                }
                if (waiting_for_mod & 1L)
                {
                    record_error(ParseError::UNMATCHED_MODULUS);
                }
                else
                {
                    waiting_for_mod = waiting_for_mod >> 1;
                }
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                break;
            default:
                break;
            }
            break;
        case FormulaTokenType::PARAM_VARIABLE: //i.e. p1, p2, p3, p4 or p5
            s_num_ops++;
            s_num_loads++;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::USER_NAMED_VARIABLE: // i.e. c, iter, etc.
            s_num_ops++;
            s_num_loads++;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::PREDEFINED_VARIABLE: // i.e. z, pixel, whitesq, etc.
            s_num_ops++;
            s_num_loads++;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::REAL_CONSTANT: // i.e. 4, (4,0), etc.
            assignment_ok = false;
            s_num_ops++;
            s_num_loads++;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::COMPLEX_CONSTANT: // i.e. (1,2) etc.
            assignment_ok = false;
            s_num_ops++;
            s_num_loads++;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::FUNCTION:
            assignment_ok = false;
            new_statement = false;
            s_num_ops++;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            break;
        case FormulaTokenType::PARAM_FUNCTION:
            assignment_ok = false;
            s_num_ops++;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            new_statement = false;
            break;
        case FormulaTokenType::FLOW_CONTROL:
            assignment_ok = false;
            s_num_ops++;
            s_num_jumps++;
            if (!new_statement)
            {
                record_error(ParseError::JUMP_NOT_FIRST);
            }
            else
            {
                s_uses_jump = true;
                switch (this_token.id)
                {
                case TokenId::JUMP_IF:  // if
                    else_has_been_used = else_has_been_used << 1;
                    waiting_for_endif++;
                    break;
                case TokenId::JUMP_ELSE_IF: //ELSEIF
                    s_num_ops += 3; //else + two clear statements
                    s_num_jumps++;  // this involves two jumps
                    if (else_has_been_used % 2)
                    {
                        record_error(ParseError::ENDIF_REQUIRED_AFTER_ELSE);
                    }
                    else if (!waiting_for_endif)
                    {
                        record_error(ParseError::MISPLACED_ELSE_OR_ELSEIF);
                    }
                    break;
                case TokenId::JUMP_ELSE: //ELSE
                    if (else_has_been_used % 2)
                    {
                        record_error(ParseError::ENDIF_REQUIRED_AFTER_ELSE);
                    }
                    else if (!waiting_for_endif)
                    {
                        record_error(ParseError::MISPLACED_ELSE_OR_ELSEIF);
                    }
                    else_has_been_used = else_has_been_used | 1;
                    break;
                case TokenId::JUMP_END_IF: //ENDIF
                    else_has_been_used = else_has_been_used >> 1;
                    waiting_for_endif--;
                    if (waiting_for_endif < 0)
                    {
                        record_error(ParseError::ENDIF_WITH_NO_IF);
                        waiting_for_endif = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case FormulaTokenType::OPERATOR:
            s_num_ops++; //This will be corrected below in certain cases
            switch (this_token.id)
            {
            case TokenId::OP_COMMA:
            case TokenId::OP_COLON:    // end of statement and :
                s_num_ops++; // ParseStr inserts a dummy op
                if (s_paren)
                {
                    record_error(ParseError::NEED_MORE_CLOSE_PARENS);
                    s_paren = 0;
                }
                if (waiting_for_mod)
                {
                    record_error(ParseError::UNMATCHED_MODULUS);
                    waiting_for_mod = 0;
                }
                if (!expecting_arg)
                {
                    if (this_token.id == TokenId::OP_COLON)
                    {
                        s_num_ops += 2;
                    }
                    else
                    {
                        s_num_ops++;
                    }
                }
                else if (!new_statement)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                if (this_token.id == TokenId::OP_COLON && waiting_for_endif)
                {
                    record_error(ParseError::UNMATCHED_IF_IN_INIT_SECTION);
                    waiting_for_endif = 0;
                }
                if (this_token.id == TokenId::OP_COLON && already_got_colon)
                {
                    record_error(ParseError::SECOND_COLON);
                }
                if (this_token.id == TokenId::OP_COLON)
                {
                    already_got_colon = true;
                }
                new_statement = true;
                expecting_arg = true;
                assignment_ok = true;
                statement_pos = std::ftell(open_file);
                break;
            case TokenId::OP_NOT_EQUAL:     // !=
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_ASSIGN:     // =
                s_num_ops--; //this just converts a load to a store
                s_num_loads--;
                s_num_stores++;
                if (!assignment_ok)
                {
                    record_error(ParseError::ILLEGAL_ASSIGNMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_EQUAL:     // ==
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_LT:     // <
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_LE:     // <=
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_GT:     // >
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_GE:     // >=
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_MODULUS:     // | (half of the modulus operator)
                assignment_ok = false;
                if (!(waiting_for_mod & 1L))
                {
                    s_num_ops--;
                }
                if (!(waiting_for_mod & 1L) && !expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_OPERATOR);
                }
                else if ((waiting_for_mod & 1L) && expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                waiting_for_mod = waiting_for_mod ^ 1L; //switch right bit
                break;
            case TokenId::OP_OR:     // ||
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_AND:    // &&
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_PLUS:    // + case 11 (":") is up with case 0
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_MINUS:    // -
                assignment_ok = false;
                expecting_arg = true;
                break;
            case TokenId::OP_MULTIPLY:    // *
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_DIVIDE:    // /
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                expecting_arg = true;
                break;
            case TokenId::OP_POWER:    // ^
                assignment_ok = false;
                if (expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_ARGUMENT);
                }
                file_pos = std::ftell(open_file);
                frm_get_token(open_file, &this_token);
                if (this_token.str[0] == '-')
                {
                    record_error(ParseError::NO_NEG_AFTER_EXPONENT);
                }
                else
                {
                    std::fseek(open_file, file_pos, SEEK_SET);
                }
                expecting_arg = true;
                break;
            default:
                break;
            }
            break;
        case FormulaTokenType::END_OF_FORMULA:
            s_num_ops += 3; // Just need one, but a couple of extra just for the heck of it
            if (s_paren)
            {
                record_error(ParseError::NEED_MORE_CLOSE_PARENS);
                s_paren = 0;
            }
            if (waiting_for_mod)
            {
                record_error(ParseError::UNMATCHED_MODULUS);
                waiting_for_mod = 0;
            }
            if (waiting_for_endif)
            {
                record_error(ParseError::IF_WITH_NO_ENDIF);
                waiting_for_endif = 0;
            }
            if (expecting_arg && !new_statement)
            {
                record_error(ParseError::SHOULD_BE_ARGUMENT);
                statement_pos = std::ftell(open_file);
            }

            if (s_num_jumps >= MAX_JUMPS)
            {
                record_error(ParseError::TOO_MANY_JUMPS);
            }
            done = true;
            break;
        default:
            break;
        }
        if (errors_found == 3)
        {
            done = true;
        }
    }
    if (s_errors[0].start_pos)
    {
        frm_error(open_file, orig_pos);
        std::fseek(open_file, orig_pos, SEEK_SET);
        return false;
    }
    std::fseek(open_file, orig_pos, SEEK_SET);

    return true;
}
