// SPDX-License-Identifier: GPL-3.0-only
//
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

// The index into this array must correspond to enum TrigFn
// changing the order of these alters meaning of GIF extensions
// maximum 6 characters in function names or recheck all related code
NamedTrigFunction g_trig_fn[] =
// clang-format off
{
    {"sin",     d_stk_sin,      d_stk_sin,      d_stk_sin   },
    {"cosxx",   d_stk_coxx,     d_stk_coxx,     d_stk_coxx  },
    {"sinh",    d_stk_sinh,     d_stk_sinh,     d_stk_sinh  },
    {"cosh",    d_stk_cosh,     d_stk_cosh,     d_stk_cosh  },
    {"exp",     d_stk_exp,      d_stk_exp,      d_stk_exp   },
    {"log",     d_stk_log,      d_stk_log,      d_stk_log   },
    {"sqr",     d_stk_sqr,      d_stk_sqr,      d_stk_sqr   },
    {"recip",   d_stk_recip,    d_stk_recip,    d_stk_recip },
    {"ident",   stk_ident,      stk_ident,      stk_ident   },
    {"cos",     d_stk_cos,      d_stk_cos,      d_stk_cos   },
    {"tan",     d_stk_tan,      d_stk_tan,      d_stk_tan   },
    {"tanh",    d_stk_tanh,     d_stk_tanh,     d_stk_tanh  },
    {"cotan",   d_stk_cotan,    d_stk_cotan,    d_stk_cotan },
    {"cotanh",  d_stk_cotanh,   d_stk_cotanh,   d_stk_cotanh},
    {"flip",    d_stk_flip,     d_stk_flip,     d_stk_flip  },
    {"conj",    d_stk_conj,     d_stk_conj,     d_stk_conj  },
    {"zero",    d_stk_zero,     d_stk_zero,     d_stk_zero  },
    {"asin",    d_stk_asin,     d_stk_asin,     d_stk_asin  },
    {"asinh",   d_stk_asinh,    d_stk_asinh,    d_stk_asinh },
    {"acos",    d_stk_acos,     d_stk_acos,     d_stk_acos  },
    {"acosh",   d_stk_acosh,    d_stk_acosh,    d_stk_acosh },
    {"atan",    d_stk_atan,     d_stk_atan,     d_stk_atan  },
    {"atanh",   d_stk_atanh,    d_stk_atanh,    d_stk_atanh },
    {"cabs",    d_stk_cabs,     d_stk_cabs,     d_stk_cabs  },
    {"abs",     d_stk_abs,      d_stk_abs,      d_stk_abs   },
    {"sqrt",    d_stk_sqrt,     d_stk_sqrt,     d_stk_sqrt  },
    {"floor",   d_stk_floor,    d_stk_floor,    d_stk_floor },
    {"ceil",    d_stk_ceil,     d_stk_ceil,     d_stk_ceil  },
    {"trunc",   d_stk_trunc,    d_stk_trunc,    d_stk_trunc },
    {"round",   d_stk_round,    d_stk_round,    d_stk_round },
    {"one",     d_stk_one,      d_stk_one,      d_stk_one   },
};
// clang-format on

const int g_num_trig_functions{std::size(g_trig_fn)};

TrigFn g_trig_index[] =
{
    TrigFn::SIN, TrigFn::SQR, TrigFn::SINH, TrigFn::COSH
};
void (*g_l_trig0)(){l_stk_sin};
void (*g_l_trig1)(){l_stk_sqr};
void (*g_l_trig2)(){l_stk_sinh};
void (*g_l_trig3)(){l_stk_cosh};
void (*g_m_trig0)(){m_stk_sin};
void (*g_m_trig1)(){m_stk_sqr};
void (*g_m_trig2)(){m_stk_sinh};
void (*g_m_trig3)(){m_stk_cosh};
void (*g_d_trig0)(){d_stk_sin};
void (*g_d_trig1)(){d_stk_sqr};
void (*g_d_trig2)(){d_stk_sinh};
void (*g_d_trig3)(){d_stk_cosh};

// return display form of active trig functions
std::string show_trig()
{
    char tmpbuf[30];
    trig_details(tmpbuf);
    if (tmpbuf[0])
    {
        return std::string{" function="} + tmpbuf;
    }
    return {};
}

void trig_details(char *buf)
{
    int numfn;
    char tmpbuf[20];
    if (g_fractal_type == FractalType::JULIBROT || g_fractal_type == FractalType::JULIBROTFP)
    {
        numfn = (+g_fractal_specific[+g_new_orbit_type].flags >> 6) & 7;
    }
    else
    {
        numfn = (+g_cur_fractal_specific->flags >> 6) & 7;
    }
    if (g_cur_fractal_specific == &g_fractal_specific[+FractalType::FORMULA]
        || g_cur_fractal_specific == &g_fractal_specific[+FractalType::FFORMULA])
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
            g_trig_index[k] = static_cast<TrigFn>(i);
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
        g_l_trig0 = g_trig_fn[+g_trig_index[0]].lfunct;
        g_m_trig0 = g_trig_fn[+g_trig_index[0]].mfunct;
        g_d_trig0 = g_trig_fn[+g_trig_index[0]].dfunct;
        break;
    case 1:
        g_l_trig1 = g_trig_fn[+g_trig_index[1]].lfunct;
        g_m_trig1 = g_trig_fn[+g_trig_index[1]].mfunct;
        g_d_trig1 = g_trig_fn[+g_trig_index[1]].dfunct;
        break;
    case 2:
        g_l_trig2 = g_trig_fn[+g_trig_index[2]].lfunct;
        g_m_trig2 = g_trig_fn[+g_trig_index[2]].mfunct;
        g_d_trig2 = g_trig_fn[+g_trig_index[2]].dfunct;
        break;
    case 3:
        g_l_trig3 = g_trig_fn[+g_trig_index[3]].lfunct;
        g_m_trig3 = g_trig_fn[+g_trig_index[3]].mfunct;
        g_d_trig3 = g_trig_fn[+g_trig_index[3]].dfunct;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
        {
            set_trig_pointers(i);
        }
        break;
    }
}
