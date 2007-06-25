#include <string.h>

#include <string>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "calcfrac.h"
#include "evolve.h"
#include "fihelp.h"
#include "fractalp.h"
#include "history.h"
#include "miscres.h"
#include "prompts1.h"
#include "realdos.h"
#include "zoom.h"

#include "Formula.h"
#include "UIChoices.h"
#include "MathUtil.h"

enum VaryIntType
{
	VARYINT_NONE			= 0,
	VARYINT_WITH_X			= 1,
	VARYINT_WITH_Y			= 2,
	VARYINT_WITH_X_PLUS_Y	= 3,
	VARYINT_WITH_X_MINUS_Y	= 4,
	VARYINT_RANDOM			= 5,
	VARYINT_RANDOM_WEIGHTED	= 6
};

#define MAX_GRID_SIZE 51  /* This is arbitrary, = 1024/20 */

/* g_px and g_py are coordinates in the parameter grid (small images on screen) */
/* g_evolving_flags = flag, g_grid_size = dimensions of image grid (g_grid_size x g_grid_size) */
int g_px;
int g_py;
int g_evolving_flags;
int g_grid_size;
unsigned int g_this_generation_random_seed;
/* used to replay random sequences to obtain correct values when selecting a
	seed image for next generation */
double g_parameter_offset_x;
double g_parameter_offset_y;
double g_new_parameter_offset_x;
double g_new_parameter_offset_y;
double g_parameter_range_x;
double g_parameter_range_y;
double g_delta_parameter_image_x;
double g_delta_parameter_image_y;
double g_fiddle_factor;
double g_fiddle_reduction;
double g_parameter_zoom;
int g_discrete_parameter_offset_x;
int  g_discrete_parameter_offset_y;
int g_new_discrete_parameter_offset_x;
int  g_new_discrete_parameter_offset_y;
/* offset for discrete parameters x and y..*/
/* used for things like inside or outside types, bailout tests, trig fn etc */
/* variation factors, g_parameter_offset_x, g_parameter_offset_y, g_parameter_range_x/y g_delta_parameter_image_x, g_delta_parameter_image_y.. used in field mapping
	for smooth variation across screen. g_parameter_offset_x = offset param x, g_delta_parameter_image_x = delta param
	per image, g_parameter_range_x = variation across grid of param ...likewise for g_py */
/* g_fiddle_factor is amount of random mutation used in random modes ,
	g_fiddle_reduction is used to decrease g_fiddle_factor from one generation to the
	next to eventually produce a stable population */
int g_parameter_box_count = 0;

static int ecountbox[MAX_GRID_SIZE][MAX_GRID_SIZE];
static int *s_parameter_box = NULL;
static int *s_image_box = NULL;
static int s_image_box_count;

struct parameter_history_info      /* for saving evolution data of center image */
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
	BYTE function_index0;
	BYTE function_index1;
	BYTE function_index2;
	BYTE function_index3;
	int bailoutest;
};
typedef struct parameter_history_info    PARAMETER_HISTORY;
static PARAMETER_HISTORY s_old_history = { 0 };

static void vary_double(GENEBASE gene[], int randval, int i);
static int vary_int(int randvalue, int limit, int mode);
static int wrapped_positive_vary_int(int randvalue, int limit, int mode);
static void vary_inside(GENEBASE gene[], int randval, int i);
static void vary_outside(GENEBASE gene[], int randval, int i);
static void vary_power2(GENEBASE gene[], int randval, int i);
static void vary_function(GENEBASE gene[], int randval, int i);
static void vary_bail_out_test(GENEBASE gene[], int randval, int i);
static void vary_invert(GENEBASE gene[], int randval, int i);
static bool explore_check();
void spiral_map(int);
static void set_random(int);
void set_mutation_level(int);
void setup_parameter_box();
void release_parameter_box();

GENEBASE g_genes[NUMGENES] =
{
	{ &g_parameters[0],		vary_double,		5, "Param 1 real", 1 },
	{ &g_parameters[1],		vary_double,		5, "Param 1 imag", 1 },
	{ &g_parameters[2],		vary_double,		0, "Param 2 real", 1 },
	{ &g_parameters[3],		vary_double,		0, "Param 2 imag", 1 },
	{ &g_parameters[4],		vary_double,		0, "Param 3 real", 1 },
	{ &g_parameters[5],		vary_double,		0, "Param 3 imag", 1 },
	{ &g_parameters[6],		vary_double,		0, "Param 4 real", 1 },
	{ &g_parameters[7],		vary_double,		0, "Param 4 imag", 1 },
	{ &g_parameters[8],		vary_double,		0, "Param 5 real", 1 },
	{ &g_parameters[9],		vary_double,		0, "Param 5 imag", 1 },
	{ &g_inside,			vary_inside,		0, "inside color", 2 },
	{ &g_outside,			vary_outside,		0, "outside color", 3 },
	{ &g_decomposition[0],	vary_power2,		0, "decomposition", 4 },
	{ &g_inversion[0],		vary_invert,		0, "invert radius", 7 },
	{ &g_inversion[1],		vary_invert,		0, "invert center x", 7 },
	{ &g_inversion[2],		vary_invert,		0, "invert center y", 7 },
	{ &g_function_index[0],	vary_function,		0, "trig function 1", 5 },
	{ &g_function_index[1],	vary_function,		0, "trig fn 2", 5 },
	{ &g_function_index[2],	vary_function,		0, "trig fn 3", 5 },
	{ &g_function_index[3],	vary_function,		0, "trig fn 4", 5 },
	{ &g_bail_out_test,		vary_bail_out_test,	0, "bailout test", 6 }
};

