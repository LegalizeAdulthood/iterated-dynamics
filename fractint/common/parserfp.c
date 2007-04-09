/* PARSERFP.C  -- Part of FRACTINT fractal drawer.  */

/*   By Chuck Ebbert  CompuServe [76306, 1226]  */
/*                     internet: 76306.1226@compuserve.com  */

/* Fast floating-point parser code.  The functions beginning with  */
/*    "fStk" are in PARSERA.ASM.  PARSER.C calls this code after  */
/*    it has parsed the formula.  */

/*   Converts the function pointers/load pointers/store pointers  */
/*       built by parsestr() into an optimized array of function  */
/*       pointer/operand pointer pairs.  */

/*   As of 31 Dec 93, also generates executable code in memory.  */
/*       Define the varible COMPILER to generate executable code.  */
/*       COMPILER must also be defined in PARSERA.ASM. */


/* Revision history:  */

/* 15 Mar 1997 TIW  */
/*    Fixed if/else bug, replaced stopmsg with pstopmsg */

/* 09 Mar 1997 TIW/GGM  */
/*    Added support for if/else */

/* 30 Jun 1996 TIW  */
/*    Removed function names if TESTFP not defined to save memory      */
/*    Function fStkFloor added to support new 'floor' function         */
/*    Function fStkCeil  added to support new 'ceil'  function         */
/*    Function fStkTrunc added to support new 'trunc' function         */
/*    Function fStkRound added to support new 'round' function         */

/* 15 Feb 1995 CAE  */
/*    added safety tests to pointer conversion code  */
/*    added the capability for functions to require 4 free regs  */

/* 8 Feb 1995 CAE  */
/*    Comments changed.  */

/* 8 Jan 1995 JCO  */
/*    Function fStkASin added to support new 'asin' function in v19    */
/*    Function fStkASinh added to support new 'asinh' function in v19  */
/*    Function fStkACos added to support new 'acos' function in v19    */
/*    Function fStkACosh added to support new 'acosh' function in v19  */
/*    Function fStkATan added to support new 'atan' function in v19    */
/*    Function fStkATanh added to support new 'atanh' function in v19  */
/*    Function fStkSqrt added to support new 'sqrt' function in v19    */
/*    Function fStkCAbs added to support new 'cabs' function in v19    */
/*    Added support for a third parameter p3    */

/* 31 Dec 1993 CAE  */
/*    Fixed optimizer bug discovered while testing compiler.  */

/* 29 Dec 1993 CAE  */
/*    Added compiler.  */

/* 04 Dec 1993 CAE  */
/*    Added optimizations for LodImagAdd/Sub/Mul.  */

/* 06 Nov 1993 CAE  */
/*    Added optimizer support for LodRealPwr and ORClr2 functions.  */
/*    If stack top is a real, a simpler sqr() or mod() fn will be  */
/*          called (fStkSqr3() was added.)  */
/*    The identities x^0 = 1, x^1 = x, and x^-1 = recip(x) are now used by the  */
/*          optimizer.  (fStkOne() was added for this.)  */

/* 31 Oct 1993 CAE  */
/*    Optimizer converts '2*x' and 'x*2' to 'x + x'. */
/*        "     recognizes LastSqr as a real if not stored to.  */

/* 9 Oct 1993 CAE  */
/*    Functions are now converted via table search.                    */
/*    Added "real" stack count variable and debug msgs for stack size. */
/*    Added optimizer extension for commutative multiply.              */
/*    P1, P2 will be treated as consts if they are never stored to.    */
/*    Function fStkStoClr2 now emitted for sto, clr with 2 on stack.    */
/*       "     fStkZero added to support new 'zero' function in v18    */
/*    Added optimization for x^2 -> sqr(x).                            */
/*    Changed "stopmsg" to "DBUGMSG" and made all macros upper case.   */
/*       (g_debug_flag = 324 now needed for debug msgs to print.)           */

/* 12 July 1993 (for v18.1) by CAE to fix optimizer bug  */

/* 22 MAR 1993 (for Fractint v18.0)  */

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
/* #define TESTFP 1 */

/* Use startup parameter "debugflag = 324" to show debug messages after  */
/*    compiling with above #define uncommented.  */

#include <string.h>
#include <ctype.h>
#include <time.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

/* global data  */
struct fls *g_function_load_store_pointers = (struct fls *)0;

#if !defined(XFRACT)

/* not moved to PROTOTYPE.H because these only communicate within
	PARSER.C and other parser modules */

extern union Arg *Arg1, *Arg2;
extern double _1_, _2_;
extern union Arg s[20], **Store, **Load;
extern int OpPtr;
extern struct ConstArg *v;
extern int InitLodPtr, InitStoPtr, InitOpPtr;
extern void (* *f)(void);
extern JUMP_CONTROL_ST *jump_control;
extern int uses_jump, jump_index;

typedef void OLD_FN(void);  /* old C functions  */

OLD_FN  StkLod, StkClr, StkSto, EndInit, StkJumpLabel;
OLD_FN  dStkAdd, dStkSub, dStkMul, dStkDiv;
OLD_FN  dStkSqr, dStkMod;
OLD_FN  dStkSin, dStkCos, dStkSinh, dStkCosh, dStkCosXX;
OLD_FN  dStkTan, dStkTanh, dStkCoTan, dStkCoTanh;
OLD_FN  dStkLog, dStkExp, dStkPwr;
OLD_FN  dStkLT, dStkLTE;
OLD_FN  dStkFlip, dStkReal, dStkImag;
OLD_FN  dStkConj, dStkNeg, dStkAbs;
OLD_FN  dStkRecip, StkIdent;
OLD_FN  dStkGT, dStkGTE, dStkNE, dStkEQ;
OLD_FN  dStkAND, dStkOR;
OLD_FN  dStkZero;
OLD_FN  dStkSqrt;
OLD_FN  dStkASin, dStkACos, dStkASinh, dStkACosh;
OLD_FN  dStkATanh, dStkATan;
OLD_FN  dStkCAbs;
OLD_FN  dStkFloor;
OLD_FN  dStkCeil;
OLD_FN  dStkTrunc;
OLD_FN  dStkRound;
OLD_FN  StkJump, dStkJumpOnTrue, dStkJumpOnFalse;
OLD_FN  dStkOne;

typedef void (NEW_FN)(void);  /* new 387-only ASM functions  */

