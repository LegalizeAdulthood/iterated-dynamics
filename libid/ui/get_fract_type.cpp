// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_fract_type.h"

#include "engine/bailout_formula.h"
#include "engine/Inversion.h"
#include "engine/param_not_used.h"
#include "engine/Potential.h"
#include "engine/trig_fns.h"
#include "engine/type_has_param.h"
#include "engine/UserData.h"
#include "fractals/fractalp.h"
#include "fractals/ifs.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "helpcom.h"
#include "io/file_item.h"
#include "io/load_entry_text.h"
#include "io/loadfile.h"
#include "misc/ValueSaver.h"
#include "ui/full_screen_choice.h"
#include "ui/full_screen_prompt.h"
#include "ui/get_corners.h"
#include "ui/get_file_entry.h"
#include "ui/help.h"
#include "ui/id_keys.h"
#include "ui/set_default_params.h"
#include "ui/shell_sort.h"
#include "ui/stop_msg.h"

#include <fmt/format.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

namespace
{

struct FractalTypeChoice
{
    char name[15];
    int num;
};

} // namespace

static FractalTypeChoice **s_ft_choices{}; // for sel_fractype_help subrtn

// Julia inverse iteration method (jiim)
#ifdef RANDOM_RUN
static const char *s_jiim_method_prompt{"Breadth first, Depth first, Random Walk, Random Run?"};
static char const *s_jiim_method[]{
    to_string(Major::breadth_first), //
    to_string(Major::depth_first),   //
    to_string(Major::random_walk),   //
    to_string(Major::random_run)     //
};
#else
static const char *s_jiim_method_prompt{"Breadth first, Depth first, Random Walk"};
static const char *s_jiim_method[]{
    to_string(Major::BREADTH_FIRST), //
    to_string(Major::DEPTH_FIRST),   //
    to_string(Major::RANDOM_WALK)    //
};
#endif
static const char *s_jiim_left_right_names[]{
    to_string(Minor::LEFT_FIRST), //
    to_string(Minor::RIGHT_FIRST) //
};
static char s_tmp_stack[4096]{};

// forward declarations
static FractalType select_fract_type(FractalType t);
static int sel_fract_type_help(int key, int choice);
static bool select_type_params(FractalType new_fract_type, FractalType old_fract_type);

// prompt for and select fractal type
int get_fract_type()
{
    bool accepted{};
    FractalType old_fract_type = g_fractal_type;
    FractalType t{g_fractal_type};
    while (true)
    {
        t = select_fract_type(t);
        if (t == FractalType::NO_FRACTAL)
        {
            break;
        }
        accepted = select_type_params(t, g_fractal_type);
        if (accepted)
        {
            // ok, all done
            break;
        }
    }
    if (!accepted)
    {
        set_fractal_type(old_fract_type);
    }
    return accepted ? 0 : -1;
}

// Select a fractal type; returns FractalType::NO_FRACTAL if canceled out.
static FractalType select_fract_type(FractalType t)
{
    enum
    {
        MAX_FRACT_TYPES = 200
    };
    char type_name[40];
    std::array<FractalTypeChoice, MAX_FRACT_TYPES> storage{};
    std::array<FractalTypeChoice *, MAX_FRACT_TYPES> choices;
    std::array<int, MAX_FRACT_TYPES> attributes;

    // steal existing array for "choices"
    choices[0] = &storage[0];
    attributes[0] = 1;
    for (int i = 1; i < MAX_FRACT_TYPES; ++i)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }
    s_ft_choices = &choices[0];

    // setup context sensitive help
    ValueSaver save_help_mode(g_help_mode, HelpLabels::HELP_FRACTALS);
    if (t == FractalType::IFS_3D)
    {
        t = FractalType::IFS;
    }
    int num_types{};
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific specific{g_fractal_specific[i]};
        if (g_julibrot && !bit_set(specific.flags, FractalFlags::OK_JB))
        {
            continue;
        }
        std::strcpy(choices[num_types]->name, specific.name);
        choices[num_types]->name[14] = 0; // safety
        choices[num_types]->num = i;      // remember where the real item is
        ++num_types;
    }
    shell_sort(&choices, num_types, sizeof(FractalTypeChoice *)); // sort list
    int j = 0;
    for (int i = 0; i < num_types; ++i)   // find starting choice in sorted list
    {
        if (g_fractal_specific[choices[i]->num].type == t)
        {
            j = i;
            break;
        }
    }

    type_name[0] = 0;
    const int done = full_screen_choice(ChoiceFlags::HELP | ChoiceFlags::INSTRUCTIONS,
        g_julibrot ? "Select Orbit Algorithm for Julibrot" : "Select a Fractal Type", nullptr,
        "Press F2 for a description of the highlighted type", num_types, (const char **) choices.data(),
        attributes.data(), 0, 0, 0, j, nullptr, type_name, nullptr, sel_fract_type_help);
    if (done < 0)
    {
        return FractalType::NO_FRACTAL;
    }

    return g_fractal_specific[choices[done]->num].type;
}

