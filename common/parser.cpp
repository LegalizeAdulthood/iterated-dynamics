/* Parser.c (C) 1990, Mark C. Peterson, CompuServe [70441,3353]
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
#include <string>

#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "drivers.h"

#ifdef WATCH_MP
double x1, y1, x2, y2;
#endif

enum MATH_TYPE MathType = D_MATH;

#define MAX_OPS 250
#define MAX_ARGS 100

unsigned Max_Ops  = MAX_OPS;
unsigned Max_Args = MAX_ARGS;

unsigned long number_of_ops, number_of_loads, number_of_stores, number_of_jumps;

struct PEND_OP
{
    void (*f)();
    int p;
};

JUMP_CONTROL_ST jump_control[MAX_JUMPS];
int jump_index;

int InitJumpIndex;

static bool frm_prescan(FILE * open_file);

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

/* token_type definitions */
#define NOT_A_TOKEN           0
#define PARENS                1
#define PARAM_VARIABLE        2
#define USER_NAMED_VARIABLE   3
#define PREDEFINED_VARIABLE   4
#define REAL_CONSTANT         5
#define COMPLEX_CONSTANT      6
#define FUNCTION              7
#define PARAM_FUNCTION        8
#define FLOW_CONTROL          9
#define OPERATOR             10
#define END_OF_FORMULA       11

/* token IDs */
#define END_OF_FILE            1
#define ILLEGAL_CHARACTER      2
#define ILLEGAL_VARIABLE_NAME  3
#define TOKEN_TOO_LONG         4
#define FUNC_USED_AS_VAR       5
#define JUMP_MISSING_BOOLEAN   6
#define JUMP_WITH_ILLEGAL_CHAR 7
#define UNDEFINED_FUNCTION     8
#define ILLEGAL_OPERATOR       9
#define ILL_FORMED_CONSTANT   10
#define OPEN_PARENS            1
#define CLOSE_PARENS          -1

struct token_st
{
    char token_str[80];
    int token_type;
    int token_id;
    DComplex token_const;
};


/* MAX_STORES must be even to make Unix alignment work */

#define MAX_STORES ((Max_Ops/4)*2)  /* at most only half the ops can be stores */
#define MAX_LOADS  ((unsigned)(Max_Ops*.8))  /* and 80% can be loads */

static struct PEND_OP o[2300];

struct var_list_st
{
    char name[34];
    struct var_list_st * next_item;
} * var_list;

struct const_list_st
{
    DComplex complex_const;
    struct const_list_st * next_item;
} * complx_list, * real_list;

static void parser_allocate();

union Arg *Arg1, *Arg2;

/* Some of these variables should be renamed for safety */
union Arg s[20];
union Arg **Store;
union Arg **Load;
int OpPtr;
void (**f)() = nullptr;
struct ConstArg *v = nullptr;
int StoPtr, LodPtr;
int var_count;
int complx_count;
int real_count;



bool ismand = true;

unsigned int posp, vsp, LastOp;
static unsigned int n, NextOp, InitN;
static int paren;
static bool ExpectingArg = false;
int InitLodPtr, InitStoPtr, InitOpPtr, LastInitOp;
static int Delta16;
double fgLimit;
static double fg;
static int ShiftBack;
static bool SetRandom = false;
static bool Randomized = false;
static unsigned long RandNum;
bool uses_p1 = false;
bool uses_p2 = false;
bool uses_p3 = false;
bool uses_p4 = false;
bool uses_p5 = false;
bool uses_jump = false;
bool uses_ismand = false;
unsigned int chars_in_formula;

#if !defined(XFRACT)
#define ChkLongDenom(denom)                                 \
    if ((denom == 0 || overflow) && save_release > 1920)    \
    {                                                       \
        overflow = true;                                    \
        return;                                             \
    }                                                       \
    else if (denom == 0)                                    \
        return
#endif

#define ChkFloatDenom(denom)                                \
    if (fabs(denom) <= DBL_MIN)                             \
    {                                                       \
        if (save_release > 1920)                            \
            overflow = true;                                \
        return;                                             \
    }

#define LastSqr v[4].a

/* ParseErrs() defines; all calls to ParseErrs(), or any variable which will
   be used as the argument in a call to ParseErrs(), should use one of these
   defines.
*/

#define PE_NO_ERRORS_FOUND                           -1
#define PE_SHOULD_BE_ARGUMENT                         0
#define PE_SHOULD_BE_OPERATOR                         1
#define PE_NEED_A_MATCHING_OPEN_PARENS                2
#define PE_NEED_MORE_CLOSE_PARENS                     3
#define PE_UNDEFINED_OPERATOR                         4
#define PE_UNDEFINED_FUNCTION                         5
#define PE_TABLE_OVERFLOW                             6
#define PE_NO_MATCH_RIGHT_PAREN                       7
#define PE_NO_LEFT_BRACKET_FIRST_LINE                 8
#define PE_UNEXPECTED_EOF                             9
#define PE_INVALID_SYM_USING_NOSYM                   10
#define PE_FORMULA_TOO_LARGE                         11
#define PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA         12
#define PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED 13
#define PE_JUMP_NOT_FIRST                            14
#define PE_NO_CHAR_AFTER_THIS_JUMP                   15
#define PE_JUMP_NEEDS_BOOLEAN                        16
#define PE_ENDIF_REQUIRED_AFTER_ELSE                 17
#define PE_ENDIF_WITH_NO_IF                          18
#define PE_MISPLACED_ELSE_OR_ELSEIF                  19
#define PE_UNMATCHED_IF_IN_INIT_SECTION              20
#define PE_IF_WITH_NO_ENDIF                          21
#define PE_ERROR_IN_PARSING_JUMP_STATEMENTS          22
#define PE_TOO_MANY_JUMPS                            23
#define PE_FORMULA_NAME_TOO_LARGE                    24
#define PE_ILLEGAL_ASSIGNMENT                        25
#define PE_ILLEGAL_VAR_NAME                          26
#define PE_INVALID_CONST                             27
#define PE_ILLEGAL_CHAR                              28
#define PE_NESTING_TO_DEEP                           29
#define PE_UNMATCHED_MODULUS                         30
#define PE_FUNC_USED_AS_VAR                          31
#define PE_NO_NEG_AFTER_EXPONENT                     32
#define PE_TOKEN_TOO_LONG                            33
#define PE_SECOND_COLON                              34
#define PE_INVALID_CALL_TO_PARSEERRS                 35

static const char *ParseErrs(int which)
{
    int lasterr;
    static char e0[] = {"Should be an Argument"};
    static char e1[] = {"Should be an Operator"};
    static char e2[] = {"')' needs a matching '('"};
    static char e3[] = {"Need more ')'"};
    static char e4[] = {"Undefined Operator"};
    static char e5[] = {"Undefined Function"};
    static char e6[] = {"Table overflow"};
    static char e7[] = {"Didn't find matching ')' in symmetry declaration"};
    static char e8[] = {"No '{' found on first line"};
    static char e9[] = {"Unexpected EOF!"};
    static char e10[] = {"Symmetry below is invalid, will use NOSYM"};
    static char e11[] = {"Formula is too large"};
    static char e12[] = {"Insufficient memory to run fractal type 'formula'"};
    static char e13[] = {"Could not open file where formula located"};
    static char e14[] = {"No characters may precede jump instruction"};
    static char e15[] = {"No characters may follow this jump instruction"};
    static char e16[] = {"Jump instruction missing required (boolean argument)"};
    static char e17[] = {"Next jump after \"else\" must be \"endif\""};
    static char e18[] = {"\"endif\" has no matching \"if\""};
    static char e19[] = {"Misplaced \"else\" or \"elseif()\""};
    static char e20[] = {"\"if ()\" in initialization has no matching \"endif\""};
    static char e21[] = {"\"if ()\" has no matching \"endif\""};
    static char e22[] = {"Error in parsing jump statements"};
    static char e23[] = {"Formula has too many jump commands"};
    static char e24[] = {"Formula name has too many characters"};
    static char e25[] = {"Only variables are allowed to left of assignment"};
    static char e26[] = {"Illegal variable name"};
    static char e27[] = {"Invalid constant expression"};
    static char e28[] = {"This character not supported by parser"};
    static char e29[] = {"Nesting of parentheses exceeds maximum depth"};
    static char e30[] = {"Unmatched modulus operator \"|\" in this expression"}; /*last one */
    static char e31[] = {"Can't use function name as variable"};
    static char e32[] = {"Negative exponent must be enclosed in parens"};
    static char e33[] = {"Variable or constant exceeds 32 character limit"};
    static char e34[] = {"Only one \":\" permitted in a formula"};
    static char e35[] = {"Invalid ParseErrs code"};
    static const char *ErrStrings[] =
    {
        e0,e1,e2,e3,e4,e5,
        e6,e7,e8,e9,e10,
        e11,e12,e13,e14,e15,
        e16,e17,e18,e19,e20,
        e21,e22,e23,e24,e25,
        e26, e27, e28, e29, e30,
        e31, e32, e33, e34, e35
    };
    lasterr = sizeof(ErrStrings)/sizeof(ErrStrings[0]) - 1;
    if (which > lasterr)
        which = lasterr;
    return (char *)ErrStrings[which];
}

/* use the following when only float functions are implemented to
   get MP math and Integer math */

#if !defined(XFRACT)

static void mStkFunct(void (*fct)())   /* call mStk via dStk */
{
    Arg1->d = MPC2cmplx(Arg1->m);
    (*fct)();
    Arg1->m = cmplx2MPC(Arg1->d);
}

static void lStkFunct(void (*fct)())   /* call lStk via dStk */
{
    double y;
    /*
       intermediate variable needed for safety because of
       different size of double and long in Arg union
    */
    y = (double)Arg1->l.y / fg;
    Arg1->d.x = (double)Arg1->l.x / fg;
    Arg1->d.y = y;
    (*fct)();
    if (fabs(Arg1->d.x) < fgLimit && fabs(Arg1->d.y) < fgLimit)
    {
        Arg1->l.x = (long)(Arg1->d.x * fg);
        Arg1->l.y = (long)(Arg1->d.y * fg);
    }
    else
        overflow = true;
}

#endif

unsigned long NewRandNum()
{
    return RandNum = ((RandNum << 15) + rand15()) ^ RandNum;
}

void lRandom()
{
    v[7].a.l.x = NewRandNum() >> (32 - bitshift);
    v[7].a.l.y = NewRandNum() >> (32 - bitshift);
}

void dRandom()
{
    long x, y;

    /* Use the same algorithm as for fixed math so that they will generate
           the same fractals when the srand() function is used. */
    x = NewRandNum() >> (32 - bitshift);
    y = NewRandNum() >> (32 - bitshift);
    v[7].a.d.x = ((double)x / (1L << bitshift));
    v[7].a.d.y = ((double)y / (1L << bitshift));

}

#if !defined(XFRACT)
void mRandom()
{
    long x, y;

    /* Use the same algorithm as for fixed math so that they will generate
       the same fractals when the srand() function is used. */
    x = NewRandNum() >> (32 - bitshift);
    y = NewRandNum() >> (32 - bitshift);
    v[7].a.m.x = *fg2MP(x, bitshift);
    v[7].a.m.y = *fg2MP(y, bitshift);
}
#endif

void SetRandFnct()
{
    unsigned Seed;

    if (!SetRandom)
        RandNum = Arg1->l.x ^ Arg1->l.y;

    Seed = (unsigned)RandNum ^ (unsigned)(RandNum >> 16);
    srand(Seed);
    SetRandom = true;

    /* Clear out the seed */
    NewRandNum();
    NewRandNum();
    NewRandNum();
}

void RandomSeed()
{
    time_t ltime;

    /* Use the current time to randomize the random number sequence. */
    time(&ltime);
    srand((unsigned int)ltime);

    NewRandNum();
    NewRandNum();
    NewRandNum();
    Randomized = true;
}

#if !defined(XFRACT)
void lStkSRand()
{
    SetRandFnct();
    lRandom();
    Arg1->l = v[7].a.l;
}
#endif

#if !defined(XFRACT)
void mStkSRand()
{
    Arg1->l.x = Arg1->m.x.Mant ^ (long)Arg1->m.x.Exp;
    Arg1->l.y = Arg1->m.y.Mant ^ (long)Arg1->m.y.Exp;
    SetRandFnct();
    mRandom();
    Arg1->m = v[7].a.m;
}
#endif

void dStkSRand()
{
    Arg1->l.x = (long)(Arg1->d.x * (1L << bitshift));
    Arg1->l.y = (long)(Arg1->d.y * (1L << bitshift));
    SetRandFnct();
    dRandom();
    Arg1->d = v[7].a.d;
}

void (*StkSRand)() = dStkSRand;


void dStkLodDup()
{
    Arg1+=2;
    Arg2+=2;
    *Arg1 = *Load[LodPtr];
    *Arg2 = *Arg1;
    LodPtr+=2;
}

void dStkLodSqr()
{
    Arg1++;
    Arg2++;
    Arg1->d.y = Load[LodPtr]->d.x * Load[LodPtr]->d.y * 2.0;
    Arg1->d.x = (Load[LodPtr]->d.x * Load[LodPtr]->d.x) - (Load[LodPtr]->d.y * Load[LodPtr]->d.y);
    LodPtr++;
}

void dStkLodSqr2()
{
    Arg1++;
    Arg2++;
    LastSqr.d.x = Load[LodPtr]->d.x * Load[LodPtr]->d.x;
    LastSqr.d.y = Load[LodPtr]->d.y * Load[LodPtr]->d.y;
    Arg1->d.y = Load[LodPtr]->d.x * Load[LodPtr]->d.y * 2.0;
    Arg1->d.x = LastSqr.d.x - LastSqr.d.y;
    LastSqr.d.x += LastSqr.d.y;
    LastSqr.d.y = 0;
    LodPtr++;
}

void dStkLodDbl()
{
    Arg1++;
    Arg2++;
    Arg1->d.x = Load[LodPtr]->d.x * 2.0;
    Arg1->d.y = Load[LodPtr]->d.y * 2.0;
    LodPtr++;
}

void dStkSqr0()
{
    LastSqr.d.y = Arg1->d.y * Arg1->d.y; /* use LastSqr as temp storage */
    Arg1->d.y = Arg1->d.x * Arg1->d.y * 2.0;
    Arg1->d.x = Arg1->d.x * Arg1->d.x - LastSqr.d.y;
}


void dStkSqr3()
{
    Arg1->d.x = Arg1->d.x * Arg1->d.x;
}



void dStkAbs()
{
    Arg1->d.x = fabs(Arg1->d.x);
    Arg1->d.y = fabs(Arg1->d.y);
}

#if !defined(XFRACT)
void mStkAbs()
{
    if (Arg1->m.x.Exp < 0)
        Arg1->m.x.Exp = -Arg1->m.x.Exp;
    if (Arg1->m.y.Exp < 0)
        Arg1->m.y.Exp = -Arg1->m.y.Exp;
}

void lStkAbs()
{
    Arg1->l.x = labs(Arg1->l.x);
    Arg1->l.y = labs(Arg1->l.y);
}
#endif

