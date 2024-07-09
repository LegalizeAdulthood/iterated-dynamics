#include "evolve.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "choice_builder.h"
#include "cmdfiles.h"
#include "fractalp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "jb.h"
#include "param_not_used.h"
#include "parser.h"
#include "pixel_limits.h"
#include "port.h"
#include "sqr.h"
#include "trig_fns.h"
#include "type_has_param.h"
#include "value_saver.h"
#include "zoom.h"

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
struct PARAMHIST
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
    BYTE trigndx0;
    BYTE trigndx1;
    BYTE trigndx2;
    BYTE trigndx3;
    bailouts bailoutest;
};

GENEBASE g_gene_bank[NUM_GENES];

// px and py are coordinates in the parameter grid (small images on screen)
// evolving = flag, evolve_image_grid_size = dimensions of image grid (evolve_image_grid_size x evolve_image_grid_size)
int g_evolve_param_grid_x;
int g_evolve_param_grid_y;
evolution_mode_flags g_evolving{evolution_mode_flags::NONE};
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

// offset for discrete parameters x and y..
// used for things like inside or outside types, bailout tests, trig fn etc
char g_evolve_discrete_x_parameter_offset;
char g_evolve_discrete_y_parameter_offset;
char g_evolve_new_discrete_x_parameter_offset;
char g_evolve_new_discrete_y_parameter_offset;

int g_evolve_param_box_count;

static int s_ecount_box[EVOLVE_MAX_GRID_SIZE][EVOLVE_MAX_GRID_SIZE];
static std::vector<int> s_param_box_x;
static std::vector<int> s_param_box_y;
static std::vector<int> s_param_box_values;
static int s_image_box_count;
static std::vector<int> s_image_box_x;
static std::vector<int> s_image_box_y;
static std::vector<int> s_image_box_values;
static PARAMHIST s_old_history{};

void varydbl(GENEBASE gene[], int randval, int i);
int varyint(int randvalue, int limit, int mode);
int wrapped_positive_varyint(int randvalue, int limit, int mode);
void varyinside(GENEBASE gene[], int randval, int i);
void varyoutside(GENEBASE gene[], int randval, int i);
void varypwr2(GENEBASE gene[], int randval, int i);
void varytrig(GENEBASE gene[], int randval, int i);
void varybotest(GENEBASE gene[], int randval, int i);
void varyinv(GENEBASE gene[], int randval, int i);
static bool explore_check();
void spiralmap(int);
static void set_random(int);
void set_mutation_level(int);
void SetupParamBox();
void ReleaseParamBox();

void copy_genes_from_bank(GENEBASE gene[NUM_GENES])
{
    std::copy(&g_gene_bank[0], &g_gene_bank[NUM_GENES], &gene[0]);
}