static int sel_fract_type_help(const int key, const int choice)
{
    if (key == ID_KEY_F2)
    {
        ValueSaver saved_help_mode{g_help_mode, g_fractal_specific[s_ft_choices[choice]->num].help_text};
        help::help();
    }
    return 0;
}

void set_fractal_default_functions(const FractalType previous)
{
    switch (g_fractal_type)
    {
    case FractalType::BIFURCATION:
        if (previous != FractalType::BIFURCATION)
        {
            set_trig_array(0, "ident");
        }
        break;

    case FractalType::BIF_STEWART:
        if (previous != FractalType::BIF_STEWART)
        {
            set_trig_array(0, "ident");
        }
        break;

    case FractalType::BIF_LAMBDA:
        if (previous != FractalType::BIF_LAMBDA)
        {
            set_trig_array(0, "ident");
        }
        break;

    case FractalType::BIF_EQ_SIN_PI:
        if (previous != FractalType::BIF_EQ_SIN_PI)
        {
            set_trig_array(0, "sin");
        }
        break;

    case FractalType::BIF_PLUS_SIN_PI:
        if (previous != FractalType::BIF_PLUS_SIN_PI)
        {
            set_trig_array(0, "sin");
        }
        break;

    // Next assumes that user going between popcorn and popcornjul
    // might not want to change function variables
    case FractalType::POPCORN:
    case FractalType::POPCORN_JUL:
        if (!(previous == FractalType::POPCORN || previous == FractalType::POPCORN_JUL))
        {
            set_function_param_defaults();
        }
        break;

    // set LATOO function defaults
    case FractalType::LATOO:
        if (previous != FractalType::LATOO)
        {
            set_function_param_defaults();
        }
        break;

    default:
        break;
    }
}

namespace {

class SaveFractalType
{
public:
    SaveFractalType() = default;

    ~SaveFractalType()
    {
        if (!m_commit)
        {
            g_fractal_type = m_fractal_type;
            g_cur_fractal_specific = m_fractal_specific;
        }
    }

    void commit()
    {
        m_commit = true;
    }

private:
    bool m_commit{};
    FractalType m_fractal_type{g_fractal_type};
    FractalSpecific *m_fractal_specific{g_cur_fractal_specific};
};

} // namespace

