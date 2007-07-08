/* PARSERFP.C  -- Part of FRACTINT fractal drawer.  */

/*   By Chuck Ebbert  CompuServe [76306, 1226]  */
/*                     internet: 76306.1226@compuserve.com  */

/* Fast floating-point parser code.  The functions beginning with  */
/*    "fStk" are in PARSERA.ASM.  PARSER.C calls this code after  */
/*    it has parsed the formula.  */

/*   Converts the function pointers/load pointers/store pointers  */
/*       built by parsestr() into an optimized array of function  */
/*       pointer/operand pointer pairs.  */

/*   Also generates executable code in memory.  */
/*       Define the varible COMPILER to generate executable code.  */
/*       COMPILER must also be defined in PARSERA.ASM. */

/* ******************************************************************* */
/*                                                                     */
/*  (c) Copyright 1992-1995 Chuck Ebbert.  All rights reserved.        */
/*                                                                     */
/*    This code may be freely distributed and used in non-commercial   */
/*    programs provided the author is credited either during program   */
/*    execution or in the documentation, and this copyright notice     */
/*    is left intact.  Sale of this code, or its use in any commercial */
/*    product requires permission from the author.  Nominal            */
/*    distribution and handling fees may be charged by shareware and   */
/*    freeware distributors.                                           */
/*                                                                     */
/* ******************************************************************* */

/* Uncomment the next line to enable debug messages.  */
#define TESTFP 1

/* Use startup parameter "debugflag = 324" to show debug messages after  */
/*    compiling with above #define uncommented.  */

#include <ctype.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "realdos.h"

#include "Formula.h"

#if !defined(XFRACT)

/* not moved to PROTOTYPE.H because these only communicate within
	PARSER.C and other parser modules */

typedef void OLD_FN();  /* old C functions  */

OLD_FN StkLod;
OLD_FN StkClr;
OLD_FN StkSto;
OLD_FN EndInit;
OLD_FN StkJumpLabel;
OLD_FN dStkAdd;
OLD_FN dStkSub;
OLD_FN dStkMul;
OLD_FN dStkDiv;
OLD_FN dStkSqr;
OLD_FN dStkMod;
OLD_FN dStkSin;
OLD_FN dStkCos;
OLD_FN dStkSinh;
OLD_FN dStkCosh;
OLD_FN dStkCosXX;
OLD_FN dStkTan;
OLD_FN dStkTanh;
OLD_FN dStkCoTan;
OLD_FN dStkCoTanh;
OLD_FN dStkLog;
OLD_FN dStkExp;
OLD_FN dStkPwr;
OLD_FN dStkLT;
OLD_FN dStkLTE;
OLD_FN dStkFlip;
OLD_FN dStkReal;
OLD_FN dStkImag;
OLD_FN dStkConj;
OLD_FN dStkNeg;
OLD_FN dStkAbs;
OLD_FN dStkRecip;
OLD_FN StkIdent;
OLD_FN dStkGT;
OLD_FN dStkGTE;
OLD_FN dStkNE;
OLD_FN dStkEQ;
OLD_FN dStkAND;
OLD_FN dStkOR;
OLD_FN dStkZero;
OLD_FN dStkSqrt;
OLD_FN dStkASin;
OLD_FN dStkACos;
OLD_FN dStkASinh;
OLD_FN dStkACosh;
OLD_FN dStkATanh;
OLD_FN dStkATan;
OLD_FN dStkCAbs;
OLD_FN dStkFloor;
OLD_FN dStkCeil;
OLD_FN dStkTrunc;
OLD_FN dStkRound;
OLD_FN StkJump;
OLD_FN dStkJumpOnTrue;
OLD_FN dStkJumpOnFalse;
OLD_FN dStkOne;

typedef void NEW_FN();  /* new 387-only ASM functions  */

NEW_FN fStkPull2;  /* pull up fpu stack from 2 to 4  */
NEW_FN fStkPush2;  /* push down fpu stack from 8 to 6  */
NEW_FN fStkPush2a;  /* push down fpu stack from 6 to 4  */
NEW_FN fStkPush4;  /* push down fpu stack from 8 to 4  */
NEW_FN fStkLodDup;  /* lod, dup  */
NEW_FN fStkLodSqr;  /* lod, sqr, dont save magnitude(i.e. lastsqr)  */
NEW_FN fStkLodSqr2;  /* lod, sqr, save lastsqr  */
NEW_FN fStkStoDup;  /* store, duplicate  */
NEW_FN fStkStoSqr;  /* store, sqr, save lastsqr  */
NEW_FN fStkStoSqr0;  /* store, sqr, dont save lastsqr  */
NEW_FN fStkLodDbl;  /* load, double  */
NEW_FN fStkStoDbl;  /* store, double  */
NEW_FN fStkReal2;  /* fast ver. of real  */
NEW_FN fStkSqr;  /* sqr, save magnitude in lastsqr  */
NEW_FN fStkSqr0;  /* sqr, no save magnitude  */
NEW_FN fStkClr1;  /* clear fpu  */
NEW_FN fStkClr2;  /* test stack top, clear fpu  */
NEW_FN fStkStoClr1;  /* store, clr1  */
NEW_FN fStkAdd;
NEW_FN fStkSub;
NEW_FN fStkSto;
NEW_FN fStkSto2;  /* fast ver. of sto  */
NEW_FN fStkLod;
NEW_FN fStkEndInit;
NEW_FN fStkMod;
NEW_FN fStkMod2;  /* faster mod  */
NEW_FN fStkLodMod2;
NEW_FN fStkStoMod2;
NEW_FN fStkLTE;
NEW_FN fStkLodLTEMul;
NEW_FN fStkLTE2;
NEW_FN fStkLodLTE;
NEW_FN fStkLodLTE2;
NEW_FN fStkLodLTEAnd2;
NEW_FN fStkLT;
NEW_FN fStkLodLTMul;
NEW_FN fStkLT2;
NEW_FN fStkLodLT;
NEW_FN fStkLodLT2;
NEW_FN fStkGTE;
NEW_FN fStkLodGTE;
NEW_FN fStkLodGTE2;
NEW_FN fStkGT;
NEW_FN fStkGT2;
NEW_FN fStkLodGT;
NEW_FN fStkLodGT2;
NEW_FN fStkEQ;
NEW_FN fStkLodEQ;
NEW_FN fStkNE;
NEW_FN fStkLodNE;
NEW_FN fStkAND;
NEW_FN fStkANDClr2;
NEW_FN fStkOR;
NEW_FN fStkORClr2;
NEW_FN fStkSin;
NEW_FN fStkSinh;
NEW_FN fStkCos;
NEW_FN fStkCosh;
NEW_FN fStkCosXX;
NEW_FN fStkTan;
NEW_FN fStkTanh;
NEW_FN fStkCoTan;
NEW_FN fStkCoTanh;
NEW_FN fStkLog;
NEW_FN fStkExp;
NEW_FN fStkPwr;
NEW_FN fStkMul;
NEW_FN fStkDiv;
NEW_FN fStkFlip;
NEW_FN fStkReal;
NEW_FN fStkImag;
NEW_FN fStkRealFlip;
NEW_FN fStkImagFlip;
NEW_FN fStkConj;
NEW_FN fStkNeg;
NEW_FN fStkAbs;
NEW_FN fStkRecip;
NEW_FN fStkLodReal;
NEW_FN fStkLodRealC;
NEW_FN fStkLodImag;
NEW_FN fStkLodRealFlip;
NEW_FN fStkLodRealAbs;
NEW_FN fStkLodRealMul;
NEW_FN fStkLodRealAdd;
NEW_FN fStkLodRealSub;
NEW_FN fStkLodRealPwr;
NEW_FN fStkLodImagMul;
NEW_FN fStkLodImagAdd;
NEW_FN fStkLodImagSub;
NEW_FN fStkLodImagFlip;
NEW_FN fStkLodImagAbs;
NEW_FN fStkLodConj;
NEW_FN fStkLodAdd;
NEW_FN fStkLodSub;
NEW_FN fStkLodSubMod;
NEW_FN fStkLodMul;
NEW_FN fStkPLodAdd;
NEW_FN fStkPLodSub;  /* push-lod-add/sub  */
NEW_FN fStkIdent;
NEW_FN fStkStoClr2;  /* store, clear stack by popping  */
NEW_FN fStkZero;  /* to support new parser fn.  */
NEW_FN fStkDbl;  /* double the stack top */
NEW_FN fStkOne;
NEW_FN fStkSqr3;  /* sqr3 is sqr/mag of a real */
NEW_FN fStkSqrt;
NEW_FN fStkASin;
NEW_FN fStkACos;
NEW_FN fStkASinh;
NEW_FN fStkACosh;
NEW_FN fStkATanh;
NEW_FN fStkATan;
NEW_FN fStkCAbs;
NEW_FN fStkFloor;
NEW_FN fStkCeil;
NEW_FN fStkTrunc;
NEW_FN fStkRound; /* rounding functions */
NEW_FN fStkJump;
NEW_FN fStkJumpOnTrue;
NEW_FN fStkJumpOnFalse;
NEW_FN fStkJumpLabel; /* flow */
NEW_FN fStkOne;   /* to support new parser fn.  */

