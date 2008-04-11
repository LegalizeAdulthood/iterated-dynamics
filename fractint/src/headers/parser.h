#if !defined(PARSER_H)
#define PARSER_H

/*** Formula Declarations ***/
enum MathType
{
	FLOATING_POINT_MATH,
	FIXED_POINT_MATH
};

// the following are declared 4 dimensional as an experiment
// changing declarations to ComplexD and ComplexL restores the code
// to 2D
union Arg
{
   ComplexD d;
   ComplexL l;
};

struct ConstArg
{
   const char *name;
   int name_length;
   Arg argument;
};

extern Arg *g_argument1;
extern Arg *g_argument2;

extern void lStkSin();
extern void lStkCos();
extern void lStkSinh();
extern void lStkCosh();
extern void lStkLog();
extern void lStkExp();
extern void lStkSqr();
extern void dStkSin();
extern void dStkCos();
extern void dStkSinh();
extern void dStkCosh();
extern void dStkLog();
extern void dStkExp();
extern void dStkSqr();

typedef void OLD_FN();  // old C functions

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

#endif
