/*
        Various routines that prompt for things.
*/
#include "port.h"
#include "prototyp.h"

#include "prompts1.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fracsuba.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "full_screen_prompt.h"
#include "get_key_no_help.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "jb.h"
#include "load_entry_text.h"
#include "loadfile.h"
#include "lorenz.h"
#include "lsys_fns.h"
#include "merge_path_names.h"
#include "miscres.h"
#include "os.h"
#include "parser.h"
#include "prompts2.h"
#include "realdos.h"
#include "set_bailout_formula.h"
#include "zoom.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

static  fractal_type select_fracttype(fractal_type t);
static  int sel_fractype_help(int curkey, int choice);
static bool select_type_params(fractal_type newfractype, fractal_type oldfractype);
static void set_default_parms();
static long gfe_choose_entry(int type, char const *title, char const *filename, char *entryname);
static  int check_gfe_key(int curkey, int choice);
static  void format_parmfile_line(int choice, char *buf);

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS     2
#define GETPARM    3

static char ifsmask[13]     = {"*.ifs"};
static char formmask[13]    = {"*.frm"};
static char lsysmask[13]    = {"*.l"};
bool g_julibrot = false;                  // flag for julibrot

// ---------------------------------------------------------------------

int get_fracttype()             // prompt for and select fractal type
{
    fractal_type t;
    int done = -1;
    fractal_type oldfractype = g_fractal_type;
    while (true)
    {
        t = select_fracttype(g_fractal_type);
        if (t == fractal_type::NOFRACTAL)
        {
            break;
        }
        bool i = select_type_params(t, g_fractal_type);
        if (!i)
        {
            // ok, all done
            done = 0;
            break;
        }
        if (i)   // can't return to prior image anymore
        {
            done = 1;
        }
    }
    if (done < 0)
    {
        g_fractal_type = oldfractype;
    }
    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
    return done;
}

struct FT_CHOICE
{
    char name[15];
    int  num;
};
static FT_CHOICE **ft_choices; // for sel_fractype_help subrtn

// subrtn of get_fracttype, separated so that storage gets freed up
static fractal_type select_fracttype(fractal_type t)
{
    int numtypes;
#define MAXFTYPES 200
    char tname[40];
    FT_CHOICE storage[MAXFTYPES] = { 0 };
    FT_CHOICE *choices[MAXFTYPES];
    int attributes[MAXFTYPES];

    // steal existing array for "choices"
    choices[0] = &storage[0];
    attributes[0] = 1;
    for (int i = 1; i < MAXFTYPES; ++i)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }
    ft_choices = &choices[0];

    // setup context sensitive help
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPFRACTALS;
    if (t == fractal_type::IFS3D)
    {
        t = fractal_type::IFS;
    }
    {
        int i = -1;
        int j = -1;
        while (g_fractal_specific[++i].name)
        {
            if (g_julibrot)
            {
                if (!((g_fractal_specific[i].flags & OKJB) && *g_fractal_specific[i].name != '*'))
                {
                    continue;
                }
            }
            if (g_fractal_specific[i].name[0] == '*')
            {
                continue;
            }
            std::strcpy(choices[++j]->name, g_fractal_specific[i].name);
            choices[j]->name[14] = 0; // safety
            choices[j]->num = i;      // remember where the real item is
        }
        numtypes = j + 1;
    }
    shell_sort(&choices, numtypes, sizeof(FT_CHOICE *), lccompare); // sort list
    int j = 0;
    for (int i = 0; i < numtypes; ++i)   // find starting choice in sorted list
    {
        if (choices[i]->num == static_cast<int>(t)
            || choices[i]->num == static_cast<int>(g_fractal_specific[static_cast<int>(t)].tofloat))
        {
            j = i;
        }
    }

    tname[0] = 0;
    int done = fullscreen_choice(CHOICE_HELP | CHOICE_INSTRUCTIONS,
            g_julibrot ? "Select Orbit Algorithm for Julibrot" : "Select a Fractal Type",
            nullptr, "Press " FK_F2 " for a description of the highlighted type", numtypes,
            (char const **)choices, attributes, 0, 0, 0, j, nullptr, tname, nullptr, sel_fractype_help);
    fractal_type result = fractal_type::NOFRACTAL;
    if (done >= 0)
    {
        result = static_cast<fractal_type>(choices[done]->num);
        if ((result == fractal_type::FORMULA || result == fractal_type::FFORMULA)
            && g_formula_filename == g_command_file)
        {
            g_formula_filename = g_search_for.frm;
        }
        if (result == fractal_type::LSYSTEM
            && g_l_system_filename == g_command_file)
        {
            g_l_system_filename = g_search_for.lsys;
        }
        if ((result == fractal_type::IFS || result == fractal_type::IFS3D)
            && g_ifs_filename == g_command_file)
        {
            g_ifs_filename = g_search_for.ifs;
        }
    }


    g_help_mode = old_help_mode;
    return result;
}

static int sel_fractype_help(int curkey, int choice)
{
    if (curkey == FIK_F2)
    {
        help_labels const old_help_mode = g_help_mode;
        g_help_mode = g_fractal_specific[(*(ft_choices+choice))->num].helptext;
        help(0);
        g_help_mode = old_help_mode;
    }
    return 0;
}