/* remove push operator from stack top  */
#define REMOVE_PUSH			\
	do						\
	{						\
		--s_convert_index;	\
		s_stack_count += 2;	\
	}						\
	while (0)

#define CLEAR_STK 127
#define MAX_STACK 8   /* max # of stack register avail  */

#ifdef TESTFP
int pstopmsg(int x, char *msg)
{
	static FILE *fp = NULL;
	if (fp == NULL)
	{
		fp = fopen("fpdebug.txt", "wt");
	}
	if (fp)
	{
		fprintf(fp, "%s\n", msg);
		fflush(fp);
	}
	return x; /* just to quiet warnings */
}

#define stop_message pstopmsg

#define DBUGMSG(y) if (DEBUGMODE_NO_HELP_F1_ESC == g_debug_mode || DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode) stop_message(0, (y))
#define DBUGMSG1(y, p) \
		if (DEBUGMODE_NO_HELP_F1_ESC == g_debug_mode || DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode){ \
			sprintf(cDbgMsg, (y), (p)); \
			stop_message(0, cDbgMsg); \
		}
#define DBUGMSG2(y, p, q) \
		if (DEBUGMODE_NO_HELP_F1_ESC == g_debug_mode || DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode){ \
			sprintf(cDbgMsg, (y), (p), (q)); \
			stop_message(0, cDbgMsg); \
		}
#define DBUGMSG3(y, p, q, r) \
		if (DEBUGMODE_NO_HELP_F1_ESC == g_debug_mode || DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode){ \
			sprintf(cDbgMsg, (y), (p), (q), (r)); \
			stop_message(0, cDbgMsg); \
		}
#define DBUGMSG4(y, p, q, r, s) \
		if (DEBUGMODE_NO_HELP_F1_ESC == g_debug_mode || DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode){ \
			sprintf(cDbgMsg, (y), (p), (q), (r), (s)); \
			stop_message(0, cDbgMsg); \
		}
#else

#define DBUGMSG(y)
#define DBUGMSG1(y, p)
#define DBUGMSG2(y, p, q)
#define DBUGMSG3(y, p, q, r)
#define DBUGMSG4(y, p, q, r, s)
#endif /* TESTFP */

enum FunctionEntryType
{
	FN_LOD = 0,
	FN_CLR = 1,
	FN_ADD = 2,
	FN_SUB = 3,
	FN_MUL = 4,
	FN_DIV = 5,
	FN_STO = 6,
	FN_SQR = 7,
	FN_ENDINIT = 8,
	FN_MOD = 9,
	FN_LTE = 10,
	FN_SIN = 11,
	FN_COS = 12,
	FN_SINH = 13,
	FN_COSH = 14,
	FN_COSXX = 15,
	FN_TAN = 16,
	FN_TANH = 17,
	FN_COTAN = 18,
	FN_COTANH = 19,
	FN_LOG = 20,
	FN_EXP = 21,
	FN_PWR = 22,
	FN_LT = 23,
	FN_FLIP = 24,
	FN_REAL = 25,
	FN_IMAG = 26,
	FN_CONJ = 27,
	FN_NEG = 28,
	FN_ABS = 29,
	FN_RECIP = 30,
	FN_IDENT = 31,
	FN_GT = 32,
	FN_GTE = 33,
	FN_NE = 34,
	FN_EQ = 35,
	FN_AND = 36,
	FN_OR = 37,
	FN_ZERO = 38,
	FN_SQRT = 39,
	FN_ASIN = 40,
	FN_ACOS = 41,
	FN_ASINH = 42,
	FN_ACOSH = 43,
	FN_ATANH = 44,
	FN_ATAN = 45,
	FN_CABS = 46,
	FN_FLOOR = 47,
	FN_CEIL = 48,
	FN_TRUNC = 49,
	FN_ROUND = 50,
	FN_JUMP = 51,
	FN_JUMP_ON_TRUE = 52,
	FN_JUMP_ON_FALSE = 53,
	FN_JUMP_LABEL = 54,
	FN_ONE = 55
};


/* number of "old" functions in the table.  */
/* these are the ones that the parser outputs  */

#define LAST_OLD_FN   FN_ONE
#define NUM_OLD_FNS   LAST_OLD_FN + 1

/* total number of functions in the table.  */

#define LAST_FN          FN_ONE
#define NUM_FNS          LAST_FN + 1

static int s_real_stack_count;		/* how many scalars are really on stack */
static int s_stack_count;			/* # scalars on FPU stack */
static bool s_last_sqr_used;		/* was lastsqr loaded in the formula? */
static bool s_last_sqr_stored;		/* was lastsqr stored explicitly in the formula? */
static bool s_p1_constant;			/* is p1 a constant? */
static bool s_p2_constant;			/* ...and p2? */
static bool s_p3_constant;			/* ...and p3? */
static bool s_p4_constant;			/* ...and p4? */
static bool s_p5_constant;			/* ...and p5? */
static int s_convert_index;			/* subscript of next free entry in m_function_load_store_pointers  */

static void (*s_previous_function)();  /* previous function pointer  */

