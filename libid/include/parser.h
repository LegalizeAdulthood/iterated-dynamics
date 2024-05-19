#pragma once

#include <string>

extern bool                  g_frm_uses_ismand;
extern bool                  g_frm_uses_p1;
extern bool                  g_frm_uses_p2;
extern bool                  g_frm_uses_p3;
extern bool                  g_frm_uses_p4;
extern bool                  g_frm_uses_p5;
extern double                g_fudge_limit;
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

void dStkMul();
void lStkMul();
void dStkAbs();
void mStkAbs();
void lStkAbs();
void dStkSqr();
void mStkSqr();
void lStkSqr();
void dStkConj();
void mStkConj();
void lStkConj();
void dStkZero();
void mStkZero();
void lStkZero();
void dStkOne();
void mStkOne();
void lStkOne();
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
int Formula();
int BadFormula();
int form_per_pixel();
int frm_get_param_stuff(char const *Name);
bool run_formula(const std::string &name, bool report_bad_sym);
bool fpFormulaSetup();
bool intFormulaSetup();
void init_misc();
void free_workarea();
