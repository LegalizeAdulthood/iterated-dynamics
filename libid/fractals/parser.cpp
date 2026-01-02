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
#include "engine/convert_center_mag.h"
#include "engine/Inversion.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel_grid.h"
#include "fractals/formula.h"
#include "fractals/fractalp.h"
#include "fractals/interpreter.h"
#include "fractals/newton.h"
#include "io/file_item.h"
#include "io/library.h"
#include "math/arg.h"
#include "math/cmplx.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "math/rand15.h"
#include "misc/debug_flags.h"
#include "misc/id.h"
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
#include <functional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

using namespace id::config;
using namespace id::engine;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::fractals
{

namespace
{

enum
{
    MAX_OPS = 250,
    MAX_ARGS = 100,
    MAX_JUMPS = 200 // size of JUMP_CONTROL array
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

enum class GetFormulaError
{
    NONE = 0,
    UNEXPECTED_EOF,
    NAME_TOO_LONG,
    NO_LEFT_BRACKET_FIRST_LINE,
    NO_MATCH_RIGHT_PAREN,
    BAD_SYMMETRY,
};

struct FormulaEntry
{
    std::string name;
    SymmetryType symmetry;
    std::string body;
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
    FunctionPtr ptr;
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

struct ParserState
{
    int paren{};
    unsigned int n{};
    unsigned int next_op{};
    unsigned int init_n{};
    bool expecting_arg{};
    unsigned int chars_in_formula{};
};

} // namespace

// forward declarations
static bool frm_prescan(std::FILE *open_file);
static void parser_allocate();

CompiledFormula g_formula;
static ParserState s_parser;

static constexpr std::array<const char *, 4> JUMP_LIST
{
    "if",
    "elseif",
    "else",
    "endif"
};
static FormulaEntry s_formula_entry;
static std::array<ErrorData, 3> s_errors{};

static const std::array<FunctList, 34> FUNC_LIST
{
    FunctList{"sin",   d_stk_sin},
    FunctList{"sinh",  d_stk_sinh},
    FunctList{"cos",   d_stk_cos},
    FunctList{"cosh",  d_stk_cosh},
    FunctList{"sqr",   d_stk_sqr},
    FunctList{"log",   d_stk_log},
    FunctList{"exp",   d_stk_exp},
    FunctList{"abs",   d_stk_abs},
    FunctList{"conj",  d_stk_conj},
    FunctList{"real",  d_stk_real},
    FunctList{"imag",  d_stk_imag},
    FunctList{"fn1",   d_stk_fn1},
    FunctList{"fn2",   d_stk_fn2},
    FunctList{"fn3",   d_stk_fn3},
    FunctList{"fn4",   d_stk_fn4},
    FunctList{"flip",  d_stk_flip},
    FunctList{"tan",   d_stk_tan},
    FunctList{"tanh",  d_stk_tanh},
    FunctList{"cotan", d_stk_cotan},
    FunctList{"cotanh", d_stk_cotanh},
    FunctList{"cosxx", d_stk_cosxx},
    FunctList{"srand", d_stk_srand},
    FunctList{"asin",  d_stk_asin},
    FunctList{"asinh", d_stk_asinh},
    FunctList{"acos",  d_stk_acos},
    FunctList{"acosh", d_stk_acosh},
    FunctList{"atan",  d_stk_atan},
    FunctList{"atanh", d_stk_atanh},
    FunctList{"sqrt",  d_stk_sqrt},
    FunctList{"cabs",  d_stk_cabs},
    FunctList{"floor", d_stk_floor},
    FunctList{"ceil",  d_stk_ceil},
    FunctList{"trunc", d_stk_trunc},
    FunctList{"round", d_stk_round},
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

const std::array<const char *, 19> VARIABLES
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

static void push_jump(const JumpControlType type)
{
    JumpControl value{};
    value.type = type;
    g_formula.jump_control.push_back(value);
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

#define MAX_STORES ((g_formula.max_ops/4)*2)  // at most only half the ops can be stores
#define MAX_LOADS  ((unsigned)(g_formula.max_ops*.8))  // and 80% can be loads

static bool check_denom(const double denom)
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
    if (constexpr int last_err = std::size(MESSAGES) - 1; +which > last_err)
    {
        which = static_cast<ParseError>(last_err);
    }
    return MESSAGES[+which];
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

static ConstArg *is_const(const char *str, const int len)
{
    // next line enforces variable vs constant naming convention
    for (unsigned n = 0U; n < g_formula.var_index; n++)
    {
        if (g_formula.vars[n].len == len)
        {
            if (string_case_equal(g_formula.vars[n].s, str, len))
            {
                if (n == 1)          // The formula uses 'p1'.
                {
                    g_formula.uses_p1 = true;
                }
                if (n == 2)          // The formula uses 'p2'.
                {
                    g_formula.uses_p2 = true;
                }
                if (n == 7)          // The formula uses 'rand'.
                {
                    g_formula.uses_rand = true;
                }
                if (n == 8)          // The formula uses 'p3'.
                {
                    g_formula.uses_p3 = true;
                }
                if (n == 13)          // The formula uses 'ismand'.
                {
                    g_formula.uses_ismand = true;
                }
                if (n == 17)          // The formula uses 'p4'.
                {
                    g_formula.uses_p4 = true;
                }
                if (n == 18)          // The formula uses 'p5'.
                {
                    g_formula.uses_p5 = true;
                }
                if (!is_const_pair(str))
                {
                    return &g_formula.vars[n];
                }
            }
        }
    }
    g_formula.vars[g_formula.var_index].s = str;
    g_formula.vars[g_formula.var_index].len = len;
    g_formula.vars[g_formula.var_index].a.d.x = 0.0;
    g_formula.vars[g_formula.var_index].a.d.y = 0.0;

    if (std::isdigit(str[0])
        || (str[0] == '-' && (std::isdigit(str[1]) || str[1] == '.'))
        || str[0] == '.')
    {
        DComplex z;
        assert(g_formula.op_index > 0);
        assert(g_formula.op_index == g_formula.ops.size());
        if (g_formula.ops.back().f == d_stk_neg)
        {
            g_formula.ops.pop_back();
            g_formula.op_index--;
            str = str - 1;
            s_parser.init_n--;
            g_formula.vars[g_formula.var_index].len++;
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
                g_formula.vars[g_formula.var_index].len = j;
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
        g_formula.vars[g_formula.var_index].a.d = z;
        g_formula.vars[g_formula.var_index].s = str;
    }
    return &g_formula.vars[g_formula.var_index++];
}

static JumpControlType is_jump(const char *str, const int len)
{
    for (int i = 0; i < static_cast<int>(JUMP_LIST.size()); i++)
    {
        if (static_cast<int>(std::strlen(JUMP_LIST[i])) == len && string_case_equal(JUMP_LIST[i], str, len))
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
static int which_fn(const char *s, const int len)
{
    int out;
    if (len != 3)
    {
        out = 0;
    }
    else if (!string_case_equal(s, "fn", 2))
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

static FunctionPtr is_func(const char *str, const int len)
{
    unsigned n = skip_white_space(&str[len]);
    if (str[len+n] == '(')
    {
        for (n = 0; n < static_cast<unsigned>(FUNC_LIST.size()); n++)
        {
            if (string_case_equal(FUNC_LIST[n].s, str, len))
            {
                // count function variables
                if (const int funct_num = which_fn(str, len))
                {
                    g_formula.max_function = std::max(funct_num, g_formula.max_function);
                }
                return *FUNC_LIST[n].ptr;
            }
        }
        return funct_not_found;
    }
    return not_a_funct;
}

static void sort_precedence(int &op_index)
{
    const int this_op = s_parser.next_op++;
    while (g_formula.ops[this_op].p > g_formula.ops[s_parser.next_op].p && s_parser.next_op < g_formula.op_index)
    {
        sort_precedence(op_index);
    }
    if (op_index > static_cast<int>(g_formula.fns.size()))
    {
        throw std::runtime_error(
            "OpPtr (" + std::to_string(op_index) + ") exceeds size of f[] (" + std::to_string(g_formula.fns.size()) + ")");
    }
    g_formula.fns.push_back(g_formula.ops[this_op].f);
    ++op_index;
}

static void push_pending_op(const FunctionPtr f, const int p)
{
    g_formula.ops.push_back(PendingOp{f, p});
    ++g_formula.op_index;
    assert(g_formula.op_index == g_formula.ops.size());
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
    g_formula.uses_jump = false;
    g_formula.uses_rand = false;
    g_formula.jump_control.clear();

    g_formula.max_function = 0;
    for (g_formula.var_index = 0; g_formula.var_index < static_cast<unsigned>(VARIABLES.size()); g_formula.var_index++)
    {
        g_formula.vars[g_formula.var_index].s = VARIABLES[g_formula.var_index];
        g_formula.vars[g_formula.var_index].len = static_cast<int>(std::strlen(VARIABLES[g_formula.var_index]));
    }
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
    const double const_pi = std::atan(1.0) * 4;
    const double const_e = std::exp(1.0);
    g_formula.vars[7].a.d.y = 0.0;
    g_formula.vars[7].a.d.x = g_formula.vars[7].a.d.y;
    g_formula.vars[11].a.d.x = static_cast<double>(g_logical_screen.x_dots);
    g_formula.vars[11].a.d.y = static_cast<double>(g_logical_screen.y_dots);
    g_formula.vars[12].a.d.x = static_cast<double>(g_max_iterations);
    g_formula.vars[12].a.d.y = 0;
    g_formula.vars[13].a.d.x = g_frm_is_mandelbrot ? 1.0 : 0.0;
    g_formula.vars[13].a.d.y = 0;
    g_formula.vars[14].a.d.x = x_ctr;
    g_formula.vars[14].a.d.y = y_ctr;
    g_formula.vars[15].a.d.x = static_cast<double>(magnification);
    g_formula.vars[15].a.d.y = x_mag_factor;
    g_formula.vars[16].a.d.x = rotation;
    g_formula.vars[16].a.d.y = skew;

    g_formula.vars[1].a.d.x = g_params[0];
    g_formula.vars[1].a.d.y = g_params[1];
    g_formula.vars[2].a.d.x = g_params[2];
    g_formula.vars[2].a.d.y = g_params[3];
    g_formula.vars[5].a.d.x = const_pi;
    g_formula.vars[5].a.d.y = 0.0;
    g_formula.vars[6].a.d.x = const_e;
    g_formula.vars[6].a.d.y = 0.0;
    g_formula.vars[8].a.d.x = g_params[4];
    g_formula.vars[8].a.d.y = g_params[5];
    g_formula.vars[17].a.d.x = g_params[6];
    g_formula.vars[17].a.d.y = g_params[7];
    g_formula.vars[18].a.d.x = g_params[8];
    g_formula.vars[18].a.d.y = g_params[9];

    g_formula.op_index = 0;
    g_formula.ops.clear();
    g_formula.store_index = 0;
    g_formula.load_index = 0;
    s_parser.paren = 0;
    g_formula.last_init_op = 0;
    s_parser.expecting_arg = true;
    for (s_parser.n = 0; text[s_parser.n]; s_parser.n++)
    {
        if (!text[s_parser.n])
        {
            break;
        }
        s_parser.init_n = s_parser.n;
        switch (text[s_parser.n])
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;
        case '(':
            s_parser.paren++;
            break;
        case ')':
            s_parser.paren--;
            break;
        case '|':
            if (text[s_parser.n+1] == '|')
            {
                s_parser.expecting_arg = true;
                s_parser.n++;
                push_pending_op(d_stk_or, 7 - (s_parser.paren + equals) * 15);
            }
            else if (mod_flag == s_parser.paren-1)
            {
                s_parser.paren--;
                mod_flag = mods[--m_d_stk];
            }
            else
            {
                assert(m_d_stk < mods.size());
                mods[m_d_stk++] = mod_flag;
                push_pending_op(d_stk_mod, 2 - (s_parser.paren + equals) * 15);
                mod_flag = s_parser.paren++;
            }
            break;
        case ',':
        case ';':
            if (!s_parser.expecting_arg)
            {
                s_parser.expecting_arg = true;
                push_pending_op(nullptr, 15);
                push_pending_op(stk_clr, -30000);
                s_parser.paren = 0;
                equals = 0;
            }
            break;
        case ':':
            s_parser.expecting_arg = true;
            push_pending_op(nullptr, 15);
            push_pending_op(end_init, -30000);
            s_parser.paren = 0;
            equals = 0;
            g_formula.last_init_op = 10000;
            break;
        case '+':
            s_parser.expecting_arg = true;
            push_pending_op(d_stk_add, 4 - (s_parser.paren + equals)*15);
            break;
        case '-':
            if (s_parser.expecting_arg)
            {
                push_pending_op(d_stk_neg, 2 - (s_parser.paren + equals)*15);
            }
            else
            {
                push_pending_op(d_stk_sub, 4 - (s_parser.paren + equals)*15);
                s_parser.expecting_arg = true;
            }
            break;
        case '&':
            s_parser.expecting_arg = true;
            s_parser.n++;
            push_pending_op(d_stk_and, 7 - (s_parser.paren + equals)*15);
            break;
        case '!':
            s_parser.expecting_arg = true;
            s_parser.n++;
            push_pending_op(d_stk_ne, 6 - (s_parser.paren + equals)*15);
            break;
        case '<':
            s_parser.expecting_arg = true;
            {
                FunctionPtr fn;
                if (text[s_parser.n + 1] == '=')
                {
                    s_parser.n++;
                    fn = d_stk_lte;
                }
                else
                {
                    fn = d_stk_lt;
                }
                push_pending_op(fn, 6 - (s_parser.paren + equals) * 15);
            }
            break;
        case '>':
            s_parser.expecting_arg = true;
            {
                FunctionPtr fn;
                if (text[s_parser.n + 1] == '=')
                {
                    s_parser.n++;
                    fn = d_stk_gte;
                }
                else
                {
                    fn = d_stk_gt;
                }
                push_pending_op(fn, 6 - (s_parser.paren + equals) * 15);
            }
            break;
        case '*':
            s_parser.expecting_arg = true;
            push_pending_op(d_stk_mul, 3 - (s_parser.paren + equals)*15);
            break;
        case '/':
            s_parser.expecting_arg = true;
            push_pending_op(d_stk_div, 3 - (s_parser.paren + equals)*15);
            break;
        case '^':
            s_parser.expecting_arg = true;
            push_pending_op(d_stk_pwr, 2 - (s_parser.paren + equals)*15);
            break;
        case '=':
            s_parser.expecting_arg = true;
            if (text[s_parser.n+1] == '=')
            {
                s_parser.n++;
                push_pending_op(d_stk_eq, 6 - (s_parser.paren + equals)*15);
            }
            else
            {
                g_formula.ops[g_formula.op_index-1].f = stk_sto;
                g_formula.ops[g_formula.op_index-1].p = 5 - (s_parser.paren + equals)*15;
                g_formula.store[g_formula.store_index++] = g_formula.load[--g_formula.load_index];
                equals++;
            }
            break;
        default:
            while (std::isalnum(text[s_parser.n+1]) || text[s_parser.n+1] == '.' || text[s_parser.n+1] == '_')
            {
                s_parser.n++;
            }
            len = s_parser.n + 1 -s_parser.init_n;
            s_parser.expecting_arg = false;
            if (const JumpControlType type = is_jump(&text[s_parser.init_n], len); type != JumpControlType::NONE)
            {
                g_formula.uses_jump = true;
                switch (type)
                {
                case JumpControlType::IF:
                    s_parser.expecting_arg = true;
                    push_jump(JumpControlType::IF);
                    push_pending_op(d_stk_jump_on_false, 1);
                    break;
                case JumpControlType::ELSE_IF:
                    s_parser.expecting_arg = true;
                    push_jump(JumpControlType::ELSE_IF);
                    push_jump(JumpControlType::ELSE_IF);
                    push_pending_op(stk_jump, 1);
                    push_pending_op(nullptr, 15);
                    push_pending_op(stk_clr, -30000);
                    push_pending_op(d_stk_jump_on_false, 1);
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
                if (const FunctionPtr fn = is_func(&text[s_parser.init_n], len); fn != not_a_funct)
                {
                    push_pending_op(fn,  1 - (s_parser.paren + equals)*15);
                    s_parser.expecting_arg = true;
                }
                else
                {
                    ConstArg *c = is_const(&text[s_parser.init_n], len);
                    g_formula.load[g_formula.load_index++] = &c->a;
                    push_pending_op(stk_lod, 1 - (s_parser.paren + equals)*15);
                    s_parser.n = s_parser.init_n + c->len - 1;
                }
            }
            break;
        }
    }
    push_pending_op(nullptr, 16);
    s_parser.next_op = 0;
    g_formula.op_count = g_formula.op_index;
    int op_index{};
    while (s_parser.next_op < g_formula.op_index)
    {
        if (g_formula.ops[s_parser.next_op].f)
        {
            sort_precedence(op_index);
        }
        else
        {
            s_parser.next_op++;
            g_formula.op_count--;
        }
    }
    return false;
}

static int fill_if_group(const int endif_index, JumpPtrs *jump_data)
{
    int i   = endif_index;
    int ljp = endif_index; // ljp means "last jump processed"
    while (i > 0)
    {
        i--;
        switch (g_formula.jump_control[i].type)
        {
        case JumpControlType::IF:    //if (); this concludes processing of this group
            g_formula.jump_control[i].ptrs = jump_data[ljp];
            g_formula.jump_control[i].dest_jump_index = ljp + 1;
            return i;
        case JumpControlType::ELSE_IF:    //elseif* (2 jumps, the 'else' and the 'if')
            // first, the "if" part
            g_formula.jump_control[i].ptrs = jump_data[ljp];
            g_formula.jump_control[i].dest_jump_index = ljp + 1;

            // then, the else part
            i--; //fall through to "else" is intentional
        case JumpControlType::ELSE:
            g_formula.jump_control[i].ptrs = jump_data[endif_index];
            g_formula.jump_control[i].dest_jump_index = endif_index + 1;
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

    for (int op = 0; op < static_cast<int>(g_formula.op_count); op++)
    {
        if (find_new_func)
        {
            if (i < static_cast<int>(g_formula.jump_control.size()))
            {
                switch (g_formula.jump_control[i].type)
                {
                case JumpControlType::IF:
                    jump_func = d_stk_jump_on_false;
                    break;
                case JumpControlType::ELSE_IF:
                    check_for_else = !check_for_else;
                    if (check_for_else)
                    {
                        jump_func = stk_jump;
                    }
                    else
                    {
                        jump_func = d_stk_jump_on_false;
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
        if (*g_formula.fns[op] == stk_lod)
        {
            load_count++;
        }
        else if (*g_formula.fns[op] == stk_sto)
        {
            store_count++;
        }
        else if (*g_formula.fns[op] == jump_func)
        {
            JumpPtrs value{};
            value.jump_op_ptr = op;
            value.jump_lod_ptr = load_count;
            value.jump_sto_ptr = store_count;
            jump_data.push_back(value);
            i++;
            find_new_func = true;
        }
    }

    // Following for safety only; all should always be false
    if (i != static_cast<int>(g_formula.jump_control.size())             //
        || g_formula.jump_control[i - 1].type != JumpControlType::END_IF //
        || g_formula.jump_control[0].type != JumpControlType::IF)
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

static int frm_get_char(const std::function<int()> &get_char)
{
    int c;
    bool done = false;
    bool line_wrap = false;
    while (!done)
    {
        c = get_char();
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
            while ((c = get_char()) != '\n' && c != EOF)
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

static int frm_get_char(std::FILE *open_file)
{
    return frm_get_char([open_file] { return std::getc(open_file); });
}

// This function also gets flow control info

static void get_func_info(Token *tok)
{
    for (int i = 0; i < static_cast<int>(FUNC_LIST.size()); i++)
    {
        if (std::strcmp(FUNC_LIST[i].s, tok->str) == 0)
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
        if (std::strcmp(VARIABLES[i], tok->str) == 0)
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

/// @brief Fills in token structure where numeric constant is indicated.
///
/// This function parses numeric constants from the input file stream, including
/// support for scientific notation (e.g., 1.23e-4). It is called twice to fill
/// in the components of a complex constant (see is_complex_constant()).
///
/// @param open_file The open file stream from which to read the constant.
/// @param tok The Token structure to fill with the parsed constant information.
/// @return true on success, false if the token is NOT_A_TOKEN.
///
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
            tok->str[i] = static_cast<char>(0);
            tok->type = FormulaTokenType::NOT_A_TOKEN;
            tok->id   = TokenId::END_OF_FILE;
            return false;
        CASE_NUM:
            tok->str[i++] = static_cast<char>(c);
            file_pos = std::ftell(open_file);
            break;
        case '.':
            if (got_decimal_already || !getting_base)
            {
                tok->str[i++] = static_cast<char>(c);
                tok->str[i++] = static_cast<char>(0);
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            tok->str[i++] = static_cast<char>(c);
            got_decimal_already = true;
            file_pos = std::ftell(open_file);
            break;
        default :
            if (c == 'e' && getting_base && (std::isdigit(tok->str[i-1]) || (tok->str[i-1] == '.' && i > 1)))
            {
                tok->str[i++] = static_cast<char>(c);
                getting_base = false;
                got_decimal_already = false;
                file_pos = std::ftell(open_file);
                c = frm_get_char(open_file);
                if (c == '-' || c == '+')
                {
                    tok->str[i++] = static_cast<char>(c);
                    file_pos = std::ftell(open_file);
                }
                else
                {
                    std::fseek(open_file, file_pos, SEEK_SET);
                }
            }
            else if (std::isalpha(c) || c == '_')
            {
                tok->str[i++] = static_cast<char>(c);
                tok->str[i++] = static_cast<char>(0);
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            else if (tok->str[i-1] == 'e' || (tok->str[i-1] == '.' && i == 1))
            {
                tok->str[i++] = static_cast<char>(c);
                tok->str[i++] = static_cast<char>(0);
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id = TokenId::ILL_FORMED_CONSTANT;
                return false;
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                tok->str[i++] = static_cast<char>(0);
                done = true;
            }
            break;
        }
        if (i == 33 && tok->str[32])
        {
            tok->str[33] = static_cast<char>(0);
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
    tok->str[1] = static_cast<char>(0);  // so we can concatenate later

    const long file_pos = std::ftell(open_file);
    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        const std::filesystem::path path{get_save_path(WriteFile::ROOT, "frmconst.txt")};
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
            temp_tok.str[0] = static_cast<char>(c);
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
                temp_tok.str[0] = static_cast<char>(c);
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
    tok->str[1] = static_cast<char>(0);
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
        const long file_pos = std::ftell(open_file);
        switch (c)
        {
CASE_ALPHA:
CASE_NUM:
        case '_':
            if (i < 79)
            {
                tok->str[i++] = static_cast<char>(c);
            }
            else
            {
                tok->str[i] = static_cast<char>(0);
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
                tok->str[i] = static_cast<char>(0);
                return false;
            }
            if (var_name_too_long)
            {
                tok->type = FormulaTokenType::NOT_A_TOKEN;
                tok->id   = TokenId::TOKEN_TOO_LONG;
                tok->str[i] = static_cast<char>(0);
                std::fseek(open_file, last_file_pos, SEEK_SET);
                return false;
            }
            tok->str[i] = static_cast<char>(0);
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
    tok->str[0] = static_cast<char>(0);
    tok->type = FormulaTokenType::NOT_A_TOKEN;
    tok->id   = TokenId::END_OF_FILE;
    return false;
}

static void frm_get_eos(std::FILE *open_file, Token *this_token)
{
    long last_file_pos = std::ftell(open_file);
    int c;

    for (c = frm_get_char(open_file); c == '\n' || c == ',' || c == ':'; c = frm_get_char(open_file))
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

/// @brief Fills the token structure with the next token from the input file.
///
/// This function reads the next token from the provided file stream and populates
/// the given Token structure with the token's details. It handles various token
/// types including operators, constants, variables, functions, and flow control
/// keywords.
///
/// @param open_file The open file stream from which to read the token.
/// @param this_token The Token structure to fill with token information.
/// @return true if a valid token was successfully read and parsed, false if the
///         token is NOT_A_TOKEN or END_OF_FORMULA.
///
static bool frm_get_token(std::FILE *open_file, Token *this_token)
{
    int i = 1;
    long file_pos;

    switch (int c = frm_get_char(open_file); c)
    {
CASE_NUM:
    case '.':
        this_token->str[0] = static_cast<char>(c);
        return frm_get_constant(open_file, this_token);
CASE_ALPHA:
    case '_':
        this_token->str[0] = static_cast<char>(c);
        return frm_get_alpha(open_file, this_token);
CASE_TERMINATOR:
        this_token->type = FormulaTokenType::OPERATOR; // this may be changed below
        this_token->str[0] = static_cast<char>(c);
        file_pos = std::ftell(open_file);
        if (c == '<' || c == '>' || c == '=')
        {
            c = frm_get_char(open_file);
            if (c == '=')
            {
                this_token->str[i++] = static_cast<char>(c);
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
                this_token->str[i++] = static_cast<char>(c);
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                this_token->str[1] = static_cast<char>(0);
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
                this_token->str[i++] = static_cast<char>(c);
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
                this_token->str[i++] = static_cast<char>(c);
            }
            else
            {
                std::fseek(open_file, file_pos, SEEK_SET);
                this_token->str[1] = static_cast<char>(0);
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
        this_token->str[i] = static_cast<char>(0);
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
        this_token->str[0] = static_cast<char>(0);
        this_token->type = FormulaTokenType::NOT_A_TOKEN;
        this_token->id = TokenId::END_OF_FILE;
        return false;
    default:
        this_token->str[0] = static_cast<char>(c);
        this_token->str[1] = static_cast<char>(0);
        this_token->type = FormulaTokenType::NOT_A_TOKEN;
        this_token->id = TokenId::ILLEGAL_CHARACTER;
        return false;
    }
}

int frm_get_param_stuff(std::filesystem::path &path, const char *name)
{
    std::FILE *debug_token = nullptr;
    int c;
    Token current_token;
    std::FILE * entry_file = nullptr;
    g_formula.uses_ismand = false;
    g_formula.uses_p1 = false;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = false;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = false;
    g_formula.max_function = 0;

    if (g_formula_name.empty())
    {
        return 0;  //  and don't reset the pointers
    }
    if (find_file_item(path, name, &entry_file, ItemType::FORMULA))
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
        const std::filesystem::path path{get_save_path(WriteFile::ROOT, "frmtokens.txt")};
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
                g_formula.uses_p1 = true;
            }
            else if (current_token.id == TokenId::VAR_P2)
            {
                g_formula.uses_p2 = true;
            }
            else if (current_token.id == TokenId::VAR_P3)
            {
                g_formula.uses_p3 = true;
            }
            else if (current_token.id == TokenId::VAR_P4)
            {
                g_formula.uses_p4 = true;
            }
            else if (current_token.id == TokenId::VAR_P5)
            {
                g_formula.uses_p5 = true;
            }
            else if (current_token.id == TokenId::VAR_IS_MANDEL)
            {
                g_formula.uses_ismand = true;
            }
            break;
        case FormulaTokenType::PARAM_FUNCTION:
            if (+current_token.id - 10 > g_formula.max_function)
            {
                g_formula.max_function = static_cast<char>(+current_token.id - 10);
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
        g_formula.uses_ismand = false;
        g_formula.uses_p1 = false;
        g_formula.uses_p2 = false;
        g_formula.uses_p3 = false;
        g_formula.uses_p4 = false;
        g_formula.uses_p5 = false;
        g_formula.max_function = 0;
        return 0;
    }
    return 1;
}

static std::string get_formula_name(std::FILE *open_file, int &c, GetFormulaError &err)
{
    std::string result;
    result.reserve(ITEM_NAME_LEN);

    int i{};
    bool done{};
    bool at_end_of_name{};
    while (!done)
    {
        c = std::getc(open_file);
        switch (c)
        {
        case EOF:
            err = GetFormulaError::UNEXPECTED_EOF;
            return {};
        case '\r':
        case '\n':
            err = GetFormulaError::NO_LEFT_BRACKET_FIRST_LINE;
            return {};
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
                result.append(1, static_cast<char>(c));
            }
            break;
        }
    }

    if (i > ITEM_NAME_LEN)
    {
        err = GetFormulaError::NAME_TOO_LONG;
        return {};
    }

    return result;
}

static SymmetryType get_formula_symmetry(std::FILE *open_file, int &c, GetFormulaError &err)
{
    SymmetryType symmetry = SymmetryType::NONE;
    if (c == '(')
    {
        char sym_buf[20];
        bool done = false;
        int i = 0;
        while (!done)
        {
            c = std::getc(open_file);
            switch (c)
            {
            case EOF:
                err = GetFormulaError::UNEXPECTED_EOF;
                return {};
            case '\r':
            case '\n':
                err = GetFormulaError::NO_LEFT_BRACKET_FIRST_LINE;
                return {};
            case '{':
                err = GetFormulaError::NO_MATCH_RIGHT_PAREN;
                return {};
            case ' ':
            case '\t':
                break;
            case ')':
                done = true;
                break;
            default :
                if (i < 19)
                {
                    sym_buf[i++] = static_cast<char>(std::toupper(c));
                }
                break;
            }
        }
        sym_buf[i] = static_cast<char>(0);
        const auto it = std::find_if(SYMMETRY_NAMES.begin(),
            SYMMETRY_NAMES.end(),
            [&sym_buf](const SymmetryName &sym_name) { return string_case_equal(sym_name.s, sym_buf); });
        if (it == SYMMETRY_NAMES.end())
        {
            err = GetFormulaError::BAD_SYMMETRY;
            return {};
        }
        symmetry = it->n;
    }
    return symmetry;
}

static std::string get_formula_body(std::FILE *open_file, int &c, GetFormulaError &err)
{
    std::string result;
    result.reserve(1024);
    while (c != '{')
    {
        bool done = false;
        while (!done)
        {
            c = std::getc(open_file);
            switch (c)
            {
            case EOF:
                err = GetFormulaError::UNEXPECTED_EOF;
                return {};
            case '\r':
            case '\n':
                err = GetFormulaError::NO_LEFT_BRACKET_FIRST_LINE;
                return {};
            case '{':
                done = true;
                break;
            default :
                break;
            }
        }
    }
    while (c != '}')
    {
        c = std::getc(open_file);
        switch (c)
        {
        case EOF:
            err = GetFormulaError::UNEXPECTED_EOF;
            return {};
        case '}':
            break;
        default :
            result.append(1, static_cast<char>(c));
            break;
        }
    }
    return result;
}

class FilePositionSaver
{
public:
    FilePositionSaver(std::FILE *file) :
        m_file(file),
        m_position(std::ftell(file))
    {
    }

    ~FilePositionSaver()
    {
        std::fseek(m_file, m_position, SEEK_SET);
    }

    void cancel()
    {
        m_position = std::ftell(m_file);
    }

private:
    std::FILE *m_file;
    long m_position;
};

static FormulaEntry get_formula_entry(std::FILE *open_file, GetFormulaError &err)
{
    FilePositionSaver saved_pos{open_file};
    FormulaEntry result{};

    int c;
    result.name = get_formula_name(open_file, c, err);
    if (err != GetFormulaError::NONE)
    {
        return {};
    }

    result.symmetry = c == '(' ? get_formula_symmetry(open_file, c, err) : SymmetryType::NONE;
    if (err != GetFormulaError::NONE)
    {
        return {};
    }

    result.body = get_formula_body(open_file, c, err);
    if (err != GetFormulaError::NONE)
    {
        return {};
    }

    saved_pos.cancel();
    return result;
}

static std::string to_string(GetFormulaError err)
{
    switch (err)
    {
    case GetFormulaError::NONE:
        break;
    case GetFormulaError::UNEXPECTED_EOF:
        return parse_error_text(ParseError::UNEXPECTED_EOF);
    case GetFormulaError::NAME_TOO_LONG:
        return parse_error_text(ParseError::FORMULA_NAME_TOO_LARGE);
    case GetFormulaError::NO_LEFT_BRACKET_FIRST_LINE:
        return parse_error_text(ParseError::NO_LEFT_BRACKET_FIRST_LINE);
    case GetFormulaError::NO_MATCH_RIGHT_PAREN:
        return parse_error_text(ParseError::NO_MATCH_RIGHT_PAREN);
    case GetFormulaError::BAD_SYMMETRY:
        return parse_error_text(ParseError::INVALID_SYM_USING_NOSYM);
    }
    throw std::runtime_error("Unknown formula error code: " + std::to_string(static_cast<int>(err)));
}

/// @brief Performs error checking on the formula name and symmetry up to the open brace on the first line.
/// @param open_file Pointer to the open file containing the formula.
/// @param report_bad_sym Boolean flag indicating whether to report invalid symmetry.
/// @return true on success, false if errors are found that should prevent formula execution.
///
static bool frm_check_name_and_sym(std::FILE * open_file, const bool report_bad_sym)
{
    const long file_pos = std::ftell(open_file);
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
        const int k = static_cast<int>(std::strlen(parse_error_text(ParseError::FORMULA_NAME_TOO_LARGE)));
        char msg_buff[100];
        std::strcpy(msg_buff, parse_error_text(ParseError::FORMULA_NAME_TOO_LARGE));
        std::strcat(msg_buff, ":\n   ");
        std::fseek(open_file, file_pos, SEEK_SET);
        for (j = 0; j < i && j < 25; j++)
        {
            msg_buff[j+k+2] = static_cast<char>(std::getc(open_file));
        }
        msg_buff[j+k+2] = static_cast<char>(0);
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
                    sym_buf[i++] = static_cast<char>(std::toupper(c));
                }
                break;
            }
        }
        sym_buf[i] = static_cast<char>(0);
        constexpr int num_names{SYMMETRY_NAMES.size()};
        for (i = 0; i < num_names; i++)
        {
            if (string_case_equal(SYMMETRY_NAMES[i].s, sym_buf))
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

/// @brief Sets the symmetry and converts a formula into a string with no spaces.
///
/// This function converts a formula into a string with one comma after each expression
/// except where the ':' is placed and except the final expression in the formula. The
/// open file passed as an argument is open in "rb" mode and is positioned at the first
/// letter of the name of the formula to be prepared.
///
/// @param file The open file containing the formula.
/// @param report_bad_sym Boolean flag indicating whether to report invalid symmetry.
/// @return A string containing the prepared formula, or an empty string on error.
///
static std::string prepare_formula(std::FILE *file, const bool report_bad_sym)
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

    if (s_parser.chars_in_formula > 8190)
    {
        std::fseek(file, file_pos, SEEK_SET);
        return {};
    }

    std::FILE *debug_fp{};
    if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
    {
        const std::filesystem::path path{get_save_path(WriteFile::ROOT, "debugfrm.txt")};
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

//  returns true if an error occurred
bool parse_formula(std::filesystem::path &path, const std::string &name, const bool report_bad_sym)
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
    if (find_file_item(path, name, &entry_file, ItemType::FORMULA))
    {
        stop_msg(parse_error_text(ParseError::COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
        return true;
    }

    {
        FilePositionSaver saved_pos{entry_file};
        GetFormulaError err{};
        s_formula_entry = get_formula_entry(entry_file, err);
        if (err != GetFormulaError::NONE)
        {
            stop_msg(to_string(err));
            return true;
        }
    }
    g_formula.formula = prepare_formula(entry_file, report_bad_sym);
    std::fclose(entry_file);

    if (!g_formula.formula.empty())  //  No errors while making string
    {
        parser_allocate();  //  parse_formula_text() will test if this alloc worked
        if (parse_formula_text(g_formula.formula))
        {
            return true;   //  parse failed, don't change fn pointers
        }
        if (g_formula.uses_jump && fill_jump_struct())
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

void init_misc()
{
    static Arg arg_first;
    static Arg arg_second;
    if (g_formula.vars.empty())
    {
        g_formula.vars.resize(5);
    }
    g_arg1 = &arg_first;
    g_arg2 = &arg_second; // needed by all the ?stk* functions
    g_formula.uses_p1 = false;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = false;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = false;
    g_formula.uses_jump = false;
    g_formula.uses_rand = false;
    g_formula.uses_ismand = false;
}

static void parser_allocate()
{
    free_work_area();
    g_formula.max_ops = 2300;
    g_formula.max_args = static_cast<unsigned>(g_formula.max_ops / 2.5);

    g_formula.fns.reserve(g_formula.max_ops);
    g_formula.store.resize(MAX_STORES);
    g_formula.load.resize(MAX_LOADS);
    g_formula.vars.resize(g_formula.max_args);

    if (!parse_formula_text(g_formula.formula))
    {
        // per Chuck Ebbert, fudge these up a little
        g_formula.max_ops = g_formula.op_index + 4;
        g_formula.max_args = g_formula.var_index + 4;
    }
    g_formula.uses_p1 = false;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = false;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = false;
}

void free_work_area()
{
    g_formula.store.clear();
    g_formula.load.clear();
    g_formula.vars.clear();
    g_formula.fns.clear();
}

static void frm_error(std::FILE * open_file, const long begin_frm)
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
        int i = static_cast<int>(std::strlen(msg_buff));
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
                chars_in_error = static_cast<int>(std::strlen(tok.str));
                statement_len += chars_in_error;
                token_count++;
            }
            else
            {
                frm_get_token(open_file, &tok);
                statement_len += static_cast<int>(std::strlen(tok.str));
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
                chars_to_error -= static_cast<int>(std::strlen(tok.str));
                token_count--;
            }
        }
        else
        {
            std::fseek(open_file, s_errors[j].error_pos, SEEK_SET);
            chars_to_error = 0;
            token_count = 1;
        }
        while (static_cast<int>(std::strlen(&msg_buff[i])) <=74 && token_count--)
        {
            frm_get_token(open_file, &tok);
            std::strcat(msg_buff, tok.str);
        }
        std::fseek(open_file, s_errors[j].error_pos, SEEK_SET);
        frm_get_token(open_file, &tok);
        if (static_cast<int>(std::strlen(&msg_buff[i])) > 74)
        {
            msg_buff[i + 74] = static_cast<char>(0);
        }
        std::strcat(msg_buff, "\n");
        i = static_cast<int>(std::strlen(msg_buff));
        while (chars_to_error-- > -2)
        {
            std::strcat(msg_buff, " ");
        }
        if (s_errors[j].error_number == ParseError::TOKEN_TOO_LONG)
        {
            chars_in_error = 33;
        }
        while (chars_in_error-- && static_cast<int>(std::strlen(&msg_buff[i])) <=74)
        {
            std::strcat(msg_buff, "^");
        }
        std::strcat(msg_buff, "\n");
    }
    stop_msg(StopMsgFlags::FIXED_FONT, msg_buff);
}

/// @brief Parses the formula from an open file for syntax errors and accumulates data for memory allocation.
///
/// This function takes an open file with the file pointer positioned at the beginning of the relevant formula,
/// and parses the formula, token by token, for syntax errors. It also accumulates data for memory allocation
/// to be done later.
///
/// @param open_file Pointer to the open file containing the formula.
/// @return true if parsing succeeds (no errors found), false if errors are found.
///
static bool frm_prescan(std::FILE *open_file)
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

    unsigned long num_jumps{};
    s_parser.chars_in_formula = 0U;
    g_formula.uses_jump = false;
    s_parser.paren = 0;

    long statement_pos{std::ftell(open_file)};
    const long orig_pos{statement_pos};
    for (ErrorData &error : s_errors)
    {
        error.start_pos    = 0L;
        error.error_pos    = 0L;
        error.error_number = ParseError::NONE;
    }
    const auto record_error = [&](const ParseError err)
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
        s_parser.chars_in_formula += static_cast<int>(std::strlen(this_token.str));
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
                if (++s_parser.paren > MAX_PARENS)
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
                if (s_parser.paren)
                {
                    s_parser.paren--;
                }
                else
                {
                    record_error(ParseError::NEED_A_MATCHING_OPEN_PARENS);
                    s_parser.paren = 0;
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
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::USER_NAMED_VARIABLE: // i.e. c, iter, etc.
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::PREDEFINED_VARIABLE: // i.e. z, pixel, whitesq, etc.
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::REAL_CONSTANT: // i.e. 4, (4,0), etc.
            assignment_ok = false;
            new_statement = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            expecting_arg = false;
            break;
        case FormulaTokenType::COMPLEX_CONSTANT: // i.e. (1,2) etc.
            assignment_ok = false;
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
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            break;
        case FormulaTokenType::PARAM_FUNCTION:
            assignment_ok = false;
            if (!expecting_arg)
            {
                record_error(ParseError::SHOULD_BE_OPERATOR);
            }
            new_statement = false;
            break;
        case FormulaTokenType::FLOW_CONTROL:
            assignment_ok = false;
            num_jumps++;
            if (!new_statement)
            {
                record_error(ParseError::JUMP_NOT_FIRST);
            }
            else
            {
                g_formula.uses_jump = true;
                switch (this_token.id)
                {
                case TokenId::JUMP_IF:  // if
                    else_has_been_used = else_has_been_used << 1;
                    waiting_for_endif++;
                    break;
                case TokenId::JUMP_ELSE_IF: //ELSEIF
                    num_jumps++;  // this involves two jumps
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
            switch (this_token.id)
            {
            case TokenId::OP_COMMA:
            case TokenId::OP_COLON:    // end of statement and :
                if (s_parser.paren)
                {
                    record_error(ParseError::NEED_MORE_CLOSE_PARENS);
                    s_parser.paren = 0;
                }
                if (waiting_for_mod)
                {
                    record_error(ParseError::UNMATCHED_MODULUS);
                    waiting_for_mod = 0;
                }
                if (!expecting_arg)
                {
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
                }
                if (!(waiting_for_mod & 1L) && !expecting_arg)
                {
                    record_error(ParseError::SHOULD_BE_OPERATOR);
                }
                else if (waiting_for_mod & 1L && expecting_arg)
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
            if (s_parser.paren)
            {
                record_error(ParseError::NEED_MORE_CLOSE_PARENS);
                s_parser.paren = 0;
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

            if (num_jumps >= MAX_JUMPS)
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

struct OperationName
{
    FunctionPtr fn;
    std::string_view name;
};

static const std::array<OperationName, 67> s_op_names{
    OperationName{d_stk_add, "ADD"},                     //
    OperationName{d_stk_sub, "SUB"},                     //
    OperationName{d_stk_mul, "MUL"},                     //
    OperationName{d_stk_div, "DIV"},                     //
    OperationName{d_stk_sqr, "SQR"},                     //
    OperationName{d_stk_sin, "SIN"},                     //
    OperationName{d_stk_cos, "COS"},                     //
    OperationName{d_stk_cosh, "COSH"},                   //
    OperationName{d_stk_sinh, "SINH"},                   //
    OperationName{d_stk_tan, "TAN"},                     //
    OperationName{d_stk_tanh, "TANH"},                   //
    OperationName{d_stk_cotan, "COTAN"},                 //
    OperationName{d_stk_cotanh, "COTANH"},               //
    OperationName{d_stk_cosxx, "COSXX"},                 //
    OperationName{d_stk_log, "LOG"},                     //
    OperationName{d_stk_exp, "EXP"},                     //
    OperationName{d_stk_sqrt, "SQRT"},                   //
    OperationName{d_stk_abs, "ABS"},                     //
    OperationName{d_stk_conj, "CONJ"},                   //
    OperationName{d_stk_real, "REAL"},                   //
    OperationName{d_stk_imag, "IMAG"},                   //
    OperationName{d_stk_flip, "FLIP"},                   //
    OperationName{d_stk_neg, "NEG"},                     //
    OperationName{d_stk_pwr, "PWR"},                     //
    OperationName{d_stk_mod, "MOD"},                     //
    OperationName{d_stk_lt, "LT"},                       //
    OperationName{d_stk_gt, "GT"},                       //
    OperationName{d_stk_lte, "LTE"},                     //
    OperationName{d_stk_gte, "GTE"},                     //
    OperationName{d_stk_eq, "EQ"},                       //
    OperationName{d_stk_ne, "NE"},                       //
    OperationName{d_stk_or, "OR"},                       //
    OperationName{d_stk_and, "AND"},                     //
    OperationName{d_stk_asin, "ASIN"},                   //
    OperationName{d_stk_asinh, "ASINH"},                 //
    OperationName{d_stk_acos, "ACOS"},                   //
    OperationName{d_stk_acosh, "ACOSH"},                 //
    OperationName{d_stk_atan, "ATAN"},                   //
    OperationName{d_stk_atanh, "ATANH"},                 //
    OperationName{d_stk_cabs, "CABS"},                   //
    OperationName{d_stk_floor, "FLOOR"},                 //
    OperationName{d_stk_ceil, "CEIL"},                   //
    OperationName{d_stk_trunc, "TRUNC"},                 //
    OperationName{d_stk_round, "ROUND"},                 //
    OperationName{d_stk_zero, "ZERO"},                   //
    OperationName{d_stk_one, "ONE"},                     //
    OperationName{d_stk_srand, "SRAND"},                 //
    OperationName{d_stk_fn1, "FN1"},                     //
    OperationName{d_stk_fn2, "FN2"},                     //
    OperationName{d_stk_fn3, "FN3"},                     //
    OperationName{d_stk_fn4, "FN4"},                     //
    OperationName{stk_lod, "LOAD"},                      //
    OperationName{d_stk_lod_dup, "LOAD_DUP"},            //
    OperationName{d_stk_lod_sqr, "LOAD_SQR"},            //
    OperationName{d_stk_lod_sqr2, "LOAD_SQR2"},          //
    OperationName{d_stk_lod_dbl, "LOAD_DBL"},            //
    OperationName{stk_sto, "STORE"},                     //
    OperationName{stk_clr, "CLEAR"},                     //
    OperationName{d_stk_jump_on_false, "JUMP_IF_FALSE"}, //
    OperationName{d_stk_jump_on_true, "JUMP_IF_TRUE"},   //
    OperationName{stk_jump, "JUMP"},                     //
    OperationName{stk_jump_label, "JUMP_LABEL"},         //
    OperationName{end_init, "END_INIT"},                 //
    OperationName{d_stk_sqr0, "SQR0"},                   //
    OperationName{d_stk_sqr3, "SQR3"},                   //
    OperationName{stk_ident, "IDENT"},                   //
    OperationName{d_stk_recip, "RECIP"},                 //
};

static const char *get_operation_name(FunctionPtr fn)
{
    if (const auto it = std::find_if(s_op_names.begin(), s_op_names.end(),
            [fn](const OperationName &op_name) { return op_name.fn == fn; });
        it != s_op_names.end())
    {
        return it->name.data();
    }
    return "?unknown function";
}

static std::string get_variable_name(int var_index)
{
    // Check if it's a predefined variable
    if (var_index < static_cast<int>(VARIABLES.size()))
    {
        return std::string(VARIABLES[var_index]);
    }

    // Check if it's in the valid range of parsed variables
    if (var_index < static_cast<int>(g_formula.vars.size()) && var_index < g_formula.var_index)
    {
        const ConstArg &var = g_formula.vars[var_index];
        if (var.s != nullptr && var.len > 0)
        {
            // Use len to avoid reading past the intended string
            return std::string(var.s, var.len);
        }
    }

    return fmt::format("VAR_{}", var_index);
}

static int operator+(SymmetryType value)
{
    return static_cast<int>(value);
}

static void get_globals(std::ostringstream &oss)
{
    oss << fmt::format("=== Formula: {} ===\n", g_formula_name);
    oss << fmt::format("Symmetry: {}\n", +g_symmetry);
    oss << fmt::format("Total Operations: {}\n", g_formula.op_count);
    oss << fmt::format("Init Operations: {}\n", g_formula.last_init_op);
    oss << fmt::format("Variables: {}\n", g_formula.var_index);
    oss << fmt::format("Uses Jumps: {}\n", g_formula.uses_jump ? "yes" : "no");
    oss << fmt::format("Uses p1: {} p2: {} p3: {} p4: {} p5: {}\n", //
        g_formula.uses_p1, g_formula.uses_p2, g_formula.uses_p3,    //
        g_formula.uses_p4, g_formula.uses_p5);
    oss << fmt::format("Uses ismand: {}\n", g_formula.uses_ismand);
    oss << "\n";
}

static void get_variables(std::ostringstream &oss)
{
    // Dump variables
    oss << "=== Variables ===\n";
    for (int i = 0; i < g_formula.var_index && i < static_cast<int>(g_formula.vars.size()); ++i)
    {
        oss << fmt::format(
            "{:3d}: {:15s} = ({:.6f}, {:.6f})\n", i, get_variable_name(i), g_formula.vars[i].a.d.x, g_formula.vars[i].a.d.y);
    }
    oss << "\n";
}

static void get_operations(std::ostringstream &oss)
{
    // Dump operations
    oss << "=== Operations ===\n";
    for (int i = 0; i < static_cast<int>(g_formula.fns.size()); ++i)
    {
        oss << fmt::format("{:3d}: {}\n", i, get_operation_name(g_formula.fns[i]));
    }
    oss << "\n";
}

static void get_load_table(std::ostringstream &oss)
{
    // Count actual loads used by scanning operations
    int load_count = 0;
    for (size_t i = 0; i < g_formula.fns.size(); ++i)
    {
        FunctionPtr fn = g_formula.fns[i];
        if (fn == stk_lod)
        {
            load_count++;
        }
        else if (fn == d_stk_lod_dup)
        {
            load_count += 2;  // LOD_DUP uses 2 loads
        }
        else if (fn == d_stk_lod_sqr || fn == d_stk_lod_sqr2 || fn == d_stk_lod_dbl)
        {
            load_count++;
        }
    }

    // Dump load table - only entries actually used
    oss << "=== Load Table ===\n";
    for (int i = 0; i < load_count && i < static_cast<int>(g_formula.load.size()); ++i)
    {
        int var_idx = -1;
        if (g_formula.load[i] != nullptr)
        {
            for (unsigned v = 0; v < g_formula.var_index; ++v)
            {
                if (g_formula.load[i] == &g_formula.vars[v].a)
                {
                    var_idx = v;
                    break;
                }
            }
        }
        oss << fmt::format("{:3d}: {}\n", i, var_idx >= 0 ? get_variable_name(var_idx) : "UNKNOWN");
    }
    oss << "\n";
}

static void get_store_table(std::ostringstream &oss)
{
    // Count actual stores used by scanning operations
    int store_count = 0;
    for (const FunctionPtr &fn : g_formula.fns)
    {
        if (fn == stk_sto)
        {
            store_count++;
        }
    }

    // Dump store table - only entries actually used
    oss << "=== Store Table ===\n";
    for (int i = 0; i < store_count && i < static_cast<int>(g_formula.store.size()); ++i)
    {
        int var_idx = -1;
        if (g_formula.store[i] != nullptr)
        {
            for (unsigned v = 0; v < g_formula.var_index; ++v)
            {
                if (g_formula.store[i] == &g_formula.vars[v].a)
                {
                    var_idx = v;
                    break;
                }
            }
        }
        oss << fmt::format("{:3d}: {}\n", i, var_idx >= 0 ? get_variable_name(var_idx) : "UNKNOWN");
    }
    oss << "\n";
}

static void get_jump_control_table(std::ostringstream &oss)
{
    // Dump jump control table
    if (g_formula.uses_jump)
    {
        oss << "=== Jump Control Table ===\n";
        for (size_t i = 0; i < g_formula.jump_control.size(); ++i)
        {
            const char *type_name = "UNKNOWN";
            switch (g_formula.jump_control[i].type)
            {
            case JumpControlType::IF:
                type_name = "IF";
                break;
            case JumpControlType::ELSE_IF:
                type_name = "ELSE_IF";
                break;
            case JumpControlType::ELSE:
                type_name = "ELSE";
                break;
            case JumpControlType::END_IF:
                type_name = "END_IF";
                break;
            default:
                break;
            }
            oss << fmt::format("{:3d}: {:8s} -> op:{:3d} load:{:3d} store:{:3d} dest:{:3d}\n", i, type_name,
                g_formula.jump_control[i].ptrs.jump_op_ptr, g_formula.jump_control[i].ptrs.jump_lod_ptr,
                g_formula.jump_control[i].ptrs.jump_sto_ptr, g_formula.jump_control[i].dest_jump_index);
        }
        oss << "\n";
    }
}

std::string get_parser_state()
{
    if (g_formula_name.empty() || g_formula.fns.empty())
    {
        return "No formula loaded\n";
    }

    std::ostringstream oss;
    get_globals(oss);
    get_variables(oss);
    get_operations(oss);
    get_load_table(oss);
    get_store_table(oss);
    get_jump_control_table(oss);
    return oss.str();
}

bool parse_formula(const std::string &formula_text, std::string &error_msg)
{
    // Clear previous state
    free_work_area();
    g_formula.formula = formula_text;
    g_formula_name = "TEST_FORMULA";

    // Allocate structures
    parser_allocate();

    // Parse
    if (parse_formula_text(g_formula.formula))
    {
        error_msg = "Parse failed";
        return false;
    }

    // Fill jump structures if needed
    if (g_formula.uses_jump && fill_jump_struct())
    {
        error_msg = "Jump structure fill failed";
        return false;
    }

    error_msg.clear();
    return true;
}

void parser_reset()
{
    free_work_area();
    g_formula.formula.clear();
    g_formula.uses_jump = false;
    g_formula.jump_control.clear();
    g_formula_name.clear();
    g_formula.op_index = 0;
    g_formula.var_index = 0;
    g_formula.last_init_op = 0;
    g_formula.store_index = 0;
    g_formula.load_index = 0;
    g_formula.uses_p1 = false;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = false;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = false;
    g_formula.uses_ismand = false;
    g_formula.max_function = 0;
}

} // namespace id::fractals
