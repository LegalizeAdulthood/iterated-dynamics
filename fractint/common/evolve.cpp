#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "fihelp.h"

#define VARYINT_NONE			0
#define VARYINT_WITH_X			1
#define VARYINT_WITH_Y			2
#define VARYINT_WITH_X_PLUS_Y	3
#define VARYINT_WITH_X_MINUS_Y	4
#define VARYINT_RANDOM			5
#define VARYINT_RANDOM_WEIGHTED	6

#define MAX_GRID_SIZE 51  /* This is arbitrary, = 1024/20 */

/* g_px and g_py are coordinates in the parameter grid (small images on screen) */
/* g_evolving = flag, g_grid_size = dimensions of image grid (g_grid_size x g_grid_size) */
int g_px;
int g_py;
int g_evolving;
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
char g_discrete_parameter_offset_x;
char g_discrete_parameter_offset_y;
char g_new_discrete_parameter_offset_x;
char g_new_discrete_parameter_offset_y;
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
	BYTE trigndx0;
	BYTE trigndx1;
	BYTE trigndx2;
	BYTE trigndx3;
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
static void vary_trig(GENEBASE gene[], int randval, int i);
static void vary_bail_out_test(GENEBASE gene[], int randval, int i);
static void vary_invert(GENEBASE gene[], int randval, int i);
static int explore_check();
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
	{ &g_trig_index[0],		vary_trig,			0, "trig function 1", 5 },
	{ &g_trig_index[1],		vary_trig,			0, "trig fn 2", 5 },
	{ &g_trig_index[2],		vary_trig,			0, "trig fn 3", 5 },
	{ &g_trig_index[3],		vary_trig,			0, "trig fn 4", 5 },
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
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3 ;
	g_trig_index[0] = s_old_history.trigndx0;
	g_trig_index[1] = s_old_history.trigndx1;
	g_trig_index[2] = s_old_history.trigndx2;
	g_trig_index[3] = s_old_history.trigndx3;
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
	s_old_history.trigndx0 = g_trig_index[0];
	s_old_history.trigndx1 = g_trig_index[1];
	s_old_history.trigndx2 = g_trig_index[2];
	s_old_history.trigndx3 = g_trig_index[3];
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
	int choices[9] = {-59, -60, -61, -100, -101, -102, -103, -104, -1};
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
	int choices[7] = {Mod, Real, Imag, Or, And, Manh, Manr};
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

static void vary_trig(GENEBASE gene[], int randval, int i)
{
	if (gene[i].mutate)
	{
		/* Changed following to BYTE since trigfn is an array of BYTEs and if one */
		/* of the functions isn't varied, it's value was being zeroed by the high */
		/* BYTE of the preceeding function.  JCO  2 MAY 2001 */
		*(BYTE *) gene[i].addr = (BYTE) wrapped_positive_vary_int(randval, g_num_trig_fn, gene[i].mutate);
	}
	/* replaced '30' with g_num_trig_fn, set in prompts1.c */
	set_trig_pointers(5); /*set all trig ptrs up*/
	return;
}