/* the entries in this table must be in the same order as  */
/*    the #defines above  */
/* this table is searched sequentially  */
struct function_entry
{
	char *name;  /* function name  */
	void (*in_function)();  /* 'old' function pointer  */
			/* (infn points to an operator fn in parser.c)  */
	void (*out_function)();  /* pointer to equiv. fast fn.  */
	char minimum_registers;  /* min regs needed on stack by this fn.  */
			/* (legal values are 0, 2, 4)  */
	char free_registers;  /* free regs required by this fn  */
			/* (legal values are 0, 2, 4)  */
	char stack_delta;  /* net change to # of values on the fp stack  */
			/* (legal values are -2, 0, +2)  */
};
static function_entry function_entries[NUM_OLD_FNS] =  /* array of function entries  */
{
	{ "Lod",			StkLod,				fStkLod,			0, 2, +2 },			/*  0  */
	{ "Clr",			StkClr,				fStkClr1,			0, 0,  CLEAR_STK },	/*  1  */
	{ "+",				dStkAdd,			fStkAdd,			4, 0, -2 },			/*  2  */
	{ "-",				dStkSub,			fStkSub,			4, 0, -2 },			/*  3  */
	{ "*",				dStkMul,			fStkMul,			4, 2, -2 },			/*  4  */
	{ "/",				dStkDiv,			fStkDiv,			4, 2, -2 },			/*  5  */
	{ "Sto",			StkSto,				fStkSto,			2, 0,  0 },			/*  6  */
	{ "Sqr",			dStkSqr,			fStkSqr,			2, 2,  0 },			/*  7  */
	{ ":",				EndInit,			fStkEndInit,		0, 0,  CLEAR_STK },/*  8  */
	{ "Mod",			dStkMod,			fStkMod,			2, 0,  0 },			/*  9  */
	{ "<=",				dStkLTE,			fStkLTE,			4, 0, -2 },			/* 10  */
	{ "Sin",			dStkSin,			fStkSin,			2, 2,  0 },			/* 11  */
	{ "Cos",			dStkCos,			fStkCos,			2, 2,  0 },			/* 12  */
	{ "Sinh",			dStkSinh,			fStkSinh,			2, 2,  0 },			/* 13  */
	{ "Cosh",			dStkCosh,			fStkCosh,			2, 2,  0 },			/* 14  */
	{ "Cosxx",			dStkCosXX,			fStkCosXX,			2, 2,  0 },			/* 15  */
	{ "Tan",			dStkTan,			fStkTan,			2, 2,  0 },			/* 16  */
	{ "Tanh",			dStkTanh,			fStkTanh,			2, 2,  0 },			/* 17  */
	{ "CoTan",			dStkCoTan,			fStkCoTan,			2, 2,  0 },			/* 18  */
	{ "CoTanh",			dStkCoTanh,			fStkCoTanh,			2, 2,  0 },			/* 19  */
	{ "Log",			dStkLog,			fStkLog,			2, 2,  0 },			/* 20  */
	{ "Exp",			dStkExp,			fStkExp,			2, 2,  0 },			/* 21  */
	{ "^",				dStkPwr,			fStkPwr,			4, 2, -2 },			/* 22  */
	{ "<",				dStkLT,				fStkLT,				4, 0, -2 },			/* 23  */
	{ "Flip",			dStkFlip,			fStkFlip,			2, 0,  0 },			/* 24  */
	{ "Real",			dStkReal,			fStkReal,			2, 0,  0 },			/* 25  */
	{ "Imag",			dStkImag,			fStkImag,			2, 0,  0 },			/* 26  */
	{ "Conj",			dStkConj,			fStkConj,			2, 0,  0 },			/* 27  */
	{ "Neg",			dStkNeg,			fStkNeg,			2, 0,  0 },			/* 28  */
	{ "Abs",			dStkAbs,			fStkAbs,			2, 0,  0 },			/* 29  */
	{ "Recip",			dStkRecip,			fStkRecip,			2, 2,  0 },			/* 30  */
	{ "Ident",			StkIdent,			fStkIdent,			2, 0,  0 },			/* 31  */
	{ ">",				dStkGT,				fStkGT,				4, 0, -2 },			/* 32  */
	{ ">=",				dStkGTE,			fStkGTE,			4, 0, -2 },			/* 33  */
	{ "!=",				dStkNE,				fStkNE,				4, 0, -2 },			/* 34  */
	{ "==",				dStkEQ,				fStkEQ,				4, 0, -2 },			/* 35  */
	{ "&&",				dStkAND,			fStkAND,			4, 0, -2 },			/* 36  */
	{ "||",				dStkOR,				fStkOR,				4, 0, -2 },			/* 37  */
	{ "Zero",			dStkZero,			fStkZero,			2, 0,  0 },			/* 38  */
	{ "Sqrt",			dStkSqrt,			fStkSqrt,			2, 2,  0 },			/* 39  */
	{ "ASin",			dStkASin,			fStkASin,			2, 4,  0 },			/* 40  */
	{ "ACos",			dStkACos,			fStkACos,			2, 4,  0 },			/* 41  */
	{ "ASinh",			dStkASinh,			fStkASinh,			2, 4,  0 },			/* 42  */
	{ "ACosh",			dStkACosh,			fStkACosh,			2, 4,  0 },			/* 43  */
	{ "ATanh",			dStkATanh,			fStkATanh,			2, 4,  0 },			/* 44  */
	{ "ATan",			dStkATan,			fStkATan,			2, 4,  0 },			/* 45  */
	{ "CAbs",			dStkCAbs,			fStkCAbs,			2, 0,  0 },			/* 46  */
	{ "Floor",			dStkFloor,			fStkFloor,			2, 0,  0 },			/* 47  */
	{ "Ceil",			dStkCeil,			fStkCeil,			2, 0,  0 },			/* 48  */
	{ "Trunc",			dStkTrunc,			fStkTrunc,			2, 0,  0 },			/* 49  */
	{ "Round",			dStkRound,			fStkRound,			2, 0,  0 },			/* 50  */
	{ "Jump",			StkJump,			fStkJump,			0, 0,  0 },			/* 51  */
	{ "JumpOnTrue",		dStkJumpOnTrue,		fStkJumpOnTrue,		2, 0,  0 },			/* 52  */
	{ "JumpOnFalse",	dStkJumpOnFalse,	fStkJumpOnFalse,	2, 0,  0 },			/* 53  */
	{ "JumpLabel",		StkJumpLabel,		fStkJumpLabel,		0, 0,  0 },			/* 54  */
	{ "One",			dStkOne,			fStkOne,			2, 0,  0 }			/* 55  */
};

#ifdef TESTFP
static char cDbgMsg[255];
#endif  /* TESTFP  */

/* is stack top a real?  */
static bool stack_top_is_real()
{
	return s_previous_function == fStkReal
		|| s_previous_function == fStkReal2
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC
		|| s_previous_function == fStkLodRealAbs
		|| s_previous_function == fStkImag
		|| s_previous_function == fStkLodImag;
}

void Formula::peephole_optimizer(t_function_pointer &function)
{
	/* This optimizer grew from a simple if statement into a monster.  */

	/* Most of the bugs in the optimizer have been in the code that  */
	/*   juggles the overflow stack.  */

	/* For the following:  */
	/*   * == s_convert_index points to this  */
	/*  () == this is about to be added to the array  */

	if (function == fStkLod)  /* about to add Lod to the array  */
	{
		peephole_optimize_load(function);
	}
	else if (function == fStkAdd)
	{
		peephole_optimize_add(function);
	}
	else if (function == fStkSub)
	{
		peephole_optimize_sub(function);
	}
	else if (function == fStkMul)
	{
		peephole_optimize_mul(function);
	}
	else if (s_previous_function == fStkSto && function == fStkClr1)
	{
		peephole_optimize_store_clear(function);
	}
	else if (function == fStkDiv)
	{
		peephole_optimize_divide(function);
	}
	else if (function == fStkReal)
	{
		peephole_optimize_real(function);
	}
	else if (s_previous_function == fStkLod && function == fStkImag)
	{
		peephole_optimize_load_imaginary(function);
	}
	else if (s_previous_function == fStkLod && function == fStkConj)
	{
		peephole_optimize_load_conjugate(function);
	}
	else if (function == fStkMod && s_stack_count < MAX_STACK)
	{
		peephole_optimize_modulus(function);
	}
	else if (function == fStkFlip)
	{
		peephole_optimize_flip(function);
	}
	else if (function == fStkAbs)
	{
		peephole_optimize_abs(function);
	}
	else if (function == fStkSqr)
	{
		peephole_optimize_sqr(function);
	}
	else if (function == fStkPwr)
	{
		peephole_optimize_power(function);
	}
	else if (function == fStkLTE)
	{
		peephole_optimize_less_equal(function);
	}
	else if (function == fStkLT)
	{
		peephole_optimize_less(function);
	}
	else if (function == fStkGT)
	{
		peephole_optimize_greater(function);
	}
	else if (function == fStkGTE)
	{
		peephole_optimize_greater_equal(function);
	}
	else if (function == fStkNE)
	{
		peephole_optimize_not_equal(function);
	}
	else if (function == fStkEQ)
	{
		peephole_optimize_equal(function);
	}
}