static bool select_type_params( // prompt for new fractal type parameters
    fractal_type newfractype,        // new fractal type
    fractal_type oldfractype         // previous fractal type
)
{
    bool ret;

    help_labels const old_help_mode = g_help_mode;

sel_type_restart:
    ret = false;
    g_fractal_type = newfractype;
    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];

    if (g_fractal_type == fractal_type::LSYSTEM)
    {
        g_help_mode = help_labels::HT_LSYS;
        if (get_file_entry(GETLSYS, "L-System", lsysmask, g_l_system_filename, g_l_system_name) < 0)
        {
            ret = true;
            goto sel_type_exit;
        }
    }
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        g_help_mode = help_labels::HT_FORMULA;
        if (get_file_entry(GETFORMULA, "Formula", formmask, g_formula_filename, g_formula_name) < 0)
        {
            ret = true;
            goto sel_type_exit;
        }
    }
    if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
    {
        g_help_mode = help_labels::HT_IFS;
        if (get_file_entry(GETIFS, "IFS", ifsmask, g_ifs_filename, g_ifs_name) < 0)
        {
            ret = true;
            goto sel_type_exit;
        }
    }

    if (((g_fractal_type == fractal_type::BIFURCATION) || (g_fractal_type == fractal_type::LBIFURCATION))
        && !((oldfractype == fractal_type::BIFURCATION) || (oldfractype == fractal_type::LBIFURCATION)))
    {
        set_trig_array(0, "ident");
    }
    if (((g_fractal_type == fractal_type::BIFSTEWART) || (g_fractal_type == fractal_type::LBIFSTEWART))
        && !((oldfractype == fractal_type::BIFSTEWART) || (oldfractype == fractal_type::LBIFSTEWART)))
    {
        set_trig_array(0, "ident");
    }
    if (((g_fractal_type == fractal_type::BIFLAMBDA) || (g_fractal_type == fractal_type::LBIFLAMBDA))
        && !((oldfractype == fractal_type::BIFLAMBDA) || (oldfractype == fractal_type::LBIFLAMBDA)))
    {
        set_trig_array(0, "ident");
    }
    if (((g_fractal_type == fractal_type::BIFEQSINPI) || (g_fractal_type == fractal_type::LBIFEQSINPI))
        && !((oldfractype == fractal_type::BIFEQSINPI) || (oldfractype == fractal_type::LBIFEQSINPI)))
    {
        set_trig_array(0, "sin");
    }
    if (((g_fractal_type == fractal_type::BIFADSINPI) || (g_fractal_type == fractal_type::LBIFADSINPI))
        && !((oldfractype == fractal_type::BIFADSINPI) || (oldfractype == fractal_type::LBIFADSINPI)))
    {
        set_trig_array(0, "sin");
    }

    /*
     * Next assumes that user going between popcorn and popcornjul
     * might not want to change function variables
     */
    if (((g_fractal_type == fractal_type::FPPOPCORN)
            || (g_fractal_type == fractal_type::LPOPCORN)
            || (g_fractal_type == fractal_type::FPPOPCORNJUL)
            || (g_fractal_type == fractal_type::LPOPCORNJUL))
        && !((oldfractype == fractal_type::FPPOPCORN)
            || (oldfractype == fractal_type::LPOPCORN)
            || (oldfractype == fractal_type::FPPOPCORNJUL)
            || (oldfractype == fractal_type::LPOPCORNJUL)))
    {
        set_function_parm_defaults();
    }

    // set LATOO function defaults
    if (g_fractal_type == fractal_type::LATOO && oldfractype != fractal_type::LATOO)
    {
        set_function_parm_defaults();
    }
    set_default_parms();

    if (get_fract_params(0) < 0)
    {
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA
            || g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D
            || g_fractal_type == fractal_type::LSYSTEM)
        {
            goto sel_type_restart;
        }
        else
        {
            ret = true;
        }
    }
    else
    {
        if (newfractype != oldfractype)
        {
            g_invert = 0;
            g_inversion[2] = 0;
            g_inversion[1] = g_inversion[2];
            g_inversion[0] = g_inversion[1];
        }
    }

sel_type_exit:
    g_help_mode = old_help_mode;
    return ret;
}

static void set_default_parms()
{
    g_x_min = g_cur_fractal_specific->xmin;
    g_x_max = g_cur_fractal_specific->xmax;
    g_y_min = g_cur_fractal_specific->ymin;
    g_y_max = g_cur_fractal_specific->ymax;
    g_x_3rd = g_x_min;
    g_y_3rd = g_y_min;

    if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
    {
        aspectratio_crop(g_screen_aspect, g_final_aspect_ratio);
    }
    for (int i = 0; i < 4; i++)
    {
        g_params[i] = g_cur_fractal_specific->paramvalue[i];
        if (g_fractal_type != fractal_type::CELLULAR
            && g_fractal_type != fractal_type::FROTH
            && g_fractal_type != fractal_type::FROTHFP
            && g_fractal_type != fractal_type::ANT)
        {
            roundfloatd(&g_params[i]); // don't round cellular, frothybasin or ant
        }
    }
    int extra = find_extra_param(g_fractal_type);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS-4; i++)
        {
            g_params[i+4] = g_more_fractal_params[extra].paramvalue[i];
        }
    }
    if (g_debug_flag != debug_flags::force_arbitrary_precision_math)
    {
        bf_math = bf_math_type::NONE;
    }
    else if (bf_math != bf_math_type::NONE)
    {
        fractal_floattobf();
    }
}

#define MAXFRACTALS 25

static int build_fractal_list(int fractals[], int *last_val, char const *nameptr[])
{
    int numfractals = 0;
    for (int i = 0; i < g_num_fractal_types; i++)
    {
        if ((g_fractal_specific[i].flags & OKJB) && *g_fractal_specific[i].name != '*')
        {
            fractals[numfractals] = i;
            if (i == static_cast<int>(g_new_orbit_type)
                    || i == static_cast<int>(g_fractal_specific[static_cast<int>(g_new_orbit_type)].tofloat))
            {
                *last_val = numfractals;
            }
            nameptr[numfractals] = g_fractal_specific[i].name;
            numfractals++;
            if (numfractals >= MAXFRACTALS)
            {
                break;
            }
        }
    }
    return numfractals;
}

std::string const g_julibrot_3d_options[] =
{
    "monocular", "lefteye", "righteye", "red-blue"
};

// JIIM
#ifdef RANDOM_RUN
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk, Random Run?";
char const *JIIMmethod[] =
{
    "breadth", "depth", "walk", "run"
};
#else
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk";
std::string const g_jiim_method[] =
{
    "breadth", "depth", "walk"
};
#endif
static char JIIMstr2[] = "Left first or Right first?";
std::string const g_jiim_left_right[] = {"left", "right"};

