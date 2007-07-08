/* Parser.cpp (C) 1990, Mark C. Peterson, CompuServe [70441, 3353]
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
	405-C Queen St. Suite #181
	Southington, CT 06489
	(203) 276-9721
*/
#include <cassert>
#include <string>
#include <sstream>

#include <time.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"

#include "drivers.h"
#include "fpu.h"
#include "fractals.h"
#include "jiim.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"

#include "Formula.h"
#include "MathUtil.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef WATCH_MP
double x1;
double y1;
double x2;
double y2;
#endif

#define MAX_OPS 250
#define MAX_ARGS 100
#define MAX_BOXX 8192  /* max size of g_box_x array */

struct PEND_OP
{
	void (*function)();
	int p;
};

// indices correspond to VariableNames enum
static const char *Constants[] =
{
	"pixel",
	"p1",
	"p2",
	"z",
	"LastSqr",
	"pi",
	"e",
	"rand",
	"p3",
	"whitesq",
	"scrnpix",
	"scrnmax",
	"maxit",
	"ismand",
	"center",
	"magxmag",
	"rotskew",
	"p4",
	"p5"
};

struct SYMMETRY
{
	const char *s;
	SymmetryType n;
};

static SYMMETRY SymStr[] =
{
	{ "XAXIS_NOIMAG",	SYMMETRY_X_AXIS_NO_IMAGINARY },
	{ "PI_SYM_NOPARM",	SYMMETRY_PI_NO_PARAMETER },
	{ "ORIGIN_NOPARM",	SYMMETRY_ORIGIN_NO_PARAMETER },
	{ "XYAXIS_NOPARM",	SYMMETRY_XY_AXIS_NO_PARAMETER },
	{ "YAXIS_NOPARM",	SYMMETRY_Y_AXIS_NO_PARAMETER },
	{ "XAXIS_NOPARM",	SYMMETRY_X_AXIS_NO_PARAMETER },
	{ "NOSYM",			SYMMETRY_NONE },
	{ "XAXIS",			SYMMETRY_X_AXIS },
	{ "YAXIS",			SYMMETRY_Y_AXIS },
	{ "XYAXIS",			SYMMETRY_XY_AXIS },
	{ "ORIGIN",			SYMMETRY_ORIGIN },
	{ "PI_SYM",			SYMMETRY_PI },
	{ "XAXIS_NOREAL",	SYMMETRY_X_AXIS_NO_REAL },
	{ "NOPLOT",			SYMMETRY_NO_PLOT },
	{ "",				SYMMETRY_NONE }
};

class Random
{
public:
	Random()
		: m_random_number(0),
		m_set_random(false),
		m_randomized(false)
	{
	}
	~Random()
	{
	}

	bool random() const				{ return m_set_random; }
	bool randomized() const			{ return m_randomized; }

	void set_randomized(bool value) { m_randomized = value; }
	void set_random(bool value)		{ m_set_random = value; }

	unsigned long new_random_number();
	void set_random_function();
	void seed();

private:
	int m_random_number;
	bool m_set_random;
	bool m_randomized;
};

Formula g_formula_state;
double g_fudge_limit = 0.0;
bool g_is_mand = true;

static Random s_random;
static double s_fudge = 0.0;
static int s_delta16 = 0;
static int s_shift_back = 0;

Formula::Formula()
	: m_math_type(D_MATH),
	m_number_of_ops(0),
	m_number_of_loads(0),
	m_number_of_stores(0),
	m_number_of_jumps(0),
	m_initial_jump_index(0),
	m_variable_count(0),
	m_complex_count(0),
	m_real_count(0),
	m_chars_in_formula(0),
	m_next_operation(0),
	m_initial_n(0),
	m_parenthesis_count(0),
	m_expecting_arg(false),
	m_set_random(0),
	m_variable_list(NULL),
	m_complex_list(NULL),
	m_real_list(NULL),
	m_last_op(0),
	m_parser_vsp(0),
	m_formula_max_ops(MAX_OPS),
	m_formula_max_args(MAX_ARGS),
	m_op_index(0),
	m_uses_jump(false),
	m_jump_index(0),
	m_store_ptr(0),
	m_load_ptr(0),
	m_is_mand(1),
	m_posp(0),
	m_last_init_op(0),
	m_uses_is_mand(0),
	m_fudge_limit(0.0),
	m_uses_p1(false),
	m_uses_p2(false),
	m_uses_p3(false),
	m_uses_p4(false),
	m_uses_p5(false),
	m_max_function_number(0),
	m_function_load_store_pointers(NULL),
	m_variables(NULL),
	m_store(NULL),
	m_load(NULL),
	m_functions(NULL),
	m_arg1(),
	m_arg2(),
	m_formula_text(NULL),
	m_file_pos(0),
	m_statement_pos(0),
	m_errors_found(0),
	m_initial_load_pointer(0),
	m_initial_store_pointer(0),
	m_initial_op_pointer(0)
{
	{
		Arg zero_arg = { 0 };
		for (int i = 0; i < NUM_OF(m_argument_stack); i++)
		{
			m_argument_stack[i] = zero_arg;
		}
	}
	{
		JUMP_CONTROL zero_jump_control = { JUMPTYPE_NONE };
		for (int i = 0; i < NUM_OF(m_jump_control); i++)
		{
			m_jump_control[i] = zero_jump_control;
		}
	}
	{
		error_data zero_error_data = { 0 };
		for (int i = 0; i < NUM_OF(m_errors); i++)
		{
			m_errors[i] = zero_error_data;
		}
	}
	m_prepare_formula_text[0] = 0;
}

