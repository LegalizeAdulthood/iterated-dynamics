// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/evolve.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/param_not_used.h"
#include "engine/pixel_limits.h"
#include "engine/type_has_param.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "fractals/parser.h"
#include "helpdefs.h"
#include "math/sqr.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/id_keys.h"
#include "ui/trig_fns.h"
#include "ui/zoom.h"

#include <config/port.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <vector>

enum
{
    EVOLVE_MAX_GRID_SIZE = 51  // This is arbitrary, = 1024/20
};

// for saving evolution data of center image
struct ParamHistory
{
    double param0;
    double param1;
    double param2;
    double param3;
    double param4;
    double param5;
    double param6;
    double param7;
    double param8;
    double param9;
    int inside;
    int outside;
    int decomp0;
    double invert0;
    double invert1;
    double invert2;
    Byte trig_index0;
    Byte trig_index1;
    Byte trig_index2;
    Byte trig_index3;
    Bailout bailout_test;
};

GeneBase g_gene_bank[NUM_GENES];

// px and py are coordinates in the parameter grid (small images on screen)
// evolving = flag, evolve_image_grid_size = dimensions of image grid (evolve_image_grid_size x evolve_image_grid_size)
int g_evolve_param_grid_x;
int g_evolve_param_grid_y;
EvolutionModeFlags g_evolving{EvolutionModeFlags::NONE};
int g_evolve_image_grid_size;

// used to replay random sequences to obtain correct values when selecting a
// seed image for next generation
unsigned int g_evolve_this_generation_random_seed;

// variation factors, opx, opy, paramrangex/y dpx, dpy.
// used in field mapping for smooth variation across screen.
// opx =offset param x,
// dpx = delta param per image,
// paramrangex = variation across grid of param ...likewise for py
// evolve_max_random_mutation is amount of random mutation used in random modes ,
// evolve_mutation_reduction_factor is used to decrease evolve_max_random_mutation from one generation to the
// next to eventually produce a stable population
double g_evolve_x_parameter_offset;
double g_evolve_y_parameter_offset;
double g_evolve_new_x_parameter_offset;
double g_evolve_new_y_parameter_offset;
double g_evolve_x_parameter_range;
double g_evolve_y_parameter_range;
double g_evolve_dist_per_x;
double g_evolve_dist_per_y;
double g_evolve_max_random_mutation;
double g_evolve_mutation_reduction_factor;
double g_evolve_param_zoom;

// offset for discrete parameters x and y...
// used for things like inside or outside types, bailout tests, trig fn etc
char g_evolve_discrete_x_parameter_offset;
char g_evolve_discrete_y_parameter_offset;
char g_evolve_new_discrete_x_parameter_offset;
char g_evolve_new_discrete_y_parameter_offset;

int g_evolve_param_box_count;

static int s_evol_count_box[EVOLVE_MAX_GRID_SIZE][EVOLVE_MAX_GRID_SIZE];
static std::vector<int> s_param_box_x;
static std::vector<int> s_param_box_y;
static std::vector<int> s_param_box_values;
static int s_image_box_count;
static std::vector<int> s_image_box_x;
static std::vector<int> s_image_box_y;
static std::vector<int> s_image_box_values;
static ParamHistory s_old_history{};

static void vary_dbl(GeneBase gene[], int rand_val, int i);
static int vary_int(int rand_value, int limit, Variations mode);
static int wrapped_positive_vary_int(int rand_value, int limit, Variations mode);
static void vary_inside(GeneBase gene[], int rand_val, int i);
static void vary_outside(GeneBase gene[], int rand_val, int i);
static void vary_pwr2(GeneBase gene[], int rand_val, int i);
static void vary_trig(GeneBase gene[], int rand_val, int i);
static void vary_bo_test(GeneBase gene[], int rand_val, int i);
static void vary_inv(GeneBase gene[], int rand_val, int i);
static bool explore_check();
static void set_random(int count);

void copy_genes_from_bank(GeneBase gene[NUM_GENES])
{
    std::copy(&g_gene_bank[0], &g_gene_bank[NUM_GENES], &gene[0]);
}