void Formula::peephole_optimize_load(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		&& m_load[m_load_ptr-1] == m_load[m_load_ptr])
	{
		/* previous non-adjust operator was Lod of same operand  */
		/* ? lodx ? (*lodx)  */
		if (is_function(--s_convert_index, fStkPush2)) /* prev fn was push  */
		{
			/* ? lod *push (lod)  */
			--s_convert_index;  /* found  *lod push (lod)  */
			if (is_function(s_convert_index-1, fStkPush2)) /* always more ops here  */
			{
				DBUGMSG("push *lod push (lod) -> push4 (*loddup)");
				set_function(s_convert_index-1, fStkPush4);
			}
			else  /* prev op not push  */
			{
				DBUGMSG("op *lod push (lod) -> op pusha(p=0) (*loddup)");
				set_no_operand(s_convert_index);  /* use 'alternate' push fn.  */
				set_function(s_convert_index++, fStkPush2a);  /* push w/2 free on stack  */
				/* operand ptr will be set below  */
			}
		}
		else  /* never  push *lod (lod)  so must be  */
		{
			DBUGMSG("op *lod (lod) -> op (*loddup)");
		}
		function = fStkLodDup;
	}
	else if (s_previous_function == fStkSto2
		&& m_store[m_store_ptr-1] == m_load[m_load_ptr])
	{
		/* store, load of same value  */
		/* only one operand on stack here when prev oper is Sto2  */
		DBUGMSG("*sto2 (lod) -> (*stodup)");
		--s_convert_index;
		function = fStkStoDup;
	}
	/* This may cause roundoff problems when later operators  */
	/*  use the rounded value that was stored here, while the next  */
	/*  operator uses the more accurate internal value.  */
	else if (s_previous_function == fStkStoClr2
		&& m_store[m_store_ptr-1] == m_load[m_load_ptr])
	{
		/* store, clear, load same value found  */
		/* only one operand was on stack so this is safe  */
		DBUGMSG("*StoClr2 (Lod) -> (*Sto2)");
		--s_convert_index;
		function = fStkSto2;  /* use different Sto fn  */
	}
	else
	{
		if (m_load[m_load_ptr] == &m_variables[VARIABLE_LAST_SQR].argument
			&& s_last_sqr_stored)
		{
			/* -- LastSqr is a real. */
			DBUGMSG("(*lod[lastsqr]) -> (*lodreal)");
			function = fStkLodReal;
		}
		else if (s_p1_constant && m_load[m_load_ptr]->d.y == 0.0)
		{
			DBUGMSG("(*lod) -> (*lodrealc)");
			function = fStkLodRealC;  /* a real const is being loaded  */
		}
	}
	set_operand(s_convert_index, m_load[m_load_ptr++]);
}

void Formula::peephole_optimize_add_load_dup(t_function_pointer &function)
{
	--s_convert_index;  /* found  ? *loddup (add)  */
	if (s_convert_index != 0 && is_function(s_convert_index-1, fStkPush2a))
	{
		/* because  push lod lod  is impossible so is  push loddup  */
		DBUGMSG("pusha *loddup (add) -> (*loddbl),stk += 2");
		REMOVE_PUSH;
		set_operand_next(s_convert_index);
	}
	else if (s_convert_index != 0 && is_function(s_convert_index-1, fStkPush4))
	{
		DBUGMSG("push4 *loddup (add) -> push2 (*loddbl),stk+=2");
		set_function(s_convert_index-1, fStkPush2);
		s_stack_count += 2;
	}
	else
	{
		DBUGMSG("op *loddup (add) -> op (*loddbl)");
	}
	function = fStkLodDbl;
}

void Formula::peephole_optimize_add_store_dup(t_function_pointer &function)
{
	DBUGMSG("stodup (*add) -> (*stodbl)");
	/* there are always exactly 4 on stack here  */
	--s_convert_index;
	function = fStkStoDbl;
}

void Formula::peephole_optimize_add_load(t_function_pointer &function)
{
	--s_convert_index;     /*  ? *lod (add)  */
	if (is_function(s_convert_index-1, fStkPush2))
	{
		DBUGMSG("*push load (add) -> (*plodadd),stk+=2");
		REMOVE_PUSH;
		set_operand_next(s_convert_index);
		function = fStkPLodAdd;
	}
	else
	{
		DBUGMSG("op *lod (add) -> op (*lodadd)");
		function = fStkLodAdd;
	}
}

void Formula::peephole_optimize_add_load_real(t_function_pointer &function)
{
	--s_convert_index;  /* found  ? *lodreal (add)  */
	if (is_function(s_convert_index-1, fStkPush2))
	{
		DBUGMSG("*push lodreal (add) -> (*lodrealadd),stk+=2");
		REMOVE_PUSH;
		set_operand_next(s_convert_index);
	}
	else
	{
		DBUGMSG("*lodreal (add) -> (*lodrealadd)");
	}
	function = fStkLodRealAdd;
}

void Formula::peephole_optimize_add_load_imaginary(t_function_pointer &function)
{
	--s_convert_index;  /* found  ? *lodimag (add)  */
	if (is_function(s_convert_index-1, fStkPush2))
	{
		DBUGMSG("*push lodimag (add) -> (*lodimagadd),stk+=2");
		REMOVE_PUSH;
		set_operand_next(s_convert_index);
	}
	else
	{
		DBUGMSG("*lodimag (add) -> (*lodimagadd)");
	}
	function = fStkLodImagAdd;
}

void Formula::peephole_optimize_add(t_function_pointer &function)
{
	if (s_previous_function == fStkLodDup) /* there is never a push before add  */
	{
		peephole_optimize_add_load_dup(function);
	}
	else if (s_previous_function == fStkStoDup)
	{
		peephole_optimize_add_store_dup(function);
	}
	else if (s_previous_function == fStkLod) /* have found  lod (*add)  */
	{
		peephole_optimize_add_load(function);
	}
	else if (s_previous_function == fStkLodReal || s_previous_function == fStkLodRealC)
	{
		peephole_optimize_add_load_real(function);
	}
	else if (s_previous_function == fStkLodImag)
	{
		peephole_optimize_add_load_imaginary(function);
	}
}

void Formula::peephole_optimize_sub(t_function_pointer &function)
{
	if (s_previous_function == fStkLod)
	{
		/* found  lod (*sub)  */
		--s_convert_index;  /*  *lod (sub)  */
		/* there is never a sequence (lod push sub)  */
		if (is_function(s_convert_index-1, fStkPush2))
		{
			DBUGMSG("*push lod (sub) -> (*plodsub),stk+=2");
			REMOVE_PUSH;
			set_operand_next(s_convert_index);
			function = fStkPLodSub;
		}
		else
		{
			DBUGMSG("*lod (sub) -> (*lodsub)");
			function = fStkLodSub;
		}
	}
	else if (s_previous_function == fStkLodReal || s_previous_function == fStkLodRealC)
	{
		--s_convert_index;  /*  ? *lodreal (sub)  */
		if (is_function(s_convert_index-1, fStkPush2))
		{
			DBUGMSG("*push lodreal (sub) -> (*lodrealsub),stk+=2");
			REMOVE_PUSH;
			set_operand_next(s_convert_index);
		}
		else
		{
			DBUGMSG("*lodreal (sub) -> (*lodrealsub)");
		}
		function = fStkLodRealSub;
	}
	else if (s_previous_function == fStkLodImag)
	{
		--s_convert_index;  /*  ? *lodimag (sub)  */
		if (is_function(s_convert_index-1, fStkPush2))
		{
			DBUGMSG("*push lodimag (sub) -> (*lodimagsub),stk+=2");
			REMOVE_PUSH;
			set_operand_next(s_convert_index);
		}
		else
		{
			DBUGMSG("*lodimag (sub) -> (*lodimagsub)");
		}
		function = fStkLodImagSub;
	}
}