Formula::~Formula()
{
	free_work_area();
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

/* token IDs */
enum TokenIdType
{
	TOKENID_END_OF_FILE = 1,
	TOKENID_ILLEGAL_CHARACTER = 2,
	TOKENID_ILLEGAL_VARIABLE_NAME = 3,
	TOKENID_TOKEN_TOO_LONG = 4,
	TOKENID_FUNC_USED_AS_VAR = 5,
	TOKENID_JUMP_MISSING_BOOLEAN = 6,
	TOKENID_JUMP_WITH_ILLEGAL_CHAR = 7,
	TOKENID_UNDEFINED_FUNCTION = 8,
	TOKENID_ILLEGAL_OPERATOR = 9,
	TOKENID_ILL_FORMED_CONSTANT = 10,
	TOKENID_OPEN_PARENS = 1,
	TOKENID_CLOSE_PARENS = -1
};

/* CAE fp added MAX_STORES and LOADS */
/* MAX_STORES must be even to make Unix alignment work */
/* TW made dependent on m_formula_max_ops */

#define MAX_STORES ((m_formula_max_ops/4)*2)  /* at most only half the ops can be stores */
#define MAX_LOADS (unsigned(m_formula_max_ops*.8))  /* and 80% can be loads */

static PEND_OP s_ops[2300];

Arg *Arg1;
Arg *Arg2;

#if !defined(XFRACT)
#define ChkLongDenom(denom)\
	do \
	{ \
		if ((denom == 0 || g_overflow) && g_save_release > 1920) \
		{\
			g_overflow = 1; \
			return; \
		}\
		else if (denom == 0) \
		{ \
			return; \
		} \
	} \
	while (0)

#endif

#define ChkFloatDenom(denom)\
	do \
	{ \
		if (fabs(denom) <= DBL_MIN)		\
		{								\
			if (g_save_release > 1920)	\
			{							\
				g_overflow = 1;			\
			}							\
			return;						\
		} \
	} \
	while (0)

/* error_messages() defines; all calls to error_messages(), or any variable which will
	be used as the argument in a call to error_messages(), should use one of these
	defines.
*/

enum ParserErrorType
{
	PE_NO_ERRORS_FOUND = -1,
	PE_SHOULD_BE_ARGUMENT,
	PE_SHOULD_BE_OPERATOR,
	PE_NEED_A_MATCHING_OPEN_PARENS,
	PE_NEED_MORE_CLOSE_PARENS,
	PE_UNDEFINED_OPERATOR,
	PE_UNDEFINED_FUNCTION,
	PE_TABLE_OVERFLOW,
	PE_NO_MATCH_RIGHT_PAREN,
	PE_NO_LEFT_BRACKET_FIRST_LINE,
	PE_UNEXPECTED_EOF,
	PE_INVALID_SYM_USING_NOSYM,
	PE_FORMULA_TOO_LARGE,
	PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA,
	PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED,
	PE_JUMP_NOT_FIRST,
	PE_NO_CHAR_AFTER_THIS_JUMP,
	PE_JUMP_NEEDS_BOOLEAN,
	PE_ENDIF_REQUIRED_AFTER_ELSE,
	PE_ENDIF_WITH_NO_IF,
	PE_MISPLACED_ELSE_OR_ELSEIF,
	PE_UNMATCHED_IF_IN_INIT_SECTION,
	PE_IF_WITH_NO_ENDIF,
	PE_ERROR_IN_PARSING_JUMP_STATEMENTS,
	PE_TOO_MANY_JUMPS,
	PE_FORMULA_NAME_TOO_LARGE,
	PE_ILLEGAL_ASSIGNMENT,
	PE_ILLEGAL_VAR_NAME,
	PE_INVALID_CONST,
	PE_ILLEGAL_CHAR,
	PE_NESTING_TOO_DEEP,
	PE_UNMATCHED_MODULUS,
	PE_FUNC_USED_AS_VAR,
	PE_NO_NEG_AFTER_EXPONENT,
	PE_TOKEN_TOO_LONG,
	PE_SECOND_COLON,
	PE_INVALID_CALL_TO_PARSEERRS
};

const char *Formula::error_messages(int which)
{
	static const char *error_strings[] =
	{
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
		"Next jump after \"else\" must be \"endif\"",
		"\"endif\" has no matching \"if\"",
		"Misplaced \"else\" or \"elseif()\"",
		"\"if ()\" in initialization has no matching \"endif\"",
		"\"if ()\" has no matching \"endif\"",
		"Error in parsing jump statements",
		"Formula has too many jump commands",
		"Formula name has too many characters",
		"Only variables are allowed to left of assignment",
		"Illegal variable name",
		"Invalid constant expression",
		"This character not supported by parser",
		"Nesting of parentheses exceeds maximum depth",
		"Unmatched modulus operator \"|\" in this expression",
		"Can't use function name as variable",
		"Negative exponent must be enclosed in parens",
		"Variable or constant exceeds 32 character limit",
		"Only one \":\" permitted in a formula",
		"Invalid Formula::errors code"
	};
	int last_error = NUM_OF(error_strings) - 1;
	return error_strings[which > last_error ? last_error : which];
}

static double double_from_fixpoint(long quantity)
{
	return double(quantity)/s_fudge;
}

static long fixpoint_from_double(double quantity)
{
	return long(quantity*s_fudge);
}

static int int_strlen(const char *text)
{
	return int(strlen(text));
}

/* use the following when only float functions are implemented to
	get MP math and Integer math */

#if !defined(XFRACT)
static void lStkFunct(void (*function)())   /* call lStk via dStk */
{
	/*
		intermediate variable needed for safety because of
		different size of double and long in Arg union
	*/
	double y = double_from_fixpoint(Arg1->l.y);
	Arg1->d.x = double_from_fixpoint(Arg1->l.x);
	Arg1->d.y = y;
	(*function)();
	if (fabs(Arg1->d.x) < g_fudge_limit && fabs(Arg1->d.y) < g_fudge_limit)
	{
		Arg1->l.x = fixpoint_from_double(Arg1->d.x);
		Arg1->l.y = fixpoint_from_double(Arg1->d.y);
	}
	else
	{
		g_overflow = 1;
	}
}
#endif

unsigned long Random::new_random_number()
{
	m_random_number = ((m_random_number << 15) + rand15()) ^ m_random_number;
	return m_random_number;
}

unsigned long new_random_number()
{
	return s_random.new_random_number();
}

void lRandom()
{
	g_formula_state.Random_l();
}

static long fixpoint_to_long(long quantity)
{
	return quantity >> (32 - g_bit_shift);
}

void Formula::Random_l()
{
	m_variables[VARIABLE_RAND].argument.l.x = fixpoint_to_long(new_random_number());
	m_variables[VARIABLE_RAND].argument.l.y = fixpoint_to_long(new_random_number());
}

void dRandom()
{
	g_formula_state.Random_d();
}

void Formula::Random_d()
{
	/* Use the same algorithm as for fixed math so that they will generate
          the same fractals when the srand() function is used. */
	long x = fixpoint_to_long(new_random_number());
	long y = fixpoint_to_long(new_random_number());
	m_variables[VARIABLE_RAND].argument.d.x = (double(x) / (1L << g_bit_shift));
	m_variables[VARIABLE_RAND].argument.d.y = (double(y) / (1L << g_bit_shift));

}

void Random::set_random_function()
{
	unsigned Seed;

	if (!m_set_random)
	{
		m_random_number = Arg1->l.x ^ Arg1->l.y;
	}

	Seed = unsigned(m_random_number)^unsigned(m_random_number >> 16);
	srand(Seed);
	m_set_random = true;

	/* Clear out the seed */
	new_random_number();
	new_random_number();
	new_random_number();
}

void Random::seed()
{
	time_t ltime;

	/* Use the current time to randomize the random number sequence. */
	time(&ltime);
	srand(unsigned(ltime));

	new_random_number();
	new_random_number();
	new_random_number();
	m_randomized = true;
}

void SetRandFnct()
{
	s_random.set_random_function();
}

void RandomSeed()
{
	s_random.seed();
}

void lStkSRand()
{
	g_formula_state.StackStoreRandom_l();
}

void Formula::StackStoreRandom_l()
{
#if !defined(XFRACT)
	SetRandFnct();
	lRandom();
	Arg1->l = m_variables[VARIABLE_RAND].argument.l;
#endif
}

void dStkSRand()
{
	g_formula_state.StackStoreRandom_d();
}

void Formula::StackStoreRandom_d()
{
	Arg1->l.x = long(Arg1->d.x*(1L << g_bit_shift));
	Arg1->l.y = long(Arg1->d.y*(1L << g_bit_shift));
	SetRandFnct();
	dRandom();
	Arg1->d = m_variables[VARIABLE_RAND].argument.d;
}

void (*StkSRand)() = dStkSRand;


void Formula::StackLoadDup_d()
{
	Arg1 += 2;
	Arg2 += 2;
	*Arg2 = *Arg1 = *m_load[m_load_ptr];
	m_load_ptr += 2;
}

void dStkLodSqr()
{
	return g_formula_state.StackLoadSqr_d();
}

void Formula::StackLoadSqr_d()
{
	Arg1++;
	Arg2++;
	Arg1->d.y = m_load[m_load_ptr]->d.x*m_load[m_load_ptr]->d.y*2.0;
	Arg1->d.x = (m_load[m_load_ptr]->d.x*m_load[m_load_ptr]->d.x) - (m_load[m_load_ptr]->d.y*m_load[m_load_ptr]->d.y);
	m_load_ptr++;
}

void dStkLodSqr2()
{
	g_formula_state.StackLoadSqr2_d();
}

void Formula::StackLoadSqr2_d()
{
	Arg1++;
	Arg2++;
	m_variables[VARIABLE_LAST_SQR].argument.d.x = m_load[m_load_ptr]->d.x*m_load[m_load_ptr]->d.x;
	m_variables[VARIABLE_LAST_SQR].argument.d.y = m_load[m_load_ptr]->d.y*m_load[m_load_ptr]->d.y;
	Arg1->d.y = m_load[m_load_ptr]->d.x*m_load[m_load_ptr]->d.y*2.0;
	Arg1->d.x = m_variables[VARIABLE_LAST_SQR].argument.d.x - m_variables[VARIABLE_LAST_SQR].argument.d.y;
	m_variables[VARIABLE_LAST_SQR].argument.d.x += m_variables[VARIABLE_LAST_SQR].argument.d.y;
	m_variables[VARIABLE_LAST_SQR].argument.d.y = 0;
	m_load_ptr++;
}

void dStkLodDbl()
{
	g_formula_state.StackLoadDouble();
}

void Formula::StackLoadDouble()
{
	Arg1++;
	Arg2++;
	Arg1->d.x = m_load[m_load_ptr]->d.x*2.0;
	Arg1->d.y = m_load[m_load_ptr]->d.y*2.0;
	m_load_ptr++;
}

void dStkSqr0()
{
	g_formula_state.StackSqr0();
}

void Formula::StackSqr0()
{
	m_variables[VARIABLE_LAST_SQR].argument.d.y = Arg1->d.y*Arg1->d.y; /* use LastSqr as temp storage */
	Arg1->d.y = Arg1->d.x*Arg1->d.y*2.0;
	Arg1->d.x = Arg1->d.x*Arg1->d.x - m_variables[VARIABLE_LAST_SQR].argument.d.y;
}

void dStkSqr3()
{
	Arg1->d.x = Arg1->d.x*Arg1->d.x;
}

void dStkAbs()
{
	Arg1->d.x = fabs(Arg1->d.x);
	Arg1->d.y = fabs(Arg1->d.y);
}

#if !defined(XFRACT)
void lStkAbs()
{
	Arg1->l.x = labs(Arg1->l.x);
	Arg1->l.y = labs(Arg1->l.y);
}
#endif

void (*StkAbs)() = dStkAbs;

void dStkSqr()
{
	g_formula_state.StackSqr_d();
}

void Formula::StackSqr_d()
{
	m_variables[VARIABLE_LAST_SQR].argument.d.x = Arg1->d.x*Arg1->d.x;
	m_variables[VARIABLE_LAST_SQR].argument.d.y = Arg1->d.y*Arg1->d.y;
	Arg1->d.y = Arg1->d.x*Arg1->d.y*2.0;
	Arg1->d.x = m_variables[VARIABLE_LAST_SQR].argument.d.x - m_variables[VARIABLE_LAST_SQR].argument.d.y;
	m_variables[VARIABLE_LAST_SQR].argument.d.x += m_variables[VARIABLE_LAST_SQR].argument.d.y;
	m_variables[VARIABLE_LAST_SQR].argument.d.y = 0;
}

void lStkSqr()
{
	g_formula_state.StackSqr_l();
}

void Formula::StackSqr_l()
{
#if !defined(XFRACT)
	m_variables[VARIABLE_LAST_SQR].argument.l.x = multiply(Arg1->l.x, Arg1->l.x, g_bit_shift);
	m_variables[VARIABLE_LAST_SQR].argument.l.y = multiply(Arg1->l.y, Arg1->l.y, g_bit_shift);
	Arg1->l.y = multiply(Arg1->l.x, Arg1->l.y, g_bit_shift) << 1;
	Arg1->l.x = m_variables[VARIABLE_LAST_SQR].argument.l.x - m_variables[VARIABLE_LAST_SQR].argument.l.y;
	m_variables[VARIABLE_LAST_SQR].argument.l.x += m_variables[VARIABLE_LAST_SQR].argument.l.y;
	m_variables[VARIABLE_LAST_SQR].argument.l.y = 0L;
#endif
}

void (*StkSqr)() = dStkSqr;

void dStkAdd()
{
	Arg2->d.x += Arg1->d.x;
	Arg2->d.y += Arg1->d.y;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkAdd()
{
	Arg2->l.x += Arg1->l.x;
	Arg2->l.y += Arg1->l.y;
	Arg1--;
	Arg2--;
}
#endif

void (*StkAdd)() = dStkAdd;

void dStkSub()
{
	Arg2->d.x -= Arg1->d.x;
	Arg2->d.y -= Arg1->d.y;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkSub()
{
	Arg2->l.x -= Arg1->l.x;
	Arg2->l.y -= Arg1->l.y;
	Arg1--;
	Arg2--;
}
#endif

void (*StkSub)() = dStkSub;

void dStkConj()
{
	Arg1->d.y = -Arg1->d.y;
}

#if !defined(XFRACT)
void lStkConj()
{
	Arg1->l.y = -Arg1->l.y;
}
#endif

void (*StkConj)() = dStkConj;

void dStkFloor()
{
	Arg1->d.x = floor(Arg1->d.x);
	Arg1->d.y = floor(Arg1->d.y);
}

#if !defined(XFRACT)
void lStkFloor()
{
	/*
	* Kill fractional part. This operation truncates negative numbers
	* toward negative infinity as desired.
	*/
	Arg1->l.x = (Arg1->l.x) >> g_bit_shift;
	Arg1->l.y = (Arg1->l.y) >> g_bit_shift;
	Arg1->l.x = (Arg1->l.x) << g_bit_shift;
	Arg1->l.y = (Arg1->l.y) << g_bit_shift;
}
#endif

void (*StkFloor)() = dStkFloor;

void dStkCeil()
{
	Arg1->d.x = ceil(Arg1->d.x);
	Arg1->d.y = ceil(Arg1->d.y);
}

#if !defined(XFRACT)
void lStkCeil()
{
	/* the shift operation does the "floor" operation, so we
		negate everything before the operation */
	Arg1->l.x = (-Arg1->l.x) >> g_bit_shift;
	Arg1->l.y = (-Arg1->l.y) >> g_bit_shift;
	Arg1->l.x = -((Arg1->l.x) << g_bit_shift);
	Arg1->l.y = -((Arg1->l.y) << g_bit_shift);
}
#endif

void (*StkCeil)() = dStkCeil;

void dStkTrunc()
{
	Arg1->d.x = int(Arg1->d.x);
	Arg1->d.y = int(Arg1->d.y);
}

#if !defined(XFRACT)
void lStkTrunc()
{
	/* shifting and shifting back truncates positive numbers,
		so we make the numbers positive */
	int signx = sign(Arg1->l.x);
	int signy = sign(Arg1->l.y);
	Arg1->l.x = labs(Arg1->l.x);
	Arg1->l.y = labs(Arg1->l.y);
	Arg1->l.x = (Arg1->l.x) >> g_bit_shift;
	Arg1->l.y = (Arg1->l.y) >> g_bit_shift;
	Arg1->l.x = (Arg1->l.x) << g_bit_shift;
	Arg1->l.y = (Arg1->l.y) << g_bit_shift;
	Arg1->l.x = signx*Arg1->l.x;
	Arg1->l.y = signy*Arg1->l.y;
}
#endif

void (*StkTrunc)() = dStkTrunc;

void dStkRound()
{
	Arg1->d.x = floor(Arg1->d.x + .5);
	Arg1->d.y = floor(Arg1->d.y + .5);
}

#if !defined(XFRACT)
void lStkRound()
{
	/* Add .5 then truncate */
	Arg1->l.x += (1L << g_bit_shift_minus_1);
	Arg1->l.y += (1L << g_bit_shift_minus_1);
	lStkFloor();
}
#endif

void (*StkRound)() = dStkRound;

void dStkZero()
{
	Arg1->d.y = Arg1->d.x = 0.0;
}

#if !defined(XFRACT)
void lStkZero()
{
	Arg1->l.y = Arg1->l.x = 0;
}
#endif

void (*StkZero)() = dStkZero;

void dStkOne()
{
	Arg1->d.x = 1.0;
	Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void lStkOne()
{
	Arg1->l.x = long(s_fudge);
	Arg1->l.y = 0L;
}
#endif

void (*StkOne)() = dStkOne;


void dStkReal()
{
	Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void lStkReal()
{
	Arg1->l.y = 0l;
}
#endif

void (*StkReal)() = dStkReal;

void dStkImag()
{
	Arg1->d.x = Arg1->d.y;
	Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void lStkImag()
{
	Arg1->l.x = Arg1->l.y;
	Arg1->l.y = 0l;
}
#endif

void (*StkImag)() = dStkImag;

void dStkNeg()
{
	Arg1->d.x = -Arg1->d.x;
	Arg1->d.y = -Arg1->d.y;
}

#if !defined(XFRACT)
void lStkNeg()
{
	Arg1->l.x = -Arg1->l.x;
	Arg1->l.y = -Arg1->l.y;
}
#endif

void (*StkNeg)() = dStkNeg;

void dStkMul()
{
	FPUcplxmul(&Arg2->d, &Arg1->d, &Arg2->d);
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkMul()
{
	long x = multiply(Arg2->l.x, Arg1->l.x, g_bit_shift) -
		multiply(Arg2->l.y, Arg1->l.y, g_bit_shift);
	long y = multiply(Arg2->l.y, Arg1->l.x, g_bit_shift) +
		multiply(Arg2->l.x, Arg1->l.y, g_bit_shift);
	Arg2->l.x = x;
	Arg2->l.y = y;
	Arg1--;
	Arg2--;
}
#endif

void (*StkMul)() = dStkMul;

void dStkDiv()
{
	FPUcplxdiv(&Arg2->d, &Arg1->d, &Arg2->d);
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkDiv()
{
	long mod = multiply(Arg1->l.x, Arg1->l.x, g_bit_shift) +
		multiply(Arg1->l.y, Arg1->l.y, g_bit_shift);
	long x = divide(Arg1->l.x, mod, g_bit_shift);
	long y = -divide(Arg1->l.y, mod, g_bit_shift);
	long x2 = multiply(Arg2->l.x, x, g_bit_shift) - multiply(Arg2->l.y, y, g_bit_shift);
	long y2 = multiply(Arg2->l.y, x, g_bit_shift) + multiply(Arg2->l.x, y, g_bit_shift);
	Arg2->l.x = x2;
	Arg2->l.y = y2;
	Arg1--;
	Arg2--;
}
#endif

void (*StkDiv)() = dStkDiv;

void dStkMod()
{
	Arg1->d.x = (Arg1->d.x*Arg1->d.x) + (Arg1->d.y*Arg1->d.y);
	Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void lStkMod()
{
	Arg1->l.x = multiply(Arg1->l.x, Arg1->l.x, g_bit_shift) +
	multiply(Arg1->l.y, Arg1->l.y, g_bit_shift);
	if (Arg1->l.x < 0)
	{
		g_overflow = true;
	}
	Arg1->l.y = 0L;
}

void lStkModOld()
{
	Arg1->l.x = multiply(Arg2->l.x, Arg1->l.x, g_bit_shift) +
	multiply(Arg2->l.y, Arg1->l.y, g_bit_shift);
	if (Arg1->l.x < 0)
	{
		g_overflow = true;
	}
	Arg1->l.y = 0L;
}
#endif

void (*StkMod)() = dStkMod;

void StkSto()
{
	g_formula_state.StackStore();
}

void Formula::StackStore()
{
	*m_store[m_store_ptr++] = *Arg1;
}

void (*PtrStkSto)() = StkSto;

void StkLod()
{
	g_formula_state.StackLoad();
}

void Formula::StackLoad()
{
	Arg1++;
	Arg2++;
	*Arg1 = *m_load[m_load_ptr++];
}

void StkClr()
{
	g_formula_state.StackClear();
}

void Formula::StackClear()
{
	m_argument_stack[0] = *Arg1;
	Arg1 = &m_argument_stack[0];
	Arg2 = Arg1;
	Arg2--;
}

void (*PtrStkClr)() = StkClr;

void dStkFlip()
{
	double t;

	t = Arg1->d.x;
	Arg1->d.x = Arg1->d.y;
	Arg1->d.y = t;
}

#if !defined(XFRACT)
void lStkFlip()
{
	long t;

	t = Arg1->l.x;
	Arg1->l.x = Arg1->l.y;
	Arg1->l.y = t;
}
#endif

void (*StkFlip)() = dStkFlip;

void dStkSin()
{
	double sinx;
	double cosx;
	FPUsincos(&Arg1->d.x, &sinx, &cosx);
	double sinhy;
	double coshy;
	FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
	Arg1->d.x = sinx*coshy;
	Arg1->d.y = cosx*sinhy;
}

#if !defined(XFRACT)
void lStkSin()
{
	long x = Arg1->l.x >> s_delta16;
	long y = Arg1->l.y >> s_delta16;
	long sinx;
	long cosx;
	SinCos086(x, &sinx, &cosx);
	long sinhy;
	long coshy;
	SinhCosh086(y, &sinhy, &coshy);
	Arg1->l.x = multiply(sinx, coshy, s_shift_back);
	Arg1->l.y = multiply(cosx, sinhy, s_shift_back);
}
#endif

void (*StkSin)() = dStkSin;

/* The following functions are supported by both the parser and for fn
	variable replacement. */

void dStkTan()
{
	Arg1->d.x *= 2;
	Arg1->d.y *= 2;
	double sinx;
	double cosx;
	FPUsincos(&Arg1->d.x, &sinx, &cosx);
	double sinhy;
	double coshy;
	FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
	double denom = cosx + coshy;
	ChkFloatDenom(denom);
	Arg1->d.x = sinx/denom;
	Arg1->d.y = sinhy/denom;
}

#if !defined(XFRACT)
void lStkTan()
{
	long x = (Arg1->l.x >> s_delta16)*2;
	long y = (Arg1->l.y >> s_delta16)*2;
	long sinx;
	long cosx;
	SinCos086(x, &sinx, &cosx);
	long sinhy;
	long coshy;
	SinhCosh086(y, &sinhy, &coshy);
	long denom = cosx + coshy;
	ChkLongDenom(denom);
	Arg1->l.x = divide(sinx, denom, g_bit_shift);
	Arg1->l.y = divide(sinhy, denom, g_bit_shift);
}
#endif

void (*StkTan)() = dStkTan;

void dStkTanh()
{
	Arg1->d.x *= 2;
	Arg1->d.y *= 2;
	double siny;
	double cosy;
	FPUsincos(&Arg1->d.y, &siny, &cosy);
	double sinhx;
	double coshx;
	FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
	double denom = coshx + cosy;
	ChkFloatDenom(denom);
	Arg1->d.x = sinhx/denom;
	Arg1->d.y = siny/denom;
}

#if !defined(XFRACT)
void lStkTanh()
{
	long x = Arg1->l.x >> s_delta16;
	x <<= 1;
	long y = Arg1->l.y >> s_delta16;
	y <<= 1;
	long siny;
	long cosy;
	SinCos086(y, &siny, &cosy);
	long sinhx;
	long coshx;
	SinhCosh086(x, &sinhx, &coshx);
	long denom = coshx + cosy;
	ChkLongDenom(denom);
	Arg1->l.x = divide(sinhx, denom, g_bit_shift);
	Arg1->l.y = divide(siny, denom, g_bit_shift);
}
#endif

void (*StkTanh)() = dStkTanh;

void dStkCoTan()
{
	Arg1->d.x *= 2;
	Arg1->d.y *= 2;
	double sinx;
	double cosx;
	FPUsincos(&Arg1->d.x, &sinx, &cosx);
	double sinhy;
	double coshy;
	FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
	double denom = coshy - cosx;
	ChkFloatDenom(denom);
	Arg1->d.x = sinx/denom;
	Arg1->d.y = -sinhy/denom;
}

#if !defined(XFRACT)
void lStkCoTan()
{
	long x = Arg1->l.x >> s_delta16;
	x <<= 1;
	long y = Arg1->l.y >> s_delta16;
	y <<= 1;
	long sinx;
	long cosx;
	SinCos086(x, &sinx, &cosx);
	long sinhy;
	long coshy;
	SinhCosh086(y, &sinhy, &coshy);
	long denom = coshy - cosx;
	ChkLongDenom(denom);
	Arg1->l.x = divide(sinx, denom, g_bit_shift);
	Arg1->l.y = -divide(sinhy, denom, g_bit_shift);
}
#endif

void (*StkCoTan)() = dStkCoTan;

void dStkCoTanh()
{
	Arg1->d.x *= 2;
	Arg1->d.y *= 2;
	double siny;
	double cosy;
	FPUsincos(&Arg1->d.y, &siny, &cosy);
	double sinhx;
	double coshx;
	FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
	double denom = coshx - cosy;
	ChkFloatDenom(denom);
	Arg1->d.x = sinhx/denom;
	Arg1->d.y = -siny/denom;
}

#if !defined(XFRACT)
void lStkCoTanh()
{
	long x = Arg1->l.x >> s_delta16;
	x <<= 1;
	long y = Arg1->l.y >> s_delta16;
	y <<= 1;
	long siny;
	long cosy;
	SinCos086(y, &siny, &cosy);
	long sinhx;
	long coshx;
	SinhCosh086(x, &sinhx, &coshx);
	long denom = coshx - cosy;
	ChkLongDenom(denom);
	Arg1->l.x = divide(sinhx, denom, g_bit_shift);
	Arg1->l.y = -divide(siny, denom, g_bit_shift);
}
#endif

void (*StkCoTanh)() = dStkCoTanh;

/* The following functions are not directly used by the parser - support
	for the parser was not provided because the existing parser language
	represents these quite easily. They are used for fn variable support
	in miscres.c but are placed here because they follow the pattern of
	the other parser functions. */

void dStkRecip()
{
	double mod;
	mod = Arg1->d.x*Arg1->d.x + Arg1->d.y*Arg1->d.y;
	ChkFloatDenom(mod);
	Arg1->d.x =  Arg1->d.x/mod;
	Arg1->d.y = -Arg1->d.y/mod;
}

#if !defined(XFRACT)
void lStkRecip()
{
	long mod;
	mod = multiply(Arg1->l.x, Arg1->l.x, g_bit_shift)
		+ multiply(Arg1->l.y, Arg1->l.y, g_bit_shift);
	if (g_save_release > 1920)
	{
		ChkLongDenom(mod);
	}
	else if (mod <= 0L)
	{
		return;
	}
	Arg1->l.x =  divide(Arg1->l.x, mod, g_bit_shift);
	Arg1->l.y = -divide(Arg1->l.y, mod, g_bit_shift);
}
#endif

void StkIdent()  /* do nothing - the function Z */
{
}

void dStkSinh()
{
	double siny;
	double cosy;
	FPUsincos(&Arg1->d.y, &siny, &cosy);
	double sinhx;
	double coshx;
	FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
	Arg1->d.x = sinhx*cosy;
	Arg1->d.y = coshx*siny;
}

#if !defined(XFRACT)
void lStkSinh()
{
	long x = Arg1->l.x >> s_delta16;
	long y = Arg1->l.y >> s_delta16;
	long siny;
	long cosy;
	SinCos086(y, &siny, &cosy);
	long sinhx;
	long coshx;
	SinhCosh086(x, &sinhx, &coshx);
	Arg1->l.x = multiply(cosy, sinhx, s_shift_back);
	Arg1->l.y = multiply(siny, coshx, s_shift_back);
}
#endif

void (*StkSinh)() = dStkSinh;

void dStkCos()
{
	double sinx;
	double cosx;
	FPUsincos(&Arg1->d.x, &sinx, &cosx);
	double sinhy;
	double coshy;
	FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
	Arg1->d.x = cosx*coshy;
	Arg1->d.y = -sinx*sinhy;
}

#if !defined(XFRACT)
void lStkCos()
{
	long x = Arg1->l.x >> s_delta16;
	long y = Arg1->l.y >> s_delta16;
	long sinx;
	long cosx;
	SinCos086(x, &sinx, &cosx);
	long sinhy;
	long coshy;
	SinhCosh086(y, &sinhy, &coshy);
	Arg1->l.x = multiply(cosx, coshy, s_shift_back);
	Arg1->l.y = -multiply(sinx, sinhy, s_shift_back);
}
#endif

void (*StkCos)() = dStkCos;

/* Bogus version of cos, to replicate bug which was in regular cos till v16: */

void dStkCosXX()
{
	dStkCos();
	Arg1->d.y = -Arg1->d.y;
}

#if !defined(XFRACT)
void lStkCosXX()
{
	lStkCos();
	Arg1->l.y = -Arg1->l.y;
}
#endif

void (*StkCosXX)() = dStkCosXX;

void dStkCosh()
{
	double siny;
	double cosy;
	FPUsincos(&Arg1->d.y, &siny, &cosy);
	double sinhx;
	double coshx;
	FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
	Arg1->d.x = coshx*cosy;
	Arg1->d.y = sinhx*siny;
}

#if !defined(XFRACT)
void lStkCosh()
{
	long x = Arg1->l.x >> s_delta16;
	long y = Arg1->l.y >> s_delta16;
	long siny;
	long cosy;
	SinCos086(y, &siny, &cosy);
	long sinhx;
	long coshx;
	SinhCosh086(x, &sinhx, &coshx);
	Arg1->l.x = multiply(cosy, coshx, s_shift_back);
	Arg1->l.y = multiply(siny, sinhx, s_shift_back);
}
#endif

void (*StkCosh)() = dStkCosh;

void dStkASin()
{
	Arcsinz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkASin()
{
	lStkFunct(dStkASin);
}
#endif

void (*StkASin)() = dStkASin;

void dStkASinh()
{
	Arcsinhz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkASinh()
{
	lStkFunct(dStkASinh);
}
#endif

void (*StkASinh)() = dStkASinh;

void dStkACos()
{
	Arccosz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkACos()
{
	lStkFunct(dStkACos);
}
#endif

void (*StkACos)() = dStkACos;

void dStkACosh()
{
	Arccoshz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkACosh()
{
	lStkFunct(dStkACosh);
}
#endif

void (*StkACosh)() = dStkACosh;

void dStkATan()
{
	Arctanz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkATan()
{
	lStkFunct(dStkATan);
}
#endif

void (*StkATan)() = dStkATan;

void dStkATanh()
{
	Arctanhz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void lStkATanh()
{
	lStkFunct(dStkATanh);
}
#endif

void (*StkATanh)() = dStkATanh;

void dStkSqrt()
{
	Arg1->d = ComplexSqrtFloat(Arg1->d.x, Arg1->d.y);
}

#if !defined(XFRACT)
void lStkSqrt()
{
	Arg1->l = ComplexSqrtLong(Arg1->l.x, Arg1->l.y);
}
#endif

void (*StkSqrt)() = dStkSqrt;

void dStkCAbs()
{
	Arg1->d.x = sqrt(sqr(Arg1->d.x) + sqr(Arg1->d.y));
	Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void lStkCAbs()
{
	lStkFunct(dStkCAbs);
}
#endif

void (*StkCAbs)() = dStkCAbs;

void dStkLT()
{
	Arg2->d.x = double(Arg2->d.x < Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkLT()
{
	Arg2->l.x = long(Arg2->l.x < Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkLT)() = dStkLT;

void dStkGT()
{
	Arg2->d.x = double(Arg2->d.x > Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkGT()
{
	Arg2->l.x = long(Arg2->l.x > Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkGT)() = dStkGT;

void dStkLTE()
{
	Arg2->d.x = double(Arg2->d.x <= Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkLTE()
{
	Arg2->l.x = long(Arg2->l.x <= Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkLTE)() = dStkLTE;

void dStkGTE()
{
	Arg2->d.x = double(Arg2->d.x >= Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkGTE()
{
	Arg2->l.x = long(Arg2->l.x >= Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkGTE)() = dStkGTE;

void dStkEQ()
{
	Arg2->d.x = double(Arg2->d.x == Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkEQ()
{
	Arg2->l.x = long(Arg2->l.x == Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkEQ)() = dStkEQ;

void dStkNE()
{
	Arg2->d.x = double(Arg2->d.x != Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkNE()
{
	Arg2->l.x = long(Arg2->l.x != Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkNE)() = dStkNE;

void dStkOR()
{
	Arg2->d.x = double(Arg2->d.x || Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkOR()
{
	Arg2->l.x = long(Arg2->l.x || Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkOR)() = dStkOR;

void dStkAND()
{
	Arg2->d.x = double(Arg2->d.x && Arg1->d.x);
	Arg2->d.y = 0.0;
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkAND()
{
	Arg2->l.x = long(Arg2->l.x && Arg1->l.x) << g_bit_shift;
	Arg2->l.y = 0l;
	Arg1--;
	Arg2--;
}
#endif

void (*StkAND)() = dStkAND;
void dStkLog()
{
	FPUcplxlog(&Arg1->d, &Arg1->d);
}

#if !defined(XFRACT)
void lStkLog()
{
	lStkFunct(dStkLog);
}
#endif

void (*StkLog)() = dStkLog;

void FPUcplxexp(ComplexD *x, ComplexD *z)
{
	FPUcplxexp387(x, z);
}

void dStkExp()
{
	FPUcplxexp(&Arg1->d, &Arg1->d);
}

#if !defined(XFRACT)
void lStkExp()
{
	lStkFunct(dStkExp);
}
#endif

void (*StkExp)() = dStkExp;

void dStkPwr()
{
	Arg2->d = ComplexPower(Arg2->d, Arg1->d);
	Arg1--;
	Arg2--;
}

#if !defined(XFRACT)
void lStkPwr()
{
	ComplexD x;
	ComplexD y;

	x.x = double_from_fixpoint(Arg2->l.x);
	x.y = double_from_fixpoint(Arg2->l.y);
	y.x = double_from_fixpoint(Arg1->l.x);
	y.y = double_from_fixpoint(Arg1->l.y);
	x = ComplexPower(x, y);
	if (fabs(x.x) < g_fudge_limit && fabs(x.y) < g_fudge_limit)
	{
		Arg2->l.x = fixpoint_from_double(x.x);
		Arg2->l.y = fixpoint_from_double(x.y);
	}
	else
	{
		g_overflow = true;
	}
	Arg1--;
	Arg2--;
}
#endif

void (*StkPwr)() = dStkPwr;

void Formula::end_init()
{
	m_last_init_op = m_op_index;
	m_initial_jump_index = m_jump_index;
}

void StkJump()
{
	g_formula_state.StackJump();
}

void Formula::StackJump()
{
	m_op_index =  m_jump_control[m_jump_index].ptrs.JumpOpPtr;
	m_load_ptr = m_jump_control[m_jump_index].ptrs.JumpLodPtr;
	m_store_ptr = m_jump_control[m_jump_index].ptrs.JumpStoPtr;
	m_jump_index = m_jump_control[m_jump_index].DestJumpIndex;
}

void dStkJumpOnFalse()
{
	return g_formula_state.StackJumpOnFalse_d();
}

void Formula::StackJumpOnFalse_d()
{
	if (Arg1->d.x == 0)
	{
		StkJump();
	}
	else
	{
		m_jump_index++;
	}
}

void lStkJumpOnFalse()
{
	g_formula_state.StackJumpOnFalse_l();
}

void Formula::StackJumpOnFalse_l()
{
	if (Arg1->l.x == 0)
	{
		StkJump();
	}
	else
	{
		m_jump_index++;
	}
}

void (*StkJumpOnFalse)() = dStkJumpOnFalse;

void dStkJumpOnTrue()
{
	g_formula_state.StackJumpOnTrue_d();
}

void Formula::StackJumpOnTrue_d()
{
	if (Arg1->d.x)
	{
		StkJump();
	}
	else
	{
		m_jump_index++;
	}
}

void lStkJumpOnTrue()
{
	g_formula_state.StackJumpOnTrue_l();
}

void Formula::StackJumpOnTrue_l()
{
	if (Arg1->l.x)
	{
		StkJump();
	}
	else
	{
		m_jump_index++;
	}
}

void (*StkJumpOnTrue)() = dStkJumpOnTrue;

void StkJumpLabel()
{
	g_formula_state.StackJumpLabel();
}

void Formula::StackJumpLabel()
{
	m_jump_index++;
}

static int count_white_space(const char *text)
{
	int n = 0;
	while (text[n] && isspace(text[n]))
	{
		n++;
	}
	return n;
}

/* detect if constant is part of a (a, b) construct */
static bool is_constant_pair(const char *text)
{
	/* skip past first number */
	int n;
	for (n = 0; isdigit(text[n]) || text[n] == '.'; n++)
	{
	}
	if (text[n] == ',')
	{
		int j = n + count_white_space(&text[n + 1]) + 1;
		if (isdigit(text[j])
			|| (text[j] == '-' && (isdigit(text[j + 1]) || text[j + 1] == '.'))
			|| text[j] == '.')
		{
			return true;
		}
	}
	return false;
}

ConstArg *Formula::is_constant(const char *text, int length)
{
	ComplexD z;
	unsigned j;
	/* next line enforces variable vs constant naming convention */
	for (int n = 0; n < m_parser_vsp; n++)
	{
		if (m_variables[n].name_length == length)
		{
			if (!strnicmp(m_variables[n].name, text, length))
			{
				switch (n)
				{
				case VARIABLE_P1:		m_uses_p1 = true;		break;
				case VARIABLE_P2:		m_uses_p2 = true;		break;
				case VARIABLE_P3:		m_uses_p3 = true;		break;
				case VARIABLE_P4:		m_uses_p4 = true;		break;
				case VARIABLE_P5:		m_uses_p5 = true;		break;
				case VARIABLE_IS_MAND:	m_uses_is_mand = true;	break;
				case VARIABLE_RAND:		RandomSeed();			break;
				case VARIABLE_SCRN_PIX:
				case VARIABLE_SCRN_MAX:
				case VARIABLE_MAX_IT:
#if !defined(XFRACT)
					if (m_math_type == L_MATH)
					{
						driver_unget_key('f');
					}
#endif
					break;
				}
				if (!is_constant_pair(text))
				{
					return &m_variables[n];
				}
			}
		}
	}
	m_variables[m_parser_vsp].name = text;
	m_variables[m_parser_vsp].name_length = length;
	m_variables[m_parser_vsp].argument.d.x = 0.0;
	m_variables[m_parser_vsp].argument.d.y = 0.0;

	/* m_variables[m_parser_vsp].a should already be zeroed out */
	switch (m_math_type)
	{
#if !defined(XFRACT)
	case L_MATH:
		m_variables[m_parser_vsp].argument.l.x = 0;
		m_variables[m_parser_vsp].argument.l.y = 0;
		break;
#endif
	}

	if (isdigit(text[0])
		|| (text[0] == '-' && (isdigit(text[1]) || text[1] == '.'))
		|| text[0] == '.')
	{
		if (s_ops[m_posp-1].function == StkNeg)
		{
			m_posp--;
			text -= 1;
			m_initial_n--;
			m_variables[m_parser_vsp].name_length++;
		}
		int n;
		for (n = 1; isdigit(text[n]) || text[n] == '.'; n++)
		{
		}
		if (text[n] == ',')
		{
			j = n + count_white_space(&text[n + 1]) + 1;
			if (isdigit(text[j])
				|| (text[j] == '-' && (isdigit(text[j + 1]) || text[j + 1] == '.'))
				|| text[j] == '.')
			{
				z.y = atof(&text[j]);
				for (; isdigit(text[j]) || text[j] == '.' || text[j] == '-'; j++)
				{
				}
				m_variables[m_parser_vsp].name_length = j;
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
		z.x = atof(text);
		switch (m_math_type)
		{
		case D_MATH:
			m_variables[m_parser_vsp].argument.d = z;
			break;
#if !defined(XFRACT)
		case L_MATH:
			m_variables[m_parser_vsp].argument.l.x = fixpoint_from_double(z.x);
			m_variables[m_parser_vsp].argument.l.y = fixpoint_from_double(z.y);
			break;
#endif
		}
		m_variables[m_parser_vsp].name = text;
	}
	return &m_variables[m_parser_vsp++];
}

struct function_list_item
{
	const char *name;
	void (**function)();
};

void (*StkTrig0)() = dStkSin;
void (*StkTrig1)() = dStkSqr;
void (*StkTrig2)() = dStkSinh;
void (*StkTrig3)() = dStkCosh;

static const char *s_jump_keywords[] =
{
	"if",
	"elseif",
	"else",
	"endif"
};

static JumpType is_jump_keyword(const char *Str, int Len)
{
	for (int i = 0; i < NUM_OF(s_jump_keywords); i++)
	{
		if (int_strlen(s_jump_keywords[i]) == Len)
		{
			if (!strnicmp(s_jump_keywords[i], Str, Len))
			{
				return JumpType(i + 1);
			}
		}
	}
	return JUMPTYPE_NONE;
}

static function_list_item s_function_list[] =
{
	{"sin",    &StkSin },
	{"sinh",   &StkSinh },
	{"cos",    &StkCos },
	{"cosh",   &StkCosh },
	{"sqr",    &StkSqr },
	{"log",    &StkLog },
	{"exp",    &StkExp },
	{"abs",    &StkAbs },
	{"conj",   &StkConj },
	{"real",   &StkReal },
	{"imag",   &StkImag },
	{"fn1",    &StkTrig0 },
	{"fn2",    &StkTrig1 },
	{"fn3",    &StkTrig2 },
	{"fn4",    &StkTrig3 },
	{"flip",   &StkFlip },
	{"tan",    &StkTan },
	{"tanh",   &StkTanh },
	{"cotan",  &StkCoTan },
	{"cotanh", &StkCoTanh },
	{"cosxx",  &StkCosXX },
	{"srand",  &StkSRand },
	{"asin",   &StkASin },
	{"asinh",  &StkASinh },
	{"acos",   &StkACos },
	{"acosh",  &StkACosh },
	{"atan",   &StkATan },
	{"atanh",  &StkATanh },
	{"sqrt",   &StkSqrt },
	{"cabs",   &StkCAbs },
	{"floor",  &StkFloor },
	{"ceil",   &StkCeil },
	{"trunc",  &StkTrunc },
	{"round",  &StkRound },
};

enum OperatorType
{
	OPERATOR_COMMA = 0,
	OPERATOR_NOT_EQUAL,
	OPERATOR_ASSIGNMENT,
	OPERATOR_EQUAL,
	OPERATOR_LESS,
	OPERATOR_LESS_EQUAL,
	OPERATOR_GREATER,
	OPERATOR_GREATER_EQUAL,
	OPERATOR_MODULUS,
	OPERATOR_OR,
	OPERATOR_AND,
	OPERATOR_COLON,
	OPERATOR_PLUS,
	OPERATOR_MINUS,
	OPERATOR_MULTIPLY,
	OPERATOR_DIVIDE,
	OPERATOR_RAISE_POWER
};

static const char *s_operator_list[] =
{
	",",	/*  0 */
	"!=",	/*  1 */
	"=",	/*  2 */
	"==",	/*  3 */
	"<",	/*  4 */
	"<=",	/*  5 */
	">",	/*  6 */
	">=",	/*  7 */
	"|",	/*  8 */
	"||",	/*  9 */
	"&&",	/* 10 */
	":",	/* 11 */
	"+",	/* 12 */
	"-",	/* 13 */
	"*",	/* 14 */
	"/",	/* 15 */
	"^"		/* 16 */
};

static void not_a_function()
{
}

static void function_not_found()
{
}

/* determine if text names a function and if so which one */
static int which_function(const char *text, int length)
{
	if ((length != 3) || strnicmp(text, "fn", 2))
	{
		return 0;
	}

	int function_number = atoi(text + 2);
	if (function_number < 1 || function_number > 4)
	{
		return 0;
	}

	return function_number;
}

t_function *Formula::is_function(const char *text, int length)
{
	unsigned n = count_white_space(&text[length]);
	if (text[length + n] == '(')
	{
		for (int i = 0; i < NUM_OF(s_function_list); i++)
		{
			if (int_strlen(s_function_list[i].name) == length)
			{
				if (!strnicmp(s_function_list[i].name, text, length))
				{
					/* count function variables */
					int function_number = which_function(text, length);
					if (function_number != 0)
					{
						if (function_number > m_max_function_number)
						{
							m_max_function_number = function_number;
						}
					}
					return *s_function_list[i].function;
				}
			}
		}
		return function_not_found;
	}
	return not_a_function;
}

void Formula::RecSortPrec()
{
	int current = m_next_operation++;
	while (s_ops[current].p > s_ops[m_next_operation].p && m_next_operation < m_posp)
	{
		RecSortPrec();
	}
	m_functions[m_op_index++] = s_ops[current].function;
}

int Formula::GetP(int offset, int store_count)
{
	return offset - (m_parenthesis_count + store_count)*15;
}

void Formula::StoreFunction(void (*function)(), int p)
{
	s_ops[m_posp].function = function;
	s_ops[m_posp++].p = p;
}

void Formula::StoreFunction(void (*function)(), int offset, int store_count)
{
	StoreFunction(function, GetP(offset, store_count));
}

bool Formula::ParseStr(const char *text, int pass)
{
	int modulus_flag = 999;
	int store_count = 0;
	int modulus[20];
	int modulus_stack = 0;
	s_random.set_random(false);
	s_random.set_randomized(false);
	m_uses_jump = false;
	m_jump_index = 0;
	if (!m_store || !m_load || !m_functions)
	{
		stop_message(0, error_messages(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
		return true;
	}
	switch (m_math_type)
	{
	case D_MATH:
		StkAdd = dStkAdd;
		StkSub = dStkSub;
		StkNeg = dStkNeg;
		StkMul = dStkMul;
		StkSin = dStkSin;
		StkSinh = dStkSinh;
		StkLT = dStkLT;
		StkLTE = dStkLTE;
		StkMod = dStkMod;
		StkSqr = dStkSqr;
		StkCos = dStkCos;
		StkCosh = dStkCosh;
		StkLog = dStkLog;
		StkExp = dStkExp;
		StkPwr = dStkPwr;
		StkDiv = dStkDiv;
		StkAbs = dStkAbs;
		StkReal = dStkReal;
		StkImag = dStkImag;
		StkConj = dStkConj;
		StkTrig0 = g_trig0_d;
		StkTrig1 = g_trig1_d;
		StkTrig2 = g_trig2_d;
		StkTrig3 = g_trig3_d;
		StkFlip = dStkFlip;
		StkTan = dStkTan;
		StkTanh = dStkTanh;
		StkCoTan = dStkCoTan;
		StkCoTanh = dStkCoTanh;
		StkCosXX = dStkCosXX;
		StkGT  = dStkGT;
		StkGTE = dStkGTE;
		StkEQ  = dStkEQ;
		StkNE  = dStkNE;
		StkAND = dStkAND;
		StkOR  = dStkOR;
		StkSRand = dStkSRand;
		StkASin = dStkASin;
		StkASinh = dStkASinh;
		StkACos = dStkACos;
		StkACosh = dStkACosh;
		StkATan = dStkATan;
		StkATanh = dStkATanh;
		StkCAbs = dStkCAbs;
		StkSqrt = dStkSqrt;
		StkZero = dStkZero;
		StkFloor = dStkFloor;
		StkCeil = dStkCeil;
		StkTrunc = dStkTrunc;
		StkRound = dStkRound;
		StkJumpOnTrue  = dStkJumpOnTrue;
		StkJumpOnFalse = dStkJumpOnFalse;
		StkOne = dStkOne;
		break;
#if !defined(XFRACT)
	case L_MATH:
		s_delta16 = g_bit_shift - 16;
		s_shift_back = 32 - g_bit_shift;
		StkAdd = lStkAdd;
		StkSub = lStkSub;
		StkNeg = lStkNeg;
		StkMul = lStkMul;
		StkSin = lStkSin;
		StkSinh = lStkSinh;
		StkLT = lStkLT;
		StkLTE = lStkLTE;
		StkMod = (g_save_release > 1826) ? lStkMod : lStkModOld;
		StkSqr = lStkSqr;
		StkCos = lStkCos;
		StkCosh = lStkCosh;
		StkLog = lStkLog;
		StkExp = lStkExp;
		StkPwr = lStkPwr;
		StkDiv = lStkDiv;
		StkAbs = lStkAbs;
		StkReal = lStkReal;
		StkImag = lStkImag;
		StkConj = lStkConj;
		StkTrig0 = g_trig0_l;
		StkTrig1 = g_trig1_l;
		StkTrig2 = g_trig2_l;
		StkTrig3 = g_trig3_l;
		StkFlip = lStkFlip;
		StkTan  = lStkTan;
		StkTanh  = lStkTanh;
		StkCoTan  = lStkCoTan;
		StkCoTanh  = lStkCoTanh;
		StkCosXX = lStkCosXX;
		StkGT  = lStkGT;
		StkGTE = lStkGTE;
		StkEQ  = lStkEQ;
		StkNE  = lStkNE;
		StkAND = lStkAND;
		StkOR  = lStkOR;
		StkSRand = lStkSRand;
		StkASin = lStkASin;
		StkACos = lStkACos;
		StkACosh = lStkACosh;
		StkATan = lStkATan;
		StkATanh = lStkATanh;
		StkCAbs = lStkCAbs;
		StkSqrt = lStkSqrt;
		StkZero = lStkZero;
		StkFloor = lStkFloor;
		StkCeil = lStkCeil;
		StkTrunc = lStkTrunc;
		StkRound = lStkRound;
		StkJumpOnTrue  = lStkJumpOnTrue;
		StkJumpOnFalse = lStkJumpOnFalse;
		StkOne = lStkOne;
		break;
#endif
	}
	m_max_function_number = 0;
	for (m_parser_vsp = 0; m_parser_vsp < NUM_OF(Constants); m_parser_vsp++)
	{
		m_variables[m_parser_vsp].name = Constants[m_parser_vsp];
		m_variables[m_parser_vsp].name_length = int_strlen(Constants[m_parser_vsp]);
	}
	double center_real;
	double center_imag;
	LDBL magnification;
	double x_magnification_factor;
	double rotation;
	double skew;
	convert_center_mag(&center_real, &center_imag, &magnification, &x_magnification_factor, &rotation, &skew);
	m_variables[VARIABLE_RAND].argument.d.x = m_variables[VARIABLE_RAND].argument.d.y = 0.0;
	m_variables[VARIABLE_SCRN_MAX].argument.d.x = double(g_x_dots);
	m_variables[VARIABLE_SCRN_MAX].argument.d.y = double(g_y_dots);
	m_variables[VARIABLE_MAX_IT].argument.d.x = double(g_max_iteration);
	m_variables[VARIABLE_MAX_IT].argument.d.y = 0;
	m_variables[VARIABLE_IS_MAND].argument.d.x = g_is_mand ? 1.0 : 0.0;
	m_variables[VARIABLE_IS_MAND].argument.d.y = 0;
	m_variables[VARIABLE_CENTER].argument.d.x = center_real;
	m_variables[VARIABLE_CENTER].argument.d.y = center_imag;
	m_variables[VARIABLE_MAG_X_MAG].argument.d.x = double(magnification);
	m_variables[VARIABLE_MAG_X_MAG].argument.d.y = x_magnification_factor;
	m_variables[VARIABLE_ROT_SKEW].argument.d.x = rotation;
	m_variables[VARIABLE_ROT_SKEW].argument.d.y = skew;

	switch (m_math_type)
	{
	case D_MATH:
		m_variables[VARIABLE_P1].argument.d.x = g_parameters[0];
		m_variables[VARIABLE_P1].argument.d.y = g_parameters[1];
		m_variables[VARIABLE_P2].argument.d.x = g_parameters[2];
		m_variables[VARIABLE_P2].argument.d.y = g_parameters[3];
		m_variables[VARIABLE_PI].argument.d.x = MathUtil::Pi;
		m_variables[VARIABLE_PI].argument.d.y = 0.0;
		m_variables[VARIABLE_E].argument.d.x = MathUtil::e;
		m_variables[VARIABLE_E].argument.d.y = 0.0;
		m_variables[VARIABLE_P3].argument.d.x = g_parameters[4];
		m_variables[VARIABLE_P3].argument.d.y = g_parameters[5];
		m_variables[VARIABLE_P4].argument.d.x = g_parameters[6];
		m_variables[VARIABLE_P4].argument.d.y = g_parameters[7];
		m_variables[VARIABLE_P5].argument.d.x = g_parameters[8];
		m_variables[VARIABLE_P5].argument.d.y = g_parameters[9];
		break;
#if !defined(XFRACT)
	case L_MATH:
		m_variables[VARIABLE_P1].argument.l.x = fixpoint_from_double(g_parameters[0]);
		m_variables[VARIABLE_P1].argument.l.y = fixpoint_from_double(g_parameters[1]);
		m_variables[VARIABLE_P2].argument.l.x = fixpoint_from_double(g_parameters[2]);
		m_variables[VARIABLE_P2].argument.l.y = fixpoint_from_double(g_parameters[3]);
		m_variables[VARIABLE_PI].argument.l.x = fixpoint_from_double(MathUtil::Pi);
		m_variables[VARIABLE_PI].argument.l.y = 0L;
		m_variables[VARIABLE_E].argument.l.x = fixpoint_from_double(MathUtil::e);
		m_variables[VARIABLE_E].argument.l.y = 0L;
		m_variables[VARIABLE_P3].argument.l.x = fixpoint_from_double(g_parameters[4]);
		m_variables[VARIABLE_P3].argument.l.y = fixpoint_from_double(g_parameters[5]);
		m_variables[VARIABLE_SCRN_MAX].argument.l.x = g_x_dots << g_bit_shift;
		m_variables[VARIABLE_SCRN_MAX].argument.l.y = g_y_dots << g_bit_shift;
		m_variables[VARIABLE_MAX_IT].argument.l.x = g_max_iteration << g_bit_shift;
		m_variables[VARIABLE_MAX_IT].argument.l.y = 0L;
		m_variables[VARIABLE_IS_MAND].argument.l.x = (g_is_mand ? 1 : 0) << g_bit_shift;
		m_variables[VARIABLE_IS_MAND].argument.l.y = 0L;
		m_variables[VARIABLE_CENTER].argument.l.x = fixpoint_from_double(m_variables[VARIABLE_CENTER].argument.d.x);
		m_variables[VARIABLE_CENTER].argument.l.y = fixpoint_from_double(m_variables[VARIABLE_CENTER].argument.d.y);
		m_variables[VARIABLE_MAG_X_MAG].argument.l.x = fixpoint_from_double(m_variables[VARIABLE_MAG_X_MAG].argument.d.x);
		m_variables[VARIABLE_MAG_X_MAG].argument.l.y = fixpoint_from_double(m_variables[VARIABLE_MAG_X_MAG].argument.d.y);
		m_variables[VARIABLE_ROT_SKEW].argument.l.x = fixpoint_from_double(m_variables[VARIABLE_ROT_SKEW].argument.d.x);
		m_variables[VARIABLE_ROT_SKEW].argument.l.y = fixpoint_from_double(m_variables[VARIABLE_ROT_SKEW].argument.d.y);
		m_variables[VARIABLE_P4].argument.l.x = fixpoint_from_double(g_parameters[6]);
		m_variables[VARIABLE_P4].argument.l.y = fixpoint_from_double(g_parameters[7]);
		m_variables[VARIABLE_P5].argument.l.x = fixpoint_from_double(g_parameters[8]);
		m_variables[VARIABLE_P5].argument.l.y = fixpoint_from_double(g_parameters[9]);
		break;
#endif
	}

	m_last_init_op = 0;
	m_parenthesis_count = 0;
	m_op_index = 0;
	m_load_ptr = 0;
	m_store_ptr = 0;
	m_posp = 0;
	m_expecting_arg = true;
	for (int n = 0; text[n]; n++)
	{
		if (!text[n])
		{
			break;
		}
		m_initial_n = n;
		switch (text[n])
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			break;
		case '(':
			m_parenthesis_count++;
			break;
		case ')':
			m_parenthesis_count--;
			break;
		case '|':
			if (text[n + 1] == '|')
			{
				m_expecting_arg = true;
				n++;
				StoreFunction(StkOR, 7, store_count);
			}
			else if (modulus_flag == m_parenthesis_count-1)
			{
				m_parenthesis_count--;
				modulus_flag = modulus[--modulus_stack];
			}
			else
			{
				modulus[modulus_stack++] = modulus_flag;
				StoreFunction(StkMod, 2, store_count);
				modulus_flag = m_parenthesis_count++;
			}
			break;
		case ',':
		case ';':
			if (!m_expecting_arg)
			{
				m_expecting_arg = true;
				StoreFunction(NULL, 15);
				StoreFunction(StkClr, -30000);
				store_count = 0;
				m_parenthesis_count = 0;
			}
			break;
		case ':':
			m_expecting_arg = true;
			StoreFunction(NULL, 15);
			StoreFunction(EndInit, -30000);
			store_count = 0;
			m_parenthesis_count = 0;
			m_last_init_op = 10000;
			break;
		case '+':
			m_expecting_arg = true;
			StoreFunction(StkAdd, 4, store_count);
			break;
		case '-':
			if (m_expecting_arg)
			{
				StoreFunction(StkNeg, 2, store_count);
			}
			else
			{
				StoreFunction(StkSub, 4, store_count);
				m_expecting_arg = true;
			}
			break;
		case '&':
			m_expecting_arg = true;
			n++;
			StoreFunction(StkAND, 7, store_count);
			break;
		case '!':
			m_expecting_arg = true;
			n++;
			StoreFunction(StkNE, 6, store_count);
			break;
		case '<':
			m_expecting_arg = true;
			{
				void (*function)() = NULL;
				if (text[n + 1] == '=')
				{
					n++;
					function = StkLTE;
				}
				else
				{
					function = StkLT;
				}
				StoreFunction(function, 6, store_count);
			}
			break;
		case '>':
			m_expecting_arg = true;
			{
				void (*function)() = NULL;
				if (text[n + 1] == '=')
				{
					n++;
					function = StkGTE;
				}
				else
				{
					function = StkGT;
				}
				StoreFunction(function, 6, store_count);
			}
			break;
		case '*':
			m_expecting_arg = true;
			StoreFunction(StkMul, 3, store_count);
			break;
		case '/':
			m_expecting_arg = true;
			StoreFunction(StkDiv, 3, store_count);
			break;
		case '^':
			m_expecting_arg = true;
			StoreFunction(StkPwr, 2, store_count);
			break;
		case '=':
			m_expecting_arg = true;
			if (text[n + 1] == '=')
			{
				n++;
				StoreFunction(StkEQ, 6, store_count);
			}
			else
			{
				s_ops[m_posp-1].function = StkSto;
				s_ops[m_posp-1].p = GetP(5, store_count);
				m_store[m_store_ptr++] = m_load[--m_load_ptr];
				store_count++;
			}
			break;
		default:
			while (isalnum(text[n + 1]) || text[n + 1] == '.' || text[n + 1] == '_')
			{
				n++;
			}
			int length = (n + 1) - m_initial_n;
			m_expecting_arg = false;
			JumpType jump_type = is_jump_keyword(&text[m_initial_n], length);
			if (jump_type != JUMPTYPE_NONE)
			{
				m_uses_jump = true;
				switch (jump_type)
				{
				case JUMPTYPE_IF:
					m_expecting_arg = true;
					m_jump_control[m_jump_index++].type = JUMPTYPE_IF;
					StoreFunction(StkJumpOnFalse, 1);
					break;
				case JUMPTYPE_ELSEIF:
					m_expecting_arg = true;
					m_jump_control[m_jump_index++].type = JUMPTYPE_ELSEIF;
					m_jump_control[m_jump_index++].type = JUMPTYPE_ELSEIF;
					StoreFunction(StkJump, 1);
					StoreFunction(NULL, 15);
					StoreFunction(StkClr, -30000);
					StoreFunction(StkJumpOnFalse, 1);
					break;
				case JUMPTYPE_ELSE:
					m_jump_control[m_jump_index++].type = JUMPTYPE_ELSE;
					StoreFunction(StkJump, 1);
					break;
				case JUMPTYPE_ENDIF:
					m_jump_control[m_jump_index++].type = JUMPTYPE_ENDIF;
					StoreFunction(StkJumpLabel, 1);
					break;
				default:
					assert(false && "Bad jump type");
					break;
				}
			}
			else
			{
				s_ops[m_posp].function = is_function(&text[m_initial_n], length);
				if (s_ops[m_posp].function != not_a_function)
				{
					s_ops[m_posp++].p = GetP(1, store_count);
					m_expecting_arg = true;
				}
				else
				{
					ConstArg *c = is_constant(&text[m_initial_n], length);
					m_load[m_load_ptr++] = &(c->argument);
					StoreFunction(StkLod, 1, store_count);
					n = m_initial_n + c->name_length - 1;
				}
			}
			break;
		}
	}
	StoreFunction(NULL, 16);
	m_next_operation = 0;
	m_last_op = m_posp;
	while (m_next_operation < m_posp)
	{
		if (s_ops[m_next_operation].function)
		{
			RecSortPrec();
		}
		else
		{
			m_next_operation++;
			m_last_op--;
		}
	}
	return false;
}


int Formula::orbit()
{
	if (g_formula_name[0] == 0 || g_overflow)
	{
		return 1;
	}

	m_load_ptr = m_initial_load_pointer;
	m_store_ptr = m_initial_store_pointer;
	m_op_index = m_initial_op_pointer;
	m_jump_index = m_initial_jump_index;
	/* Set the random number */
	if (s_random.random() || s_random.randomized())
	{
		switch (m_math_type)
		{
		case D_MATH:
			dRandom();
			break;
#if !defined(XFRACT)
		case L_MATH:
			lRandom();
			break;
#endif
		}
	}

	Arg1 = &m_argument_stack[0];
	Arg2 = Arg1-1;
	while (m_op_index < m_last_op)
	{
		m_functions[m_op_index]();
		m_op_index++;
	}

	switch (m_math_type)
	{
	case D_MATH:
		g_old_z = g_new_z = m_variables[VARIABLE_Z].argument.d;
		return Arg1->d.x == 0.0;
#if !defined(XFRACT)
	case L_MATH:
		g_old_z_l = g_new_z_l = m_variables[VARIABLE_Z].argument.l;
		if (g_overflow)
		{
			return 1;
		}
		return Arg1->l.x == 0L;
#endif
	}
	return 1;
}

int Formula::per_pixel()
{
#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif
	if (g_formula_name[0] == 0)
	{
		return 1;
	}
	g_overflow = false;
	m_load_ptr = 0;
	m_store_ptr = 0;
	m_op_index = 0;
	m_jump_index = 0;
	Arg1 = &m_argument_stack[0];
	Arg2 = Arg1;
	Arg2--;


	m_variables[VARIABLE_SCRN_PIX].argument.d.x = double(g_col);
	m_variables[VARIABLE_SCRN_PIX].argument.d.y = double(g_row);

	switch (m_math_type)
	{
	case D_MATH:
		m_variables[VARIABLE_WHITE_SQ].argument.d.x = ((g_row + g_col) & 1) ? 1.0 : 0.0;
		m_variables[VARIABLE_WHITE_SQ].argument.d.y = 0.0;
		break;


#if !defined(XFRACT)
	case L_MATH:
		m_variables[VARIABLE_WHITE_SQ].argument.l.x = fixpoint_from_double((g_row + g_col) & 1);
		m_variables[VARIABLE_WHITE_SQ].argument.l.y = 0L;
		m_variables[VARIABLE_SCRN_PIX].argument.l.x = g_col;
		m_variables[VARIABLE_SCRN_PIX].argument.l.x <<= g_bit_shift;
		m_variables[VARIABLE_SCRN_PIX].argument.l.y = g_row;
		m_variables[VARIABLE_SCRN_PIX].argument.l.y <<= g_bit_shift;
		break;
#endif
	}

	/* TW started additions for inversion support here 4/17/94 */
	if (g_invert)
	{
		invert_z(&g_old_z);
		switch (m_math_type)
		{
		case D_MATH:
			m_variables[VARIABLE_PIXEL].argument.d.x = g_old_z.x;
			m_variables[VARIABLE_PIXEL].argument.d.y = g_old_z.y;
			break;
#if !defined(XFRACT)
		case L_MATH:
			/* watch out for overflow */
			if (sqr(g_old_z.x) + sqr(g_old_z.y) >= 127)
			{
				g_old_z.x = 8;  /* value to bail out in one iteration */
				g_old_z.y = 8;
			}
			/* convert to fudged longs */
			m_variables[VARIABLE_PIXEL].argument.l.x = fixpoint_from_double(g_old_z.x);
			m_variables[VARIABLE_PIXEL].argument.l.y = fixpoint_from_double(g_old_z.y);
			break;
#endif
		}
	}
	else
	{
		/* TW end of inversion support changes here 4/17/94 */
		switch (m_math_type)
		{
		case D_MATH:
			m_variables[VARIABLE_PIXEL].argument.d.x = g_dx_pixel();
			m_variables[VARIABLE_PIXEL].argument.d.y = g_dy_pixel();
			break;
#if !defined(XFRACT)
		case L_MATH:
			m_variables[VARIABLE_PIXEL].argument.l.x = g_lx_pixel();
			m_variables[VARIABLE_PIXEL].argument.l.y = g_ly_pixel();
			break;
#endif
		}
	}

	if (m_last_init_op)
	{
		m_last_init_op = m_last_op;
	}
	while (m_op_index < m_last_init_op)
	{
		m_functions[m_op_index]();
		m_op_index++;
	}
	m_initial_load_pointer = m_load_ptr;
	m_initial_store_pointer = m_store_ptr;
	m_initial_op_pointer = m_op_index;
	/* Set old variable for orbits */
	switch (m_math_type)
	{
	case D_MATH:
		g_old_z = m_variables[VARIABLE_Z].argument.d;
		break;
#if !defined(XFRACT)
	case L_MATH:
		g_old_z_l = m_variables[VARIABLE_Z].argument.l;
		break;
#endif
	}

	return g_overflow ? 0 : 1;
}

int Formula::fill_if_group(int endif_index, JUMP_PTRS *jump_data)
{
	int i = endif_index;
	int last_jump_processed = endif_index;
	while (i > 0)
	{
		i--;
		switch (m_jump_control[i].type)
		{
		case JUMPTYPE_IF:    /*if (); this concludes processing of this group*/
			m_jump_control[i].ptrs = jump_data[last_jump_processed];
			m_jump_control[i].DestJumpIndex = last_jump_processed + 1;
			return i;

		case JUMPTYPE_ELSEIF:    /*elseif* (2 jumps, the else and the if*/
			/* first, the "if" part */
			m_jump_control[i].ptrs = jump_data[last_jump_processed];
			m_jump_control[i].DestJumpIndex = last_jump_processed + 1;

			/* then, the else part */
			i--; /*fall through to "else" is intentional*/

		case JUMPTYPE_ELSE:
			m_jump_control[i].ptrs = jump_data[endif_index];
			m_jump_control[i].DestJumpIndex = endif_index + 1;
			last_jump_processed = i;
			break;

		case JUMPTYPE_ENDIF:
			i = fill_if_group(i, jump_data);
			break;

		default:
			break;
		}
	}
	assert(false && "should never get here");
	return -1;
}

bool Formula::fill_jump_struct()
{
	/* Completes all entries in jump structure. Returns 1 on error) */
	/* On entry, m_jump_index is the number of jump functions in the formula*/
	int i = 0;
	int load_count = 0;
	int store_count = 0;
	bool check_for_else = false;
	void (*JumpFunc)() = NULL;
	bool find_new_func = true;

	JUMP_PTRS jump_data[MAX_JUMPS];

	for (m_op_index = 0; m_op_index < m_last_op; m_op_index++)
	{
		if (find_new_func)
		{
			switch (m_jump_control[i].type)
			{
			case JUMPTYPE_IF:
				JumpFunc = StkJumpOnFalse;
				break;
			case JUMPTYPE_ELSEIF:
				check_for_else = !check_for_else;
				JumpFunc = check_for_else ? StkJump : StkJumpOnFalse;
				break;
			case JUMPTYPE_ELSE:
				JumpFunc = StkJump;
				break;
			case JUMPTYPE_ENDIF:
				JumpFunc = StkJumpLabel;
				break;
			default:
				break;
			}
			find_new_func = false;
		}
		if (*(m_functions[m_op_index]) == StkLod)
		{
			load_count++;
		}
		else if (*(m_functions[m_op_index]) == StkSto)
		{
			store_count++;
		}
		else if (*(m_functions[m_op_index]) == JumpFunc)
		{
			jump_data[i].JumpOpPtr = m_op_index;
			jump_data[i].JumpLodPtr = load_count;
			jump_data[i].JumpStoPtr = store_count;
			i++;
			find_new_func = true;
		}
	}

	/* Following for safety only; all should always be false */
	if (i != m_jump_index || m_jump_control[i - 1].type != JUMPTYPE_ENDIF
		|| m_jump_control[0].type != JUMPTYPE_IF)
	{
		return true;
	}

	while (i > 0)
	{
		i = fill_if_group(i-1, jump_data);
	}
	return (i < 0);
}

static int formula_get_char(FILE *openfile)
{
	int c;
	int done = 0;
	int linewrap = 0;
	while (!done)
	{
		c = getc(openfile);
		switch (c)
		{
		case '\r': case ' ' : case '\t' :
			break;
		case '\\':
			linewrap = 1;
			break;
		case ';' :
			do
			{
				c = getc(openfile);
			}
			while (c != '\n' && c != EOF && c != '\032');
			if (c == EOF || c == '\032')
			{
				done = 1;
			}
		case '\n' :
			if (!linewrap)
			{
				done = 1;
			}
			linewrap = 0;
			break;
		default:
			done = 1;
			break;
		}
	}
	return tolower(c);
}

/* This function also gets flow control info */

static void get_function_information(token_st *tok)
{
	int i;
	for (i = 0; i < NUM_OF(s_function_list); i++)
	{
		if (!strcmp(s_function_list[i].name, tok->token_str))
		{
			tok->token_id = i;
			tok->token_type = (i >= 11 && i <= 14) ? TOKENTYPE_PARAMETER_FUNCTION : TOKENTYPE_FUNCTION;
			return;
		}
	}

	for (i = 0; i < 4; i++)  /*pick up flow control*/
	{
		if (!strcmp(s_jump_keywords[i], tok->token_str))
		{
			tok->token_type = TOKENTYPE_FLOW_CONTROL;
			tok->token_id   = i + 1;
			return;
		}
	}
	tok->token_type = TOKENTYPE_NONE;
	tok->token_id   = TOKENID_UNDEFINED_FUNCTION;
	return;
}

static void get_variable_information(token_st *tok)
{
	int i;

	for (i = 0; i < NUM_OF(Constants); i++)
	{
		if (!strcmp(Constants[i], tok->token_str))
		{
			tok->token_id = i;
			switch (i)
			{
			case 1: case 2: case 8: case 13: case 17: case 18:
				tok->token_type = TOKENTYPE_PARAMETER_VARIABLE;
				break;
			default:
				tok->token_type = TOKENTYPE_PREDEFINED_VARIABLE;
				break;
			}
			return;
		}
	}
	tok->token_type = TOKENTYPE_USER_VARIABLE;
	tok->token_id   = 0;
}

/* fills in token structure where numeric constant is indicated */
/* Note - this function will be called twice to fill in the components
		of a complex constant. See is_complex_constant() below. */
/* returns true on success, false on TOKENTYPE_NONE */
static bool formula_get_constant(FILE *openfile, token_st *tok)
{
	int c;
	int i = 1;
	int getting_base = 1;
	long filepos = ftell(openfile);
	int got_decimal_already = 0;
	int done = 0;
	tok->token_const.x = 0.0;          /*initialize values to 0*/
	tok->token_const.y = 0.0;
	if (tok->token_str[0] == '.')
	{
		got_decimal_already = 1;
	}
	while (!done)
	{
		c = formula_get_char(openfile);
		switch (c)
		{
		case EOF: case '\032':
			tok->token_str[i] = 0;
			tok->token_type = TOKENTYPE_NONE;
			tok->token_id   = TOKENID_END_OF_FILE;
			return false;
		CASE_NUM:
			tok->token_str[i++] = char(c);
			filepos = ftell(openfile);
			break;
		case '.':
			if (got_decimal_already || !getting_base)
			{
				tok->token_str[i++] = char(c);
				tok->token_str[i++] = 0;
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id = TOKENID_ILL_FORMED_CONSTANT;
				return false;
			}
			else
			{
				tok->token_str[i++] = char(c);
				got_decimal_already = 1;
				filepos = ftell(openfile);
			}
			break;
		default :
			if (c == 'e' && getting_base && (isdigit(tok->token_str[i-1]) || (tok->token_str[i-1] == '.' && i > 1)))
			{
				tok->token_str[i++] = char(c);
				getting_base = 0;
				got_decimal_already = 0;
				filepos = ftell(openfile);
				c = formula_get_char(openfile);
				if (c == '-' || c == '+')
				{
					tok->token_str[i++] = char(c);
					filepos = ftell(openfile);
				}
				else
				{
					fseek(openfile, filepos, SEEK_SET);
				}
			}
			else if (isalpha(c) || c == '_')
			{
				tok->token_str[i++] = char(c);
				tok->token_str[i++] = 0;
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id = TOKENID_ILL_FORMED_CONSTANT;
				return false;
			}
			else if (tok->token_str[i-1] == 'e' || (tok->token_str[i-1] == '.' && i == 1))
			{
				tok->token_str[i++] = char(c);
				tok->token_str[i++] = 0;
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id = TOKENID_ILL_FORMED_CONSTANT;
				return false;
			}
			else
			{
				fseek(openfile, filepos, SEEK_SET);
				tok->token_str[i++] = 0;
				done = 1;
			}
			break;
		}
		if (i == 33 && tok->token_str[32])
		{
			tok->token_str[33] = 0;
			tok->token_type = TOKENTYPE_NONE;
			tok->token_id = TOKENID_TOKEN_TOO_LONG;
			return false;
		}
	}    /* end of while loop. Now fill in the value */
	tok->token_const.x = atof(tok->token_str);
	tok->token_type = TOKENTYPE_REAL_CONSTANT;
	tok->token_id   = 0;
	return true;
}

static void is_complex_constant(FILE *openfile, token_st *tok)
{
	/* should test to make sure tok->token_str[0] == '(' */
	token_st temp_tok;
	long filepos;
	int c;
	int sign_value = 1;
	int done = 0;
	int getting_real = 1;
	FILE *debug_token = NULL;
	tok->token_str[1] = 0;  /* so we can concatenate later */

	filepos = ftell(openfile);
	if (DEBUGMODE_DISK_MESSAGES == g_debug_mode)
	{
		debug_token = fopen("frmconst.txt", "at");
	}

	while (!done)
	{
		c = formula_get_char(openfile);
		switch (c)
		{
		CASE_NUM : case '.':
			if (debug_token != NULL)
			{
				fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
			}
			temp_tok.token_str[0] = char(c);
			break;
		case '-' :
			if (debug_token != NULL)
			{
				fprintf(debug_token,  "First char is a minus\n");
			}
			sign_value = -1;
			c = formula_get_char(openfile);
			if (c == '.' || isdigit(c))
			{
				if (debug_token != NULL)
				{
					fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
				}
				temp_tok.token_str[0] = char(c);
			}
			else
			{
				if (debug_token != NULL)
				{
					fprintf(debug_token,  "First char not a . or NUM\n");
				}
				done = 1;
			}
			break;
		default:
			if (debug_token != NULL)
			{
				fprintf(debug_token,  "First char not a . or NUM\n");
			}
			done = 1;
			break;
		}
		if (debug_token != NULL)
		{
				fprintf(debug_token,  "Calling frmgetconstant unless done is 1; done is %d\n", done);
		}
		if (!done && formula_get_constant(openfile, &temp_tok))
		{
			c = formula_get_char(openfile);
			if (debug_token != NULL)
			{
				fprintf(debug_token, "frmgetconstant returned 1; next token is %c\n", c);
			}
			if (getting_real && c == ',')  /*we have the real part now*/
			{
				if (sign_value == -1)
				{
					strcat(tok->token_str, "-");
				}
				strcat(tok->token_str, temp_tok.token_str);
				strcat(tok->token_str, ",");
				tok->token_const.x = temp_tok.token_const.x*sign_value;
				getting_real = 0;
				sign_value = 1;
			}
			else if (!getting_real && c == ')')  /* we have the complex part */
			{
				if (sign_value == -1)
				{
					strcat(tok->token_str, "-");
				}
				strcat(tok->token_str, temp_tok.token_str);
				strcat(tok->token_str, ")");
				tok->token_const.y = temp_tok.token_const.x*sign_value;
				tok->token_type = tok->token_const.y ? TOKENTYPE_COMPLEX_CONSTANT : TOKENTYPE_REAL_CONSTANT;
				tok->token_id   = 0;
				if (debug_token != NULL)
				{
					fprintf(debug_token,  "Exiting with type set to %d\n", tok->token_const.y ? TOKENTYPE_COMPLEX_CONSTANT : TOKENTYPE_REAL_CONSTANT);
					fclose(debug_token);
				}
				return;
			}
			else
			{
				done = 1;
			}
		}
		else
		{
			done = 1;
		}
	}
	fseek(openfile, filepos, SEEK_SET);
	tok->token_str[1] = 0;
	tok->token_const.y = tok->token_const.x = 0.0;
	tok->token_type = TOKENTYPE_PARENTHESIS;
	tok->token_id = TOKENID_OPEN_PARENS;
	if (debug_token != NULL)
	{
		fprintf(debug_token,  "Exiting with ID set to OPEN_PARENS\n");
		fclose(debug_token);
	}
	return;
}

static bool formula_get_alpha(FILE *openfile, token_st *tok)
{
	int c;
	int i = 1;
	int var_name_too_long = 0;
	long filepos;
	long last_filepos = ftell(openfile);
	while ((c = formula_get_char(openfile)) != EOF && c != '\032')
	{
		filepos = ftell(openfile);
		switch (c)
		{
		CASE_ALPHA: CASE_NUM: case '_':
			if (i < 79)
			{
				tok->token_str[i++] = char(c);
			}
			else
			{
				tok->token_str[i] = 0;
			}
			if (i == 33)
			{
				var_name_too_long = 1;
			}
			last_filepos = filepos;
			break;
		default:
			if (c == '.')  /*illegal character in variable or func name*/
			{
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id   = TOKENID_ILLEGAL_VARIABLE_NAME;
				tok->token_str[i++] = '.';
				tok->token_str[i] = 0;
				return false;
			}
			else if (var_name_too_long)
			{
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id   = TOKENID_TOKEN_TOO_LONG;
				tok->token_str[i] = 0;
				fseek(openfile, last_filepos, SEEK_SET);
				return false;
			}
			tok->token_str[i] = 0;
			fseek(openfile, last_filepos, SEEK_SET);
			get_function_information(tok);
			if (c == '(')  /*getfuncinfo() correctly filled structure*/
			{
				if (tok->token_type == TOKENTYPE_NONE)
				{
					return false;
				}
				else if (tok->token_type == TOKENTYPE_FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4))
				{
					tok->token_type = TOKENTYPE_NONE;
					tok->token_id   = TOKENID_JUMP_WITH_ILLEGAL_CHAR;
					return false;
				}
				else
				{
					return true;
				}
			}
			/*can't use function names as variables*/
			else if (tok->token_type == TOKENTYPE_FUNCTION || tok->token_type == TOKENTYPE_PARAMETER_FUNCTION)
			{
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id   = TOKENID_FUNC_USED_AS_VAR;
				return false;
			}
			else if (tok->token_type == TOKENTYPE_FLOW_CONTROL && (tok->token_id == 1 || tok->token_id == 2))
			{
				tok->token_type = TOKENTYPE_NONE;
				tok->token_id   = TOKENID_JUMP_MISSING_BOOLEAN;
				return false;
			}
			else if (tok->token_type == TOKENTYPE_FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4))
			{
				if (c == ',' || c == '\n' || c == ':')
				{
					return true;
				}
				else
				{
					tok->token_type = TOKENTYPE_NONE;
					tok->token_id   = TOKENID_JUMP_WITH_ILLEGAL_CHAR;
					return false;
				}
			}
			else
			{
				get_variable_information(tok);
				return true;
			}
		}
	}
	tok->token_str[0] = 0;
	tok->token_type = TOKENTYPE_NONE;
	tok->token_id   = TOKENID_END_OF_FILE;
	return false;
}

static void formula_get_end_of_string(FILE *openfile, token_st *this_token)
{
	long last_filepos = ftell(openfile);
	int c;

	for (c = formula_get_char(openfile); (c == '\n' || c == ',' || c == ':'); c = formula_get_char(openfile))
	{
		if (c == ':')
		{
			this_token->token_str[0] = ':';
		}
		last_filepos = ftell(openfile);
	}
	if (c == '}')
	{
		this_token->token_str[0] = '}';
		this_token->token_type = TOKENTYPE_END_OF_FORMULA;
		this_token->token_id   = 0;
	}
	else
	{
		fseek(openfile, last_filepos, SEEK_SET);
		if (this_token->token_str[0] == '\n')
		{
			this_token->token_str[0] = ',';
		}
	}
}

/* fills token structure; returns true on success and false on
  TOKENTYPE_NONE and TOKENTYPE_END_OF_FORMULA
*/
static bool formula_get_token(FILE *openfile, token_st *this_token)
{
	long filepos;
	int i = 1;
	int c = formula_get_char(openfile);
	switch (c)
	{
	CASE_NUM: case '.':
		this_token->token_str[0] = char(c);
		return formula_get_constant(openfile, this_token);
	CASE_ALPHA: case '_':
		this_token->token_str[0] = char(c);
		return formula_get_alpha(openfile, this_token);
	CASE_TERMINATOR:
		this_token->token_type = TOKENTYPE_OPERATOR; /* this may be changed below */
		this_token->token_str[0] = char(c);
		filepos = ftell(openfile);
		if (c == '<' || c == '>' || c == '=')
		{
			c = formula_get_char(openfile);
			if (c == '=')
			{
				this_token->token_str[i++] = char(c);
			}
			else
			{
				fseek(openfile, filepos, SEEK_SET);
			}
		}
		else if (c == '!')
		{
			c = formula_get_char(openfile);
			if (c == '=')
			{
				this_token->token_str[i++] = char(c);
			}
			else
			{
				fseek(openfile, filepos, SEEK_SET);
				this_token->token_str[1] = 0;
				this_token->token_type = TOKENTYPE_NONE;
				this_token->token_id = TOKENID_ILLEGAL_OPERATOR;
				return false;
			}
		}
		else if (c == '|')
		{
			c = formula_get_char(openfile);
			if (c == '|')
			{
				this_token->token_str[i++] = char(c);
			}
			else
			{
				fseek(openfile, filepos, SEEK_SET);
			}
		}
		else if (c == '&')
		{
			c = formula_get_char(openfile);
			if (c == '&')
			{
				this_token->token_str[i++] = char(c);
			}
			else
			{
				fseek(openfile, filepos, SEEK_SET);
				this_token->token_str[1] = 0;
				this_token->token_type = TOKENTYPE_NONE;
				this_token->token_id = TOKENID_ILLEGAL_OPERATOR;
				return false;
			}
		}
		else if (this_token->token_str[0] == '}')
		{
			this_token->token_type = TOKENTYPE_END_OF_FORMULA;
			this_token->token_id   = 0;
		}
		else if (this_token->token_str[0] == '\n'
			|| this_token->token_str[0] == ','
			|| this_token->token_str[0] == ':')
		{
			formula_get_end_of_string(openfile, this_token);
		}
		else if (this_token->token_str[0] == ')')
		{
			this_token->token_type = TOKENTYPE_PARENTHESIS;
			this_token->token_id = TOKENID_CLOSE_PARENS;
		}
		else if (this_token->token_str[0] == '(')
		{
			/* the following function will set token_type to TOKENTYPE_PARENTHESIS and
				token_id to OPEN_PARENS if this is not the start of a
				complex constant */
			is_complex_constant(openfile, this_token);
			return true;
		}
		this_token->token_str[i] = 0;
		if (this_token->token_type == TOKENTYPE_OPERATOR)
		{
			for (i = 0; i < NUM_OF(s_operator_list); i++)
			{
				if (!strcmp(s_operator_list[i], this_token->token_str))
				{
					this_token->token_id = i;
				}
			}
		}
		return (this_token->token_str[0] != '}');
	case EOF: case '\032':
		this_token->token_str[0] = 0;
		this_token->token_type = TOKENTYPE_NONE;
		this_token->token_id = TOKENID_END_OF_FILE;
		return false;
	default:
		this_token->token_str[0] = char(c);
		this_token->token_str[1] = 0;
		this_token->token_type = TOKENTYPE_NONE;
		this_token->token_id = TOKENID_ILLEGAL_CHARACTER;
		return false;
	}
}

int Formula::get_parameter(const char *Name)
{
	m_uses_p1 = false;
	m_uses_p2 = false;
	m_uses_p3 = false;
	m_uses_p4 = false;
	m_uses_p5 = false;
	m_uses_is_mand = false;
	m_max_function_number = 0;

	if (g_formula_name[0] == 0)
	{
		return 0;  /*  and don't reset the pointers  */
	}

	FILE *entry_file = NULL;
	if (find_file_item(g_formula_filename, Name, &entry_file, ITEMTYPE_FORMULA))
	{
		stop_message(0, error_messages(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
		return 0;
	}

	{
		int c;
		do
		{
			c = formula_get_char(entry_file);
		}
		while (c != '{' && c != EOF && c != '\032');
		if (c != '{')
		{
			stop_message(0, error_messages(PE_UNEXPECTED_EOF));
			fclose(entry_file);
			return 0;
		}
	}

	FILE *debug_token = NULL;
	if (DEBUGMODE_DISK_MESSAGES == g_debug_mode)
	{
		debug_token = fopen("frmtokens.txt", "at");
		if (debug_token != NULL)
		{
			fprintf(debug_token, "%s\n", Name);
		}
	}
	token_st current_token;
	while (formula_get_token(entry_file, &current_token))
	{
		if (debug_token != NULL)
		{
			fprintf(debug_token, "%s\n", current_token.token_str);
			fprintf(debug_token, "token_type is %d\n", current_token.token_type);
			fprintf(debug_token, "token_id is %d\n", current_token.token_id);
			if (current_token.token_type == TOKENTYPE_REAL_CONSTANT || current_token.token_type == TOKENTYPE_COMPLEX_CONSTANT)
			{
				fprintf(debug_token, "Real value is %f\n", current_token.token_const.x);
				fprintf(debug_token, "Imag value is %f\n", current_token.token_const.y);
			}
			fprintf(debug_token, "\n");
		}
		switch (current_token.token_type)
		{
		case TOKENTYPE_PARAMETER_VARIABLE:
			if (current_token.token_id == 1)
			{
				m_uses_p1 = true;
			}
			else if (current_token.token_id == 2)
			{
				m_uses_p2 = true;
			}
			else if (current_token.token_id == 8)
			{
				m_uses_p3 = true;
			}
			else if (current_token.token_id == 13)
			{
				m_uses_is_mand = true;
			}
			else if (current_token.token_id == 17)
			{
				m_uses_p4 = true;
			}
			else if (current_token.token_id == 18)
			{
				m_uses_p5 = true;
			}
			break;
		case TOKENTYPE_PARAMETER_FUNCTION:
			if ((current_token.token_id - 10) > m_max_function_number)
			{
				m_max_function_number = current_token.token_id - 10;
			}
			break;
		}
	}
	fclose(entry_file);
	if (debug_token)
	{
		fclose(debug_token);
	}
	if (current_token.token_type != TOKENTYPE_END_OF_FORMULA)
	{
		m_uses_p1 = false;
		m_uses_p2 = false;
		m_uses_p3 = false;
		m_uses_p4 = false;
		m_uses_p5 = false;
		m_uses_is_mand = false;
		m_max_function_number = 0;
		return 0;
	}
	return 1;
}

/* check_name_and_symmetry():
	error checking to the open brace on the first line;
	return true on success
	return false if errors are found which should cause
	the formula not to be executed
*/

bool Formula::check_name_and_symmetry(FILE *open_file, bool report_bad_symmetry)
{
	long filepos = ftell(open_file);
	int c;
	int i;
	int done;
	int at_end_of_name;

	/* first, test name */
	done = at_end_of_name = i = 0;
	while (!done)
	{
		c = getc(open_file);
		switch (c)
		{
		case EOF: case '\032':
			stop_message(0, error_messages(PE_UNEXPECTED_EOF));
			return false;
		case '\r': case '\n':
			stop_message(0, error_messages(PE_NO_LEFT_BRACKET_FIRST_LINE));
			return false;
		case ' ': case '\t':
			at_end_of_name = 1;
			break;
		case '(': case '{':
			done = 1;
			break;
		default :
			if (!at_end_of_name)
			{
				i++;
			}
			break;
		}
	}

	if (i > ITEMNAMELEN)
	{
		int j;
		int k = int_strlen(error_messages(PE_FORMULA_NAME_TOO_LARGE));
		char msgbuf[100];
		strcpy(msgbuf, error_messages(PE_FORMULA_NAME_TOO_LARGE));
		strcat(msgbuf, ":\n   ");
		fseek(open_file, filepos, SEEK_SET);
		for (j = 0; j < i && j < 25; j++)
		{
			msgbuf[j + k + 2] = char(getc(open_file));
		}
		msgbuf[j + k + 2] = 0;
		stop_message(STOPMSG_FIXED_FONT, msgbuf);
		return false;
	}
	/* get symmetry */
	g_symmetry = SYMMETRY_NONE;
	if (c == '(')
	{
		char sym_buf[20];
		done = i = 0;
		while (!done)
		{
			c = getc(open_file);
			switch (c)
			{
			case EOF: case '\032':
				stop_message(0, error_messages(PE_UNEXPECTED_EOF));
				return false;
			case '\r': case '\n':
				stop_message(STOPMSG_FIXED_FONT, error_messages(PE_NO_LEFT_BRACKET_FIRST_LINE));
				return false;
			case '{':
				stop_message(STOPMSG_FIXED_FONT, error_messages(PE_NO_MATCH_RIGHT_PAREN));
				return false;
			case ' ': case '\t':
				break;
			case ')':
				done = 1;
				break;
			default :
				if (i < 19)
				{
					sym_buf[i++] = char(toupper(c));
				}
				break;
			}
		}
		sym_buf[i] = 0;
		for (i = 0; SymStr[i].s[0]; i++)
		{
			if (!stricmp(SymStr[i].s, sym_buf))
			{
				g_symmetry = SymStr[i].n;
				break;
			}
		}
		if (SymStr[i].s[0] == 0 && report_bad_symmetry)
		{
			char *msgbuf = (char *) malloc(int_strlen(error_messages(PE_INVALID_SYM_USING_NOSYM))
							+ int_strlen(sym_buf) + 6);
			strcpy(msgbuf, error_messages(PE_INVALID_SYM_USING_NOSYM));
			strcat(msgbuf, ":\n   ");
			strcat(msgbuf, sym_buf);
			stop_message(STOPMSG_FIXED_FONT, msgbuf);
			free(msgbuf);
		}
	}
	if (c != '{')
	{
		done = 0;
		while (!done)
		{
			c = getc(open_file);
			switch (c)
			{
			case EOF: case '\032':
				stop_message(STOPMSG_FIXED_FONT, error_messages(PE_UNEXPECTED_EOF));
				return false;
			case '\r': case '\n':
				stop_message(STOPMSG_FIXED_FONT, error_messages(PE_NO_LEFT_BRACKET_FIRST_LINE));
				return false;
			case '{':
				done = 1;
				break;
			default :
				break;
			}
		}
	}
	return true;
}

const char *Formula::PrepareFormula(FILE *file, bool report_bad_symmetry)
{
	/* This function sets the
	symmetry and converts a formula into a string  with no spaces,
	and one comma after each expression except where the ':' is placed
	and except the final expression in the formula. The open file passed
	as an argument is open in "rb" mode and is positioned at the first
	letter of the name of the formula to be prepared. This function
	is called from RunFormula() below.
	*/

	FILE *debug_fp = NULL;
	token_st temp_tok;
	int Done;
	long filepos = ftell(file);

	/*Test for a repeat*/
	if (!check_name_and_symmetry(file, report_bad_symmetry))
	{
		fseek(file, filepos, SEEK_SET);
		return NULL;
	}
	if (!prescan(file))
	{
		fseek(file, filepos, SEEK_SET);
		return NULL;
	}

	if (m_chars_in_formula > 8190)
	{
		fseek(file, filepos, SEEK_SET);
		return NULL;
	}

	if (DEBUGMODE_DISK_MESSAGES == g_debug_mode)
	{
		debug_fp = fopen("debugfrm.txt", "at");
		if (debug_fp != NULL)
		{
			fprintf(debug_fp, "%s\n", g_formula_name);
			if (g_symmetry != 0)
			{
				fprintf(debug_fp, "%s\n", SymStr[g_symmetry].s);
			}
		}
	}

	m_prepare_formula_text[0] = 0; /* To permit concantenation later */

	Done = 0;

	/*skip opening end-of-lines */
	while (!Done)
	{
		formula_get_token(file, &temp_tok);
		if (temp_tok.token_type == TOKENTYPE_NONE)
		{
			stop_message(STOPMSG_FIXED_FONT, "Unexpected token error in PrepareFormula\n");
			fseek(file, filepos, SEEK_SET);
			return NULL;
		}
		else if (temp_tok.token_type == TOKENTYPE_END_OF_FORMULA)
		{
			stop_message(STOPMSG_FIXED_FONT, "Formula has no executable instructions\n");
			fseek(file, filepos, SEEK_SET);
			return NULL;
		}
		if (temp_tok.token_str[0] == ',')
		{
			;
		}
		else
		{
			strcat(m_prepare_formula_text, temp_tok.token_str);
			Done = 1;
		}
	}

	Done = 0;
	while (!Done)
	{
		formula_get_token(file, &temp_tok);
		switch (temp_tok.token_type)
		{
		case TOKENTYPE_NONE:
			stop_message(STOPMSG_FIXED_FONT, "Unexpected token error in PrepareFormula\n");
			fseek(file, filepos, SEEK_SET);
			return NULL;
		case TOKENTYPE_END_OF_FORMULA:
			Done = 1;
			fseek(file, filepos, SEEK_SET);
			break;
		default:
			strcat(m_prepare_formula_text, temp_tok.token_str);
			break;
		}
	}

	if (debug_fp != NULL && m_prepare_formula_text != NULL)
	{
		fprintf(debug_fp, "   %s\n", m_prepare_formula_text);
	}
	if (debug_fp != NULL)
	{
		fclose(debug_fp);
	}

	return m_prepare_formula_text;
}

int BadFormula()
{
	/*  this is called when a formula is bad, instead of calling  */
	/*     the normal functions which will produce undefined results  */
	return 1;
}

int Formula::RunFormula(const char *Name, bool report_bad_symmetry)
{
	FILE *entry_file = NULL;

	/*  CAE changed fn 12 July 1993 to fix problem when formula not found  */

	/*  first set the pointers so they point to a fn which always returns 1  */
	g_current_fractal_specific->per_pixel = BadFormula;
	g_current_fractal_specific->orbitcalc = BadFormula;

	if (g_formula_name[0] == 0)
	{
		return 1;  /*  and don't reset the pointers  */
	}

	/* add search for FRM files in directory */
	if (find_file_item(g_formula_filename, Name, &entry_file, ITEMTYPE_FORMULA))
	{
		stop_message(0, error_messages(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
		return 1;
	}

	m_formula_text = PrepareFormula(entry_file, report_bad_symmetry);
	fclose(entry_file);

	if (m_formula_text)  /*  No errors while making string */
	{
		allocate();  /*  ParseStr() will test if this alloc worked  */
		if (ParseStr(m_formula_text, 1))
		{
			return 1;   /*  parse failed, don't change fn pointers  */
		}
		else
		{
			if (m_uses_jump && fill_jump_struct())
			{
				stop_message(0, error_messages(PE_ERROR_IN_PARSING_JUMP_STATEMENTS));
				return 1;
			}

			/* all parses succeeded so set the pointers back to good functions*/
			g_current_fractal_specific->per_pixel = form_per_pixel;
			g_current_fractal_specific->orbitcalc = formula_orbit;
			return 0;
		}
	}
	else
	{
		return 1;   /* error in making string*/
	}
}


int Formula::setup_fp()
{
	int RunFormRes;
	/* TODO: when parsera.c contains assembly equivalents, remove !defined(_WIN32) */
#if !defined(XFRACT) && !defined(_WIN32)
	MathType = D_MATH;
	/* CAE changed below for fp */
	RunFormRes = !RunFormula(g_formula_name, false); /* RunFormula() returns 1 for failure */
	if (RunFormRes && !(g_orbit_save & ORBITSAVE_SOUND) && !s_random.randomized()
		&& (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL))
	{
		CvtStk(); /* run fast assembler code in parsera.asm */
		return 1;
	}
	return RunFormRes;
#else
	m_math_type = D_MATH;
	RunFormRes = !RunFormula(g_formula_name, false); /* RunForm() returns 1 for failure */
#if 0
	if (RunFormRes && !(g_orbit_save & ORBITSAVE_SOUND) && !s_random.randomized()
		&& (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL))
	{
		CvtStk(); /* run fast assembler code in parsera.asm */
		return 1;
	}
#endif
	return RunFormRes;
#endif
}

int Formula::setup_int()
{
#if defined(XFRACT)
	return integer_unsupported();
#else
	m_math_type = L_MATH;
	s_fudge = double(1L << g_bit_shift);
	g_fudge_limit = double_from_fixpoint(0x7fffffffL);
	s_shift_back = 32 - g_bit_shift;
	return !RunFormula(g_formula_name, false);
#endif
}

void Formula::init_misc()
{
	Arg1 = &m_arg1;
	Arg2 = &m_arg2; /* needed by all the ?Stk* functions */
	s_fudge = double(1L << g_bit_shift);
	g_fudge_limit = double_from_fixpoint(0x7fffffffL);
	s_shift_back = 32 - g_bit_shift;
	s_delta16 = g_bit_shift - 16;
	g_bit_shift_minus_1 = g_bit_shift-1;
	m_uses_p1 = false;
	m_uses_p2 = false;
	m_uses_p3 = false;
	m_uses_p4 = false;
	m_uses_p5 = false;
	m_uses_is_mand = false;
	m_uses_jump = false;
}


/*
		Allocate sub-arrays from one main malloc, using global variable
		g_type_specific_work_area; calcfrac.c releases this area when calculation
		ends or is terminated.
		Moved the "f" array to be allocated as part of this.
*/

void Formula::allocate()
{
	/* TW Jan 1 1996 Made two passes to determine actual values of
		m_formula_max_ops and m_formula_max_args. */
	for (int pass = 0; pass < 2; pass++)
	{
		free_work_area();
		if (pass == 0)
		{
			m_formula_max_ops = 2300; /* this value uses up about 64K memory */
			m_formula_max_args = (unsigned) (m_formula_max_ops/2.5);
		}

		m_functions = new t_function_pointer[m_formula_max_ops];
		m_store = new Arg *[MAX_STORES];
		m_load = new Arg *[MAX_LOADS];
		m_variables = new ConstArg[m_formula_max_args];
		m_function_load_store_pointers = new function_load_store[m_formula_max_ops];

		if (pass == 0)
		{
			if (!ParseStr(m_formula_text, pass))
			{
				/* per Chuck Ebbert, fudge these up a little */
				m_formula_max_ops = m_posp + 4;
				m_formula_max_args = m_parser_vsp + 4;
			}
		}
	}
	m_uses_p1 = false;
	m_uses_p2 = false;
	m_uses_p3 = false;
	m_uses_p4 = false;
	m_uses_p5 = false;
#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif
}

void free_work_area()
{
	g_formula_state.free_work_area();
}

void Formula::free_work_area()
{
	delete[] m_functions;
	delete[] m_store;
	delete[] m_load;
	delete[] m_variables;
	delete[] m_function_load_store_pointers;

	m_functions = NULL;
	m_store = NULL;
	m_load = NULL;
	m_variables = NULL;
	m_function_load_store_pointers = NULL;
}


void Formula::frm_error(FILE *open_file, long begin_frm)
{
	char msgbuf[900];
	strcpy(msgbuf, "\n");

	token_st tok;
	int token_count;
	int chars_to_error = 0;
	int chars_in_error = 0;
	for (int j = 0; j < NUM_OF(m_errors) && m_errors[j].start_pos; j++)
	{
		int initialization_error = (m_errors[j].error_number == PE_SECOND_COLON) ? 1 : 0;
		fseek(open_file, begin_frm, SEEK_SET);
		int i;
		int line_number = 1;
		while (ftell(open_file) != m_errors[j].error_pos)
		{
			i = fgetc(open_file);
			if (i == '\n')
			{
				line_number++;
			}
			else if (i == EOF || i == '}')
			{
				stop_message(0, "Unexpected EOF or end-of-formula in error function.\n");
				fseek(open_file, m_errors[j].error_pos, SEEK_SET);
				formula_get_token(open_file, &tok); /*reset file to end of error token */
				return;
			}
		}
		sprintf(&msgbuf[int_strlen(msgbuf)], "Error(%d) at line %d:  %s\n  ",
			m_errors[j].error_number, line_number, error_messages(m_errors[j].error_number));
		i = int_strlen(msgbuf);
		fseek(open_file, m_errors[j].start_pos, SEEK_SET);
		int statement_len = 0;
		token_count = 0;
		bool done = false;
		while (!done)
		{
			long filepos = ftell(open_file);
			if (filepos == m_errors[j].error_pos)
			{
				chars_to_error = statement_len;
				formula_get_token(open_file, &tok);
				chars_in_error = int_strlen(tok.token_str);
				statement_len += chars_in_error;
				token_count++;
			}
			else
			{
				formula_get_token(open_file, &tok);
				statement_len += int_strlen(tok.token_str);
				token_count++;
			}
			if ((tok.token_type == TOKENTYPE_END_OF_FORMULA)
				|| (tok.token_type == TOKENTYPE_OPERATOR
					&& (tok.token_id == 0 || tok.token_id == 11))
				|| (tok.token_type == TOKENTYPE_NONE && tok.token_id == TOKENID_END_OF_FILE))
			{
				done = true;
				if (token_count > 1 && !initialization_error)
				{
					token_count--;
				}
			}
		}
		fseek(open_file, m_errors[j].start_pos, SEEK_SET);
		if (chars_in_error < 74)
		{
			while (chars_to_error + chars_in_error > 74)
			{
				formula_get_token(open_file, &tok);
				chars_to_error -= int_strlen(tok.token_str);
				token_count--;
			}
		}
		else
		{
			fseek(open_file, m_errors[j].error_pos, SEEK_SET);
			chars_to_error = 0;
			token_count = 1;
		}
		while (int_strlen(&msgbuf[i]) <= 74 && token_count--)
		{
			formula_get_token(open_file, &tok);
			strcat(msgbuf, tok.token_str);
		}
		fseek(open_file, m_errors[j].error_pos, SEEK_SET);
		formula_get_token(open_file, &tok);
		if (int_strlen(&msgbuf[i]) > 74)
		{
			msgbuf[i + 74] = 0;
		}
		strcat(msgbuf, "\n");
		i = int_strlen(msgbuf);
		while (chars_to_error-- > -2)
		{
			strcat(msgbuf, " ");
		}
		if (m_errors[j].error_number == PE_TOKEN_TOO_LONG)
		{
			chars_in_error = 33;
		}
		while (chars_in_error-- && int_strlen(&msgbuf[i]) <= 74)
		{
			strcat(msgbuf, "^");
		}
		strcat(msgbuf, "\n");
	}
	stop_message(8, msgbuf);
	return;
}

void Formula::display_var_list()
{
	if (m_variable_list)
	{
		m_variable_list->display();
	}
}

void var_list_st::display() const
{
	stop_message(0, "List of user defined variables:\n");
	for (const var_list_st *p = this; p; p = p->next_item)
	{
		stop_message(0, p->name);
	}
}

void Formula::display_const_lists()
{
	if (m_complex_list)
	{
		m_complex_list->display("Complex constants are:");
	}
	if (m_real_list)
	{
		m_real_list->display("Real constants are:");
	}
}

void const_list_st::display(const char *title) const
{
	char msgbuf[800];
	stop_message(0, title);
	for (const const_list_st *p = this; p; p = p->next_item)
	{
		sprintf(msgbuf, "%f, %f\n", p->complex_const.x, p->complex_const.y);
		stop_message(0, msgbuf);
	}
}

var_list_st *var_list_alloc()
{
	return (var_list_st *) malloc(sizeof(var_list_st));
}


const_list_st *const_list_alloc()
{
	return (const_list_st *) malloc(sizeof(const_list_st));
}

void Formula::init_var_list()
{
	var_list_st *temp, *p;
	for (p = m_variable_list; p; p = temp)
	{
		temp = p->next_item;
		free(p);
	}
	m_variable_list = NULL;
}


void Formula::init_const_lists()
{
	const_list_st *temp, *p;
	for (p = m_complex_list; p; p = temp)
	{
		temp = p->next_item;
		free(p);
	}
	m_complex_list = NULL;
	for (p = m_real_list; p; p = temp)
	{
		temp = p->next_item;
		free(p);
	}
	m_real_list = NULL;
}

var_list_st *var_list_st::add(var_list_st *p, token_st tok)
{
	if (p == NULL)
	{
		p = var_list_alloc();
		if (p == NULL)
		{
			return NULL;
		}
		strcpy(p->name, tok.token_str);
		p->next_item = NULL;
	}
	else if (strcmp(p->name, tok.token_str) == 0)
	{
	}
	else
	{
		p->next_item = add(p->next_item, tok);
		if (p->next_item == NULL)
		{
			return NULL;
		}
	}
	return p;
}

const_list_st *const_list_st::add(const_list_st *p, token_st tok)
{
	if (p == NULL)
	{
		p = const_list_alloc();
		if (p == NULL)
		{
			return NULL;
		}
		p->complex_const.x = tok.token_const.x;
		p->complex_const.y = tok.token_const.y;
		p->next_item = NULL;
	}
	else if (p->complex_const.x != tok.token_const.x
			 || p->complex_const.y != tok.token_const.y)
	{
		p->next_item = add(p->next_item, tok);
		if (p->next_item == NULL)
		{
			return NULL;
		}
	}
	return p;
}

void Formula::count_lists()
{
	var_list_st *p;
	const_list_st *q;

	m_variable_count = 0;
	m_complex_count = 0;
	m_real_count = 0;

	for (p = m_variable_list; p; p = p->next_item)
	{
		m_variable_count++;
	}
	for (q = m_complex_list; q; q = q->next_item)
	{
		m_complex_count++;
	}
	for (q = m_real_list; q; q = q->next_item)
	{
		m_real_count++;
	}
}



/*Formula::prescan() takes an open file with the file pointer positioned at
the beginning of the relevant formula, and parses the formula, token
by token, for syntax errors. The function also accumulates data for
memory allocation to be done later.

The function returns 1 if success, and 0 if errors are found.
*/

void Formula::record_error(int error_code)
{
	if (!m_errors_found || m_errors[m_errors_found - 1].start_pos != m_statement_pos)
	{
		m_errors[m_errors_found].start_pos      = m_statement_pos;
		m_errors[m_errors_found].error_pos      = m_file_pos;
		m_errors[m_errors_found++].error_number = error_code;
	}
}

bool Formula::prescan(FILE *open_file)
{
	int i;
	long orig_pos;
	int done = 0;
	token_st this_token;
	m_errors_found = 0;
	bool ExpectingArg = true;
	int NewStatement = true;
	int assignment_ok = true;
	int already_got_colon = false;
	unsigned long else_has_been_used = 0;
	unsigned long waiting_for_mod = 0;
	int waiting_for_endif = 0;
	int max_parens = sizeof(long)*8;

	m_number_of_ops = 0;
	m_number_of_loads = 0;
	m_number_of_stores = 0;
	m_number_of_jumps = 0;
	m_chars_in_formula = (unsigned) 0;
	m_uses_jump = false;
	m_parenthesis_count = 0;

	init_var_list();
	init_const_lists();

	orig_pos = m_statement_pos = ftell(open_file);
	for (i = 0; i < 3; i++)
	{
		m_errors[i].start_pos    = 0L;
		m_errors[i].error_pos    = 0L;
		m_errors[i].error_number = 0;
	}

	while (!done)
	{
		m_file_pos = ftell(open_file);
		formula_get_token(open_file, &this_token);
		m_chars_in_formula += int_strlen(this_token.token_str);
		switch (this_token.token_type)
		{
		case TOKENTYPE_NONE:
			assignment_ok = false;
			switch (this_token.token_id)
			{
			case TOKENID_END_OF_FILE:
				stop_message(0, error_messages(PE_UNEXPECTED_EOF));
				fseek(open_file, orig_pos, SEEK_SET);
				return false;
			case TOKENID_ILLEGAL_CHARACTER:
				record_error(PE_ILLEGAL_CHAR);
				break;
			case TOKENID_ILLEGAL_VARIABLE_NAME:
				record_error(PE_ILLEGAL_VAR_NAME);
				break;
			case TOKENID_TOKEN_TOO_LONG:
				record_error(PE_TOKEN_TOO_LONG);
				break;
			case TOKENID_FUNC_USED_AS_VAR:
				record_error(PE_FUNC_USED_AS_VAR);
				break;
			case TOKENID_JUMP_MISSING_BOOLEAN:
				record_error(PE_JUMP_NEEDS_BOOLEAN);
				break;
			case TOKENID_JUMP_WITH_ILLEGAL_CHAR:
				record_error(PE_NO_CHAR_AFTER_THIS_JUMP);
				break;
			case TOKENID_UNDEFINED_FUNCTION:
				record_error(PE_UNDEFINED_FUNCTION);
				break;
			case TOKENID_ILLEGAL_OPERATOR:
				record_error(PE_UNDEFINED_OPERATOR);
				break;
			case TOKENID_ILL_FORMED_CONSTANT:
				record_error(PE_INVALID_CONST);
				break;
			default:
				stop_message(0, "Unexpected arrival at default case in prescan()");
				fseek(open_file, orig_pos, SEEK_SET);
				return false;
			}
			break;
		case TOKENTYPE_PARENTHESIS:
			assignment_ok = false;
			NewStatement = false;
			switch (this_token.token_id)
			{
			case TOKENID_OPEN_PARENS:
				if (++m_parenthesis_count > max_parens)
				{
					record_error(PE_NESTING_TOO_DEEP);
				}
				else if (!ExpectingArg)
				{
					record_error(PE_SHOULD_BE_OPERATOR);
				}
				waiting_for_mod <<= 1;
				break;
			case TOKENID_CLOSE_PARENS:
				if (m_parenthesis_count)
				{
					m_parenthesis_count--;
				}
				else
				{
					record_error(PE_NEED_A_MATCHING_OPEN_PARENS);
					m_parenthesis_count = 0;
				}
				if (waiting_for_mod & 1L)
				{
					record_error(PE_UNMATCHED_MODULUS);
				}
				else
				{
					waiting_for_mod >>= 1;
				}
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				break;
			default:
				break;
			}
			break;
		case TOKENTYPE_PARAMETER_VARIABLE: /*i.e. p1, p2, p3, p4 or p5*/
			m_number_of_ops++;
			m_number_of_loads++;
			NewStatement = false;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			ExpectingArg = false;
			break;
		case TOKENTYPE_USER_VARIABLE: /* i.e. c, iter, etc. */
			m_number_of_ops++;
			m_number_of_loads++;
			NewStatement = false;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			ExpectingArg = false;
			m_variable_list = var_list_st::add(m_variable_list, this_token);
			if (m_variable_list == NULL)
			{
				stop_message(0, error_messages(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
				fseek(open_file, orig_pos, SEEK_SET);
				init_var_list();
				init_const_lists();
				return false;
			}
			break;
		case TOKENTYPE_PREDEFINED_VARIABLE: /* i.e. z, pixel, whitesq, etc. */
			m_number_of_ops++;
			m_number_of_loads++;
			NewStatement = false;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			ExpectingArg = false;
			break;
		case TOKENTYPE_REAL_CONSTANT: /* i.e. 4, (4,0), etc.) */
			assignment_ok = false;
			m_number_of_ops++;
			m_number_of_loads++;
			NewStatement = false;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			ExpectingArg = false;
			m_real_list = const_list_st::add(m_real_list, this_token);
			if (m_real_list == NULL)
			{
				stop_message(0, error_messages(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
				fseek(open_file, orig_pos, SEEK_SET);
				init_var_list();
				init_const_lists();
				return false;
			}
			break;
		case TOKENTYPE_COMPLEX_CONSTANT: /* i.e. (1,2) etc. */
			assignment_ok = false;
			m_number_of_ops++;
			m_number_of_loads++;
			NewStatement = false;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			ExpectingArg = false;
			m_complex_list = const_list_st::add(m_complex_list, this_token);
			if (m_complex_list == NULL)
			{
				stop_message(0, error_messages(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
				fseek(open_file, orig_pos, SEEK_SET);
				init_var_list();
				init_const_lists();
				return false;
			}
			break;
		case TOKENTYPE_FUNCTION:
			assignment_ok = false;
			NewStatement = false;
			m_number_of_ops++;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			break;
		case TOKENTYPE_PARAMETER_FUNCTION:
			assignment_ok = false;
			NewStatement = false;
			m_number_of_ops++;
			if (!ExpectingArg)
			{
				record_error(PE_SHOULD_BE_OPERATOR);
			}
			NewStatement = false;
			break;
		case TOKENTYPE_FLOW_CONTROL:
			assignment_ok = false;
			m_number_of_ops++;
			m_number_of_jumps++;
			if (!NewStatement)
			{
				record_error(PE_JUMP_NOT_FIRST);
			}
			else
			{
				m_uses_jump = true;
				switch (this_token.token_id)
				{
				case 1:  /* if */
					else_has_been_used <<= 1;
					waiting_for_endif++;
					break;
				case 2: /*ELSEIF*/
					m_number_of_ops += 3; /*else + two clear statements*/
					m_number_of_jumps++;  /* this involves two jumps */
					if (else_has_been_used & 1)
					{
						record_error(PE_ENDIF_REQUIRED_AFTER_ELSE);
					}
					else if (!waiting_for_endif)
					{
						record_error(PE_MISPLACED_ELSE_OR_ELSEIF);
					}
					break;
				case 3: /*ELSE*/
					if (else_has_been_used & 1)
					{
						record_error(PE_ENDIF_REQUIRED_AFTER_ELSE);
					}
					else if (!waiting_for_endif)
					{
						record_error(PE_MISPLACED_ELSE_OR_ELSEIF);
					}
					else_has_been_used |= 1;
					break;
				case 4: /*ENDIF*/
					else_has_been_used >>= 1;
					waiting_for_endif--;
					if (waiting_for_endif < 0)
					{
						record_error(PE_ENDIF_WITH_NO_IF);
						waiting_for_endif = 0;
					}
					break;
				default:
					break;
				}
			}
			break;
		case TOKENTYPE_OPERATOR:
			m_number_of_ops++; /*This will be corrected below in certain cases*/
			switch (this_token.token_id)
			{
			case 0: case 11:    /* end of statement and : */
				m_number_of_ops++; /* ParseStr inserts a dummy op*/
				if (m_parenthesis_count)
				{
					record_error(PE_NEED_MORE_CLOSE_PARENS);
					m_parenthesis_count = 0;
				}
				if (waiting_for_mod)
				{
					record_error(PE_UNMATCHED_MODULUS);
					waiting_for_mod = 0;
				}
				if (!ExpectingArg)
				{
					if (this_token.token_id == 11)
					{
						m_number_of_ops += 2;
					}
					else
					{
						m_number_of_ops++;
					}
				}
				else if (!NewStatement)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				if (this_token.token_id == 11 && waiting_for_endif)
				{
					record_error(PE_UNMATCHED_IF_IN_INIT_SECTION);
					waiting_for_endif = 0;
				}
				if (this_token.token_id == 11 && already_got_colon)
				{
					record_error(PE_SECOND_COLON);
				}
				if (this_token.token_id == 11)
				{
					already_got_colon = true;
				}
				NewStatement = true;
				assignment_ok = true;
				ExpectingArg = true;
				m_statement_pos = ftell(open_file);
				break;
			case 1:     /* != */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 2:     /* = */
				m_number_of_ops--; /*this just converts a load to a store*/
				m_number_of_loads--;
				m_number_of_stores++;
				if (!assignment_ok)
				{
					record_error(PE_ILLEGAL_ASSIGNMENT);
				}
				ExpectingArg = true;
				break;
			case 3:     /* == */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 4:     /* < */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 5:     /* <= */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 6:     /* > */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 7:     /* >= */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 8:     /* | */ /* (half of the modulus operator */
				assignment_ok = false;
				if (!waiting_for_mod & 1L)
				{
					m_number_of_ops--;
				}
				if (!(waiting_for_mod & 1L) && !ExpectingArg)
				{
					record_error(PE_SHOULD_BE_OPERATOR);
				}
				else if ((waiting_for_mod & 1L) && ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				waiting_for_mod ^= 1L; /*switch right bit*/
				break;
			case 9:     /* || */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 10:    /* && */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 12:    /* + */ /* case 11 (":") is up with case 0 */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 13:    /* - */
				assignment_ok = false;
				ExpectingArg = true;
				break;
			case 14:    /* * */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 15:    /* / */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				ExpectingArg = true;
				break;
			case 16:    /* ^ */
				assignment_ok = false;
				if (ExpectingArg)
				{
					record_error(PE_SHOULD_BE_ARGUMENT);
				}
				m_file_pos = ftell(open_file);
				formula_get_token (open_file, &this_token);
				if (this_token.token_str[0] == '-')
				{
					record_error(PE_NO_NEG_AFTER_EXPONENT);
				}
				else
				{
					fseek(open_file, m_file_pos, SEEK_SET);
				}
				ExpectingArg = true;
				break;
			default:
				break;
			}
			break;
		case TOKENTYPE_END_OF_FORMULA:
			m_number_of_ops += 3; /* Just need one, but a couple of extra just for the heck of it */
			if (m_parenthesis_count)
			{
				record_error(PE_NEED_MORE_CLOSE_PARENS);
				m_parenthesis_count = 0;
			}
			if (waiting_for_mod)
			{
				record_error(PE_UNMATCHED_MODULUS);
				waiting_for_mod = 0;
			}
			if (waiting_for_endif)
			{
				record_error(PE_IF_WITH_NO_ENDIF);
				waiting_for_endif = 0;
			}
			if (ExpectingArg && !NewStatement)
			{
				record_error(PE_SHOULD_BE_ARGUMENT);
				m_statement_pos = ftell(open_file);
			}

			if (m_number_of_jumps >= MAX_JUMPS)
			{
				record_error(PE_TOO_MANY_JUMPS);
			}
			done = 1;
			break;

		default:
			break;
		}
		if (m_errors_found == NUM_OF(m_errors))
		{
			done = 1;
		}
	}
	if (m_errors[0].start_pos)
	{
		frm_error(open_file, orig_pos);
		fseek(open_file, orig_pos, SEEK_SET);
		return false;
	}
	fseek(open_file, orig_pos, SEEK_SET);

	count_lists();

	return true;
}

const char *Formula::info_line1() const
{
	static char buffer[80];
	std::ostringstream text;
	text << " MaxOps (posp) " << m_posp
		<< " MaxArgs (vsp) " << m_parser_vsp
		<< std::ends;
	::strncpy(buffer, text.str().c_str(), 79);
	buffer[79] = 0;
	return &buffer[0];
}

const char *Formula::info_line2() const
{
	static char buffer[80];
	std::ostringstream text;
	text << "   Store ptr " << m_store_ptr
		<< " Loadptr " << m_load_ptr
		<< " MaxOps var " << m_formula_max_ops
		<< " MaxArgs var " << m_formula_max_args
		<< " LastInitOp " << m_last_init_op
		<< std::ends;
	::strncpy(buffer, text.str().c_str(), 79);
	buffer[79] = 0;
	return &buffer[0];
}

// TODO: remove these glue functions when everything has been refactored to objects
void EndInit()
{
	g_formula_state.end_init();
}

int formula_orbit()
{
	return g_formula_state.orbit();
}

int form_per_pixel()
{
	return g_formula_state.per_pixel();
}

int formula_setup_fp()
{
	return g_formula_state.setup_fp();
}

int formula_setup_int()
{
	return g_formula_state.setup_int();
}

void dStkLodDup()
{
	g_formula_state.StackLoadDup_d();
}
