#include "trig_fns.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "fractalp.h"
#include "jb.h"
#include "parser.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

// The index into this array must correspond to enum trig_fn
trig_funct_lst g_trig_fn[] =
// changing the order of these alters meaning of *.fra file
// maximum 6 characters in function names or recheck all related code
// clang-format off
{
    {"sin",   dStkSin,   dStkSin,   dStkSin   },
    {"cosxx", dStkCosXX, dStkCosXX, dStkCosXX },
    {"sinh",  dStkSinh,  dStkSinh,  dStkSinh  },
    {"cosh",  dStkCosh,  dStkCosh,  dStkCosh  },
    {"exp",   dStkExp,   dStkExp,   dStkExp   },
    {"log",   dStkLog,   dStkLog,   dStkLog   },
    {"sqr",   dStkSqr,   dStkSqr,   dStkSqr   },
    {"recip", dStkRecip, dStkRecip, dStkRecip }, // from recip on new in v16
    {"ident", StkIdent,  StkIdent,  StkIdent  },
    {"cos",   dStkCos,   dStkCos,   dStkCos   },
    {"tan",   dStkTan,   dStkTan,   dStkTan   },
    {"tanh",  dStkTanh,  dStkTanh,  dStkTanh  },
    {"cotan", dStkCoTan, dStkCoTan, dStkCoTan },
    {"cotanh", dStkCoTanh, dStkCoTanh, dStkCoTanh},
    {"flip",  dStkFlip,  dStkFlip,  dStkFlip  },
    {"conj",  dStkConj,  dStkConj,  dStkConj  },
    {"zero",  dStkZero,  dStkZero,  dStkZero  },
    {"asin",  dStkASin,  dStkASin,  dStkASin  },
    {"asinh", dStkASinh, dStkASinh, dStkASinh },
    {"acos",  dStkACos,  dStkACos,  dStkACos  },
    {"acosh", dStkACosh, dStkACosh, dStkACosh },
    {"atan",  dStkATan,  dStkATan,  dStkATan  },
    {"atanh", dStkATanh, dStkATanh, dStkATanh },
    {"cabs",  dStkCAbs,  dStkCAbs,  dStkCAbs  },
    {"abs",   dStkAbs,   dStkAbs,   dStkAbs   },
    {"sqrt",  dStkSqrt,  dStkSqrt,  dStkSqrt  },
    {"floor", dStkFloor, dStkFloor, dStkFloor },
    {"ceil",  dStkCeil,  dStkCeil,  dStkCeil  },
    {"trunc", dStkTrunc, dStkTrunc, dStkTrunc },
    {"round", dStkRound, dStkRound, dStkRound },
    {"one",   dStkOne,   dStkOne,   dStkOne   },
};
// clang-format on

#define NUMTRIGFN  sizeof(g_trig_fn)/sizeof(trig_funct_lst)

const int g_num_trig_functions = NUMTRIGFN;

trig_fn g_trig_index[] =
{
    trig_fn::SIN, trig_fn::SQR, trig_fn::SINH, trig_fn::COSH
};
void (*ltrig0)() = lStkSin;
void (*ltrig1)() = lStkSqr;
void (*ltrig2)() = lStkSinh;
void (*ltrig3)() = lStkCosh;
void (*mtrig0)() = mStkSin;
void (*mtrig1)() = mStkSqr;
void (*mtrig2)() = mStkSinh;
void (*mtrig3)() = mStkCosh;
void (*dtrig0)() = dStkSin;
void (*dtrig1)() = dStkSqr;
void (*dtrig2)() = dStkSinh;
void (*dtrig3)() = dStkCosh;

// return display form of active trig functions
std::string showtrig()
{
    char tmpbuf[30];
    trigdetails(tmpbuf);
    if (tmpbuf[0])
    {
        return std::string{" function="} + tmpbuf;
    }
    return {};
}

void trigdetails(char *buf)
{
    int numfn;
    char tmpbuf[20];
    if (g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
    {
        numfn = (g_fractal_specific[+g_new_orbit_type].flags >> 6) & 7;
    }
    else
    {
        numfn = (g_cur_fractal_specific->flags >> 6) & 7;
    }
    if (g_cur_fractal_specific == &g_fractal_specific[+fractal_type::FORMULA]
        || g_cur_fractal_specific == &g_fractal_specific[+fractal_type::FFORMULA])
    {
        numfn = g_max_function;
    }
    *buf = 0; // null string if none
    if (numfn > 0)
    {
        std::strcpy(buf, g_trig_fn[static_cast<int>(g_trig_index[0])].name);
        int i = 0;
        while (++i < numfn)
        {
            std::snprintf(tmpbuf, std::size(tmpbuf), "/%s", g_trig_fn[static_cast<int>(g_trig_index[i])].name);
            std::strcat(buf, tmpbuf);
        }
    }
}

// set array of trig function indices according to "function=" command
int set_trig_array(int k, char const *name)
{
    char trigname[10];
    char *slash;
    std::strncpy(trigname, name, 6);
    trigname[6] = 0; // safety first

    slash = std::strchr(trigname, '/');
    if (slash != nullptr)
    {
        *slash = 0;
    }

    strlwr(trigname);

    for (int i = 0; i < g_num_trig_functions; i++)
    {
        if (std::strcmp(trigname, g_trig_fn[i].name) == 0)
        {
            g_trig_index[k] = static_cast<trig_fn>(i);
            set_trig_pointers(k);
            break;
        }
    }
    return 0;
}
void set_trig_pointers(int which)
{
    // set trig variable functions to avoid array lookup time
    switch (which)
    {
    case 0:
        ltrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].lfunct;
        mtrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].mfunct;
        dtrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].dfunct;
        break;
    case 1:
        ltrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].lfunct;
        mtrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].mfunct;
        dtrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].dfunct;
        break;
    case 2:
        ltrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].lfunct;
        mtrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].mfunct;
        dtrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].dfunct;
        break;
    case 3:
        ltrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].lfunct;
        mtrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].mfunct;
        dtrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].dfunct;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
        {
            set_trig_pointers(i);
        }
        break;
    }
}