template <int N>
bool equal(const std::int16_t (&lhs)[N], const std::int16_t (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

inline bool within_eps(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-6f;
}

bool operator==(const EVOLUTION_INFO &lhs, const EVOLUTION_INFO &rhs)
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
        && lhs.discrete_y_paramter_offset == rhs.discrete_y_paramter_offset   //
        && lhs.px == rhs.px                                                   //
        && lhs.py == rhs.py                                                   //
        && lhs.sxoffs == rhs.sxoffs                                           //
        && lhs.syoffs == rhs.syoffs                                           //
        && lhs.xdots == rhs.xdots                                             //
        && lhs.ydots == rhs.ydots                                             //
        && equal(lhs.mutate, rhs.mutate)                                      //
        && lhs.ecount == rhs.ecount;                                          //
}

void copy_genes_to_bank(GENEBASE const gene[NUM_GENES])
{
    // cppcheck-suppress arrayIndexOutOfBounds
    std::copy(&gene[0], &gene[NUM_GENES], &g_gene_bank[0]);
}

// set up pointers and mutation params for all usable image
// control variables... revise as necessary when
// new vars come along... don't forget to increment NUM_GENES
// as well
void initgene()
{
    //                        Use only 15 letters below: 123456789012345
    GENEBASE gene[NUM_GENES] =
    {
        { &g_params[0], varydbl, variations::RANDOM,       "Param 1 real", 1 },
        { &g_params[1], varydbl, variations::RANDOM,       "Param 1 imag", 1 },
        { &g_params[2], varydbl, variations::NONE,         "Param 2 real", 1 },
        { &g_params[3], varydbl, variations::NONE,         "Param 2 imag", 1 },
        { &g_params[4], varydbl, variations::NONE,         "Param 3 real", 1 },
        { &g_params[5], varydbl, variations::NONE,         "Param 3 imag", 1 },
        { &g_params[6], varydbl, variations::NONE,         "Param 4 real", 1 },
        { &g_params[7], varydbl, variations::NONE,         "Param 4 imag", 1 },
        { &g_params[8], varydbl, variations::NONE,         "Param 5 real", 1 },
        { &g_params[9], varydbl, variations::NONE,         "Param 5 imag", 1 },
        { &g_inside_color, varyinside, variations::NONE,        "inside color", 2 },
        { &g_outside_color, varyoutside, variations::NONE,      "outside color", 3 },
        { &g_decomp[0], varypwr2, variations::NONE,       "decomposition", 4 },
        { &g_inversion[0], varyinv, variations::NONE,     "invert radius", 7 },
        { &g_inversion[1], varyinv, variations::NONE,     "invert center x", 7 },
        { &g_inversion[2], varyinv, variations::NONE,     "invert center y", 7 },
        { &g_trig_index[0], varytrig, variations::NONE,      "trig function 1", 5 },
        { &g_trig_index[1], varytrig, variations::NONE,      "trig fn 2", 5 },
        { &g_trig_index[2], varytrig, variations::NONE,      "trig fn 3", 5 },
        { &g_trig_index[3], varytrig, variations::NONE,      "trig fn 4", 5 },
        { &g_bail_out_test, varybotest, variations::NONE,    "bailout test", 6 }
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
    s_old_history.trigndx0 = static_cast<BYTE>(g_trig_index[0]);
    s_old_history.trigndx1 = static_cast<BYTE>(g_trig_index[1]);
    s_old_history.trigndx2 = static_cast<BYTE>(g_trig_index[2]);
    s_old_history.trigndx3 = static_cast<BYTE>(g_trig_index[3]);
    s_old_history.bailoutest = g_bail_out_test;
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
    g_trig_index[0] = static_cast<trig_fn>(s_old_history.trigndx0);
    g_trig_index[1] = static_cast<trig_fn>(s_old_history.trigndx1);
    g_trig_index[2] = static_cast<trig_fn>(s_old_history.trigndx2);
    g_trig_index[3] = static_cast<trig_fn>(s_old_history.trigndx3);
    g_bail_out_test = static_cast<bailouts>(s_old_history.bailoutest);
}

// routine to vary doubles
void varydbl(GENEBASE gene[], int randval, int i)
{
    int lclpy = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    switch (gene[i].mutate)
    {
    default:
    case variations::NONE:
        break;
    case variations::X:
        *(double *)gene[i].addr = g_evolve_param_grid_x * g_evolve_dist_per_x + g_evolve_x_parameter_offset; //paramspace x coord * per view delta px + offset
        break;
    case variations::Y:
        *(double *)gene[i].addr = lclpy * g_evolve_dist_per_y + g_evolve_y_parameter_offset; //same for y
        break;
    case variations::X_PLUS_Y:
        *(double *)gene[i].addr = g_evolve_param_grid_x*g_evolve_dist_per_x+ g_evolve_x_parameter_offset +(lclpy*g_evolve_dist_per_y)+ g_evolve_y_parameter_offset; //and x+y
        break;
    case variations::X_MINUS_Y:
        *(double *)gene[i].addr = (g_evolve_param_grid_x*g_evolve_dist_per_x+ g_evolve_x_parameter_offset)-(lclpy*g_evolve_dist_per_y+ g_evolve_y_parameter_offset); //and x-y
        break;
    case variations::RANDOM:
        *(double *)gene[i].addr += (((double)randval / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation;
        break;
    case variations::WEIGHTED_RANDOM:
    {
        int mid = g_evolve_image_grid_size /2;
        double radius =  std::sqrt(static_cast<double>(sqr(g_evolve_param_grid_x - mid) + sqr(lclpy - mid)));
        *(double *)gene[i].addr += ((((double)randval / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation) * radius;
    }
    break;
    }
}

int varyint(int randvalue, int limit, variations mode)
{
    int ret = 0;
    int lclpy = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    switch (mode)
    {
    default:
    case variations::NONE:
        break;
    case variations::X:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x)%limit;
        break;
    case variations::Y:
        ret = (g_evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::X_PLUS_Y:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x+ g_evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::X_MINUS_Y:
        ret = (g_evolve_discrete_x_parameter_offset +g_evolve_param_grid_x)-(g_evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::RANDOM:
        ret = randvalue % limit;
        break;
    case variations::WEIGHTED_RANDOM:
    {
        int mid = g_evolve_image_grid_size /2;
        double radius =  std::sqrt(static_cast<double>(sqr(g_evolve_param_grid_x - mid) + sqr(lclpy - mid)));
        ret = (int)((((randvalue / RAND_MAX) * 2 * g_evolve_max_random_mutation) - g_evolve_max_random_mutation) * radius);
        ret %= limit;
        break;
    }
    }
    return ret;
}

int wrapped_positive_varyint(int randvalue, int limit, variations mode)
{
    int i;
    i = varyint(randvalue, limit, mode);
    if (i < 0)
    {
        return limit + i;
    }
    else
    {
        return i;
    }
}

void varyinside(GENEBASE gene[], int randval, int i)
{
    int choices[9] = { ZMAG, BOF60, BOF61, EPSCROSS, STARTRAIL, PERIOD, FMODI, ATANI, ITER };
    if (gene[i].mutate != variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 9, gene[i].mutate)];
    }
}

void varyoutside(GENEBASE gene[], int randval, int i)
{
    int choices[8] = { ITER, REAL, IMAG, MULT, SUM, ATAN, FMOD, TDIS };
    if (gene[i].mutate != variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 8, gene[i].mutate)];
    }
}

void varybotest(GENEBASE gene[], int randval, int i)
{
    int choices[7] =
    {
        static_cast<int>(bailouts::Mod),
        static_cast<int>(bailouts::Real),
        static_cast<int>(bailouts::Imag),
        static_cast<int>(bailouts::Or),
        static_cast<int>(bailouts::And),
        static_cast<int>(bailouts::Manh),
        static_cast<int>(bailouts::Manr)
    };
    if (gene[i].mutate != variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 7, gene[i].mutate)];
        // move this next bit to varybot where it belongs
        set_bailout_formula(g_bail_out_test);
    }
}

void varypwr2(GENEBASE gene[], int randval, int i)
{
    int choices[9] = {0, 2, 4, 8, 16, 32, 64, 128, 256};
    if (gene[i].mutate != variations::NONE)
    {
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 9, gene[i].mutate)];
    }
}