void Formula::peephole_optimize_mul_load_dup(t_function_pointer &function)
{
	/* found  loddup ? (*mul)  */
	if (is_function(--s_convert_index, fStkPush2))
	{
		DBUGMSG("loddup *push (mul) -> (*lodsqr),stk+=2");
		REMOVE_PUSH;
	}
	else
	{
		DBUGMSG("*loddup (mul) -> (*lodsqr)");
	}
	function = fStkLodSqr;
}

void Formula::peephole_optimize_mul_store_dup(t_function_pointer &function)
{
	DBUGMSG("stodup (mul) -> (*stosqr0)");
	--s_convert_index;
	function = fStkStoSqr0;  /* dont save lastsqr here ever  */
}

void Formula::peephole_optimize_mul_load(t_function_pointer &function)
{
	--s_convert_index;  /*  lod *? (mul)  */
	if (is_function(s_convert_index, fStkPush2)) /*  lod *push (mul))  */
	{
		--s_convert_index;  /* ? *lod push (mul)  */
		if (is_function(s_convert_index-1, fStkPush2))
		{
			DBUGMSG("push *lod push (mul) -> push4 (*lodmul)");
			set_function(s_convert_index-1, fStkPush4);
		}
		else
		{
			DBUGMSG("op *lod push (mul) -> op pusha (*lodmul)");
			set_operand(s_convert_index + 1, get_operand(s_convert_index));  /* fix operand ptr  */
			set_function(s_convert_index, fStkPush2a);
			set_no_operand(s_convert_index);
			s_convert_index++;
		}
	}
	else
	{
		DBUGMSG("*lod (mul) -> (*lodmul)");
	}
	function = fStkLodMul;

	/*  change loadreal a, lodmul b --> lod b, lodrealmul a  */
	set_no_function(s_convert_index);  /* mark the pending fn as null  */
	if (is_function(s_convert_index-1, fStkPush4)
		|| is_function(s_convert_index-1, fStkPush2a))
	{
		--s_convert_index;  /* look back past this push  */
	}

	if (is_function(s_convert_index-1, fStkLodRealC)
		&& m_load[m_load_ptr-2]->d.x == 2.0)
	{
		/* -- Convert '2*a' into 'a + a'. */
		if (is_no_function(s_convert_index))
		{
			DBUGMSG("lodreal[2] (*lodmul[b])"
				" -> (*loddbl[b])");
			set_operand(s_convert_index-1, get_operand(s_convert_index));
		}
		else if (is_function(s_convert_index, fStkPush2a))
		{
			DBUGMSG("lodreal[2] *pusha (lodmul[b])"
				" -> loddbl[b],stk+=2");
			set_operand_prev_next(s_convert_index);
			s_stack_count += 2;
		}
		else if (is_function(s_convert_index, fStkPush4))
		{
			DBUGMSG("lodreal[2] *push4 (lodmul[b])"
				" -> loddbl[b],stk+=4");
			set_operand_prev_next(s_convert_index);
			s_stack_count += 4;
		}
		set_no_function(--s_convert_index);  /* so no increment later  */
		function = fStkLodDbl;
	}
	else if (is_function(s_convert_index-1, fStkLodReal)
		|| is_function(s_convert_index-1, fStkLodRealC))
	{
		/* lodreal *?push?? (*?lodmul)  */
		Arg *otemp = get_operand(s_convert_index-1);  /* save previous fn's operand  */
		set_function(s_convert_index-1, fStkLod);  /* prev fn = lod  */
		/* Moved setting of prev lodptr to below */
		/* This was a bug causing a bad loadptr to be set here  */
		/* 3 lines marked 'prev lodptr = this' below replace this line  */
		if (is_no_function(s_convert_index))
		{
			DBUGMSG("lodreal[a] (*lodmul[b])"
				" -> lod[b] (*lodrealmul[a])");
			set_operand(s_convert_index-1, get_operand(s_convert_index));  /* prev lodptr = this  */
		}
		else if (is_function(s_convert_index, fStkPush2a))
		{
			DBUGMSG("lodreal[a] *pusha (lodmul[b])"
				" -> lod[b] (*lodrealmul[a]),stk+=2");
			/* set this fn ptr to null so cvtptrx won't be incr later  */
			set_no_function(s_convert_index);
			set_operand_prev_next(s_convert_index);  /* prev lodptr = this  */
			s_stack_count += 2;
		}
		else if (is_function(s_convert_index, fStkPush4))
		{
			DBUGMSG("lodreal[a] *push4 (lodmul[b])"
				" -> lod[b] push2 (*lodrealmul[a]),stk+=2");
			set_function(s_convert_index++, fStkPush2);
			set_operand(s_convert_index-2, get_operand(s_convert_index));  /* prev lodptr = this  */
			/* we know cvtptrx points to a null function now  */
			s_stack_count += 2;
		}
		set_operand(s_convert_index, otemp);  /* switch the operands  */
		function = fStkLodRealMul;  /* next fn is now lodrealmul  */
	}

	if (!is_no_function(s_convert_index))
	{
		s_convert_index++;  /* adjust cvtptrx back to normal if needed  */
	}
}

void Formula::peephole_optimize_mul_real(t_function_pointer &function)
{
	--s_convert_index;  /* found  lodreal *? (mul)  */
	if (is_function(s_convert_index, fStkPush2))
	{
		DBUGMSG("lodreal *push2 (mul) -> (*lodrealmul),stk+=2");
		REMOVE_PUSH;
	}
	else
	{
		DBUGMSG("*lodreal (mul) -> (*lodrealmul)");
	}
	function = fStkLodRealMul;

	if (s_previous_function == fStkLodRealC  /* use s_previous_function here  */
		&& m_load[m_load_ptr-1]->d.x == 2.0)
	{
		if (is_function(s_convert_index, fStkPush2))
		{
			DBUGMSG("push (*lodrealmul[2]) -> (*dbl),stk+=2");
			REMOVE_PUSH;
		}
		else
		{
			DBUGMSG("*lodrealmul[2] -> (*dbl)");
		}
		set_no_operand(s_convert_index);
		function = fStkDbl;

		if (is_function(s_convert_index-1, fStkLod))
		{
			DBUGMSG("lod (*dbl) -> (*loddbl)");
			--s_convert_index;
			function = fStkLodDbl;
		}
		else if (is_function(s_convert_index-1, fStkSto2))
		{
			DBUGMSG("sto2 (*dbl) -> (*stodbl)");
			--s_convert_index;
			function = fStkStoDbl;
		}
	}
}

void Formula::peephole_optimize_mul_load_imaginary(t_function_pointer &function)
{
	--s_convert_index;  /* found  lodimag *? (mul)  */
	if (is_function(s_convert_index, fStkPush2))
	{
		DBUGMSG("lodimag *push2 (mul) -> (*lodimagmul),stk+=2");
		REMOVE_PUSH;
	}
	else
	{
		DBUGMSG("*lodimag (mul) -> (*lodimagmul)");
	}
	function = fStkLodImagMul;
}

void Formula::peephole_optimize_mul_load_less(t_function_pointer &function)
{
	/* this shortcut fails if  Lod LT Pull Mul  found  */
	DBUGMSG("LodLT (*Mul) -> (*LodLTMul)");
	--s_convert_index;  /* never  lod LT Push Mul  here  */
	function = fStkLodLTMul;
}

