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

/*      Chuck Ebbert (CompuServe [76306,1226] ) changed code marked 'CAE fp'    */
/*   for fast 387 floating-point math.  See PARSERA.ASM and PARSERFP.C */
/*   (13 Dec 1992.)  */
/* */
/*   Modified 12 July 1993 by CAE to fix crash when formula not found.  */

#include <string.h>
#include <ctype.h>
#include <time.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"


#ifdef WATCH_MP
double x1, y1, x2, y2;
#endif

#define MAX_OPS 250
#define MAX_ARGS 100
#define MAX_BOXX 8192  /* max size of boxx array */

unsigned Max_Ops  = MAX_OPS;
unsigned Max_Args = MAX_ARGS;

unsigned long number_of_ops, number_of_loads, number_of_stores, number_of_jumps;

struct PEND_OP {
   void (far *f)(void);
   int p;
};

#ifndef XFRACT
/* reuse an array in the decoder */
JUMP_CONTROL_ST far *jump_control = (JUMP_CONTROL_ST far *) sizeofstring;
#else
JUMP_CONTROL_ST jump_control[MAX_JUMPS];
#endif

int jump_index, InitJumpIndex;

static int frm_prescan (FILE * open_file);

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

struct token_st {
   char token_str[80];
   int token_type;
   int token_id;
   _CMPLX token_const;
};


/* CAE fp added MAX_STORES and LOADS */
/* MAX_STORES must be even to make Unix alignment work */
/* TW made dependent on Max_Ops */

#define MAX_STORES ((Max_Ops/4)*2)  /* at most only half the ops can be stores */
#define MAX_LOADS  ((unsigned)(Max_Ops*.8))  /* and 80% can be loads */
/* PB 901103 made some of the following static for safety */

static struct PEND_OP far *o;

#if 0
static void ops_allocate(void);
static void vars_allocate(void);
#endif

struct var_list_st {
   char name[34];
   struct var_list_st far * next_item;
} far * var_list;

struct const_list_st {
   _CMPLX complex_const;
   struct const_list_st far * next_item;
} far * complx_list, far * real_list;

static void parser_allocate(void);

union Arg *Arg1, *Arg2;
/* PB 910417 removed unused "a" array */

/* CAE fp  made some of the following non-static for PARSERA.ASM */
/* Some of these variables should be renamed for safety */
union Arg s[20], far * far *Store, far * far *Load;     /* static CAE fp */
int StoPtr, LodPtr, OpPtr;      /* static CAE fp */
int var_count;
int complx_count;
int real_count;


void (far * far *f)(void) = (void(far * far *)(void))0; /* static CAE fp */

short int ismand = 1;

unsigned int posp, vsp, LastOp;     /* CAE fp made non-static */
static unsigned int n, NextOp, InitN;
static int paren, ExpectingArg;
struct ConstArg far *v = (struct ConstArg far *)0;      /* was static CAE fp */
int InitLodPtr, InitStoPtr, InitOpPtr, LastInitOp;      /* was static CAE fp */
double fgLimit;           /* TIW 05-04-91 */
static int SetRandom;     /* MCP 11-21-91 */
static int Randomized;
static unsigned long RandNum;
short uses_p1, uses_p2, uses_p3, uses_p4, uses_p5, uses_jump;
short uses_ismand;
unsigned int chars_in_formula;

#define ChkFloatDenom(denom)\
    if (fabs(denom) <= DBL_MIN) {\
	if (save_release > 1920) overflow = 1;\
	return;\
    }

#define LastSqr v[4].a

#if (_MSC_VER >= 700)
#pragma code_seg ("parser1_text")     /* place following in an overlay */
#endif

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

static char far *ParseErrs(int which)
{
   int lasterr;
   static FCODE e0[] = {"Should be an Argument"};
   static FCODE e1[] = {"Should be an Operator"};
   static FCODE e2[] = {"')' needs a matching '('"};
   static FCODE e3[] = {"Need more ')'"};
   static FCODE e4[] = {"Undefined Operator"};
   static FCODE e5[] = {"Undefined Function"};
   static FCODE e6[] = {"Table overflow"};
   static FCODE e7[] = {"Didn't find matching ')' in symmetry declaration"};
   static FCODE e8[] = {"No '{' found on first line"};
   static FCODE e9[] = {"Unexpected EOF!"};
   static FCODE e10[] = {"Symmetry below is invalid, will use NOSYM"};
   static FCODE e11[] = {"Formula is too large"};
   static FCODE e12[] = {"Insufficient memory to run fractal type 'formula'"};
   static FCODE e13[] = {"Could not open file where formula located"};
   static FCODE e14[] = {"No characters may precede jump instruction"};
   static FCODE e15[] = {"No characters may follow this jump instruction"};
   static FCODE e16[] = {"Jump instruction missing required (boolean argument)"};
   static FCODE e17[] = {"Next jump after \"else\" must be \"endif\""};
   static FCODE e18[] = {"\"endif\" has no matching \"if\""};
   static FCODE e19[] = {"Misplaced \"else\" or \"elseif()\""};
   static FCODE e20[] = {"\"if()\" in initialization has no matching \"endif\""};
   static FCODE e21[] = {"\"if()\" has no matching \"endif\""};
   static FCODE e22[] = {"Error in parsing jump statements"};
   static FCODE e23[] = {"Formula has too many jump commands"};
   static FCODE e24[] = {"Formula name has too many characters"};
   static FCODE e25[] = {"Only variables are allowed to left of assignment"};
   static FCODE e26[] = {"Illegal variable name"};
   static FCODE e27[] = {"Invalid constant expression"};
   static FCODE e28[] = {"This character not supported by parser"};
   static FCODE e29[] = {"Nesting of parentheses exceeds maximum depth"};
   static FCODE e30[] = {"Unmatched modulus operator \"|\" in this expression"}; /*last one */
   static FCODE e31[] = {"Can't use function name as variable"};
   static FCODE e32[] = {"Negative exponent must be enclosed in parens"};
   static FCODE e33[] = {"Variable or constant exceeds 32 character limit"};
   static FCODE e34[] = {"Only one \":\" permitted in a formula"};
   static FCODE e35[] = {"Invalid ParseErrs code"};
   static PFCODE ErrStrings[] = { e0,e1,e2,e3,e4,e5,
				  e6,e7,e8,e9,e10,
				  e11,e12,e13,e14,e15,
				  e16,e17,e18,e19,e20,
				  e21,e22,e23,e24,e25,
				  e26, e27, e28, e29, e30,
                                  e31, e32, e33, e34, e35
				 };
   lasterr = sizeof(ErrStrings)/sizeof(ErrStrings[0]) - 1;
   if(which > lasterr)
     which = lasterr;
   return((char far *)ErrStrings[which]);
}

#if (_MSC_VER >= 700)
#pragma code_seg ()       /* back to normal segment */
#endif

/* Random number code, MCP 11-21-91 */

unsigned long NewRandNum(void)
{
   return(RandNum = ((RandNum << 15) + rand15()) ^ RandNum);
}

void dRandom(void)
{
   long x, y;

   /* Use the same algorithm as for fixed math so that they will generate
	  the same fractals when the srand() function is used. */
   x = NewRandNum() >> (32 - bitshift);
   y = NewRandNum() >> (32 - bitshift);
   v[7].a.d.x = ((double)x / (1L << bitshift));
   v[7].a.d.y = ((double)y / (1L << bitshift));

}


void SetRandFnct(void)
{
   /* Clear out the seed */
   NewRandNum();
   NewRandNum();
   NewRandNum();
}

void RandomSeed(void)
{
   time_t ltime;

   /* Use the current time to randomize the random number sequence. */
   time(&ltime);
   srand((unsigned int)ltime);

   NewRandNum();
   NewRandNum();
   NewRandNum();
   Randomized = 1;
}

void dStkSRand(void)
{
   dRandom();
}

void (*StkSRand)(void) = dStkSRand;