// prompt for new fractal type parameters; returns true on params accepted.
static bool select_type_params(const FractalType new_fract_type, const FractalType old_fract_type)
{
    SaveFractalType saved_fractal_type;

sel_type_restart:
    set_fractal_type(new_fract_type);

    if (g_fractal_type == FractalType::L_SYSTEM)
    {
        ValueSaver saved_help_mode(g_help_mode, HelpLabels::HT_L_SYSTEM);
        const std::string saved_filename{g_l_system_filename.string()};
        const std::string saved_name{g_l_system_name};
        if (get_file_entry(ItemType::L_SYSTEM, g_l_system_filename, g_l_system_name) < 0)
        {
            g_l_system_filename = saved_filename;
            g_l_system_name = saved_name;
            return false;
        }
    }
    else if (g_fractal_type == FractalType::FORMULA)
    {
        ValueSaver saved_help_mode(g_help_mode, HelpLabels::HT_FORMULA);
        const std::string saved_filename{g_formula_filename.string()};
        const std::string saved_name{g_formula_name};
        if (get_file_entry(ItemType::FORMULA, g_formula_filename, g_formula_name) < 0)
        {
            g_formula_filename = saved_filename;
            g_formula_name = saved_name;
            return false;
        }
    }
    else if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
    {
        ValueSaver saved_help_mode(g_help_mode, HelpLabels::HT_IFS);
        const std::string saved_filename{g_ifs_filename.string()};
        const std::string saved_name{g_ifs_name};
        if (get_file_entry(ItemType::IFS, g_ifs_filename, g_ifs_name) < 0)
        {
            g_ifs_filename = saved_filename;
            g_ifs_name = saved_name;
            return false;
        }
    }

    save_trig_functions();
    save_params();
    set_fractal_default_functions(old_fract_type);
    set_default_params();

    if (get_fract_params(false) < 0)
    {
        restore_trig_functions();
        restore_params();
        if (g_fractal_type == FractalType::FORMULA ||
            g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D ||
            g_fractal_type == FractalType::L_SYSTEM)
        {
            goto sel_type_restart;
        }
        return false;
    }

    if (new_fract_type != old_fract_type)
    {
        g_inversion.invert = 0;
        g_inversion.params.fill(0.0);
    }

    saved_fractal_type.commit();
    return true;
}

int get_fract_params(bool prompt_for_type_params)        // prompt for type-specific params
{
    const char *julia_orbit_name = nullptr;
    int num_params;
    int num_trig;
    std::array<FullScreenValues, 30> param_values;
    std::array<const char *, 30> choices;
    // ReSharper disable once CppTooWideScope
    char bailout_msg[50];
    long old_bailout = 0L;
    int prompt_num;
    char msg[120];
    const char *type_name;
    int ret = 0;
    static const char *trg[] =
    {
        "First Function", "Second Function", "Third Function", "Fourth Function"
    };
    std::FILE *entry_file;
    std::vector<const char *> trig_name_ptr;
    const char *bail_name_ptr[] = {"mod", "real", "imag", "or", "and", "manh", "manr"};
    FractalSpecific *jb_orbit = nullptr;
    int first_param = 0;
    int last_param  = MAX_PARAMS;
    std::array<double, MAX_PARAMS> old_param;
    int fn_key_mask = 0;

    old_bailout = g_user.bailout_value;
    g_julibrot = g_fractal_type == FractalType::JULIBROT;
    FractalType current_type = g_fractal_type;
    g_cur_fractal_specific = get_fractal_specific(current_type);
    s_tmp_stack[0] = 0;
    HelpLabels help = g_cur_fractal_specific->help_formula;
    if (help < HelpLabels::NONE)
    {
        const char *entry_name;
        if (help == HelpLabels::SPECIAL_FORMULA)
        {
            // special for formula
            entry_name = g_formula_name.c_str();
        }
        else if (help == HelpLabels::SPECIAL_L_SYSTEM)
        {
            // special for lsystem
            entry_name = g_l_system_name.c_str();
        }
        else if (help == HelpLabels::SPECIAL_IFS)
        {
            // special for ifs
            entry_name = g_ifs_name.c_str();
        }
        else
        {
            // this shouldn't happen
            entry_name = nullptr;
        }
        const auto item_for_help = [](HelpLabels label)
        {
            switch (label)
            {
            case HelpLabels::SPECIAL_IFS:
                return ItemType::IFS;
            case HelpLabels::SPECIAL_L_SYSTEM:
                return ItemType::L_SYSTEM;
            case HelpLabels::SPECIAL_FORMULA:
                return ItemType::FORMULA;
            default:
                throw std::runtime_error(
                    "Invalid help label " + std::to_string(static_cast<int>(label)) + " for find_file_item");
            }
        };
        const auto item_path = [](HelpLabels label) -> std::filesystem::path &
        {
            switch (label)
            {
            case HelpLabels::SPECIAL_FORMULA:
                return g_formula_filename;
            case HelpLabels::SPECIAL_IFS:
                return g_ifs_filename;
            case HelpLabels::SPECIAL_L_SYSTEM:
                return g_l_system_filename;
            default:
                throw std::runtime_error(
                    "Invalid help label " + std::to_string(static_cast<int>(label)) + " for find_file_item");
            }
        };
        if (find_file_item(item_path(help), entry_name, &entry_file, item_for_help(help)) == 0)
        {
            load_entry_text(entry_file, s_tmp_stack, 17, 0, 0);
            std::fclose(entry_file);
            if (g_fractal_type == FractalType::FORMULA)
            {
                frm_get_param_stuff(entry_name); // no error check, should be okay, from above
            }
        }
    }
    else if (help >= HelpLabels::HELP_INDEX)
    {
        int c;
        int lines;
        read_help_topic(help, 0, 2000, s_tmp_stack); // need error handling here ??
        s_tmp_stack[2000-static_cast<int>(help)] = 0;
        int i = 0;
        lines = 0;
        int j = 0;
        int k = 1;
        while ((c = s_tmp_stack[i++]) != 0)
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
                s_tmp_stack[j++] = static_cast<char>(c);
            }
        }
        while (--j >= 0 && s_tmp_stack[j] == '\n')
        {
        }
        s_tmp_stack[j+1] = 0;
    }
    FractalSpecific *save_specific = g_cur_fractal_specific;
    int orbit_bailout;