void restore_parameter_history()
{
	g_parameters[0] = s_old_history.param0;
	g_parameters[1] = s_old_history.param1;
	g_parameters[2] = s_old_history.param2;
	g_parameters[3] = s_old_history.param3;
	g_parameters[4] = s_old_history.param4;
	g_parameters[5] = s_old_history.param5;
	g_parameters[6] = s_old_history.param6;
	g_parameters[7] = s_old_history.param7;
	g_parameters[8] = s_old_history.param8;
	g_parameters[9] = s_old_history.param9;
	g_inside = s_old_history.inside;
	g_outside = s_old_history.outside;
	g_decomposition[0] = s_old_history.decomp0;
	g_inversion[0] = s_old_history.invert0;
	g_inversion[1] = s_old_history.invert1;
	g_inversion[2] = s_old_history.invert2;
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
	g_function_index[0] = s_old_history.function_index0;
	g_function_index[1] = s_old_history.function_index1;
	g_function_index[2] = s_old_history.function_index2;
	g_function_index[3] = s_old_history.function_index3;
	g_bail_out_test = (bailouts) s_old_history.bailoutest;
}

void save_parameter_history()
{
	s_old_history.param0 = g_parameters[0];
	s_old_history.param1 = g_parameters[1];
	s_old_history.param2 = g_parameters[2];
	s_old_history.param3 = g_parameters[3];
	s_old_history.param4 = g_parameters[4];
	s_old_history.param5 = g_parameters[5];
	s_old_history.param6 = g_parameters[6];
	s_old_history.param7 = g_parameters[7];
	s_old_history.param8 = g_parameters[8];
	s_old_history.param9 = g_parameters[9];
	s_old_history.inside = g_inside;
	s_old_history.outside = g_outside;
	s_old_history.decomp0 = g_decomposition[0];
	s_old_history.invert0 = g_inversion[0];
	s_old_history.invert1 = g_inversion[1];
	s_old_history.invert2 = g_inversion[2];
	s_old_history.function_index0 = BYTE(g_function_index[0]);
	s_old_history.function_index1 = BYTE(g_function_index[1]);
	s_old_history.function_index2 = BYTE(g_function_index[2]);
	s_old_history.function_index3 = BYTE(g_function_index[3]);
	s_old_history.bailoutest = g_bail_out_test;
}

static void vary_double(GENEBASE gene[], int randval, int i) /* routine to vary doubles */
{
	int lclpy = g_grid_size - g_py - 1;
	switch (gene[i].mutate)
	{
	default:
	case VARYINT_NONE:
		break;
	case VARYINT_WITH_X:
		*(double *)gene[i].addr = g_px*g_delta_parameter_image_x + g_parameter_offset_x; /*paramspace x coord*per view delta g_px + offset */
		break;
	case VARYINT_WITH_Y:
		*(double *)gene[i].addr = lclpy*g_delta_parameter_image_y + g_parameter_offset_y; /*same for y */
		break;
	case VARYINT_WITH_X_PLUS_Y:
		*(double *)gene[i].addr = g_px*g_delta_parameter_image_x + g_parameter_offset_x +(lclpy*g_delta_parameter_image_y) + g_parameter_offset_y; /*and x + y */
		break;
	case VARYINT_WITH_X_MINUS_Y:
		*(double *)gene[i].addr = (g_px*g_delta_parameter_image_x + g_parameter_offset_x)-(lclpy*g_delta_parameter_image_y + g_parameter_offset_y); /*and x-y*/
		break;
	case VARYINT_RANDOM:
		*(double *)gene[i].addr += (((double)randval / RAND_MAX)*2*g_fiddle_factor) - g_fiddle_factor;
		break;
	case VARYINT_RANDOM_WEIGHTED:  /* weighted random mutation, further out = further change */
		{
			int mid = g_grid_size /2;
			double radius =  sqrt((double) (sqr(g_px - mid) + sqr(lclpy - mid)));
			*(double *)gene[i].addr += ((((double)randval / RAND_MAX)*2*g_fiddle_factor) - g_fiddle_factor)*radius;
		}
		break;
	}
return;
}