template <int N>
static bool equal(const std::int16_t (&lhs)[N], const std::int16_t (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

static bool within_eps(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-6f;
}

bool operator==(const EvolutionInfo &lhs, const EvolutionInfo &rhs)
{
    return lhs.evolving == rhs.evolving                                       //
        && lhs.image_grid_size == rhs.image_grid_size                         //
        && lhs.this_generation_random_seed == rhs.this_generation_random_seed //
        && lhs.max_random_mutation == rhs.max_random_mutation                 //
        && within_eps(lhs.x_parameter_range, rhs.x_parameter_range)           //
        && within_eps(lhs.y_parameter_range, rhs.y_parameter_range)           //
        && within_eps(lhs.x_parameter_offset, rhs.x_parameter_offset)         //
        && within_eps(lhs.y_parameter_offset, rhs.y_parameter_offset)         //
        && lhs.discrete_x_parameter_offset == rhs.discrete_x_parameter_offset //
        && lhs.discrete_y_parameter_offset == rhs.discrete_y_parameter_offset //
        && lhs.px == rhs.px                                                   //
        && lhs.py == rhs.py                                                   //
        && lhs.screen_x_offset == rhs.screen_x_offset                         //
        && lhs.screen_y_offset == rhs.screen_y_offset                         //
        && lhs.x_dots == rhs.x_dots                                           //
        && lhs.y_dots == rhs.y_dots                                           //
        && equal(lhs.mutate, rhs.mutate)                                      //
        && lhs.count == rhs.count;                                            //
}

void copy_genes_to_bank(const GeneBase gene[NUM_GENES])
{
    // cppcheck-suppress arrayIndexOutOfBounds
    std::copy(&gene[0], &gene[NUM_GENES], &g_gene_bank[0]);
}

// set up pointers and mutation params for all usable image
// control variables... revise as necessary when
// new vars come along... don't forget to increment NUM_GENES
// as well
void init_gene()
{
    //                        Use only 15 letters below: 123456789012345
    GeneBase gene[NUM_GENES] =
    {
        { &g_params[0], vary_dbl, Variations::RANDOM,       "Param 1 real", 1 },
        { &g_params[1], vary_dbl, Variations::RANDOM,       "Param 1 imag", 1 },
        { &g_params[2], vary_dbl, Variations::NONE,         "Param 2 real", 1 },
        { &g_params[3], vary_dbl, Variations::NONE,         "Param 2 imag", 1 },
        { &g_params[4], vary_dbl, Variations::NONE,         "Param 3 real", 1 },
        { &g_params[5], vary_dbl, Variations::NONE,         "Param 3 imag", 1 },
        { &g_params[6], vary_dbl, Variations::NONE,         "Param 4 real", 1 },
        { &g_params[7], vary_dbl, Variations::NONE,         "Param 4 imag", 1 },
        { &g_params[8], vary_dbl, Variations::NONE,         "Param 5 real", 1 },
        { &g_params[9], vary_dbl, Variations::NONE,         "Param 5 imag", 1 },
        { &g_inside_color, vary_inside, Variations::NONE,        "inside color", 2 },
        { &g_outside_color, vary_outside, Variations::NONE,      "outside color", 3 },
        { &g_decomp[0], vary_pwr2, Variations::NONE,       "decomposition", 4 },
        { &g_inversion[0], vary_inv, Variations::NONE,     "invert radius", 7 },
        { &g_inversion[1], vary_inv, Variations::NONE,     "invert center x", 7 },
        { &g_inversion[2], vary_inv, Variations::NONE,     "invert center y", 7 },
        { &g_trig_index[0], vary_trig, Variations::NONE,      "trig function 1", 5 },
        { &g_trig_index[1], vary_trig, Variations::NONE,      "trig fn 2", 5 },
        { &g_trig_index[2], vary_trig, Variations::NONE,      "trig fn 3", 5 },
        { &g_trig_index[3], vary_trig, Variations::NONE,      "trig fn 4", 5 },
        { &g_bailout_test, vary_bo_test, Variations::NONE,    "bailout test", 6 }
    };

    copy_genes_to_bank(gene);
}

void save_param_history()
{
    // save the old parameter history
    s_old_history.param0 = g_params[0];
    s_old_history.param1 = g_params[1];
    s_old_history.param2 = g_params[2];
    s_old_history.param3 = g_params[3];
    s_old_history.param4 = g_params[4];
    s_old_history.param5 = g_params[5];
    s_old_history.param6 = g_params[6];
    s_old_history.param7 = g_params[7];
    s_old_history.param8 = g_params[8];
    s_old_history.param9 = g_params[9];
    s_old_history.inside = g_inside_color;
    s_old_history.outside = g_outside_color;
    s_old_history.decomp0 = g_decomp[0];
    s_old_history.invert0 = g_inversion[0];
    s_old_history.invert1 = g_inversion[1];
    s_old_history.invert2 = g_inversion[2];
    s_old_history.trig_index0 = static_cast<Byte>(g_trig_index[0]);
    s_old_history.trig_index1 = static_cast<Byte>(g_trig_index[1]);
    s_old_history.trig_index2 = static_cast<Byte>(g_trig_index[2]);
    s_old_history.trig_index3 = static_cast<Byte>(g_trig_index[3]);
    s_old_history.bailout_test = g_bailout_test;
}

void restore_param_history()
{
    // restore the old parameter history
    g_params[0] = s_old_history.param0;
    g_params[1] = s_old_history.param1;
    g_params[2] = s_old_history.param2;
    g_params[3] = s_old_history.param3;
    g_params[4] = s_old_history.param4;
    g_params[5] = s_old_history.param5;
    g_params[6] = s_old_history.param6;
    g_params[7] = s_old_history.param7;
    g_params[8] = s_old_history.param8;
    g_params[9] = s_old_history.param9;
    g_inside_color = s_old_history.inside;
    g_outside_color = s_old_history.outside;
    g_decomp[0] = s_old_history.decomp0;
    g_inversion[0] = s_old_history.invert0;
    g_inversion[1] = s_old_history.invert1;
    g_inversion[2] = s_old_history.invert2;
    g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
    g_trig_index[0] = static_cast<TrigFn>(s_old_history.trig_index0);
    g_trig_index[1] = static_cast<TrigFn>(s_old_history.trig_index1);
    g_trig_index[2] = static_cast<TrigFn>(s_old_history.trig_index2);
    g_trig_index[3] = static_cast<TrigFn>(s_old_history.trig_index3);
    g_bailout_test = s_old_history.bailout_test;
}

// routine to vary doubles
void vary_dbl(GeneBase gene[], int rand_val, int i)
{
    int delta_y = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    switch (gene[i].mutate)
    {
    default:
    case Variations::NONE:
        break;
    case Variations::X:
        *(double *)gene[i].addr = g_evolve_param_grid_x * g_evolve_dist_per_x + g_evolve_x_parameter_offset; //paramspace x coord * per view delta px + offset
        break;
    case Variations::Y:
        *(double *)gene[i].addr = delta_y * g_evolve_dist_per_y + g_evolve_y_parameter_offset; //same for y
        break;
    case Variations::X_PLUS_Y:
        *(double *)gene[i].addr = g_evolve_param_grid_x*g_evolve_dist_per_x+ g_evolve_x_parameter_offset +(delta_y*g_evolve_dist_per_y)+ g_evolve_y_parameter_offset; //and x+y
        break;
    case Variations::X_MINUS_Y:
        *(double *)gene[i].addr = (g_evolve_param_grid_x*g_evolve_dist_per_x+ g_evolve_x_parameter_offset)-(delta_y*g_evolve_dist_per_y+ g_evolve_y_parameter_offset); //and x-y
        break;
    case Variations::RANDOM:
        *(double *)gene[i].addr += (((double)rand_val / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation;
        break;
    case Variations::WEIGHTED_RANDOM:
    {
        int mid = g_evolve_image_grid_size /2;
        double radius =  std::sqrt(static_cast<double>(sqr(g_evolve_param_grid_x - mid) + sqr(delta_y - mid)));
        *(double *)gene[i].addr += ((((double)rand_val / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation) * radius;
    }
    break;
    }
}

static int vary_int(int rand_value, int limit, Variations mode)
{
    int ret = 0;
    int delta_y = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    switch (mode)
    {
    default:
    case Variations::NONE:
        break;
    case Variations::X:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x)%limit;
        break;
    case Variations::Y:
        ret = (g_evolve_discrete_y_parameter_offset +delta_y)%limit;
        break;
    case Variations::X_PLUS_Y:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x+ g_evolve_discrete_y_parameter_offset +delta_y)%limit;
        break;
    case Variations::X_MINUS_Y:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x)-(g_evolve_discrete_y_parameter_offset +delta_y)%limit;
        break;
    case Variations::RANDOM:
        ret = rand_value % limit;
        break;
    case Variations::WEIGHTED_RANDOM:
    {
        int mid = g_evolve_image_grid_size /2;
        double radius =  std::sqrt(static_cast<double>(sqr(g_evolve_param_grid_x - mid) + sqr(delta_y - mid)));
        ret = (int)((((rand_value / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation) * radius);
        ret %= limit;
        break;
    }
    }
    return ret;
}

int wrapped_positive_vary_int(int rand_value, int limit, Variations mode)
{
    int i = vary_int(rand_value, limit, mode);
    return i < 0 ? limit + i : i;
}

void vary_inside(GeneBase gene[], int rand_val, int i)
{
    int choices[9] = { ZMAG, BOF60, BOF61, EPS_CROSS, STAR_TRAIL, PERIOD, FMODI, ATANI, ITER };
    if (gene[i].mutate != Variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_vary_int(rand_val, 9, gene[i].mutate)];
    }
}

void vary_outside(GeneBase gene[], int rand_val, int i)
{
    int choices[8] = { ITER, REAL, IMAG, MULT, SUM, ATAN, FMOD, TDIS };
    if (gene[i].mutate != Variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_vary_int(rand_val, 8, gene[i].mutate)];
    }
}

void vary_bo_test(GeneBase gene[], int rand_val, int i)
{
    int choices[7] =
    {
        static_cast<int>(Bailout::MOD),
        static_cast<int>(Bailout::REAL),
        static_cast<int>(Bailout::IMAG),
        static_cast<int>(Bailout::OR),
        static_cast<int>(Bailout::AND),
        static_cast<int>(Bailout::MANH),
        static_cast<int>(Bailout::MANR)
    };
    if (gene[i].mutate != Variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_vary_int(rand_val, 7, gene[i].mutate)];
        // move this next bit to varybot where it belongs
        set_bailout_formula(g_bailout_test);
    }
}

void vary_pwr2(GeneBase gene[], int rand_val, int i)
{
    int choices[9] = {0, 2, 4, 8, 16, 32, 64, 128, 256};
    if (gene[i].mutate != Variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_vary_int(rand_val, 9, gene[i].mutate)];
    }
}

void vary_trig(GeneBase gene[], int rand_val, int i)
{
    if (gene[i].mutate != Variations::NONE)
    {
        *static_cast<TrigFn *>(gene[i].addr) =
            static_cast<TrigFn>(wrapped_positive_vary_int(rand_val, g_num_trig_functions, gene[i].mutate));
    }
    set_trig_pointers(5); //set all trig ptrs up
}

void vary_inv(GeneBase gene[], int rand_val, int i)
{
    if (gene[i].mutate != Variations::NONE)
    {
        vary_dbl(gene, rand_val, i);
    }
    g_invert = (g_inversion[0] == 0.0) ? 0 : 3 ;
}

// ---------------------------------------------------------------------
//
//  get_evolve_params() is called whenever the 'ctrl_e' key
//  is pressed.  Return codes are:
//    -1  routine was ESCAPEd - no need to re-generate the images
//     0  minor variable changed.  No need to re-generate the image.
//     1  major parms changed.  Re-generate the images.
//
static int get_the_rest()
{
    ChoiceBuilder<20> choices;
    const char *evolve_modes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    GeneBase gene[NUM_GENES];

    copy_genes_from_bank(gene);

    int num_trig = (+g_cur_fractal_specific->flags >> 6) & 7;
    if (g_fractal_type == FractalType::FORMULA)
    {
        num_trig = g_max_function;
    }

choose_vars_restart:
    choices.reset();
    for (int num = MAX_PARAMS; num < (NUM_GENES - 5); num++)
    {
        choices.list(gene[num].name, 7, 7, evolve_modes, static_cast<int>(gene[num].mutate));
    }

    for (int num = (NUM_GENES - 5); num < (NUM_GENES - 5 + num_trig); num++)
    {
        choices.list(gene[num].name, 7, 7, evolve_modes, static_cast<int>(gene[num].mutate));
    }

    if (g_cur_fractal_specific->calc_type == standard_fractal_type &&
        bit_set(g_cur_fractal_specific->flags, FractalFlags::BAIL_TEST))
    {
        choices.list(gene[NUM_GENES - 1].name, 7, 7, evolve_modes, static_cast<int>(gene[NUM_GENES - 1].mutate));
    }

    choices.comment("");
    choices.comment("Press F2 to set all to off");
    choices.comment("Press F3 to set all on");
    choices.comment("Press F4 to randomize all");

    switch (int i = choices.prompt("Variable tweak central 2 of 2", 16 | 8 | 4); i)
    {
    case ID_KEY_F2: // set all off
        for (int num = MAX_PARAMS; num < NUM_GENES; num++)
        {
            gene[num].mutate = Variations::NONE;
        }
        goto choose_vars_restart;
    case ID_KEY_F3: // set all on...alternate x and y for field map
        for (int num = MAX_PARAMS; num < NUM_GENES; num ++)
        {
            gene[num].mutate = static_cast<Variations>((num % 2) + 1);
        }
        goto choose_vars_restart;
    case ID_KEY_F4: // Randomize all
        for (int num = MAX_PARAMS; num < NUM_GENES; num ++)
        {
            gene[num].mutate = static_cast<Variations>(std::rand() % static_cast<int>(Variations::NUM));
        }
        goto choose_vars_restart;
    case -1:
        return -1;
    default:
        break;
    }

    // read out values
    for (int num = MAX_PARAMS; num < (NUM_GENES - 5); num++)
    {
        gene[num].mutate = static_cast<Variations>(choices.read_list());
    }

    for (int num = (NUM_GENES - 5); num < (NUM_GENES - 5 + num_trig); num++)
    {
        gene[num].mutate = static_cast<Variations>(choices.read_list());
    }

    if (g_cur_fractal_specific->calc_type == standard_fractal_type &&
        bit_set(g_cur_fractal_specific->flags, FractalFlags::BAIL_TEST))
    {
        gene[NUM_GENES - 1].mutate = static_cast<Variations>(choices.read_list());
    }

    copy_genes_to_bank(gene);
    return 1; // if you were here, you want to regenerate
}

int get_variations()
{
    const char *evolve_modes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    ChoiceBuilder<20> choices;
    GeneBase gene[NUM_GENES];
    int first_param = 0;
    int last_param  = MAX_PARAMS;
    int changed = -1;

    copy_genes_from_bank(gene);

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

        if (g_frm_uses_p5)   // set last parameter
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

    int num_params = 0;
    for (int i = first_param; i < last_param; i++)
    {
        if (type_has_param(g_julibrot ? g_new_orbit_type : g_fractal_type, i) == 0)
        {
            if (g_fractal_type == FractalType::FORMULA)
            {
                if (param_not_used(i))
                {
                    continue;
                }
            }
            break;
        }
        num_params++;
    }

    if (g_fractal_type != FractalType::FORMULA)
    {
        last_param = num_params;
    }

choose_vars_restart:
    choices.reset();
    for (int num = first_param; num < last_param; num++)
    {
        if (g_fractal_type == FractalType::FORMULA)
        {
            if (param_not_used(num))
            {
                continue;
            }
        }
        choices.list(gene[num].name, 7, 7, evolve_modes, static_cast<int>(gene[num].mutate));
    }
    choices.comment("")
        .comment("Press F2 to set all to off")
        .comment("Press F3 to set all on")
        .comment("Press F4 to randomize all")
        .comment("Press F6 for second page"); // F5 gets eaten

    switch (int i = choices.prompt("Variable tweak central 1 of 2", 64 | 16 | 8 | 4); i)
    {
    case ID_KEY_F2: // set all off
        for (int num = 0; num < MAX_PARAMS; num++)
        {
            gene[num].mutate = Variations::NONE;
        }
        goto choose_vars_restart;
    case ID_KEY_F3: // set all on...alternate x and y for field map
        for (int num = 0; num < MAX_PARAMS; num ++)
        {
            gene[num].mutate = static_cast<Variations>((num % 2) + 1);
        }
        goto choose_vars_restart;
    case ID_KEY_F4: // Randomize all
        for (int num =0; num < MAX_PARAMS; num ++)
        {
            gene[num].mutate = static_cast<Variations>(std::rand() % static_cast<int>(Variations::NUM));
        }
        goto choose_vars_restart;
    case ID_KEY_F6: // go to second screen, put array away first
        copy_genes_to_bank(gene);
        changed = get_the_rest();
        copy_genes_from_bank(gene);
        goto choose_vars_restart;
    case -1:
        return changed;
    default:
        break;
    }

    // read out values
    for (int num = first_param; num < last_param; num++)
    {
        if (g_fractal_type == FractalType::FORMULA)
        {
            if (param_not_used(num))
            {
                continue;
            }
        }
        gene[num].mutate = static_cast<Variations>(choices.read_list());
    }

    copy_genes_to_bank(gene);
    return 1; // if you were here, you want to regenerate
}

void set_mutation_level(int strength)
{
    // scan through the gene array turning on random variation for all parms that
    // are suitable for this level of mutation
    for (GeneBase &elem : g_gene_bank)
    {
        if (elem.level <= strength)
        {
            elem.mutate = Variations::RANDOM;
        }
        else
        {
            elem.mutate = Variations::NONE;
        }
    }
}

int get_evolve_params()
{
    ChoiceBuilder<20> choices;
    int i;
    int old_variations = 0;

    // fill up the previous values arrays
    EvolutionModeFlags old_evolving = g_evolving;
    int old_image_grid_size = g_evolve_image_grid_size;
    double old_x_parameter_range = g_evolve_x_parameter_range;
    double old_y_parameter_range = g_evolve_y_parameter_range;
    double old_x_parameter_offset = g_evolve_x_parameter_offset;
    double old_y_parameter_offset = g_evolve_y_parameter_offset;
    double old_max_random_mutation = g_evolve_max_random_mutation;

get_evol_restart:

    if (bit_set(g_evolving, EvolutionModeFlags::RAND_WALK) ||
        bit_set(g_evolving, EvolutionModeFlags::RAND_PARAM))
    {
        // adjust field param to make some sense when changing from random modes
        // maybe should adjust for aspect ratio here?
        g_evolve_y_parameter_range = g_evolve_max_random_mutation * 2;
        g_evolve_x_parameter_range = g_evolve_max_random_mutation * 2;
        g_evolve_x_parameter_offset = g_params[0] - g_evolve_max_random_mutation;
        g_evolve_y_parameter_offset = g_params[1] - g_evolve_max_random_mutation;
        // set middle image to last selected and edges to +- evolve_max_random_mutation
    }

    choices.reset()
        .yes_no("Evolution mode? (no for full screen)", bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
        .int_number("Image grid size (odd numbers only)", g_evolve_image_grid_size);

    if (explore_check())
    {
        // test to see if any parms are set to linear
        // variation 'explore mode'
        choices.yes_no("Show parameter zoom box?", bit_set(g_evolving, EvolutionModeFlags::PARAM_BOX))
            .float_number("x parameter range (across screen)", g_evolve_x_parameter_range)
            .float_number("x parameter offset (left hand edge)", g_evolve_x_parameter_offset)
            .float_number("y parameter range (up screen)", g_evolve_y_parameter_range)
            .float_number("y parameter offset (lower edge)", g_evolve_y_parameter_offset);
    }

    choices.float_number("Max random mutation", g_evolve_max_random_mutation)
        .float_number("Mutation reduction factor (between generations)", g_evolve_mutation_reduction_factor)
        .yes_no("Grouting? ", !bit_set(g_evolving, EvolutionModeFlags::NO_GROUT))
        .comment("")
        .comment("Press F4 to reset view parameters to defaults.")
        .comment("Press F2 to halve mutation levels")
        .comment("Press F3 to double mutation levels")
        .comment("Press F6 to control which parameters are varied");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_EVOLVER};
        i = choices.prompt("Evolution Mode Options", 255);
    }
    if (i < 0)
    {
        // in case this point has been reached after calling sub menu with F6
        g_evolving = old_evolving;
        g_evolve_image_grid_size = old_image_grid_size;
        g_evolve_x_parameter_range = old_x_parameter_range;
        g_evolve_y_parameter_range = old_y_parameter_range;
        g_evolve_x_parameter_offset = old_x_parameter_offset;
        g_evolve_y_parameter_offset = old_y_parameter_offset;
        g_evolve_max_random_mutation = old_max_random_mutation;

        return -1;
    }

    if (i == ID_KEY_F4)
    {
        set_current_params();
        g_evolve_max_random_mutation = 1;
        g_evolve_mutation_reduction_factor = 1.0;
        goto get_evol_restart;
    }
    if (i == ID_KEY_F2)
    {
        g_evolve_x_parameter_range = g_evolve_x_parameter_range / 2;
        g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset + g_evolve_x_parameter_range /2;
        g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range / 2;
        g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
        g_evolve_max_random_mutation = g_evolve_max_random_mutation / 2;
        goto get_evol_restart;
    }
    if (i == ID_KEY_F3)
    {
        double center_x = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
        g_evolve_x_parameter_range = g_evolve_x_parameter_range * 2;
        g_evolve_new_x_parameter_offset = center_x - g_evolve_x_parameter_range / 2;
        g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
        double center_y = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range * 2;
        g_evolve_new_y_parameter_offset = center_y - g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
        g_evolve_max_random_mutation = g_evolve_max_random_mutation * 2;
        goto get_evol_restart;
    }

    int j = i;

    // now check out the results
    g_evolving = choices.read_yes_no() ? EvolutionModeFlags::FIELD_MAP : EvolutionModeFlags::NONE;
    g_view_window = g_evolving != EvolutionModeFlags::NONE;

    if (g_evolving == EvolutionModeFlags::NONE && i != ID_KEY_F6)    // don't need any of the other parameters
    {
        return 1;             // the following code can set evolving even if it's off
    }

    g_evolve_image_grid_size = choices.read_int_number();
    int tmp = g_screen_x_dots / (MIN_PIXELS << 1);
    // (sxdots / 20), max # of subimages @ 20 pixels per subimage
    // EVOLVE_MAX_GRID_SIZE == 1024 / 20 == 51
    g_evolve_image_grid_size = std::min<int>(g_evolve_image_grid_size, EVOLVE_MAX_GRID_SIZE);
    g_evolve_image_grid_size = std::min(g_evolve_image_grid_size, tmp);
    g_evolve_image_grid_size = std::max(g_evolve_image_grid_size, 3);
    g_evolve_image_grid_size |= 1; // make sure evolve_image_grid_size is odd
    if (explore_check())
    {
        g_evolving |= choices.read_yes_no() ? EvolutionModeFlags::PARAM_BOX : EvolutionModeFlags::NONE;
        g_evolve_x_parameter_range = choices.read_float_number();
        g_evolve_x_parameter_offset = choices.read_float_number();
        g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset;
        g_evolve_y_parameter_range = choices.read_float_number();
        g_evolve_y_parameter_offset = choices.read_float_number();
        g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset;
    }

    g_evolve_max_random_mutation = choices.read_float_number();
    g_evolve_mutation_reduction_factor = choices.read_float_number();
    g_evolving |= choices.read_yes_no() ? EvolutionModeFlags::NONE : EvolutionModeFlags::NO_GROUT;
    g_view_x_dots = (g_screen_x_dots / g_evolve_image_grid_size)-2;
    g_view_y_dots = (g_screen_y_dots / g_evolve_image_grid_size)-2;
    if (!g_view_window)
    {
        g_view_y_dots = 0;
        g_view_x_dots = 0;
    }

    i = 0;

    if (g_evolving != old_evolving
        || (g_evolve_image_grid_size != old_image_grid_size)
        || (g_evolve_x_parameter_range != old_x_parameter_range)
        || (g_evolve_x_parameter_offset != old_x_parameter_offset)
        || (g_evolve_y_parameter_range != old_y_parameter_range)
        || (g_evolve_y_parameter_offset != old_y_parameter_offset)
        || (g_evolve_max_random_mutation != old_max_random_mutation)
        || (old_variations > 0))
    {
        i = 1;
    }

    if (g_evolving != EvolutionModeFlags::NONE && old_evolving == EvolutionModeFlags::NONE)
    {
        save_param_history();
    }

    if (g_evolving == EvolutionModeFlags::NONE && g_evolving == old_evolving)
    {
        i = 0;
    }

    if (j == ID_KEY_F6)
    {
        old_variations = get_variations();
        set_current_params();
        if (old_variations > 0)
        {
            g_view_window = true;
            g_evolving |= EvolutionModeFlags::FIELD_MAP;   // leave other settings alone
        }
        g_evolve_max_random_mutation = 1;
        g_evolve_mutation_reduction_factor = 1.0;
        goto get_evol_restart;
    }
    return i;
}

