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

extern void (*ltrig0)();
extern void (*ltrig1)();
extern void (*ltrig2)();
extern void (*ltrig3)();
extern void (*dtrig0)();
extern void (*dtrig1)();
extern void (*dtrig2)();
extern void (*dtrig3)();
extern void (*mtrig0)();
extern void (*mtrig1)();
extern void (*mtrig2)();
extern void (*mtrig3)();

std::string showtrig();
void trigdetails(char *buf);
int set_trig_array(int k, char const *name);
void set_trig_pointers(int);