void varytrig(GENEBASE gene[], int randval, int i)
{
    if (gene[i].mutate != variations::NONE)
    {
        *static_cast<trig_fn *>(gene[i].addr) =
            static_cast<trig_fn>(wrapped_positive_varyint(randval, g_num_trig_functions, gene[i].mutate));
    }
    set_trig_pointers(5); //set all trig ptrs up
}

void varyinv(GENEBASE gene[], int randval, int i)
{
    if (gene[i].mutate != variations::NONE)
    {
        varydbl(gene, randval, i);
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
int get_the_rest()
{
    ChoiceBuilder<20> choices;
    char const *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    int i, numtrig;
    GENEBASE gene[NUM_GENES];

    copy_genes_from_bank(gene);

    numtrig = (+g_cur_fractal_specific->flags >> 6) & 7;
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        numtrig = g_max_function;
    }

choose_vars_restart:
    choices.reset();
    for (int num = MAX_PARAMS; num < (NUM_GENES - 5); num++)
    {
        choices.list(gene[num].name, 7, 7, evolvmodes, static_cast<int>(gene[num].mutate));
    }

    for (int num = (NUM_GENES - 5); num < (NUM_GENES - 5 + numtrig); num++)
    {
        choices.list(gene[num].name, 7, 7, evolvmodes, static_cast<int>(gene[num].mutate));
    }

    if (g_cur_fractal_specific->calctype == standard_fractal &&
        bit_set(g_cur_fractal_specific->flags, fractal_flags::BAILTEST))
    {
        choices.list(gene[NUM_GENES - 1].name, 7, 7, evolvmodes, static_cast<int>(gene[NUM_GENES - 1].mutate));
    }

    choices.comment("");
    choices.comment("Press F2 to set all to off");
    choices.comment("Press F3 to set all on");
    choices.comment("Press F4 to randomize all");

    i = choices.prompt("Variable tweak central 2 of 2", 16 | 8 | 4);

    switch (i)
    {
    case ID_KEY_F2: // set all off
        for (int num = MAX_PARAMS; num < NUM_GENES; num++)
        {
            gene[num].mutate = variations::NONE;
        }
        goto choose_vars_restart;
    case ID_KEY_F3: // set all on..alternate x and y for field map
        for (int num = MAX_PARAMS; num < NUM_GENES; num ++)
        {
            gene[num].mutate = static_cast<variations>((num % 2) + 1);
        }
        goto choose_vars_restart;
    case ID_KEY_F4: // Randomize all
        for (int num = MAX_PARAMS; num < NUM_GENES; num ++)
        {
            gene[num].mutate = static_cast<variations>(std::rand() % static_cast<int>(variations::NUM));
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
        gene[num].mutate = static_cast<variations>(choices.read_list());
    }

    for (int num = (NUM_GENES - 5); num < (NUM_GENES - 5 + numtrig); num++)
    {
        gene[num].mutate = static_cast<variations>(choices.read_list());
    }

    if (g_cur_fractal_specific->calctype == standard_fractal &&
        bit_set(g_cur_fractal_specific->flags, fractal_flags::BAILTEST))
    {
        gene[NUM_GENES - 1].mutate = static_cast<variations>(choices.read_list());
    }

    copy_genes_to_bank(gene);
    return 1; // if you were here, you want to regenerate
}

int get_variations()
{
    char const *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    int k, numparams;
    ChoiceBuilder<20> choices;
    GENEBASE gene[NUM_GENES];
    int firstparm = 0;
    int lastparm  = MAX_PARAMS;
    int chngd = -1;

    copy_genes_from_bank(gene);

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

        if (g_frm_uses_p5)   // set last parameter
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

    numparams = 0;
    for (int i = firstparm; i < lastparm; i++)
    {
        if (typehasparm(g_julibrot ? g_new_orbit_type : g_fractal_type, i, nullptr) == 0)
        {
            if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
            {
                if (paramnotused(i))
                {
                    continue;
                }
            }
            break;
        }
        numparams++;
    }

    if (g_fractal_type != fractal_type::FORMULA && g_fractal_type != fractal_type::FFORMULA)
    {
        lastparm = numparams;
    }

choose_vars_restart:
    choices.reset();
    for (int num = firstparm; num < lastparm; num++)
    {
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            if (paramnotused(num))
            {
                continue;
            }
        }
        choices.list(gene[num].name, 7, 7, evolvmodes, static_cast<int>(gene[num].mutate));
    }
    choices.comment("")
        .comment("Press F2 to set all to off")
        .comment("Press F3 to set all on")
        .comment("Press F4 to randomize all")
        .comment("Press F6 for second page"); // F5 gets eaten

    int i = choices.prompt("Variable tweak central 1 of 2", 64 | 16 | 8 | 4);

    switch (i)
    {
    case ID_KEY_F2: // set all off
        for (int num = 0; num < MAX_PARAMS; num++)
        {
            gene[num].mutate = variations::NONE;
        }
        goto choose_vars_restart;
    case ID_KEY_F3: // set all on..alternate x and y for field map
        for (int num = 0; num < MAX_PARAMS; num ++)
        {
            gene[num].mutate = static_cast<variations>((num % 2) + 1);
        }
        goto choose_vars_restart;
    case ID_KEY_F4: // Randomize all
        for (int num =0; num < MAX_PARAMS; num ++)
        {
            gene[num].mutate = static_cast<variations>(std::rand() % static_cast<int>(variations::NUM));
        }
        goto choose_vars_restart;
    case ID_KEY_F6: // go to second screen, put array away first
        copy_genes_to_bank(gene);
        chngd = get_the_rest();
        copy_genes_from_bank(gene);
        goto choose_vars_restart;
    case -1:
        return chngd;
    default:
        break;
    }

    // read out values
    for (int num = firstparm; num < lastparm; num++)
    {
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            if (paramnotused(num))
            {
                continue;
            }
        }
        gene[num].mutate = static_cast<variations>(choices.read_list());
    }

    copy_genes_to_bank(gene);
    return 1; // if you were here, you want to regenerate
}

