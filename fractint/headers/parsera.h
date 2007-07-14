#if !defined(PARSERA_H)
#define PARSERA_H

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

extern int formula_per_pixel_fp();
extern int formula_fp();
extern void image_setup();

#endif