static int vary_int(int randvalue, int limit, int mode)
{
	int ret = 0;
	int lclpy = g_grid_size - g_py - 1;
	switch (mode)
	{
	default:
	case VARYINT_NONE:
		break;
	case VARYINT_WITH_X: /* vary with x */
		ret = (g_discrete_parameter_offset_x + g_px) % limit;
		break;
	case VARYINT_WITH_Y: /* vary with y */
		ret = (g_discrete_parameter_offset_y + lclpy) % limit;
		break;
	case VARYINT_WITH_X_PLUS_Y: /* vary with x + y */
		ret = (g_discrete_parameter_offset_x + g_px + g_discrete_parameter_offset_y + lclpy) % limit;
		break;
	case VARYINT_WITH_X_MINUS_Y: /* vary with x-y */
		ret = (g_discrete_parameter_offset_x + g_px)-(g_discrete_parameter_offset_y + lclpy) % limit;
		break;
	case VARYINT_RANDOM: /* random mutation */
		ret = randvalue % limit;
		break;
	case VARYINT_RANDOM_WEIGHTED:  /* weighted random mutation, further out = further change */
		{
			int mid = g_grid_size /2;
			double radius =  sqrt((double) (sqr(g_px - mid) + sqr(lclpy - mid)));
			ret = (int)((((randvalue / RAND_MAX)*2*g_fiddle_factor) - g_fiddle_factor)*radius);
			ret %= limit;
		}
		break;
	}
	return ret;
}

static int wrapped_positive_vary_int(int randvalue, int limit, int mode)
{
	int i;
	i = vary_int(randvalue, limit, mode);
	return (i < 0) ? (limit + i) : i;
}

static void vary_inside(GENEBASE gene[], int randval, int i)
{
	int choices[9] =
	{
		COLORMODE_Z_MAGNITUDE,
		COLORMODE_BEAUTY_OF_FRACTALS_60,
		COLORMODE_BEAUTY_OF_FRACTALS_61,
		COLORMODE_EPSILON_CROSS,
		COLORMODE_STAR_TRAIL,
		COLORMODE_PERIOD,
		COLORMODE_FLOAT_MODULUS_INTEGER,
		COLORMODE_INVERSE_TANGENT_INTEGER,
		-1
	};
	if (gene[i].mutate)
	{
		*(int*)gene[i].addr = choices[wrapped_positive_vary_int(randval, 9, gene[i].mutate)];
	}
	return;
}

static void vary_outside(GENEBASE gene[], int randval, int i)
{
	int choices[8] = {-1, -2, -3, -4, -5, -6, -7, -8};
	if (gene[i].mutate)
	{
		*(int*)gene[i].addr = choices[wrapped_positive_vary_int(randval, 8, gene[i].mutate)];
	}
	return;
}

static void vary_bail_out_test(GENEBASE gene[], int randval, int i)
{
	int choices[7] =
	{
		BAILOUT_MODULUS, BAILOUT_REAL, BAILOUT_IMAGINARY, BAILOUT_OR, BAILOUT_AND, BAILOUT_MANHATTAN, BAILOUT_MANHATTAN_R
	};
	if (gene[i].mutate)
	{
		*(int*)gene[i].addr = choices[wrapped_positive_vary_int(randval, 7, gene[i].mutate)];
		/* move this next bit to varybot where it belongs */
		set_bail_out_formula(g_bail_out_test);
	}
	return;
}

static void vary_power2(GENEBASE gene[], int randval, int i)
{
	int choices[9] = {0, 2, 4, 8, 16, 32, 64, 128, 256};
	if (gene[i].mutate)
	{
		*(int*)gene[i].addr = choices[wrapped_positive_vary_int(randval, 9, gene[i].mutate)];
	}
	return;
}

static void vary_function(GENEBASE gene[], int randval, int i)
{
	if (gene[i].mutate)
	{
		/* Changed following to BYTE since trigfn is an array of BYTEs and if one */
		/* of the functions isn't varied, it's value was being zeroed by the high */
		/* BYTE of the preceeding function.  JCO  2 MAY 2001 */
		*(BYTE *) gene[i].addr = (BYTE) wrapped_positive_vary_int(randval, g_num_function_list, gene[i].mutate);
	}
	/* replaced '30' with g_num_function_list, set in prompts1.c */
	set_trig_pointers(5); /*set all trig ptrs up*/
	return;
}

static void vary_invert(GENEBASE gene[], int randval, int i)
{
	if (gene[i].mutate)
	{
		vary_double(gene, randval, i);
	}
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
}