// The index into this array must correspond to enum trig_fn
trig_funct_lst g_trig_fn[] =
// changing the order of these alters meaning of *.fra file
// maximum 6 characters in function names or recheck all related code
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

#define NUMTRIGFN  sizeof(g_trig_fn)/sizeof(trig_funct_lst)

extern const int g_num_trig_functions = NUMTRIGFN;

static char tstack[4096] = { 0 };

namespace
{

char const *jiim_left_right_list[] =
{
    g_jiim_left_right[0].c_str(), g_jiim_left_right[1].c_str()
};

char const *jiim_method_list[] =
{
    g_jiim_method[0].c_str(), g_jiim_method[1].c_str(), g_jiim_method[2].c_str()
};

char const *julia_3d_options_list[] =
{
    g_julibrot_3d_options[0].c_str(),
    g_julibrot_3d_options[1].c_str(),
    g_julibrot_3d_options[2].c_str(),
    g_julibrot_3d_options[3].c_str()
};

}

// ---------------------------------------------------------------------
int get_fract_params(int caller)        // prompt for type-specific parms
{
    char const *v0 = "From cx (real part)";
    char const *v1 = "From cy (imaginary part)";
    char const *v2 = "To   cx (real part)";
    char const *v3 = "To   cy (imaginary part)";
    char const *juliorbitname = nullptr;
    int numparams, numtrig;
    fullscreenvalues paramvalues[30];
    char const *choices[30];
    long oldbailout = 0L;
    int promptnum;
    char msg[120];
    char const *type_name;
    char const *tmpptr;
    char bailoutmsg[50];
    int ret = 0;
    help_labels old_help_mode;
    char parmprompt[MAX_PARAMS][55];
    static char const *trg[] =
    {
        "First Function", "Second Function", "Third Function", "Fourth Function"
    };
    char *filename;
    char const *entryname;
    std::FILE *entryfile;
    char const *trignameptr[NUMTRIGFN];
    char const *bailnameptr[] = {"mod", "real", "imag", "or", "and", "manh", "manr"};
    fractalspecificstuff *jborbit = nullptr;
    int firstparm = 0;
    int lastparm  = MAX_PARAMS;
    double oldparam[MAX_PARAMS];
    int fkeymask = 0;

    oldbailout = g_bail_out;
    g_julibrot = g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP;
    fractal_type curtype = g_fractal_type;
    {
        int i;
        if (g_cur_fractal_specific->name[0] == '*'
            && (i = static_cast<int>(g_cur_fractal_specific->tofloat)) != static_cast<int>(fractal_type::NOFRACTAL)
            && g_fractal_specific[i].name[0] != '*')
        {
            curtype = static_cast<fractal_type>(i);
        }
    }
    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(curtype)];
    tstack[0] = 0;
    help_labels help_formula = g_cur_fractal_specific->helpformula;
    if (help_formula < help_labels::NONE)
    {
        bool use_filename_ref = false;
        std::string &filename_ref = g_formula_filename;
        if (help_formula == help_labels::SPECIAL_FORMULA)
        {
            // special for formula
            use_filename_ref = true;
            entryname = g_formula_name.c_str();
        }
        else if (help_formula == help_labels::SPECIAL_L_SYSTEM)
        {
            // special for lsystem
            use_filename_ref = true;
            filename_ref = g_l_system_filename;
            entryname = g_l_system_name.c_str();
        }
        else if (help_formula == help_labels::SPECIAL_IFS)
        {
            // special for ifs
            use_filename_ref = true;
            filename_ref = g_ifs_filename;
            entryname = g_ifs_name.c_str();
        }
        else
        {
            // this shouldn't happen
            filename = nullptr;
            entryname = nullptr;
        }
        if ((!use_filename_ref && find_file_item(filename, entryname, &entryfile, -1-static_cast<int>(help_formula)) == 0)
            || (use_filename_ref && find_file_item(filename_ref, entryname, &entryfile, -1-static_cast<int>(help_formula)) == 0))
        {
            load_entry_text(entryfile, tstack, 17, 0, 0);
            std::fclose(entryfile);
            if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
            {
                frm_get_param_stuff(entryname); // no error check, should be okay, from above
            }
        }
    }
    else if (help_formula >= help_labels::IDHELP_INDEX)
    {
        int c, lines;
        read_help_topic(help_formula, 0, 2000, tstack); // need error handling here ??
        tstack[2000-static_cast<int>(help_formula)] = 0;
        int i = 0;
        lines = 0;
        int j = 0;
        int k = 1;
        while ((c = tstack[i++]) != 0)
        {
            // stop at ctl, blank, or line with col 1 nonblank, max 16 lines
            if (k && c == ' ' && ++k <= 5)
            {
            } // skip 4 blanks at start of line
            else
            {
                if (c == '\n')
                {
                    if (k)
                    {
                        break; // blank line
                    }
                    if (++lines >= 16)
                    {
                        break;
                    }
                    k = 1;
                }
                else if (c < 16)   // a special help format control char
                {
                    break;
                }
                else
                {
                    if (k == 1)   // line starts in column 1
                    {
                        break;
                    }
                    k = 0;
                }
                tstack[j++] = (char)c;
            }
        }
        while (--j >= 0 && tstack[j] == '\n')
        {
        }
        tstack[j+1] = 0;
    }
    fractalspecificstuff *savespecific = g_cur_fractal_specific;
    int orbit_bailout;