gfp_top:
    prompt_num = 0;
    if (g_julibrot)
    {
        if (const FractalType i = select_fract_type(g_new_orbit_type); i == FractalType::NO_FRACTAL)
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
        jb_orbit = get_fractal_specific(g_new_orbit_type);
        julia_orbit_name = jb_orbit->name;
    }

    if (g_fractal_type == FractalType::FORMULA)
    {
        if (g_frm_uses_p1)    // set first parameter
        {
            first_param = 0;
        }
        else if (g_frm_uses_p2)
        {
            first_param = 2;
        }
        else if (g_frm_uses_p3)
        {
            first_param = 4;
        }
        else if (g_frm_uses_p4)
        {
            first_param = 6;
        }
        else
        {
            first_param = 8; // uses_p5 or no parameter
        }

        if (g_frm_uses_p5)    // set last parameter
        {
            last_param = 10;
        }
        else if (g_frm_uses_p4)
        {
            last_param = 8;
        }
        else if (g_frm_uses_p3)
        {
            last_param = 6;
        }
        else if (g_frm_uses_p2)
        {
            last_param = 4;
        }
        else
        {
            last_param = 2; // uses_p1 or no parameter
        }
    }

    if (g_julibrot)
    {
        g_cur_fractal_specific = jb_orbit;
        first_param = 2; // in most case Julibrot does not need first two parms
        if (g_new_orbit_type == FractalType::QUAT_JUL        // all parameters needed
            || g_new_orbit_type == FractalType::HYPER_CMPLX_J)
        {
            first_param = 0;
            last_param = 4;
        }
        if (g_new_orbit_type == FractalType::QUAT           // no parameters needed
            || g_new_orbit_type == FractalType::HYPER_CMPLX)
        {
            first_param = 4;
        }
    }
    num_params = 0;
    {
        int j = 0;
        for (int i = first_param; i < last_param; i++)
        {
            std::array<const char *, MAX_PARAMS> param_prompt;
            char tmp_buf[30];
            if (!type_has_param(g_julibrot ? g_new_orbit_type : g_fractal_type, i, &param_prompt[j]))
            {
                if (current_type == FractalType::FORMULA)
                {
                    if (param_not_used(i))
                    {
                        continue;
                    }
                }
                break;
            }
            num_params++;
            choices[prompt_num] = param_prompt[j++];
            param_values[prompt_num].type = 'd';

            if (choices[prompt_num][0] == '+')
            {
                choices[prompt_num]++;
                param_values[prompt_num].type = 'D';
            }
            else if (choices[prompt_num][0] == '#')
            {
                choices[prompt_num]++;
            }
            *fmt::format_to(tmp_buf, "{:.17g}", g_params[i]).out = '\0';
            param_values[prompt_num].uval.dval = std::atof(tmp_buf);
            old_param[i] = param_values[prompt_num++].uval.dval;
        }
    }

    /* The following is a goofy kludge to make reading in the formula
     * parameters work.
     */
    if (current_type == FractalType::FORMULA)
    {
        num_params = last_param - first_param;
    }

    num_trig = +g_cur_fractal_specific->flags >> 6 & 7;
    if (current_type == FractalType::FORMULA)
    {
        num_trig = g_max_function;
    }

    trig_name_ptr.resize(g_num_trig_functions);
    for (int i = g_num_trig_functions-1; i >= 0; --i)
    {
        trig_name_ptr[i] = g_trig_fn[i].name;
    }
    for (int i = 0; i < num_trig; i++)
    {
        param_values[prompt_num].type = 'l';
        param_values[prompt_num].uval.ch.val  = +g_trig_index[i];
        param_values[prompt_num].uval.ch.list_len = g_num_trig_functions;
        param_values[prompt_num].uval.ch.vlen = 6;
        param_values[prompt_num].uval.ch.list = trig_name_ptr.data();
        choices[prompt_num++] = trg[i];
    }
    type_name = g_cur_fractal_specific->name;

    orbit_bailout = g_cur_fractal_specific->orbit_bailout;
    if (orbit_bailout != 0                                            //
        && g_cur_fractal_specific->calc_type == standard_fractal_type //
        && bit_set(g_cur_fractal_specific->flags, FractalFlags::BAIL_TEST))
    {
        param_values[prompt_num].type = 'l';
        param_values[prompt_num].uval.ch.val  = static_cast<int>(g_bailout_test);
        param_values[prompt_num].uval.ch.list_len = 7;
        param_values[prompt_num].uval.ch.vlen = 6;
        param_values[prompt_num].uval.ch.list = bail_name_ptr;
        choices[prompt_num++] = "Bailout Test (mod, real, imag, or, and, manh, manr)";
    }

    if (orbit_bailout)
    {
        if (g_potential.params[0] != 0.0 && g_potential.params[2] != 0.0)
        {
            param_values[prompt_num].type = '*';
            choices[prompt_num++] = "Bailout: continuous potential (Y screen) value in use";
        }
        else
        {
            const char *tmp_ptr;
            choices[prompt_num] = "Bailout value (0 means use default)";
            param_values[prompt_num].type = 'L';
            old_bailout = g_user.bailout_value;
            param_values[prompt_num++].uval.Lval = old_bailout;
            param_values[prompt_num].type = '*';
            tmp_ptr = type_name;
            if (g_user.biomorph_value != -1)
            {
                orbit_bailout = 100;
                tmp_ptr = "biomorph";
            }
            *fmt::format_to(bailout_msg, "    ({:s} default is {:d})", tmp_ptr, orbit_bailout).out = '\0';
            choices[prompt_num++] = bailout_msg;
        }
    }
    if (g_julibrot)
    {
        const char *v0 = "From cx (real part)";
        const char *v1 = "From cy (imaginary part)";
        const char *v2 = "To   cx (real part)";
        const char *v3 = "To   cy (imaginary part)";
        switch (g_new_orbit_type)
        {
        case FractalType::QUAT:
        case FractalType::HYPER_CMPLX:
            v0 = "From cj (3rd dim)";
            v1 = "From ck (4th dim)";
            v2 = "To   cj (3rd dim)";
            v3 = "To   ck (4th dim)";
            break;
        case FractalType::QUAT_JUL:
        case FractalType::HYPER_CMPLX_J:
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

        g_cur_fractal_specific = save_specific;
        param_values[prompt_num].uval.dval = g_julibrot_x_max;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = v0;
        param_values[prompt_num].uval.dval = g_julibrot_y_max;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = v1;
        param_values[prompt_num].uval.dval = g_julibrot_x_min;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = v2;
        param_values[prompt_num].uval.dval = g_julibrot_y_min;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = v3;
        param_values[prompt_num].uval.ival = g_julibrot_z_dots;
        param_values[prompt_num].type = 'i';
        choices[prompt_num++] = "Number of z pixels";

        param_values[prompt_num].type = 'l';
        param_values[prompt_num].uval.ch.val  = static_cast<int>(g_julibrot_3d_mode);
        param_values[prompt_num].uval.ch.list_len = 4;
        param_values[prompt_num].uval.ch.vlen = 9;
        param_values[prompt_num].uval.ch.list = g_julibrot_3d_options;
        choices[prompt_num++] = "3D Mode";

        param_values[prompt_num].uval.dval = g_eyes;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Distance between eyes";
        param_values[prompt_num].uval.dval = g_julibrot_origin;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Location of z origin";
        param_values[prompt_num].uval.dval = g_julibrot_depth;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Depth of z";
        param_values[prompt_num].uval.dval = g_julibrot_height;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Screen height";
        param_values[prompt_num].uval.dval = g_julibrot_width;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Screen width";
        param_values[prompt_num].uval.dval = g_julibrot_dist;
        param_values[prompt_num].type = 'f';
        choices[prompt_num++] = "Distance to Screen";
    }

    if (current_type == FractalType::INVERSE_JULIA)
    {
        choices[prompt_num] = s_jiim_method_prompt;
        param_values[prompt_num].type = 'l';
        param_values[prompt_num].uval.ch.list = s_jiim_method;
        param_values[prompt_num].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
        paramvalues[promptnum].uval.ch.llen = 4;
#else
        param_values[prompt_num].uval.ch.list_len = 3; // disable random run
#endif
        param_values[prompt_num++].uval.ch.val  = static_cast<int>(g_major_method);

        choices[prompt_num] = "Left first or Right first?";
        param_values[prompt_num].type = 'l';
        param_values[prompt_num].uval.ch.list = s_jiim_left_right_names;
        param_values[prompt_num].uval.ch.vlen = 5;
        param_values[prompt_num].uval.ch.list_len = 2;
        param_values[prompt_num++].uval.ch.val  = static_cast<int>(g_inverse_julia_minor_method);
    }

    if (current_type == FractalType::FORMULA && g_frm_uses_ismand)
    {
        choices[prompt_num] = "ismand";
        param_values[prompt_num].type = 'y';
        param_values[prompt_num++].uval.ch.val = g_is_mandelbrot ? 1 : 0;
    }

    if (prompt_for_type_params && g_display_3d > Display3DMode::NONE)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Current type has no type-specific parameters");
        goto gfp_exit;
    }
    if (g_julibrot)
    {
        *fmt::format_to(msg, "Julibrot Parameters (orbit={:s})", julia_orbit_name).out = '\0';
    }
    else
    {
        *fmt::format_to(msg, "Parameters for fractal type {:s}", type_name).out = '\0';
    }
    if (g_bf_math == BFMathType::NONE)
    {
        std::strcat(msg, "\n(Press F6 for corner parameters)");
        fn_key_mask = 1U << 6;     // F6 exits
    }
    full_screen_reset_scrolling();
    while (true)
    {
        ValueSaver saved_help_mode{g_help_mode, g_cur_fractal_specific->help_text};
        int i = full_screen_prompt(msg, prompt_num, choices.data(), param_values.data(), fn_key_mask, s_tmp_stack);
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
        if (i != ID_KEY_F6)
        {
            break;
        }
        if (g_bf_math == BFMathType::NONE)
        {
            if (get_corners() > 0)
            {
                ret = 1;
            }
        }
    }
    prompt_num = 0;
    for (int i = first_param; i < num_params+first_param; i++)
    {
        if (current_type == FractalType::FORMULA)
        {
            if (param_not_used(i))
            {
                continue;
            }
        }
        if (old_param[i] != param_values[prompt_num].uval.dval)
        {
            g_params[i] = param_values[prompt_num].uval.dval;
            ret = 1;
        }
        ++prompt_num;
    }

    for (int i = 0; i < num_trig; i++)
    {
        if (param_values[prompt_num].uval.ch.val != +g_trig_index[i])
        {
            set_trig_array(i, g_trig_fn[param_values[prompt_num].uval.ch.val].name);
            ret = 1;
        }
        ++prompt_num;
    }

    if (g_julibrot)
    {
        g_cur_fractal_specific = jb_orbit;
    }

    orbit_bailout = g_cur_fractal_specific->orbit_bailout;
    if (orbit_bailout != 0                                            //
        && g_cur_fractal_specific->calc_type == standard_fractal_type //
        && bit_set(g_cur_fractal_specific->flags, FractalFlags::BAIL_TEST))
    {
        if (param_values[prompt_num].uval.ch.val != static_cast<int>(g_bailout_test))
        {
            g_bailout_test = static_cast<Bailout>(param_values[prompt_num].uval.ch.val);
            ret = 1;
        }
        prompt_num++;
    }
    else
    {
        g_bailout_test = Bailout::MOD;
    }
    set_bailout_formula(g_bailout_test);

    if (orbit_bailout)
    {
        if (g_potential.params[0] != 0.0 && g_potential.params[2] != 0.0)
        {
            prompt_num++;
        }
        else
        {
            g_user.bailout_value = param_values[prompt_num++].uval.Lval;
            if (g_user.bailout_value != 0 && (g_user.bailout_value < 1 || g_user.bailout_value > 2100000000L))
            {
                g_user.bailout_value = old_bailout;
            }
            if (g_user.bailout_value != old_bailout)
            {
                ret = 1;
            }
            prompt_num++;
        }
    }

    if (g_julibrot)
    {
        g_julibrot_x_max    = param_values[prompt_num++].uval.dval;
        g_julibrot_y_max    = param_values[prompt_num++].uval.dval;
        g_julibrot_x_min    = param_values[prompt_num++].uval.dval;
        g_julibrot_y_min    = param_values[prompt_num++].uval.dval;
        g_julibrot_z_dots      = param_values[prompt_num++].uval.ival;
        g_julibrot_3d_mode = static_cast<Julibrot3DMode>(param_values[prompt_num++].uval.ch.val);
        g_eyes     = static_cast<float>(param_values[prompt_num++].uval.dval);
        g_julibrot_origin   = static_cast<float>(param_values[prompt_num++].uval.dval);
        g_julibrot_depth    = static_cast<float>(param_values[prompt_num++].uval.dval);
        g_julibrot_height   = static_cast<float>(param_values[prompt_num++].uval.dval);
        g_julibrot_width    = static_cast<float>(param_values[prompt_num++].uval.dval);
        g_julibrot_dist     = static_cast<float>(param_values[prompt_num++].uval.dval);
        ret = 1;  // force new calc since not resumable anyway
    }
    if (current_type == FractalType::INVERSE_JULIA)
    {
        if (param_values[prompt_num].uval.ch.val != static_cast<int>(g_major_method)
            || param_values[prompt_num+1].uval.ch.val != static_cast<int>(g_inverse_julia_minor_method))
        {
            ret = 1;
        }
        g_major_method = static_cast<Major>(param_values[prompt_num++].uval.ch.val);
        g_inverse_julia_minor_method = static_cast<Minor>(param_values[prompt_num++].uval.ch.val);
    }
    if (current_type == FractalType::FORMULA && g_frm_uses_ismand)
    {
        if (g_is_mandelbrot != (param_values[prompt_num].uval.ch.val != 0))
        {
            g_is_mandelbrot = param_values[prompt_num].uval.ch.val != 0;
            ret = 1;
        }
        ++prompt_num;
    }
gfp_exit:
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    return ret;
}

} // namespace id::ui