/* --------------------------------------------------------------------- */
/*
	get_evolve_params() is called from FRACTINT.C whenever the 'ctrl_e' key
	is pressed.  Return codes are:
		-1  routine was ESCAPEd - no need to re-generate the images
		0  minor variable changed.  No need to re-generate the image.
		1  major parms changed.  Re-generate the images.
*/
static int get_the_rest()
{
	int numtrig = (g_current_fractal_specific->flags >> 6) & 7;
	if (fractal_type_formula(g_fractal_type))
	{
		numtrig = g_formula_state.max_fn();
	}

choose_vars_restart:
	{
		const char *evolvmodes[] =
		{
			"no", "x", "y", "x+y", "x-y", "random", "spread"
		};
		UIChoices dialog("Variable tweak central 2 of 2", 28);
		for (int num = MAX_PARAMETERS; num < (NUMGENES - 5); num++)
		{
			dialog.push(g_genes[num].name, evolvmodes, NUM_OF(evolvmodes), g_genes[num].mutate);
		}
		for (int num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
		{
			dialog.push(g_genes[num].name, evolvmodes, NUM_OF(evolvmodes), g_genes[num].mutate);
		}
		if (g_current_fractal_specific->calculate_type == standard_fractal &&
			(g_current_fractal_specific->flags & FRACTALFLAG_BAIL_OUT_TESTS))
		{
			dialog.push(g_genes[NUMGENES - 1].name, evolvmodes, NUM_OF(evolvmodes), g_genes[NUMGENES - 1].mutate);
		}
		dialog.push("");
		dialog.push("Press F2 to set all to off");
		dialog.push("Press F3 to set all on");
		dialog.push("Press F4 to randomize all");

		int i = dialog.prompt();
		switch (i)
		{
		case FIK_F2: /* set all off */
			for (int num = MAX_PARAMETERS; num < NUMGENES; num++)
			{
				g_genes[num].mutate = VARYINT_NONE;
			}
			goto choose_vars_restart;
		case FIK_F3: /* set all on..alternate x and y for field map */
			for (int num = MAX_PARAMETERS; num < NUMGENES; num ++)
			{
				g_genes[num].mutate = (char)((num % 2) + 1);
			}
			goto choose_vars_restart;
		case FIK_F4: /* Randomize all */
			for (int num = MAX_PARAMETERS; num < NUMGENES; num ++)
			{
				g_genes[num].mutate = (char)(rand() % 6);
			}
			goto choose_vars_restart;
		case -1:
			return -1;
		default:
			break;
		}

		int k = -1;
		for (int num = MAX_PARAMETERS; num < (NUMGENES - 5); num++)
		{
			g_genes[num].mutate = (char)(dialog.values(++k).uval.ch.val);
		}

		for (int num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
		{
			g_genes[num].mutate = (char)(dialog.values(++k).uval.ch.val);
		}

		if (g_current_fractal_specific->calculate_type == standard_fractal &&
			(g_current_fractal_specific->flags & FRACTALFLAG_BAIL_OUT_TESTS))
		{
			g_genes[NUMGENES - 1].mutate = (char)(dialog.values(++k).uval.ch.val);
		}

		return 1; /* if you were here, you want to regenerate */
	}
}

static int get_variations()
{
	int firstparm = 0;
	int lastparm  = MAX_PARAMETERS;
	if (fractal_type_formula(g_fractal_type))
	{
		if (g_formula_state.uses_p1())  /* set first parameter */
		{
			firstparm = 0;
		}
		else if (g_formula_state.uses_p2())
		{
			firstparm = 2;
		}
		else if (g_formula_state.uses_p3())
		{
			firstparm = 4;
		}
		else if (g_formula_state.uses_p4())
		{
			firstparm = 6;
		}
		else
		{
			firstparm = 8; /* g_formula_state.uses_p5() or no parameter */
		}

		if (g_formula_state.uses_p5()) /* set last parameter */
		{
			lastparm = 10;
		}
		else if (g_formula_state.uses_p4())
		{
			lastparm = 8;
		}
		else if (g_formula_state.uses_p3())
		{
			lastparm = 6;
		}
		else if (g_formula_state.uses_p2())
		{
			lastparm = 4;
		}
		else
		{
			lastparm = 2; /* g_formula_state.uses_p1() or no parameter */
		}
	}

	int numparams = 0;
	for (int i = firstparm; i < lastparm; i++)
	{
		if (type_has_parameter(g_julibrot ? g_new_orbit_type : g_fractal_type, i, NULL) == 0)
		{
			if (fractal_type_formula(g_fractal_type))
			{
				if (parameter_not_used(i))
				{
					continue;
				}
			}
			break;
		}
		numparams++;
	}

	if (!fractal_type_formula(g_fractal_type))
	{
		lastparm = numparams;
	}

choose_vars_restart:
	{
		UIChoices dialog("Variable tweak central 1 of 2", 92);
		const char *evolvmodes[] = 
		{
			"no", "x", "y", "x+y", "x-y", "random", "spread"
		};
		for (int num = firstparm; num < lastparm; num++)
		{
			if (fractal_type_formula(g_fractal_type))
			{
				if (parameter_not_used(num))
				{
					continue;
				}
			}
			dialog.push(g_genes[num].name, evolvmodes, NUM_OF(evolvmodes), g_genes[num].mutate);
		}
		dialog.push("");
		dialog.push("Press F2 to set all to off");
		dialog.push("Press F3 to set all on");
		dialog.push("Press F4 to randomize all");
		dialog.push("Press F6 for second page");

		int i = dialog.prompt();
		int chngd = -1;
		switch (i)
		{
		case FIK_F2: /* set all off */
			for (int num = 0; num < MAX_PARAMETERS; num++)
			{
				g_genes[num].mutate = VARYINT_NONE;
			}
			goto choose_vars_restart;
		case FIK_F3: /* set all on..alternate x and y for field map */
			for (int num = 0; num < MAX_PARAMETERS; num ++)
			{
				g_genes[num].mutate = (char) ((num % 2) + 1);
			}
			goto choose_vars_restart;
		case FIK_F4: /* Randomize all */
			for (int num = 0; num < MAX_PARAMETERS; num ++)
			{
				g_genes[num].mutate = (char) (rand() % 6);
			}
			goto choose_vars_restart;
		case FIK_F6: /* go to second screen, put array away first */
			{
				GENEBASE save[NUMGENES];
				for (int g = 0; g < NUMGENES; g++)
				{
					save[g] = g_genes[g];
				}
				chngd = get_the_rest();
				for (int g = 0; g < NUMGENES; g++)
				{
					g_genes[g] = save[g];
				}
			}
			goto choose_vars_restart;
		case -1:
			return chngd;
		default:
			break;
		}

		int k = 0;
		for (int num = firstparm; num < lastparm; num++)
		{
			if (fractal_type_formula(g_fractal_type))
			{
				if (parameter_not_used(num))
				{
					continue;
				}
			}
			g_genes[num].mutate = (char)(dialog.values(k++).uval.ch.val);
		}

		return 1; /* if you were here, you want to regenerate */
	}
}

void set_mutation_level(int strength)
{
	/* scan through the gene array turning on random variation for all parms that */
	/* are suitable for this level of mutation */
	int i;

	for (i = 0; i < NUMGENES; i++)
	{
		g_genes[i].mutate = (g_genes[i].level <= strength) ? VARYINT_RANDOM : VARYINT_NONE; /* 5 = random mutation mode */
	}
	return;
}

int get_evolve_parameters()
{
	int old_variations = 0;
	/* fill up the previous values arrays */
	int old_evolving = g_evolving_flags;
	int old_gridsz = g_grid_size;
	double old_paramrangex = g_parameter_range_x;
	double old_paramrangey = g_parameter_range_y;
	double old_opx = g_parameter_offset_x;
	double old_opy = g_parameter_offset_y;
	double old_fiddlefactor = g_fiddle_factor;

get_evol_restart:
	int j;
	if ((g_evolving_flags & EVOLVE_RANDOM_WALK) || (g_evolving_flags & EVOLVE_RANDOM_PARAMETER))
	{
		/* adjust field param to make some sense when changing from random modes*/
		/* maybe should adjust for aspect ratio here? */
		g_parameter_range_x = g_parameter_range_y = g_fiddle_factor*2;
		g_parameter_offset_x = g_parameters[0] - g_fiddle_factor;
		g_parameter_offset_y = g_parameters[1] - g_fiddle_factor;
		/* set middle image to last selected and edges to +- g_fiddle_factor */
	}

	{
		UIChoices dialog(HELPEVOL, "Evolution Mode Options", 255);
		dialog.push("Evolution mode? (no for full screen)", (g_evolving_flags & EVOLVE_FIELD_MAP) != 0);
		dialog.push("Image grid size (odd numbers only)", g_grid_size);
		if (explore_check())  /* test to see if any parms are set to linear */
		{
			/* variation 'explore mode' */
			dialog.push("Show parameter zoom box?", (g_evolving_flags & EVOLVE_PARAMETER_BOX) != 0);
			dialog.push("x parameter range (across screen)", static_cast<float>(g_parameter_range_x));
			dialog.push("x parameter offset (left hand edge)", static_cast<float>(g_parameter_offset_x));
			dialog.push("y parameter range (up screen)", static_cast<float>(g_parameter_range_y));
			dialog.push("y parameter offset (lower edge)", static_cast<float>(g_parameter_offset_y));
		}
		dialog.push("Max random mutation", static_cast<float>(g_fiddle_factor));
		dialog.push("Mutation reduction factor (between generations)", static_cast<float>(g_fiddle_reduction));
		dialog.push("Grouting? ", (g_evolving_flags & EVOLVE_NO_GROUT) == 0);
		dialog.push("");
		dialog.push("Press F4 to reset view parameters to defaults.");
		dialog.push("Press F2 to halve mutation levels");
		dialog.push("Press F3 to double mutation levels");
		dialog.push("Press F6 to control which parameters are varied");
		int i = dialog.prompt();
		if (i < 0)
		{
			/* in case this point has been reached after calling sub menu with F6 */
			g_evolving_flags      = old_evolving;
			g_grid_size        = old_gridsz;
			g_parameter_range_x   = old_paramrangex;
			g_parameter_range_y   = old_paramrangey;
			g_parameter_offset_x           = old_opx;
			g_parameter_offset_y           = old_opy;
			g_fiddle_factor  = old_fiddlefactor;

			return -1;
		}
		switch (i)
		{
		case FIK_F4:
			set_current_parameters();
			g_fiddle_factor = 1;
			g_fiddle_reduction = 1.0;
			goto get_evol_restart;
		case FIK_F2:
			g_parameter_range_x /= 2;
			g_parameter_offset_x = g_parameter_offset_x + g_parameter_range_x/2;
			g_new_parameter_offset_y = g_parameter_offset_x;
			g_parameter_range_y /= 2;
			g_parameter_offset_y = g_parameter_offset_y + g_parameter_range_y/2;
			g_new_parameter_offset_y = g_parameter_offset_y;
			g_fiddle_factor /= 2;
			goto get_evol_restart;
		case FIK_F3:
			{
				double centerx = g_parameter_offset_x + g_parameter_range_x / 2;
				g_parameter_range_x *= 2;
				g_parameter_offset_x = g_new_parameter_offset_x = centerx - g_parameter_range_x / 2;
				double centery = g_parameter_offset_y + g_parameter_range_y / 2;
				g_parameter_range_y *= 2;
				g_parameter_offset_y = g_new_parameter_offset_y = centery - g_parameter_range_y / 2;
				g_fiddle_factor *= 2;
				goto get_evol_restart;
			}
		}
		j = i;
		int k = -1;
		g_evolving_flags = dialog.values(++k).uval.ch.val;
		g_view_window = (g_evolving_flags != 0);

		if (!g_evolving_flags && i != FIK_F6)  /* don't need any of the other parameters */
		{
			return 1;              /* the following code can set g_evolving_flags even if it's off */
		}

		g_grid_size = dialog.values(++k).uval.ival;
		/* (g_screen_width / 20), max # of subimages @ 20 pixels per subimage */
		/* MAX_GRID_SIZE == 1024 / 20 == 51 */
		g_grid_size = MathUtil::Clamp(g_grid_size, 3, std::min(MAX_GRID_SIZE, g_screen_width / (MIN_PIXELS << 1)));
		g_grid_size |= 1; /* make sure g_grid_size is odd */
		if (explore_check())
		{
			int temp = (EVOLVE_PARAMETER_BOX*dialog.values(++k).uval.ch.val);
			if (g_evolving_flags)
			{
				g_evolving_flags += temp;
			}
			g_parameter_range_x = dialog.values(++k).uval.dval;
			g_new_parameter_offset_x = g_parameter_offset_x = dialog.values(++k).uval.dval;
			g_parameter_range_y = dialog.values(++k).uval.dval;
			g_new_parameter_offset_y = g_parameter_offset_y = dialog.values(++k).uval.dval;
		}
		g_fiddle_factor = dialog.values(++k).uval.dval;
		g_fiddle_reduction = dialog.values(++k).uval.dval;
		if (!(dialog.values(++k).uval.ch.val))
		{
			g_evolving_flags |= EVOLVE_NO_GROUT;
		}
	}
	g_view_x_dots = (g_screen_width / g_grid_size)-2;
	g_view_y_dots = (g_screen_height / g_grid_size)-2;
	if (!g_view_window)
	{
		g_view_x_dots = 0;
		g_view_y_dots = 0;
	}

	int result = 0;
	if (g_evolving_flags != old_evolving
		|| (g_grid_size != old_gridsz) ||(g_parameter_range_x != old_paramrangex)
		|| (g_parameter_offset_x != old_opx) || (g_parameter_range_y != old_paramrangey)
		|| (g_parameter_offset_y != old_opy)  || (g_fiddle_factor != old_fiddlefactor)
		|| (old_variations > 0))
	{
		result = 1;
	}

	if (g_evolving_flags && !old_evolving)
	{
		save_parameter_history();
	}

	if (!g_evolving_flags && (g_evolving_flags == old_evolving))
	{
		result = 0;
	}

	if (j == FIK_F6)
	{
		old_variations = get_variations();
		set_current_parameters();
		if (old_variations > 0)
		{
			g_view_window = true;
			g_evolving_flags |= EVOLVE_FIELD_MAP;   /* leave other settings alone */
		}
		g_fiddle_factor = 1;
		g_fiddle_reduction = 1.0;
		goto get_evol_restart;
	}
	return result;
}

void setup_parameter_box()
{
	int vidsize;
	g_parameter_box_count = 0;
	g_parameter_zoom = ((double)g_grid_size-1.0)/2.0;
	/* need to allocate 2 int arrays for g_box_x and g_box_y plus 1 byte array for values */
	vidsize = (g_x_dots + g_y_dots)*4*sizeof(int);
	vidsize += g_x_dots + g_y_dots + 2;
	if (!s_parameter_box)
	{
		s_parameter_box = (int *) malloc(vidsize);
	}
	if (!s_parameter_box)
	{
		text_temp_message("Sorry...can't allocate mem for parmbox");
		g_evolving_flags = EVOLVE_NONE;
	}
	g_parameter_box_count = 0;

	/* vidsize = (vidsize / g_grid_size) + 3; */ /* allocate less mem for smaller box */
	/* taken out above as *all* pixels get plotted in small boxes */
	if (!s_image_box)
	{
		s_image_box = (int *) malloc(vidsize);
	}
	if (!s_image_box)
	{
		text_temp_message("Sorry...can't allocate mem for imagebox");
	}
}

void release_parameter_box()
{
	if (s_parameter_box)
	{
		free(s_parameter_box);
		s_parameter_box = NULL;
	}
	if (s_image_box)
	{
		free(s_image_box);
		s_image_box = NULL;
	}
}

void set_current_parameters()
{
	g_parameter_range_x = g_current_fractal_specific->x_max - g_current_fractal_specific->x_min;
	g_parameter_offset_x = g_new_parameter_offset_x = - (g_parameter_range_x / 2);
	g_parameter_range_y = g_current_fractal_specific->y_max - g_current_fractal_specific->y_min;
	g_parameter_offset_y = g_new_parameter_offset_y = - (g_parameter_range_y / 2);
	return;
}

void fiddle_parameters(GENEBASE gene[], int ecount)
{
	/* call with g_px, g_py ... parameter set co-ords*/
	/* set random seed then call rnd enough times to get to g_px, g_py */
	/* 5/2/96 adding in indirection */
	/* 26/2/96 adding in multiple methods and field map */
	/* 29/4/96 going for proper handling of the whole gene array */
	/*         bung in a pile of switches to allow for expansion to any
			future variable types */
	/* 11/6/96 scrapped most of switches above and used function pointers
			instead */
	/* 4/1/97  picking it up again after the last step broke it all horribly! */

	int i;

	/* when writing routines to vary param types make sure that rand() gets called
	the same number of times whether gene[].mutate is set or not to allow
	user to change it between generations without screwing up the duplicability
	of the sequence and starting from the wrong point */

	/* this function has got simpler and simpler throughout the construction of the
	evolver feature and now consists of just these few lines to loop through all
	the variables referenced in the gene array and call the functions required
	to vary them, aren't pointers marvellous! */

	if ((g_px == g_grid_size / 2) && (g_py == g_grid_size / 2)) /* return if middle image */
	{
		return;
	}

	set_random(ecount);   /* generate the right number of pseudo randoms */

	for (i = 0; i < NUMGENES; i++)
	{
		(*(gene[i].varyfunc))(gene, rand(), i);
	}
}

static void set_random(int ecount)
{
	/* This must be called with ecount set correctly for the spiral map. */
	/* Call this routine to set the random # to the proper value */
	/* if it may have changed, before fiddle_parameters() is called. */
	/* Now called by fiddle_parameters(). */
	int index;
	int i;

	srand(g_this_generation_random_seed);
	for (index = 0; index < ecount; index++)
	{
		for (i = 0; i < NUMGENES; i++)
		{
			rand();
		}
	}
}

static bool explore_check()
{
	/* checks through gene array to see if any of the parameters are set to */
	/* one of the non random variation modes. Used to see if g_parameter_zoom box is */
	/* needed */
	bool nonrandom = false;

	for (int i = 0; i < NUMGENES && !(nonrandom); i++)
	{
		if ((g_genes[i].mutate > VARYINT_NONE) && (g_genes[i].mutate < VARYINT_RANDOM))
		{
			nonrandom = true;
		}
	}
	return nonrandom;
}

void draw_parameter_box(int mode)
{
	/* draws parameter zoom box in evolver mode */
	/* clears boxes off screen if mode = 1, otherwise, redraws boxes */
	Coordinate tl, tr, bl, br;
	int grout;
	if (!(g_evolving_flags & EVOLVE_PARAMETER_BOX))
	{
		return; /* don't draw if not asked to! */
	}
	grout = !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
	s_image_box_count = g_box_count;
	if (g_box_count)
	{
		/* stash normal zoombox pixels */
		memcpy(&s_image_box[0], &g_box_x[0], g_box_count*sizeof(g_box_x[0]));
		memcpy(&s_image_box[g_box_count], &g_box_y[0], g_box_count*sizeof(g_box_y[0]));
		memcpy(&s_image_box[g_box_count*2], &g_box_values[0], g_box_count*sizeof(g_box_values[0]));
		clear_box(); /* to avoid probs when one box overlaps the other */
	}
	if (g_parameter_box_count != 0)   /* clear last parmbox */
	{
		g_box_count = g_parameter_box_count;
		memcpy(&g_box_x[0], &s_parameter_box[0], g_box_count*sizeof(g_box_x[0]));
		memcpy(&g_box_y[0], &s_parameter_box[g_box_count], g_box_count*sizeof(g_box_y[0]));
		memcpy(&g_box_values[0], &s_parameter_box[g_box_count*2], g_box_count*sizeof(g_box_values[0]));
		clear_box();
	}

	if (mode == 1)
	{
		g_box_count = s_image_box_count;
		g_parameter_box_count = 0;
		return;
	}

	g_box_count = 0;
	/*draw larger box to show parm zooming range */
	tl.x = bl.x = ((g_px -(int)g_parameter_zoom)*(int)(g_dx_size + 1 + grout))-g_sx_offset-1;
	tl.y = tr.y = ((g_py -(int)g_parameter_zoom)*(int)(g_dy_size + 1 + grout))-g_sy_offset-1;
	br.x = tr.x = ((g_px +1 + (int)g_parameter_zoom)*(int)(g_dx_size + 1 + grout))-g_sx_offset;
	br.y = bl.y = ((g_py +1 + (int)g_parameter_zoom)*(int)(g_dy_size + 1 + grout))-g_sy_offset;
#ifndef XFRACT
	add_box(br);
	add_box(tr);
	add_box(bl);
	add_box(tl);
	draw_lines(tl, tr, bl.x-tl.x, bl.y-tl.y);
	draw_lines(tl, bl, tr.x-tl.x, tr.y-tl.y);
#else
	g_box_x[0] = tl.x + g_sx_offset;
	g_box_y[0] = tl.y + g_sy_offset;
	g_box_x[1] = tr.x + g_sx_offset;
	g_box_y[1] = tr.y + g_sy_offset;
	g_box_x[2] = br.x + g_sx_offset;
	g_box_y[2] = br.y + g_sy_offset;
	g_box_x[3] = bl.x + g_sx_offset;
	g_box_y[3] = bl.y + g_sy_offset;
	g_box_count = 8;
#endif
	if (g_box_count)
	{
		display_box();
		/* stash pixel values for later */
		memcpy(&s_parameter_box[0], &g_box_x[0], g_box_count*sizeof(g_box_x[0]));
		memcpy(&s_parameter_box[g_box_count], &g_box_y[0], g_box_count*sizeof(g_box_y[0]));
		memcpy(&s_parameter_box[g_box_count*2], &g_box_values[0], g_box_count*sizeof(g_box_values[0]));
	}
	g_parameter_box_count = g_box_count;
	g_box_count = s_image_box_count;
	if (s_image_box_count)
	{
		/* and move back old values so that everything can proceed as normal */
		memcpy(&g_box_x[0], &s_image_box[0], g_box_count*sizeof(g_box_x[0]));
		memcpy(&g_box_y[0], &s_image_box[g_box_count], g_box_count*sizeof(g_box_y[0]));
		memcpy(&g_box_values[0], &s_image_box[g_box_count*2], g_box_count*sizeof(g_box_values[0]));
		display_box();
	}
	return;
}

void set_evolve_ranges()
{
	int lclpy = g_grid_size - g_py - 1;
	/* set up ranges and offsets for parameter explorer/evolver */
	g_parameter_range_x = g_delta_parameter_image_x*(g_parameter_zoom*2.0);
	g_parameter_range_y = g_delta_parameter_image_y*(g_parameter_zoom*2.0);
	g_new_parameter_offset_x = g_parameter_offset_x + (((double)g_px-g_parameter_zoom)*g_delta_parameter_image_x);
	g_new_parameter_offset_y = g_parameter_offset_y + (((double)lclpy-g_parameter_zoom)*g_delta_parameter_image_y);

	g_new_discrete_parameter_offset_x = g_discrete_parameter_offset_x + g_px - g_grid_size/2;
	g_new_discrete_parameter_offset_y = g_discrete_parameter_offset_y + lclpy - g_grid_size/2;
	return;
}

void spiral_map(int count)
{
	/* maps out a clockwise spiral for a prettier and possibly   */
	/* more intuitively useful order of drawing the sub images.  */
	/* All the malarky with count is to allow resuming */

	int i;
	int mid;
	int offset;
	i = 0;
	mid = g_grid_size / 2;
	if (count == 0)  /* start in the middle */
	{
		g_px = g_py = mid;
		return;
	}
	for (offset = 1; offset <= mid; offset ++)
	{
		/* first do the top row */
		g_py = (mid - offset);
		for (g_px = (mid - offset) + 1; g_px <mid + offset; g_px++)
		{
			i++;
			if (i == count)
			{
				return;
			}
		}
		/* then do the right hand column */
		for (; g_py < mid + offset; g_py++)
		{
			i++;
			if (i == count)
			{
				return;
			}
		}
		/* then reverse along the bottom row */
		for (; g_px > mid - offset; g_px--)
		{
			i++;
			if (i == count)
			{
				return;
			}
		}
		/* then up the left to finish */
		for (; g_py >= mid - offset; g_py--)
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
	/* unmaps the clockwise spiral */
	/* All this malarky is to allow selecting different subimages */
	/* Returns the count from the center subimage to the current g_px & g_py */
	int mid;
	static int last_grid_size = 0;

	mid = g_grid_size / 2;
	if ((g_px == mid && g_py == mid) || (last_grid_size != g_grid_size))
	{
		int i;
		int gridsqr;
		/* set up array and return */
		gridsqr = g_grid_size*g_grid_size;
		ecountbox[g_px][g_py] = 0;  /* we know the first one, do the rest */
		for (i = 1; i < gridsqr; i++)
		{
			spiral_map(i);
			ecountbox[g_px][g_py] = i;
		}
		last_grid_size = g_grid_size;
		g_px = mid;
		g_py = mid;
		return 0;
	}
	return ecountbox[g_px][g_py];
}