void setup_param_box()
{
    g_evolve_param_box_count = 0;
    g_evolve_param_zoom = ((double) g_evolve_image_grid_size -1.0)/2.0;
    // need to allocate 2 int arrays for g_box_x and g_box_y plus 1 byte array for values
    const int num_box_values = (g_logical_screen_x_dots + g_logical_screen_y_dots) * 2;
    const int num_values = g_logical_screen_x_dots + g_logical_screen_y_dots + 2;

    s_param_box_x.resize(num_box_values);
    s_param_box_y.resize(num_box_values);
    s_param_box_values.resize(num_values);

    s_image_box_x.resize(num_box_values);
    s_image_box_y.resize(num_box_values);
    s_image_box_values.resize(num_values);
}

void release_param_box()
{
    s_param_box_x.clear();
    s_param_box_y.clear();
    s_param_box_values.clear();
    s_image_box_x.clear();
    s_image_box_y.clear();
    s_image_box_values.clear();
}

void set_current_params()
{
    g_evolve_x_parameter_range = g_cur_fractal_specific->x_max - g_cur_fractal_specific->x_min;
    g_evolve_new_x_parameter_offset = - (g_evolve_x_parameter_range / 2);
    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
    g_evolve_y_parameter_range = g_cur_fractal_specific->y_max - g_cur_fractal_specific->y_min;
    g_evolve_new_y_parameter_offset = - (g_evolve_y_parameter_range / 2);
    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
}

