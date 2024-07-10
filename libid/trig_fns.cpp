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
// changing the order of these alters meaning of GIF extensions
// maximum 6 characters in function names or recheck all related code
trig_funct_lst g_trig_fn[] =
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

const int g_num_trig_functions{std::size(g_trig_fn)};

trig_fn g_trig_index[] =
{
    trig_fn::SIN, trig_fn::SQR, trig_fn::SINH, trig_fn::COSH
};
void (*g_ltrig0)(){lStkSin};
void (*g_ltrig1)(){lStkSqr};
void (*g_ltrig2)(){lStkSinh};
void (*g_ltrig3)(){lStkCosh};
void (*g_mtrig0)(){mStkSin};
void (*g_mtrig1)(){mStkSqr};
void (*g_mtrig2)(){mStkSinh};
void (*g_mtrig3)(){mStkCosh};
void (*g_dtrig0)(){dStkSin};
void (*g_dtrig1)(){dStkSqr};
void (*g_dtrig2)(){dStkSinh};
void (*g_dtrig3)(){dStkCosh};

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
        numfn = (+g_fractal_specific[+g_new_orbit_type].flags >> 6) & 7;
    }
    else
    {
        numfn = (+g_cur_fractal_specific->flags >> 6) & 7;
    }
    if (g_cur_fractal_specific == &g_fractal_specific[+fractal_type::FORMULA]
        || g_cur_fractal_specific == &g_fractal_specific[+fractal_type::FFORMULA])
    {
        numfn = g_max_function;
    }
    *buf = 0; // null string if none
    if (numfn > 0)
    {
        std::strcpy(buf, g_trig_fn[+g_trig_index[0]].name);
        int i = 0;
        while (++i < numfn)
        {
            std::snprintf(tmpbuf, std::size(tmpbuf), "/%s", g_trig_fn[+g_trig_index[i]].name);
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
        g_ltrig0 = g_trig_fn[+g_trig_index[0]].lfunct;
        g_mtrig0 = g_trig_fn[+g_trig_index[0]].mfunct;
        g_dtrig0 = g_trig_fn[+g_trig_index[0]].dfunct;
        break;
    case 1:
        g_ltrig1 = g_trig_fn[+g_trig_index[1]].lfunct;
        g_mtrig1 = g_trig_fn[+g_trig_index[1]].mfunct;
        g_dtrig1 = g_trig_fn[+g_trig_index[1]].dfunct;
        break;
    case 2:
        g_ltrig2 = g_trig_fn[+g_trig_index[2]].lfunct;
        g_mtrig2 = g_trig_fn[+g_trig_index[2]].mfunct;
        g_dtrig2 = g_trig_fn[+g_trig_index[2]].dfunct;
        break;
    case 3:
        g_ltrig3 = g_trig_fn[+g_trig_index[3]].lfunct;
        g_mtrig3 = g_trig_fn[+g_trig_index[3]].mfunct;
        g_dtrig3 = g_trig_fn[+g_trig_index[3]].dfunct;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
        {
            set_trig_pointers(i);
        }
        break;
    }
}