void (*StkAbs)() = dStkAbs;

void dStkSqr()
{
    LastSqr.d.x = Arg1->d.x * Arg1->d.x;
    LastSqr.d.y = Arg1->d.y * Arg1->d.y;
    Arg1->d.y = Arg1->d.x * Arg1->d.y * 2.0;
    Arg1->d.x = LastSqr.d.x - LastSqr.d.y;
    LastSqr.d.x += LastSqr.d.y;
    LastSqr.d.y = 0;
}

#if !defined(XFRACT)
void mStkSqr()
{
    LastSqr.m.x = *MPmul(Arg1->m.x, Arg1->m.x);
    LastSqr.m.y = *MPmul(Arg1->m.y, Arg1->m.y);
    Arg1->m.y = *MPmul(Arg1->m.x, Arg1->m.y);
    Arg1->m.y.Exp++;
    Arg1->m.x = *MPsub(LastSqr.m.x, LastSqr.m.y);
    LastSqr.m.x = *MPadd(LastSqr.m.x, LastSqr.m.y);
    LastSqr.m.y.Exp = 0;
    LastSqr.m.y.Mant = (long) LastSqr.m.y.Exp;
}

void lStkSqr()
{
    LastSqr.l.x = multiply(Arg1->l.x, Arg1->l.x, bitshift);
    LastSqr.l.y = multiply(Arg1->l.y, Arg1->l.y, bitshift);
    Arg1->l.y = multiply(Arg1->l.x, Arg1->l.y, bitshift) << 1;
    Arg1->l.x = LastSqr.l.x - LastSqr.l.y;
    LastSqr.l.x += LastSqr.l.y;
    LastSqr.l.y = 0L;
}
#endif

void (*StkSqr)() = dStkSqr;

void dStkAdd()
{
    Arg2->d.x += Arg1->d.x;
    Arg2->d.y += Arg1->d.y;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)

void mStkAdd()
{
    Arg2->m = MPCadd(Arg2->m, Arg1->m);
    Arg1--;
    Arg2--;
}

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
void mStkSub()
{
    Arg2->m = MPCsub(Arg2->m, Arg1->m);
    Arg1--;
    Arg2--;
}

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
void mStkConj()
{
    Arg1->m.y.Exp ^= 0x8000;
}

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
void mStkFloor()
{
    mStkFunct(dStkFloor);   /* call lStk via dStk */
}

void lStkFloor()
{
    /*
     * Kill fractional part. This operation truncates negative numbers
     * toward negative infinity as desired.
     */
    Arg1->l.x = (Arg1->l.x) >> bitshift;
    Arg1->l.y = (Arg1->l.y) >> bitshift;
    Arg1->l.x = (Arg1->l.x) << bitshift;
    Arg1->l.y = (Arg1->l.y) << bitshift;
}
#endif

void (*StkFloor)() = dStkFloor;

void dStkCeil()
{
    Arg1->d.x = ceil(Arg1->d.x);
    Arg1->d.y = ceil(Arg1->d.y);
}

#if !defined(XFRACT)
void mStkCeil()
{
    mStkFunct(dStkCeil);   /* call lStk via dStk */
}

void lStkCeil()
{
    /* the shift operation does the "floor" operation, so we
       negate everything before the operation */
    Arg1->l.x = (-Arg1->l.x) >> bitshift;
    Arg1->l.y = (-Arg1->l.y) >> bitshift;
    Arg1->l.x = -((Arg1->l.x) << bitshift);
    Arg1->l.y = -((Arg1->l.y) << bitshift);
}
#endif

void (*StkCeil)() = dStkCeil;

void dStkTrunc()
{
    Arg1->d.x = (int)(Arg1->d.x);
    Arg1->d.y = (int)(Arg1->d.y);
}

#if !defined(XFRACT)
void mStkTrunc()
{
    mStkFunct(dStkTrunc);   /* call lStk via dStk */
}

void lStkTrunc()
{
    /* shifting and shifting back truncates positive numbers,
       so we make the numbers positive */
    int signx, signy;
    signx = sign(Arg1->l.x);
    signy = sign(Arg1->l.y);
    Arg1->l.x = labs(Arg1->l.x);
    Arg1->l.y = labs(Arg1->l.y);
    Arg1->l.x = (Arg1->l.x) >> bitshift;
    Arg1->l.y = (Arg1->l.y) >> bitshift;
    Arg1->l.x = (Arg1->l.x) << bitshift;
    Arg1->l.y = (Arg1->l.y) << bitshift;
    Arg1->l.x = signx*Arg1->l.x;
    Arg1->l.y = signy*Arg1->l.y;
}
#endif

void (*StkTrunc)() = dStkTrunc;

void dStkRound()
{
    Arg1->d.x = floor(Arg1->d.x+.5);
    Arg1->d.y = floor(Arg1->d.y+.5);
}

#if !defined(XFRACT)
void mStkRound()
{
    mStkFunct(dStkRound);   /* call lStk via dStk */
}

void lStkRound()
{
    /* Add .5 then truncate */
    Arg1->l.x += (1L<<bitshiftless1);
    Arg1->l.y += (1L<<bitshiftless1);
    lStkFloor();
}
#endif

void (*StkRound)() = dStkRound;

void dStkZero()
{
    Arg1->d.x = 0.0;
    Arg1->d.y = Arg1->d.x;
}

#if !defined(XFRACT)
void mStkZero()
{
    Arg1->m.x.Exp = 0;
    Arg1->m.x.Mant = Arg1->m.x.Exp;
    Arg1->m.y.Exp = 0;
    Arg1->m.y.Mant = Arg1->m.y.Exp;
}

void lStkZero()
{
    Arg1->l.x = 0;
    Arg1->l.y = Arg1->l.x;
}
#endif

void (*StkZero)() = dStkZero;

