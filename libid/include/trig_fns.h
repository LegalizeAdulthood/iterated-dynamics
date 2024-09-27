// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// The values must match the values in the trigfn array in prompts1
enum class trig_fn
{
    SIN = 0,
    COSXX,
    SINH,
    COSH,
    EXP,
    LOG,
    SQR,
    RECIP,
    IDENT,
    COS,
    TAN,
    TANH,
    COTAN,
    COTANH,
    FLIP,
    CONJ,
    ZERO,
    ASIN,
    ASINH,
    ACOS,
    ACOSH,
    FN_ATAN,
    ATANH,
    CABS,
    ABS,
    SQRT,
    FLOOR,
    CEIL,
    TRUNC,
    ROUND,
    ONE
};

inline int operator+(trig_fn value)
{
    return static_cast<int>(value);
}

struct trig_funct_lst
{
    char const *name;
    void (*lfunct)();
    void (*dfunct)();
    void (*mfunct)();
};

extern trig_funct_lst        g_trig_fn[];
extern trig_fn               g_trig_index[];
extern const int             g_num_trig_functions;

extern void (*g_ltrig0)();
extern void (*g_ltrig1)();
extern void (*g_ltrig2)();
extern void (*g_ltrig3)();
extern void (*g_dtrig0)();
extern void (*g_dtrig1)();
extern void (*g_dtrig2)();
extern void (*g_dtrig3)();
extern void (*g_mtrig0)();
extern void (*g_mtrig1)();
extern void (*g_mtrig2)();
extern void (*g_mtrig3)();

std::string showtrig();
void trigdetails(char *buf);
int set_trig_array(int k, char const *name);
void set_trig_pointers(int);