gfp_top:
    promptnum = 0;
    if (g_julibrot)
    {
        fractal_type i = select_fracttype(g_new_orbit_type);
        if (i == fractal_type::NOFRACTAL)
        {
            if (ret == 0)
            {
                ret = -1;
            }
            g_julibrot = false;
            goto gfp_exit;
        }
        else
        {
            g_new_orbit_type = i;
        }
        jborbit = &g_fractal_specific[static_cast<int>(g_new_orbit_type)];
        juliorbitname = jborbit->name;
    }

    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        if (g_frm_uses_p1)    // set first parameter
        {
            firstparm = 0;
        }
        else if (g_frm_uses_p2)
        {
            firstparm = 2;
        }
        else if (g_frm_uses_p3)
        {
            firstparm = 4;
        }
        else if (g_frm_uses_p4)
        {
            firstparm = 6;
        }
        else
        {
            firstparm = 8; // uses_p5 or no parameter
        }

        if (g_frm_uses_p5)    // set last parameter
        {
            lastparm = 10;
        }
        else if (g_frm_uses_p4)
        {
            lastparm = 8;
        }
        else if (g_frm_uses_p3)
        {
            lastparm = 6;
        }
        else if (g_frm_uses_p2)
        {
            lastparm = 4;
        }
        else
        {
            lastparm = 2; // uses_p1 or no parameter
        }
    }

    if (g_julibrot)
    {
        g_cur_fractal_specific = jborbit;
        firstparm = 2; // in most case Julibrot does not need first two parms
        if (g_new_orbit_type == fractal_type::QUATJULFP        // all parameters needed
            || g_new_orbit_type == fractal_type::HYPERCMPLXJFP)
        {
            firstparm = 0;
            lastparm = 4;
        }
        if (g_new_orbit_type == fractal_type::QUATFP           // no parameters needed
            || g_new_orbit_type == fractal_type::HYPERCMPLXFP)
        {
            firstparm = 4;
        }
    }
    numparams = 0;
    {
        int j = 0;
        for (int i = firstparm; i < lastparm; i++)
        {
            char tmpbuf[30];
            if (!typehasparm(g_julibrot ? g_new_orbit_type : g_fractal_type, i, parmprompt[j]))
            {
                if (curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA)
                {
                    if (paramnotused(i))
                    {
                        continue;
                    }
                }
                break;
            }
            numparams++;
            choices[promptnum] = parmprompt[j++];
            paramvalues[promptnum].type = 'd';

            if (choices[promptnum][0] == '+')
            {
                choices[promptnum]++;
                paramvalues[promptnum].type = 'D';
            }
            else if (choices[promptnum][0] == '#')
            {
                choices[promptnum]++;
            }
            std::sprintf(tmpbuf, "%.17g", g_params[i]);
            paramvalues[promptnum].uval.dval = atof(tmpbuf);
            oldparam[i] = paramvalues[promptnum++].uval.dval;
        }
    }

    /* The following is a goofy kludge to make reading in the formula
     * parameters work.
     */
    if (curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA)
    {
        numparams = lastparm - firstparm;
    }

    numtrig = (g_cur_fractal_specific->flags >> 6) & 7;
    if (curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA)
    {
        numtrig = g_max_function;
    }

    for (int i = NUMTRIGFN-1; i >= 0; --i)
    {
        trignameptr[i] = g_trig_fn[i].name;
    }
    for (int i = 0; i < numtrig; i++)
    {
        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.val  = static_cast<int>(g_trig_index[i]);
        paramvalues[promptnum].uval.ch.llen = NUMTRIGFN;
        paramvalues[promptnum].uval.ch.vlen = 6;
        paramvalues[promptnum].uval.ch.list = trignameptr;
        choices[promptnum++] = trg[i];
    }
    type_name = g_cur_fractal_specific->name;
    if (*type_name == '*')
    {
        ++type_name;
    }

    orbit_bailout = g_cur_fractal_specific->orbit_bailout;
    if (orbit_bailout != 0
        && g_cur_fractal_specific->calctype == standard_fractal
        && (g_cur_fractal_specific->flags & BAILTEST))
    {
        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.val  = static_cast<int>(g_bail_out_test);
        paramvalues[promptnum].uval.ch.llen = 7;
        paramvalues[promptnum].uval.ch.vlen = 6;
        paramvalues[promptnum].uval.ch.list = bailnameptr;
        choices[promptnum++] = "Bailout Test (mod, real, imag, or, and, manh, manr)";
    }

    if (orbit_bailout)
    {
        if (g_potential_params[0] != 0.0 && g_potential_params[2] != 0.0)
        {
            paramvalues[promptnum].type = '*';
            choices[promptnum++] = "Bailout: continuous potential (Y screen) value in use";
        }
        else
        {
            choices[promptnum] = "Bailout value (0 means use default)";
            paramvalues[promptnum].type = 'L';
            oldbailout = g_bail_out;
            paramvalues[promptnum++].uval.Lval = oldbailout;
            paramvalues[promptnum].type = '*';
            tmpptr = type_name;
            if (g_user_biomorph_value != -1)
            {
                orbit_bailout = 100;
                tmpptr = "biomorph";
            }
            std::sprintf(bailoutmsg, "    (%s default is %d)", tmpptr, orbit_bailout);
            choices[promptnum++] = bailoutmsg;
        }
    }
    if (g_julibrot)
    {
        switch (g_new_orbit_type)
        {
        case fractal_type::QUATFP:
        case fractal_type::HYPERCMPLXFP:
            v0 = "From cj (3rd dim)";
            v1 = "From ck (4th dim)";
            v2 = "To   cj (3rd dim)";
            v3 = "To   ck (4th dim)";
            break;
        case fractal_type::QUATJULFP:
        case fractal_type::HYPERCMPLXJFP:
            v0 = "From zj (3rd dim)";
            v1 = "From zk (4th dim)";
            v2 = "To   zj (3rd dim)";
            v3 = "To   zk (4th dim)";
            break;
        default:
            v0 = "From cx (real part)";
            v1 = "From cy (imaginary part)";
            v2 = "To   cx (real part)";
            v3 = "To   cy (imaginary part)";
            break;
        }

        g_cur_fractal_specific = savespecific;
        paramvalues[promptnum].uval.dval = g_julibrot_x_max;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = v0;
        paramvalues[promptnum].uval.dval = g_julibrot_y_max;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = v1;
        paramvalues[promptnum].uval.dval = g_julibrot_x_min;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = v2;
        paramvalues[promptnum].uval.dval = g_julibrot_y_min;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = v3;
        paramvalues[promptnum].uval.ival = g_julibrot_z_dots;
        paramvalues[promptnum].type = 'i';
        choices[promptnum++] = "Number of z pixels";

        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.val  = g_julibrot_3d_mode;
        paramvalues[promptnum].uval.ch.llen = 4;
        paramvalues[promptnum].uval.ch.vlen = 9;
        paramvalues[promptnum].uval.ch.list = julia_3d_options_list;
        choices[promptnum++] = "3D Mode";

        paramvalues[promptnum].uval.dval = g_eyes_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Distance between eyes";
        paramvalues[promptnum].uval.dval = g_julibrot_origin_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Location of z origin";
        paramvalues[promptnum].uval.dval = g_julibrot_depth_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Depth of z";
        paramvalues[promptnum].uval.dval = g_julibrot_height_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Screen height";
        paramvalues[promptnum].uval.dval = g_julibrot_width_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Screen width";
        paramvalues[promptnum].uval.dval = g_julibrot_dist_fp;
        paramvalues[promptnum].type = 'f';
        choices[promptnum++] = "Distance to Screen";
    }

    if (curtype == fractal_type::INVERSEJULIA || curtype == fractal_type::INVERSEJULIAFP)
    {
        choices[promptnum] = JIIMstr1;
        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.list = jiim_method_list;
        paramvalues[promptnum].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
        paramvalues[promptnum].uval.ch.llen = 4;
#else
        paramvalues[promptnum].uval.ch.llen = 3; // disable random run
#endif
        paramvalues[promptnum++].uval.ch.val  = static_cast<int>(g_major_method);

        choices[promptnum] = JIIMstr2;
        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.list = jiim_left_right_list;
        paramvalues[promptnum].uval.ch.vlen = 5;
        paramvalues[promptnum].uval.ch.llen = 2;
        paramvalues[promptnum++].uval.ch.val  = static_cast<int>(g_inverse_julia_minor_method);
    }

    if ((curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA) && g_frm_uses_ismand)
    {
        choices[promptnum] = "ismand";
        paramvalues[promptnum].type = 'y';
        paramvalues[promptnum++].uval.ch.val = g_is_mandelbrot ? 1 : 0;
    }

    if (caller && (g_display_3d > display_3d_modes::NONE))
    {
        stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Current type has no type-specific parameters");
        goto gfp_exit;
    }
    if (g_julibrot)
    {
        std::sprintf(msg, "Julibrot Parameters (orbit=%s)", juliorbitname);
    }
    else
    {
        std::sprintf(msg, "Parameters for fractal type %s", type_name);
    }
    if (bf_math == bf_math_type::NONE)
    {
        std::strcat(msg, "\n(Press " FK_F6 " for corner parameters)");
        fkeymask = 1U << 6;     // F6 exits
    }
    full_screen_reset_scrolling();
    while (true)
    {
        old_help_mode = g_help_mode;
        g_help_mode = g_cur_fractal_specific->helptext;
        int i = fullscreen_prompt(msg, promptnum, choices, paramvalues, fkeymask, tstack);
        g_help_mode = old_help_mode;
        if (i < 0)
        {
            if (g_julibrot)
            {
                goto gfp_top;
            }
            if (ret == 0)
            {
                ret = -1;
            }
            goto gfp_exit;
        }
        if (i != FIK_F6)
        {
            break;
        }
        if (bf_math == bf_math_type::NONE)
        {
            if (get_corners() > 0)
            {
                ret = 1;
            }
        }
    }
    promptnum = 0;
    for (int i = firstparm; i < numparams+firstparm; i++)
    {
        if (curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA)
        {
            if (paramnotused(i))
            {
                continue;
            }
        }
        if (oldparam[i] != paramvalues[promptnum].uval.dval)
        {
            g_params[i] = paramvalues[promptnum].uval.dval;
            ret = 1;
        }
        ++promptnum;
    }

    for (int i = 0; i < numtrig; i++)
    {
        if (paramvalues[promptnum].uval.ch.val != (int)g_trig_index[i])
        {
            set_trig_array(i, g_trig_fn[paramvalues[promptnum].uval.ch.val].name);
            ret = 1;
        }
        ++promptnum;
    }

    if (g_julibrot)
    {
        g_cur_fractal_specific = jborbit;
    }

    orbit_bailout = g_cur_fractal_specific->orbit_bailout;
    if (orbit_bailout != 0
        && g_cur_fractal_specific->calctype == standard_fractal
        && (g_cur_fractal_specific->flags & BAILTEST))
    {
        if (paramvalues[promptnum].uval.ch.val != static_cast<int>(g_bail_out_test))
        {
            g_bail_out_test = static_cast<bailouts>(paramvalues[promptnum].uval.ch.val);
            ret = 1;
        }
        promptnum++;
    }
    else
    {
        g_bail_out_test = bailouts::Mod;
    }
    setbailoutformula(g_bail_out_test);

    if (orbit_bailout)
    {
        if (g_potential_params[0] != 0.0 && g_potential_params[2] != 0.0)
        {
            promptnum++;
        }
        else
        {
            g_bail_out = paramvalues[promptnum++].uval.Lval;
            if (g_bail_out != 0 && (g_bail_out < 1 || g_bail_out > 2100000000L))
            {
                g_bail_out = oldbailout;
            }
            if (g_bail_out != oldbailout)
            {
                ret = 1;
            }
            promptnum++;
        }
    }

    if (g_julibrot)
    {
        g_julibrot_x_max    = paramvalues[promptnum++].uval.dval;
        g_julibrot_y_max    = paramvalues[promptnum++].uval.dval;
        g_julibrot_x_min    = paramvalues[promptnum++].uval.dval;
        g_julibrot_y_min    = paramvalues[promptnum++].uval.dval;
        g_julibrot_z_dots      = paramvalues[promptnum++].uval.ival;
        g_julibrot_3d_mode = paramvalues[promptnum++].uval.ch.val;
        g_eyes_fp     = (float)paramvalues[promptnum++].uval.dval;
        g_julibrot_origin_fp   = (float)paramvalues[promptnum++].uval.dval;
        g_julibrot_depth_fp    = (float)paramvalues[promptnum++].uval.dval;
        g_julibrot_height_fp   = (float)paramvalues[promptnum++].uval.dval;
        g_julibrot_width_fp    = (float)paramvalues[promptnum++].uval.dval;
        g_julibrot_dist_fp     = (float)paramvalues[promptnum++].uval.dval;
        ret = 1;  // force new calc since not resumable anyway
    }
    if (curtype == fractal_type::INVERSEJULIA || curtype == fractal_type::INVERSEJULIAFP)
    {
        if (paramvalues[promptnum].uval.ch.val != static_cast<int>(g_major_method)
            || paramvalues[promptnum+1].uval.ch.val != static_cast<int>(g_inverse_julia_minor_method))
        {
            ret = 1;
        }
        g_major_method = static_cast<Major>(paramvalues[promptnum++].uval.ch.val);
        g_inverse_julia_minor_method = static_cast<Minor>(paramvalues[promptnum++].uval.ch.val);
    }
    if ((curtype == fractal_type::FORMULA || curtype == fractal_type::FFORMULA) && g_frm_uses_ismand)
    {
        if (g_is_mandelbrot != (paramvalues[promptnum].uval.ch.val != 0))
        {
            g_is_mandelbrot = (paramvalues[promptnum].uval.ch.val != 0);
            ret = 1;
        }
        ++promptnum;
    }
