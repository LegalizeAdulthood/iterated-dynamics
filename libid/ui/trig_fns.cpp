// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/trig_fns.h"

#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "fractals/parser.h"
#include "ui/cmdfiles.h"

#include <config/strlwr.h>

#include <cstdio>
#include <cstring>

// The index into this array must correspond to enum TrigFn
// changing the order of these alters meaning of GIF extensions
// maximum 6 characters in function names or recheck all related code
NamedTrigFunction g_trig_fn[] =
// clang-format off
{
    {"sin",     d_stk_sin,      d_stk_sin,      d_stk_sin   },
    {"cosxx",   d_stk_cosxx,     d_stk_cosxx,     d_stk_cosxx  },
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
    char tmp_buff[30];
    trig_details(tmp_buff);
    if (tmp_buff[0])
    {
        return std::string{" function="} + tmp_buff;
    }
    return {};
}

void trig_details(char *buf)
{
    int num_fn;
    if (g_fractal_type == FractalType::JULIBROT || g_fractal_type == FractalType::JULIBROT_FP)
    {
        num_fn = (+g_fractal_specific[+g_new_orbit_type].flags >> 6) & 7;
    }
    else
    {
        num_fn = (+g_cur_fractal_specific->flags >> 6) & 7;
    }
    if (g_cur_fractal_specific == &g_fractal_specific[+FractalType::FORMULA]
        || g_cur_fractal_specific == &g_fractal_specific[+FractalType::FORMULA_FP])
    {
        num_fn = g_max_function;
    }
    *buf = 0; // null string if none
    if (num_fn > 0)
    {
        std::strcpy(buf, g_trig_fn[+g_trig_index[0]].name);
        int i = 0;
        while (++i < num_fn)
        {
            char tmp_buff[20];
            std::snprintf(tmp_buff, std::size(tmp_buff), "/%s", g_trig_fn[+g_trig_index[i]].name);
            std::strcat(buf, tmp_buff);
        }
    }
}

// set array of trig function indices according to "function=" command
int set_trig_array(int k, char const *name)
{
    char trig_name[10];
    std::strncpy(trig_name, name, 6);
    trig_name[6] = 0; // safety first

    char *slash = std::strchr(trig_name, '/');
    if (slash != nullptr)
    {
        *slash = 0;
    }

    strlwr(trig_name);

    for (int i = 0; i < g_num_trig_functions; i++)
    {
        if (std::strcmp(trig_name, g_trig_fn[i].name) == 0)
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
        g_l_trig0 = g_trig_fn[+g_trig_index[0]].l_fn;
        g_m_trig0 = g_trig_fn[+g_trig_index[0]].m_fn;
        g_d_trig0 = g_trig_fn[+g_trig_index[0]].d_fn;
        break;
    case 1:
        g_l_trig1 = g_trig_fn[+g_trig_index[1]].l_fn;
        g_m_trig1 = g_trig_fn[+g_trig_index[1]].m_fn;
        g_d_trig1 = g_trig_fn[+g_trig_index[1]].d_fn;
        break;
    case 2:
        g_l_trig2 = g_trig_fn[+g_trig_index[2]].l_fn;
        g_m_trig2 = g_trig_fn[+g_trig_index[2]].m_fn;
        g_d_trig2 = g_trig_fn[+g_trig_index[2]].d_fn;
        break;
    case 3:
        g_l_trig3 = g_trig_fn[+g_trig_index[3]].l_fn;
        g_m_trig3 = g_trig_fn[+g_trig_index[3]].m_fn;
        g_d_trig3 = g_trig_fn[+g_trig_index[3]].d_fn;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
        {
            set_trig_pointers(i);
        }
        break;
    }
}
