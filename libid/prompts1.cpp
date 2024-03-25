/*
        Various routines that prompt for things.
*/
#include "port.h"
#include "prototyp.h"

#include "prompts1.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "calc_frac_init.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "find_extra_param.h"
#include "find_file_item.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "full_screen_prompt.h"
#include "get_corners.h"
#include "get_file_entry.h"
#include "get_key_no_help.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "help_title.h"
#include "id_data.h"
#include "jb.h"
#include "load_entry_text.h"
#include "loadfile.h"
#include "lorenz.h"
#include "lsys_fns.h"
#include "os.h"
#include "param_not_used.h"
#include "parser.h"
#include "set_default_parms.h"
#include "shell_sort.h"
#include "stop_msg.h"
#include "trig_fns.h"
#include "type_has_param.h"
#include "zoom.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static  fractal_type select_fracttype(fractal_type t);
static  int sel_fractype_help(int curkey, int choice);
static bool select_type_params(fractal_type newfractype, fractal_type oldfractype);

static char ifsmask[13]     = {"*.ifs"};
static char formmask[13]    = {"*.frm"};
static char lsysmask[13]    = {"*.l"};

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
    shell_sort(&choices, numtypes, sizeof(FT_CHOICE *)); // sort list
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

// JIIM
#ifdef RANDOM_RUN
static const char *JIIMstr1{"Breadth first, Depth first, Random Walk, Random Run?"};
static char const *s_jiim_method[]{
    to_string(Major::breadth_first), //
    to_string(Major::depth_first),   //
    to_string(Major::random_walk),   //
    to_string(Major::random_run)     //
};
#else
static const char *JIIMstr1{"Breadth first, Depth first, Random Walk"};
static const char *s_jiim_method[]{
    to_string(Major::breadth_first), //
    to_string(Major::depth_first),   //
    to_string(Major::random_walk)    //
};
#endif
static char JIIMstr2[] = "Left first or Right first?";

static char tstack[4096] = { 0 };

static char const *jiim_left_right_list[]{
    to_string(Minor::left_first), //
    to_string(Minor::right_first) //
};

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
    std::vector<char const *> trignameptr;
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
            paramvalues[promptnum].uval.dval = std::atof(tmpbuf);
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

    trignameptr.resize(g_num_trig_functions);
    for (int i = g_num_trig_functions-1; i >= 0; --i)
    {
        trignameptr[i] = g_trig_fn[i].name;
    }
    for (int i = 0; i < numtrig; i++)
    {
        paramvalues[promptnum].type = 'l';
        paramvalues[promptnum].uval.ch.val  = static_cast<int>(g_trig_index[i]);
        paramvalues[promptnum].uval.ch.llen = g_num_trig_functions;
        paramvalues[promptnum].uval.ch.vlen = 6;
        paramvalues[promptnum].uval.ch.list = trignameptr.data();
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
        paramvalues[promptnum].uval.ch.val  = static_cast<int>(g_julibrot_3d_mode);
        paramvalues[promptnum].uval.ch.llen = 4;
        paramvalues[promptnum].uval.ch.vlen = 9;
        paramvalues[promptnum].uval.ch.list = g_julibrot_3d_options;
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
        paramvalues[promptnum].uval.ch.list = s_jiim_method;
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
    set_bailout_formula(g_bail_out_test);

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
        g_julibrot_3d_mode = static_cast<julibrot_3d_mode>(paramvalues[promptnum++].uval.ch.val);
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