gfp_exit:
    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
    return ret;
}

int find_extra_param(fractal_type type)
{
    int i, ret;
    fractal_type curtyp;
    ret = -1;
    i = -1;

    if (g_fractal_specific[static_cast<int>(type)].flags & MORE)
    {
        while ((curtyp = g_more_fractal_params[++i].type) != type && curtyp != fractal_type::NOFRACTAL);
        if (curtyp == type)
        {
            ret = i;
        }
    }
    return ret;
}

void load_params(fractal_type fractype)
{
    for (int i = 0; i < 4; ++i)
    {
        g_params[i] = g_fractal_specific[static_cast<int>(fractype)].paramvalue[i];
        if (fractype != fractal_type::CELLULAR && fractype != fractal_type::ANT)
        {
            roundfloatd(&g_params[i]); // don't round cellular or ant
        }
    }
    int extra = find_extra_param(fractype);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS-4; i++)
        {
            g_params[i+4] = g_more_fractal_params[extra].paramvalue[i];
        }
    }
}

bool check_orbit_name(char const *orbitname)
{
    int numtypes;
    char const *nameptr[MAXFRACTALS];
    int fractals[MAXFRACTALS];
    int last_val;

    numtypes = build_fractal_list(fractals, &last_val, nameptr);
    bool bad = true;
    for (int i = 0; i < numtypes; i++)
    {
        if (std::strcmp(orbitname, nameptr[i]) == 0)
        {
            g_new_orbit_type = static_cast<fractal_type>(fractals[i]);
            bad = false;
            break;
        }
    }
    return bad;
}