NEW_FN  fStkPull2;  /* pull up g_fpu stack from 2 to 4  */
NEW_FN  fStkPush2;  /* push down g_fpu stack from 8 to 6  */
NEW_FN  fStkPush2a;  /* push down g_fpu stack from 6 to 4  */
NEW_FN  fStkPush4;  /* push down g_fpu stack from 8 to 4  */
NEW_FN  fStkLodDup;  /* lod, dup  */
NEW_FN  fStkLodSqr;  /* lod, sqr, dont save magnitude(i.e. lastsqr)  */
NEW_FN  fStkLodSqr2;  /* lod, sqr, save lastsqr  */
NEW_FN  fStkStoDup;  /* store, duplicate  */
NEW_FN  fStkStoSqr;  /* store, sqr, save lastsqr  */
NEW_FN  fStkStoSqr0;  /* store, sqr, dont save lastsqr  */
NEW_FN  fStkLodDbl;  /* load, double  */
NEW_FN  fStkStoDbl;  /* store, double  */
NEW_FN  fStkReal2;  /* fast ver. of real  */
NEW_FN  fStkSqr;  /* sqr, save magnitude in lastsqr  */
NEW_FN  fStkSqr0;  /* sqr, no save magnitude  */
NEW_FN  fStkClr1;  /* clear g_fpu  */
NEW_FN  fStkClr2;  /* test stack top, clear g_fpu  */
NEW_FN  fStkStoClr1;  /* store, clr1  */
NEW_FN  fStkAdd, fStkSub;
NEW_FN  fStkSto, fStkSto2;  /* fast ver. of sto  */
NEW_FN  fStkLod, fStkEndInit;
NEW_FN  fStkMod, fStkMod2;  /* faster mod  */
NEW_FN  fStkLodMod2, fStkStoMod2;
NEW_FN  fStkLTE, fStkLodLTEMul, fStkLTE2, fStkLodLTE;
NEW_FN  fStkLodLTE2, fStkLodLTEAnd2;
NEW_FN  fStkLT, fStkLodLTMul, fStkLT2, fStkLodLT;
NEW_FN  fStkLodLT2;
NEW_FN  fStkGTE, fStkLodGTE, fStkLodGTE2;
NEW_FN  fStkGT, fStkGT2, fStkLodGT, fStkLodGT2;
NEW_FN  fStkEQ, fStkLodEQ, fStkNE, fStkLodNE;
NEW_FN  fStkAND, fStkANDClr2, fStkOR, fStkORClr2;
NEW_FN  fStkSin, fStkSinh, fStkCos, fStkCosh, fStkCosXX;
NEW_FN  fStkTan, fStkTanh, fStkCoTan, fStkCoTanh;
NEW_FN  fStkLog, fStkExp, fStkPwr;
NEW_FN  fStkMul, fStkDiv;
NEW_FN  fStkFlip, fStkReal, fStkImag, fStkRealFlip, fStkImagFlip;
NEW_FN  fStkConj, fStkNeg, fStkAbs, fStkRecip;
NEW_FN  fStkLodReal, fStkLodRealC, fStkLodImag;
NEW_FN  fStkLodRealFlip, fStkLodRealAbs;
NEW_FN  fStkLodRealMul, fStkLodRealAdd, fStkLodRealSub, fStkLodRealPwr;
NEW_FN  fStkLodImagMul, fStkLodImagAdd, fStkLodImagSub;  /* CAE 4Dec93  */
NEW_FN  fStkLodImagFlip, fStkLodImagAbs;
NEW_FN  fStkLodConj;
NEW_FN  fStkLodAdd, fStkLodSub, fStkLodSubMod, fStkLodMul;
NEW_FN  fStkPLodAdd, fStkPLodSub;  /* push-lod-add/sub  */
NEW_FN  fStkIdent;
NEW_FN  fStkStoClr2;  /* store, clear stack by popping  */
NEW_FN  fStkZero;  /* to support new parser fn.  */
NEW_FN  fStkDbl;  /* double the stack top  CAE 31OCT93  */
NEW_FN  fStkOne, fStkSqr3;  /* sqr3 is sqr/mag of a real  CAE 09NOV93  */
NEW_FN  fStkSqrt;
NEW_FN  fStkASin, fStkACos, fStkASinh, fStkACosh;
NEW_FN  fStkATanh, fStkATan;
NEW_FN  fStkCAbs;
NEW_FN  fStkFloor, fStkCeil, fStkTrunc, fStkRound; /* rounding functions */
NEW_FN  fStkJump, fStkJumpOnTrue, fStkJumpOnFalse, fStkJumpLabel; /* flow */
NEW_FN  fStkOne;   /* to support new parser fn.  */

/* check to see if a const is being loaded  */
/* the really awful hack below gets the first char of the name  */
/*    of the variable being accessed  */
/* if first char not alpha, or const p1, p2, or p3 are being accessed  */
/*    then this is a const.  */
#define IS_CONST(x) \
		(!isalpha(**(((char **)x) - 2)) \
		|| (x == &PARM1 && p1const) \
		|| (x == &PARM2 && p2const) \
		|| (x == &PARM3 && p3const) \
		|| (x == &PARM4 && p4const) \
		|| (x == &PARM5 && p5const))

/* is stack top a real?  */
#define STACK_TOP_IS_REAL \
		(prevfptr == fStkReal || prevfptr == fStkReal2 \
		|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC \
		|| prevfptr == fStkLodRealAbs \
		|| prevfptr == fStkImag || prevfptr == fStkLodImag)

/* remove push operator from stack top  */
#define REMOVE_PUSH --cvtptrx, stkcnt += 2

#define CLEAR_STK 127
#define FNPTR(x) g_function_load_store_pointers[(x)].function  /* function pointer */
#define OPPTR(x) g_function_load_store_pointers[(x)].operand   /* operand pointer */
#define NO_OPERAND (union Arg  *)0
#define NO_FUNCTION (void (*)(void))0
#define LASTSQR v[4].a
#define PARM1 v[1].a
#define PARM2 v[2].a
#define PARM3 v[8].a
#define PARM4 v[17].a
#define PARM5 v[18].a
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

#define stopmsg pstopmsg

#define DBUGMSG(y) if (DEBUGFLAG_NO_HELP_F1_ESC == g_debug_flag || DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag) stopmsg(0, (y))
#define DBUGMSG1(y, p) \
		if (DEBUGFLAG_NO_HELP_F1_ESC == g_debug_flag || DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag){ \
			sprintf(cDbgMsg, (y), (p)); \
			stopmsg(0, cDbgMsg); \
		}
#define DBUGMSG2(y, p, q) \
		if (DEBUGFLAG_NO_HELP_F1_ESC == g_debug_flag || DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag){ \
			sprintf(cDbgMsg, (y), (p), (q)); \
			stopmsg(0, cDbgMsg); \
		}
#define DBUGMSG3(y, p, q, r) \
		if (DEBUGFLAG_NO_HELP_F1_ESC == g_debug_flag || DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag){ \
			sprintf(cDbgMsg, (y), (p), (q), (r)); \
			stopmsg(0, cDbgMsg); \
		}
#define DBUGMSG4(y, p, q, r, s) \
		if (DEBUGFLAG_NO_HELP_F1_ESC == g_debug_flag || DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag){ \
			sprintf(cDbgMsg, (y), (p), (q), (r), (s)); \
			stopmsg(0, cDbgMsg); \
		}
#define FNAME(a, b, c, d, e, f) a, b, c, d, e, f    /* use the function name string */
#else

#define DBUGMSG(y)
#define DBUGMSG1(y, p)
#define DBUGMSG2(y, p, q)
#define DBUGMSG3(y, p, q, r)
#define DBUGMSG4(y, p, q, r, s)
#define FNAME(a, b, c, d, e, f) b, c, d, e, f    /* don't use the function name string */
#endif /* TESTFP */

#define FN_LOD            0
#define FN_CLR            1
#define FN_ADD            2
#define FN_SUB            3
#define FN_MUL            4
#define FN_DIV            5
#define FN_STO            6
#define FN_SQR            7
#define FN_ENDINIT        8
#define FN_MOD            9
#define FN_LTE           10
#define FN_SIN           11
#define FN_COS           12
#define FN_SINH          13
#define FN_COSH          14
#define FN_COSXX         15
#define FN_TAN           16
#define FN_TANH          17
#define FN_COTAN         18
#define FN_COTANH        19
#define FN_LOG           20
#define FN_EXP           21
#define FN_PWR           22
#define FN_LT            23
#define FN_FLIP          24
#define FN_REAL          25
#define FN_IMAG          26
#define FN_CONJ          27
#define FN_NEG           28
#define FN_ABS           29
#define FN_RECIP         30
#define FN_IDENT         31
#define FN_GT            32
#define FN_GTE           33
#define FN_NE            34
#define FN_EQ            35
#define FN_AND           36
#define FN_OR            37
#define FN_ZERO          38
#define FN_SQRT          39
#define FN_ASIN          40
#define FN_ACOS          41
#define FN_ASINH         42
#define FN_ACOSH         43
#define FN_ATANH         44
#define FN_ATAN          45
#define FN_CABS          46
#define FN_FLOOR         47
#define FN_CEIL          48
#define FN_TRUNC         49
#define FN_ROUND         50
#define FN_JUMP          51
#define FN_JUMP_ON_TRUE  52
#define FN_JUMP_ON_FALSE 53
#define FN_JUMP_LABEL    54
#define FN_ONE           55