void Formula::peephole_optimize_mul_load_less_equal(t_function_pointer &function)
{
	DBUGMSG("LodLTE (*mul) -> (*LodLTEmul)");
	--s_convert_index;
	function = fStkLodLTEMul;
}

void Formula::peephole_optimize_mul(t_function_pointer &function)
{
	if (s_previous_function == fStkLodDup)
	{
		peephole_optimize_mul_load_dup(function);
	}
	else if (s_previous_function == fStkStoDup) /* no pushes here, 4 on stk.  */
	{
		peephole_optimize_mul_store_dup(function);
	}
	else if (s_previous_function == fStkLod)
	{
		peephole_optimize_mul_load(function);
	}
	else if (s_previous_function == fStkLodReal || s_previous_function == fStkLodRealC)
	{
		peephole_optimize_mul_real(function);
	}
	else if (s_previous_function == fStkLodImag)
	{
		peephole_optimize_mul_load_imaginary(function);
	}
	else if (s_previous_function == fStkLodLT && !is_function(s_convert_index-1, fStkPull2))
	{
		peephole_optimize_mul_load_less(function);
	}
	else if (s_previous_function == fStkLodLTE && !is_function(s_convert_index-1, fStkPull2))
	{
		peephole_optimize_mul_load_less_equal(function);
	}
}

void Formula::peephole_optimize_store_clear(t_function_pointer &function)
{
	--s_convert_index;
	if (s_stack_count == 2)
	{
		DBUGMSG("sto (*clr1) -> (*stoclr2)");
		function = fStkStoClr2;
	}
	else
	{
		DBUGMSG("sto (*clr1) -> (*stoclr1)");
		function = fStkStoClr1;
	}
}

void Formula::peephole_optimize_divide(t_function_pointer &function)
{
	if (s_previous_function == fStkLodRealC && m_parser_vsp < m_formula_max_args - 1)
	{
		/* have found a divide by a real constant  */
		/*  and there is space to create a new one  */
		/* lodrealc ? (*div)  */
		if (is_function(--s_convert_index, fStkPush2))
		{
			DBUGMSG("lodrealc *push (div) -> (*lodrealmul),stk+=2");
			REMOVE_PUSH;
		}
		else
		{
			DBUGMSG("*lodrealc (div) -> (*lodrealmul)");
		}
		m_variables[m_parser_vsp].name = NULL;  /* this constant has no name  */
		m_variables[m_parser_vsp].name_length = 0;
		m_variables[m_parser_vsp].argument.d.x = 1.0/m_load[m_load_ptr - 1]->d.x;
		m_variables[m_parser_vsp].argument.d.y = 0.0;
		set_operand(s_convert_index, &m_variables[m_parser_vsp++].argument);
		function = fStkLodRealMul;
	}
}

void Formula::peephole_optimize_real(t_function_pointer &function)
{
	if (s_previous_function == fStkLod)
	{
		DBUGMSG("lod (*real) -> (*lodreal)");
		--s_convert_index;
		function = fStkLodReal;
	}
	else if (s_stack_count < MAX_STACK)
	{
		DBUGMSG("(*real) -> (*real2)");
		function = fStkReal2;
	}
}

void Formula::peephole_optimize_load_imaginary(t_function_pointer &function)
{
	DBUGMSG("lod (*imag) -> lodimag");
	--s_convert_index;
	function = fStkLodImag;
}

void Formula::peephole_optimize_load_conjugate(t_function_pointer &function)
{
	DBUGMSG("lod (*conj) -> (*lodconj)");
	--s_convert_index;
	function = fStkLodConj;
}

void Formula::peephole_optimize_modulus(t_function_pointer &function)
{
	DBUGMSG("(*mod) -> (*mod2)");
	function = fStkMod2;  /* use faster version if room on stack  */
	if (s_previous_function == fStkLod)
	{
		DBUGMSG("lod (*mod2) -> (*lodmod2)");
		--s_convert_index;
		function = fStkLodMod2;
	}
	else if (s_previous_function == fStkSto || s_previous_function == fStkSto2)
	{
		DBUGMSG("sto (*mod2) -> (*stomod2)");
		--s_convert_index;
		function = fStkStoMod2;
	}
	else if (s_previous_function == fStkLodSub)
	{
		DBUGMSG("lodsub (*mod2) -> (*lodsubmod)");
		--s_convert_index;
		function = fStkLodSubMod;
	}
	else if (stack_top_is_real())
	{
		DBUGMSG("(*mod2[st real]) -> (*sqr3)");
		function = fStkSqr3;
	}
}

void Formula::peephole_optimize_flip(t_function_pointer &function)
{
	if (s_previous_function == fStkReal || s_previous_function == fStkReal2)
	{
		DBUGMSG("real (*flip) -> (*realflip)");
		--s_convert_index;
		function = fStkRealFlip;
	}
	else if (s_previous_function == fStkImag)
	{
		DBUGMSG("imag (*flip) -> (*imagflip)");
		--s_convert_index;
		function = fStkImagFlip;
	}
	else if (s_previous_function == fStkLodReal)
	{
		DBUGMSG("lodreal (*flip) -> (*lodrealflip)");
		--s_convert_index;
		function = fStkLodRealFlip;
	}
	else if (s_previous_function == fStkLodImag)
	{
		DBUGMSG("lodimag (*flip) -> (*lodimagflip)");
		--s_convert_index;
		function = fStkLodImagFlip;
	}
}

void Formula::peephole_optimize_abs(t_function_pointer &function)
{
	if (s_previous_function == fStkLodReal)
	{
		DBUGMSG("lodreal (*abs) -> (*lodrealabs)");
		--s_convert_index;
		function = fStkLodRealAbs;
	}
	else if (s_previous_function == fStkLodImag)
	{
		DBUGMSG("lodimag (*abs) -> (*lodimagabs)");
		--s_convert_index;
		function = fStkLodImagAbs;
	}
}

void Formula::peephole_optimize_sqr(t_function_pointer &function)
{
	if (s_previous_function == fStkLod && !is_function(s_convert_index-1, fStkPush2))
	{
		DBUGMSG("lod (*sqr) -> (*lodsqr)");
		--s_convert_index;
		function = fStkLodSqr;  /* assume no need to save lastsqr  */
		if (s_last_sqr_used)
		{
			DBUGMSG("(*lodsqr) -> (*lodsqr2)");
			function = fStkLodSqr2;  /* lastsqr is being used  */
		}
	}
	else if (s_previous_function == fStkSto2)
	{
		DBUGMSG("sto2 (*sqr) -> (*stosqr0)");
		--s_convert_index;
		function = fStkStoSqr0;  /* assume no need to save lastsqr  */
		if (s_last_sqr_used)
		{
			DBUGMSG("(*stosqr0) -> (*stosqr)");
			function = fStkStoSqr;  /* save lastsqr  */
		}
	}
	else
	{
		if (!s_last_sqr_used)
		{
			DBUGMSG("(*sqr) -> (*sqr0)");
			function = fStkSqr0;  /* don't save lastsqr  */
			if (stack_top_is_real())
			{
				DBUGMSG("(*sqr0[st real]) -> (*sqr3)");
				function = fStkSqr3;
			}
		}
	}
}

