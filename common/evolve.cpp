#include <algorithm>
#include <vector>

#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#define PARMBOX 128
GENEBASE gene_bank[NUMGENES];

// px and py are coordinates in the parameter grid (small images on screen)
// evolving = flag, evolve_image_grid_size = dimensions of image grid (evolve_image_grid_size x evolve_image_grid_size)
int px, py, evolving, evolve_image_grid_size;
#define EVOLVE_MAX_GRID_SIZE 51  // This is arbitrary, = 1024/20
static int ecountbox[EVOLVE_MAX_GRID_SIZE][EVOLVE_MAX_GRID_SIZE];

// used to replay random sequences to obtain correct values when selecting a
// seed image for next generation
unsigned int evolve_this_generation_random_seed;

// variation factors, opx, opy, paramrangex/y dpx, dpy.
// used in field mapping for smooth variation across screen.
// opx =offset param x,
// dpx = delta param per image,
// paramrangex = variation across grid of param ...likewise for py
// evolve_max_random_mutation is amount of random mutation used in random modes ,
// evolve_mutation_reduction_factor is used to decrease evolve_max_random_mutation from one generation to the
// next to eventually produce a stable population
double evolve_x_parameter_offset;
double evolve_y_parameter_offset;
double evolve_new_x_parameter_offset;
double evolve_new_y_parameter_offset;
double evolve_x_parameter_range;
double evolve_y_parameter_range;
double dpx;
double dpy;
double evolve_max_random_mutation;
double evolve_mutation_reduction_factor;
double parmzoom;

// offset for discrete parameters x and y..
// used for things like inside or outside types, bailout tests, trig fn etc
char evolve_discrete_x_parameter_offset;
char evolve_discrete_y_parameter_offset;
char evolve_new_discrete_x_parameter_offset;
char evolve_new_discrete_y_parameter_offset;

int prmboxcount;
std::vector<int> param_box_x;
std::vector<int> param_box_y;
std::vector<int> param_box_values;
int imgboxcount;
std::vector<int> image_box_x;
std::vector<int> image_box_y;
std::vector<int> image_box_values;

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

void param_history(int mode);
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

void copy_genes_from_bank(GENEBASE gene[NUMGENES])
{
    std::copy(&gene_bank[0], &gene_bank[NUMGENES], &gene[0]);
}

void copy_genes_to_bank(GENEBASE const gene[NUMGENES])
{
    // cppcheck-suppress arrayIndexOutOfBounds
    std::copy(&gene[0], &gene[NUMGENES], &gene_bank[0]);
}

// set up pointers and mutation params for all usable image
// control variables in fractint... revise as necessary when
// new vars come along... don't forget to increment NUMGENES
// (in fractint.h ) as well
void initgene()
{
    //                        Use only 15 letters below: 123456789012345
    GENEBASE gene[NUMGENES] = {
        { &param[0], varydbl, variations::RANDOM,       "Param 1 real", 1 },
        { &param[1], varydbl, variations::RANDOM,       "Param 1 imag", 1 },
        { &param[2], varydbl, variations::NONE,         "Param 2 real", 1 },
        { &param[3], varydbl, variations::NONE,         "Param 2 imag", 1 },
        { &param[4], varydbl, variations::NONE,         "Param 3 real", 1 },
        { &param[5], varydbl, variations::NONE,         "Param 3 imag", 1 },
        { &param[6], varydbl, variations::NONE,         "Param 4 real", 1 },
        { &param[7], varydbl, variations::NONE,         "Param 4 imag", 1 },
        { &param[8], varydbl, variations::NONE,         "Param 5 real", 1 },
        { &param[9], varydbl, variations::NONE,         "Param 5 imag", 1 },
        { &inside, varyinside, variations::NONE,        "inside color", 2 },
        { &outside, varyoutside, variations::NONE,      "outside color", 3 },
        { &decomp[0], varypwr2, variations::NONE,       "decomposition", 4 },
        { &inversion[0], varyinv, variations::NONE,     "invert radius", 7 },
        { &inversion[1], varyinv, variations::NONE,     "invert center x", 7 },
        { &inversion[2], varyinv, variations::NONE,     "invert center y", 7 },
        { &trigndx[0], varytrig, variations::NONE,      "trig function 1", 5 },
        { &trigndx[1], varytrig, variations::NONE,      "trig fn 2", 5 },
        { &trigndx[2], varytrig, variations::NONE,      "trig fn 3", 5 },
        { &trigndx[3], varytrig, variations::NONE,      "trig fn 4", 5 },
        { &bailoutest, varybotest, variations::NONE,    "bailout test", 6 }
    };

    copy_genes_to_bank(gene);
}