/* number of "old" functions in the table.  */
/* these are the ones that the parser outputs  */

#define LAST_OLD_FN   FN_ONE
#define NUM_OLD_FNS   LAST_OLD_FN + 1

/* total number of functions in the table.  */

#define LAST_FN          FN_ONE
#define NUM_FNS          LAST_FN + 1

static unsigned char
	realstkcnt,   /* how many scalars are really on stack  */
	stkcnt,       /* # scalars on FPU stack  */
	lastsqrused,  /* was lastsqr loaded in the formula?  */
	lastsqrreal,  /* was lastsqr stored explicitly in the formula?  */
	p1const,      /* is p1 a constant?  */
	p2const,      /* ...and p2?  */
	p3const,      /* ...and p3?  */
	p4const,      /* ...and p4?  */
	p5const;      /* ...and p5?  */

static unsigned int
	cvtptrx;      /* subscript of next free entry in g_function_load_store_pointers  */

static void (*prevfptr)(void);  /* previous function pointer  */

/* the entries in this table must be in the same order as  */
/*    the #defines above  */
/* this table is searched sequentially  */
struct fn_entry
{

#ifdef TESTFP
	char *fname;  /* function name  */
#endif
	void (*infn)(void);  /* 'old' function pointer  */
			/* (infn points to an operator fn in parser.c)  */

	void (*outfn)(void);  /* pointer to equiv. fast fn.  */

	char min_regs;  /* min regs needed on stack by this fn.  */
			/* (legal values are 0, 2, 4)  */

	char free_regs;  /* free regs required by this fn  */
			/* (legal values are 0, 2, 4)  */

	char delta;  /* net change to # of values on the fp stack  */
			/* (legal values are -2, 0, +2)  */

} static afe[NUM_OLD_FNS] =  /* array of function entries  */
{

	{FNAME("Lod",     StkLod,      fStkLod,    0, 2, +2) },          /*  0  */
	{FNAME("Clr",     StkClr,      fStkClr1,   0, 0,  CLEAR_STK) },  /*  1  */
	{FNAME("+",       dStkAdd,     fStkAdd,    4, 0, -2) },          /*  2  */
	{FNAME("-",       dStkSub,     fStkSub,    4, 0, -2) },          /*  3  */
	{FNAME("*",       dStkMul,     fStkMul,    4, 2, -2) },          /*  4  */
	{FNAME("/",       dStkDiv,     fStkDiv,    4, 2, -2) },          /*  5  */
	{FNAME("Sto",     StkSto,      fStkSto,    2, 0,  0) },          /*  6  */
	{FNAME("Sqr",     dStkSqr,     fStkSqr,    2, 2,  0) },          /*  7  */
	{FNAME(":",       EndInit,     fStkEndInit, 0, 0,  CLEAR_STK) },  /*  8  */
	{FNAME("Mod",     dStkMod,     fStkMod,    2, 0,  0) },          /*  9  */
	{FNAME("<=",      dStkLTE,     fStkLTE,    4, 0, -2) },          /* 10  */
	{FNAME("Sin",     dStkSin,     fStkSin,    2, 2,  0) },          /* 11  */
	{FNAME("Cos",     dStkCos,     fStkCos,    2, 2,  0) },          /* 12  */
	{FNAME("Sinh",    dStkSinh,    fStkSinh,   2, 2,  0) },          /* 13  */
	{FNAME("Cosh",    dStkCosh,    fStkCosh,   2, 2,  0) },          /* 14  */
	{FNAME("Cosxx",   dStkCosXX,   fStkCosXX,  2, 2,  0) },          /* 15  */
	{FNAME("Tan",     dStkTan,     fStkTan,    2, 2,  0) },          /* 16  */
	{FNAME("Tanh",    dStkTanh,    fStkTanh,   2, 2,  0) },          /* 17  */
	{FNAME("CoTan",   dStkCoTan,   fStkCoTan,  2, 2,  0) },          /* 18  */
	{FNAME("CoTanh",  dStkCoTanh,  fStkCoTanh, 2, 2,  0) },          /* 19  */
	{FNAME("Log",     dStkLog,     fStkLog,    2, 2,  0) },          /* 20  */
	{FNAME("Exp",     dStkExp,     fStkExp,    2, 2,  0) },          /* 21  */
	{FNAME("^",       dStkPwr,     fStkPwr,    4, 2, -2) },          /* 22  */
	{FNAME("<",       dStkLT,      fStkLT,     4, 0, -2) },          /* 23  */
	{FNAME("Flip",    dStkFlip,    fStkFlip,   2, 0,  0) },          /* 24  */
	{FNAME("Real",    dStkReal,    fStkReal,   2, 0,  0) },          /* 25  */
	{FNAME("Imag",    dStkImag,    fStkImag,   2, 0,  0) },          /* 26  */
	{FNAME("Conj",    dStkConj,    fStkConj,   2, 0,  0) },          /* 27  */
	{FNAME("Neg",     dStkNeg,     fStkNeg,    2, 0,  0) },          /* 28  */
	{FNAME("Abs",     dStkAbs,     fStkAbs,    2, 0,  0) },          /* 29  */
	{FNAME("Recip",   dStkRecip,   fStkRecip,  2, 2,  0) },          /* 30  */
	{FNAME("Ident",   StkIdent,    fStkIdent,  2, 0,  0) },          /* 31  */
	{FNAME(">",       dStkGT,      fStkGT,     4, 0, -2) },          /* 32  */
	{FNAME(">=",      dStkGTE,     fStkGTE,    4, 0, -2) },          /* 33  */
	{FNAME("!=",      dStkNE,      fStkNE,     4, 0, -2) },          /* 34  */
	{FNAME("==",      dStkEQ,      fStkEQ,     4, 0, -2) },          /* 35  */
	{FNAME("&&",      dStkAND,     fStkAND,    4, 0, -2) },          /* 36  */
	{FNAME("||",      dStkOR,      fStkOR,     4, 0, -2) },          /* 37  */
	{FNAME("Zero",    dStkZero,    fStkZero,   2, 0,  0) },          /* 38  */
	{FNAME("Sqrt",    dStkSqrt,    fStkSqrt,   2, 2,  0) },          /* 39  */
	{FNAME("ASin",    dStkASin,    fStkASin,   2, 4,  0) },          /* 40  */
	{FNAME("ACos",    dStkACos,    fStkACos,   2, 4,  0) },          /* 41  */
	{FNAME("ASinh",   dStkASinh,   fStkASinh,  2, 4,  0) },          /* 42  */
	{FNAME("ACosh",   dStkACosh,   fStkACosh,  2, 4,  0) },          /* 43  */
	{FNAME("ATanh",   dStkATanh,   fStkATanh,  2, 4,  0) },          /* 44  */
	{FNAME("ATan",    dStkATan,    fStkATan,   2, 4,  0) },          /* 45  */
	{FNAME("CAbs",    dStkCAbs,    fStkCAbs,   2, 0,  0) },          /* 46  */
	{FNAME("Floor",   dStkFloor,   fStkFloor,  2, 0,  0) },          /* 47  */
	{FNAME("Ceil",    dStkCeil,    fStkCeil,   2, 0,  0) },          /* 48  */
	{FNAME("Trunc",   dStkTrunc,   fStkTrunc,  2, 0,  0) },          /* 49  */
	{FNAME("Round",   dStkRound,   fStkRound,  2, 0,  0) },          /* 50  */
	{FNAME("Jump",        StkJump,         fStkJump,       0, 0, 0)}, /* 51  */
	{FNAME("JumpOnTrue",  dStkJumpOnTrue,  fStkJumpOnTrue, 2, 0, 0)}, /* 52  */
	{FNAME("JumpOnFalse", dStkJumpOnFalse, fStkJumpOnFalse, 2, 0, 0)}, /* 53  */
	{FNAME("JumpLabel",   StkJumpLabel,    fStkJumpLabel,  0, 0, 0)}, /* 54  */
	{FNAME("One",     dStkOne,     fStkOne,    2, 0,  0) }           /* 55  */
};