void set_mutation_level(int strength)
{
    // scan through the gene array turning on random variation for all parms that
    // are suitable for this level of mutation
    for (auto &elem : g_gene_bank)
    {
        if (elem.level <= strength)
        {
            elem.mutate = variations::RANDOM;
        }
        else
        {
            elem.mutate = variations::NONE;
        }
    }
}

int get_evolve_Parms()
{
    ChoiceBuilder<20> choices;
    int i, j, tmp;
    int old_image_grid_size;
    int old_variations = 0;
    double old_x_parameter_range, old_y_parameter_range, old_x_parameter_offset, old_y_parameter_offset, old_max_random_mutation;

    // fill up the previous values arrays
    evolution_mode_flags old_evolving = g_evolving;
    old_image_grid_size = g_evolve_image_grid_size;
    old_x_parameter_range = g_evolve_x_parameter_range;
    old_y_parameter_range = g_evolve_y_parameter_range;
    old_x_parameter_offset = g_evolve_x_parameter_offset;
    old_y_parameter_offset = g_evolve_y_parameter_offset;
    old_max_random_mutation = g_evolve_max_random_mutation;

get_evol_restart:

    if (bit_set(g_evolving, evolution_mode_flags::RANDWALK) ||
        bit_set(g_evolving, evolution_mode_flags::RANDPARAM))
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
        .yes_no("Evolution mode? (no for full screen)", bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
        .int_number("Image grid size (odd numbers only)", g_evolve_image_grid_size);

    if (explore_check())
    {
        // test to see if any parms are set to linear
        // variation 'explore mode'
        choices.yes_no("Show parameter zoom box?", bit_set(g_evolving, evolution_mode_flags::PARMBOX))
            .float_number("x parameter range (across screen)", g_evolve_x_parameter_range)
            .float_number("x parameter offset (left hand edge)", g_evolve_x_parameter_offset)
            .float_number("y parameter range (up screen)", g_evolve_y_parameter_range)
            .float_number("y parameter offset (lower edge)", g_evolve_y_parameter_offset);
    }

    choices.float_number("Max random mutation", g_evolve_max_random_mutation)
        .float_number("Mutation reduction factor (between generations)", g_evolve_mutation_reduction_factor)
        .yes_no("Grouting? ", !bit_set(g_evolving, evolution_mode_flags::NOGROUT))
        .comment("")
        .comment("Press F4 to reset view parameters to defaults.")
        .comment("Press F2 to halve mutation levels")
        .comment("Press F3 to double mutation levels")
        .comment("Press F6 to control which parameters are varied");

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_EVOL};
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
        double centerx, centery;
        centerx = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
        g_evolve_x_parameter_range = g_evolve_x_parameter_range * 2;
        g_evolve_new_x_parameter_offset = centerx - g_evolve_x_parameter_range / 2;
        g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
        centery = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range * 2;
        g_evolve_new_y_parameter_offset = centery - g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
        g_evolve_max_random_mutation = g_evolve_max_random_mutation * 2;
        goto get_evol_restart;
    }

    j = i;

    // now check out the results
    g_evolving = choices.read_yes_no() ? evolution_mode_flags::FIELDMAP : evolution_mode_flags::NONE;
    g_view_window = g_evolving != evolution_mode_flags::NONE;

    if (g_evolving == evolution_mode_flags::NONE && i != ID_KEY_F6)    // don't need any of the other parameters
    {
        return 1;             // the following code can set evolving even if it's off
    }

    g_evolve_image_grid_size = choices.read_int_number();
    tmp = g_screen_x_dots / (MIN_PIXELS << 1);
    // (sxdots / 20), max # of subimages @ 20 pixels per subimage
    // EVOLVE_MAX_GRID_SIZE == 1024 / 20 == 51
    if (g_evolve_image_grid_size > EVOLVE_MAX_GRID_SIZE)
    {
        g_evolve_image_grid_size = EVOLVE_MAX_GRID_SIZE;
    }
    if (g_evolve_image_grid_size > tmp)
    {
        g_evolve_image_grid_size = tmp;
    }
    if (g_evolve_image_grid_size < 3)
    {
        g_evolve_image_grid_size = 3;
    }
    g_evolve_image_grid_size |= 1; // make sure evolve_image_grid_size is odd
    if (explore_check())
    {
        g_evolving |= choices.read_yes_no() ? evolution_mode_flags::PARMBOX : evolution_mode_flags::NONE;
        g_evolve_x_parameter_range = choices.read_float_number();
        g_evolve_x_parameter_offset = choices.read_float_number();
        g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset;
        g_evolve_y_parameter_range = choices.read_float_number();
        g_evolve_y_parameter_offset = choices.read_float_number();
        g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset;
    }

    g_evolve_max_random_mutation = choices.read_float_number();
    g_evolve_mutation_reduction_factor = choices.read_float_number();
    g_evolving |= choices.read_yes_no() ? evolution_mode_flags::NONE : evolution_mode_flags::NOGROUT;
    g_view_x_dots = (g_screen_x_dots / g_evolve_image_grid_size)-2;
    g_view_y_dots = (g_screen_y_dots / g_evolve_image_grid_size)-2;
    if (!g_view_window)
    {
        g_view_y_dots = 0;
        g_view_x_dots = g_view_y_dots;
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

    if (g_evolving != evolution_mode_flags::NONE && old_evolving == evolution_mode_flags::NONE)
    {
        save_param_history();
    }

    if (g_evolving == evolution_mode_flags::NONE && g_evolving == old_evolving)
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
            g_evolving |= evolution_mode_flags::FIELDMAP;   // leave other settings alone
        }
        g_evolve_max_random_mutation = 1;
        g_evolve_mutation_reduction_factor = 1.0;
        goto get_evol_restart;
    }
    return i;
}

