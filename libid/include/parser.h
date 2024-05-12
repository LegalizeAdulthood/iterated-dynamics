#pragma once

#include "mpmath.h"

#include <vector>

#define MAX_JUMPS 200  // size of JUMP_CONTROL array

struct JUMP_PTRS_ST
{
    int      JumpOpPtr;
    int      JumpLodPtr;
    int      JumpStoPtr;
};

enum class jump_control_type
{
    NONE = 0,
    IF = 1,
    ELSE_IF = 2,
    ELSE = 3,
    END_IF = 4
};

struct JUMP_CONTROL_ST
{
    jump_control_type type;
    JUMP_PTRS_ST ptrs;
    int DestJumpIndex;
};

// function, load, store pointers
struct fn_operand
{
    void (*function)();
    Arg *operand;
};

extern bool                  g_frm_uses_ismand;
extern bool                  g_frm_uses_p1;
extern bool                  g_frm_uses_p2;
extern bool                  g_frm_uses_p3;
extern bool                  g_frm_uses_p4;
extern bool                  g_frm_uses_p5;
extern double                g_fudge_limit;
extern std::vector<fn_operand> g_function_operands;
extern bool                  g_is_mandelbrot;
extern int                   g_last_init_op;
extern unsigned              g_last_op;
extern int                   g_load_index;
extern char                  g_max_function;
extern unsigned              g_max_function_args;
extern unsigned              g_max_function_ops;
extern unsigned              g_operation_index;
extern int                   g_store_index;
extern unsigned              g_variable_index;

unsigned long NewRandNum();
void lRandom();
void dRandom();
void mRandom();
void SetRandFnct();
void RandomSeed();
void lStkSRand();
void mStkSRand();
void dStkSRand();
void dStkAbs();
void mStkAbs();
void lStkAbs();
void dStkSqr();
void mStkSqr();
void lStkSqr();
void dStkAdd();
void mStkAdd();
void lStkAdd();
void dStkSub();
void mStkSub();
void lStkSub();
void dStkConj();
void mStkConj();
void lStkConj();
void dStkZero();
void mStkZero();
void lStkZero();
void dStkOne();
void mStkOne();
void lStkOne();
void dStkReal();
void mStkReal();
void lStkReal();
void dStkImag();
void mStkImag();
void lStkImag();
void dStkNeg();
void mStkNeg();
void lStkNeg();
void dStkMul();
void mStkMul();
void lStkMul();
void dStkDiv();
void mStkDiv();
void lStkDiv();
void StkSto();
void StkLod();
void dStkMod();
void mStkMod();
void lStkMod();
void StkClr();
void dStkFlip();
void mStkFlip();
void lStkFlip();
void dStkSin();
void mStkSin();
void lStkSin();
void dStkTan();
void mStkTan();
void lStkTan();
void dStkTanh();
void mStkTanh();
void lStkTanh();
void dStkCoTan();
void mStkCoTan();
void lStkCoTan();
void dStkCoTanh();
void mStkCoTanh();
void lStkCoTanh();
void dStkRecip();
void mStkRecip();
void lStkRecip();
void StkIdent();
void dStkSinh();
void mStkSinh();
void lStkSinh();
void dStkCos();
void mStkCos();
void lStkCos();
void dStkCosXX();
void mStkCosXX();
void lStkCosXX();
void dStkCosh();
void mStkCosh();
void lStkCosh();
void dStkLT();
void mStkLT();
void lStkLT();
void dStkGT();
void mStkGT();
void lStkGT();
void dStkLTE();
void mStkLTE();
void lStkLTE();
void dStkGTE();
void mStkGTE();
void lStkGTE();
void dStkEQ();
void mStkEQ();
void lStkEQ();
void dStkNE();
void mStkNE();
void lStkNE();
void dStkOR();
void mStkOR();
void lStkOR();
void dStkAND();
void mStkAND();
void lStkAND();
void dStkLog();
void mStkLog();
void lStkLog();
void dStkExp();
void mStkExp();
void lStkExp();
void dStkPwr();
void mStkPwr();
void lStkPwr();
void dStkASin();
void mStkASin();
void lStkASin();
void dStkASinh();
void mStkASinh();
void lStkASinh();
void dStkACos();
void mStkACos();
void lStkACos();
void dStkACosh();
void mStkACosh();
void lStkACosh();
void dStkATan();
void mStkATan();
void lStkATan();
void dStkATanh();
void mStkATanh();
void lStkATanh();
void dStkCAbs();
void mStkCAbs();
void lStkCAbs();
void dStkSqrt();
void mStkSqrt();
void lStkSqrt();
void dStkFloor();
void mStkFloor();
void lStkFloor();
void dStkCeil();
void mStkCeil();
void lStkCeil();
void dStkTrunc();
void mStkTrunc();
void lStkTrunc();
void dStkRound();
void mStkRound();
void lStkRound();
void EndInit();
void NotAFnct();
void FnctNotFound();
int CvtStk();
int fFormula();
void RecSortPrec();
int Formula();
int BadFormula();
int form_per_pixel();
int frm_get_param_stuff(char const *Name);
bool run_formula(const std::string &name, bool report_bad_sym);
bool fpFormulaSetup();
bool intFormulaSetup();
void init_misc();
void free_workarea();
int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);