static void vary_invert(GENEBASE gene[], int randval, int i)
{
	if (gene[i].mutate)
	{
		vary_double(gene, randval, i);
	}
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3 ;
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
	char *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
	int i, k, num, numtrig;
	char *choices[20];
	struct full_screen_values uvalues[20];

	numtrig = (g_current_fractal_specific->flags >> 6) & 7;
	if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
	{
		numtrig = g_max_fn;
	}

choose_vars_restart:
	k = -1;
	for (num = MAX_PARAMETERS; num < (NUMGENES - 5); num++)
	{
		choices[++k] = g_genes[num].name;
		uvalues[k].type = 'l';
		uvalues[k].uval.ch.vlen = 7;
		uvalues[k].uval.ch.llen = 7;
		uvalues[k].uval.ch.list = evolvmodes;
		uvalues[k].uval.ch.val =  g_genes[num].mutate;
	}

	for (num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
	{
		choices[++k] = g_genes[num].name;
		uvalues[k].type = 'l';
		uvalues[k].uval.ch.vlen = 7;
		uvalues[k].uval.ch.llen = 7;
		uvalues[k].uval.ch.list = evolvmodes;
		uvalues[k].uval.ch.val =  g_genes[num].mutate;
	}

	if (g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & BAILTEST))
	{
		choices[++k] = g_genes[NUMGENES - 1].name;
		uvalues[k].type = 'l';
		uvalues[k].uval.ch.vlen = 7;
		uvalues[k].uval.ch.llen = 7;
		uvalues[k].uval.ch.list = evolvmodes;
		uvalues[k].uval.ch.val =  g_genes[NUMGENES - 1].mutate;
	}

	choices[++k]= "";
	uvalues[k].type = '*';
	choices[++k]= "Press F2 to set all to off";
	uvalues[k].type ='*';
	choices[++k]= "Press F3 to set all on";
	uvalues[k].type = '*';
	choices[++k]= "Press F4 to randomize all";
	uvalues[k].type = '*';

	i = full_screen_prompt("Variable tweak central 2 of 2", k + 1, choices, uvalues, 28, NULL);

	switch (i)
	{
	case FIK_F2: /* set all off */
		for (num = MAX_PARAMETERS; num < NUMGENES; num++)
		{
			g_genes[num].mutate = VARYINT_NONE;
		}
		goto choose_vars_restart;
	case FIK_F3: /* set all on..alternate x and y for field map */
		for (num = MAX_PARAMETERS; num < NUMGENES; num ++)
		{
			g_genes[num].mutate = (char)((num % 2) + 1);
		}
		goto choose_vars_restart;
	case FIK_F4: /* Randomize all */
		for (num = MAX_PARAMETERS; num < NUMGENES; num ++)
		{
			g_genes[num].mutate = (char)(rand() % 6);
		}
		goto choose_vars_restart;
	case -1:
		return -1;
	default:
		break;
	}

	/* read out values */
	k = -1;
	for (num = MAX_PARAMETERS; num < (NUMGENES - 5); num++)
	{
		g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);
	}

	for (num = (NUMGENES - 5); num < (NUMGENES - 5 + numtrig); num++)
	{
		g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);
	}

	if (g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & BAILTEST))
	{
		g_genes[NUMGENES - 1].mutate = (char)(uvalues[++k].uval.ch.val);
	}

	return 1; /* if you were here, you want to regenerate */
}