// ---------------------------------------------------------------------

static std::FILE *gfe_file;

long get_file_entry(int type, char const *title, char const *fmask,
                    char *filename, char *entryname)
{
    // Formula, LSystem, etc type structure, select from file
    // containing definitions in the form    name { ... }
    bool firsttry;
    long entry_pointer;
    bool newfile = false;
    while (true)
    {
        firsttry = false;
        // binary mode used here - it is more work, but much faster,
        //     especially when ftell or fgetpos is used
        while (newfile || (gfe_file = std::fopen(filename, "rb")) == nullptr)
        {
            char buf[60];
            newfile = false;
            if (firsttry)
            {
                stopmsg(STOPMSG_NONE, std::string{"Can't find "} + filename);
            }
            std::sprintf(buf, "Select %s File", title);
            if (getafilename(buf, fmask, filename))
            {
                return -1;
            }

            firsttry = true; // if around open loop again it is an error
        }
        setvbuf(gfe_file, tstack, _IOFBF, 4096); // improves speed when file is big
        newfile = false;
        entry_pointer = gfe_choose_entry(type, title, filename, entryname);
        if (entry_pointer == -2)
        {
            newfile = true; // go to file list,
            continue;    // back to getafilename
        }
        if (entry_pointer == -1)
        {
            return -1;
        }
        switch (type)
        {
        case GETFORMULA:
            if (!RunForm(entryname, true))
            {
                return 0;
            }
            break;
        case GETLSYS:
            if (LLoad() == 0)
            {
                return 0;
            }
            break;
        case GETIFS:
            if (ifsload() == 0)
            {
                g_fractal_type = !g_ifs_type ? fractal_type::IFS : fractal_type::IFS3D;
                g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
                set_default_parms(); // to correct them if 3d
                return 0;
            }
            break;
        case GETPARM:
            return entry_pointer;
        }
    }
}

long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, char *entryname)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    long const result = get_file_entry(type, title, fmask, buf, entryname);
    filename = buf;
    return result;
}

long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, std::string &entryname)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    char name_buf[ITEM_NAME_LEN];
    std::strncpy(name_buf, entryname.c_str(), ITEM_NAME_LEN);
    name_buf[ITEM_NAME_LEN - 1] = 0;
    long const result = get_file_entry(type, title, fmask, buf, name_buf);
    filename = buf;
    entryname = name_buf;
    return result;
}

struct entryinfo
{
    char name[ITEM_NAME_LEN+2];
    long point; // points to the ( or the { following the name
};
static entryinfo **gfe_choices; // for format_getparm_line
static char const *gfe_title;

// skip to next non-white space character and return it
int skip_white_space(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = getc(infile);
        (*file_offset)++;
    }
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return c;
}

// skip to end of line
int skip_comment(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = getc(infile);
        (*file_offset)++;
    }
    while (c != '\n' && c != '\r' && c != EOF && c != '\032');
    return c;
}

#define MAXENTRIES 2000L