void SetupParamBox()
{
    g_evolve_param_box_count = 0;
    g_evolve_param_zoom = ((double) g_evolve_image_grid_size -1.0)/2.0;
    // need to allocate 2 int arrays for g_box_x and g_box_y plus 1 byte array for values
    int const num_box_values = (g_logical_screen_x_dots + g_logical_screen_y_dots)*2;
    int const num_values = g_logical_screen_x_dots + g_logical_screen_y_dots + 2;

    s_param_box_x.resize(num_box_values);
    s_param_box_y.resize(num_box_values);
    s_param_box_values.resize(num_values);

    s_image_box_x.resize(num_box_values);
    s_image_box_y.resize(num_box_values);
    s_image_box_values.resize(num_values);
}

void ReleaseParamBox()
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
    g_evolve_x_parameter_range = g_cur_fractal_specific->xmax - g_cur_fractal_specific->xmin;
    g_evolve_new_x_parameter_offset = - (g_evolve_x_parameter_range / 2);
    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
    g_evolve_y_parameter_range = g_cur_fractal_specific->ymax - g_cur_fractal_specific->ymin;
    g_evolve_new_y_parameter_offset = - (g_evolve_y_parameter_range / 2);
    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
}

void fiddleparms(GENEBASE gene[], int ecount)
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

    set_random(ecount);   // generate the right number of pseudo randoms

    for (int i = 0; i < NUM_GENES; i++)
    {
        (*(gene[i].varyfunc))(gene, std::rand(), i);
    }

}