void Formula::peephole_optimize_power_load_real_constant(t_function_pointer &function)
{
	double dTemp = m_load[m_load_ptr-1]->d.x;
	if (dTemp == 2.0 || dTemp == 1.0 || dTemp == -1.0 || dTemp == 0.0)
	{
		/* change ^[-1, 0, 1, or 2] to recip, one, ident, sqr */
		if (is_function(s_convert_index-1, fStkPush2))
		{
			DBUGMSG("LodRealC[-1,0,1,2] Push (*Pwr)"
				" -> (*[recip,1,ident,Sqr0]), stk+=2");
			REMOVE_PUSH;  /* lod[?] (push) *pwr */
		}
		else
		{
			DBUGMSG("LodRealC[-1,0,1,2] (*Pwr)"
				" -> (*[recip,1,ident,sqr0])");
		}
		--s_convert_index;
		set_no_operand(s_convert_index);
		if (dTemp == 2.0)
		{
			DBUGMSG("[]=Sqr0");
			function = fStkSqr0;  /* no need to compute lastsqr here  */
			if (is_function(s_convert_index-1, fStkLod))
			{
				DBUGMSG("Lod (*Sqr0) -> (*LodSqr)");
				--s_convert_index;
				function = fStkLodSqr;  /* dont save lastsqr  */
			}
			else if (is_function(s_convert_index-1, fStkSto2))
			{
				DBUGMSG("Sto2 (*Sqr0) -> (*StoSqr0)");
				--s_convert_index;
				function = fStkStoSqr0;  /* dont save lastsqr  */
			}
		}
		else if (dTemp == 1.0)
		{
			DBUGMSG("[]=Ident");
			function = fStkIdent;
		}
		else if (dTemp == 0.0)
		{
			DBUGMSG("[]=One");
			function = fStkOne;
		}
		else if (dTemp == -1.0)
		{
			DBUGMSG("[]=Recip");
			function = fStkRecip;
		}
	}
	else if (is_function(s_convert_index-1, s_previous_function))
	{
		--s_convert_index;
		function = fStkLodRealPwr;  /* see comments below  */
	}
}

void Formula::peephole_optimize_power_load_real(t_function_pointer &function)
{
	/* don't handle pushes here, lodrealpwr needs 4 free  */
	DBUGMSG("LodReal (*Pwr) -> (*LodRealPwr)");
	--s_convert_index;
	function = fStkLodRealPwr;
}

void Formula::peephole_optimize_power(t_function_pointer &function)
{
	if (s_previous_function == fStkLodRealC)
	{
		peephole_optimize_power_load_real_constant(function);
	}
	else if (s_previous_function == fStkLodReal && is_function(s_convert_index-1, s_previous_function))
	{
		peephole_optimize_power_load_real(function);
	}
}

void Formula::peephole_optimize_less_equal(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*LTE) -> (*LodLTE)");
		--s_convert_index;
		function = fStkLodLTE;
	}
}

void Formula::peephole_optimize_less(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*LT) -> (*LodLT)");
		--s_convert_index;
		function = fStkLodLT;
	}
}

void Formula::peephole_optimize_greater(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*GT) -> (*LodGT)");
		--s_convert_index;
		function = fStkLodGT;
	}
}

void Formula::peephole_optimize_greater_equal(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*GTE) -> (*LodGTE)");
		--s_convert_index;
		function = fStkLodGTE;
	}
}

void Formula::peephole_optimize_not_equal(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*NE) -> (*LodNE)");
		--s_convert_index;
		function = fStkLodNE;
	}
}

void Formula::peephole_optimize_equal(t_function_pointer &function)
{
	if (s_previous_function == fStkLod
		|| s_previous_function == fStkLodReal
		|| s_previous_function == fStkLodRealC)
	{
		DBUGMSG("Lod (*EQ) -> (*LodEQ)");
		--s_convert_index;
		function = fStkLodEQ;
	}
}

bool Formula::CvtFptr(void (*function)(), int minimum_stack, int free_stack, int delta_stack)
{
#ifdef TESTFP
	int previous_stack_count;
#endif

	int Max_On_Stack = MAX_STACK - free_stack;  /* max regs allowed on stack  */
	int Num_To_Push; /* number of regs to push  */

	/* first do some sanity checks  */
	if ((delta_stack != -2 && delta_stack != 0 && delta_stack != 2 && delta_stack != CLEAR_STK)
		|| (free_stack != 0 && free_stack != 2 && free_stack != 4)
		|| (minimum_stack != 0 && minimum_stack != 2 && minimum_stack != 4))
	{
awful_error:
		stop_message (0, "FATAL INTERNAL PARSER ERROR!");
		return false;  /* put out dire message and revert to old parser  */
	}

	/* this if statement inserts a stack push or pull into the token array  */
	/*   it would be much better to do this *after* optimization  */
	if (s_stack_count < minimum_stack)  /* not enough operands on fpu stack  */
	{
		DBUGMSG2("Inserted pull.  Stack: %2d --> %2d", s_stack_count, s_stack_count + 2);
		set_no_operand(s_convert_index);
		set_function(s_convert_index++, fStkPull2);  /* so adjust the stack, pull operand  */
		s_stack_count += 2;
	}
	else if (s_stack_count > Max_On_Stack)  /* too many operands  */
	{

		Num_To_Push = s_stack_count - Max_On_Stack;
		if (Num_To_Push == 2)
		{
			if (s_stack_count == MAX_STACK)
			{
				/* push stack down from max to max-2  */
				set_function(s_convert_index, fStkPush2);
			}
			else if (s_stack_count == MAX_STACK - 2)
			{
				/* push stack down from max-2 to max-4  */
				set_function(s_convert_index, fStkPush2a);
			}
			else
			{
				goto awful_error;
			}
			DBUGMSG2("Inserted push.  Stack: %2d --> %2d", s_stack_count, s_stack_count-2);
			set_no_operand(s_convert_index++);
			s_stack_count -= 2;
		}
		else if (Num_To_Push == 4)
		{
			/* push down from max to max-4  */
			set_function(s_convert_index, fStkPush4);
			DBUGMSG2("Inserted push.  Stack: %2d --> %2d", s_stack_count, s_stack_count-4);
			set_no_operand(s_convert_index++);
			s_stack_count -= 4;
		}
		else
		{
			goto awful_error;
		}
	}

	/* set the operand pointer here for store function  */
	if (function == fStkSto)
	{
		set_operand(s_convert_index, m_store[m_store_ptr++]);
	}
	else if (function == fStkLod && DEBUGMODE_SKIP_OPTIMIZER == g_debug_mode)
	{
		/* when disabling optimizer, set load pointer here  */
		set_operand(s_convert_index, m_load[m_load_ptr++]);
	}
	else  /* the optimizer will set the pointer for load fn.  */
	{
		set_no_operand(s_convert_index);
	}

	if (DEBUGMODE_SKIP_OPTIMIZER != g_debug_mode)
	{
		peephole_optimizer(function);
	}
	
	set_function(s_convert_index++, function);
	s_previous_function = function;
#ifdef TESTFP
	previous_stack_count = s_stack_count;
#endif
	if (delta_stack == CLEAR_STK)
	{
		s_real_stack_count = s_stack_count = 0;
	}
	else
	{
		s_stack_count += delta_stack;
		s_real_stack_count += delta_stack;
	}

	DBUGMSG3("Stack:  %2d --> %2d,  Real stack:  %2d",
			previous_stack_count, s_stack_count, s_real_stack_count);

	return true;
}