int scan_entries(std::FILE *infile, entryinfo *choices, char const *itemname)
{
    /*
    function returns the number of entries found; if a
    specific entry is being looked for, returns -1 if
    the entry is found, 0 otherwise.
    */
    char buf[101];
    int exclude_entry;
    long name_offset, temp_offset;
    long file_offset = -1;
    int numentries = 0;

    while (true)
    {
        // scan the file for entry names
        int c, len;
top:
        c = skip_white_space(infile, &file_offset);
        if (c == ';')
        {
            c = skip_comment(infile, &file_offset);
            if (c == EOF || c == '\032')
            {
                break;
            }
            continue;
        }
        temp_offset = file_offset;
        name_offset = temp_offset;
        // next equiv roughly to fscanf(..,"%40[^* \n\r\t({\032]",buf)
        len = 0;
        // allow spaces in entry names in next
        while (c != ' ' && c != '\t' && c != '(' && c != ';'
            && c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032')
        {
            if (len < 40)
            {
                buf[len++] = (char) c;
            }
            c = getc(infile);
            ++file_offset;
            if (c == '\n' || c == '\r')
            {
                goto top;
            }
        }
        buf[len] = 0;
        while (c != '{' &&  c != EOF && c != '\032')
        {
            if (c == ';')
            {
                c = skip_comment(infile, &file_offset);
            }
            else
            {
                c = getc(infile);
                ++file_offset;
                if (c == '\n' || c == '\r')
                {
                    goto top;
                }
            }
        }
        if (c == '{')
        {
            while (c != '}' && c != EOF && c != '\032')
            {
                if (c == ';')
                {
                    c = skip_comment(infile, &file_offset);
                }
                else
                {
                    if (c == '\n' || c == '\r')       // reset temp_offset to
                    {
                        temp_offset = file_offset;  // beginning of new line
                    }
                    c = getc(infile);
                    ++file_offset;
                }
                if (c == '{') //second '{' found
                {
                    if (temp_offset == name_offset) //if on same line, skip line
                    {
                        skip_comment(infile, &file_offset);
                        goto top;
                    }
                    else
                    {
                        std::fseek(infile, temp_offset, SEEK_SET); //else, go back to
                        file_offset = temp_offset - 1;        //beginning of line
                        goto top;
                    }
                }
            }
            if (c != '}')     // i.e. is EOF or '\032'
            {
                break;
            }

            if (strnicmp(buf, "frm:", 4) == 0 ||
                    strnicmp(buf, "ifs:", 4) == 0 ||
                    strnicmp(buf, "par:", 4) == 0)
            {
                exclude_entry = 4;
            }
            else if (strnicmp(buf, "lsys:", 5) == 0)
            {
                exclude_entry = 5;
            }
            else
            {
                exclude_entry = 0;
            }

            buf[ITEM_NAME_LEN + exclude_entry] = 0;
            if (itemname != nullptr)  // looking for one entry
            {
                if (stricmp(buf, itemname) == 0)
                {
                    std::fseek(infile, name_offset + (long) exclude_entry, SEEK_SET);
                    return -1;
                }
            }
            else // make a whole list of entries
            {
                if (buf[0] != 0 && stricmp(buf, "comment") != 0 && !exclude_entry)
                {
                    std::strcpy(choices[numentries].name, buf);
                    choices[numentries].point = name_offset;
                    if (++numentries >= MAXENTRIES)
                    {
                        std::sprintf(buf, "Too many entries in file, first %ld used", MAXENTRIES);
                        stopmsg(STOPMSG_NONE, buf);
                        break;
                    }
                }
            }
        }
        else if (c == EOF || c == '\032')
        {
            break;
        }
    }
    return numentries;
}

// subrtn of get_file_entry, separated so that storage gets freed up
static long gfe_choose_entry(int type, char const *title, char const *filename, char *entryname)
{
#ifdef XFRACT
    char const *o_instr = "Press " FK_F6 " to select file, " FK_F2 " for details, " FK_F4 " to toggle sort ";
    // keep the above line length < 80 characters
#else
    char const *o_instr = "Press " FK_F6 " to select different file, " FK_F2 " for details, " FK_F4 " to toggle sort ";
#endif
    int numentries;
    char buf[101];
    entryinfo storage[MAXENTRIES + 1];
    entryinfo *choices[MAXENTRIES + 1] = { nullptr };
    int attributes[MAXENTRIES + 1] = { 0 };
    void (*formatitem)(int, char *);
    int boxwidth, boxdepth, colwidth;
    char instr[80];

    static bool dosort = true;

    gfe_choices = &choices[0];
    gfe_title = title;

retry:
    for (int i = 0; i < MAXENTRIES+1; i++)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }

    helptitle(); // to display a clue when file big and next is slow

    numentries = scan_entries(gfe_file, &storage[0], nullptr);
    if (numentries == 0)
    {
        stopmsg(STOPMSG_NONE, "File doesn't contain any valid entries");
        std::fclose(gfe_file);
        return -2; // back to file list
    }
    std::strcpy(instr, o_instr);
    if (dosort)
    {
        std::strcat(instr, "off");
        shell_sort((char *) &choices, numentries, sizeof(entryinfo *), lccompare);
    }
    else
    {
        std::strcat(instr, "on");
    }

    std::strcpy(buf, entryname); // preset to last choice made
    std::string const heading{std::string{title} + " Selection\n"
        + "File: " + filename};
    formatitem = nullptr;
    boxdepth = 0;
    colwidth = boxdepth;
    boxwidth = colwidth;
    if (type == GETPARM)
    {
        formatitem = format_parmfile_line;
        boxwidth = 1;
        boxdepth = 16;
        colwidth = 76;
    }

    int i = fullscreen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
        heading.c_str(), nullptr, instr, numentries, (char const **) choices,
        attributes, boxwidth, boxdepth, colwidth, 0,
        formatitem, buf, nullptr, check_gfe_key);
    if (i == -FIK_F4)
    {
        rewind(gfe_file);
        dosort = !dosort;
        goto retry;
    }
    std::fclose(gfe_file);
    if (i < 0)
    {
        // go back to file list or cancel
        return (i == -FIK_F6) ? -2 : -1;
    }
    std::strcpy(entryname, choices[i]->name);
    return choices[i]->point;
}