void fiddle_params(GeneBase gene[], int count)
{
    // call with px, py ... parameter set co-ords
    // set random seed then call rnd enough times to get to px, py

    // when writing routines to vary param types make sure that rand() gets called
    // the same number of times whether gene[].mutate is set or not to allow
    // user to change it between generations without screwing up the duplicability
    // of the sequence and starting from the wrong point

    // this function has got simpler and simpler throughout the construction of the
    // evolver feature and now consists of just these few lines to loop through all
    // the variables referenced in the gene array and call the functions required
    // to vary them, aren't pointers marvellous!

    // return if middle image
    if ((g_evolve_param_grid_x == g_evolve_image_grid_size / 2)
        && (g_evolve_param_grid_y == g_evolve_image_grid_size / 2))
    {
        return;
    }

    set_random(count);   // generate the right number of pseudo randoms

    for (int i = 0; i < NUM_GENES; i++)
    {
        (*(gene[i].vary_fn))(gene, std::rand(), i);
    }
}

static void set_random(int count)
{
    // This must be called with ecount set correctly for the spiral map.
    // Call this routine to set the random # to the proper value
    // if it may have changed, before fiddleparms() is called.
    // Now called by fiddleparms().
    std::srand(g_evolve_this_generation_random_seed);
    for (int index = 0; index < count; index++)
    {
        for (int i = 0; i < NUM_GENES; i++)
        {
            std::rand();
        }
    }
}