namespace
{

PARAMHIST oldhistory = { 0 };

void save_param_history()
{
    // save the old parameter history
    oldhistory.param0 = param[0];
    oldhistory.param1 = param[1];
    oldhistory.param2 = param[2];
    oldhistory.param3 = param[3];
    oldhistory.param4 = param[4];
    oldhistory.param5 = param[5];
    oldhistory.param6 = param[6];
    oldhistory.param7 = param[7];
    oldhistory.param8 = param[8];
    oldhistory.param9 = param[9];
    oldhistory.inside = inside;
    oldhistory.outside = outside;
    oldhistory.decomp0 = decomp[0];
    oldhistory.invert0 = inversion[0];
    oldhistory.invert1 = inversion[1];
    oldhistory.invert2 = inversion[2];
    oldhistory.trigndx0 = static_cast<BYTE>(trigndx[0]);
    oldhistory.trigndx1 = static_cast<BYTE>(trigndx[1]);
    oldhistory.trigndx2 = static_cast<BYTE>(trigndx[2]);
    oldhistory.trigndx3 = static_cast<BYTE>(trigndx[3]);
    oldhistory.bailoutest = bailoutest;
}

void restore_param_history()
{
    // restore the old parameter history
    param[0] = oldhistory.param0;
    param[1] = oldhistory.param1;
    param[2] = oldhistory.param2;
    param[3] = oldhistory.param3;
    param[4] = oldhistory.param4;
    param[5] = oldhistory.param5;
    param[6] = oldhistory.param6;
    param[7] = oldhistory.param7;
    param[8] = oldhistory.param8;
    param[9] = oldhistory.param9;
    inside = oldhistory.inside;
    outside = oldhistory.outside;
    decomp[0] = oldhistory.decomp0;
    inversion[0] = oldhistory.invert0;
    inversion[1] = oldhistory.invert1;
    inversion[2] = oldhistory.invert2;
    invert = (inversion[0] == 0.0) ? 0 : 3;
    trigndx[0] = static_cast<trig_fn>(oldhistory.trigndx0);
    trigndx[1] = static_cast<trig_fn>(oldhistory.trigndx1);
    trigndx[2] = static_cast<trig_fn>(oldhistory.trigndx2);
    trigndx[3] = static_cast<trig_fn>(oldhistory.trigndx3);
    bailoutest = static_cast<bailouts>(oldhistory.bailoutest);
}

}

// mode = 0 for save old history,
// mode = 1 for restore old history
void param_history(int mode)
{
    if (mode == 0)
    {
        save_param_history();
    }

    if (mode == 1)
    {
        restore_param_history();
    }
}

// routine to vary doubles
void varydbl(GENEBASE gene[], int randval, int i)
{
    int lclpy = evolve_image_grid_size - py - 1;
    switch (gene[i].mutate)
    {
    default:
    case variations::NONE:
        break;
    case variations::X:
        *(double *)gene[i].addr = px * dpx + evolve_x_parameter_offset; //paramspace x coord * per view delta px + offset
        break;
    case variations::Y:
        *(double *)gene[i].addr = lclpy * dpy + evolve_y_parameter_offset; //same for y
        break;
    case variations::X_PLUS_Y:
        *(double *)gene[i].addr = px*dpx+ evolve_x_parameter_offset +(lclpy*dpy)+ evolve_y_parameter_offset; //and x+y
        break;
    case variations::X_MINUS_Y:
        *(double *)gene[i].addr = (px*dpx+ evolve_x_parameter_offset)-(lclpy*dpy+ evolve_y_parameter_offset); //and x-y
        break;
    case variations::RANDOM:
        *(double *)gene[i].addr += (((double)randval / RAND_MAX) * 2 * evolve_max_random_mutation) - evolve_max_random_mutation;
        break;
    case variations::WEIGHTED_RANDOM:
    {
        int mid = evolve_image_grid_size /2;
        double radius =  sqrt(static_cast<double>(sqr(px - mid) + sqr(lclpy - mid)));
        *(double *)gene[i].addr += ((((double)randval / RAND_MAX) * 2 * evolve_max_random_mutation) - evolve_max_random_mutation) * radius;
    }
    break;
    }
    return;
}