static int check_gfe_key(int curkey, int choice)
{
    char infhdg[60];
    char infbuf[25*80];
    char blanks[79];         // used to clear the entry portion of screen
    std::memset(blanks, ' ', 78);
    blanks[78] = (char) 0;

    if (curkey == FIK_F6)
    {
        return 0-FIK_F6;
    }
    if (curkey == FIK_F4)
    {
        return 0-FIK_F4;
    }
    if (curkey == FIK_F2)
    {
        int widest_entry_line = 0;
        int lines_in_entry = 0;
        bool comment = false;
        int c = 0;
        int widthct = 0;
        std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
        while ((c = fgetc(gfe_file)) != EOF && c != '\032')
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n')
            {
                comment = false;
                lines_in_entry++;
                widthct =  -1;
            }
            else if (c == '\t')
            {
                widthct += 7 - widthct % 8;
            }
            else if (c == '\r')
            {
                continue;
            }
            if (++widthct > widest_entry_line)
            {
                widest_entry_line = widthct;
            }
            if (c == '}' && !comment)
            {
                lines_in_entry++;
                break;
            }
        }
        bool in_scrolling_mode = false; // true if entry doesn't fit available space
        if (c == EOF || c == '\032')
        {
            // should never happen
            std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
            in_scrolling_mode = false;
        }
        std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
        load_entry_text(gfe_file, infbuf, 17, 0, 0);
        if (lines_in_entry > 17 || widest_entry_line > 74)
        {
            in_scrolling_mode = true;
        }
        std::strcpy(infhdg, gfe_title);
        std::strcat(infhdg, " file entry:\n\n");
        // ... instead, call help with buffer?  heading added
        driver_stack_screen();
        helptitle();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);

        g_text_cbase = 0;
        driver_put_string(2, 1, C_GENERAL_HI, infhdg);
        g_text_cbase = 2; // left margin is 2
        driver_put_string(4, 0, C_GENERAL_MED, infbuf);
        driver_put_string(-1, 0, C_GENERAL_LO,
            "\n"
            "\n"
            " Use " UPARR1 ", " DNARR1 ", " RTARR1 ", " LTARR1
                ", PgUp, PgDown, Home, and End to scroll text\n"
            "Any other key to return to selection list");

        int top_line = 0;
        int left_column = 0;
        bool done = false;
        bool rewrite_infbuf = false;  // if true: rewrite the entry portion of screen
        while (!done)
        {
            if (rewrite_infbuf)
            {
                rewrite_infbuf = false;
                std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
                load_entry_text(gfe_file, infbuf, 17, top_line, left_column);
                for (int i = 4; i < (lines_in_entry < 17 ? lines_in_entry + 4 : 21); i++)
                {
                    driver_put_string(i, 0, C_GENERAL_MED, blanks);
                }
                driver_put_string(4, 0, C_GENERAL_MED, infbuf);
            }
            int i = getakeynohelp();
            if (i == FIK_DOWN_ARROW        || i == FIK_CTL_DOWN_ARROW
                || i == FIK_UP_ARROW       || i == FIK_CTL_UP_ARROW
                || i == FIK_LEFT_ARROW     || i == FIK_CTL_LEFT_ARROW
                || i == FIK_RIGHT_ARROW    || i == FIK_CTL_RIGHT_ARROW
                || i == FIK_HOME           || i == FIK_CTL_HOME
                || i == FIK_END            || i == FIK_CTL_END
                || i == FIK_PAGE_UP        || i == FIK_CTL_PAGE_UP
                || i == FIK_PAGE_DOWN      || i == FIK_CTL_PAGE_DOWN)
            {
                switch (i)
                {
                case FIK_DOWN_ARROW:
                case FIK_CTL_DOWN_ARROW: // down one line
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line++;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_UP_ARROW:
                case FIK_CTL_UP_ARROW:  // up one line
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line--;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_LEFT_ARROW:
                case FIK_CTL_LEFT_ARROW:  // left one column
                    if (in_scrolling_mode && left_column > 0)
                    {
                        left_column--;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_RIGHT_ARROW:
                case FIK_CTL_RIGHT_ARROW: // right one column
                    if (in_scrolling_mode && std::strchr(infbuf, '\021') != nullptr)
                    {
                        left_column++;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_PAGE_DOWN:
                case FIK_CTL_PAGE_DOWN: // down 17 lines
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line += 17;
                        if (top_line > lines_in_entry - 17)
                        {
                            top_line = lines_in_entry - 17;
                        }
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_PAGE_UP:
                case FIK_CTL_PAGE_UP: // up 17 lines
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line -= 17;
                        if (top_line < 0)
                        {
                            top_line = 0;
                        }
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_END:
                case FIK_CTL_END:       // to end of entry
                    if (in_scrolling_mode)
                    {
                        top_line = lines_in_entry - 17;
                        left_column = 0;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_HOME:
                case FIK_CTL_HOME:     // to beginning of entry
                    if (in_scrolling_mode)
                    {
                        left_column = 0;
                        top_line = left_column;
                        rewrite_infbuf = true;
                    }
                    break;
                default:
                    break;
                }
            }
            else
            {
                done = true;  // a key other than scrolling key was pressed
            }
        }
        g_text_cbase = 0;
        driver_hide_text_cursor();
        driver_unstack_screen();
    }
    return 0;
}

static void format_parmfile_line(int choice, char *buf)
{
    int c, i;
    char line[80];
    std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
    while (getc(gfe_file) != '{')
    {
    }
    do
    {
        c = getc(gfe_file);
    }
    while (c == ' ' || c == '\t' || c == ';');
    i = 0;
    while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032')
    {
        line[i++] = (char)((c == '\t') ? ' ' : c);
        c = getc(gfe_file);
    }
    line[i] = 0;
    std::sprintf(buf, "%-20s%-56s", gfe_choices[choice]->name, line);
}