static bool explore_check()
{
    // checks through gene array to see if any of the parameters are set to
    // one of the non-random variation modes. Used to see if parmzoom box is
    // needed
    return std::any_of(std::begin(g_gene_bank), std::end(g_gene_bank), [](const GeneBase &gene)
        { return (gene.mutate != Variations::NONE) && (gene.mutate < Variations::RANDOM); });
}

void draw_param_box(int mode)
{
    // draws parameter zoom box in evolver mode
    // clears boxes off-screen if mode = 1, otherwise, redraws boxes
    Coord tl;
    Coord tr;
    Coord bl;
    Coord br;
    if (!bit_set(g_evolving, EvolutionModeFlags::PARAM_BOX))
    {
        return; // don't draw if not asked!
    }
    const int grout = bit_set(g_evolving, EvolutionModeFlags::NO_GROUT) ? 0 : 1;
    s_image_box_count = g_box_count;
    if (g_box_count)
    {
        // stash normal zoombox pixels
        std::copy(&g_box_x[0], &g_box_x[g_box_count*2], s_image_box_x.data());
        std::copy(&g_box_y[0], &g_box_y[g_box_count*2], s_image_box_y.data());
        std::copy(&g_box_values[0], &g_box_values[g_box_count], s_image_box_values.data());
        clear_box(); // to avoid probs when one box overlaps the other
    }
    if (g_evolve_param_box_count != 0)
    {
        // clear last parmbox
        g_box_count = g_evolve_param_box_count;
        std::copy(s_param_box_x.data(), &s_param_box_x[g_box_count*2], &g_box_x[0]);
        std::copy(s_param_box_y.data(), &s_param_box_y[g_box_count*2], &g_box_y[0]);
        std::copy(s_param_box_values.data(), &s_param_box_values[g_box_count], &g_box_values[0]);
        clear_box();
    }

    if (mode == 1)
    {
        g_box_count = s_image_box_count;
        g_evolve_param_box_count = 0;
        return;
    }

    //draw larger box to show parm zooming range
    bl.x = ((g_evolve_param_grid_x -(int)g_evolve_param_zoom) * (int)(g_logical_screen_x_size_dots+1+grout))-g_logical_screen_x_offset-1;
    tl.x = bl.x;
    tr.y = ((g_evolve_param_grid_y -(int)g_evolve_param_zoom) * (int)(g_logical_screen_y_size_dots+1+grout))-g_logical_screen_y_offset-1;
    tl.y = tr.y;
    tr.x = ((g_evolve_param_grid_x +1+(int)g_evolve_param_zoom) * (int)(g_logical_screen_x_size_dots+1+grout))-g_logical_screen_x_offset;
    br.x = tr.x;
    bl.y = ((g_evolve_param_grid_y +1+(int)g_evolve_param_zoom) * (int)(g_logical_screen_y_size_dots+1+grout))-g_logical_screen_y_offset;
    br.y = bl.y;
    g_box_count = 0;
    add_box(br);
    add_box(tr);
    add_box(bl);
    add_box(tl);
    draw_lines(tl, tr, bl.x-tl.x, bl.y-tl.y);
    draw_lines(tl, bl, tr.x-tl.x, tr.y-tl.y);
    if (g_box_count)
    {
        display_box();
        // stash pixel values for later
        std::copy(&g_box_x[0], &g_box_x[g_box_count*2], s_param_box_x.data());
        std::copy(&g_box_y[0], &g_box_y[g_box_count*2], s_param_box_y.data());
        std::copy(&g_box_values[0], &g_box_values[g_box_count], s_param_box_values.data());
    }
    g_evolve_param_box_count = g_box_count;
    g_box_count = s_image_box_count;
    if (s_image_box_count)
    {
        // and move back old values so that everything can proceed as normal
        std::copy(s_image_box_x.data(), &s_image_box_x[g_box_count*2], &g_box_x[0]);
        std::copy(s_image_box_y.data(), &s_image_box_y[g_box_count*2], &g_box_y[0]);
        std::copy(s_image_box_values.data(), &s_image_box_values[g_box_count], &g_box_values[0]);
        display_box();
    }
}

