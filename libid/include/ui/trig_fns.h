// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// The values must match the values in the trigfn array in prompts1
enum class TrigFn
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

inline int operator+(TrigFn value)
{
    return static_cast<int>(value);
}

struct NamedTrigFunction
{
    char const *name;
    void (*l_fn)();
    void (*d_fn)();
    void (*m_fn)();
};

extern NamedTrigFunction     g_trig_fn[];
extern TrigFn                g_trig_index[];
extern const int             g_num_trig_functions;
extern void                (*g_l_trig0)();
extern void                (*g_l_trig1)();
extern void                (*g_l_trig2)();
extern void                (*g_l_trig3)();
extern void                (*g_d_trig0)();
extern void                (*g_d_trig1)();
extern void                (*g_d_trig2)();
extern void                (*g_d_trig3)();
extern void                (*g_m_trig0)();
extern void                (*g_m_trig1)();
extern void                (*g_m_trig2)();
extern void                (*g_m_trig3)();

std::string show_trig();
void trig_details(char *buf);
int set_trig_array(int k, char const *name);
void set_trig_pointers(int which);