void dStkLodDup()
{
   Arg1+=2;
   Arg2+=2;
   *Arg2 = *Arg1 = *Load[LodPtr];
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

void dStkStoDup(){}
void dStkStoSqr(){}
void dStkStoSqr0(){}

void dStkLodDbl()
{
   Arg1++;
   Arg2++;
   Arg1->d.x = Load[LodPtr]->d.x * 2.0;
   Arg1->d.y = Load[LodPtr]->d.y * 2.0;
   LodPtr++;
}

void dStkStoDbl(){}
void dStkReal2(){}

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

void dStkAbs(void) {
   Arg1->d.x = fabs(Arg1->d.x);
   Arg1->d.y = fabs(Arg1->d.y);
}

void (*StkAbs)(void) = dStkAbs;

void dStkSqr(void) {
   LastSqr.d.x = Arg1->d.x * Arg1->d.x;
   LastSqr.d.y = Arg1->d.y * Arg1->d.y;
   Arg1->d.y = Arg1->d.x * Arg1->d.y * 2.0;
   Arg1->d.x = LastSqr.d.x - LastSqr.d.y;
   LastSqr.d.x += LastSqr.d.y;
   LastSqr.d.y = 0;
}

void (*StkSqr)(void) = dStkSqr;

void dStkAdd(void) {
   Arg2->d.x += Arg1->d.x;
   Arg2->d.y += Arg1->d.y;
   Arg1--;
   Arg2--;
}

void (*StkAdd)(void) = dStkAdd;

void dStkSub(void) {
   Arg2->d.x -= Arg1->d.x;
   Arg2->d.y -= Arg1->d.y;
   Arg1--;
   Arg2--;
}

void (*StkSub)(void) = dStkSub;

void dStkConj(void) {
   Arg1->d.y = -Arg1->d.y;
}

void (*StkConj)(void) = dStkConj;

void dStkFloor(void) {
   Arg1->d.x = floor(Arg1->d.x);
   Arg1->d.y = floor(Arg1->d.y);
}

void (*StkFloor)(void) = dStkFloor;

void dStkCeil(void) {
   Arg1->d.x = ceil(Arg1->d.x);
   Arg1->d.y = ceil(Arg1->d.y);
}

void (*StkCeil)(void) = dStkCeil;

void dStkTrunc(void) {
   Arg1->d.x = (int)(Arg1->d.x);
   Arg1->d.y = (int)(Arg1->d.y);
}

void (*StkTrunc)(void) = dStkTrunc;

void dStkRound(void) {
   Arg1->d.x = floor(Arg1->d.x+.5);
   Arg1->d.y = floor(Arg1->d.y+.5);
}

void (*StkRound)(void) = dStkRound;

void dStkZero(void) {
   Arg1->d.y = Arg1->d.x = 0.0;
}

void (*StkZero)(void) = dStkZero;

void dStkOne(void) {
   Arg1->d.x = 1.0;
   Arg1->d.y = 0.0;
}

void (*StkOne)(void) = dStkOne;

void dStkReal(void) {
   Arg1->d.y = 0.0;
}

void (*StkReal)(void) = dStkReal;

void dStkImag(void) {
   Arg1->d.x = Arg1->d.y;
   Arg1->d.y = 0.0;
}

void (*StkImag)(void) = dStkImag;

void dStkNeg(void) {
   Arg1->d.x = -Arg1->d.x;
   Arg1->d.y = -Arg1->d.y;
}

void (*StkNeg)(void) = dStkNeg;

void dStkMul(void) {
   FPUcplxmul(&Arg2->d, &Arg1->d, &Arg2->d);
   Arg1--;
   Arg2--;
}

void (*StkMul)(void) = dStkMul;

void dStkDiv(void) {
   FPUcplxdiv(&Arg2->d, &Arg1->d, &Arg2->d);
   Arg1--;
   Arg2--;
}

void (*StkDiv)(void) = dStkDiv;

void dStkMod(void) {
   Arg1->d.x = (Arg1->d.x * Arg1->d.x) + (Arg1->d.y * Arg1->d.y);
   Arg1->d.y = 0.0;
}

void (*StkMod)(void) = dStkMod;

void StkSto(void) {
   *Store[StoPtr++] = *Arg1;
}

void (*PtrStkSto)(void) = StkSto;

void StkLod(void) {
   Arg1++;
   Arg2++;
   *Arg1 = *Load[LodPtr++];
}

void (*PtrStkLod)(void) = StkLod;

void StkClr(void) {
   s[0] = *Arg1;
   Arg1 = &s[0];
   Arg2 = Arg1;
   Arg2--;
}

void (*PtrStkClr)(void) = StkClr;

/* MCP 4-9-91, Added Flip() */

void dStkFlip(void) {
   double t;

   t = Arg1->d.x;
   Arg1->d.x = Arg1->d.y;
   Arg1->d.y = t;
}

void (*StkFlip)(void) = dStkFlip;

void dStkSin(void) {
   double sinx, cosx, sinhy, coshy;

   FPUsincos(&Arg1->d.x, &sinx, &cosx);
   FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
   Arg1->d.x = sinx*coshy;
   Arg1->d.y = cosx*sinhy;
}

void (*StkSin)(void) = dStkSin;

/* The following functions are supported by both the parser and for fn
   variable replacement. TIW 04-22-91 */

void dStkTan(void) {
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

void (*StkTan)(void) = dStkTan;

void dStkTanh(void) {
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

void (*StkTanh)(void) = dStkTanh;

void dStkCoTan(void) {
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

void (*StkCoTan)(void) = dStkCoTan;

void dStkCoTanh(void) {
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

void (*StkCoTanh)(void) = dStkCoTanh;

/* The following functions are not directly used by the parser - support
   for the parser was not provided because the existing parser language
   represents these quite easily. They are used for fn variable support
   in miscres.c but are placed here because they follow the pattern of
   the other parser functions. TIW 04-22-91 */

void dStkRecip(void) {
   double mod;
   mod =Arg1->d.x * Arg1->d.x + Arg1->d.y * Arg1->d.y;
   ChkFloatDenom(mod);
   Arg1->d.x =  Arg1->d.x/mod;
   Arg1->d.y = -Arg1->d.y/mod;
}

void StkIdent(void) { /* do nothing - the function Z */
}
/* End TIW 04-22-91 */

void dStkSinh(void) {
   double siny, cosy, sinhx, coshx;

   FPUsincos(&Arg1->d.y, &siny, &cosy);
   FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
   Arg1->d.x = sinhx*cosy;
   Arg1->d.y = coshx*siny;
}

void (*StkSinh)(void) = dStkSinh;

void dStkCos(void) {
   double sinx, cosx, sinhy, coshy;

   FPUsincos(&Arg1->d.x, &sinx, &cosx);
   FPUsinhcosh(&Arg1->d.y, &sinhy, &coshy);
   Arg1->d.x = cosx*coshy;
   Arg1->d.y = -sinx*sinhy; /* TIW 04-25-91 sign */
}

void (*StkCos)(void) = dStkCos;

/* Bogus version of cos, to replicate bug which was in regular cos till v16: */

void dStkCosXX(void) {
   dStkCos();
   Arg1->d.y = -Arg1->d.y;
}

void (*StkCosXX)(void) = dStkCosXX;

void dStkCosh(void) {
   double siny, cosy, sinhx, coshx;

   FPUsincos(&Arg1->d.y, &siny, &cosy);
   FPUsinhcosh(&Arg1->d.x, &sinhx, &coshx);
   Arg1->d.x = coshx*cosy;
   Arg1->d.y = sinhx*siny;
}

void (*StkCosh)(void) = dStkCosh;

/* TIW added arc functions here 11-25-94 */

void dStkASin(void) {
   Arcsinz(Arg1->d, &(Arg1->d));
}

void (*StkASin)(void) = dStkASin;

void dStkASinh(void) {
   Arcsinhz(Arg1->d, &(Arg1->d));
}

void (*StkASinh)(void) = dStkASinh;

void dStkACos(void) {
   Arccosz(Arg1->d, &(Arg1->d));
}

void (*StkACos)(void) = dStkACos;

void dStkACosh(void) {
   Arccoshz(Arg1->d, &(Arg1->d));
}

void (*StkACosh)(void) = dStkACosh;

void dStkATan(void) {
   Arctanz(Arg1->d, &(Arg1->d));
}

void (*StkATan)(void) = dStkATan;

void dStkATanh(void) {
   Arctanhz(Arg1->d, &(Arg1->d));
}

void (*StkATanh)(void) = dStkATanh;

void dStkSqrt(void) {
   Arg1->d = ComplexSqrtFloat(Arg1->d.x, Arg1->d.y);
}

void (*StkSqrt)(void) = dStkSqrt;

void dStkCAbs(void) {
   Arg1->d.x = sqrt(sqr(Arg1->d.x)+sqr(Arg1->d.y));
   Arg1->d.y = 0.0;
}

void (*StkCAbs)(void) = dStkCAbs;

/* TIW end arc functions 11-25-94 */

void dStkLT(void) {
   Arg2->d.x = (double)(Arg2->d.x < Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkLT)(void) = dStkLT;

void dStkGT(void) {
   Arg2->d.x = (double)(Arg2->d.x > Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkGT)(void) = dStkGT;

void dStkLTE(void) {
   Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkLTE)(void) = dStkLTE;

void dStkGTE(void) {
   Arg2->d.x = (double)(Arg2->d.x >= Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkGTE)(void) = dStkGTE;

void dStkEQ(void) {
   Arg2->d.x = (double)(Arg2->d.x == Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkEQ)(void) = dStkEQ;

void dStkNE(void) {
   Arg2->d.x = (double)(Arg2->d.x != Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkNE)(void) = dStkNE;

void dStkOR(void) {
   Arg2->d.x = (double)(Arg2->d.x || Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkOR)(void) = dStkOR;

void dStkAND(void) {
   Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
   Arg2->d.y = 0.0;
   Arg1--;
   Arg2--;
}

void (*StkAND)(void) = dStkAND;

void dStkLog(void) {
   FPUcplxlog(&Arg1->d, &Arg1->d);
}

void (*StkLog)(void) = dStkLog;

void FPUcplxexp(_CMPLX *x, _CMPLX *z) {
   double e2x, siny, cosy;

   if(fpu >= 387)
      FPUcplxexp387(x, z);
   else {
      e2x = exp(x->x);
      FPUsincos(&x->y, &siny, &cosy);
      z->x = e2x * cosy;
      z->y = e2x * siny;
   }
}

void dStkExp(void) {
   FPUcplxexp(&Arg1->d, &Arg1->d);
}

void (*StkExp)(void) = dStkExp;

void dStkPwr(void) {
   Arg2->d = ComplexPower(Arg2->d, Arg1->d);
   Arg1--;
   Arg2--;
}

void (*StkPwr)(void) = dStkPwr;

void EndInit(void) {
   LastInitOp = OpPtr;
   InitJumpIndex = jump_index;
}

void (*PtrEndInit)(void) = EndInit;

void StkJump (void)
{
   OpPtr =  jump_control[jump_index].ptrs.JumpOpPtr;
   LodPtr = jump_control[jump_index].ptrs.JumpLodPtr;
   StoPtr = jump_control[jump_index].ptrs.JumpStoPtr;
   jump_index = jump_control[jump_index].DestJumpIndex;
}

void dStkJumpOnFalse (void)
{
   if(Arg1->d.x == 0)
      StkJump();
   else
      jump_index++;
}

void (*StkJumpOnFalse)(void) = dStkJumpOnFalse;

void dStkJumpOnTrue (void)
{
   if(Arg1->d.x)
      StkJump();
   else
      jump_index++;
}


void (*StkJumpOnTrue)(void) = dStkJumpOnTrue;

void StkJumpLabel (void)
{
   jump_index++;
}

#if (_MSC_VER >= 700)
#pragma code_seg ("parser1_text")     /* place following in an overlay */
#endif

unsigned SkipWhiteSpace(char *Str) {
   unsigned n, Done;

   for(Done = n = 0; !Done; n++) {
      switch(Str[n]) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
	 break;
      default:
	 Done = 1;
      }
   }
   return(n - 1);
}

/* detect if constant is part of a (a,b) construct */
static int isconst_pair(char *Str) {
   int n,j;
   int answer = 0;
   /* skip past first number */
   for(n = 0; isdigit(Str[n]) || Str[n] == '.'; n++);
   if(Str[n] == ',') {
      j = n + SkipWhiteSpace(&Str[n+1]) + 1;
      if(isdigit(Str[j])
	  || (Str[j] == '-' && (isdigit(Str[j+1]) || Str[j+1] == '.'))
	  || Str[j] == '.') {
	    answer = 1;
      }
   }
   return(answer);
}

struct ConstArg far *isconst(char *Str, int Len) {
   _CMPLX z;
   unsigned n, j;
   /* next line enforces variable vs constant naming convention */
   for(n = 0; n < vsp; n++) {
      if(v[n].len == Len) {
	 if(!strnicmp(v[n].s, Str, Len))
	 {
	    if(n == 1)        /* The formula uses 'p1'. */
	       uses_p1 = 1;
	    if(n == 2)        /* The formula uses 'p2'. */
	       uses_p2 = 1;
	    if(n == 7)        /* The formula uses 'rand'. */
	       RandomSeed();
	    if(n == 8)        /* The formula uses 'p3'. */
	       uses_p3 = 1;
	    if(n == 13)        /* The formula uses 'ismand'. */
	       uses_ismand = 1;
       if(n == 17)        /* The formula uses 'p4'. */
          uses_p4 = 1;
       if(n == 18)        /* The formula uses 'p5'. */
          uses_p5 = 1;
	    if(!isconst_pair(Str))
	       return(&v[n]);
	 }
      }
   }
   v[vsp].s = Str;
   v[vsp].len = Len;
   v[vsp].a.d.x = v[vsp].a.d.y = 0.0;

   if(isdigit(Str[0])
       || (Str[0] == '-' && (isdigit(Str[1]) || Str[1] == '.'))
       || Str[0] == '.') {
      if(o[posp-1].f == StkNeg) {
	 posp--;
	 Str = Str - 1;
	 InitN--;
	 v[vsp].len++;
      }
      for(n = 1; isdigit(Str[n]) || Str[n] == '.'; n++);
      if(Str[n] == ',') {
	 j = n + SkipWhiteSpace(&Str[n+1]) + 1;
	 if(isdigit(Str[j])
	     || (Str[j] == '-' && (isdigit(Str[j+1]) || Str[j+1] == '.'))
	     || Str[j] == '.') {
	    z.y = atof(&Str[j]);
	    for(; isdigit(Str[j]) || Str[j] == '.' || Str[j] == '-'; j++);
	    v[vsp].len = j;
	 }
	 else
	    z.y = 0.0;
      }
      else
	 z.y = 0.0;
      z.x = atof(Str);
      v[vsp].a.d = z;
      v[vsp].s = Str;
   }
   return(&v[vsp++]);
}

struct FNCT_LIST {
   char far *s;              /* TIW 03-31-91 added far */
   void (**ptr)(void);
};

/* TIW 03-30-91 START */
void (*StkTrig0)(void) = dStkSin;
void (*StkTrig1)(void) = dStkSqr;
void (*StkTrig2)(void) = dStkSinh;
void (*StkTrig3)(void) = dStkCosh;

char far * JumpList[] = {
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

   int i;

   for(i = 0; *JumpList[i]; i++)
      if(far_strlen(JumpList[i]) == Len)
	 if(!far_strnicmp(JumpList[i], Str, Len))
	    return i + 1;
   return 0;
}


char maxfn = 0;
/* TIW 03-30-91 STOP */

struct FNCT_LIST far FnctList[] = {   /* TIW 03-31-91 added far */
   {s_sin,   &StkSin},
   {s_sinh,  &StkSinh},
   {s_cos,   &StkCos},
   {s_cosh,  &StkCosh},
   {s_sqr,   &StkSqr},
   {s_log,   &StkLog},
   {s_exp,   &StkExp},
   {s_abs,   &StkAbs},
   {s_conj,  &StkConj},
   {s_real,  &StkReal},
   {s_imag,  &StkImag},
   {s_fn1,   &StkTrig0},   /* TIW 03-30-91 */
   {s_fn2,   &StkTrig1},   /* TIW 03-30-91 */
   {s_fn3,   &StkTrig2},   /* TIW 03-30-91 */
   {s_fn4,   &StkTrig3},   /* TIW 03-30-91 */
   {s_flip,  &StkFlip},    /* MCP 4-9-91 */
   {s_tan,   &StkTan},     /* TIW 04-22-91 */
   {s_tanh,  &StkTanh},    /* TIW 04-22-91 */
   {s_cotan, &StkCoTan},   /* TIW 04-24-91 */
   {s_cotanh,&StkCoTanh},  /* TIW 04-24-91 */
   {s_cosxx, &StkCosXX},   /* PB  04-28-91 */
   {s_srand, &StkSRand},   /* MCP 11-21-91 */
   {s_asin,  &StkASin},    /* TIW 11-26-94 */
   {s_asinh, &StkASinh},   /* TIW 11-26-94 */
   {s_acos,  &StkACos},    /* TIW 11-26-94 */
   {s_acosh, &StkACosh},   /* TIW 11-26-94 */
   {s_atan,  &StkATan},    /* TIW 11-26-94 */
   {s_atanh, &StkATanh},   /* TIW 11-26-94 */
   {s_sqrt,  &StkSqrt},    /* TIW 11-26-94 */
   {s_cabs,  &StkCAbs},    /* TIW 11-26-94 */
   {s_floor, &StkFloor},   /* TIW 06-30-96 */
   {s_ceil,  &StkCeil},    /* TIW 06-30-96 */
   {s_trunc, &StkTrunc},   /* TIW 06-30-96 */
   {s_round, &StkRound},   /* TIW 06-30-96 */
};

struct OP_LIST {
   char *s;
   void (**ptr)(void);
};

struct OP_LIST far OPList[] = {
    {","  , &PtrStkClr  }, /*  0 */
    {"!=" , &StkNE      }, /*  1 */
    {"="  , &PtrStkSto  }, /*  2 */
    {"==" , &StkEQ      }, /*  3 */
    {"<"  , &StkLT      }, /*  4 */
    {"<=" , &StkLTE     }, /*  5 */
    {">"  , &StkGT      }, /*  6 */
    {">=" , &StkGTE     }, /*  7 */
    {"|"  , &StkMod     }, /*  8 */
    {"||" , &StkOR      }, /*  9 */
    {"&&" , &StkAND     }, /* 10 */
    {":"  , &PtrEndInit }, /* 11 */
    {"+"  , &StkAdd     }, /* 12 */
    {"-"  , &StkSub     }, /* 13 */
    {"*"  , &StkMul     }, /* 14 */
    {"/"  , &StkDiv     }, /* 15 */
    {"^"  , &StkPwr     }, /* 16 */
};


void NotAFnct(void) { }
void FnctNotFound(void) { }

/* determine if s names a function and if so which one */
/* TIW 04-22-91 */
int whichfn(char *s, int len)
{
   int out;
   if(len != 3)
      out = 0;
   else if(strnicmp(s,"fn",2))
      out = 0;
   else
      out = atoi(s+2);
   if(out < 1 || out > 4)
      out = 0;
   return(out);
}

#ifndef XFRACT
void (far *isfunct(char *Str, int Len))(void)
#else
void (*isfunct(Str, Len))()
char *Str;
int Len;
#endif
{
   unsigned n;
   int functnum;    /* TIW 04-22-91 */

   n = SkipWhiteSpace(&Str[Len]);
   if(Str[Len+n] == '(') {
      for(n = 0; n < sizeof(FnctList) / sizeof(struct FNCT_LIST); n++) {
	 if(far_strlen(FnctList[n].s) == Len) {        /* TIW 03-31-91 added far */
	    if(!far_strnicmp(FnctList[n].s, Str, Len)) {  /* TIW 03-31-91 added far */
	       /* count function variables */
	       if((functnum = whichfn(Str, Len)) != 0)    /* TIW 04-22-91 */
		  if(functnum > maxfn)                  /* TIW 04-22-91 */
		     maxfn = (char)functnum;                  /* TIW 04-22-91 */
	       return(*FnctList[n].ptr);
	    }
	 }
      }
      return(FnctNotFound);
   }
   return(NotAFnct);
}

void RecSortPrec(void) {
   int ThisOp = NextOp++;
   while(o[ThisOp].p > o[NextOp].p && NextOp < posp)
      RecSortPrec();
   f[OpPtr++] = o[ThisOp].f;
}

static char *Constants[] = {
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
   s_ismand,       /* v[13] */
   "center",       /* v[14] */
   "magxmag",      /* v[15] */
   "rotskew",      /* v[16] */
   "p4",           /* v[17] */
   "p5"            /* v[18] */
};

struct SYMETRY {
   char *s;
   int n;
} SymStr[] = {
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

static int ParseStr(char *Str, int pass) {
   struct ConstArg far *c;
   int ModFlag = 999, Len, Equals = 0, Mod[20], mdstk = 0;
   int jumptype;
   double const_pi, const_e;
   double Xctr, Yctr, Xmagfactor, Rotation, Skew;
   LDBL Magnification;
   SetRandom = Randomized = 0;
   uses_jump = 0;
   jump_index = 0;
   if(pass == 0)
      o = (struct PEND_OP far *)
    ((char far *)typespecific_workarea + total_formula_mem-sizeof(struct PEND_OP) * Max_Ops);
   else if(used_extra == 1)
      o = (struct PEND_OP far *)
    ((char far *)typespecific_workarea + total_formula_mem-sizeof(struct PEND_OP) * Max_Ops);
   else
      o = (struct PEND_OP far *)farmemalloc(sizeof(struct PEND_OP) * (long)Max_Ops);
   if( !o || !typespecific_workarea) {
      stopmsg(0,ParseErrs(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
      return(1);
   }
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
   StkTrig0 = dtrig0;   /* TIW 03-30-91 */
   StkTrig1 = dtrig1;   /* TIW 03-30-91 */
   StkTrig2 = dtrig2;   /* TIW 03-30-91 */
   StkTrig3 = dtrig3;   /* TIW 03-30-91 */
   StkFlip = dStkFlip;
   StkTan = dStkTan;    /* TIW 04-22-91 */
   StkTanh = dStkTanh;  /* TIW 04-22-91 */
   StkCoTan = dStkCoTan;    /* TIW 04-24-91 */
   StkCoTanh = dStkCoTanh;  /* TIW 04-24-91 */
   StkCosXX = dStkCosXX;    /* PB  04-28-91 */
   StkGT  = dStkGT;         /* MCP 11-3-91 */
   StkGTE = dStkGTE;        /* MCP 11-3-91 */
   StkEQ  = dStkEQ;         /* MCP 11-3-91 */
   StkNE  = dStkNE;         /* MCP 11-3-91 */
   StkAND = dStkAND;        /* MCP 11-3-91 */
   StkOR  = dStkOR ;        /* MCP 11-3-91 */
   StkSRand = dStkSRand;    /* MCP 11-21-91 */
   StkASin = dStkASin;      /* TIW 11-25-94 */
   StkASinh = dStkASinh;    /* TIW 11-25-94 */
   StkACos = dStkACos;      /* TIW 11-25-94 */
   StkACosh = dStkACosh;    /* TIW 11-25-94 */
   StkATan = dStkATan;      /* TIW 11-25-94 */
   StkATanh = dStkATanh;    /* TIW 11-25-94 */
   StkCAbs = dStkCAbs;      /* TIW 11-25-94 */
   StkSqrt = dStkSqrt;      /* TIW 11-25-94 */
   StkZero = dStkZero;      /* JCO 12-31-94 */
   StkFloor = dStkFloor;    /* TIW 06-30-96 */
   StkCeil = dStkCeil;      /* TIW 06-30-96 */
   StkTrunc = dStkTrunc;    /* TIW 06-30-96 */
   StkRound = dStkRound;    /* TIW 06-30-96 */
   StkJumpOnTrue  = dStkJumpOnTrue;     /* GGM 02-10-97 */
   StkJumpOnFalse = dStkJumpOnFalse;    /* GGM 02-10-97 */
   StkOne = dStkOne;        /* GGM 10-08-97 */
   maxfn = 0;   /* TIW 03-30-91 */
   for(vsp = 0; vsp < sizeof(Constants) / sizeof(char*); vsp++) {
      v[vsp].s = Constants[vsp];
      v[vsp].len = strlen(Constants[vsp]);
   }
   cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
   const_pi = atan(1.0) * 4;
   const_e  = exp(1.0);
   v[7].a.d.x = v[7].a.d.y = 0.0;
   v[11].a.d.x = (double)xdots;
   v[11].a.d.y = (double)ydots;
   v[12].a.d.x = (double)maxit;
   v[12].a.d.y = 0;
   v[13].a.d.x = (double)ismand;
   v[13].a.d.y = 0;
   v[14].a.d.x = Xctr;
   v[14].a.d.y = Yctr;
   v[15].a.d.x = (double)Magnification;
   v[15].a.d.y = Xmagfactor;
   v[16].a.d.x = Rotation;
   v[16].a.d.y = Skew;

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

   LastInitOp = paren = OpPtr = LodPtr = StoPtr = posp = 0;
   ExpectingArg = 1;
   for(n = 0; Str[n]; n++) {
      if(!Str[n])
	 break;
      InitN = n;
      switch(Str[n]) {
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
	    if(Str[n+1] == '|') {
	       ExpectingArg = 1;
	       n++;
	       o[posp].f = StkOR;
	       o[posp++].p = 7 - (paren + Equals)*15;
	    }
	    else if(ModFlag == paren-1) {
	       paren--;
	       ModFlag = Mod[--mdstk];
	    }
	    else {
	       Mod[mdstk++] = ModFlag;
	       o[posp].f = StkMod;
	       o[posp++].p = 2 - (paren + Equals)*15;
	       ModFlag = paren++;
	    }
	    break;
	 case ',':
	 case ';':
	    if(!ExpectingArg) {
	       ExpectingArg = 1;
	       o[posp].f = (void(far*)(void))0;
	       o[posp++].p = 15;
	       o[posp].f = StkClr;
	       o[posp++].p = -30000;
	       Equals = paren = 0;
	    }
	    break;
	 case ':':
	    ExpectingArg = 1;
	    o[posp].f = (void(far*)(void))0;
	    o[posp++].p = 15;
	    o[posp].f = EndInit;
	    o[posp++].p = -30000;
	    Equals = paren = 0;
	    LastInitOp = 10000;
	    break;
	 case '+':
	    ExpectingArg = 1;
	    o[posp].f = StkAdd;
	    o[posp++].p = 4 - (paren + Equals)*15;
	    break;
	 case '-':
	    if(ExpectingArg) {
	       o[posp].f = StkNeg;
	       o[posp++].p = 2 - (paren + Equals)*15;
	    }
	    else {
	       o[posp].f = StkSub;
	       o[posp++].p = 4 - (paren + Equals)*15;
	       ExpectingArg = 1;
	    }
	    break;
	 case '&':
	    ExpectingArg = 1;
	    n++;
	    o[posp].f = StkAND;
	    o[posp++].p = 7 - (paren + Equals)*15;
	    break;
	 case '!':
	    ExpectingArg = 1;
	    n++;
	    o[posp].f = StkNE;
	    o[posp++].p = 6 - (paren + Equals)*15;
	    break;
	 case '<':
	    ExpectingArg = 1;
	    if(Str[n+1] == '=') {
	       n++;
	       o[posp].f = StkLTE;
	    }
	    else
	       o[posp].f = StkLT;
	    o[posp++].p = 6 - (paren + Equals)*15;
	    break;
	 case '>':
	    ExpectingArg = 1;
	    if(Str[n+1] == '=') {
	       n++;
	       o[posp].f = StkGTE;
	    }
	    else
	       o[posp].f = StkGT;
	    o[posp++].p = 6 - (paren + Equals)*15;
	    break;
	 case '*':
	    ExpectingArg = 1;
	    o[posp].f = StkMul;
	    o[posp++].p = 3 - (paren + Equals)*15;
	    break;
	 case '/':
	    ExpectingArg = 1;
	    o[posp].f = StkDiv;
	    o[posp++].p = 3 - (paren + Equals)*15;
	    break;
	 case '^':
	    ExpectingArg = 1;
	    o[posp].f = StkPwr;
	    o[posp++].p = 2 - (paren + Equals)*15;
	    break;
	 case '=':
	    ExpectingArg = 1;
	    if(Str[n+1] == '=') {
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
	    while(isalnum(Str[n+1]) || Str[n+1] == '.' || Str[n+1] == '_')
	       n++;
	    Len = (n+1)-InitN;
	    ExpectingArg = 0;
	    if((jumptype = isjump(&Str[InitN], Len)) != 0) {
	       uses_jump = 1;
	       switch (jumptype) {
		  case 1:                      /* if */
		     ExpectingArg = 1;
		     jump_control[jump_index++].type = 1;
		     o[posp].f = StkJumpOnFalse;
		     o[posp++].p = 1;
		     break;
		  case 2:                     /* elseif */
		     ExpectingArg = 1;
		     jump_control[jump_index++].type = 2;
		     jump_control[jump_index++].type = 2;
		     o[posp].f = StkJump;
		     o[posp++].p = 1;
		     o[posp].f = (void(far*)(void))0;
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
	    else {
	       o[posp].f = isfunct(&Str[InitN], Len);
	       if(o[posp].f != NotAFnct) {
		  o[posp++].p = 1 - (paren + Equals)*15;
		  ExpectingArg = 1;
	       }
	       else {
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
   o[posp].f = (void(far*)(void))0;
   o[posp++].p = 16;
   NextOp = 0;
   LastOp = posp;
   while(NextOp < posp) {
      if(o[NextOp].f)
	 RecSortPrec();
      else {
	 NextOp++;
	 LastOp--;
      }
   }
   if(pass > 0 && used_extra == 0)
      farmemfree(o);
   return(0);
}

#if (_MSC_VER >= 700)
#pragma code_seg ()       /* back to normal segment */
#endif

int Formula(void) {
   if(FormName[0] == 0 || overflow) return(1);

   LodPtr = InitLodPtr;
   StoPtr = InitStoPtr;
   OpPtr = InitOpPtr;
   jump_index=InitJumpIndex;
   /* Set the random number, MCP 11-21-91 */
   if(SetRandom || Randomized) 
   {
      dRandom();
   }

   Arg1 = &s[0];
   Arg2 = Arg1-1;
   while(OpPtr < (int)LastOp) {
      f[OpPtr]();
      OpPtr++;
   }

   old = new = v[3].a.d;
   return(Arg1->d.x == 0.0);
}

int form_per_pixel(void) {
   if (FormName[0] == 0) return(1);
   overflow = LodPtr = StoPtr = OpPtr = jump_index = 0;
   Arg1 = &s[0];
   Arg2 = Arg1;
   Arg2--;


   v[10].a.d.x = (double)col;
   v[10].a.d.y = (double)row;

   if((row+col)&1)
      v[9].a.d.x = 1.0;
   else
      v[9].a.d.x = 0.0;
   v[9].a.d.y = 0.0;


   /* TW started additions for inversion support here 4/17/94 */
   {
      if(invert) 
      {
	 invertz2(&old);
	 v[0].a.d.x = old.x;
	 v[0].a.d.y = old.y;
      }
      else
      {
	 v[0].a.d.x = dxpixel();
	 v[0].a.d.y = dypixel();
      }
   }

   if(LastInitOp)
      LastInitOp = LastOp;
   while(OpPtr < LastInitOp) {
      f[OpPtr]();
      OpPtr++;
   }
   InitLodPtr = LodPtr;
   InitStoPtr = StoPtr;
   InitOpPtr = OpPtr;
   /* Set old variable for orbits TIW 12-18-93 */
   old = v[3].a.d;

   if(overflow)
      return(0);
   else
      return(1);
}

int fill_if_group(int endif_index, JUMP_PTRS_ST* jump_data)
{
   int i   = endif_index;
   int ljp = endif_index; /* ljp means "last jump processed" */
   while(i > 0) {
      i--;
      switch (jump_control[i].type) {
	 case 1:    /*if(); this concludes processing of this group*/
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

int fill_jump_struct(void)
{ /* Completes all entries in jump structure. Returns 1 on error) */
  /* On entry, jump_index is the number of jump functions in the formula*/
   int i = 0;
   int loadcount = 0;
   int storecount = 0;
   int checkforelse = 0;
   void (*JumpFunc)(void) = NULL;
   int find_new_func = 1;
   JUMP_PTRS_ST jump_data[MAX_JUMPS];

   for (OpPtr = 0; OpPtr < (int) LastOp; OpPtr++) {
      if(find_new_func) {
	 switch (jump_control[i].type) {
	    case 1:
	       JumpFunc = StkJumpOnFalse;
	       break;
	    case 2:
	       checkforelse = !checkforelse;
	       if(checkforelse)
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
	 find_new_func = 0;
      }
      if(*(f[OpPtr]) == StkLod)
	 loadcount++;
      else if(*(f[OpPtr]) == StkSto)
	 storecount++;
      else if(*(f[OpPtr]) == JumpFunc) {
	 jump_data[i].JumpOpPtr = OpPtr;
	 jump_data[i].JumpLodPtr = loadcount;
	 jump_data[i].JumpStoPtr = storecount;
	 i++;
	 find_new_func = 1;
      }
   }

   /* Following for safety only; all should always be false */
   if(i != jump_index || jump_control[i - 1].type != 4
	   || jump_control[0].type != 1) {
      return 1;
   }

   while(i > 0) {
      i--;
      i = fill_if_group(i, jump_data);
   }
   return i < 0 ? 1 : 0;
}

static char *FormStr;

#if (_MSC_VER >= 700)
#pragma code_seg ("parser1_text")     /* place following in an overlay */
#endif

int frmgetchar (FILE * openfile)
{
   int c;
   int done = 0;
   int linewrap = 0;
   while (!done) {
      c = getc(openfile);
      switch (c) {
	 case '\r': case ' ' : case '\t' :
	   break;
	 case '\\':
	   linewrap = 1;
	   break;
	 case ';' :
	   while ((c = getc(openfile)) != '\n' && c != EOF && c != '\032')
	     {}
	   if(c == EOF || c == '\032')
	      done = 1;
	 case '\n' :
	    if(!linewrap)
	       done = 1;
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

void getfuncinfo(struct token_st * tok)
{
   int i;
   for(i=0; i < sizeof(FnctList)/ sizeof(struct FNCT_LIST); i++) {
      if(!far_strcmp(FnctList[i].s, tok->token_str)) {
	 tok->token_id = i;
	 if(i >= 11 && i <= 14)
	    tok->token_type = PARAM_FUNCTION;
	 else
	    tok->token_type = FUNCTION;
	 return;
      }
   }

   for (i=0; i < 4; i++) { /*pick up flow control*/
      if(!far_strcmp(JumpList[i], tok->token_str)) {
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
   int i;

   for(i=0; i < sizeof(Constants) / sizeof(char*); i++) {
      if(!far_strcmp(Constants[i], tok->token_str)) {
	 tok->token_id = i;
	 switch(i) {
       case 1: case 2: case 8: case 13: case 17: case 18:
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

int frmgetconstant(FILE * openfile, struct token_st * tok)
{
   int c;
   int i = 1;
   int getting_base = 1;
   long filepos = ftell(openfile);
   int got_decimal_already = 0;
   int done = 0;
   tok->token_const.x = 0.0;          /*initialize values to 0*/
   tok->token_const.y = 0.0;
   if(tok->token_str[0] == '.')
      got_decimal_already = 1;
   while (!done) {
      switch (c=frmgetchar(openfile)) {
	 case EOF: case '\032':
	    tok->token_str[i] = (char) 0;
	    tok->token_type = NOT_A_TOKEN;
	    tok->token_id   = END_OF_FILE;
	    return 0;
	 CASE_NUM:
	    tok->token_str[i++] = (char) c;
	    filepos=ftell(openfile);
	    break;
	 case '.':
	    if (got_decimal_already || !getting_base) {
	       tok->token_str[i++] = (char) c;
	       tok->token_str[i++] = (char) 0;
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id = ILL_FORMED_CONSTANT;
	       return 0;
	     }
	    else {
	       tok->token_str[i++] = (char) c;
	       got_decimal_already = 1;
	       filepos=ftell(openfile);
	    }
	    break;
	 default :
	    if(c == 'e' && getting_base && (isdigit(tok->token_str[i-1]) || (tok->token_str[i-1] == '.' && i > 1))) {
	       tok->token_str[i++] = (char) c;
	       getting_base = 0;
	       got_decimal_already = 0;
	       filepos=ftell(openfile);
	       if((c = frmgetchar(openfile)) == '-' || c == '+') {
		  tok->token_str[i++] = (char) c;
		  filepos = ftell(openfile);
	       }
	       else {
		  fseek(openfile, filepos, SEEK_SET);
	       }
	    }
	    else if(isalpha(c) || c == '_') {
	       tok->token_str[i++] = (char) c;
	       tok->token_str[i++] = (char) 0;
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id = ILL_FORMED_CONSTANT;
	       return 0;
	    }
	    else if (tok->token_str[i-1] == 'e' || (tok->token_str[i-1] == '.' && i == 1)) {
	       tok->token_str[i++] = (char) c;
	       tok->token_str[i++] = (char) 0;
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id = ILL_FORMED_CONSTANT;
	       return 0;
	    }
	    else {
	       fseek(openfile, filepos, SEEK_SET);
	       tok->token_str[i++] = (char) 0;
	       done = 1;
	    }
	    break;
      }
      if (i == 33 && tok->token_str[32]) {
         tok->token_str[33] = (char) 0;
	 tok->token_type = NOT_A_TOKEN;
	 tok->token_id = TOKEN_TOO_LONG;
	 return 0;
      }
   }    /* end of while loop. Now fill in the value */
   tok->token_const.x = atof(tok->token_str);
   tok->token_type = REAL_CONSTANT;
   tok->token_id   = 0;
   return 1;
}

void is_complex_constant(FILE * openfile, struct token_st * tok)
{
   /* should test to make sure tok->token_str[0] == '(' */
   struct token_st temp_tok;
   long filepos;
   int c;
   int sign_value = 1;
   int done = 0;
   int getting_real = 1;
   FILE * debug_token = NULL;
   tok->token_str[1] = (char) 0;  /* so we can concatenate later */

   filepos = ftell(openfile);
   if (debugflag == 96) {
      debug_token = fopen("frmconst.txt","at");
   }

   while (!done) {
      switch (c = frmgetchar(openfile)) {
	 CASE_NUM : case '.':
	    if (debug_token != NULL) {
	       fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
	    }
	    temp_tok.token_str[0] = (char) c;
	    break;
	 case '-' :
	    if (debug_token != NULL) {
	       fprintf(debug_token,  "First char is a minus\n");
	    }
	    sign_value = -1;
	    if ((c = frmgetchar(openfile)) == '.' || isdigit(c)) {
	       if (debug_token != NULL) {
		  fprintf(debug_token,  "Set temp_tok.token_str[0] to %c\n", c);
	       }
	       temp_tok.token_str[0] = (char) c;
	    }
	    else {
	       if (debug_token != NULL) {
		  fprintf(debug_token,  "First char not a . or NUM\n");
	       }
	       done = 1;
	    }
	    break;
	 default:
	    if (debug_token != NULL) {
	      fprintf(debug_token,  "First char not a . or NUM\n");
	    }
	    done = 1;
	    break;
      }
      if (debug_token != NULL) {
	    fprintf(debug_token,  "Calling frmgetconstant unless done is 1; done is %d\n", done);
      }
      if (!done && frmgetconstant (openfile, &temp_tok)) {
	 c = frmgetchar(openfile);
	 if (debug_token != NULL) {
	    fprintf(debug_token, "frmgetconstant returned 1; next token is %c\n", c);
	 }
	 if (getting_real && c == ',') {   /*we have the real part now*/
	    if (sign_value == -1) {
	      strcat(tok->token_str, "-");
	    }
	    strcat(tok->token_str, temp_tok.token_str);
	    strcat(tok->token_str, ",");
	    tok->token_const.x = temp_tok.token_const.x * sign_value;
	    getting_real = 0;
	    sign_value = 1;
	 }
	 else if (!getting_real && c == ')') { /* we have the complex part */
	    if (sign_value == -1) {
	      strcat(tok->token_str, "-");
	    }
	    strcat(tok->token_str, temp_tok.token_str);
	    strcat(tok->token_str, ")");
	    tok->token_const.y = temp_tok.token_const.x * sign_value;
	    tok->token_type = tok->token_const.y ? COMPLEX_CONSTANT : REAL_CONSTANT;
	    tok->token_id   = 0;
	    if (debug_token != NULL) {
	       fprintf(debug_token,  "Exiting with type set to %d\n", tok->token_const.y ? COMPLEX_CONSTANT : REAL_CONSTANT);
	       fclose (debug_token);
	    }
	    return;
	 }
	 else
	    done = 1;
      }
      else
	 done = 1;
   }
   fseek (openfile, filepos, SEEK_SET);
   tok->token_str[1] = (char) 0;
   tok->token_const.y = tok->token_const.x = 0.0;
   tok->token_type = PARENS;
   tok->token_id = OPEN_PARENS;
   if (debug_token != NULL) {
     fprintf(debug_token,  "Exiting with ID set to OPEN_PARENS\n");
     fclose (debug_token);
   }
   return;
}

int frmgetalpha(FILE * openfile, struct token_st * tok)
{
   int c;
   int i = 1;
   int var_name_too_long = 0;
   long filepos;
   long last_filepos = ftell(openfile);
   while ((c=frmgetchar(openfile)) != EOF && c != '\032') {
      filepos = ftell(openfile);
      switch (c) {
	 CASE_ALPHA: CASE_NUM: case '_':
	    if (i<79)
	       tok->token_str[i++] = (char) c;
	    else {
	       tok->token_str[i] = (char) 0;
	    }
	    if(i == 33) {
	       var_name_too_long=1;
	    }
	    last_filepos = filepos;
	    break;
	 default:
	    if (c == '.') { /*illegal character in variable or func name*/
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id   = ILLEGAL_VARIABLE_NAME;
	       tok->token_str[i++] = '.';
	       tok->token_str[i] = (char) 0;
	       return 0;
	    }
	    else if (var_name_too_long) {
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id   = TOKEN_TOO_LONG;
	       tok->token_str[i] = (char) 0;
	       fseek(openfile, last_filepos, SEEK_SET);
	       return 0;
	    }
	    tok->token_str[i] = (char) 0;
	    fseek(openfile, last_filepos, SEEK_SET);
	    getfuncinfo(tok);
	    if(c=='(') { /*getfuncinfo() correctly filled structure*/
	       if (tok->token_type == NOT_A_TOKEN)
		  return 0;
	       else if (tok->token_type == FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4)) {
		  tok->token_type = NOT_A_TOKEN;
		  tok->token_id   = JUMP_WITH_ILLEGAL_CHAR;
		  return 0;
	       }
	       else
		  return 1;
	    }
	    /*can't use function names as variables*/
	    else if(tok->token_type == FUNCTION || tok->token_type == PARAM_FUNCTION) {
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id   = FUNC_USED_AS_VAR;
	       return 0;
	    }
	    else if(tok->token_type == FLOW_CONTROL && (tok->token_id == 1 || tok->token_id == 2)) {
	       tok->token_type = NOT_A_TOKEN;
	       tok->token_id   = JUMP_MISSING_BOOLEAN;
	       return 0;
	    }
	    else if (tok->token_type == FLOW_CONTROL && (tok->token_id == 3 || tok->token_id == 4)) {
	       if (c == ',' || c == '\n' || c == ':')
		 return 1;
	       else {
		  tok->token_type = NOT_A_TOKEN;
		  tok->token_id   = JUMP_WITH_ILLEGAL_CHAR;
		  return 0;
	       }
	    }
	    else {
	       getvarinfo(tok);
	       return 1;
	    }
      }
   }
   tok->token_str[0] = (char) 0;
   tok->token_type = NOT_A_TOKEN;
   tok->token_id   = END_OF_FILE;
   return 0;
}

void frm_get_eos (FILE * openfile, struct token_st * this_token)
{
   long last_filepos = ftell(openfile);
   int c;
   while ((c = frmgetchar(openfile)) == '\n' || c == ',' || c == ':') {
      if (c == ':') {
    this_token->token_str[0] = ':';
      }
      last_filepos = ftell(openfile);
   }
   if (c == '}') {
      this_token->token_str[0] = '}';
      this_token->token_type = END_OF_FORMULA;
      this_token->token_id   = 0;
   }
   else {
      fseek (openfile, last_filepos, SEEK_SET);
      if (this_token->token_str[0] == '\n') {
    this_token->token_str[0] = ',';
      }
   }
}

/*frmgettoken fills token structure; returns 1 on success and 0 on
  NOT_A_TOKEN and END_OF_FORMULA
*/

int frmgettoken(FILE * openfile, struct token_st * this_token)
{
   int c;
   int i=1;
   long filepos;

   switch (c = frmgetchar(openfile)) {
      CASE_NUM: case '.':
	 this_token->token_str[0] = (char) c;
	 return frmgetconstant(openfile, this_token);
      CASE_ALPHA: case '_':
	 this_token->token_str[0] = (char) c;
	 return frmgetalpha(openfile, this_token);
      CASE_TERMINATOR:
	 this_token->token_type = OPERATOR; /* this may be changed below */
	 this_token->token_str[0] = (char) c;
	 filepos = ftell(openfile);
	 if(c=='<' || c=='>' || c=='=') {
	    if((c=frmgetchar(openfile)) == '=')
	       this_token->token_str[i++] = (char) c;
	    else {
	       fseek(openfile, filepos, SEEK_SET);
	    }
	 }
	 else if(c=='!') {
	    if((c=frmgetchar(openfile)) == '=')
	       this_token->token_str[i++] = (char) c;
	    else {
	       fseek(openfile, filepos, SEEK_SET);
	       this_token->token_str[1] = (char) 0;
	       this_token->token_type = NOT_A_TOKEN;
	       this_token->token_id = ILLEGAL_OPERATOR;
	       return 0;
	    }
	 }
	 else if(c=='|') {
	    if((c=frmgetchar(openfile)) == '|')
	       this_token->token_str[i++] = (char) c;
	    else
	       fseek(openfile, filepos, SEEK_SET);
	 }
	 else if (c=='&') {
	    if((c=frmgetchar(openfile)) == '&')
	       this_token->token_str[i++] = (char) c;
	    else {
	       fseek(openfile, filepos, SEEK_SET);
	       this_token->token_str[1] = (char) 0;
	       this_token->token_type = NOT_A_TOKEN;
	       this_token->token_id = ILLEGAL_OPERATOR;
	       return 0;
	    }
	 }
	 else if(this_token->token_str[0] == '}') {
	    this_token->token_type = END_OF_FORMULA;
	    this_token->token_id   = 0;
	 }
	 else if (this_token->token_str[0] == '\n'
	     || this_token->token_str[0] == ','
	     || this_token->token_str[0] == ':' ) {
	    frm_get_eos(openfile, this_token);
	 }
	 else if (this_token->token_str[0] == ')') {
	    this_token->token_type = PARENS;
	    this_token->token_id = CLOSE_PARENS;
	 }
	 else if (this_token->token_str[0] == '(') {
	      /* the following function will set token_type to PARENS and
		 token_id to OPEN_PARENS if this is not the start of a
		 complex constant */
	    is_complex_constant(openfile, this_token);
	       return 1;
	 }
	 this_token->token_str[i] = (char) 0;
	 if(this_token->token_type == OPERATOR) {
	    for(i=0; i < sizeof(OPList)/sizeof(struct OP_LIST); i++) {
	       if(!far_strcmp(OPList[i].s, this_token->token_str)) {
		  this_token->token_id = i;
	       }
	    }
	 }
	 return this_token->token_str[0] == '}' ? 0 : 1;
      case EOF: case '\032':
	 this_token->token_str[0] = (char) 0;
	 this_token->token_type = NOT_A_TOKEN;
	 this_token->token_id = END_OF_FILE;
	 return 0;
      default:
	 this_token->token_str[0] = (char) c;
	 this_token->token_str[1] = (char) 0;
	 this_token->token_type = NOT_A_TOKEN;
	 this_token->token_id = ILLEGAL_CHARACTER;
	 return 0;
   }
}

int frm_get_param_stuff (char * Name)
{
   FILE *debug_token = NULL;
   int c;
   struct token_st current_token;
   FILE * entry_file = NULL;
   uses_p1 = uses_p2 = uses_p3 = uses_ismand = maxfn = 0;
   uses_p4 = uses_p5 = 0;

   if (FormName[0] == 0 ) {
      return 0;  /*  and don't reset the pointers  */
   }
   if (find_file_item(FormFileName,Name,&entry_file, 1)) {
      stopmsg(0, ParseErrs(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
      return 0;
   }
   while((c=frmgetchar(entry_file)) != '{' && c != EOF && c != '\032')
     {}
   if (c != '{') {
      stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
      fclose(entry_file);
      return 0;
   }

   if(debugflag == 96) {
      if((debug_token = fopen("frmtokens.txt","at")) != NULL)
	 fprintf(debug_token,"%s\n", Name);
   }
   while(frmgettoken(entry_file, &current_token)) {
      if (debug_token != NULL) {
	 fprintf(debug_token,"%s\n", current_token.token_str);
	 fprintf(debug_token,"token_type is %d\n", current_token.token_type);
	 fprintf(debug_token,"token_id is %d\n", current_token.token_id);
	 if (current_token.token_type == REAL_CONSTANT || current_token.token_type == COMPLEX_CONSTANT) {
	    fprintf(debug_token,"Real value is %f\n", current_token.token_const.x);
	    fprintf(debug_token,"Imag value is %f\n", current_token.token_const.y);
	 }
	 fprintf(debug_token,"\n");
      }
      switch (current_token.token_type) {
	 case PARAM_VARIABLE:
	    if(current_token.token_id == 1)
	       uses_p1 = 1;
	    else if(current_token.token_id == 2)
	       uses_p2 = 1;
	    else if(current_token.token_id == 8)
	       uses_p3 = 1;
	    else if(current_token.token_id == 13)
	       uses_ismand = 1;
       else if(current_token.token_id == 17)
          uses_p4 = 1;
       else if(current_token.token_id == 18)
          uses_p5 = 1;
	    break;
	 case PARAM_FUNCTION:
	    if((current_token.token_id - 10) > maxfn)
	       maxfn = (char) (current_token.token_id - 10);
	    break;
      }
   }
   fclose(entry_file);
   if(debug_token)
      fclose(debug_token);
   if (current_token.token_type != END_OF_FORMULA) {
      uses_p1 = uses_p2 = uses_p3 = uses_ismand = maxfn = 0;
      uses_p4 = uses_p5 = 0;
      return 0;
   }
   return 1;
}

/* frm_check_name_and_sym():
     error checking to the open brace on the first line; return 1
     on success, 2 if an invalid symmetry is found, and 0 if errors
     are found which should cause the formula not to be executed
*/

int frm_check_name_and_sym (FILE * open_file, int report_bad_sym)
{
   long filepos = ftell(open_file);
   int c, i, done, at_end_of_name;

     /* first, test name */
   done = at_end_of_name = i = 0;
   while(!done) {
      switch (c = getc(open_file)) {
	 case EOF: case '\032':
	    stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
	    return 0;
	 case '\r': case '\n':
	    stopmsg(0,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
	    return 0;
	 case ' ': case '\t':
	    at_end_of_name = 1;
	    break;
	 case '(': case '{':
	    done = 1;
	    break;
	 default :
	    if (!at_end_of_name)
	       i++;
	    break;
      }
   }

   if(i > ITEMNAMELEN) {
      int j;
      int k = far_strlen(ParseErrs(PE_FORMULA_NAME_TOO_LARGE));
      char msgbuf[100];
      far_strcpy(msgbuf, ParseErrs(PE_FORMULA_NAME_TOO_LARGE));
      strcat(msgbuf, ":\n   ");
      fseek(open_file, filepos, SEEK_SET);
      for(j = 0; j < i && j < 25; j++)
	 msgbuf[j+k+2] = (char) getc(open_file);
      msgbuf[j+k+2] = (char) 0;
      stopmsg(8, msgbuf);
      return 0;
   }
      /* get symmetry */
   symmetry = 0;
   if (c == '(') {
      char sym_buf[20];
      done = i = 0;
      while(!done) {
	 switch (c = getc(open_file)) {
	    case EOF: case '\032':
	       stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
	       return 0;
	    case '\r': case '\n':
	       stopmsg(8,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
	       return 0;
	    case '{':
	       stopmsg(8,ParseErrs(PE_NO_MATCH_RIGHT_PAREN));
	       return 0;
	    case ' ': case '\t':
	       break;
	    case ')':
	       done = 1;
	       break;
	    default :
	       if(i < 19)
		  sym_buf[i++] = (char) toupper(c);
	       break;
	 }
      }
      sym_buf[i] = (char) 0;
      for(i = 0; SymStr[i].s[0]; i++) {
	 if(!stricmp(SymStr[i].s, sym_buf)) {
	    symmetry = SymStr[i].n;
	    break;
	 }
      }
      if(SymStr[i].s[0] == (char) 0 && report_bad_sym) {
	 char far * msgbuf = (char far*) farmemalloc(far_strlen(ParseErrs(PE_INVALID_SYM_USING_NOSYM))
			    + strlen(sym_buf) + 6);
	 far_strcpy(msgbuf, ParseErrs(PE_INVALID_SYM_USING_NOSYM));
	 far_strcat(msgbuf, ":\n   ");
	 far_strcat(msgbuf, sym_buf);
	 stopmsg(8, msgbuf);
	 farmemfree(msgbuf);
      }
   }
   if (c != '{') {
      done = 0;
      while(!done) {
	 switch (c = getc(open_file)) {
	    case EOF: case '\032':
	       stopmsg(8,ParseErrs(PE_UNEXPECTED_EOF));
	       return 0;
	    case '\r': case '\n':
	       stopmsg(8,ParseErrs(PE_NO_LEFT_BRACKET_FIRST_LINE));
	       return 0;
	    case '{':
	       done = 1;
	       break;
	    default :
	       break;
	 }
      }
   }
   return 1;
}

static char *PrepareFormula(FILE * File, int from_prompts1c) {

    /* GGM 5-23-96: replaces FindFormula(). This function sets the
       symmetry and converts a formula into a string  with no spaces,
       and one comma after each expression except where the ':' is placed
       and except the final expression in the formula. The open file passed
       as an argument is open in "rb" mode and is positioned at the first
       letter of the name of the formula to be prepared. This function
       is called from RunForm() below.
    */

   FILE *debug_fp = NULL;
   char *FormulaStr;
   struct token_st temp_tok;
   int Done;
   long filepos = ftell(File);

/* char debugmsg[500];
*/

   /*Test for a repeat*/

   if (frm_check_name_and_sym(File, from_prompts1c) == 0) {
      fseek(File, filepos, SEEK_SET);
      return NULL;
   }
   if(!frm_prescan(File)) {
      fseek(File, filepos, SEEK_SET);
      return NULL;
   }

   if(chars_in_formula > 8190) {
      fseek(File, filepos, SEEK_SET);
      return NULL;
   }

   if(debugflag == 96)
   {
      if((debug_fp = fopen("debugfrm.txt","at")) != NULL) {
	 fprintf(debug_fp,"%s\n",FormName);
	 if(symmetry != 0)
	    fprintf(debug_fp,"%s\n", SymStr[symmetry].s);
      }
   }

   FormulaStr = (char *)boxx;
   FormulaStr[0] = (char) 0; /* To permit concantenation later */

   Done = 0;

    /*skip opening end-of-lines */
   while(!Done) {
      frmgettoken(File, &temp_tok);
      if (temp_tok.token_type == NOT_A_TOKEN) {
	 stopmsg(8, "Unexpected token error in PrepareFormula\n");
	 fseek(File, filepos, SEEK_SET);
	 return NULL;
      }
      else if (temp_tok.token_type == END_OF_FORMULA) {
	 stopmsg(8, "Formula has no executable instructions\n");
	 fseek(File, filepos, SEEK_SET);
	 return NULL;
      }
      if (temp_tok.token_str[0] == ',')
	 ;
      else {
	 strcat(FormulaStr, temp_tok.token_str);
	 Done = 1;
      }
   }

   Done = 0;
   while(!Done) {
      frmgettoken(File, &temp_tok);
      switch (temp_tok.token_type) {
	 case NOT_A_TOKEN:
	    stopmsg(8, "Unexpected token error in PrepareFormula\n");
	    fseek(File, filepos, SEEK_SET);
	    return NULL;
	 case END_OF_FORMULA:
	    Done = 1;
	    fseek(File, filepos, SEEK_SET);
	    break;
	 default:
	    strcat(FormulaStr, temp_tok.token_str);
	    break;
      }
   }

   if(debug_fp != NULL && FormulaStr != NULL)
      fprintf(debug_fp,"   %s\n",FormulaStr);
   if(debug_fp != NULL)
      fclose(debug_fp);


/* sprintf(debugmsg, "Chars in formula per boxx is %u.\n", strlen(FormulaStr));
      v[14].a.l.x = (long)(v[14].a.d.x * fg);
      v[14].a.l.y = (long)(v[14].a.d.y * fg);
      v[15].a.l.x = (long)(v[15].a.d.x * fg);
      v[15].a.l.y = (long)(v[15].a.d.y * fg);
      v[16].a.l.x = (long)(v[16].a.d.x * fg);
      v[16].a.l.y = (long)(v[16].a.d.y * fg);
   stopmsg(0, debugmsg);
*/
   return FormulaStr;
}

int BadFormula(void) {
   /*  moved from Parsera.Asm by CAE  12 July 1993  */

   /*  this is called when a formula is bad, instead of calling  */
   /*     the normal functions which will produce undefined results  */
   return 1;
}

int RunForm(char *Name, int from_prompts1c) {  /*  returns 1 if an error occurred  */

   FILE * entry_file = NULL;

   /*  CAE changed fn 12 July 1993 to fix problem when formula not found  */

   /*  first set the pointers so they point to a fn which always returns 1  */
   curfractalspecific->per_pixel = BadFormula;
   curfractalspecific->orbitcalc = BadFormula;

   if (FormName[0] == 0 ){
      return 1;  /*  and don't reset the pointers  */
   }

   /* TW 5-31-94 add search for FRM files in directory */
   if (find_file_item(FormFileName,Name,&entry_file, 1)) {
      stopmsg(0, ParseErrs(PE_COULD_NOT_OPEN_FILE_WHERE_FORMULA_LOCATED));
      return 1;
   }

   FormStr = PrepareFormula(entry_file, from_prompts1c );
   fclose(entry_file);

   if(FormStr)  /*  No errors while making string */
   {
      parser_allocate();  /*  ParseStr() will test if this alloc worked  */
      if (ParseStr(FormStr,1))
	 return 1;   /*  parse failed, don't change fn pointers  */
      else
      {
	 if(uses_jump == 1 && fill_jump_struct() == 1) {
	    stopmsg(0, ParseErrs(PE_ERROR_IN_PARSING_JUMP_STATEMENTS));
	    return 1;
	 }

	 /* all parses succeeded so set the pointers back to good functions*/
	 curfractalspecific->per_pixel = form_per_pixel;
	 curfractalspecific->orbitcalc = Formula;
	 return 0;
      }
   }
   else
      return 1;   /* error in making string*/
}

int fpFormulaSetup(void) {

   int RunFormRes;              /* CAE fp */
#ifndef XFRACT
   if (fpu > 0) {
	/* CAE changed below for fp */
      RunFormRes = !RunForm(FormName, 0); /* RunForm() returns 1 for failure */
      if (RunFormRes && fpu >=387 && debugflag != 90 && (orbitsave&2) == 0
	    && !Randomized)
	 return CvtStk(); /* run fast assembler code in parsera.asm */
      return RunFormRes;
   }
   else {
      return !RunForm(FormName, 0);
   }
#else
   RunFormRes = !RunForm(FormName, 0); /* RunForm() returns 1 for failure */
#if 0
   if (RunFormRes && fpu == -1 && debugflag != 90 && (orbitsave&2) == 0
         && !Randomized)
      return CvtStk(); /* run fast assembler code in parsera.asm */
#endif
   return RunFormRes;
#endif
}

/* TIW added 06-20-90 so functions can be called from fractals.c */
void init_misc()
{
   static struct ConstArg far vv[5];
   static union Arg argfirst,argsecond;
   if(!v)
      v = vv;
   Arg1 = &argfirst; Arg2 = &argsecond; /* needed by all the ?Stk* functions */
   uses_p1 = uses_p2 = uses_p3 = uses_jump = uses_ismand = 0;
   uses_p4 = uses_p5 = 0;
}


/* PB 910417 here to end changed.
	Allocate sub-arrays from one main farmemalloc, using global variable
	typespecific_workarea; calcfrac.c releases this area when calculation
	ends or is terminated.
	Moved the "f" array to be allocated as part of this.
	*/

long total_formula_mem;
BYTE used_extra = 0;
static void parser_allocate(void)
{
   /* CAE fp changed below for v18 */
   /* Note that XFRACT will waste about 6k here for pfls */
   /* Somewhat more memory is now allocated than in v17 here */
   /* however Store and Load were reduced in size to help make up for it */

   long f_size,Store_size,Load_size,v_size, p_size;
   int pass, is_bad_form=0;
   long end_dx_array;
   /* TW Jan 1 1996 Made two passes to determine actual values of
      Max_Ops and Max_Args. Now use the end of extraseg if possible, so
      if less than 2048x2048 resolution is used, usually no farmemalloc
      calls are needed */
   for(pass = 0; pass < 2; pass++)
   {
      free_workarea();
      if(pass == 0) {
	 Max_Ops = 2300; /* this value uses up about 64K memory */
	 Max_Args = (unsigned)(Max_Ops/2.5);
      }
      f_size = sizeof(void(far * far *)(void)) * Max_Ops;
      Store_size = sizeof(union Arg far *) * MAX_STORES;
      Load_size = sizeof(union Arg far *) * MAX_LOADS;
      v_size = sizeof(struct ConstArg) * Max_Args;
      p_size = sizeof(struct fls far *) * Max_Ops;
      total_formula_mem = f_size+Load_size+Store_size+v_size+p_size /*+ jump_size*/
	 + sizeof(struct PEND_OP) * Max_Ops;
      used_extra = 0;

      if(use_grid)
	 end_dx_array = 2L*(long)(xdots+ydots)*sizeof(double);
      else
	 end_dx_array = 0;

      if(pass == 0 || is_bad_form)
      {
	 typespecific_workarea = (char far *)MK_FP(extraseg,0);
	 used_extra = 1;
      }
      else if(1L<<16 > end_dx_array + total_formula_mem)
      {
	 typespecific_workarea = (char far *)MK_FP(extraseg,0) + end_dx_array;
	 used_extra = 1;
      }
      else if(is_bad_form == 0)
      {
	 typespecific_workarea =
	    farmemalloc((long)(f_size+Load_size+Store_size+v_size+p_size));
	 used_extra = 0;
      }
      f = (void(far * far *)(void))typespecific_workarea;
      Store = (union Arg far * far *)(f + Max_Ops);
      Load = (union Arg far * far *)(Store + MAX_STORES);
      v = (struct ConstArg far *)(Load + MAX_LOADS);
      pfls = (struct fls far *)(v + Max_Args);

      if(pass == 0)
      {
	 if((is_bad_form = ParseStr(FormStr,pass)) == 0)
	 {
	    /* per Chuck Ebbert, fudge these up a little */
	    Max_Ops = posp+4;
	    Max_Args = vsp+4;
	 }
	 typespecific_workarea = NULL;
      }
   }
   uses_p1 = uses_p2 = uses_p3 = uses_p4 = uses_p5 = 0;
}

void free_workarea()
{
   if(typespecific_workarea && used_extra == 0) {
      farmemfree(typespecific_workarea);
   }
   typespecific_workarea = NULL;
   Store = (union Arg far * far *)0;
   Load = (union Arg far * far *)0;
   v = (struct ConstArg far *)0;
   f = (void(far * far *)(void))0;      /* CAE fp */
   pfls = (struct fls far * )0;   /* CAE fp */
   total_formula_mem = 0;

   /* restore extraseg */
   set_grid_pointers();
   fill_dx_array();
}


struct error_data_st {
   long start_pos;
   long error_pos;
   int error_number;
} errors[3];


void frm_error(FILE * open_file, long begin_frm)
{
   struct token_st tok;
/* char debugmsg[500];
*/ int i, chars_to_error=0, chars_in_error=0, token_count;
   int statement_len, line_number;
   int done;
   char msgbuf[900];
   long filepos;
   int j;
   int initialization_error;
   strcpy (msgbuf, "\n");

   for(j=0; j < 3 && errors[j].start_pos; j++) {
      initialization_error = errors[j].error_number == PE_SECOND_COLON ? 1 : 0;
      fseek(open_file, begin_frm, SEEK_SET);
      line_number = 1;
      while (ftell(open_file) != errors[j].error_pos) {
	 if((i = fgetc(open_file)) == '\n') {
	    line_number++;
	 }
	 else if (i == EOF || i == '}') {
	    stopmsg(0, "Unexpected EOF or end-of-formula in error function.\n");
	    fseek (open_file, errors[j].error_pos, SEEK_SET);
	    frmgettoken(open_file, &tok); /*reset file to end of error token */
	    return;
	 }
      }
#ifndef XFRACT
      sprintf(&msgbuf[strlen(msgbuf)], "Error(%d) at line %d:  %Fs\n  ", errors[j].error_number, line_number, ParseErrs(errors[j].error_number));
#else
      sprintf(&msgbuf[strlen(msgbuf)], "Error(%d) at line %d:  %s\n  ", errors[j].error_number, line_number, ParseErrs(errors[j].error_number));
#endif
      i = strlen(msgbuf);
/*    sprintf(debugmsg, "msgbuf is: %s\n and i is %d\n", msgbuf, i);
      stopmsg (0, debugmsg);
*/    fseek(open_file, errors[j].start_pos, SEEK_SET);
      statement_len = token_count = 0;
      done = 0;
      while(!done) {
	 filepos = ftell (open_file);
	 if(filepos == errors[j].error_pos)  {
/*          stopmsg(0, "About to get error token\n");
*/          chars_to_error = statement_len;
	    frmgettoken(open_file, &tok);
	    chars_in_error = strlen(tok.token_str);
	    statement_len += chars_in_error;
	    token_count++;
/*          sprintf(debugmsg, "Error is %s\nChars in error is %d\nChars to error is %d\n", tok.token_str, chars_in_error, chars_to_error);
	    stopmsg (0, debugmsg);
*/       }
	 else {
	    frmgettoken(open_file, &tok);
/*          sprintf(debugmsg, "Just got %s\n", tok.token_str);
	    stopmsg (0, debugmsg);
*/          statement_len += strlen(tok.token_str);
	    token_count++;
	 }
	 if ((tok.token_type == END_OF_FORMULA)
	  || (tok.token_type == OPERATOR
		   && (tok.token_id == 0 || tok.token_id == 11))
	  || (tok.token_type == NOT_A_TOKEN && tok.token_id == END_OF_FILE)){
	    done = 1;
            if (token_count > 1 && !initialization_error) {
	       token_count--;
	    }
	 }
      }
      fseek(open_file, errors[j].start_pos, SEEK_SET);
      if(chars_in_error < 74) {
	 while (chars_to_error + chars_in_error > 74) {
/*          stopmsg(0, "chars in error less than 74, but late in line");
*/          frmgettoken(open_file, &tok);
	    chars_to_error -= strlen(tok.token_str);
	    token_count--;
	 }
      }
      else {
	 fseek(open_file, errors[j].error_pos, SEEK_SET);
	 chars_to_error = 0;
	 token_count = 1;
      }
/*    stopmsg(0, "Back to beginning of statement to build msgbuf");
*/    while (strlen(&msgbuf[i]) <=74 && token_count--) {
	 frmgettoken (open_file, &tok);
	 strcat (msgbuf, tok.token_str);
/*         stopmsg(0, &msgbuf[i]);
*/    }
      fseek (open_file, errors[j].error_pos, SEEK_SET);
      frmgettoken (open_file, &tok);
      if (strlen(&msgbuf[i]) > 74)
	 msgbuf[i + 74] = (char) 0;
      strcat(msgbuf, "\n");
      i = strlen(msgbuf);
      while (chars_to_error-- > -2)
	 strcat (msgbuf, " ");
/*    sprintf(debugmsg, "Going into final line, chars in error is %d", chars_in_error);
      stopmsg(0, debugmsg);
*/
      if(errors[j].error_number == PE_TOKEN_TOO_LONG) {
	 chars_in_error = 33;
      }
      while (chars_in_error-- && strlen(&msgbuf[i]) <=74)
	 strcat (msgbuf, "^");
      strcat (msgbuf, "\n");
   }
   stopmsg (8, msgbuf);
   return;
}

void display_var_list()
{
   struct var_list_st far * p;
   stopmsg(0, "List of user defined variables:\n");
   for (p = var_list; p; p=p->next_item) {
      stopmsg(0, p->name);
   }

}

void display_const_lists()
{
   struct const_list_st far * p;
   char msgbuf[800];
   stopmsg (0, "Complex constants are:");
   for (p = complx_list; p; p=p->next_item) {
      sprintf(msgbuf, "%f, %f\n", p->complex_const.x, p->complex_const.y);
      stopmsg(0, msgbuf);
   }
   stopmsg (0, "Real constants are:");
   for (p = real_list; p; p=p->next_item) {
      sprintf(msgbuf, "%f, %f\n", p->complex_const.x, p->complex_const.y);
      stopmsg(0, msgbuf);
   }
}


struct var_list_st far *var_list_alloc() {
   return (struct var_list_st far*) farmemalloc(sizeof(struct var_list_st));
}


struct const_list_st  far *const_list_alloc() {
   return (struct const_list_st far *) farmemalloc(sizeof(struct const_list_st));
}

void init_var_list()
{
   struct var_list_st far * temp, far * p;
   for (p = var_list; p; p=temp) {
      temp = p->next_item;
      farmemfree(p);
   }
   var_list = NULL;
}


void init_const_lists()
{
   struct const_list_st far * temp, far * p;
   for (p = complx_list; p; p=temp) {
      temp = p->next_item;
      farmemfree(p);
   }
   complx_list = NULL;
   for (p = real_list; p; p=temp) {
      temp = p->next_item;
      farmemfree(p);
   }
   real_list = NULL;
}

struct var_list_st far * add_var_to_list (struct var_list_st far * p, struct token_st tok) {
   if (p == NULL) {
      if ((p = var_list_alloc()) == NULL)
         return NULL;
      far_strcpy(p->name, tok.token_str);
      p->next_item = NULL;
   }
   else if (far_strcmp(p->name, tok.token_str) == 0) {
   }
   else {
      if ((p->next_item = add_var_to_list(p->next_item, tok)) == NULL)
         return NULL;
   }
   return p;
}

struct const_list_st far *  add_const_to_list (struct const_list_st far * p, struct token_st tok) {
   if (p == NULL) {
      if ((p = const_list_alloc()) == NULL)
         return NULL;
      p->complex_const.x = tok.token_const.x;
      p->complex_const.y = tok.token_const.y;
      p->next_item = NULL;
   }
   else if (p->complex_const.x == tok.token_const.x && p->complex_const.y == tok.token_const.y) {
   }
   else
      if ((p->next_item = add_const_to_list(p->next_item, tok)) == NULL)
         return NULL;
   return p;
}

void count_lists()
{
/* char msgbuf[800];
*/ struct var_list_st far * p;
   struct const_list_st far * q;

   var_count = 0;
   complx_count = 0;
   real_count = 0;

   for (p = var_list; p; p=p->next_item) {
      var_count++;
   }
   for (q = complx_list; q; q=q->next_item) {
      complx_count++;
   }
   for (q = real_list; q; q=q->next_item) {
      real_count++;
   }
/*   sprintf(msgbuf, "Number of vars is %d\nNumber of complx is %d\nNumber of real is %d\n", var_count, complx_count, real_count);
   stopmsg(0, msgbuf);
*/
}



/*frm_prescan() takes an open file with the file pointer positioned at
  the beginning of the relevant formula, and parses the formula, token
  by token, for syntax errors. The function also accumulates data for
  memory allocation to be done later.

  The function returns 1 if success, and 0 if errors are found.
*/

int disable_fastparser;
int must_use_float;


int frm_prescan (FILE * open_file)
{
   long filepos;
   int i;
   long statement_pos, orig_pos;
   int done = 0;
   struct token_st this_token;
   int errors_found = 0;
   int ExpectingArg = 1;
   int NewStatement = 1;
   int assignment_ok = 1;
   int already_got_colon = 0;
   unsigned long else_has_been_used = 0;
   unsigned long waiting_for_mod = 0;
   int waiting_for_endif = 0;
   int max_parens = sizeof(long) * 8;
/* char debugmsg[800];
   stopmsg (0, "Entering prescan");
*/

   disable_fastparser = 0;
   must_use_float     = 0;

   number_of_ops = number_of_loads = number_of_stores = number_of_jumps = (unsigned) 0L;
   chars_in_formula = (unsigned) 0;
   uses_jump = (short) 0;
   paren = 0;

   init_var_list();
   init_const_lists();

   orig_pos = statement_pos = ftell(open_file);
   for (i = 0; i < 3; i++) {
      errors[i].start_pos    = 0L;
      errors[i].error_pos    = 0L;
      errors[i].error_number = 0;
   }

   while (!done) {
/*    char msgbuf[80] = "Just got ";
*/    filepos = ftell (open_file);
      frmgettoken (open_file, &this_token);
/*    strcat(msgbuf, this_token.token_str);
      stopmsg (0, msgbuf);
      sprintf (debugmsg, "Errors structure\n0: %ld, %ld, %d\n1: %ld, %ld, %d\n2: %ld, %ld, %d\n\n",
	  errors[0].start_pos, errors[0].error_pos, errors[0].error_number,
	  errors[1].start_pos, errors[1].error_pos, errors[1].error_number,
	  errors[2].start_pos, errors[2].error_pos, errors[2].error_number);
      stopmsg (0, debugmsg);
*/
      chars_in_formula += strlen(this_token.token_str);
      switch (this_token.token_type) {
	 case NOT_A_TOKEN:
	    assignment_ok = 0;
	    switch (this_token.token_id) {
	       case END_OF_FILE:
		  stopmsg(0,ParseErrs(PE_UNEXPECTED_EOF));
		  fseek(open_file, orig_pos, SEEK_SET);
		  return 0;
	       case ILLEGAL_CHARACTER:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_ILLEGAL_CHAR;
		  }
		  break;
	       case ILLEGAL_VARIABLE_NAME:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_ILLEGAL_VAR_NAME;
		  }
		  break;
	       case TOKEN_TOO_LONG:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_TOKEN_TOO_LONG;
		  }
		  break;
	       case FUNC_USED_AS_VAR:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_FUNC_USED_AS_VAR;
		  }
		  break;
	       case JUMP_MISSING_BOOLEAN:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_JUMP_NEEDS_BOOLEAN;
		  }
		  break;
	       case JUMP_WITH_ILLEGAL_CHAR:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_NO_CHAR_AFTER_THIS_JUMP;
		  }
		  break;
	       case UNDEFINED_FUNCTION:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_UNDEFINED_FUNCTION;
		  }
		  break;
	       case ILLEGAL_OPERATOR:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_UNDEFINED_OPERATOR;
		  }
		  break;
	       case ILL_FORMED_CONSTANT:
		  if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		     errors[errors_found].start_pos      = statement_pos;
		     errors[errors_found].error_pos      = filepos;
		     errors[errors_found++].error_number = PE_INVALID_CONST;
		  }
		  break;
	       default:
		  stopmsg(0, "Unexpected arrival at default case in prescan()");
		  fseek(open_file, orig_pos, SEEK_SET);
		  return 0;
	    }
	    break;
	 case PARENS:
	    assignment_ok = 0;
	    NewStatement = 0;
	    switch (this_token.token_id) {
	       case OPEN_PARENS:
		  if(++paren > max_parens) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_NESTING_TO_DEEP;
		     }
		  }
		  else if(!ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
		     }
		  }
		  waiting_for_mod = waiting_for_mod << 1;
		  break;
	       case CLOSE_PARENS:
		  if(paren) {
		     paren--;
                  }                           
		  else {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_NEED_A_MATCHING_OPEN_PARENS;
		     }
		     paren = 0;
		  }
		  if (waiting_for_mod & 1L) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
		     }
		  }
		  else {
		     waiting_for_mod = waiting_for_mod >> 1;
		  }
		  if (ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
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
	    NewStatement = 0;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    switch (this_token.token_id) {
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
	    ExpectingArg = 0;
	    break;
	 case USER_NAMED_VARIABLE: /* i.e. c, iter, etc. */
	    number_of_ops++;
	    number_of_loads++;
	    NewStatement = 0;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    ExpectingArg = 0;
/*            if ((var_list = add_var_to_list (var_list, this_token)) == NULL) {
	       stopmsg(0, ParseErrs(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
	       fseek(open_file, orig_pos, SEEK_SET);
	       init_var_list();
	       init_const_lists();
	       return 0;
	    }
*/          break;
	 case PREDEFINED_VARIABLE: /* i.e. z, pixel, whitesq, etc. */
	    number_of_ops++;
	    number_of_loads++;
	    NewStatement = 0;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    switch (this_token.token_id) {
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
	    ExpectingArg = 0;
	    break;
	 case REAL_CONSTANT: /* i.e. 4, (4,0), etc.) */
	    assignment_ok = 0;
	    number_of_ops++;
	    number_of_loads++;
	    NewStatement = 0;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    ExpectingArg = 0;
/*            if ((real_list = add_const_to_list (real_list, this_token)) == NULL) {
	       stopmsg(0, ParseErrs(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
	       fseek(open_file, orig_pos, SEEK_SET);
	       init_var_list();
	       init_const_lists();
	       return 0;
	    }
*/          break;
	 case COMPLEX_CONSTANT: /* i.e. (1,2) etc. */
	    assignment_ok = 0;
	    number_of_ops++;
	    number_of_loads++;
	    NewStatement = 0;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    ExpectingArg = 0;
/*          if ((complx_list = add_const_to_list (complx_list, this_token)) == NULL) {
	       stopmsg(0, ParseErrs(PE_INSUFFICIENT_MEM_FOR_TYPE_FORMULA));
	       fseek(open_file, orig_pos, SEEK_SET);
	       init_var_list();
	       init_const_lists();
	       return 0;
	     }
*/           break;
	 case FUNCTION:
	    assignment_ok = 0;
	    NewStatement = 0;
	    number_of_ops++;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    switch (this_token.token_id) {
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
	    assignment_ok = 0;
	    NewStatement = 0;
	    number_of_ops++;
	    if(!ExpectingArg) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
	       }
	    }
	    switch (this_token.token_id) {
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
	    NewStatement = 0;
	    break;
	 case FLOW_CONTROL:
	    assignment_ok = 0;
	    number_of_ops++;
	    number_of_jumps++;
	    if(!NewStatement) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_JUMP_NOT_FIRST;
	       }
	    }
	    else {
	       uses_jump = 1;
	       switch (this_token.token_id) {
		  case 1:  /* if */
		     else_has_been_used = else_has_been_used << 1;
		     waiting_for_endif++;
		     break;
		  case 2: /*ELSEIF*/
		     number_of_ops += 3; /*else + two clear statements*/
		     number_of_jumps++;  /* this involves two jumps */
		     if (else_has_been_used % 2) {
			if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			   errors[errors_found].start_pos      = statement_pos;
			   errors[errors_found].error_pos      = filepos;
			   errors[errors_found++].error_number = PE_ENDIF_REQUIRED_AFTER_ELSE;
			}
		     }
		     else if (!waiting_for_endif) {
			if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			   errors[errors_found].start_pos      = statement_pos;
			   errors[errors_found].error_pos      = filepos;
			   errors[errors_found++].error_number = PE_MISPLACED_ELSE_OR_ELSEIF;
			}
		     }
		     break;
		  case 3: /*ELSE*/
		     if (else_has_been_used % 2) {
			if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			   errors[errors_found].start_pos      = statement_pos;
			   errors[errors_found].error_pos      = filepos;
			   errors[errors_found++].error_number = PE_ENDIF_REQUIRED_AFTER_ELSE;
			}
		     }
		     else if (!waiting_for_endif) {
			if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
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
		     if(waiting_for_endif < 0) {
			if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
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
	    switch (this_token.token_id) {
	       case 0: case 11:    /* end of statement and : */
		  number_of_ops++; /* ParseStr inserts a dummy op*/
		  if (paren) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_NEED_MORE_CLOSE_PARENS;
		     }
		     paren = 0;
		  }
		  if (waiting_for_mod) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
		     }
		     waiting_for_mod = 0;
		  }
		  if (!ExpectingArg) {
		     if (this_token.token_id == 11)
			number_of_ops += 2;
		     else
			number_of_ops++;
		  }
		  else if (!NewStatement) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  if (this_token.token_id == 11 && waiting_for_endif) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_UNMATCHED_IF_IN_INIT_SECTION;
		     }
		     waiting_for_endif = 0;
		  }
                  if (this_token.token_id == 11 && already_got_colon) {
                     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
                        errors[errors_found].start_pos      = statement_pos;
                        errors[errors_found].error_pos      = filepos;
                        errors[errors_found++].error_number = PE_SECOND_COLON;
                     }
                  }
                  if (this_token.token_id == 11)
                     already_got_colon = 1;
		  NewStatement = ExpectingArg = assignment_ok = 1;
		  statement_pos = ftell(open_file);
		  break;
	       case 1:     /* != */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 2:     /* = */
		  number_of_ops--; /*this just converts a load to a store*/
		  number_of_loads--;
		  number_of_stores++;
		  if(!assignment_ok) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_ILLEGAL_ASSIGNMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 3:     /* == */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 4:     /* < */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 5:     /* <= */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 6:     /* > */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 7:     /* >= */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 8:     /* | */ /* (half of the modulus operator */
		  assignment_ok = 0;
		  if(!waiting_for_mod & 1L) {
		     number_of_ops--;
		  }
		  if(!(waiting_for_mod & 1L) && !ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_OPERATOR;
		     }
		  }
		  else if ((waiting_for_mod & 1L) && ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  waiting_for_mod = waiting_for_mod ^ 1L; /*switch right bit*/
		  break;
	       case 9:     /* || */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 10:    /* && */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 12:    /* + */ /* case 11 (":") is up with case 0 */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 13:    /* - */
		  assignment_ok = 0;
		  ExpectingArg = 1;
		  break;
	       case 14:    /* * */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 15:    /* / */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  ExpectingArg = 1;
		  break;
	       case 16:    /* ^ */
		  assignment_ok = 0;
		  if(ExpectingArg) {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
		     }
		  }
		  filepos = ftell(open_file);
		  frmgettoken (open_file, &this_token);
		  if (this_token.token_str[0] == '-') {
		     if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
			errors[errors_found].start_pos      = statement_pos;
			errors[errors_found].error_pos      = filepos;
			errors[errors_found++].error_number = PE_NO_NEG_AFTER_EXPONENT;
		     }
		  }
		  else
		     fseek(open_file, filepos, SEEK_SET);
		  ExpectingArg = 1;
		  break;
	       default:
		  break;
	    }
	    break;
	 case END_OF_FORMULA:
	    number_of_ops+= 3; /* Just need one, but a couple of extra just for the heck of it */
	    if (paren) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_NEED_MORE_CLOSE_PARENS;
	       }
	       paren = 0;
	    }
	    if (waiting_for_mod) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_UNMATCHED_MODULUS;
	       }
	       waiting_for_mod = 0;
	    }
	    if (waiting_for_endif) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_IF_WITH_NO_ENDIF;
	       }
	       waiting_for_endif = 0;
	    }
	    if (ExpectingArg && !NewStatement) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_SHOULD_BE_ARGUMENT;
	       }
	       statement_pos = ftell(open_file);
	    }

	    if (number_of_jumps >= MAX_JUMPS) {
	       if(!errors_found || errors[errors_found-1].start_pos != statement_pos) {
		  errors[errors_found].start_pos      = statement_pos;
		  errors[errors_found].error_pos      = filepos;
		  errors[errors_found++].error_number = PE_TOO_MANY_JUMPS;
	       }
	    }



	    done = 1;
	    break;
	 default:
	    break;
      }
      if (errors_found == 3)
	 done = 1;
   }
   if(errors[0].start_pos) {
/*    sprintf (debugmsg, "Errors structure on entering frm_error\n 0: %ld, %ld, %d\n1: %ld, %ld, %d\n2: %ld, %ld, %d\n\n",
	  errors[0].start_pos, errors[0].error_pos, errors[0].error_number,
	  errors[1].start_pos, errors[1].error_pos, errors[1].error_number,
	  errors[2].start_pos, errors[2].error_pos, errors[2].error_number);
      stopmsg (0, debugmsg);
*/    frm_error(open_file, orig_pos);
      fseek(open_file, orig_pos, SEEK_SET);
      return 0;
   }
   fseek(open_file, orig_pos, SEEK_SET);

/*   display_var_list();
   display_const_lists();
*/   count_lists();

/* sprintf(debugmsg, "Chars in formula per prescan() is %u.\n", chars_in_formula);
   stopmsg(0, debugmsg);
*/ return 1;
}