int varyint(int randvalue, int limit, variations mode)
{
    int ret = 0;
    int lclpy = evolve_image_grid_size - py - 1;
    switch (mode)
    {
    default:
    case variations::NONE:
        break;
    case variations::X:
        ret = (evolve_discrete_x_parameter_offset +px)%limit;
        break;
    case variations::Y:
        ret = (evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::X_PLUS_Y:
        ret = (evolve_discrete_x_parameter_offset +px+ evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::X_MINUS_Y:
        ret = (evolve_discrete_x_parameter_offset +px)-(evolve_discrete_y_parameter_offset +lclpy)%limit;
        break;
    case variations::RANDOM:
        ret = randvalue % limit;
        break;
    case variations::WEIGHTED_RANDOM:
    {
        int mid = evolve_image_grid_size /2;
        double radius =  sqrt(static_cast<double>(sqr(px - mid) + sqr(lclpy - mid)));
        ret = (int)((((randvalue / RAND_MAX) * 2 * evolve_max_random_mutation) - evolve_max_random_mutation) * radius);
        ret %= limit;
        break;
    }
    }
    return (ret);
}

int wrapped_positive_varyint(int randvalue, int limit, variations mode)
{
    int i;
    i = varyint(randvalue, limit, mode);
    if (i < 0)
        return (limit + i);
    else
        return (i);
}

void varyinside(GENEBASE gene[], int randval, int i)
{
    int choices[9] = { ZMAG, BOF60, BOF61, EPSCROSS, STARTRAIL, PERIOD, FMODI, ATANI, ITER };
    if (gene[i].mutate != variations::NONE)
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 9, gene[i].mutate)];
    return;
}

void varyoutside(GENEBASE gene[], int randval, int i)
{
    int choices[8] = { ITER, REAL, IMAG, MULT, SUM, ATAN, FMOD, TDIS };
    if (gene[i].mutate != variations::NONE)
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 8, gene[i].mutate)];
    return;
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
        setbailoutformula(bailoutest);
    }
    return;
}

void varypwr2(GENEBASE gene[], int randval, int i)
{
    int choices[9] = {0, 2, 4, 8, 16, 32, 64, 128, 256};
    if (gene[i].mutate != variations::NONE)
        *(int*)gene[i].addr = choices[wrapped_positive_varyint(randval, 9, gene[i].mutate)];
    return;
}

void varytrig(GENEBASE gene[], int randval, int i)
{
    if (gene[i].mutate != variations::NONE)
    {
        *static_cast<trig_fn *>(gene[i].addr) =
            static_cast<trig_fn>(wrapped_positive_varyint(randval, numtrigfn, gene[i].mutate));
    }
    set_trig_pointers(5); //set all trig ptrs up
    return;
}

void varyinv(GENEBASE gene[], int randval, int i)
{
    if (gene[i].mutate != variations::NONE)
        varydbl(gene, randval, i);
    invert = (inversion[0] == 0.0) ? 0 : 3 ;
}