#ifdef TESTFP
static char cDbgMsg[255];
#endif  /* TESTFP  */

static int CvtFptr(void (* ffptr)(void), int MinStk, int FreeStk,
		int Delta)
{
	union Arg  *otemp;    /* temp operand ptr  */
	union Arg *testload;
#ifdef TESTFP
	int prevstkcnt;
#endif
	double dTemp;

	int Max_On_Stack = MAX_STACK - FreeStk;  /* max regs allowed on stack  */
	int Num_To_Push; /* number of regs to push  */

	/* first do some sanity checks  */ /* CAE 15Feb95  */
	if ((Delta != -2 && Delta != 0 && Delta != 2 && Delta != CLEAR_STK)
			|| (FreeStk != 0 && FreeStk != 2 && FreeStk != 4)
			|| (MinStk != 0 && MinStk != 2 && MinStk != 4))
	{
awful_error:
		stopmsg (0, "FATAL INTERNAL PARSER ERROR!");
		return 0;  /* put out dire message and revert to old parser  */
	}

	/* this if statement inserts a stack push or pull into the token array  */
	/*   it would be much better to do this *after* optimization  */
	if ((int)stkcnt < MinStk)  /* not enough operands on g_fpu stack  */
	{
		DBUGMSG2("Inserted pull.  Stack: %2d --> %2d", stkcnt, stkcnt + 2);
		OPPTR(cvtptrx) = NO_OPERAND;
		FNPTR(cvtptrx++) = fStkPull2;  /* so adjust the stack, pull operand  */
		stkcnt += 2;
	}
	else if ((int)stkcnt > Max_On_Stack)  /* too many operands  */
	{

		Num_To_Push = stkcnt - Max_On_Stack;
		if (Num_To_Push == 2)
		{
			if (stkcnt == MAX_STACK)
			{
				/* push stack down from max to max-2  */
				FNPTR(cvtptrx) = fStkPush2;
			}
			else if (stkcnt == MAX_STACK - 2)
			{
				/* push stack down from max-2 to max-4  */
				FNPTR(cvtptrx) = fStkPush2a;
			}
			else
			{
				goto awful_error;
			}
			DBUGMSG2("Inserted push.  Stack: %2d --> %2d", stkcnt, stkcnt-2);
			OPPTR(cvtptrx++) = NO_OPERAND;
			stkcnt -= 2;
		}
		else if (Num_To_Push == 4)
		{
			/* push down from max to max-4  */
			FNPTR(cvtptrx) = fStkPush4;
			DBUGMSG2("Inserted push.  Stack: %2d --> %2d", stkcnt, stkcnt-4);
			OPPTR(cvtptrx++) = NO_OPERAND;
			stkcnt -= 4;
		}
		else
		{
			goto awful_error;
		}
	}

	/* set the operand pointer here for store function  */
	if (ffptr == fStkSto)
	{
		OPPTR(cvtptrx) = (void  *)FP_OFF((Store[g_store_ptr++]));
	}
	else if (ffptr == fStkLod && DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag)
	{
		/* when disabling optimizer, set load pointer here  */
		OPPTR(cvtptrx) = (void  *)FP_OFF((Load[g_lod_ptr++]));
	}
	else  /* the optimizer will set the pointer for load fn.  */
	{
		OPPTR(cvtptrx) = NO_OPERAND;
	}

	if (DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag)
	{
		goto SkipOptimizer;
	} /* --------------------------  begin optimizer  --------------------- */

	/* This optimizer grew from a simple if statement into a monster.  */

	/* Most of the bugs in the optimizer have been in the code that  */
	/*   juggles the overflow stack.  */

	/* For the following:  */
	/*   * == cvtptrx points to this  */
	/*  () == this is about to be added to the array  */

	/* ******************************************************************** */
	if (ffptr == fStkLod)  /* about to add Lod to the array  */
	{

		if (prevfptr == fStkLod && Load[g_lod_ptr-1] == Load[g_lod_ptr])
		{
			/* previous non-adjust operator was Lod of same operand  */
			/* ? lodx ? (*lodx)  */
			if (FNPTR(--cvtptrx) == fStkPush2) /* prev fn was push  */
			{
				/* ? lod *push (lod)  */
				--cvtptrx;  /* found  *lod push (lod)  */
				if (FNPTR(cvtptrx-1) == fStkPush2) /* always more ops here  */
				{
					DBUGMSG("push *lod push (lod) -> push4 (*loddup)");
					FNPTR(cvtptrx-1) = fStkPush4;
				}
				else  /* prev op not push  */
				{
					DBUGMSG("op *lod push (lod) -> op pusha(p=0) (*loddup)");
					OPPTR(cvtptrx) = NO_OPERAND;  /* use 'alternate' push fn.  */
					FNPTR(cvtptrx++) = fStkPush2a;  /* push w/2 free on stack  */
					/* operand ptr will be set below  */
				}
			}
			else  /* never  push *lod (lod)  so must be  */
			{
				DBUGMSG("op *lod (lod) -> op (*loddup)");
			}
			ffptr = fStkLodDup;
		}
		else if (prevfptr == fStkSto2
				&& Store[g_store_ptr-1] == Load[g_lod_ptr])
				{
			/* store, load of same value  */
			/* only one operand on stack here when prev oper is Sto2  */
			DBUGMSG("*sto2 (lod) -> (*stodup)");
			--cvtptrx;
			ffptr = fStkStoDup;
		}
		/* This may cause roundoff problems when later operators  */
		/*  use the rounded value that was stored here, while the next  */
		/*  operator uses the more accurate internal value.  */
		else if (prevfptr == fStkStoClr2
					&& Store[g_store_ptr-1] == Load[g_lod_ptr])
					{
			/* store, clear, load same value found  */
			/* only one operand was on stack so this is safe  */
			DBUGMSG("*StoClr2 (Lod) -> (*Sto2)");
			--cvtptrx;
			ffptr = fStkSto2;  /* use different Sto fn  */
		}
		else
		{
			testload = Load[g_lod_ptr];
			if (testload == &LASTSQR && lastsqrreal)
			{
				/* -- LastSqr is a real.  CAE 31OCT93  */
				DBUGMSG("(*lod[lastsqr]) -> (*lodreal)");
				ffptr = fStkLodReal;
			}
			else if (IS_CONST(testload) && testload->d.y == 0.0)
			{
				DBUGMSG("(*lod) -> (*lodrealc)");
				ffptr = fStkLodRealC;  /* a real const is being loaded  */
			}
		}
		/* set the operand ptr here  */
		OPPTR(cvtptrx) = (void  *)FP_OFF((Load[g_lod_ptr++]));
	}
	/* ******************************************************************** */
	else if (ffptr == fStkAdd)
	{

		if (prevfptr == fStkLodDup) /* there is never a push before add  */
		{
			--cvtptrx;  /* found  ? *loddup (add)  */
			if (cvtptrx != 0 && FNPTR(cvtptrx-1) == fStkPush2a)
			{
				/* because  push lod lod  is impossible so is  push loddup  */
				DBUGMSG("pusha *loddup (add) -> (*loddbl),stk += 2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptr  */
			}
			else if (cvtptrx != 0 && FNPTR(cvtptrx-1) == fStkPush4)
			{
				DBUGMSG("push4 *loddup (add) -> push2 (*loddbl),stk+=2");
				FNPTR(cvtptrx-1) = fStkPush2;
				stkcnt += 2;  /*  CAE added 12 July 1993 to fix bug  */
			}
			else
			{
				DBUGMSG("op *loddup (add) -> op (*loddbl)");
			}
			ffptr = fStkLodDbl;
		}
		else if (prevfptr == fStkStoDup)
		{
			DBUGMSG("stodup (*add) -> (*stodbl)");
			/* there are always exactly 4 on stack here  */
			--cvtptrx;
			ffptr = fStkStoDbl;
		}
		else if (prevfptr == fStkLod) /* have found  lod (*add)  */
		{
			--cvtptrx;     /*  ? *lod (add)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push load (add) -> (*plodadd),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
				ffptr = fStkPLodAdd;
			}
			else
			{
				DBUGMSG("op *lod (add) -> op (*lodadd)");
				ffptr = fStkLodAdd;
			}
		}
		else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
		{
			--cvtptrx;  /* found  ? *lodreal (add)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push lodreal (add) -> (*lodrealadd),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
			}
			else
			{
				DBUGMSG("*lodreal (add) -> (*lodrealadd)");
			}
			ffptr = fStkLodRealAdd;
		}
		else if (prevfptr == fStkLodImag) /* CAE 4DEC93  */
		{
			--cvtptrx;  /* found  ? *lodimag (add)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push lodimag (add) -> (*lodimagadd),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
			}
			else
			{
				DBUGMSG("*lodimag (add) -> (*lodimagadd)");
			}
			ffptr = fStkLodImagAdd;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkSub)
	{

		if (prevfptr == fStkLod)
		{
			/* found  lod (*sub)  */
			--cvtptrx;  /*  *lod (sub)  */
			/* there is never a sequence (lod push sub)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push lod (sub) -> (*plodsub),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
				ffptr = fStkPLodSub;
			}
			else
			{
				DBUGMSG("*lod (sub) -> (*lodsub)");
				ffptr = fStkLodSub;
			}
		}
		else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
		{
			--cvtptrx;  /*  ? *lodreal (sub)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push lodreal (sub) -> (*lodrealsub),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
			}
			else
			{
				DBUGMSG("*lodreal (sub) -> (*lodrealsub)");
			}
			ffptr = fStkLodRealSub;
		}
		else if (prevfptr == fStkLodImag) /* CAE 4DEC93  */
		{
			--cvtptrx;  /*  ? *lodimag (sub)  */
			if (FNPTR(cvtptrx-1) == fStkPush2)
			{
				DBUGMSG("*push lodimag (sub) -> (*lodimagsub),stk+=2");
				REMOVE_PUSH;
				OPPTR(cvtptrx) = OPPTR(cvtptrx + 1);  /* fix opptrs  */
			}
			else
			{
				DBUGMSG("*lodimag (sub) -> (*lodimagsub)");
			}
			ffptr = fStkLodImagSub;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkMul)
	{

		if (prevfptr == fStkLodDup)
		{
			/* found  loddup ? (*mul)  */
			if (FNPTR(--cvtptrx) == fStkPush2)
			{
				DBUGMSG("loddup *push (mul) -> (*lodsqr),stk+=2");
				REMOVE_PUSH;
			}
			else
			{
				DBUGMSG("*loddup (mul) -> (*lodsqr)");
			}
			ffptr = fStkLodSqr;
		}
		else if (prevfptr == fStkStoDup) /* no pushes here, 4 on stk.  */
		{
			DBUGMSG("stodup (mul) -> (*stosqr0)");
			--cvtptrx;
			ffptr = fStkStoSqr0;  /* dont save lastsqr here ever  */
		}
		else if (prevfptr == fStkLod)
		{
			--cvtptrx;  /*  lod *? (mul)  */
			if (FNPTR(cvtptrx) == fStkPush2) /*  lod *push (mul)  */
			{
				--cvtptrx;  /* ? *lod push (mul)  */
				if (FNPTR(cvtptrx-1) == fStkPush2)
				{
					DBUGMSG("push *lod push (mul) -> push4 (*lodmul)");
					FNPTR(cvtptrx-1) = fStkPush4;
				}
				else
				{
					DBUGMSG("op *lod push (mul) -> op pusha (*lodmul)");
					OPPTR(cvtptrx + 1) = OPPTR(cvtptrx);  /* fix operand ptr  */
					FNPTR(cvtptrx) = fStkPush2a;
					OPPTR(cvtptrx) = NO_OPERAND;
					cvtptrx++;
				}
			}
			else
			{
				DBUGMSG("*lod (mul) -> (*lodmul)");
			}
			ffptr = fStkLodMul;

			/**********************  begin extension  ***  CAE 9 Oct 93  ****/
			/*  change loadreal a, lodmul b --> lod b, lodrealmul a  */

			FNPTR(cvtptrx) = NO_FUNCTION;  /* mark the pending fn as null  */
			if (FNPTR(cvtptrx-1) == fStkPush4
					|| FNPTR(cvtptrx-1) == fStkPush2a)
					{
				--cvtptrx;  /* look back past this push  */
			}

			if (FNPTR(cvtptrx-1) == fStkLodRealC
					&& Load[g_lod_ptr-2]->d.x == _2_)
					{
				/* -- Convert '2*a' into 'a + a'.                CAE 31OCT93  */
				if (FNPTR(cvtptrx) == NO_FUNCTION)
				{
					DBUGMSG("lodreal[2] (*lodmul[b])"
							" -> (*loddbl[b])");
					OPPTR(cvtptrx-1) = OPPTR(cvtptrx);
				}
				else if (FNPTR(cvtptrx) == fStkPush2a)
				{
					DBUGMSG("lodreal[2] *pusha (lodmul[b])"
							" -> loddbl[b],stk+=2");
					OPPTR(cvtptrx-1) = OPPTR(cvtptrx + 1);
					stkcnt += 2;
				}
				else if (FNPTR(cvtptrx) == fStkPush4)
				{
					DBUGMSG("lodreal[2] *push4 (lodmul[b])"
							" -> loddbl[b],stk+=4");
					OPPTR(cvtptrx-1) = OPPTR(cvtptrx + 1);
					stkcnt += 4;
				}
				FNPTR(--cvtptrx) = NO_FUNCTION;  /* so no increment later  */
				ffptr = fStkLodDbl;
			}
			else if (FNPTR(cvtptrx-1) == fStkLodReal
					|| FNPTR(cvtptrx-1) == fStkLodRealC)
					{
				/* lodreal *?push?? (*?lodmul)  */
				otemp = OPPTR(cvtptrx-1);  /* save previous fn's operand  */
				FNPTR(cvtptrx-1) = fStkLod;  /* prev fn = lod  */
				/* Moved setting of prev lodptr to below         CAE 31DEC93  */
				/* This was a bug causing a bad loadptr to be set here  */
				/* 3 lines marked 'prev lodptr = this' below replace this line  */
				if (FNPTR(cvtptrx) == NO_FUNCTION)
				{
					DBUGMSG("lodreal[a] (*lodmul[b])"
						" -> lod[b] (*lodrealmul[a])");
					OPPTR(cvtptrx-1) = OPPTR(cvtptrx);  /* prev lodptr = this  */
				}
				else if (FNPTR(cvtptrx) == fStkPush2a)
				{
					DBUGMSG("lodreal[a] *pusha (lodmul[b])"
							" -> lod[b] (*lodrealmul[a]),stk+=2");
					/* set this fn ptr to null so cvtptrx won't be incr later  */
					FNPTR(cvtptrx) = NO_FUNCTION;
					OPPTR(cvtptrx-1) = OPPTR(cvtptrx + 1);  /* prev lodptr = this  */
					stkcnt += 2;
				}
				else if (FNPTR(cvtptrx) == fStkPush4)
				{
					DBUGMSG("lodreal[a] *push4 (lodmul[b])"
							" -> lod[b] push2 (*lodrealmul[a]),stk+=2");
					FNPTR(cvtptrx++) = fStkPush2;
					OPPTR(cvtptrx-2) = OPPTR(cvtptrx);  /* prev lodptr = this  */
					/* we know cvtptrx points to a null function now  */
					stkcnt += 2;
				}
				OPPTR(cvtptrx) = otemp;  /* switch the operands  */
				ffptr = fStkLodRealMul;  /* next fn is now lodrealmul  */
			}

			if (FNPTR(cvtptrx) != NO_FUNCTION)
			{
				cvtptrx++;  /* adjust cvtptrx back to normal if needed  */
			}
			/* **********************  end extension  *********************** */
		}
		else if (prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
		{

			--cvtptrx;  /* found  lodreal *? (mul)  */
			if (FNPTR(cvtptrx) == fStkPush2)
			{
				DBUGMSG("lodreal *push2 (mul) -> (*lodrealmul),stk+=2");
				REMOVE_PUSH;
			}
			else
			{
				DBUGMSG("*lodreal (mul) -> (*lodrealmul)");
			}
			ffptr = fStkLodRealMul;

			/**********************  begin extension  ***  CAE 31OCT93  ****/
			if (prevfptr == fStkLodRealC  /* use prevfptr here  */
					&& Load[g_lod_ptr-1]->d.x == _2_)
					{
				if (FNPTR(cvtptrx) == fStkPush2)
				{
					DBUGMSG("push (*lodrealmul[2]) -> (*dbl),stk+=2");
					REMOVE_PUSH;
				}
				else
				{
					DBUGMSG("*lodrealmul[2] -> (*dbl)");
				}
				OPPTR(cvtptrx) = NO_OPERAND;
				ffptr = fStkDbl;

				if (FNPTR(cvtptrx-1) == fStkLod)
				{
					DBUGMSG("lod (*dbl) -> (*loddbl)");
					--cvtptrx;
					ffptr = fStkLodDbl;
				}
				else if (FNPTR(cvtptrx-1) == fStkSto2)
				{
					DBUGMSG("sto2 (*dbl) -> (*stodbl)");
					--cvtptrx;
					ffptr = fStkStoDbl;
				}
			}
			/************************  end extension  ***  CAE 31OCT93  ****/
		}
		else if (prevfptr == fStkLodImag) /* CAE 4DEC93  */
		{

			--cvtptrx;  /* found  lodimag *? (mul)  */
			if (FNPTR(cvtptrx) == fStkPush2)
			{
				DBUGMSG("lodimag *push2 (mul) -> (*lodimagmul),stk+=2");
				REMOVE_PUSH;
			}
			else
			{
				DBUGMSG("*lodimag (mul) -> (*lodimagmul)");
			}
			ffptr = fStkLodImagMul;
		}
		else if (prevfptr == fStkLodLT && FNPTR(cvtptrx-1) != fStkPull2)
		{
			/* this shortcut fails if  Lod LT Pull Mul  found  */
			DBUGMSG("LodLT (*Mul) -> (*LodLTMul)");
			--cvtptrx;  /* never  lod LT Push Mul  here  */
			ffptr = fStkLodLTMul;
		}
		else if (prevfptr == fStkLodLTE && FNPTR(cvtptrx-1) != fStkPull2)
		{
			DBUGMSG("LodLTE (*mul) -> (*LodLTEmul)");
			--cvtptrx;
			ffptr = fStkLodLTEMul;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkClr1 && prevfptr == fStkSto)
	{

		--cvtptrx;
		if (stkcnt == 2)
		{
			DBUGMSG("sto (*clr1) -> (*stoclr2)");
			ffptr = fStkStoClr2;
		}
		else
		{
			DBUGMSG("sto (*clr1) -> (*stoclr1)");
			ffptr = fStkStoClr1;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkDiv)
	{

		if (prevfptr == fStkLodRealC && g_parser_vsp < g_formula_max_args - 1)
		{
			/* have found a divide by a real constant  */
			/*  and there is space to create a new one  */
			/* lodrealc ? (*div)  */
			if (FNPTR(--cvtptrx) == fStkPush2)
			{
				DBUGMSG("lodrealc *push (div) -> (*lodrealmul),stk+=2");
				REMOVE_PUSH;
			}
			else
			{
				DBUGMSG("*lodrealc (div) -> (*lodrealmul)");
			}
			v[g_parser_vsp].s = (void  *)0;  /* this constant has no name  */
			v[g_parser_vsp].len = 0;
			v[g_parser_vsp].a.d.x = _1_ / Load[g_lod_ptr-1]->d.x;
			v[g_parser_vsp].a.d.y = 0.0;
			{
				void *p = &v[g_parser_vsp++].a;
				OPPTR(cvtptrx) = (void  *)FP_OFF(p);  /* isn't C fun!  */
			}
			ffptr = fStkLodRealMul;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkReal)
	{

		if (prevfptr == fStkLod)
		{
			DBUGMSG("lod (*real) -> (*lodreal)");
			--cvtptrx;
			ffptr = fStkLodReal;
		}
		else if (stkcnt < MAX_STACK)
		{
			DBUGMSG("(*real) -> (*real2)");
			ffptr = fStkReal2;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkImag && prevfptr == fStkLod)
	{

		DBUGMSG("lod (*imag) -> lodimag");
		--cvtptrx;
		ffptr = fStkLodImag;
	}
	/* ******************************************************************** */
	else if (ffptr == fStkConj && prevfptr == fStkLod)
	{

		DBUGMSG("lod (*conj) -> (*lodconj)");
		--cvtptrx;
		ffptr = fStkLodConj;
	}
	/* ******************************************************************** */
	else if (ffptr == fStkMod && stkcnt < MAX_STACK)
	{

		DBUGMSG("(*mod) -> (*mod2)");
		ffptr = fStkMod2;  /* use faster version if room on stack  */
		if (prevfptr == fStkLod)
		{
			DBUGMSG("lod (*mod2) -> (*lodmod2)");
			--cvtptrx;
			ffptr = fStkLodMod2;
		}
		else if (prevfptr == fStkSto || prevfptr == fStkSto2)
		{
			DBUGMSG("sto (*mod2) -> (*stomod2)");
			--cvtptrx;
			ffptr = fStkStoMod2;
		}
		else if (prevfptr == fStkLodSub)
		{
			DBUGMSG("lodsub (*mod2) -> (*lodsubmod)");
			--cvtptrx;
			ffptr = fStkLodSubMod;
		}
		else if (STACK_TOP_IS_REAL) /* CAE 06NOV93  */
		{
			DBUGMSG("(*mod2[st real]) -> (*sqr3)");
			ffptr = fStkSqr3;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkFlip)
	{

		if (prevfptr == fStkReal || prevfptr == fStkReal2)
		{
			DBUGMSG("real (*flip) -> (*realflip)");
			--cvtptrx;
			ffptr = fStkRealFlip;
		}
		else if (prevfptr == fStkImag)
		{
			DBUGMSG("imag (*flip) -> (*imagflip)");
			--cvtptrx;
			ffptr = fStkImagFlip;
		}
		else if (prevfptr == fStkLodReal)
		{
			DBUGMSG("lodreal (*flip) -> (*lodrealflip)");
			--cvtptrx;
			ffptr = fStkLodRealFlip;
		}
		else if (prevfptr == fStkLodImag)
		{
			DBUGMSG("lodimag (*flip) -> (*lodimagflip)");
			--cvtptrx;
			ffptr = fStkLodImagFlip;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkAbs)
	{

		if (prevfptr == fStkLodReal)
		{
			DBUGMSG("lodreal (*abs) -> (*lodrealabs)");
			--cvtptrx;
			ffptr = fStkLodRealAbs;
		}
		else if (prevfptr == fStkLodImag)
		{
			DBUGMSG("lodimag (*abs) -> (*lodimagabs)");
			--cvtptrx;
			ffptr = fStkLodImagAbs;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkSqr)
	{

		if (prevfptr == fStkLod && FNPTR(cvtptrx-1) != fStkPush2)
		{
			DBUGMSG("lod (*sqr) -> (*lodsqr)");
			--cvtptrx;
			ffptr = fStkLodSqr;  /* assume no need to save lastsqr  */
			if (lastsqrused)
			{
				DBUGMSG("(*lodsqr) -> (*lodsqr2)");
				ffptr = fStkLodSqr2;  /* lastsqr is being used  */
			}
		}
		else if (prevfptr == fStkSto2)
		{
			DBUGMSG("sto2 (*sqr) -> (*stosqr0)");
			--cvtptrx;
			ffptr = fStkStoSqr0;  /* assume no need to save lastsqr  */
			if (lastsqrused)
			{
				DBUGMSG("(*stosqr0) -> (*stosqr)");
				ffptr = fStkStoSqr;  /* save lastsqr  */
			}
		}
		else
		{
			if (!lastsqrused)
			{
				DBUGMSG("(*sqr) -> (*sqr0)");
				ffptr = fStkSqr0;  /* don't save lastsqr  */
				if (STACK_TOP_IS_REAL) /* CAE 06NOV93  */
				{
					DBUGMSG("(*sqr0[st real]) -> (*sqr3)");
					ffptr = fStkSqr3;
				}
			}
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkPwr)
	{

		if (prevfptr == fStkLodRealC)
		{
			dTemp = Load[g_lod_ptr-1]->d.x;
			if (dTemp == _2_ || dTemp == _1_ || dTemp == -1.0 || dTemp == 0.0)
			{
				/* change ^[-1, 0, 1, or 2] to recip, one, ident, sqr  CAE 06NOV93  */
				if (FNPTR(cvtptrx-1) == fStkPush2)
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
				--cvtptrx;
				OPPTR(cvtptrx) = NO_OPERAND;
				if (dTemp == _2_)
				{
					DBUGMSG("[]=Sqr0");
					ffptr = fStkSqr0;  /* no need to compute lastsqr here  */
					if (FNPTR(cvtptrx-1) == fStkLod)
					{
						DBUGMSG("Lod (*Sqr0) -> (*LodSqr)");
						--cvtptrx;
						ffptr = fStkLodSqr;  /* dont save lastsqr  */
					}
					else if (FNPTR(cvtptrx-1) == fStkSto2)
					{
						DBUGMSG("Sto2 (*Sqr0) -> (*StoSqr0)");
						--cvtptrx;
						ffptr = fStkStoSqr0;  /* dont save lastsqr  */
					}
				}
				else if (dTemp == _1_)
				{
					DBUGMSG("[]=Ident");
					ffptr = fStkIdent;
				}
				else if (dTemp == 0.0)
				{
					DBUGMSG("[]=One");
					ffptr = fStkOne;
				}
				else if (dTemp == -1.0)
				{
					DBUGMSG("[]=Recip");
					ffptr = fStkRecip;
				}
			}
			else if (FNPTR(cvtptrx-1) == prevfptr)
			{
				--cvtptrx;
				ffptr = fStkLodRealPwr;  /* see comments below  */
			}
		}
		else if (prevfptr == fStkLodReal && FNPTR(cvtptrx-1) == prevfptr)
		{
			/* CAE 6NOV93  */
			/* don't handle pushes here, lodrealpwr needs 4 free  */
			DBUGMSG("LodReal (*Pwr) -> (*LodRealPwr)");
			--cvtptrx;
			ffptr = fStkLodRealPwr;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkLTE)
	{

		if (prevfptr == fStkLod
				|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*LTE) -> (*LodLTE)");
			--cvtptrx;
			ffptr = fStkLodLTE;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkLT)
	{

		if (prevfptr == fStkLod || prevfptr == fStkLodReal
				|| prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*LT) -> (*LodLT)");
			--cvtptrx;
			ffptr = fStkLodLT;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkGT)
	{

		if (prevfptr == fStkLod
				|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*GT) -> (*LodGT)");
			--cvtptrx;
			ffptr = fStkLodGT;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkGTE)
	{

		if (prevfptr == fStkLod
				|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*GTE) -> (*LodGTE)");
			--cvtptrx;
			ffptr = fStkLodGTE;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkNE)
	{

		if (prevfptr == fStkLod
				|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*NE) -> (*LodNE)");
			--cvtptrx;
			ffptr = fStkLodNE;
		}
	}
	/* ******************************************************************** */
	else if (ffptr == fStkEQ)
	{

		if (prevfptr == fStkLod
				|| prevfptr == fStkLodReal || prevfptr == fStkLodRealC)
				{
			DBUGMSG("Lod (*EQ) -> (*LodEQ)");
			--cvtptrx;
			ffptr = fStkLodEQ;
		}
	}
	/* ******************************************************************** */
SkipOptimizer:  /* -------------  end of optimizer ----------------------- */

	FNPTR(cvtptrx++) = prevfptr = ffptr;
#ifdef TESTFP
	prevstkcnt = stkcnt;
#endif
	if (Delta == CLEAR_STK)
	{
		realstkcnt = stkcnt = 0;
	}
	else
	{
		stkcnt = (unsigned char)(stkcnt + Delta);
		realstkcnt = (unsigned char)(realstkcnt + Delta);
	}

	DBUGMSG3("Stack:  %2d --> %2d,  Real stack:  %2d",
			prevstkcnt, stkcnt, realstkcnt);

	return 1;  /* return 1 for success  */
}

int fpfill_jump_struct(void)
{
	/* Completes all entries in jump structure. Returns 1 on error) */
	/* On entry, jump_index is the number of jump functions in the formula*/
	int i = 0;
	int checkforelse = 0;
	NEW_FN  * JumpFunc = NULL;
	int find_new_func = 1;
	JUMP_PTRS_ST jump_data[MAX_JUMPS];

	for (OpPtr = 0; OpPtr < (int) g_last_op; OpPtr++)
	{
		if (find_new_func)
		{
			switch (jump_control[i].type)
			{
			case 1:
				JumpFunc = fStkJumpOnFalse;
				break;
			case 2:
				checkforelse = !checkforelse;
				JumpFunc = checkforelse ? fStkJump : fStkJumpOnFalse;
				break;
			case 3:
				JumpFunc = fStkJump;
				break;
			case 4:
				JumpFunc = fStkJumpLabel;
				break;
			default:
				break;
			}
			find_new_func = 0;
		}
		if (g_function_load_store_pointers[OpPtr].function == JumpFunc)
		{
			jump_data[i].JumpOpPtr = OpPtr*4;
			i++;
			find_new_func = 1;
		}
	}

		/* Following for safety only; all should always be false */
	if (i != jump_index || jump_control[i - 1].type != 4
		|| jump_control[0].type != 1)
	{
		return 1;
	}

	while (i > 0)
	{
		i--;
		i = fill_if_group(i, jump_data);
	}
	return i < 0 ? 1 : 0;
}

extern int fform_per_pixel(void);       /* these fns are in parsera.asm  */
extern int BadFormula(void);
extern void (Img_Setup)(void);

int CvtStk()  /* convert the array of ptrs  */
{
	extern char g_formula_name[];
	void (*ftst)(void);
	void (*ntst)(void);
	union Arg *testoperand;
	struct fn_entry *pfe;
	int fnfound;

	lastsqrreal = 1;  /* assume lastsqr is real (not stored explicitly)  */
	p1const = p2const = p3const = (unsigned char)1;  /* . . . p1, p2, p3 const  */
	p4const = p5const = (unsigned char)1;  /* . . . p4, p5 const  */
	lastsqrused = 0;  /* ... and LastSqr is not used  */

	/* now see if the above assumptions are true */
	for (OpPtr = g_lod_ptr = g_store_ptr = 0; OpPtr < (int)g_last_op; OpPtr++)
	{
		ftst = f[OpPtr];
		if (ftst == StkLod)
		{
			if (Load[g_lod_ptr++] == &LASTSQR)
			{
				lastsqrused = 1;
			}
		}
		else if (ftst == StkSto)
		{
			testoperand = Store[g_store_ptr++];
			if (testoperand == &PARM1)
			{
				p1const = 0;
			}
			else if (testoperand == &PARM2)
			{
				p2const = 0;
			}
			else if (testoperand == &PARM3)
			{
				p3const = 0;
			}
			else if (testoperand == &PARM4)
			{
				p4const = 0;
			}
			else if (testoperand == &PARM5)
			{
				p5const = 0;
			}
			else if (testoperand == &LASTSQR)
			{
				lastsqrreal = 0;
			}
		}
	}

	if (!p1const)
	{
		DBUGMSG("p1 not constant");
	}
	if (!p2const)
	{
		DBUGMSG("p2 not constant");
	}
	if (!p3const)
	{
		DBUGMSG("p3 not constant");
	}
	if (!p4const)
	{
		DBUGMSG("p4 not constant");
	}
	if (!p5const)
	{
		DBUGMSG("p5 not constant");
	}
	if (lastsqrused)
	{
		DBUGMSG("LastSqr loaded");
		if (!lastsqrreal)
		{
			DBUGMSG("LastSqr stored");
		}
	}

	if (f[g_last_op-1] != StkClr)
	{
		DBUGMSG("Missing clr added at end");
		/* should be safe to modify this  */
		f[g_last_op++] = StkClr;
	}

	prevfptr = (void (*)(void))0;
	cvtptrx = realstkcnt = stkcnt = 0;

	for (OpPtr = g_lod_ptr = g_store_ptr = 0; OpPtr < (int)g_last_op; OpPtr++)
	{
		ftst = f[OpPtr];
		fnfound = 0;
		for (pfe = &afe[0]; pfe <= &afe[LAST_OLD_FN]; pfe++)
		{
			if (ftst == pfe->infn)
			{
				fnfound = 1;
				ntst = pfe->outfn;
				if (ntst == fStkClr1 && OpPtr == (int)(g_last_op-1))
				{
					ntst = fStkClr2;  /* convert the last clear to a clr2  */
					DBUGMSG("Last fn (CLR) --> (is really CLR2)");
				}
				if (ntst == fStkIdent && g_debug_flag != DEBUGFLAG_SKIP_OPTIMIZER)
				{
					/* ident will be skipped here  */
					/* this is really part of the optimizer  */
					DBUGMSG("IDENT was skipped");
				}
				else
				{
					DBUGMSG4("fn=%s, minstk=%1i, freestk=%1i, delta=%3i",
							pfe->fname,
							(int)(pfe->min_regs),
							(int)(pfe->free_regs),
							(int)(pfe->delta));
					if (!CvtFptr(ntst, pfe->min_regs, pfe->free_regs, pfe->delta))
					{
						return 1;
					}
				}
			}
		}
		if (!fnfound)
		{
			/* return success so old code will be used  */
			/* stopmsg(0, "Fast 387 parser failed, reverting to slower code.");*/
			return 1;  /* this should only happen if random numbers are used  */
		}
	} /* end for  */

	if (DEBUGFLAG_SKIP_OPTIMIZER == g_debug_flag)
	{
		goto skipfinalopt;
	} /* ------------------------------ final optimizations ---------- */

	/* cvtptrx -> one past last operator (always clr2)  */
	--cvtptrx;  /* now it points to the last operator  */
	ntst = FNPTR(cvtptrx-1);
	/* ntst is the next-to-last operator  */

	if (ntst == fStkLT)
	{
		DBUGMSG("LT Clr2 -> LT2");
		FNPTR(cvtptrx-1) = fStkLT2;
	}
	else if (ntst == fStkLodLT)
	{
		DBUGMSG("LodLT Clr2 -> LodLT2");
		FNPTR(cvtptrx-1) = fStkLodLT2;
	}
	else if (ntst == fStkLTE)
	{
		DBUGMSG("LTE Clr2 -> LTE2");
		FNPTR(cvtptrx-1) = fStkLTE2;
	}
	else if (ntst == fStkLodLTE)
	{
		DBUGMSG("LodLTE Clr2 -> LodLTE2");
		FNPTR(cvtptrx-1) = fStkLodLTE2;
	}
	else if (ntst == fStkGT)
	{
		DBUGMSG("GT Clr2 -> GT2");
		FNPTR(cvtptrx-1) = fStkGT2;
	}
	else if (ntst == fStkLodGT)
	{
		DBUGMSG("LodGT Clr2 -> LodGT2");
		FNPTR(cvtptrx-1) = fStkLodGT2;
	}
	else if (ntst == fStkLodGTE)
	{
		DBUGMSG("LodGTE Clr2 -> LodGTE2");
		FNPTR(cvtptrx-1) = fStkLodGTE2;
	}
	else if (ntst == fStkAND)
	{
		DBUGMSG("AND Clr2 -> ANDClr2");
		FNPTR(cvtptrx-1) = fStkANDClr2;
		ntst = FNPTR(cvtptrx-2);
		if (ntst == fStkLodLTE)
		{
			DBUGMSG("LodLTE ANDClr2 -> LodLTEAnd2");
			--cvtptrx;
			FNPTR(cvtptrx-1) = fStkLodLTEAnd2;
		}
	}
	else if (ntst == fStkOR) /* CAE 06NOV93  */
	{
		DBUGMSG("OR Clr2 -> ORClr2");
		FNPTR(cvtptrx-1) = fStkORClr2;
	}
	else
	{
		++cvtptrx;  /* adjust this back since no optimization was found  */
	}

skipfinalopt:  /* -------------- end of final optimizations ------------ */

	g_last_op = cvtptrx;  /* save the new operator count  */
	LASTSQR.d.y = 0.0;  /* do this once per image  */

	/* now change the pointers  */
	if (g_formula_name[0] != 0 &&
		(uses_jump == 0 || fpfill_jump_struct() == 0)) /* but only if parse succeeded  */
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
	return 1;
}

#endif /* XFRACT  */