bool Formula::fill_jump_struct_fp()
{
	/* Completes all entries in jump structure. Returns 1 on error) */
	/* On entry, m_jump_index is the number of jump functions in the formula*/
	int i = 0;
	bool check_for_else = false;
	bool find_new_func = true;
	JUMP_PTRS jump_data[MAX_JUMPS];
	NEW_FN *jump_function = NULL;
	for (m_op_index = 0; m_op_index < m_last_op; m_op_index++)
	{
		if (find_new_func)
		{
			switch (m_jump_control[i].type)
			{
			case JUMPTYPE_IF:
				jump_function = fStkJumpOnFalse;
				break;
			case JUMPTYPE_ELSEIF:
				check_for_else = !check_for_else;
				jump_function = check_for_else ? fStkJump : fStkJumpOnFalse;
				break;
			case JUMPTYPE_ELSE:
				jump_function = fStkJump;
				break;
			case JUMPTYPE_ENDIF:
				jump_function = fStkJumpLabel;
				break;
			default:
				break;
			}
			find_new_func = false;
		}
		if (m_function_load_store_pointers[m_op_index].function == jump_function)
		{
			jump_data[i].JumpOpPtr = m_op_index*4;
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

int fform_per_pixel();       /* these fns are in parsera.asm  */
int BadFormula();
void Img_Setup();

void Formula::FinalOptimizations(t_function_pointer &out_function)
{
	/* ------------------------------ final optimizations ---------- */

	/* cvtptrx -> one past last operator (always clr2)  */
	--s_convert_index;  /* now it points to the last operator  */
	out_function = get_function(s_convert_index-1);
	/* ntst is the next-to-last operator  */

	if (out_function == fStkLT)
	{
		DBUGMSG("LT Clr2 -> LT2");
		set_function(s_convert_index-1, fStkLT2);
	}
	else if (out_function == fStkLodLT)
	{
		DBUGMSG("LodLT Clr2 -> LodLT2");
		set_function(s_convert_index-1, fStkLodLT2);
	}
	else if (out_function == fStkLTE)
	{
		DBUGMSG("LTE Clr2 -> LTE2");
		set_function(s_convert_index-1, fStkLTE2);
	}
	else if (out_function == fStkLodLTE)
	{
		DBUGMSG("LodLTE Clr2 -> LodLTE2");
		set_function(s_convert_index-1, fStkLodLTE2);
	}
	else if (out_function == fStkGT)
	{
		DBUGMSG("GT Clr2 -> GT2");
		set_function(s_convert_index-1, fStkGT2);
	}
	else if (out_function == fStkLodGT)
	{
		DBUGMSG("LodGT Clr2 -> LodGT2");
		set_function(s_convert_index-1, fStkLodGT2);
	}
	else if (out_function == fStkLodGTE)
	{
		DBUGMSG("LodGTE Clr2 -> LodGTE2");
		set_function(s_convert_index-1, fStkLodGTE2);
	}
	else if (out_function == fStkAND)
	{
		DBUGMSG("AND Clr2 -> ANDClr2");
		set_function(s_convert_index-1, fStkANDClr2);
		out_function = get_function(s_convert_index-2);
		if (out_function == fStkLodLTE)
		{
			DBUGMSG("LodLTE ANDClr2 -> LodLTEAnd2");
			--s_convert_index;
			set_function(s_convert_index-1, fStkLodLTEAnd2);
		}
	}
	else if (out_function == fStkOR)
	{
		DBUGMSG("OR Clr2 -> ORClr2");
		set_function(s_convert_index-1, fStkORClr2);
	}
	else
	{
		++s_convert_index;  /* adjust this back since no optimization was found  */
	}
}
void Formula::CvtStk()  /* convert the array of ptrs  */
{
	extern char g_formula_name[];
	void (*out_function)();
	Arg *testoperand;
	s_last_sqr_stored = true;  /* assume lastsqr is real (not stored explicitly)  */
	s_p1_constant = true;
	s_p2_constant = true;
	s_p3_constant = true;  /* . . . p1, p2, p3 const  */
	s_p4_constant = true;
	s_p5_constant = true;  /* . . . p4, p5 const  */
	s_last_sqr_used = false;  /* ... and LastSqr is not used  */

	/* now see if the above assumptions are true */
	for (m_op_index = m_load_ptr = m_store_ptr = 0; m_op_index < (int)m_last_op; m_op_index++)
	{
		void (*op_function)() = m_functions[m_op_index];
		if (op_function == StkLod)
		{
			if (m_load[m_load_ptr++] == &m_variables[VARIABLE_LAST_SQR].argument)
			{
				s_last_sqr_used = true;
			}
		}
		else if (op_function == StkSto)
		{
			testoperand = m_store[m_store_ptr++];
			if (testoperand == &m_variables[VARIABLE_P1].argument)
			{
				s_p1_constant = false;
			}
			else if (testoperand == &m_variables[VARIABLE_P2].argument)
			{
				s_p2_constant = false;
			}
			else if (testoperand == &m_variables[VARIABLE_P3].argument)
			{
				s_p3_constant = false;
			}
			else if (testoperand == &m_variables[VARIABLE_P4].argument)
			{
				s_p4_constant = false;
			}
			else if (testoperand == &m_variables[VARIABLE_P5].argument)
			{
				s_p5_constant = false;
			}
			else if (testoperand == &m_variables[VARIABLE_LAST_SQR].argument)
			{
				s_last_sqr_stored = false;
			}
		}
	}

	if (!s_p1_constant)
	{
		DBUGMSG("p1 not constant");
	}
	if (!s_p2_constant)
	{
		DBUGMSG("p2 not constant");
	}
	if (!s_p3_constant)
	{
		DBUGMSG("p3 not constant");
	}
	if (!s_p4_constant)
	{
		DBUGMSG("p4 not constant");
	}
	if (!s_p5_constant)
	{
		DBUGMSG("p5 not constant");
	}
	if (s_last_sqr_used)
	{
		DBUGMSG("LastSqr loaded");
		if (!s_last_sqr_stored)
		{
			DBUGMSG("LastSqr stored");
		}
	}

	if (m_functions[m_last_op-1] != StkClr)
	{
		DBUGMSG("Missing clr added at end");
		/* should be safe to modify this  */
		m_functions[m_last_op++] = StkClr;
	}

	s_previous_function = 0;
	s_convert_index = s_real_stack_count = s_stack_count = 0;

	m_load_ptr = 0;
	m_store_ptr = 0;
	for (m_op_index = 0; m_op_index < m_last_op; m_op_index++)
	{
		bool function_found = false;
		for (int i = 0; i < NUM_OF(function_entries); i++)
		{
			const function_entry &function = function_entries[i];
			if (m_functions[m_op_index] == function.in_function)
			{
				function_found = true;
				out_function = function.out_function;
				if (out_function == fStkClr1 && m_op_index == m_last_op-1)
				{
					out_function = fStkClr2;  /* convert the last clear to a clr2  */
					DBUGMSG("Last fn (CLR) --> (is really CLR2)");
				}
				if (out_function == fStkIdent && g_debug_mode != DEBUGMODE_SKIP_OPTIMIZER)
				{
					/* ident will be skipped here  */
					/* this is really part of the optimizer  */
					DBUGMSG("IDENT was skipped");
				}
				else
				{
					DBUGMSG4("fn=%s, minstk=%1i, freestk=%1i, delta=%3i",
							function.name,
							(int)(function.minimum_registers),
							(int)(function.free_registers),
							(int)(function.stack_delta));
					if (!CvtFptr(out_function, function.minimum_registers, function.free_registers, function.stack_delta))
					{
						return;
					}
				}
			}
		}
		if (!function_found)
		{
			return;  /* this should only happen if random numbers are used  */
		}
	} /* end for  */

	if (DEBUGMODE_SKIP_OPTIMIZER != g_debug_mode)
	{
		FinalOptimizations(out_function);
	}

	m_last_op = s_convert_index;  /* save the new operator count  */
	m_variables[VARIABLE_LAST_SQR].argument.d.y = 0.0;  /* do this once per image  */

	/* now change the pointers  */
	if (g_formula_name[0] != 0 &&
		(m_uses_jump == 0 || !fill_jump_struct_fp())) /* but only if parse succeeded  */
	{
		g_current_fractal_specific->per_pixel = fform_per_pixel;
		g_current_fractal_specific->orbitcalc = fFormula;
	}
	else
	{
		g_current_fractal_specific->per_pixel = BadFormula;
		g_current_fractal_specific->orbitcalc = BadFormula;
	}

	Img_Setup();  /* call assembler setup code  */
}

#endif /* XFRACT  */