void dStkOne()
{
    Arg1->d.x = 1.0;
    Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void mStkOne()
{
    Arg1->m = MPCone;
}

void lStkOne()
{
    Arg1->l.x = (long) fg;
    Arg1->l.y = 0L;
}
#endif

void (*StkOne)() = dStkOne;


void dStkReal()
{
    Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void mStkReal()
{
    Arg1->m.y.Exp = 0;
    Arg1->m.y.Mant = (long) Arg1->m.y.Exp;
}

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
void mStkImag()
{
    Arg1->m.x = Arg1->m.y;
    Arg1->m.y.Exp = 0;
    Arg1->m.y.Mant = (long) Arg1->m.y.Exp;
}

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
void mStkNeg()
{
    Arg1->m.x.Exp ^= 0x8000;
    Arg1->m.y.Exp ^= 0x8000;
}

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
void mStkMul()
{
    Arg2->m = MPCmul(Arg2->m, Arg1->m);
    Arg1--;
    Arg2--;
}

void lStkMul()
{
    long x, y;

    x = multiply(Arg2->l.x, Arg1->l.x, bitshift) -
        multiply(Arg2->l.y, Arg1->l.y, bitshift);
    y = multiply(Arg2->l.y, Arg1->l.x, bitshift) +
        multiply(Arg2->l.x, Arg1->l.y, bitshift);
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
void mStkDiv()
{
    Arg2->m = MPCdiv(Arg2->m, Arg1->m);
    Arg1--;
    Arg2--;
}

void lStkDiv()
{
    long x, y, mod, x2, y2;

    mod = multiply(Arg1->l.x, Arg1->l.x, bitshift) +
          multiply(Arg1->l.y, Arg1->l.y, bitshift);
    x = divide(Arg1->l.x, mod, bitshift);
    y = -divide(Arg1->l.y, mod, bitshift);
    x2 = multiply(Arg2->l.x, x, bitshift) - multiply(Arg2->l.y, y, bitshift);
    y2 = multiply(Arg2->l.y, x, bitshift) + multiply(Arg2->l.x, y, bitshift);
    Arg2->l.x = x2;
    Arg2->l.y = y2;
    Arg1--;
    Arg2--;
}
#endif

void (*StkDiv)() = dStkDiv;

void dStkMod()
{
    Arg1->d.x = (Arg1->d.x * Arg1->d.x) + (Arg1->d.y * Arg1->d.y);
    Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void mStkMod()
{
    Arg1->m.x = MPCmod(Arg1->m);
    Arg1->m.y.Exp = 0;
    Arg1->m.y.Mant = (long) Arg1->m.y.Exp;
}

void lStkMod()
{
    /*   Arg1->l.x = multiply(Arg2->l.x, Arg1->l.x, bitshift) + */
    /*   multiply(Arg2->l.y, Arg1->l.y, bitshift); */
    Arg1->l.x = multiply(Arg1->l.x, Arg1->l.x, bitshift) +
                multiply(Arg1->l.y, Arg1->l.y, bitshift);
    if (Arg1->l.x < 0)
        overflow = true;
    Arg1->l.y = 0L;
}

void lStkModOld()
{
    Arg1->l.x = multiply(Arg2->l.x, Arg1->l.x, bitshift) +
                multiply(Arg2->l.y, Arg1->l.y, bitshift);
    if (Arg1->l.x < 0)
        overflow = true;
    Arg1->l.y = 0L;
}
#endif

void (*StkMod)() = dStkMod;

void StkSto()
{
    *Store[StoPtr++] = *Arg1;
}

void (*PtrStkSto)() = StkSto;

void StkLod()
{
    Arg1++;
    Arg2++;
    *Arg1 = *Load[LodPtr++];
}

void StkClr()
{
    s[0] = *Arg1;
    Arg1 = &s[0];
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
void mStkFlip()
{
    struct MP t;

    t = Arg1->m.x;
    Arg1->m.x = Arg1->m.y;
    Arg1->m.y = t;
}

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
    double sinx, cosx, sinhy, coshy;

    FPUsincos(&Arg1->d.x, &sinx, &cosx);
    FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
    Arg1->d.x = sinx*coshy;
    Arg1->d.y = cosx*sinhy;
}

#if !defined(XFRACT)
void mStkSin()
{
    mStkFunct(dStkSin);   /* call lStk via dStk */
}

void lStkSin()
{
    long x, y, sinx, cosx, sinhy, coshy;
    x = Arg1->l.x >> Delta16;
    y = Arg1->l.y >> Delta16;
    SinCos086(x, &sinx, &cosx);
    SinhCosh086(y, &sinhy, &coshy);
    Arg1->l.x = multiply(sinx, coshy, ShiftBack);
    Arg1->l.y = multiply(cosx, sinhy, ShiftBack);
}
#endif

void (*StkSin)() = dStkSin;

/* The following functions are supported by both the parser and for fn
   variable replacement.
*/
void dStkTan()
{
    double sinx, cosx, sinhy, coshy, denom;
    Arg1->d.x *= 2;
    Arg1->d.y *= 2;
    FPUsincos(&Arg1->d.x, &sinx, &cosx);
    FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
    denom = cosx + coshy;
    ChkFloatDenom(denom);
    Arg1->d.x = sinx/denom;
    Arg1->d.y = sinhy/denom;
}

#if !defined(XFRACT)
void mStkTan()
{
    mStkFunct(dStkTan);   /* call lStk via dStk */
}

void lStkTan()
{
    long x, y, sinx, cosx, sinhy, coshy, denom;
    x = Arg1->l.x >> Delta16;
    x = x << 1;
    y = Arg1->l.y >> Delta16;
    y = y << 1;
    SinCos086(x, &sinx, &cosx);
    SinhCosh086(y, &sinhy, &coshy);
    denom = cosx + coshy;
    ChkLongDenom(denom);
    Arg1->l.x = divide(sinx,denom,bitshift);
    Arg1->l.y = divide(sinhy,denom,bitshift);
}
#endif

void (*StkTan)() = dStkTan;

void dStkTanh()
{
    double siny, cosy, sinhx, coshx, denom;
    Arg1->d.x *= 2;
    Arg1->d.y *= 2;
    FPUsincos(&Arg1->d.y, &siny, &cosy);
    FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
    denom = coshx + cosy;
    ChkFloatDenom(denom);
    Arg1->d.x = sinhx/denom;
    Arg1->d.y = siny/denom;
}

#if !defined(XFRACT)
void mStkTanh()
{
    mStkFunct(dStkTanh);   /* call lStk via dStk */
}

void lStkTanh()
{
    long x, y, siny, cosy, sinhx, coshx, denom;
    x = Arg1->l.x >> Delta16;
    x = x << 1;
    y = Arg1->l.y >> Delta16;
    y = y << 1;
    SinCos086(y, &siny, &cosy);
    SinhCosh086(x, &sinhx, &coshx);
    denom = coshx + cosy;
    ChkLongDenom(denom);
    Arg1->l.x = divide(sinhx,denom,bitshift);
    Arg1->l.y = divide(siny,denom,bitshift);
}
#endif

void (*StkTanh)() = dStkTanh;

void dStkCoTan()
{
    double sinx, cosx, sinhy, coshy, denom;
    Arg1->d.x *= 2;
    Arg1->d.y *= 2;
    FPUsincos(&Arg1->d.x, &sinx, &cosx);
    FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
    denom = coshy - cosx;
    ChkFloatDenom(denom);
    Arg1->d.x = sinx/denom;
    Arg1->d.y = -sinhy/denom;
}

#if !defined(XFRACT)
void mStkCoTan()
{
    mStkFunct(dStkCoTan);   /* call lStk via dStk */
}

void lStkCoTan()
{
    long x, y, sinx, cosx, sinhy, coshy, denom;
    x = Arg1->l.x >> Delta16;
    x = x << 1;
    y = Arg1->l.y >> Delta16;
    y = y << 1;
    SinCos086(x, &sinx, &cosx);
    SinhCosh086(y, &sinhy, &coshy);
    denom = coshy - cosx;
    ChkLongDenom(denom);
    Arg1->l.x = divide(sinx,denom,bitshift);
    Arg1->l.y = -divide(sinhy,denom,bitshift);
}
#endif

void (*StkCoTan)() = dStkCoTan;

void dStkCoTanh()
{
    double siny, cosy, sinhx, coshx, denom;
    Arg1->d.x *= 2;
    Arg1->d.y *= 2;
    FPUsincos(&Arg1->d.y, &siny, &cosy);
    FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
    denom = coshx - cosy;
    ChkFloatDenom(denom);
    Arg1->d.x = sinhx/denom;
    Arg1->d.y = -siny/denom;
}

#if !defined(XFRACT)
void mStkCoTanh()
{
    mStkFunct(dStkCoTanh);   /* call lStk via dStk */
}

void lStkCoTanh()
{
    long x, y, siny, cosy, sinhx, coshx, denom;
    x = Arg1->l.x >> Delta16;
    x = x << 1;
    y = Arg1->l.y >> Delta16;
    y = y << 1;
    SinCos086(y, &siny, &cosy);
    SinhCosh086(x, &sinhx, &coshx);
    denom = coshx - cosy;
    ChkLongDenom(denom);
    Arg1->l.x = divide(sinhx,denom,bitshift);
    Arg1->l.y = -divide(siny,denom,bitshift);
}
#endif

void (*StkCoTanh)() = dStkCoTanh;

/* The following functions are not directly used by the parser - support
   for the parser was not provided because the existing parser language
   represents these quite easily. They are used for fn variable support
   in miscres.c but are placed here because they follow the pattern of
   the other parser functions.
*/

void dStkRecip()
{
    double mod;
    mod =Arg1->d.x * Arg1->d.x + Arg1->d.y * Arg1->d.y;
    ChkFloatDenom(mod);
    Arg1->d.x =  Arg1->d.x/mod;
    Arg1->d.y = -Arg1->d.y/mod;
}

#if !defined(XFRACT)
void mStkRecip()
{
    struct MP mod;
    mod = *MPadd(*MPmul(Arg1->m.x, Arg1->m.x),*MPmul(Arg1->m.y, Arg1->m.y));
    if (mod.Mant == 0L)
    {
        overflow = true;
        return;
    }
    Arg1->m.x = *MPdiv(Arg1->m.x,mod);
    Arg1->m.y = *MPdiv(Arg1->m.y,mod);
    Arg1->m.y.Exp ^= 0x8000;
}

void lStkRecip()
{
    long mod;
    mod = multiply(Arg1->l.x,Arg1->l.x,bitshift)
          + multiply(Arg1->l.y,Arg1->l.y,bitshift);
    if (save_release > 1920)
    {
        ChkLongDenom(mod);
    }
    else if (mod<=0L)
        return;
    Arg1->l.x =  divide(Arg1->l.x,mod,bitshift);
    Arg1->l.y = -divide(Arg1->l.y,mod,bitshift);
}
#endif

void StkIdent()
{ /* do nothing - the function Z */
}

void dStkSinh()
{
    double siny, cosy, sinhx, coshx;

    FPUsincos(&Arg1->d.y, &siny, &cosy);
    FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
    Arg1->d.x = sinhx*cosy;
    Arg1->d.y = coshx*siny;
}

#if !defined(XFRACT)
void mStkSinh()
{
    mStkFunct(dStkSinh);   /* call lStk via dStk */
}

void lStkSinh()
{
    long x, y, sinhx, coshx, siny, cosy;

    x = Arg1->l.x >> Delta16;
    y = Arg1->l.y >> Delta16;
    SinCos086(y, &siny, &cosy);
    SinhCosh086(x, &sinhx, &coshx);
    Arg1->l.x = multiply(cosy, sinhx, ShiftBack);
    Arg1->l.y = multiply(siny, coshx, ShiftBack);
}
#endif

void (*StkSinh)() = dStkSinh;

void dStkCos()
{
    double sinx, cosx, sinhy, coshy;

    FPUsincos(&Arg1->d.x, &sinx, &cosx);
    FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
    Arg1->d.x = cosx*coshy;
    Arg1->d.y = -sinx*sinhy;
}

#if !defined(XFRACT)
void mStkCos()
{
    mStkFunct(dStkCos);   /* call lStk via dStk */
}

void lStkCos()
{
    long x, y, sinx, cosx, sinhy, coshy;

    x = Arg1->l.x >> Delta16;
    y = Arg1->l.y >> Delta16;
    SinCos086(x, &sinx, &cosx);
    SinhCosh086(y, &sinhy, &coshy);
    Arg1->l.x = multiply(cosx, coshy, ShiftBack);
    Arg1->l.y = -multiply(sinx, sinhy, ShiftBack);
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
void mStkCosXX()
{
    mStkFunct(dStkCosXX);   /* call lStk via dStk */
}

void lStkCosXX()
{
    lStkCos();
    Arg1->l.y = -Arg1->l.y;
}
#endif

void (*StkCosXX)() = dStkCosXX;

void dStkCosh()
{
    double siny, cosy, sinhx, coshx;

    FPUsincos(&Arg1->d.y, &siny, &cosy);
    FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
    Arg1->d.x = coshx*cosy;
    Arg1->d.y = sinhx*siny;
}

#if !defined(XFRACT)
void mStkCosh()
{
    mStkFunct(dStkCosh);   /* call lStk via dStk */
}

void lStkCosh()
{
    long x, y, sinhx, coshx, siny, cosy;

    x = Arg1->l.x >> Delta16;
    y = Arg1->l.y >> Delta16;
    SinCos086(y, &siny, &cosy);
    SinhCosh086(x, &sinhx, &coshx);
    Arg1->l.x = multiply(cosy, coshx, ShiftBack);
    Arg1->l.y = multiply(siny, sinhx, ShiftBack);
}
#endif

void (*StkCosh)() = dStkCosh;

void dStkASin()
{
    Arcsinz(Arg1->d, &(Arg1->d));
}

#if !defined(XFRACT)
void mStkASin()
{
    mStkFunct(dStkASin);
}

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
void mStkASinh()
{
    mStkFunct(dStkASinh);
}

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
void mStkACos()
{
    mStkFunct(dStkACos);
}

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
void mStkACosh()
{
    mStkFunct(dStkACosh);
}

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
void mStkATan()
{
    mStkFunct(dStkATan);
}

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
void mStkATanh()
{
    mStkFunct(dStkATanh);
}

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
void mStkSqrt()
{
    mStkFunct(dStkSqrt);
}

void lStkSqrt()
{
    /* lStkFunct(dStkSqrt); */
    Arg1->l = ComplexSqrtLong(Arg1->l.x, Arg1->l.y);
}
#endif

void (*StkSqrt)() = dStkSqrt;

void dStkCAbs()
{
    Arg1->d.x = sqrt(sqr(Arg1->d.x)+sqr(Arg1->d.y));
    Arg1->d.y = 0.0;
}

#if !defined(XFRACT)
void mStkCAbs()
{
    mStkFunct(dStkCAbs);
}

void lStkCAbs()
{
    lStkFunct(dStkCAbs);
}
#endif

void (*StkCAbs)() = dStkCAbs;

void dStkLT()
{
    Arg2->d.x = (double)(Arg2->d.x < Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkLT()
{
    Arg2->m.x = *fg2MP((long)(MPcmp(Arg2->m.x, Arg1->m.x) == -1), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkLT()
{
    Arg2->l.x = (long)(Arg2->l.x < Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkLT)() = dStkLT;

void dStkGT()
{
    Arg2->d.x = (double)(Arg2->d.x > Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkGT()
{
    Arg2->m.x = *fg2MP((long)(MPcmp(Arg2->m.x, Arg1->m.x) == 1), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkGT()
{
    Arg2->l.x = (long)(Arg2->l.x > Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkGT)() = dStkGT;

void dStkLTE()
{
    Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkLTE()
{
    int comp;

    comp = MPcmp(Arg2->m.x, Arg1->m.x);
    Arg2->m.x = *fg2MP((long)(comp == -1 || comp == 0), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkLTE()
{
    Arg2->l.x = (long)(Arg2->l.x <= Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkLTE)() = dStkLTE;

void dStkGTE()
{
    Arg2->d.x = (double)(Arg2->d.x >= Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkGTE()
{
    int comp;

    comp = MPcmp(Arg2->m.x, Arg1->m.x);
    Arg2->m.x = *fg2MP((long)(comp == 1 || comp == 0), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkGTE()
{
    Arg2->l.x = (long)(Arg2->l.x >= Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkGTE)() = dStkGTE;

void dStkEQ()
{
    Arg2->d.x = (double)(Arg2->d.x == Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkEQ()
{
    int comp;

    comp = MPcmp(Arg2->m.x, Arg1->m.x);
    Arg2->m.x = *fg2MP((long)(comp == 0), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkEQ()
{
    Arg2->l.x = (long)(Arg2->l.x == Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkEQ)() = dStkEQ;

void dStkNE()
{
    Arg2->d.x = (double)(Arg2->d.x != Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkNE()
{
    int comp;

    comp = MPcmp(Arg2->m.x, Arg1->m.x);
    Arg2->m.x = *fg2MP((long)(comp != 0), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkNE()
{
    Arg2->l.x = (long)(Arg2->l.x != Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkNE)() = dStkNE;

void dStkOR()
{
    Arg2->d.x = (double)(Arg2->d.x || Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkOR()
{
    Arg2->m.x = *fg2MP((long)(Arg2->m.x.Mant || Arg1->m.x.Mant), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkOR()
{
    Arg2->l.x = (long)(Arg2->l.x || Arg1->l.x) << bitshift;
    Arg2->l.y = 0l;
    Arg1--;
    Arg2--;
}
#endif

void (*StkOR)() = dStkOR;

void dStkAND()
{
    Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
    Arg2->d.y = 0.0;
    Arg1--;
    Arg2--;
}

#if !defined(XFRACT)
void mStkAND()
{
    Arg2->m.x = *fg2MP((long)(Arg2->m.x.Mant && Arg1->m.x.Mant), 0);
    Arg2->m.y.Mant = (long)(Arg2->m.y.Exp = 0);
    Arg1--;
    Arg2--;
}

void lStkAND()
{
    Arg2->l.x = (long)(Arg2->l.x && Arg1->l.x) << bitshift;
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
void mStkLog()
{
    mStkFunct(dStkLog);   /* call lStk via dStk */
}

void lStkLog()
{
    lStkFunct(dStkLog);
}
#endif

void (*StkLog)() = dStkLog;

void FPUcplxexp(DComplex *x, DComplex *z)
{
    double pow, y;
    y = x->y;
    pow = exp(x->x);
    z->x = pow*cos(y);
    z->y = pow*sin(y);
}

void dStkExp()
{
    FPUcplxexp(&Arg1->d, &Arg1->d);
}

#if !defined(XFRACT)
void mStkExp()
{
    mStkFunct(dStkExp);   /* call lStk via dStk */
}

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
void mStkPwr()
{
    DComplex x, y;

    x = MPC2cmplx(Arg2->m);
    y = MPC2cmplx(Arg1->m);
    x = ComplexPower(x, y);
    Arg2->m = cmplx2MPC(x);
    Arg1--;
    Arg2--;
}

void lStkPwr()
{
    DComplex x, y;

    x.x = (double)Arg2->l.x / fg;
    x.y = (double)Arg2->l.y / fg;
    y.x = (double)Arg1->l.x / fg;
    y.y = (double)Arg1->l.y / fg;
    x = ComplexPower(x, y);
    if (fabs(x.x) < fgLimit && fabs(x.y) < fgLimit)
    {
        Arg2->l.x = (long)(x.x * fg);
        Arg2->l.y = (long)(x.y * fg);
    }
    else
        overflow = true;
    Arg1--;
    Arg2--;
}
#endif

void (*StkPwr)() = dStkPwr;

void EndInit()
{
    LastInitOp = OpPtr;
    InitJumpIndex = jump_index;
}

void (*PtrEndInit)() = EndInit;

void StkJump()
{
    OpPtr =  jump_control[jump_index].ptrs.JumpOpPtr;
    LodPtr = jump_control[jump_index].ptrs.JumpLodPtr;
    StoPtr = jump_control[jump_index].ptrs.JumpStoPtr;
    jump_index = jump_control[jump_index].DestJumpIndex;
}

void dStkJumpOnFalse()
{
    if (Arg1->d.x == 0)
        StkJump();
    else
        jump_index++;
}

void mStkJumpOnFalse()
{
#if !defined(XFRACT)
    if (Arg1->m.x.Mant == 0)
        StkJump();
    else
        jump_index++;
#endif
}

void lStkJumpOnFalse()
{
    if (Arg1->l.x == 0)
        StkJump();
    else
        jump_index++;
}

void (*StkJumpOnFalse)() = dStkJumpOnFalse;

void dStkJumpOnTrue()
{
    if (Arg1->d.x)
        StkJump();
    else
        jump_index++;
}

void mStkJumpOnTrue()
{
#if !defined(XFRACT)
    if (Arg1->m.x.Mant)
        StkJump();
    else
        jump_index++;
#endif
}

void lStkJumpOnTrue()
{
    if (Arg1->l.x)
        StkJump();
    else
        jump_index++;
}

void (*StkJumpOnTrue)() = dStkJumpOnTrue;

void StkJumpLabel()
{
    jump_index++;
}


unsigned SkipWhiteSpace(char *Str)
{
    unsigned n;
    bool Done = false;
    for (n = 0; !Done; n++)
    {
        switch (Str[n])
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            break;
        default:
            Done = true;
        }
    }
    return n - 1;
}

/* detect if constant is part of a (a,b) construct */
static bool isconst_pair(char *Str)
{
    int n,j;
    bool answer = false;
    /* skip past first number */
    for (n = 0; isdigit(Str[n]) || Str[n] == '.'; n++)
    {
    }
    if (Str[n] == ',')
    {
        j = n + SkipWhiteSpace(&Str[n+1]) + 1;
        if (isdigit(Str[j])
                || (Str[j] == '-' && (isdigit(Str[j+1]) || Str[j+1] == '.'))
                || Str[j] == '.')
        {
            answer = true;
        }
    }
    return answer;
}

struct ConstArg *isconst(char *Str, int Len)
{
    DComplex z;
    /* next line enforces variable vs constant naming convention */
    for (unsigned n = 0; n < vsp; n++)
    {
        if (v[n].len == Len)
        {
            if (!strnicmp(v[n].s, Str, Len))
            {
                if (n == 1)        /* The formula uses 'p1'. */
                    uses_p1 = true;
                if (n == 2)        /* The formula uses 'p2'. */
                    uses_p2 = true;
                if (n == 7)        /* The formula uses 'rand'. */
                    RandomSeed();
                if (n == 8)        /* The formula uses 'p3'. */
                    uses_p3 = true;
                if (n == 13)        /* The formula uses 'ismand'. */
                    uses_ismand = true;
                if (n == 17)        /* The formula uses 'p4'. */
                    uses_p4 = true;
                if (n == 18)        /* The formula uses 'p5'. */
                    uses_p5 = true;
#if !defined(XFRACT)
                if (n == 10 || n == 11 || n == 12)
                    if (MathType == L_MATH)
                        driver_unget_key('f');
#endif
                if (!isconst_pair(Str))
                    return &v[n];
            }
        }
    }
    v[vsp].s = Str;
    v[vsp].len = Len;
    v[vsp].a.d.x = v[vsp].a.d.y = 0.0;

#if !defined(XFRACT)
    /* v[vsp].a should already be zeroed out */
    switch (MathType)
    {
    case M_MATH:
        v[vsp].a.m.x.Mant = v[vsp].a.m.x.Exp = 0;
        v[vsp].a.m.y.Mant = v[vsp].a.m.y.Exp = 0;
        break;
    case L_MATH:
        v[vsp].a.l.x = v[vsp].a.l.y = 0;
        break;
    }
#endif

    if (isdigit(Str[0])
            || (Str[0] == '-' && (isdigit(Str[1]) || Str[1] == '.'))
            || Str[0] == '.')
    {
        if (o[posp-1].f == StkNeg)
        {
            posp--;
            Str = Str - 1;
            InitN--;
            v[vsp].len++;
        }
        unsigned n;
        for (n = 1; isdigit(Str[n]) || Str[n] == '.'; n++)
        {
        }
        if (Str[n] == ',')
        {
            unsigned j = n + SkipWhiteSpace(&Str[n+1]) + 1;
            if (isdigit(Str[j])
                    || (Str[j] == '-' && (isdigit(Str[j+1]) || Str[j+1] == '.'))
                    || Str[j] == '.')
            {
                z.y = atof(&Str[j]);
                for (; isdigit(Str[j]) || Str[j] == '.' || Str[j] == '-'; j++)
                {
                }
                v[vsp].len = j;
            }
            else
                z.y = 0.0;
        }
        else
            z.y = 0.0;
        z.x = atof(Str);
        switch (MathType)
        {
        case D_MATH:
            v[vsp].a.d = z;
            break;
#if !defined(XFRACT)
        case M_MATH:
            v[vsp].a.m = cmplx2MPC(z);
            break;
        case L_MATH:
            v[vsp].a.l.x = (long)(z.x * fg);
            v[vsp].a.l.y = (long)(z.y * fg);
            break;
#endif
        }
        v[vsp].s = Str;
    }
    return &v[vsp++];
}


struct FNCT_LIST
{
    const char *s;
    void (**ptr)();
};

void (*StkTrig0)() = dStkSin;
void (*StkTrig1)() = dStkSqr;
void (*StkTrig2)() = dStkSinh;
void (*StkTrig3)() = dStkCosh;

const char * JumpList[] =
{
    "if",
    "elseif",
    "else",
    "endif",
    ""
};



int isjump(char *Str, int Len)
{
    /* return values
        0 - Not a jump
        1 - if
        2 - elseif
        3 - else
        4 - endif
    */
    for (int i = 0; *JumpList[i]; i++)
        if ((int) strlen(JumpList[i]) == Len)
            if (!strnicmp(JumpList[i], Str, Len))
                return i + 1;
    return 0;
}


char maxfn = 0;

struct FNCT_LIST FnctList[] =
{
    {"sin",   &StkSin},
    {"sinh",  &StkSinh},
    {"cos",   &StkCos},
    {"cosh",  &StkCosh},
    {"sqr",   &StkSqr},
    {"log",   &StkLog},
    {"exp",   &StkExp},
    {"abs",   &StkAbs},
    {"conj",  &StkConj},
    {"real",  &StkReal},
    {"imag",  &StkImag},
    {"fn1",   &StkTrig0},
    {"fn2",   &StkTrig1},
    {"fn3",   &StkTrig2},
    {"fn4",   &StkTrig3},
    {"flip",  &StkFlip},
    {"tan",   &StkTan},
    {"tanh",  &StkTanh},
    {"cotan", &StkCoTan},
    {"cotanh",&StkCoTanh},
    {"cosxx", &StkCosXX},
    {"srand", &StkSRand},
    {"asin",  &StkASin},
    {"asinh", &StkASinh},
    {"acos",  &StkACos},
    {"acosh", &StkACosh},
    {"atan",  &StkATan},
    {"atanh", &StkATanh},
    {"sqrt",  &StkSqrt},
    {"cabs",  &StkCAbs},
    {"floor", &StkFloor},
    {"ceil",  &StkCeil},
    {"trunc", &StkTrunc},
    {"round", &StkRound},
};

const char *OPList[] =
{
    ",",    /*  0 */
    "!=",   /*  1 */
    "=",    /*  2 */
    "==",   /*  3 */
    "<",    /*  4 */
    "<=",   /*  5 */
    ">",    /*  6 */
    ">=",   /*  7 */
    "|",    /*  8 */
    "||",   /*  9 */
    "&&",   /* 10 */
    ":",    /* 11 */
    "+",    /* 12 */
    "-",    /* 13 */
    "*",    /* 14 */
    "/",    /* 15 */
    "^"     /* 16 */
};


void NotAFnct()
{
}
void FnctNotFound()
{
}

/* determine if s names a function and if so which one */

int whichfn(char *s, int len)
{
    int out;
    if (len != 3)
        out = 0;
    else if (strnicmp(s,"fn",2))
        out = 0;
    else
        out = atoi(s+2);
    if (out < 1 || out > 4)
        out = 0;
    return out;
}

void (*isfunct(char *Str, int Len))()
{
    unsigned n = SkipWhiteSpace(&Str[Len]);
    if (Str[Len+n] == '(')
    {
        for (n = 0; n < sizeof(FnctList)/sizeof(struct FNCT_LIST); n++)
        {
            if ((int) strlen(FnctList[n].s) == Len)
            {
                if (!strnicmp(FnctList[n].s, Str, Len))
                {
                    /* count function variables */
                    int functnum = whichfn(Str, Len);
                    if (functnum != 0)
                        if (functnum > maxfn)
                            maxfn = (char)functnum;
                    return *FnctList[n].ptr;
                }
            }
        }
        return FnctNotFound;
    }
    return NotAFnct;
}

void RecSortPrec()
{
    int ThisOp = NextOp++;
    while (o[ThisOp].p > o[NextOp].p && NextOp < posp)
        RecSortPrec();
    f[OpPtr++] = o[ThisOp].f;
}

static const char *Constants[] =
{
    "pixel",        /* v[0] */
    "p1",           /* v[1] */
    "p2",           /* v[2] */
    "z",            /* v[3] */
    "LastSqr",      /* v[4] */
    "pi",           /* v[5] */
    "e",            /* v[6] */
    "rand",         /* v[7] */
    "p3",           /* v[8] */
    "whitesq",      /* v[9] */
    "scrnpix",      /* v[10] */
    "scrnmax",      /* v[11] */
    "maxit",        /* v[12] */
    "ismand",       /* v[13] */
    "center",       /* v[14] */
    "magxmag",      /* v[15] */
    "rotskew",      /* v[16] */
    "p4",           /* v[17] */
    "p5"            /* v[18] */
};

struct SYMETRY
{
    const char *s;
    int n;
}
SymStr[] =
{
    {"NOSYM",         0},
    {"XAXIS_NOPARM", -1},
    {"XAXIS",         1},
    {"YAXIS_NOPARM", -2},
    {"YAXIS",         2},
    {"XYAXIS_NOPARM",-3},
    {"XYAXIS",        3},
    {"ORIGIN_NOPARM",-4},
    {"ORIGIN",        4},
    {"PI_SYM_NOPARM",-5},
    {"PI_SYM",        5},
    {"XAXIS_NOIMAG", -6},
    {"XAXIS_NOREAL",  6},
    {"NOPLOT",       99},
    {"",              0}
};

static bool ParseStr(char *Str, int pass)
{
    struct ConstArg *c;
    int ModFlag = 999, Len, Equals = 0, Mods[20], mdstk = 0;
    int jumptype;
    double const_pi, const_e;
    double Xctr, Yctr, Xmagfactor, Rotation, Skew;
    LDBL Magnification;
    SetRandom = false;
    Randomized = false;
    uses_jump = false;
    jump_index = 0;
    if (!typespecific_workarea)
    {
        stopmsg(0,ParseErrs(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
        return true;
    }
    switch (MathType)
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
        StkTrig0 = dtrig0;
        StkTrig1 = dtrig1;
        StkTrig2 = dtrig2;
        StkTrig3 = dtrig3;
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
        StkOR  = dStkOR ;
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
    case M_MATH:
        StkAdd = mStkAdd;
        StkSub = mStkSub;
        StkNeg = mStkNeg;
        StkMul = mStkMul;
        StkSin = mStkSin;
        StkSinh = mStkSinh;
        StkLT = mStkLT;
        StkLTE = mStkLTE;
        StkMod = mStkMod;
        StkSqr = mStkSqr;
        StkCos = mStkCos;
        StkCosh = mStkCosh;
        StkLog = mStkLog;
        StkExp = mStkExp;
        StkPwr = mStkPwr;
        StkDiv = mStkDiv;
        StkAbs = mStkAbs;
        StkReal = mStkReal;
        StkImag = mStkImag;
        StkConj = mStkConj;
        StkTrig0 = mtrig0;
        StkTrig1 = mtrig1;
        StkTrig2 = mtrig2;
        StkTrig3 = mtrig3;
        StkFlip = mStkFlip;
        StkTan  = mStkTan;
        StkTanh  = mStkTanh;
        StkCoTan  = mStkCoTan;
        StkCoTanh  = mStkCoTanh;
        StkCosXX = mStkCosXX;
        StkGT  = mStkGT;
        StkGTE = mStkGTE;
        StkEQ  = mStkEQ;
        StkNE  = mStkNE;
        StkAND = mStkAND;
        StkOR  = mStkOR ;
        StkSRand = mStkSRand;
        StkASin = mStkASin;
        StkACos = mStkACos;
        StkACosh = mStkACosh;
        StkATan = mStkATan;
        StkATanh = mStkATanh;
        StkCAbs = mStkCAbs;
        StkSqrt = mStkSqrt;
        StkZero = mStkZero;
        StkFloor = mStkFloor;
        StkCeil = mStkCeil;
        StkTrunc = mStkTrunc;
        StkRound = mStkRound;
        StkJumpOnTrue  = mStkJumpOnTrue;
        StkJumpOnFalse = mStkJumpOnFalse;
        StkOne = mStkOne;
        break;
    case L_MATH:
        Delta16 = bitshift - 16;
        ShiftBack = 32 - bitshift;
        StkAdd = lStkAdd;
        StkSub = lStkSub;
        StkNeg = lStkNeg;
        StkMul = lStkMul;
        StkSin = lStkSin;
        StkSinh = lStkSinh;
        StkLT = lStkLT;
        StkLTE = lStkLTE;
        if (save_release > 1826)
            StkMod = lStkMod;
        else
            StkMod = lStkModOld;
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
        StkTrig0 = ltrig0;
        StkTrig1 = ltrig1;
        StkTrig2 = ltrig2;
        StkTrig3 = ltrig3;
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
        StkOR  = lStkOR ;
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
    maxfn = 0;
    for (vsp = 0; vsp < sizeof(Constants) / sizeof(char*); vsp++)
    {
        v[vsp].s = Constants[vsp];
        v[vsp].len = (int) strlen(Constants[vsp]);
    }
    cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
    const_pi = atan(1.0) * 4;
    const_e  = exp(1.0);
    v[7].a.d.x = v[7].a.d.y = 0.0;
    v[11].a.d.x = (double)xdots;
    v[11].a.d.y = (double)ydots;
    v[12].a.d.x = (double)maxit;
    v[12].a.d.y = 0;
    v[13].a.d.x = ismand ? 1.0 : 0.0;
    v[13].a.d.y = 0;
    v[14].a.d.x = Xctr;
    v[14].a.d.y = Yctr;
    v[15].a.d.x = (double)Magnification;
    v[15].a.d.y = Xmagfactor;
    v[16].a.d.x = Rotation;
    v[16].a.d.y = Skew;

    switch (MathType)
    {
    case D_MATH:
        v[1].a.d.x = param[0];
        v[1].a.d.y = param[1];
        v[2].a.d.x = param[2];
        v[2].a.d.y = param[3];
        v[5].a.d.x = const_pi;
        v[5].a.d.y = 0.0;
        v[6].a.d.x = const_e;
        v[6].a.d.y = 0.0;
        v[8].a.d.x = param[4];
        v[8].a.d.y = param[5];
        v[17].a.d.x = param[6];
        v[17].a.d.y = param[7];
        v[18].a.d.x = param[8];
        v[18].a.d.y = param[9];
        break;
#if !defined(XFRACT)
    case M_MATH:
        v[1].a.m.x = *d2MP(param[0]);
        v[1].a.m.y = *d2MP(param[1]);
        v[2].a.m.x = *d2MP(param[2]);
        v[2].a.m.y = *d2MP(param[3]);
        v[5].a.m.x = *d2MP(const_pi);
        v[5].a.m.y = *d2MP(0.0);
        v[6].a.m.x = *d2MP(const_e);
        v[6].a.m.y = *d2MP(0.0);
        v[8].a.m.x = *d2MP(param[4]);
        v[8].a.m.y = *d2MP(param[5]);
        v[11].a.m  = cmplx2MPC(v[11].a.d);
        v[12].a.m  = cmplx2MPC(v[12].a.d);
        v[13].a.m  = cmplx2MPC(v[13].a.d);
        v[14].a.m  = cmplx2MPC(v[14].a.d);
        v[15].a.m  = cmplx2MPC(v[15].a.d);
        v[16].a.m  = cmplx2MPC(v[16].a.d);
        v[17].a.m.x = *d2MP(param[6]);
        v[17].a.m.y = *d2MP(param[7]);
        v[18].a.m.x = *d2MP(param[8]);
        v[18].a.m.y = *d2MP(param[9]);
        break;
    case L_MATH:
        v[1].a.l.x = (long)(param[0] * fg);
        v[1].a.l.y = (long)(param[1] * fg);
        v[2].a.l.x = (long)(param[2] * fg);
        v[2].a.l.y = (long)(param[3] * fg);
        v[5].a.l.x = (long)(const_pi * fg);
        v[5].a.l.y = 0L;
        v[6].a.l.x = (long)(const_e * fg);
        v[6].a.l.y = 0L;
        v[8].a.l.x = (long)(param[4] * fg);
        v[8].a.l.y = (long)(param[5] * fg);
        v[11].a.l.x = xdots;
        v[11].a.l.x <<= bitshift;
        v[11].a.l.y = ydots;
        v[11].a.l.y <<= bitshift;
        v[12].a.l.x = maxit;
        v[12].a.l.x <<= bitshift;
        v[12].a.l.y = 0L;
        v[13].a.l.x = ismand ? 1 : 0;
        v[13].a.l.x <<= bitshift;
        v[13].a.l.y = 0L;
        v[14].a.l.x = (long)(v[14].a.d.x * fg);
        v[14].a.l.y = (long)(v[14].a.d.y * fg);
        v[15].a.l.x = (long)(v[15].a.d.x * fg);
        v[15].a.l.y = (long)(v[15].a.d.y * fg);
        v[16].a.l.x = (long)(v[16].a.d.x * fg);
        v[16].a.l.y = (long)(v[16].a.d.y * fg);
        v[17].a.l.x = (long)(param[6] * fg);
        v[17].a.l.y = (long)(param[7] * fg);
        v[18].a.l.x = (long)(param[8] * fg);
        v[18].a.l.y = (long)(param[9] * fg);
        break;
#endif
    }

    LastInitOp = paren = OpPtr = LodPtr = StoPtr = posp = 0;
    ExpectingArg = true;
    for (n = 0; Str[n]; n++)
    {
        if (!Str[n])
            break;
        InitN = n;
        switch (Str[n])
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;
        case '(':
            paren++;
            break;
        case ')':
            paren--;
            break;
        case '|':
            if (Str[n+1] == '|')
            {
                ExpectingArg = true;
                n++;
                o[posp].f = StkOR;
                o[posp++].p = 7 - (paren + Equals)*15;
            }
            else if (ModFlag == paren-1)
            {
                paren--;
                ModFlag = Mods[--mdstk];
            }
            else
            {
                Mods[mdstk++] = ModFlag;
                o[posp].f = StkMod;
                o[posp++].p = 2 - (paren + Equals)*15;
                ModFlag = paren++;
            }
            break;
        case ',':
        case ';':
            if (!ExpectingArg)
            {
                ExpectingArg = true;
                o[posp].f = (void(*)())nullptr;
                o[posp++].p = 15;
                o[posp].f = StkClr;
                o[posp++].p = -30000;
                Equals = paren = 0;
            }
            break;
        case ':':
            ExpectingArg = true;
            o[posp].f = (void(*)())nullptr;
            o[posp++].p = 15;
            o[posp].f = EndInit;
            o[posp++].p = -30000;
            Equals = paren = 0;
            LastInitOp = 10000;
            break;
        case '+':
            ExpectingArg = true;
            o[posp].f = StkAdd;
            o[posp++].p = 4 - (paren + Equals)*15;
            break;
        case '-':
            if (ExpectingArg)
            {
                o[posp].f = StkNeg;
                o[posp++].p = 2 - (paren + Equals)*15;
            }
            else
            {
                o[posp].f = StkSub;
                o[posp++].p = 4 - (paren + Equals)*15;
                ExpectingArg = true;
            }
            break;
        case '&':
            ExpectingArg = true;
            n++;
            o[posp].f = StkAND;
            o[posp++].p = 7 - (paren + Equals)*15;
            break;
        case '!':
            ExpectingArg = true;
            n++;
            o[posp].f = StkNE;
            o[posp++].p = 6 - (paren + Equals)*15;
            break;
        case '<':
            ExpectingArg = true;
            if (Str[n+1] == '=')
            {
                n++;
                o[posp].f = StkLTE;
            }
            else
                o[posp].f = StkLT;
            o[posp++].p = 6 - (paren + Equals)*15;
            break;
        case '>':
            ExpectingArg = true;
            if (Str[n+1] == '=')
            {
                n++;
                o[posp].f = StkGTE;
            }
            else
                o[posp].f = StkGT;
            o[posp++].p = 6 - (paren + Equals)*15;
            break;
        case '*':
            ExpectingArg = true;
            o[posp].f = StkMul;
            o[posp++].p = 3 - (paren + Equals)*15;
            break;
        case '/':
            ExpectingArg = true;
            o[posp].f = StkDiv;
            o[posp++].p = 3 - (paren + Equals)*15;
            break;
        case '^':
            ExpectingArg = true;
            o[posp].f = StkPwr;
            o[posp++].p = 2 - (paren + Equals)*15;
            break;
        case '=':
            ExpectingArg = true;
            if (Str[n+1] == '=')
            {
                n++;
                o[posp].f = StkEQ;
                o[posp++].p = 6 - (paren + Equals)*15;
            }
            else {
                o[posp-1].f = StkSto;
                o[posp-1].p = 5 - (paren + Equals)*15;
                Store[StoPtr++] = Load[--LodPtr];
                Equals++;
            }
            break;
        default:
            while (isalnum(Str[n+1]) || Str[n+1] == '.' || Str[n+1] == '_')
                n++;
            Len = (n+1)-InitN;
            ExpectingArg = false;
            if ((jumptype = isjump(&Str[InitN], Len)) != 0)
            {
                uses_jump = true;
                switch (jumptype)
                {
                case 1:                      /* if */
                    ExpectingArg = true;
                    jump_control[jump_index++].type = 1;
                    o[posp].f = StkJumpOnFalse;
                    o[posp++].p = 1;
                    break;
                case 2:                     /* elseif */
                    ExpectingArg = true;
                    jump_control[jump_index++].type = 2;
                    jump_control[jump_index++].type = 2;
                    o[posp].f = StkJump;
                    o[posp++].p = 1;
                    o[posp].f = (void(*)())nullptr;
                    o[posp++].p = 15;
                    o[posp].f = StkClr;
                    o[posp++].p = -30000;
                    o[posp].f = StkJumpOnFalse;
                    o[posp++].p = 1;
                    break;
                case 3:                     /* else */
                    jump_control[jump_index++].type = 3;
                    o[posp].f = StkJump;
                    o[posp++].p = 1;
                    break;
                case 4: /* endif */
                    jump_control[jump_index++].type = 4;
                    o[posp].f = StkJumpLabel;
                    o[posp++].p = 1;
                    break;
                default:
                    break;
                }
            }
            else
            {
                o[posp].f = isfunct(&Str[InitN], Len);
                if (o[posp].f != NotAFnct)
                {
                    o[posp++].p = 1 - (paren + Equals)*15;
                    ExpectingArg = true;
                }
                else
                {
                    c = isconst(&Str[InitN], Len);
                    Load[LodPtr++] = &(c->a);
                    o[posp].f = StkLod;
                    o[posp++].p = 1 - (paren + Equals)*15;
                    n = InitN + c->len - 1;
                }
            }
            break;
        }
    }
    o[posp].f = (void(*)())nullptr;
    o[posp++].p = 16;
    NextOp = 0;
    LastOp = posp;
    while (NextOp < posp)
    {
        if (o[NextOp].f)
            RecSortPrec();
        else
        {
            NextOp++;
            LastOp--;
        }
    }
    return false;
}


int Formula()
{
    if (FormName[0] == 0 || overflow)
        return 1;

    LodPtr = InitLodPtr;
    StoPtr = InitStoPtr;
    OpPtr = InitOpPtr;
    jump_index=InitJumpIndex;
    /* Set the random number */
    if (SetRandom || Randomized)
    {
        switch (MathType)
        {
        case D_MATH:
            dRandom();
            break;
#if !defined(XFRACT)
        case L_MATH:
            lRandom();
            break;
        case M_MATH:
            mRandom();
#endif
        }
    }

    Arg1 = &s[0];
    Arg2 = Arg1-1;
    while (OpPtr < (int)LastOp)
    {
        f[OpPtr]();
        OpPtr++;
#ifdef WATCH_MP
        x1 = *MP2d(Arg1->m.x);
        y1 = *MP2d(Arg1->m.y);
        x2 = *MP2d(Arg2->m.x);
        y2 = *MP2d(Arg2->m.y);
#endif
    }

    switch (MathType)
    {
    case D_MATH:
        old = g_new = v[3].a.d;
        return Arg1->d.x == 0.0;
#if !defined(XFRACT)
    case M_MATH:
        old = g_new = MPC2cmplx(v[3].a.m);
        return Arg1->m.x.Exp == 0 && Arg1->m.x.Mant == 0;
    case L_MATH:
        lold = lnew = v[3].a.l;
        if (overflow)
            return 1;
        return Arg1->l.x == 0L;
#endif
    }
    return 1;
}

int form_per_pixel()
{
    if (FormName[0] == 0)
        return 1;
    overflow = false;
    LodPtr = StoPtr = OpPtr = jump_index = 0;
    Arg1 = &s[0];
    Arg2 = Arg1;
    Arg2--;


    v[10].a.d.x = (double)col;
    v[10].a.d.y = (double)row;

    switch (MathType)
    {
    case D_MATH:
        if ((row+col)&1)
            v[9].a.d.x = 1.0;
        else
            v[9].a.d.x = 0.0;
        v[9].a.d.y = 0.0;
        break;


#if !defined(XFRACT)
    case M_MATH:
        if ((row+col)&1)
            v[9].a.m = MPCone;
        else
        {
            v[9].a.m.x.Mant = v[9].a.m.x.Exp = 0;
            v[9].a.m.y.Mant = v[9].a.m.y.Exp = 0;
        }
        v[10].a.m = cmplx2MPC(v[10].a.d);
        break;
    case L_MATH:
        v[9].a.l.x = (long)(((row+col)&1) * fg);
        v[9].a.l.y = 0L;
        v[10].a.l.x = col;
        v[10].a.l.x <<= bitshift;
        v[10].a.l.y = row;
        v[10].a.l.y <<= bitshift;
        break;
#endif
    }

    {
        if (invert)
        {
            invertz2(&old);
            switch (MathType)
            {
            case D_MATH:
                v[0].a.d.x = old.x;
                v[0].a.d.y = old.y;
                break;
#if !defined(XFRACT)
            case M_MATH:
                v[0].a.m.x = *d2MP(old.x);
                v[0].a.m.y = *d2MP(old.y);
                break;
            case L_MATH:
                /* watch out for overflow */
                if (sqr(old.x)+sqr(old.y) >= 127)
                {
                    old.x = 8;  /* value to bail out in one iteration */
                    old.y = 8;
                }
                /* convert to fudged longs */
                v[0].a.l.x = (long)(old.x*fg);
                v[0].a.l.y = (long)(old.y*fg);
                break;
#endif
            }
        }
        else
            switch (MathType)
            {
            case D_MATH:
                v[0].a.d.x = dxpixel();
                v[0].a.d.y = dypixel();
                break;
#if !defined(XFRACT)
            case M_MATH:
                v[0].a.m.x = *d2MP(dxpixel());
                v[0].a.m.y = *d2MP(dypixel());
                break;
            case L_MATH:
                v[0].a.l.x = lxpixel();
                v[0].a.l.y = lypixel();
                break;
#endif
            }
    }

    if (LastInitOp)
        LastInitOp = LastOp;
    while (OpPtr < LastInitOp)
    {
        f[OpPtr]();
        OpPtr++;
    }
    InitLodPtr = LodPtr;
    InitStoPtr = StoPtr;
    InitOpPtr = OpPtr;
    /* Set old variable for orbits */
    switch (MathType)
    {
    case D_MATH:
        old = v[3].a.d;
        break;
#if !defined(XFRACT)
    case M_MATH:
        old = MPC2cmplx(v[3].a.m);
        break;
    case L_MATH:
        lold = v[3].a.l;
        break;
#endif
    }

    if (overflow)
        return 0;
    else
        return 1;
}

int fill_if_group(int endif_index, JUMP_PTRS_ST* jump_data)
{
    int i   = endif_index;
    int ljp = endif_index; /* ljp means "last jump processed" */
    while (i > 0)
    {
        i--;
        switch (jump_control[i].type)
        {
        case 1:    /*if (); this concludes processing of this group*/
            jump_control[i].ptrs = jump_data[ljp];
            jump_control[i].DestJumpIndex = ljp + 1;
            return i;
        case 2:    /*elseif* ( 2 jumps, the else and the if*/
            /* first, the "if" part */
            jump_control[i].ptrs = jump_data[ljp];
            jump_control[i].DestJumpIndex = ljp + 1;

            /* then, the else part */
            i--; /*fall through to "else" is intentional*/
        case 3:
            jump_control[i].ptrs = jump_data[endif_index];
            jump_control[i].DestJumpIndex = endif_index + 1;
            ljp = i;
            break;
        case 4:    /*endif*/
            i = fill_if_group(i, jump_data);
            break;
        default:
            break;
        }
    }
    return -1; /* should never get here */
}

static bool fill_jump_struct()
{   /* Completes all entries in jump structure. Returns 1 on error) */
    /* On entry, jump_index is the number of jump functions in the formula*/
    int i = 0;
    int loadcount = 0;
    int storecount = 0;
    bool checkforelse = false;
    void (*JumpFunc)() = nullptr;
    bool find_new_func = true;

    JUMP_PTRS_ST jump_data[MAX_JUMPS];

    for (OpPtr = 0; OpPtr < (int) LastOp; OpPtr++)
    {
        if (find_new_func)
        {
            switch (jump_control[i].type)
            {
            case 1:
                JumpFunc = StkJumpOnFalse;
                break;
            case 2:
                checkforelse = !checkforelse;
                if (checkforelse)
                    JumpFunc = StkJump;
                else
                    JumpFunc = StkJumpOnFalse;
                break;
            case 3:
                JumpFunc = StkJump;
                break;
            case 4:
                JumpFunc = StkJumpLabel;
                break;
            default:
                break;
            }
            find_new_func = false;
        }
        if (*(f[OpPtr]) == StkLod)
            loadcount++;
        else if (*(f[OpPtr]) == StkSto)
            storecount++;
        else if (*(f[OpPtr]) == JumpFunc)
        {
            jump_data[i].JumpOpPtr = OpPtr;
            jump_data[i].JumpLodPtr = loadcount;
            jump_data[i].JumpStoPtr = storecount;
            i++;
            find_new_func = true;
        }
    }

    /* Following for safety only; all should always be false */
    if (i != jump_index || jump_control[i - 1].type != 4
            || jump_control[0].type != 1)
    {
        return true;
    }

    while (i > 0)
    {
        i--;
        i = fill_if_group(i, jump_data);
    }
    return i < 0;
}

static char *FormStr;

int frmgetchar(FILE * openfile)
{
    int c;
    bool done = false;
    bool linewrap = false;
    while (!done)
    {
        c = getc(openfile);
        switch (c)
        {
        case '\r':
        case ' ' :
        case '\t' :
            break;
        case '\\':
            linewrap = true;
            break;
        case ';' :
            while ((c = getc(openfile)) != '\n' && c != EOF && c != '\032')
            {
            }
            if (c == EOF || c == '\032')
                done = true;
        case '\n' :
            if (!linewrap)
                done = true;
            linewrap = false;
            break;
        default:
            done = true;
            break;
        }
    }
    return tolower(c);
}

/* This function also gets flow control info */

void getfuncinfo(struct token_st * tok)
{
    for (int i = 0; i < sizeof(FnctList)/sizeof(struct FNCT_LIST); i++)
    {
        if (!strcmp(FnctList[i].s, tok->token_str))
        {
            tok->token_id = i;
            if (i >= 11 && i <= 14)
                tok->token_type = PARAM_FUNCTION;
            else
                tok->token_type = FUNCTION;
            return;
        }
    }

    for (int i = 0; i < 4; i++)        /*pick up flow control*/
    {
        if (!strcmp(JumpList[i], tok->token_str))
        {
            tok->token_type = FLOW_CONTROL;
            tok->token_id   = i + 1;
            return;
        }
    }
    tok->token_type = NOT_A_TOKEN;
    tok->token_id   = UNDEFINED_FUNCTION;
    return;
}

void getvarinfo(struct token_st * tok)
{
    for (int i = 0; i < sizeof(Constants)/sizeof(char*); i++)
    {
        if (!strcmp(Constants[i], tok->token_str))
        {
            tok->token_id = i;
            switch (i)
            {
            case 1:
            case 2:
            case 8:
            case 13:
            case 17:
            case 18:
                tok->token_type = PARAM_VARIABLE;
                break;
            default:
                tok->token_type = PREDEFINED_VARIABLE;
                break;
            }
            return;
        }
    }
    tok->token_type = USER_NAMED_VARIABLE;
    tok->token_id   = 0;
}

/* fills in token structure where numeric constant is indicated */
/* Note - this function will be called twice to fill in the components
        of a complex constant. See is_complex_constant() below. */

/* returns 1 on success, 0 on NOT_A_TOKEN */
static bool  frmgetconstant(FILE * openfile, struct token_st * tok)
{
    int c;
    int i = 1;
    bool getting_base = true;
    long filepos = ftell(openfile);
    bool got_decimal_already = false;
    bool done = false;
    tok->token_const.x = 0.0;          /*initialize values to 0*/
    tok->token_const.y = 0.0;
    if (tok->token_str[0] == '.')
        got_decimal_already = true;
    while (!done)
    {
        switch (c=frmgetchar(openfile))
        {
        case EOF:
        case '\032':
            tok->token_str[i] = (char) 0;
            tok->token_type = NOT_A_TOKEN;
            tok->token_id   = END_OF_FILE;
            return false;
CASE_NUM:
            tok->token_str[i++] = (char) c;
            filepos=ftell(openfile);
            break;
        case '.':
            if (got_decimal_already || !getting_base)
            {
                tok->token_str[i++] = (char) c;
                tok->token_str[i++] = (char) 0;
                tok->token_type = NOT_A_TOKEN;
                tok->token_id = ILL_FORMED_CONSTANT;
                return false;
            }
            else
            {
                tok->token_str[i++] = (char) c;
                got_decimal_already = true;
                filepos=ftell(openfile);
            }
            break;
        default :
            if (c == 'e' && getting_base && (isdigit(tok->token_str[i-1]) || (tok->token_str[i-1] == '.' && i > 1)))
            {
                tok->token_str[i++] = (char) c;
                getting_base = false;
                got_decimal_already = false;
                filepos=ftell(openfile);
                c = frmgetchar(openfile);
                if (c == '-' || c == '+')
                {
                    tok->token_str[i++] = (char) c;
                    filepos = ftell(openfile);
                }
                else
                {
                    fseek(openfile, filepos, SEEK_SET);
                }
            }
            else if (isalpha(c) || c == '_')
            {
                tok->token_str[i++] = (char) c;
                tok->token_str[i++] = (char) 0;
                tok->token_type = NOT_A_TOKEN;
                tok->token_id = ILL_FORMED_CONSTANT;
                return false;
            }
            else if (tok->token_str[i-1] == 'e' || (tok->token_str[i-1] == '.' && i == 1))
            {
                tok->token_str[i++] = (char) c;
                tok->token_str[i++] = (char) 0;
                tok->token_type = NOT_A_TOKEN;
                tok->token_id = ILL_FORMED_CONSTANT;
                return false;
            }
            else
            {
                fseek(openfile, filepos, SEEK_SET);
                tok->token_str[i++] = (char) 0;
                done = true;
            }
            break;
        }
        if (i == 33 && tok->token_str[32])
        {
            tok->token_str[33] = (char) 0;
            tok->token_type = NOT_A_TOKEN;
            tok->token_id = TOKEN_TOO_LONG;
            return false;
        }
    }    /* end of while loop. Now fill in the value */
    tok->token_const.x = atof(tok->token_str);
    tok->token_type = REAL_CONSTANT;
    tok->token_id   = 0;
    return true;
}

void is_complex_constant(FILE * openfile, struct token_st * tok)
{
    /* should test to make sure tok->token_str[0] == '(' */
    struct token_st temp_tok;
    long filepos;
    int c;
    int sign_value = 1;
    bool done = false;
    bool getting_real = true;
    FILE * debug_token = nullptr;
    tok->token_str[1] = (char) 0;  /* so we can concatenate later */

    filepos = ftell(openfile);
    if (debugflag == 96)
    {
        debug_token = fopen("frmconst.txt","at");
    }

    while (!done)
    {
        switch (c = frmgetchar(openfile))
        {
CASE_NUM :
        case '.':
            if (debug_token != nullptr)
            {
                fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
            }
            temp_tok.token_str[0] = (char) c;
            break;
        case '-' :
            if (debug_token != nullptr)
            {
                fprintf(debug_token,  "First char is a minus\n");
            }
            sign_value = -1;
            c = frmgetchar(openfile);
            if (c == '.' || isdigit(c))
            {
                if (debug_token != nullptr)
                {
                    fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
                }
                temp_tok.token_str[0] = (char) c;
            }
            else
            {
                if (debug_token != nullptr)
                {
                    fprintf(debug_token,  "First char not a . or NUM\n");
                }
                done = true;
            }
            break;
        default:
            if (debug_token != nullptr)
            {
                fprintf(debug_token,  "First char not a . or NUM\n");
            }
            done = true;
            break;
        }
        if (debug_token != nullptr)
        {
            fprintf(debug_token,  "Calling frmgetconstant unless done is true; done is %s\n", done ? "true" : "false");
        }
        if (!done && frmgetconstant(openfile, &temp_tok))
        {
            c = frmgetchar(openfile);
            if (debug_token != nullptr)
            {
                fprintf(debug_token, "frmgetconstant returned 1; next token is %c\n", c);
            }
            if (getting_real && c == ',')   /* we have the real part now*/
            {
                if (sign_value == -1)
                {
                    strcat(tok->token_str, "-");
                }
                strcat(tok->token_str, temp_tok.token_str);
                strcat(tok->token_str, ",");
                tok->token_const.x = temp_tok.token_const.x * sign_value;
                getting_real = false;
                sign_value = 1;
            }
            else if (!getting_real && c == ')') /* we have the complex part */
            {
                if (sign_value == -1)
                {
                    strcat(tok->token_str, "-");
                }
                strcat(tok->token_str, temp_tok.token_str);
                strcat(tok->token_str, ")");
                tok->token_const.y = temp_tok.token_const.x * sign_value;
                tok->token_type = tok->token_const.y ? COMPLEX_CONSTANT : REAL_CONSTANT;
                tok->token_id   = 0;
                if (debug_token != nullptr)
                {
                    fprintf(debug_token,  "Exiting with type set to %d\n", tok->token_const.y ? COMPLEX_CONSTANT : REAL_CONSTANT);
                    fclose(debug_token);
                }
                return;
            }
            else
                done = true;
        }
        else
            done = true;
    }
    fseek(openfile, filepos, SEEK_SET);
    tok->token_str[1] = (char) 0;
    tok->token_const.y = tok->token_const.x = 0.0;
    tok->token_type = PARENS;
    tok->token_id = OPEN_PARENS;
    if (debug_token != nullptr)
    {
        fprintf(debug_token,  "Exiting with ID set to OPEN_PARENS\n");
        fclose(debug_token);
    }
    return;
}

bool frmgetalpha(FILE * openfile, struct token_st * tok)
{
    int c;
    int i = 1;
    bool var_name_too_long = false;
    long filepos;
    long last_filepos = ftell(openfile);
    while ((c=frmgetchar(openfile)) != EOF && c != '\032')
    {
        filepos = ftell(openfile);
        switch (c)
        {
CASE_ALPHA:
CASE_NUM:
        case '_':
            if (i<79)
                tok->token_str[i++] = (char) c;
            else
            {
                tok->token_str[i] = (char) 0;
            }
            if (i == 33)
            {
                var_name_too_long = true;
            }
            last_filepos = filepos;
            break;
        default:
            if (c == '.')       /* illegal character in variable or func name */
            {
                tok->token_type = NOT_A_TOKEN;
                tok->token_id   = ILLEGAL_VARIABLE_NAME;
                tok->token_str[i++] = '.';
                tok->token_str[i] = (char) 0;
                return false;
            }
            else if (var_name_too_long)
            {
                tok->token_type = NOT_A_TOKEN;
                tok->token_id   = TOKEN_TOO_LONG;
                tok->token_str[i] = (char) 0;
                fseek(openfile, last_filepos, SEEK_SET);
                return false;
            }
            tok->token_str[i] = (char) 0;
            fseek(openfile, last_filepos, SEEK_SET);
            getfuncinfo(tok);
            if (c=='(') /*getfuncinfo() correctly filled structure*/
            {
                if (tok->token_type == NOT_A_TOKEN)
                    return false;
                else if (tok->token_type == FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4))
                {
                    tok->token_type = NOT_A_TOKEN;
                    tok->token_id   = JUMP_WITH_ILLEGAL_CHAR;
                    return false;
                }
                else
                    return true;
            }
            /*can't use function names as variables*/
            else if (tok->token_type == FUNCTION || tok->token_type == PARAM_FUNCTION)
            {
                tok->token_type = NOT_A_TOKEN;
                tok->token_id   = FUNC_USED_AS_VAR;
                return false;
            }
            else if (tok->token_type == FLOW_CONTROL && (tok->token_id == 1 || tok->token_id == 2))
            {
                tok->token_type = NOT_A_TOKEN;
                tok->token_id   = JUMP_MISSING_BOOLEAN;
                return false;
            }
            else if (tok->token_type == FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4))
            {
                if (c == ',' || c == '\n' || c == ':')
                    return true;
                else
                {
                    tok->token_type = NOT_A_TOKEN;
                    tok->token_id   = JUMP_WITH_ILLEGAL_CHAR;
                    return false;
                }
            }
            else
            {
                getvarinfo(tok);
                return true;
            }
        }
    }
    tok->token_str[0] = (char) 0;
    tok->token_type = NOT_A_TOKEN;
    tok->token_id   = END_OF_FILE;
    return false;
}

void frm_get_eos(FILE * openfile, struct token_st * this_token)
{
    long last_filepos = ftell(openfile);
    int c;

    for (c = frmgetchar(openfile); (c == '\n' || c == ',' || c == ':'); c = frmgetchar(openfile))
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
        this_token->token_type = END_OF_FORMULA;
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

/*frmgettoken fills token structure; returns 1 on success and 0 on
  NOT_A_TOKEN and END_OF_FORMULA
*/
static bool frmgettoken(FILE * openfile, struct token_st * this_token)
{
    int c;
    int i=1;
    long filepos;

    switch (c = frmgetchar(openfile))
    {
CASE_NUM:
    case '.':
        this_token->token_str[0] = (char) c;
        return frmgetconstant(openfile, this_token);
CASE_ALPHA:
    case '_':
        this_token->token_str[0] = (char) c;
        return frmgetalpha(openfile, this_token);
CASE_TERMINATOR:
        this_token->token_type = OPERATOR; /* this may be changed below */
        this_token->token_str[0] = (char) c;
        filepos = ftell(openfile);
        if (c=='<' || c=='>' || c=='=')
        {
            c=frmgetchar(openfile);
            if (c == '=')
                this_token->token_str[i++] = (char) c;
            else
            {
                fseek(openfile, filepos, SEEK_SET);
            }
        }
        else if (c=='!')
        {
            c=frmgetchar(openfile);
            if (c == '=')
                this_token->token_str[i++] = (char) c;
            else
            {
                fseek(openfile, filepos, SEEK_SET);
                this_token->token_str[1] = (char) 0;
                this_token->token_type = NOT_A_TOKEN;
                this_token->token_id = ILLEGAL_OPERATOR;
                return false;
            }
        }
        else if (c=='|')
        {
            c=frmgetchar(openfile);
            if (c == '|')
                this_token->token_str[i++] = (char) c;
            else
                fseek(openfile, filepos, SEEK_SET);
        }
        else if (c=='&')
        {
            c=frmgetchar(openfile);
            if (c == '&')
                this_token->token_str[i++] = (char) c;
            else
            {
                fseek(openfile, filepos, SEEK_SET);
                this_token->token_str[1] = (char) 0;
                this_token->token_type = NOT_A_TOKEN;
                this_token->token_id = ILLEGAL_OPERATOR;
                return false;
            }
        }
        else if (this_token->token_str[0] == '}')
        {
            this_token->token_type = END_OF_FORMULA;
            this_token->token_id   = 0;
        }
        else if (this_token->token_str[0] == '\n'
                 || this_token->token_str[0] == ','
                 || this_token->token_str[0] == ':')
        {
            frm_get_eos(openfile, this_token);
        }
        else if (this_token->token_str[0] == ')')
        {
            this_token->token_type = PARENS;
            this_token->token_id = CLOSE_PARENS;
        }
        else if (this_token->token_str[0] == '(')
        {
            /* the following function will set token_type to PARENS and
               token_id to OPEN_PARENS if this is not the start of a
               complex constant */
            is_complex_constant(openfile, this_token);
            return true;
        }
        this_token->token_str[i] = (char) 0;
        if (this_token->token_type == OPERATOR)
        {
            for (int i = 0; i < sizeof(OPList)/sizeof(OPList[0]); i++)
            {
                if (!strcmp(OPList[i], this_token->token_str))
                {
                    this_token->token_id = i;
                }
            }
        }
        return this_token->token_str[0] != '}';
    case EOF:
    case '\032':
        this_token->token_str[0] = (char) 0;
        this_token->token_type = NOT_A_TOKEN;
        this_token->token_id = END_OF_FILE;
        return false;
    default:
        this_token->token_str[0] = (char) c;
        this_token->token_str[1] = (char) 0;
        this_token->token_type = NOT_A_TOKEN;
        this_token->token_id = ILLEGAL_CHARACTER;
        return false;
    }
}

int frm_get_param_stuff(char * Name)
{
    FILE *debug_token = nullptr;
    int c;
    struct token_st current_token;
    FILE * entry_file = nullptr;
    uses_p1 = false;
    uses_p2 = false;
    uses_p3 = false;
    uses_ismand = false;
    maxfn = 0;
    uses_p4 = false;
    uses_p5 = false;

    if (FormName[0] == 0)
    {
        return 0;  /*  and don't reset the pointers  */
    }
    if (find_file_item(FormFileName,Name,&entry_file, 1))
    {
        stopmsg(0, ParseErrs(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
        return 0;
    }
    while ((c=frmgetchar(entry_file)) != '{' && c != EOF && c != '\032')
    {
    }
    if (c != '{')
    {
        stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
        fclose(entry_file);
        return 0;
    }

    if (debugflag == 96)
    {
        if ((debug_token = fopen("frmtokens.txt","at")) != nullptr)
            fprintf(debug_token,"%s\n", Name);
    }
    while (frmgettoken(entry_file, &current_token))
    {
        if (debug_token != nullptr)
        {
            fprintf(debug_token,"%s\n", current_token.token_str);
            fprintf(debug_token,"token_type is %d\n", current_token.token_type);
            fprintf(debug_token,"token_id is %d\n", current_token.token_id);
            if (current_token.token_type == REAL_CONSTANT || current_token.token_type == COMPLEX_CONSTANT)
            {
                fprintf(debug_token,"Real value is %f\n", current_token.token_const.x);
                fprintf(debug_token,"Imag value is %f\n", current_token.token_const.y);
            }
            fprintf(debug_token,"\n");
        }
        switch (current_token.token_type)
        {
        case PARAM_VARIABLE:
            if (current_token.token_id == 1)
                uses_p1 = true;
            else if (current_token.token_id == 2)
                uses_p2 = true;
            else if (current_token.token_id == 8)
                uses_p3 = true;
            else if (current_token.token_id == 13)
                uses_ismand = true;
            else if (current_token.token_id == 17)
                uses_p4 = true;
            else if (current_token.token_id == 18)
                uses_p5 = true;
            break;
        case PARAM_FUNCTION:
            if ((current_token.token_id - 10) > maxfn)
                maxfn = (char)(current_token.token_id - 10);
            break;
        }
    }
    fclose(entry_file);
    if (debug_token)
        fclose(debug_token);
    if (current_token.token_type != END_OF_FORMULA)
    {
        uses_p1 = false;
        uses_p2 = false;
        uses_p3 = false;
        uses_ismand = false;
        maxfn = 0;
        uses_p4 = false;
        uses_p5 = false;
        return 0;
    }
    return 1;
}

/* frm_check_name_and_sym():
     error checking to the open brace on the first line; return true
     on success, and false if errors are found which should cause the
     formula not to be executed
*/
static bool frm_check_name_and_sym(FILE * open_file, bool report_bad_sym)
{
    long filepos = ftell(open_file);
    int c, i;
    bool at_end_of_name = false;

    /* first, test name */
    i = 0;
    bool done = false;
    while (!done)
    {
        switch (c = getc(open_file))
        {
        case EOF:
        case '\032':
            stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
            return false;
        case '\r':
        case '\n':
            stopmsg(0,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
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
                i++;
            break;
        }
    }

    if (i > ITEMNAMELEN)
    {
        int j;
        int k = (int) strlen(ParseErrs(PE_FORMULA_NAME_TOO_LARGE));
        char msgbuf[100];
        strcpy(msgbuf, ParseErrs(PE_FORMULA_NAME_TOO_LARGE));
        strcat(msgbuf, ":\n   ");
        fseek(open_file, filepos, SEEK_SET);
        for (j = 0; j < i && j < 25; j++)
            msgbuf[j+k+2] = (char) getc(open_file);
        msgbuf[j+k+2] = (char) 0;
        stopmsg(STOPMSG_FIXED_FONT, msgbuf);
        return false;
    }
    /* get symmetry */
    symmetry = 0;
    if (c == '(')
    {
        char sym_buf[20];
        done = false;
        i = 0;
        while (!done)
        {
            switch (c = getc(open_file))
            {
            case EOF:
            case '\032':
                stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
                return false;
            case '\r':
            case '\n':
                stopmsg(STOPMSG_FIXED_FONT,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
                return false;
            case '{':
                stopmsg(STOPMSG_FIXED_FONT,ParseErrs(PE_NO_MATCH_RIGHT_PAREN));
                return false;
            case ' ':
            case '\t':
                break;
            case ')':
                done = true;
                break;
            default :
                if (i < 19)
                    sym_buf[i++] = (char) toupper(c);
                break;
            }
        }
        sym_buf[i] = (char) 0;
        for (i = 0; SymStr[i].s[0]; i++)
        {
            if (!stricmp(SymStr[i].s, sym_buf))
            {
                symmetry = SymStr[i].n;
                break;
            }
        }
        if (SymStr[i].s[0] == (char) 0 && report_bad_sym)
        {
            std::string msgbuf{ParseErrs(PE_INVALID_SYM_USING_NOSYM)};
            msgbuf += ":\n   ";
            msgbuf += sym_buf;
            stopmsg(STOPMSG_FIXED_FONT, msgbuf.c_str());
        }
    }
    if (c != '{')
    {
        done = false;
        while (!done)
        {
            switch (c = getc(open_file))
            {
            case EOF:
            case '\032':
                stopmsg(STOPMSG_FIXED_FONT,ParseErrs(PE_UNEXPECTED_EOF));
                return false;
            case '\r':
            case '\n':
                stopmsg(STOPMSG_FIXED_FONT,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
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


static char *PrepareFormula(FILE * File, bool from_prompts1c)
{
    /* This function sets the
       symmetry and converts a formula into a string  with no spaces,
       and one comma after each expression except where the ':' is placed
       and except the final expression in the formula. The open file passed
       as an argument is open in "rb" mode and is positioned at the first
       letter of the name of the formula to be prepared. This function
       is called from RunForm() below.
    */

    FILE *debug_fp = nullptr;
    char *FormulaStr;
    struct token_st temp_tok;
    long filepos = ftell(File);

    /*Test for a repeat*/

    if (!frm_check_name_and_sym(File, from_prompts1c))
    {
        fseek(File, filepos, SEEK_SET);
        return nullptr;
    }
    if (!frm_prescan(File))
    {
        fseek(File, filepos, SEEK_SET);
        return nullptr;
    }

    if (chars_in_formula > 8190)
    {
        fseek(File, filepos, SEEK_SET);
        return nullptr;
    }

    if (debugflag == 96)
    {
        if ((debug_fp = fopen("debugfrm.txt","at")) != nullptr)
        {
            fprintf(debug_fp,"%s\n",FormName);
            if (symmetry != 0)
                fprintf(debug_fp,"%s\n", SymStr[symmetry].s);
        }
    }

    FormulaStr = (char *)boxx;
    FormulaStr[0] = (char) 0; /* To permit concatenation later */

    bool Done = false;

    /*skip opening end-of-lines */
    while (!Done)
    {
        frmgettoken(File, &temp_tok);
        if (temp_tok.token_type == NOT_A_TOKEN)
        {
            stopmsg(STOPMSG_FIXED_FONT, "Unexpected token error in PrepareFormula\n");
            fseek(File, filepos, SEEK_SET);
            if (debug_fp != nullptr)
                fclose(debug_fp);
            return nullptr;
        }
        else if (temp_tok.token_type == END_OF_FORMULA)
        {
            stopmsg(STOPMSG_FIXED_FONT, "Formula has no executable instructions\n");
            fseek(File, filepos, SEEK_SET);
            if (debug_fp != nullptr)
                fclose(debug_fp);
            return nullptr;
        }
        if (temp_tok.token_str[0] == ',')
            ;
        else
        {
            strcat(FormulaStr, temp_tok.token_str);
            Done = true;
        }
    }

    Done = false;
    while (!Done)
    {
        frmgettoken(File, &temp_tok);
        switch (temp_tok.token_type)
        {
        case NOT_A_TOKEN:
            stopmsg(STOPMSG_FIXED_FONT, "Unexpected token error in PrepareFormula\n");
            fseek(File, filepos, SEEK_SET);
            if (debug_fp != nullptr)
                fclose(debug_fp);
            return nullptr;
        case END_OF_FORMULA:
            Done = true;
            fseek(File, filepos, SEEK_SET);
            break;
        default:
            strcat(FormulaStr, temp_tok.token_str);
            break;
        }
    }

    if (debug_fp != nullptr && FormulaStr != nullptr)
        fprintf(debug_fp,"   %s\n",FormulaStr);
    if (debug_fp != nullptr)
        fclose(debug_fp);


    /* sprintf(debugmsg, "Chars in formula per boxx is %u.\n", strlen(FormulaStr));
       stopmsg(0, debugmsg);
    */
    return FormulaStr;
}

int BadFormula()
{
    /*  this is called when a formula is bad, instead of calling  */
    /*     the normal functions which will produce undefined results  */
    return 1;
}

/*  returns true if an error occurred  */
bool RunForm(char *Name, bool from_prompts1c)
{
    FILE * entry_file = nullptr;

    /*  first set the pointers so they point to a fn which always returns 1  */
    curfractalspecific->per_pixel = BadFormula;
    curfractalspecific->orbitcalc = BadFormula;

    if (FormName[0] == 0)
    {
        return true;  /*  and don't reset the pointers  */
    }

    if (find_file_item(FormFileName,Name,&entry_file, 1))
    {
        stopmsg(0, ParseErrs(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
        return true;
    }

    FormStr = PrepareFormula(entry_file, from_prompts1c);
    fclose(entry_file);

    if (FormStr)  /*  No errors while making string */
    {
        parser_allocate();  /*  ParseStr() will test if this alloc worked  */
        if (ParseStr(FormStr,1))
            return true;   /*  parse failed, don't change fn pointers  */
        else
        {
            if (uses_jump && fill_jump_struct())
            {
                stopmsg(0, ParseErrs(PE_ERROR_IN_PARSING_JUMP_STATEMENTS));
                return true;
            }

            /* all parses succeeded so set the pointers back to good functions*/
            curfractalspecific->per_pixel = form_per_pixel;
            curfractalspecific->orbitcalc = Formula;
            return false;
        }
    }
    else
        return true;   /* error in making string*/
}


bool fpFormulaSetup()
{
    /* TODO: when parsera.c contains assembly equivalents, remove !defined(_WIN32) */
#if !defined(XFRACT) && !defined(_WIN32)
    MathType = D_MATH;
    bool RunFormRes = !RunForm(FormName, false); /* RunForm() returns true for failure */
    if (RunFormRes && fpu >=387 && debugflag != 90 && (orbitsave&2) == 0
            && !Randomized)
        return CvtStk() != 0; /* run fast assembler code in parsera.asm */
    return RunFormRes;
#else
    MathType = D_MATH;
    return !RunForm(FormName, false); /* RunForm() returns true for failure */
#endif
}

bool intFormulaSetup()
{
#if defined(XFRACT) || defined(_WIN32)
    static bool been_here = false;
    if (!been_here)
    {
        stopmsg(0, "This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = true;
    }
    return false;
#else
    MathType = L_MATH;
    fg = (double)(1L << bitshift);
    fgLimit = (double)0x7fffffffL / fg;
    ShiftBack = 32 - bitshift;
    return !RunForm(FormName, false);
#endif
}



void init_misc()
{
    static struct ConstArg vv[5];
    static union Arg argfirst,argsecond;
    if (!v)
        v = vv;
    Arg1 = &argfirst;
    Arg2 = &argsecond; /* needed by all the ?Stk* functions */
    fg = (double)(1L << bitshift);
    fgLimit = (double)0x7fffffffL / fg;
    ShiftBack = 32 - bitshift;
    Delta16 = bitshift - 16;
    bitshiftless1 = bitshift-1;
    uses_p1 = false;
    uses_p2 = false;
    uses_p3 = false;
    uses_jump = false;
    uses_ismand = false;
    uses_p4 = false;
    uses_p5 = false;
}


long total_formula_mem;
static void parser_allocate()
{
    /* Note that XFRACT will waste about 6k here for pfls */
    /* Somewhat more memory is now allocated than in v17 here */
    /* however Store and Load were reduced in size to help make up for it */
    long f_size,Store_size,Load_size,v_size, p_size;
    for (int pass = 0; pass < 2; pass++)
    {
        free_workarea();
        if (pass == 0)
        {
            Max_Ops = 2300; /* this value uses up about 64K memory */
            Max_Args = (unsigned)(Max_Ops/2.5);
        }
        f_size = sizeof(void (**)())*Max_Ops;
        Store_size = sizeof(union Arg *)*MAX_STORES;
        Load_size = sizeof(union Arg *)*MAX_LOADS;
        v_size = sizeof(struct ConstArg)*Max_Args;
        p_size = sizeof(struct fls *)*Max_Ops;
        total_formula_mem = f_size + Load_size + Store_size + v_size + p_size /*+ jump_size*/
                            + sizeof(struct PEND_OP)*Max_Ops;

        typespecific_workarea = malloc(f_size + Load_size + Store_size + v_size + p_size);
        f = (void (**)()) typespecific_workarea;
        Store = (union Arg **)(f + Max_Ops);
        Load = (union Arg **)(Store + MAX_STORES);
        v = (struct ConstArg *)(Load + MAX_LOADS);
        pfls = (struct fls *)(v + Max_Args);

        if (pass == 0)
        {
            if (!ParseStr(FormStr, pass))
            {
                /* per Chuck Ebbert, fudge these up a little */
                Max_Ops = posp + 4;
                Max_Args = vsp + 4;
            }
        }
    }
    uses_p1 = false;
    uses_p2 = false;
    uses_p3 = false;
    uses_p4 = false;
    uses_p5 = false;
}

void free_workarea()
{
    if (typespecific_workarea)
    {
        free(typespecific_workarea);
    }
    typespecific_workarea = nullptr;
    Store = (union Arg **) nullptr;
    Load = (union Arg **) nullptr;
    v = (struct ConstArg *) nullptr;
    f = (void (**)()) nullptr;
    pfls = (struct fls *) nullptr;
    total_formula_mem = 0;
}


struct error_data_st {
    long start_pos;
    long error_pos;
    int error_number;
} errors[3];


void frm_error(FILE * open_file, long begin_frm)
{
    struct token_st tok;
    int chars_to_error=0, chars_in_error=0, token_count;
    int statement_len, line_number;
    char msgbuf[900];
    long filepos;
    strcpy(msgbuf, "\n");

    for (int j = 0; j < 3 && errors[j].start_pos; j++)
    {
        bool const initialization_error = errors[j].error_number == PE_SECOND_COLON;
        fseek(open_file, begin_frm, SEEK_SET);
        line_number = 1;
        while (ftell(open_file) != errors[j].error_pos)
        {
            int i = fgetc(open_file);
            if (i == '\n')
            {
                line_number++;
            }
            else if (i == EOF || i == '}')
            {
                stopmsg(0, "Unexpected EOF or end-of-formula in error function.\n");
                fseek(open_file, errors[j].error_pos, SEEK_SET);
                frmgettoken(open_file, &tok); /*reset file to end of error token */
                return;
            }
        }
        sprintf(&msgbuf[(int) strlen(msgbuf)], "Error(%d) at line %d:  %s\n  ", errors[j].error_number, line_number, ParseErrs(errors[j].error_number));
        int i = (int) strlen(msgbuf);
        fseek(open_file, errors[j].start_pos, SEEK_SET);
        statement_len = token_count = 0;
        bool done = false;
        while (!done)
        {
            filepos = ftell(open_file);
            if (filepos == errors[j].error_pos) 
            {
                chars_to_error = statement_len;
                frmgettoken(open_file, &tok);
                chars_in_error = (int) strlen(tok.token_str);
                statement_len += chars_in_error;
                token_count++;
            }
            else
            {
                frmgettoken(open_file, &tok);
                statement_len += (int) strlen(tok.token_str);
                token_count++;
            }
            if ((tok.token_type == END_OF_FORMULA)
                    || (tok.token_type == OPERATOR
                        && (tok.token_id == 0 || tok.token_id == 11))
                    || (tok.token_type == NOT_A_TOKEN && tok.token_id == END_OF_FILE))
            {
                done = true;
                if (token_count > 1 && !initialization_error)
                {
                    token_count--;
                }
            }
        }
        fseek(open_file, errors[j].start_pos, SEEK_SET);
        if (chars_in_error < 74)
        {
            while (chars_to_error + chars_in_error > 74)
            {
                frmgettoken(open_file, &tok);
                chars_to_error -= (int) strlen(tok.token_str);
                token_count--;
            }
        }
        else
        {
            fseek(open_file, errors[j].error_pos, SEEK_SET);
            chars_to_error = 0;
            token_count = 1;
        }
        while ((int) strlen(&msgbuf[i]) <=74 && token_count--)
        {
            frmgettoken(open_file, &tok);
            strcat(msgbuf, tok.token_str);
        }
        fseek(open_file, errors[j].error_pos, SEEK_SET);
        frmgettoken(open_file, &tok);
        if ((int) strlen(&msgbuf[i]) > 74)
            msgbuf[i + 74] = (char) 0;
        strcat(msgbuf, "\n");
        i = (int) strlen(msgbuf);
        while (chars_to_error-- > -2)
            strcat(msgbuf, " ");
        if (errors[j].error_number == PE_TOKEN_TOO_LONG)
        {
            chars_in_error = 33;
        }
        while (chars_in_error-- && (int) strlen(&msgbuf[i]) <=74)
            strcat(msgbuf, "^");
        strcat(msgbuf, "\n");
    }
    stopmsg(8, msgbuf);
    return;
}

void display_var_list()
{
    stopmsg(0, "List of user defined variables:\n");
    for (var_list_st *p = var_list; p; p=p->next_item)
    {
        stopmsg(0, p->name);
    }

}

void display_const_lists()
{
    char msgbuf[800];
    stopmsg(0, "Complex constants are:");
    for (const_list_st *p = complx_list; p; p=p->next_item)
    {
        sprintf(msgbuf, "%f, %f\n", p->complex_const.x, p->complex_const.y);
        stopmsg(0, msgbuf);
    }
    stopmsg(0, "Real constants are:");
    for (const_list_st *p = real_list; p; p=p->next_item)
    {
        sprintf(msgbuf, "%f, %f\n", p->complex_const.x, p->complex_const.y);
        stopmsg(0, msgbuf);
    }
}


struct var_list_st *var_list_alloc()
{
    return (struct var_list_st *) malloc(sizeof(struct var_list_st));
}


struct const_list_st  *const_list_alloc()
{
    return (struct const_list_st *) malloc(sizeof(struct const_list_st));
}

void init_var_list()
{
    var_list_st *temp;
    for (var_list_st *p = var_list; p; p = temp)
    {
        temp = p->next_item;
        free(p);
    }
    var_list = nullptr;
}


void init_const_lists()
{
    const_list_st *temp;
    for (const_list_st *p = complx_list; p; p = temp)
    {
        temp = p->next_item;
        free(p);
    }
    complx_list = nullptr;
    for (const_list_st *p = real_list; p; p = temp)
    {
        temp = p->next_item;
        free(p);
    }
    real_list = nullptr;
}

struct var_list_st * add_var_to_list(struct var_list_st * p, struct token_st tok)
{
    if (p == nullptr)
    {
        p = var_list_alloc();
        if (p == nullptr)
            return nullptr;
        strcpy(p->name, tok.token_str);
        p->next_item = nullptr;
    }
    else if (strcmp(p->name, tok.token_str) == 0)
    {
    }
    else
    {
        p->next_item = add_var_to_list(p->next_item, tok);
        if (p->next_item == nullptr)
            return nullptr;
    }
    return p;
}

struct const_list_st *  add_const_to_list(struct const_list_st * p, struct token_st tok)
{
    if (p == nullptr)
    {
        p = const_list_alloc();
        if (p == nullptr)
            return nullptr;
        p->complex_const.x = tok.token_const.x;
        p->complex_const.y = tok.token_const.y;
        p->next_item = nullptr;
    }
    else if (p->complex_const.x == tok.token_const.x && p->complex_const.y == tok.token_const.y)
    {
    }
    else
    {
        p->next_item = add_const_to_list(p->next_item, tok);
        if (p->next_item == nullptr)
            return nullptr;
    }
    return p;
}

void count_lists()
{
    var_count = 0;
    complx_count = 0;
    real_count = 0;

    for (var_list_st *p = var_list; p; p = p->next_item)
    {
        var_count++;
    }
    for (const_list_st *q = complx_list; q; q = q->next_item)
    {
        complx_count++;
    }
    for (const_list_st *q = real_list; q; q = q->next_item)
    {
        real_count++;
    }
}



/*frm_prescan() takes an open file with the file pointer positioned at
  the beginning of the relevant formula, and parses the formula, token
  by token, for syntax errors. The function also accumulates data for
  memory allocation to be done later.

  The function returns 1 if success, and 0 if errors are found.
*/
bool frm_prescan(FILE * open_file)
{
    long filepos;
    long statement_pos, orig_pos;
    bool done = false;
    struct token_st this_token;
    int errors_found = 0;
    bool ExpectingArg = true;
    bool NewStatement = true;
    bool assignment_ok = true;
    bool already_got_colon = false;
    unsigned long else_has_been_used = 0;
    unsigned long waiting_for_mod = 0;
    int waiting_for_endif = 0;
    int max_parens = sizeof(long) * 8;

    number_of_ops = number_of_loads = number_of_stores = number_of_jumps = (unsigned) 0L;
    chars_in_formula = (unsigned) 0;
    uses_jump = false;
    paren = 0;

    init_var_list();
    init_const_lists();

    orig_pos = statement_pos = ftell(open_file);
    for (int i = 0; i < 3; i++)
    {
        errors[i].start_pos    = 0L;
        errors[i].error_pos    = 0L;
        errors[i].error_number = 0;
    }

    while (!done)
    {
        filepos = ftell(open_file);
        frmgettoken(open_file, &this_token);
        chars_in_formula += (int) strlen(this_token.token_str);
        switch (this_token.token_type)
        {
        case NOT_A_TOKEN:
            assignment_ok = false;
            switch (this_token.token_id)
            {
            case END_OF_FILE:
                stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
                fseek(open_file, orig_pos, SEEK_SET);
                return false;
            case ILLEGAL_CHARACTER:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_ILLEGAL_CHAR;
                }
                break;
            case ILLEGAL_VARIABLE_NAME:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_ILLEGAL_VAR_NAME;
                }
                break;
            case TOKEN_TOO_LONG:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_TOKEN_TOO_LONG;
                }
                break;
            case FUNC_USED_AS_VAR:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_FUNC_USED_AS_VAR;
                }
                break;
            case JUMP_MISSING_BOOLEAN:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_JUMP_NEEDS_BOOLEAN;
                }
                break;
            case JUMP_WITH_ILLEGAL_CHAR:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_NO_CHAR_AFTER_THIS_JUMP;
                }
                break;
            case UNDEFINED_FUNCTION:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_UNDEFINED_FUNCTION;
                }
                break;
            case ILLEGAL_OPERATOR:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_UNDEFINED_OPERATOR;
                }
                break;
            case ILL_FORMED_CONSTANT:
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_INVALID_CONST;
                }
                break;
            default:
                stopmsg(0, "Unexpected arrival at default case in prescan()");
                fseek(open_file, orig_pos, SEEK_SET);
                return false;
            }
            break;
        case PARENS:
            assignment_ok = false;
            NewStatement = false;
            switch (this_token.token_id)
            {
            case OPEN_PARENS:
                if (++paren > max_parens)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_NESTING_TO_DEEP;
                    }
                }
                else if (!ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                    }
                }
                waiting_for_mod = waiting_for_mod << 1;
                break;
            case CLOSE_PARENS:
                if (paren)
                {
                    paren--;
                }
                else
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_NEED_A_MATCHING_OPEN_PARENS;
                    }
                    paren = 0;
                }
                if (waiting_for_mod & 1L)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
                    }
                }
                else
                {
                    waiting_for_mod = waiting_for_mod >> 1;
                }
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                break;
            default:
                break;
            }
            break;
        case PARAM_VARIABLE: /*i.e. p1, p2, p3, p4 or p5*/
            number_of_ops++;
            number_of_loads++;
            NewStatement = false;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            switch (this_token.token_id)
            {
            case 1:
                break;
            case 2:
                break;
            case 8:
                break;
            case 17:
                break;
            case 18:
                break;
            default:
                break;
            }
            ExpectingArg = false;
            break;
        case USER_NAMED_VARIABLE: /* i.e. c, iter, etc. */
            number_of_ops++;
            number_of_loads++;
            NewStatement = false;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            ExpectingArg = false;
            break;
        case PREDEFINED_VARIABLE: /* i.e. z, pixel, whitesq, etc. */
            number_of_ops++;
            number_of_loads++;
            NewStatement = false;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            switch (this_token.token_id)
            {
            case 0:     /* pixel */
                break;
            case 3:     /* z */
                break;
            case 4:     /* LastSqr */
                break;
            case 5:     /* pi */
                break;
            case 6:     /* e */
                break;
            case 7:     /* rand */
                break;
            case 9:     /* whitesq */
                break;
            case 10:    /* scrnpix */
                break;
            case 11:    /* scrnmax */
                break;
            case 12:    /* maxit */
                break;
            case 13:    /* ismand */
                break;
            default:
                break;
            }
            ExpectingArg = false;
            break;
        case REAL_CONSTANT: /* i.e. 4, (4,0), etc.) */
            assignment_ok = false;
            number_of_ops++;
            number_of_loads++;
            NewStatement = false;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            ExpectingArg = false;
            break;
        case COMPLEX_CONSTANT: /* i.e. (1,2) etc. */
            assignment_ok = false;
            number_of_ops++;
            number_of_loads++;
            NewStatement = false;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            ExpectingArg = false;
            break;
        case FUNCTION:
            assignment_ok = false;
            NewStatement = false;
            number_of_ops++;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            switch (this_token.token_id)
            {
            case 0:
                break;
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
            case 8:
                break;
            case 9:
                break;
            case 10:
                break;
            case 15:
                break;
            case 16:
                break;
            case 17:
                break;
            case 18:
                break;
            case 19:
                break;
            case 20:
                break;
            case 21:
                break;
            case 22:
                break;
            case 23:
                break;
            case 24:
                break;
            case 25:
                break;
            case 26:
                break;
            case 27:
                break;
            case 28:
                break;
            case 29:
                break;
            case 30:
                break;
            case 31:
                break;
            case 32:
                break;
            case 33:
                break;
            case 34:
                break;
            default:
                break;
            }
            break;
        case PARAM_FUNCTION:
            assignment_ok = false;
            number_of_ops++;
            if (!ExpectingArg)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                }
            }
            switch (this_token.token_id)
            {
            case 11:
                break;
            case 12:
                break;
            case 13:
                break;
            case 14:
                break;
            default:
                break;
            }
            NewStatement = false;
            break;
        case FLOW_CONTROL:
            assignment_ok = false;
            number_of_ops++;
            number_of_jumps++;
            if (!NewStatement)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_JUMP_NOT_FIRST;
                }
            }
            else
            {
                uses_jump = true;
                switch (this_token.token_id)
                {
                case 1:  /* if */
                    else_has_been_used = else_has_been_used << 1;
                    waiting_for_endif++;
                    break;
                case 2: /*ELSEIF*/
                    number_of_ops += 3; /*else + two clear statements*/
                    number_of_jumps++;  /* this involves two jumps */
                    if (else_has_been_used % 2)
                    {
                        if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                        {
                            errors[errors_found].start_pos      = statement_pos;
                            errors[errors_found].error_pos      = filepos;
                            errors[errors_found++].error_number = PE_ENDIF_REQUIRED_AFTER_ELSE;
                        }
                    }
                    else if (!waiting_for_endif)
                    {
                        if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                        {
                            errors[errors_found].start_pos      = statement_pos;
                            errors[errors_found].error_pos      = filepos;
                            errors[errors_found++].error_number = PE_MISPLACED_ELSE_OR_ELSEIF;
                        }
                    }
                    break;
                case 3: /*ELSE*/
                    if (else_has_been_used % 2)
                    {
                        if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                        {
                            errors[errors_found].start_pos      = statement_pos;
                            errors[errors_found].error_pos      = filepos;
                            errors[errors_found++].error_number = PE_ENDIF_REQUIRED_AFTER_ELSE;
                        }
                    }
                    else if (!waiting_for_endif)
                    {
                        if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                        {
                            errors[errors_found].start_pos      = statement_pos;
                            errors[errors_found].error_pos      = filepos;
                            errors[errors_found++].error_number = PE_MISPLACED_ELSE_OR_ELSEIF;
                        }
                    }
                    else_has_been_used = else_has_been_used | 1;
                    break;
                case 4: /*ENDIF*/
                    else_has_been_used = else_has_been_used >> 1;
                    waiting_for_endif--;
                    if (waiting_for_endif < 0)
                    {
                        if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                        {
                            errors[errors_found].start_pos      = statement_pos;
                            errors[errors_found].error_pos      = filepos;
                            errors[errors_found++].error_number = PE_ENDIF_WITH_NO_IF;
                        }
                        waiting_for_endif = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case OPERATOR:
            number_of_ops++; /*This will be corrected below in certain cases*/
            switch (this_token.token_id)
            {
            case 0:
            case 11:    /* end of statement and : */
                number_of_ops++; /* ParseStr inserts a dummy op*/
                if (paren)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_NEED_MORE_CLOSE_PARENS;
                    }
                    paren = 0;
                }
                if (waiting_for_mod)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
                    }
                    waiting_for_mod = 0;
                }
                if (!ExpectingArg)
                {
                    if (this_token.token_id == 11)
                        number_of_ops += 2;
                    else
                        number_of_ops++;
                }
                else if (!NewStatement)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                if (this_token.token_id == 11 && waiting_for_endif)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_UNMATCHED_IF_IN_INIT_SECTION;
                    }
                    waiting_for_endif = 0;
                }
                if (this_token.token_id == 11 && already_got_colon)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SECOND_COLON;
                    }
                }
                if (this_token.token_id == 11)
                    already_got_colon = true;
                NewStatement = true;
                ExpectingArg = true;
                assignment_ok = true;
                statement_pos = ftell(open_file);
                break;
            case 1:     /* != */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 2:     /* = */
                number_of_ops--; /*this just converts a load to a store*/
                number_of_loads--;
                number_of_stores++;
                if (!assignment_ok)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_ILLEGAL_ASSIGNMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 3:     /* == */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 4:     /* < */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 5:     /* <= */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 6:     /* > */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 7:     /* >= */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 8:     /* | */ /* (half of the modulus operator */
                assignment_ok = false;
                if (!(waiting_for_mod & 1L))
                {
                    number_of_ops--;
                }
                if (!(waiting_for_mod & 1L) && !ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
                    }
                }
                else if ((waiting_for_mod & 1L) && ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                waiting_for_mod = waiting_for_mod ^ 1L; /*switch right bit*/
                break;
            case 9:     /* || */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 10:    /* && */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 12:    /* + */ /* case 11 (":") is up with case 0 */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
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
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 15:    /* / */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                ExpectingArg = true;
                break;
            case 16:    /* ^ */
                assignment_ok = false;
                if (ExpectingArg)
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                    }
                }
                filepos = ftell(open_file);
                frmgettoken(open_file, &this_token);
                if (this_token.token_str[0] == '-')
                {
                    if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                    {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_NO_NEG_AFTER_EXPONENT;
                    }
                }
                else
                    fseek(open_file, filepos, SEEK_SET);
                ExpectingArg = true;
                break;
            default:
                break;
            }
            break;
        case END_OF_FORMULA:
            number_of_ops+= 3; /* Just need one, but a couple of extra just for the heck of it */
            if (paren)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_NEED_MORE_CLOSE_PARENS;
                }
                paren = 0;
            }
            if (waiting_for_mod)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
                }
                waiting_for_mod = 0;
            }
            if (waiting_for_endif)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_IF_WITH_NO_ENDIF;
                }
                waiting_for_endif = 0;
            }
            if (ExpectingArg && !NewStatement)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
                }
                statement_pos = ftell(open_file);
            }

            if (number_of_jumps >= MAX_JUMPS)
            {
                if (!errors_found || errors[errors_found-1].start_pos != statement_pos)
                {
                    errors[errors_found].start_pos      = statement_pos;
                    errors[errors_found].error_pos      = filepos;
                    errors[errors_found++].error_number = PE_TOO_MANY_JUMPS;
                }
            }
            done = true;
            break;
        default:
            break;
        }
        if (errors_found == 3)
            done = true;
    }
    if (errors[0].start_pos)
    {
        frm_error(open_file, orig_pos);
        fseek(open_file, orig_pos, SEEK_SET);
        return false;
    }
    fseek(open_file, orig_pos, SEEK_SET);

    count_lists();

    return true;
}