void set_evolve_ranges()
{
    int delta_y = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    // set up ranges and offsets for parameter explorer/evolver
    g_evolve_x_parameter_range = g_evolve_dist_per_x*(g_evolve_param_zoom*2.0);
    g_evolve_y_parameter_range = g_evolve_dist_per_y*(g_evolve_param_zoom*2.0);
    g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset +(((double)g_evolve_param_grid_x-g_evolve_param_zoom)*g_evolve_dist_per_x);
    g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset +(((double)delta_y-g_evolve_param_zoom)*g_evolve_dist_per_y);

    g_evolve_new_discrete_x_parameter_offset = (char)(g_evolve_discrete_x_parameter_offset +(g_evolve_param_grid_x- g_evolve_image_grid_size /2));
    g_evolve_new_discrete_y_parameter_offset = (char)(g_evolve_discrete_y_parameter_offset +(delta_y- g_evolve_image_grid_size /2));
}

void spiral_map(int count)
{
    // maps out a clockwise spiral for a prettier and possibly
    // more intuitively useful order of drawing the sub images.
    // All the malarky with count is to allow resuming
    int i = 0;
    int mid = g_evolve_image_grid_size / 2;
    if (count == 0)
    {
        // start in the middle
        g_evolve_param_grid_y = mid;
        g_evolve_param_grid_x = g_evolve_param_grid_y;
        return;
    }
    for (int offset = 1; offset <= mid; offset ++)
    {
        // first do the top row
        g_evolve_param_grid_y = (mid - offset);
        for (g_evolve_param_grid_x = (mid - offset)+1; g_evolve_param_grid_x < mid+offset; g_evolve_param_grid_x++)
        {
            i++;
            if (i == count)
            {
                return;
            }
        }
        // then do the right hand column
        for (; g_evolve_param_grid_y < mid + offset; g_evolve_param_grid_y++)
        {
            i++;
            if (i == count)
            {
                return;
            }
        }
        // then reverse along the bottom row
        for (; g_evolve_param_grid_x > mid - offset; g_evolve_param_grid_x--)
        {
            i++;
            if (i == count)
            {
                return;
            }
        }
        // then up the left to finish
        for (; g_evolve_param_grid_y >= mid - offset; g_evolve_param_grid_y--)
        {
            i++;
            if (i == count)
            {
                return;
            }
        }
    }
}

int unspiral_map()
{
    // unmaps the clockwise spiral
    // All this malarky is to allow selecting different subimages
    // Returns the count from the center subimage to the current px & py
    static int old_image_grid_size = 0;

    int mid = g_evolve_image_grid_size / 2;
    if ((g_evolve_param_grid_x == mid && g_evolve_param_grid_y == mid) || (old_image_grid_size != g_evolve_image_grid_size))
    {
        // set up array and return
        int grid_sqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
        s_evol_count_box[g_evolve_param_grid_x][g_evolve_param_grid_y] = 0;  // we know the first one, do the rest
        for (int i = 1; i < grid_sqr; i++)
        {
            spiral_map(i);
            s_evol_count_box[g_evolve_param_grid_x][g_evolve_param_grid_y] = i;
        }
        old_image_grid_size = g_evolve_image_grid_size;
        g_evolve_param_grid_y = mid;
        g_evolve_param_grid_x = g_evolve_param_grid_y;
        return 0;
    }
    return s_evol_count_box[g_evolve_param_grid_x][g_evolve_param_grid_y];
}