static void set_random(int ecount)
{
    // This must be called with ecount set correctly for the spiral map.
    // Call this routine to set the random # to the proper value
    // if it may have changed, before fiddleparms() is called.
    // Now called by fiddleparms().
    srand(g_evolve_this_generation_random_seed);
    for (int index = 0; index < ecount; index++)
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
    // one of the non random variation modes. Used to see if parmzoom box is
    // needed
    for (auto &elem : g_gene_bank)
    {
        if ((elem.mutate != variations::NONE) && (elem.mutate < variations::RANDOM))
        {
            return true;
        }
    }
    return false;
}

void drawparmbox(int mode)
{
    // draws parameter zoom box in evolver mode
    // clears boxes off screen if mode = 1, otherwise, redraws boxes
    coords tl, tr, bl, br;
    if (!bit_set(g_evolving, evolution_mode_flags::PARMBOX))
    {
        return; // don't draw if not asked to!
    }
    const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
    s_image_box_count = g_box_count;
    if (g_box_count)
    {
        // stash normal zoombox pixels
        std::copy(&g_box_x[0], &g_box_x[g_box_count*2], &s_image_box_x[0]);
        std::copy(&g_box_y[0], &g_box_y[g_box_count*2], &s_image_box_y[0]);
        std::copy(&g_box_values[0], &g_box_values[g_box_count], &s_image_box_values[0]);
        clearbox(); // to avoid probs when one box overlaps the other
    }
    if (g_evolve_param_box_count != 0)
    {
        // clear last parmbox
        g_box_count = g_evolve_param_box_count;
        std::copy(&s_param_box_x[0], &s_param_box_x[g_box_count*2], &g_box_x[0]);
        std::copy(&s_param_box_y[0], &s_param_box_y[g_box_count*2], &g_box_y[0]);
        std::copy(&s_param_box_values[0], &s_param_box_values[g_box_count], &g_box_values[0]);
        clearbox();
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
    addbox(br);
    addbox(tr);
    addbox(bl);
    addbox(tl);
    drawlines(tl, tr, bl.x-tl.x, bl.y-tl.y);
    drawlines(tl, bl, tr.x-tl.x, tr.y-tl.y);
    if (g_box_count)
    {
        dispbox();
        // stash pixel values for later
        std::copy(&g_box_x[0], &g_box_x[g_box_count*2], &s_param_box_x[0]);
        std::copy(&g_box_y[0], &g_box_y[g_box_count*2], &s_param_box_y[0]);
        std::copy(&g_box_values[0], &g_box_values[g_box_count], &s_param_box_values[0]);
    }
    g_evolve_param_box_count = g_box_count;
    g_box_count = s_image_box_count;
    if (s_image_box_count)
    {
        // and move back old values so that everything can proceed as normal
        std::copy(&s_image_box_x[0], &s_image_box_x[g_box_count*2], &g_box_x[0]);
        std::copy(&s_image_box_y[0], &s_image_box_y[g_box_count*2], &g_box_y[0]);
        std::copy(&s_image_box_values[0], &s_image_box_values[g_box_count], &g_box_values[0]);
        dispbox();
    }
}

void set_evolve_ranges()
{
    int lclpy = g_evolve_image_grid_size - g_evolve_param_grid_y - 1;
    // set up ranges and offsets for parameter explorer/evolver
    g_evolve_x_parameter_range = g_evolve_dist_per_x*(g_evolve_param_zoom*2.0);
    g_evolve_y_parameter_range = g_evolve_dist_per_y*(g_evolve_param_zoom*2.0);
    g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset +(((double)g_evolve_param_grid_x-g_evolve_param_zoom)*g_evolve_dist_per_x);
    g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset +(((double)lclpy-g_evolve_param_zoom)*g_evolve_dist_per_y);

    g_evolve_new_discrete_x_parameter_offset = (char)(g_evolve_discrete_x_parameter_offset +(g_evolve_param_grid_x- g_evolve_image_grid_size /2));
    g_evolve_new_discrete_y_parameter_offset = (char)(g_evolve_discrete_y_parameter_offset +(lclpy- g_evolve_image_grid_size /2));
}

void spiralmap(int count)
{
    // maps out a clockwise spiral for a prettier and possibly
    // more intuitively useful order of drawing the sub images.
    // All the malarky with count is to allow resuming
    int i, mid;
    i = 0;
    mid = g_evolve_image_grid_size / 2;
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

int unspiralmap()
{
    // unmaps the clockwise spiral
    // All this malarky is to allow selecting different subimages
    // Returns the count from the center subimage to the current px & py
    int mid;
    static int old_image_grid_size = 0;

    mid = g_evolve_image_grid_size / 2;
    if ((g_evolve_param_grid_x == mid && g_evolve_param_grid_y == mid) || (old_image_grid_size != g_evolve_image_grid_size))
    {
        // set up array and return
        int gridsqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
        s_ecount_box[g_evolve_param_grid_x][g_evolve_param_grid_y] = 0;  // we know the first one, do the rest
        for (int i = 1; i < gridsqr; i++)
        {
            spiralmap(i);
            s_ecount_box[g_evolve_param_grid_x][g_evolve_param_grid_y] = i;
        }
        old_image_grid_size = g_evolve_image_grid_size;
        g_evolve_param_grid_y = mid;
        g_evolve_param_grid_x = g_evolve_param_grid_y;
        return 0;
    }
    return s_ecount_box[g_evolve_param_grid_x][g_evolve_param_grid_y];
}