static int get_variations()
{
	char *evolvmodes[] = {"no", "x", "y", "x+y", "x-y", "random", "spread"};
	int i, k, num, numparams;
	char *choices[20];
	struct full_screen_values uvalues[20];
	int firstparm = 0;
	int lastparm  = MAX_PARAMETERS;
	int chngd = -1;

	if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
	{
		if (g_uses_p1)  /* set first parameter */
		{
			firstparm = 0;
		}
		else if (g_uses_p2)
		{
			firstparm = 2;
		}
		else if (g_uses_p3)
		{
			firstparm = 4;
		}
		else if (g_uses_p4)
		{
			firstparm = 6;
		}
		else
		{
			firstparm = 8; /* g_uses_p5 or no parameter */
		}

		if (g_uses_p5) /* set last parameter */
		{
			lastparm = 10;
		}
		else if (g_uses_p4)
		{
			lastparm = 8;
		}
		else if (g_uses_p3)
		{
			lastparm = 6;
		}
		else if (g_uses_p2)
		{
			lastparm = 4;
		}
		else
		{
			lastparm = 2; /* g_uses_p1 or no parameter */
		}
	}

	numparams = 0;
	for (i = firstparm; i < lastparm; i++)
	{
		if (type_has_parameter(g_julibrot ? g_new_orbit_type : g_fractal_type, i, NULL) == 0)
		{
			if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
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

	if (g_fractal_type != FRACTYPE_FORMULA && g_fractal_type != FRACTYPE_FORMULA_FP)
	{
		lastparm = numparams;
	}

choose_vars_restart:
	k = -1;
	for (num = firstparm; num < lastparm; num++)
	{
		if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
		{
			if (parameter_not_used(num))
			{
				continue;
			}
		}
		choices[++k] = g_genes[num].name;
		uvalues[k].type = 'l';
		uvalues[k].uval.ch.vlen = 7;
		uvalues[k].uval.ch.llen = 7;
		uvalues[k].uval.ch.list = evolvmodes;
		uvalues[k].uval.ch.val =  g_genes[num].mutate;
	}

	choices[++k]= "";
	uvalues[k].type = '*';
	choices[++k]= "Press F2 to set all to off";
	uvalues[k].type ='*';
	choices[++k]= "Press F3 to set all on";
	uvalues[k].type = '*';
	choices[++k]= "Press F4 to randomize all";
	uvalues[k].type = '*';
	choices[++k]= "Press F6 for second page"; /* F5 gets eaten */
	uvalues[k].type = '*';

	i = full_screen_prompt("Variable tweak central 1 of 2", k + 1, choices, uvalues, 92, NULL);

	switch (i)
	{
	case FIK_F2: /* set all off */
		for (num = 0; num < MAX_PARAMETERS; num++)
		{
			g_genes[num].mutate = VARYINT_NONE;
		}
		goto choose_vars_restart;
	case FIK_F3: /* set all on..alternate x and y for field map */
		for (num = 0; num < MAX_PARAMETERS; num ++)
		{
			g_genes[num].mutate = (char)((num % 2) + 1);
		}
		goto choose_vars_restart;
	case FIK_F4: /* Randomize all */
		for (num = 0; num < MAX_PARAMETERS; num ++)
		{
			g_genes[num].mutate = (char)(rand() % 6);
		}
		goto choose_vars_restart;
	case FIK_F6: /* go to second screen, put array away first */
		{
			GENEBASE save[NUMGENES];
			int g;

			for (g = 0; g < NUMGENES; g++)
			{
				save[g] = g_genes[g];
			}
			chngd = get_the_rest();
			for (g = 0; g < NUMGENES; g++)
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

	/* read out values */
	k = -1;
	for (num = firstparm; num < lastparm; num++)
	{
		if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
		{
			if (parameter_not_used(num))
			{
				continue;
			}
		}
		g_genes[num].mutate = (char)(uvalues[++k].uval.ch.val);
	}

	return 1; /* if you were here, you want to regenerate */
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
	char *choices[20];
	struct full_screen_values uvalues[20];
	int i, j, k, tmp;
	int old_evolving, old_gridsz;
	int old_variations = 0;
	double old_paramrangex, old_paramrangey, old_opx, old_opy, old_fiddlefactor;

	/* fill up the previous values arrays */
	old_evolving      = g_evolving;
	old_gridsz        = g_grid_size;
	old_paramrangex   = g_parameter_range_x;
	old_paramrangey   = g_parameter_range_y;
	old_opx           = g_parameter_offset_x;
	old_opy           = g_parameter_offset_y;
	old_fiddlefactor  = g_fiddle_factor;

get_evol_restart:
	if ((g_evolving & EVOLVE_RAND_WALK) || (g_evolving & EVOLVE_RAND_PARAM))
	{
		/* adjust field param to make some sense when changing from random modes*/
		/* maybe should adjust for aspect ratio here? */
		g_parameter_range_x = g_parameter_range_y = g_fiddle_factor*2;
		g_parameter_offset_x = g_parameters[0] - g_fiddle_factor;
		g_parameter_offset_y = g_parameters[1] - g_fiddle_factor;
		/* set middle image to last selected and edges to +- g_fiddle_factor */
	}

	k = -1;

	choices[++k]= "Evolution mode? (no for full screen)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_evolving & EVOLVE_FIELD_MAP;

	choices[++k]= "Image grid size (odd numbers only)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_grid_size;

	if (explore_check())  /* test to see if any parms are set to linear */
	{
		/* variation 'explore mode' */
		choices[++k]= "Show parameter zoom box?";
		uvalues[k].type = 'y';
		uvalues[k].uval.ch.val = ((g_evolving & EVOLVE_PARM_BOX) / EVOLVE_PARM_BOX);

		choices[++k]= "x parameter range (across screen)";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_parameter_range_x;

		choices[++k]= "x parameter offset (left hand edge)";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_parameter_offset_x;

		choices[++k]= "y parameter range (up screen)";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_parameter_range_y;

		choices[++k]= "y parameter offset (lower edge)";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_parameter_offset_y;
	}

	choices[++k]= "Max random mutation";
	uvalues[k].type = 'f';
	uvalues[k].uval.dval = g_fiddle_factor;

	choices[++k]= "Mutation reduction factor (between generations)";
	uvalues[k].type = 'f';
	uvalues[k].uval.dval = g_fiddle_reduction;

	choices[++k]= "Grouting? ";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = !((g_evolving & EVOLVE_NO_GROUT) / EVOLVE_NO_GROUT);

	choices[++k]= "";
	uvalues[k].type = '*';

	choices[++k]= "Press F4 to reset view parameters to defaults.";
	uvalues[k].type = '*';

	choices[++k]= "Press F2 to halve mutation levels";
	uvalues[k].type = '*';

	choices[++k]= "Press F3 to double mutation levels" ;
	uvalues[k].type ='*';

	choices[++k]= "Press F6 to control which parameters are varied";
	uvalues[k].type = '*';
	i = full_screen_prompt_help(HELPEVOL, "Evolution Mode Options", k + 1, choices, uvalues, 255, NULL);
	if (i < 0)
	{
		/* in case this point has been reached after calling sub menu with F6 */
		g_evolving      = old_evolving;
		g_grid_size        = old_gridsz;
		g_parameter_range_x   = old_paramrangex;
		g_parameter_range_y   = old_paramrangey;
		g_parameter_offset_x           = old_opx;
		g_parameter_offset_y           = old_opy;
		g_fiddle_factor  = old_fiddlefactor;

		return -1;
	}

	if (i == FIK_F4)
	{
		set_current_parameters();
		g_fiddle_factor = 1;
		g_fiddle_reduction = 1.0;
		goto get_evol_restart;
	}
	if (i == FIK_F2)
	{
		g_parameter_range_x = g_parameter_range_x / 2;
		g_parameter_offset_x = g_new_parameter_offset_y = g_parameter_offset_x + g_parameter_range_x / 2;
		g_parameter_range_y = g_parameter_range_y / 2;
		g_parameter_offset_y = g_new_parameter_offset_y = g_parameter_offset_y + g_parameter_range_y / 2;
		g_fiddle_factor = g_fiddle_factor / 2;
		goto get_evol_restart;
	}
	if (i == FIK_F3)
	{
		double centerx, centery;
		centerx = g_parameter_offset_x + g_parameter_range_x / 2;
		g_parameter_range_x = g_parameter_range_x*2;
		g_parameter_offset_x = g_new_parameter_offset_x = centerx - g_parameter_range_x / 2;
		centery = g_parameter_offset_y + g_parameter_range_y / 2;
		g_parameter_range_y = g_parameter_range_y*2;
		g_parameter_offset_y = g_new_parameter_offset_y = centery - g_parameter_range_y / 2;
		g_fiddle_factor = g_fiddle_factor*2;
		goto get_evol_restart;
	}

	j = i;

	/* now check out the results (*hopefully* in the same order <grin>) */

	k = -1;

	g_view_window = g_evolving = uvalues[++k].uval.ch.val;

	if (!g_evolving && i != FIK_F6)  /* don't need any of the other parameters JCO 12JUL2002 */
	{
		return 1;              /* the following code can set g_evolving even if it's off */
	}

	g_grid_size = uvalues[++k].uval.ival;
	tmp = g_screen_width / (MIN_PIXELS << 1);
	/* (g_screen_width / 20), max # of subimages @ 20 pixels per subimage */
	/* MAX_GRID_SIZE == 1024 / 20 == 51 */
	if (g_grid_size > MAX_GRID_SIZE)
	{
		g_grid_size = MAX_GRID_SIZE;
	}
	if (g_grid_size > tmp)
	{
		g_grid_size = tmp;
	}
	if (g_grid_size < 3)
	{
		g_grid_size = 3;
	}
	g_grid_size |= 1; /* make sure g_grid_size is odd */
	if (explore_check())
	{
		tmp = (EVOLVE_PARM_BOX*uvalues[++k].uval.ch.val);
		if (g_evolving)
		{
			g_evolving += tmp;
		}
		g_parameter_range_x = uvalues[++k].uval.dval;
		g_new_parameter_offset_x = g_parameter_offset_x = uvalues[++k].uval.dval;
		g_parameter_range_y = uvalues[++k].uval.dval;
		g_new_parameter_offset_y = g_parameter_offset_y = uvalues[++k].uval.dval;
	}

	g_fiddle_factor = uvalues[++k].uval.dval;

	g_fiddle_reduction = uvalues[++k].uval.dval;

	if (!(uvalues[++k].uval.ch.val))
	{
		g_evolving |= EVOLVE_NO_GROUT;
	}

	g_view_x_dots = (g_screen_width / g_grid_size)-2;
	g_view_y_dots = (g_screen_height / g_grid_size)-2;
	if (!g_view_window)
	{
		g_view_x_dots = g_view_y_dots = 0;
	}

	i = 0;

	if (g_evolving != old_evolving
		|| (g_grid_size != old_gridsz) ||(g_parameter_range_x != old_paramrangex)
		|| (g_parameter_offset_x != old_opx) || (g_parameter_range_y != old_paramrangey)
		|| (g_parameter_offset_y != old_opy)  || (g_fiddle_factor != old_fiddlefactor)
		|| (old_variations > 0))
	{
		i = 1;
	}

	if (g_evolving && !old_evolving)
	{
		save_parameter_history();
	}

	if (!g_evolving && (g_evolving == old_evolving))
	{
		i = 0;
	}

	if (j == FIK_F6)
	{
		old_variations = get_variations();
		set_current_parameters();
		if (old_variations > 0)
		{
			g_view_window = 1;
			g_evolving |= EVOLVE_FIELD_MAP;   /* leave other settings alone */
		}
		g_fiddle_factor = 1;
		g_fiddle_reduction = 1.0;
		goto get_evol_restart;
	}
	return i;
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
		g_evolving = EVOLVE_NONE;
	}
	g_parameter_box_count = 0;

	/* vidsize = (vidsize / g_grid_size) + 3 ; */ /* allocate less mem for smaller box */
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
	int index, i;

	srand(g_this_generation_random_seed);
	for (index = 0; index < ecount; index++)
	{
		for (i = 0; i < NUMGENES; i++)
		{
			rand();
		}
	}
}

static int explore_check()
{
	/* checks through gene array to see if any of the parameters are set to */
	/* one of the non random variation modes. Used to see if g_parameter_zoom box is */
	/* needed */
	int nonrandom = FALSE;
	int i;

	for (i = 0; i < NUMGENES && !(nonrandom); i++)
	{
		if ((g_genes[i].mutate > VARYINT_NONE) && (g_genes[i].mutate < VARYINT_RANDOM))
		{
			nonrandom = TRUE;
		}
	}
	return nonrandom;
}

void draw_parameter_box(int mode)
{
	/* draws parameter zoom box in evolver mode */
	/* clears boxes off screen if mode = 1, otherwise, redraws boxes */
	struct coords tl, tr, bl, br;
	int grout;
	if (!(g_evolving & EVOLVE_PARM_BOX))
	{
		return; /* don't draw if not asked to! */
	}
	grout = !((g_evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
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

	g_new_discrete_parameter_offset_x = (char)(g_discrete_parameter_offset_x + (g_px-g_grid_size/2));
	g_new_discrete_parameter_offset_y = (char)(g_discrete_parameter_offset_y + (lclpy-g_grid_size/2));
	return;
}

void spiral_map(int count)
{
	/* maps out a clockwise spiral for a prettier and possibly   */
	/* more intuitively useful order of drawing the sub images.  */
	/* All the malarky with count is to allow resuming */

	int i, mid, offset;
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
		int i, gridsqr;
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