// ---------------------------------------------------------------------
/*
    get_evolve_params() is called from FRACTINT.C whenever the 'ctrl_e' key
    is pressed.  Return codes are:
      -1  routine was ESCAPEd - no need to re-generate the images
     0  minor variable changed.  No need to re-generate the image.
       1  major parms changed.  Re-generate the images.
*/
int get_the_rest()
{
    char const *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    int i, k, numtrig;
    char const *choices[20];
    fullscreenvalues uvalues[20];
    GENEBASE gene[NUMGENES];

    copy_genes_from_bank(gene);

    numtrig = (curfractalspecific->flags >> 6) & 7;
    if (fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA)
    {
        numtrig = maxfn;
    }

choose_vars_restart:

    k = -1;
    for (int num = MAXPARAMS; num < (NUMGENES - 5); num++)
    {
        choices[++k] = gene[num].name;
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = 7;
        uvalues[k].uval.ch.list = evolvmodes;
        uvalues[k].uval.ch.val =  static_cast<int>(gene[num].mutate);
    }

    for (int num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
    {
        choices[++k] = gene[num].name;
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = 7;
        uvalues[k].uval.ch.list = evolvmodes;
        uvalues[k].uval.ch.val =  static_cast<int>(gene[num].mutate);
    }

    if (curfractalspecific->calctype == StandardFractal &&
            (curfractalspecific->flags & BAILTEST))
    {
        choices[++k] = gene[NUMGENES - 1].name;
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = 7;
        uvalues[k].uval.ch.list = evolvmodes;
        uvalues[k].uval.ch.val = static_cast<int>(gene[NUMGENES - 1].mutate);
    }

    choices[++k] = "";
    uvalues[k].type = '*';
    choices[++k] = "Press F2 to set all to off";
    uvalues[k].type ='*';
    choices[++k] = "Press F3 to set all on";
    uvalues[k].type = '*';
    choices[++k] = "Press F4 to randomize all";
    uvalues[k].type = '*';

    i = fullscreen_prompt("Variable tweak central 2 of 2", k+1, choices, uvalues, 28, nullptr);

    switch (i)
    {
    case FIK_F2: // set all off
        for (int num = MAXPARAMS; num < NUMGENES; num++)
            gene[num].mutate = variations::NONE;
        goto choose_vars_restart;
    case FIK_F3: // set all on..alternate x and y for field map
        for (int num = MAXPARAMS; num < NUMGENES; num ++)
            gene[num].mutate = static_cast<variations>((num % 2) + 1);
        goto choose_vars_restart;
    case FIK_F4: // Randomize all
        for (int num = MAXPARAMS; num < NUMGENES; num ++)
            gene[num].mutate = static_cast<variations>(rand() % static_cast<int>(variations::NUM));
        goto choose_vars_restart;
    case -1:
        return (-1);
    default:
        break;
    }

    // read out values
    k = -1;
    for (int num = MAXPARAMS; num < (NUMGENES - 5); num++)
        gene[num].mutate = static_cast<variations>(uvalues[++k].uval.ch.val);

    for (int num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
        gene[num].mutate = static_cast<variations>(uvalues[++k].uval.ch.val);

    if (curfractalspecific->calctype == StandardFractal &&
            (curfractalspecific->flags & BAILTEST))
        gene[NUMGENES - 1].mutate = static_cast<variations>(uvalues[++k].uval.ch.val);

    copy_genes_to_bank(gene);
    return (1); // if you were here, you want to regenerate
}

int get_variations()
{
    char const *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
    int k, numparams;
    char const *choices[20];
    fullscreenvalues uvalues[20];
    GENEBASE gene[NUMGENES];
    int firstparm = 0;
    int lastparm  = MAXPARAMS;
    int chngd = -1;

    copy_genes_from_bank(gene);

    if (fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA)
    {
        if (uses_p1)  // set first parameter
            firstparm = 0;
        else if (uses_p2)
            firstparm = 2;
        else if (uses_p3)
            firstparm = 4;
        else if (uses_p4)
            firstparm = 6;
        else
            firstparm = 8; // uses_p5 or no parameter

        if (uses_p5) // set last parameter
            lastparm = 10;
        else if (uses_p4)
            lastparm = 8;
        else if (uses_p3)
            lastparm = 6;
        else if (uses_p2)
            lastparm = 4;
        else
            lastparm = 2; // uses_p1 or no parameter
    }

    numparams = 0;
    for (int i = firstparm; i < lastparm; i++)
    {
        if (typehasparm(julibrot ? neworbittype : fractype, i, nullptr) == 0)
        {
            if (fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA)
                if (paramnotused(i))
                    continue;
            break;
        }
        numparams++;
    }

    if (fractype != fractal_type::FORMULA && fractype != fractal_type::FFORMULA)
        lastparm = numparams;

choose_vars_restart:

    k = -1;
    for (int num = firstparm; num < lastparm; num++)
    {
        if (fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA)
            if (paramnotused(num))
                continue;
        choices[++k] = gene[num].name;
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = 7;
        uvalues[k].uval.ch.list = evolvmodes;
        uvalues[k].uval.ch.val = static_cast<int>(gene[num].mutate);
    }

    choices[++k] = "";
    uvalues[k].type = '*';
    choices[++k] = "Press F2 to set all to off";
    uvalues[k].type ='*';
    choices[++k] = "Press F3 to set all on";
    uvalues[k].type = '*';
    choices[++k] = "Press F4 to randomize all";
    uvalues[k].type = '*';
    choices[++k] = "Press F6 for second page"; // F5 gets eaten
    uvalues[k].type = '*';

    int i = fullscreen_prompt("Variable tweak central 1 of 2", k+1, choices, uvalues, 92, nullptr);

    switch (i)
    {
    case FIK_F2: // set all off
        for (int num = 0; num < MAXPARAMS; num++)
            gene[num].mutate = variations::NONE;
        goto choose_vars_restart;
    case FIK_F3: // set all on..alternate x and y for field map
        for (int num = 0; num < MAXPARAMS; num ++)
            gene[num].mutate = static_cast<variations>((num % 2) + 1);
        goto choose_vars_restart;
    case FIK_F4: // Randomize all
        for (int num =0; num < MAXPARAMS; num ++)
            gene[num].mutate = static_cast<variations>(rand() % static_cast<int>(variations::NUM));
        goto choose_vars_restart;
    case FIK_F6: // go to second screen, put array away first
        copy_genes_to_bank(gene);
        chngd = get_the_rest();
        copy_genes_from_bank(gene);
        goto choose_vars_restart;
    case -1:
        return (chngd);
    default:
        break;
    }

    // read out values
    k = -1;
    for (int num = firstparm; num < lastparm; num++)
    {
        if (fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA)
            if (paramnotused(num))
                continue;
        gene[num].mutate = static_cast<variations>(uvalues[++k].uval.ch.val);
    }

    copy_genes_to_bank(gene);
    return (1); // if you were here, you want to regenerate
}

void set_mutation_level(int strength)
{
    // scan through the gene array turning on random variation for all parms that
    // are suitable for this level of mutation
    for (auto &elem : gene_bank)
    {
        if (elem.level <= strength)
            elem.mutate = variations::RANDOM;
        else
            elem.mutate = variations::NONE;
    }
}

int get_evolve_Parms()
{
    char const *choices[20];
    int old_help_mode;
    fullscreenvalues uvalues[20];
    int i, j, k, tmp;
    int old_evolving, old_image_grid_size;
    int old_variations = 0;
    double old_x_parameter_range, old_y_parameter_range, old_x_parameter_offset, old_y_parameter_offset, old_max_random_mutation;

    // fill up the previous values arrays
    old_evolving      = evolving;
    old_image_grid_size = evolve_image_grid_size;
    old_x_parameter_range = evolve_x_parameter_range;
    old_y_parameter_range = evolve_y_parameter_range;
    old_x_parameter_offset = evolve_x_parameter_offset;
    old_y_parameter_offset = evolve_y_parameter_offset;
    old_max_random_mutation = evolve_max_random_mutation;

get_evol_restart:

    if ((evolving & RANDWALK) || (evolving & RANDPARAM))
    {
        // adjust field param to make some sense when changing from random modes
        // maybe should adjust for aspect ratio here?
        evolve_y_parameter_range = evolve_max_random_mutation * 2;
        evolve_x_parameter_range = evolve_max_random_mutation * 2;
        evolve_x_parameter_offset = param[0] - evolve_max_random_mutation;
        evolve_y_parameter_offset = param[1] - evolve_max_random_mutation;
        // set middle image to last selected and edges to +- evolve_max_random_mutation
    }

    k = -1;

    choices[++k] = "Evolution mode? (no for full screen)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = evolving&1;

    choices[++k] = "Image grid size (odd numbers only)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = evolve_image_grid_size;

    if (explore_check())
    {  // test to see if any parms are set to linear
        // variation 'explore mode'
        choices[++k] = "Show parameter zoom box?";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = ((evolving & PARMBOX) / PARMBOX);

        choices[++k] = "x parameter range (across screen)";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = evolve_x_parameter_range;

        choices[++k] = "x parameter offset (left hand edge)";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = evolve_x_parameter_offset;

        choices[++k] = "y parameter range (up screen)";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = evolve_y_parameter_range;

        choices[++k] = "y parameter offset (lower edge)";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = evolve_y_parameter_offset;
    }

    choices[++k] = "Max random mutation";
    uvalues[k].type = 'f';
    uvalues[k].uval.dval = evolve_max_random_mutation;

    choices[++k] = "Mutation reduction factor (between generations)";
    uvalues[k].type = 'f';
    uvalues[k].uval.dval = evolve_mutation_reduction_factor;

    choices[++k] = "Grouting? ";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = !((evolving & NOGROUT) / NOGROUT);

    choices[++k] = "";
    uvalues[k].type = '*';

    choices[++k] = "Press F4 to reset view parameters to defaults.";
    uvalues[k].type = '*';

    choices[++k] = "Press F2 to halve mutation levels";
    uvalues[k].type = '*';

    choices[++k] = "Press F3 to double mutation levels" ;
    uvalues[k].type ='*';

    choices[++k] = "Press F6 to control which parameters are varied";
    uvalues[k].type = '*';
    old_help_mode = help_mode;     // this prevents HELP from activating
    help_mode = HELPEVOL;
    i = fullscreen_prompt("Evolution Mode Options", k+1, choices, uvalues, 255, nullptr);
    help_mode = old_help_mode;     // re-enable HELP
    if (i < 0)
    {
        // in case this point has been reached after calling sub menu with F6
        evolving      = old_evolving;
        evolve_image_grid_size = old_image_grid_size;
        evolve_x_parameter_range = old_x_parameter_range;
        evolve_y_parameter_range = old_y_parameter_range;
        evolve_x_parameter_offset = old_x_parameter_offset;
        evolve_y_parameter_offset = old_y_parameter_offset;
        evolve_max_random_mutation = old_max_random_mutation;

        return (-1);
    }

    if (i == FIK_F4)
    {
        set_current_params();
        evolve_max_random_mutation = 1;
        evolve_mutation_reduction_factor = 1.0;
        goto get_evol_restart;
    }
    if (i == FIK_F2)
    {
        evolve_x_parameter_range = evolve_x_parameter_range / 2;
        evolve_new_x_parameter_offset = evolve_x_parameter_offset + evolve_x_parameter_range /2;
        evolve_x_parameter_offset = evolve_new_x_parameter_offset;
        evolve_y_parameter_range = evolve_y_parameter_range / 2;
        evolve_new_y_parameter_offset = evolve_y_parameter_offset + evolve_y_parameter_range / 2;
        evolve_y_parameter_offset = evolve_new_y_parameter_offset;
        evolve_max_random_mutation = evolve_max_random_mutation / 2;
        goto get_evol_restart;
    }
    if (i == FIK_F3)
    {
        double centerx, centery;
        centerx = evolve_x_parameter_offset + evolve_x_parameter_range / 2;
        evolve_x_parameter_range = evolve_x_parameter_range * 2;
        evolve_new_x_parameter_offset = centerx - evolve_x_parameter_range / 2;
        evolve_x_parameter_offset = evolve_new_x_parameter_offset;
        centery = evolve_y_parameter_offset + evolve_y_parameter_range / 2;
        evolve_y_parameter_range = evolve_y_parameter_range * 2;
        evolve_new_y_parameter_offset = centery - evolve_y_parameter_range / 2;
        evolve_y_parameter_offset = evolve_new_y_parameter_offset;
        evolve_max_random_mutation = evolve_max_random_mutation * 2;
        goto get_evol_restart;
    }

    j = i;

    // now check out the results (*hopefully* in the same order <grin>)

    k = -1;

    evolving = uvalues[++k].uval.ch.val;
    viewwindow = evolving != 0;

    if (!evolving && i != FIK_F6)  // don't need any of the other parameters
        return (1);             // the following code can set evolving even if it's off

    evolve_image_grid_size = uvalues[++k].uval.ival;
    tmp = sxdots / (MINPIXELS << 1);
    // (sxdots / 20), max # of subimages @ 20 pixels per subimage
    // EVOLVE_MAX_GRID_SIZE == 1024 / 20 == 51
    if (evolve_image_grid_size > EVOLVE_MAX_GRID_SIZE)
        evolve_image_grid_size = EVOLVE_MAX_GRID_SIZE;
    if (evolve_image_grid_size > tmp)
        evolve_image_grid_size = tmp;
    if (evolve_image_grid_size < 3)
        evolve_image_grid_size = 3;
    evolve_image_grid_size |= 1; // make sure evolve_image_grid_size is odd
    if (explore_check())
    {
        tmp = (PARMBOX * uvalues[++k].uval.ch.val);
        if (evolving)
            evolving += tmp;
        evolve_x_parameter_range = uvalues[++k].uval.dval;
        evolve_x_parameter_offset = uvalues[++k].uval.dval;
        evolve_new_x_parameter_offset = evolve_x_parameter_offset;
        evolve_y_parameter_range = uvalues[++k].uval.dval;
        evolve_y_parameter_offset = uvalues[++k].uval.dval;
        evolve_new_y_parameter_offset = evolve_y_parameter_offset;
    }

    evolve_max_random_mutation = uvalues[++k].uval.dval;

    evolve_mutation_reduction_factor = uvalues[++k].uval.dval;

    if (!(uvalues[++k].uval.ch.val))
        evolving = evolving + NOGROUT;

    viewxdots = (sxdots / evolve_image_grid_size)-2;
    viewydots = (sydots / evolve_image_grid_size)-2;
    if (!viewwindow)
    {
        viewydots = 0;
        viewxdots = viewydots;
    }

    i = 0;

    if (evolving != old_evolving
            || (evolve_image_grid_size != old_image_grid_size) || (evolve_x_parameter_range != old_x_parameter_range)
            || (evolve_x_parameter_offset != old_x_parameter_offset) || (evolve_y_parameter_range != old_y_parameter_range)
            || (evolve_y_parameter_offset != old_y_parameter_offset)  || (evolve_max_random_mutation != old_max_random_mutation)
            || (old_variations > 0))
        i = 1;

    if (evolving && !old_evolving)
        param_history(0); // save old history

    if (!evolving && (evolving == old_evolving))
        i = 0;

    if (j == FIK_F6)
    {
        old_variations = get_variations();
        set_current_params();
        if (old_variations > 0)
        {
            viewwindow = true;
            evolving |= 1;   // leave other settings alone
        }
        evolve_max_random_mutation = 1;
        evolve_mutation_reduction_factor = 1.0;
        goto get_evol_restart;
    }
    return (i);
}

void SetupParamBox()
{
    prmboxcount = 0;
    parmzoom = ((double) evolve_image_grid_size -1.0)/2.0;
    // need to allocate 2 int arrays for boxx and boxy plus 1 byte array for values
    int const num_box_values = (xdots + ydots)*2;
    int const num_values = xdots + ydots + 2;

    param_box_x.resize(num_box_values);
    param_box_y.resize(num_box_values);
    param_box_values.resize(num_values);

    image_box_x.resize(num_box_values);
    image_box_y.resize(num_box_values);
    image_box_values.resize(num_values);
}

void ReleaseParamBox()
{
    param_box_x.clear();
    param_box_y.clear();
    param_box_values.clear();
    image_box_x.clear();
    image_box_y.clear();
    image_box_values.clear();
}

void set_current_params()
{
    evolve_x_parameter_range = curfractalspecific->xmax - curfractalspecific->xmin;
    evolve_new_x_parameter_offset = - (evolve_x_parameter_range / 2);
    evolve_x_parameter_offset = evolve_new_x_parameter_offset;
    evolve_y_parameter_range = curfractalspecific->ymax - curfractalspecific->ymin;
    evolve_new_y_parameter_offset = - (evolve_y_parameter_range / 2);
    evolve_y_parameter_offset = evolve_new_y_parameter_offset;
    return;
}

void fiddleparms(GENEBASE gene[], int ecount)
{
    // call with px, py ... parameter set co-ords
    // set random seed then call rnd enough times to get to px, py

    /* when writing routines to vary param types make sure that rand() gets called
    the same number of times whether gene[].mutate is set or not to allow
    user to change it between generations without screwing up the duplicability
    of the sequence and starting from the wrong point */

    /* this function has got simpler and simpler throughout the construction of the
     evolver feature and now consists of just these few lines to loop through all
     the variables referenced in the gene array and call the functions required
     to vary them, aren't pointers marvellous! */

    if ((px == evolve_image_grid_size / 2) && (py == evolve_image_grid_size / 2)) // return if middle image
        return;

    set_random(ecount);   // generate the right number of pseudo randoms

    for (int i = 0; i < NUMGENES; i++)
        (*(gene[i].varyfunc))(gene, rand(), i);

}

static void set_random(int ecount)
{
    // This must be called with ecount set correctly for the spiral map.
    // Call this routine to set the random # to the proper value
    // if it may have changed, before fiddleparms() is called.
    // Now called by fiddleparms().
    srand(evolve_this_generation_random_seed);
    for (int index = 0; index < ecount; index++)
        for (int i = 0; i < NUMGENES; i++)
            rand();
}

static bool explore_check()
{
    // checks through gene array to see if any of the parameters are set to
    // one of the non random variation modes. Used to see if parmzoom box is
    // needed
    for (auto &elem : gene_bank)
        if ((elem.mutate != variations::NONE) && (elem.mutate < variations::RANDOM))
            return true;
    return false;
}

void drawparmbox(int mode)
{
    // draws parameter zoom box in evolver mode
    // clears boxes off screen if mode = 1, otherwise, redraws boxes
    coords tl, tr, bl, br;
    int grout;
    if (!(evolving & PARMBOX))
        return; // don't draw if not asked to!
    grout = !((evolving & NOGROUT)/NOGROUT) ;
    imgboxcount = boxcount;
    if (boxcount)
    {
        // stash normal zoombox pixels
        std::copy(&boxx[0], &boxx[boxcount*2], &image_box_x[0]);
        std::copy(&boxy[0], &boxy[boxcount*2], &image_box_y[0]);
        std::copy(&boxvalues[0], &boxvalues[boxcount], &image_box_values[0]);
        clearbox(); // to avoid probs when one box overlaps the other
    }
    if (prmboxcount != 0)
    {
        // clear last parmbox
        boxcount = prmboxcount;
        std::copy(&param_box_x[0], &param_box_x[boxcount*2], &boxx[0]);
        std::copy(&param_box_y[0], &param_box_y[boxcount*2], &boxy[0]);
        std::copy(&param_box_values[0], &param_box_values[boxcount], &boxvalues[0]);
        clearbox();
    }

    if (mode == 1)
    {
        boxcount = imgboxcount;
        prmboxcount = 0;
        return;
    }

    boxcount =0;
    //draw larger box to show parm zooming range
    bl.x = ((px -(int)parmzoom) * (int)(x_size_d+1+grout))-sxoffs-1;
    tl.x = bl.x;
    tr.y = ((py -(int)parmzoom) * (int)(y_size_d+1+grout))-syoffs-1;
    tl.y = tr.y;
    tr.x = ((px +1+(int)parmzoom) * (int)(x_size_d+1+grout))-sxoffs;
    br.x = tr.x;
    bl.y = ((py +1+(int)parmzoom) * (int)(y_size_d+1+grout))-syoffs;
    br.y = bl.y;
#ifndef XFRACT
    addbox(br);
    addbox(tr);
    addbox(bl);
    addbox(tl);
    drawlines(tl, tr, bl.x-tl.x, bl.y-tl.y);
    drawlines(tl, bl, tr.x-tl.x, tr.y-tl.y);
#else
    boxx[0] = tl.x + sxoffs;
    boxy[0] = tl.y + syoffs;
    boxx[1] = tr.x + sxoffs;
    boxy[1] = tr.y + syoffs;
    boxx[2] = br.x + sxoffs;
    boxy[2] = br.y + syoffs;
    boxx[3] = bl.x + sxoffs;
    boxy[3] = bl.y + syoffs;
    boxcount = 8;
#endif
    if (boxcount)
    {
        dispbox();
        // stash pixel values for later
        std::copy(&boxx[0], &boxx[boxcount*2], &param_box_x[0]);
        std::copy(&boxy[0], &boxy[boxcount*2], &param_box_y[0]);
        std::copy(&boxvalues[0], &boxvalues[boxcount], &param_box_values[0]);
    }
    prmboxcount = boxcount;
    boxcount = imgboxcount;
    if (imgboxcount)
    {
        // and move back old values so that everything can proceed as normal
        std::copy(&image_box_x[0], &image_box_x[boxcount*2], &boxx[0]);
        std::copy(&image_box_y[0], &image_box_y[boxcount*2], &boxy[0]);
        std::copy(&image_box_values[0], &image_box_values[boxcount], &boxvalues[0]);
        dispbox();
    }
    return;
}

void set_evolve_ranges()
{
    int lclpy = evolve_image_grid_size - py - 1;
    // set up ranges and offsets for parameter explorer/evolver
    evolve_x_parameter_range = dpx*(parmzoom*2.0);
    evolve_y_parameter_range = dpy*(parmzoom*2.0);
    evolve_new_x_parameter_offset = evolve_x_parameter_offset +(((double)px-parmzoom)*dpx);
    evolve_new_y_parameter_offset = evolve_y_parameter_offset +(((double)lclpy-parmzoom)*dpy);

    evolve_new_discrete_x_parameter_offset = (char)(evolve_discrete_x_parameter_offset +(px- evolve_image_grid_size /2));
    evolve_new_discrete_y_parameter_offset = (char)(evolve_discrete_y_parameter_offset +(lclpy- evolve_image_grid_size /2));
    return;
}

void spiralmap(int count)
{
    // maps out a clockwise spiral for a prettier and possibly
    // more intuitively useful order of drawing the sub images.
    // All the malarky with count is to allow resuming
    int i, mid;
    i = 0;
    mid = evolve_image_grid_size / 2;
    if (count == 0)
    { // start in the middle
        py = mid;
        px = py;
        return;
    }
    for (int offset = 1; offset <= mid; offset ++)
    {
        // first do the top row
        py = (mid - offset);
        for (px = (mid - offset)+1; px < mid+offset; px++)
        {
            i++;
            if (i == count)
                return;
        }
        // then do the right hand column
        for (; py < mid + offset; py++)
        {
            i++;
            if (i == count)
                return;
        }
        // then reverse along the bottom row
        for (; px > mid - offset; px--)
        {
            i++;
            if (i == count)
                return;
        }
        // then up the left to finish
        for (; py >= mid - offset; py--)
        {
            i++;
            if (i == count)
                return;
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

    mid = evolve_image_grid_size / 2;
    if ((px == mid && py == mid) || (old_image_grid_size != evolve_image_grid_size))
    {
        // set up array and return
        int gridsqr = evolve_image_grid_size * evolve_image_grid_size;
        ecountbox[px][py] = 0;  // we know the first one, do the rest
        for (int i = 1; i < gridsqr; i++)
        {
            spiralmap(i);
            ecountbox[px][py] = i;
        }
        old_image_grid_size = evolve_image_grid_size;
        py = mid;
        px = py;
        return (0);
    }
    return (ecountbox[px][py]);
}
