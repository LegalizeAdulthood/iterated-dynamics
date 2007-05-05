/*
		Various routines that prompt for things.
*/

#include <string.h>
#include <ctype.h>

#ifndef XFRACT
#include <io.h>
#elif !defined(__386BSD__) && !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>

#ifdef DIRENT
#include <dirent.h>
#elif !defined(__SVR4)
#include <sys/dir.h>
#else
#include <dirent.h>
#ifndef DIRENT
#define DIRENT
#endif
#endif

#endif

#if !defined(__386BSD__)
#if !defined(_WIN32)
#include <malloc.h>
#endif
#endif

#ifdef XFRACT
#include <fcntl.h>
#endif

#ifdef __hpux
#include <sys/param.h>
#endif

#ifdef __SVR4
#include <sys/param.h>
#endif

#if defined(_WIN32)
#include <direct.h>
#endif

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"
#include "busy.h"
#include "EscapeTime.h"
#include "SoundState.h"

/* Routines in this module      */

static  int check_f6_key(int curkey, int choice);
static  int filename_speedstr(int, int, int, char *, int);
static  int get_screen_corners();

/* speed key state values */
#define MATCHING         0      /* string matches list - speed key mode */
#define TEMPLATE        -2      /* wild cards present - buiding template */
#define SEARCHPATH      -3      /* no match - building path search name */

#define   FILEATTR       0x37      /* File attributes; select all but volume labels */
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16
#define   MAXNUMFILES    2977L

struct DIR_SEARCH g_dta;          /* Allocate DTA and define structure */

/* --------------------------------------------------------------------- */
/*
		get_toggles() is called from FRACTINT.C whenever the 'x' key
		is pressed.  This routine prompts for several options,
		sets the appropriate variables, and returns the following code
		to the calling routine:

		-1  routine was ESCAPEd - no need to re-generate the image.
			0  nothing changed, or minor variable such as "overwrite=".
				No need to re-generate the image.
			1  major variable changed (such as "inside=").  Re-generate
				the image.

		Finally, remember to insert variables in the list *and* check
		for them in the same order!!!
*/

int get_toggles()
{
	char *choices[20];
	char prevsavename[FILE_MAX_DIR + 1];
	char *savenameptr;
	struct full_screen_values uvalues[25];
	int i, j, k;
	char old_usr_stdcalcmode;
	long old_maxit, old_logflag;
	int old_inside, old_outside, old_soundflag;
	int old_biomorph, old_decomp;
	int old_fillcolor;
	int old_stoppass;
	double old_closeprox;
	char *calcmodes[] = {"1", "2", "3", "g", "g1", "g2", "g3", "g4", "g5", "g6", "b", "s", "t", "d", "o"};
	char *soundmodes[5] = {"off", "beep", "x", "y", "z"};
	char *insidemodes[] = {"numb", "maxiter", "zmag", "bof60", "bof61", "epsiloncross",
						"startrail", "period", "atan", "fmod"};
	char *outsidemodes[] = {"numb", "iter", "real", "imag", "mult", "summ", "atan",
						"fmod", "tdis"};

	k = -1;

	choices[++k] = "Passes (1,2,3, g[uess], b[ound], t[ess], d[iffu], o[rbit])";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 3;
	uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
	uvalues[k].uval.ch.list = calcmodes;
	uvalues[k].uval.ch.val = (g_user_standard_calculation_mode == '1') ? 0
						: (g_user_standard_calculation_mode == '2') ? 1
						: (g_user_standard_calculation_mode == '3') ? 2
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 0) ? 3
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 1) ? 4
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 2) ? 5
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 3) ? 6
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 4) ? 7
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 5) ? 8
						: (g_user_standard_calculation_mode == 'g' && g_stop_pass == 6) ? 9
						: (g_user_standard_calculation_mode == 'b') ? 10
						: (g_user_standard_calculation_mode == 's') ? 11
						: (g_user_standard_calculation_mode == 't') ? 12
						: (g_user_standard_calculation_mode == 'd') ? 13
						:        /* "o"rbits */      14;
	old_usr_stdcalcmode = g_user_standard_calculation_mode;
	old_stoppass = g_stop_pass;
#ifndef XFRACT
	choices[++k] = "Floating Point Algorithm";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_user_float_flag;
#endif
	choices[++k] = "Maximum Iterations (2 to 2,147,483,647)";
	uvalues[k].type = 'L';
	uvalues[k].uval.Lval = old_maxit = g_max_iteration;

	choices[++k] = "Inside Color (0-# of colors, if Inside=numb)";
	uvalues[k].type = 'i';
	if (g_inside >= 0)
	{
		uvalues[k].uval.ival = g_inside;
	}
	else
	{
		uvalues[k].uval.ival = 0;
	}

	choices[++k] = "Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 12;
	uvalues[k].uval.ch.llen = sizeof(insidemodes)/sizeof(*insidemodes);
	uvalues[k].uval.ch.list = insidemodes;
	if (g_inside >= 0)  /* numb */
	{
		uvalues[k].uval.ch.val = 0;
	}
	else if (g_inside == -1)  /* maxiter */
	{
		uvalues[k].uval.ch.val = 1;
	}
	else if (g_inside == ZMAG)
	{
		uvalues[k].uval.ch.val = 2;
	}
	else if (g_inside == BOF60)
	{
		uvalues[k].uval.ch.val = 3;
	}
	else if (g_inside == BOF61)
	{
		uvalues[k].uval.ch.val = 4;
	}
	else if (g_inside == EPSCROSS)
	{
		uvalues[k].uval.ch.val = 5;
	}
	else if (g_inside == STARTRAIL)
	{
		uvalues[k].uval.ch.val = 6;
	}
	else if (g_inside == PERIOD)
	{
		uvalues[k].uval.ch.val = 7;
	}
	else if (g_inside == ATANI)
	{
		uvalues[k].uval.ch.val = 8;
	}
	else if (g_inside == FMODI)
	{
		uvalues[k].uval.ch.val = 9;
	}
	old_inside = g_inside;

	choices[++k] = "Outside Color (0-# of colors, if Outside=numb)";
	uvalues[k].type = 'i';
	if (g_outside >= 0)
	{
		uvalues[k].uval.ival = g_outside;
	}
	else
	{
		uvalues[k].uval.ival = 0;
	}

	choices[++k] = "Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 4;
	uvalues[k].uval.ch.llen = sizeof(outsidemodes)/sizeof(*outsidemodes);
	uvalues[k].uval.ch.list = outsidemodes;
	if (g_outside >= 0)  /* numb */
	{
		uvalues[k].uval.ch.val = 0;
	}
	else
	{
		uvalues[k].uval.ch.val = -g_outside;
	}
	old_outside = g_outside;

	choices[++k] = "Savename (.GIF implied)";
	uvalues[k].type = 's';
	strcpy(prevsavename, g_save_name);
	savenameptr = strrchr(g_save_name, SLASHC);
	if (savenameptr == NULL)
	{
		savenameptr = g_save_name;
	}
	else
	{
		savenameptr++; /* point past slash */
	}
	strcpy(uvalues[k].uval.sval, savenameptr);

	choices[++k] = "File Overwrite ('overwrite=')";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_fractal_overwrite;

	choices[++k] = "Sound (off, beep, x, y, z)";
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 4;
	uvalues[k].uval.ch.llen = 5;
	uvalues[k].uval.ch.list = soundmodes;
	old_soundflag = g_sound_state.flags();
	uvalues[k].uval.ch.val = old_soundflag & SOUNDFLAG_ORBITMASK;

	if (g_ranges_length == 0)
	{
		choices[++k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)";
		uvalues[k].type = 'L';
		}
	else
	{
		choices[++k] = "Log Palette (n/a, ranges= parameter is in effect)";
		uvalues[k].type = '*';
		}
	uvalues[k].uval.Lval = old_logflag = g_log_palette_flag;

	choices[++k] = "Biomorph Color (-1 means OFF)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_biomorph = g_user_biomorph;

	choices[++k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_decomp = g_decomposition[0];

	choices[++k] = "Fill Color (normal,#) (works with passes=t, b and d)";
	uvalues[k].type = 's';
	if (g_fill_color < 0)
	{
		strcpy(uvalues[k].uval.sval, "normal");
	}
	else
	{
		sprintf(uvalues[k].uval.sval, "%d", g_fill_color);
	}
	old_fillcolor = g_fill_color;

	choices[++k] = "Proximity value for inside=epscross and fmod";
	uvalues[k].type = 'f'; /* should be 'd', but prompts get messed up JCO */
	uvalues[k].uval.dval = old_closeprox = g_proximity;

	i = full_screen_prompt_help(HELPXOPTS, "Basic Options\n(not all combinations make sense)", k + 1, choices, uvalues, 0, NULL);
	if (i < 0)
	{
		return -1;
		}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;
	j = 0;   /* return code */

	g_user_standard_calculation_mode = calcmodes[uvalues[++k].uval.ch.val][0];
	g_stop_pass = (int)calcmodes[uvalues[k].uval.ch.val][1] - (int)'0';

	if (g_stop_pass < 0 || g_stop_pass > 6 || g_user_standard_calculation_mode != 'g')
	{
		g_stop_pass = 0;
	}

	if (g_user_standard_calculation_mode == 'o' && g_fractal_type == FRACTYPE_LYAPUNOV) /* Oops, lyapunov type */
										/* doesn't use 'new' & breaks orbits */
	{
		g_user_standard_calculation_mode = old_usr_stdcalcmode;
	}

	if (old_usr_stdcalcmode != g_user_standard_calculation_mode)
	{
		j++;
	}
	if (old_stoppass != g_stop_pass)
	{
		j++;
	}
#ifndef XFRACT
	if (uvalues[++k].uval.ch.val != g_user_float_flag)
	{
		g_user_float_flag = (char)uvalues[k].uval.ch.val;
		j++;
	}
#endif
	++k;
	g_max_iteration = uvalues[k].uval.Lval;
	if (g_max_iteration < 0)
	{
		g_max_iteration = old_maxit;
	}
	if (g_max_iteration < 2)
	{
		g_max_iteration = 2;
	}

	if (g_max_iteration != old_maxit)
	{
		j++;
	}

	g_inside = uvalues[++k].uval.ival;
	if (g_inside < 0)
	{
		g_inside = -g_inside;
	}
	if (g_inside >= g_colors)
	{
		g_inside = (g_inside % g_colors) + (g_inside / g_colors);
	}

	{
		int tmp;
		tmp = uvalues[++k].uval.ch.val;
		if (tmp > 0)
		{
			switch (tmp)
			{
			case 1:
				g_inside = -1;  /* maxiter */
				break;
			case 2:
				g_inside = ZMAG;
				break;
			case 3:
				g_inside = BOF60;
				break;
			case 4:
				g_inside = BOF61;
				break;
			case 5:
				g_inside = EPSCROSS;
				break;
			case 6:
				g_inside = STARTRAIL;
				break;
			case 7:
				g_inside = PERIOD;
				break;
			case 8:
				g_inside = ATANI;
				break;
			case 9:
				g_inside = FMODI;
				break;
			}
		}
	}
	if (g_inside != old_inside)
	{
		j++;
	}

	g_outside = uvalues[++k].uval.ival;
	if (g_outside < 0)
	{
		g_outside = -g_outside;
	}
	if (g_outside >= g_colors)
	{
		g_outside = (g_outside % g_colors) + (g_outside / g_colors);
	}

	{
		int tmp;
		tmp = uvalues[++k].uval.ch.val;
		if (tmp > 0)
		{
			g_outside = -tmp;
		}
	}
	if (g_outside != old_outside)
	{
		j++;
	}

	strcpy(savenameptr, uvalues[++k].uval.sval);
	if (strcmp(g_save_name, prevsavename))
	{
		g_resave_flag = RESAVE_NO;
		g_started_resaves = FALSE; /* forget pending increment */
	}
	g_fractal_overwrite = uvalues[++k].uval.ch.val;

	g_sound_state.set_flags((g_sound_state.flags() & ~SOUNDFLAG_ORBITMASK) | uvalues[++k].uval.ch.val);
	if ((g_sound_state.flags() != old_soundflag)
		&& ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP
			|| (old_soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP))
	{
		j++;
	}

	g_log_palette_flag = uvalues[++k].uval.Lval;
	if (g_log_palette_flag != old_logflag)
	{
		j++;
		g_log_automatic_flag = FALSE;  /* turn it off, use the supplied value */
	}

	g_user_biomorph = uvalues[++k].uval.ival;
	if (g_user_biomorph >= g_colors)
	{
		g_user_biomorph = (g_user_biomorph % g_colors) + (g_user_biomorph / g_colors);
	}
	if (g_user_biomorph != old_biomorph)
	{
		j++;
	}

	g_decomposition[0] = uvalues[++k].uval.ival;
	if (g_decomposition[0] != old_decomp)
	{
		j++;
	}

	if (strncmp(strlwr(uvalues[++k].uval.sval), "normal", 4) == 0)
	{
		g_fill_color = -1;
	}
	else
	{
		g_fill_color = atoi(uvalues[k].uval.sval);
	}
	if (g_fill_color < 0)
	{
		g_fill_color = -1;
	}
	if (g_fill_color >= g_colors)
	{
		g_fill_color = (g_fill_color % g_colors) + (g_fill_color / g_colors);
	}
	if (g_fill_color != old_fillcolor)
	{
		j++;
	}

	++k;
	g_proximity = uvalues[k].uval.dval;
	if (g_proximity != old_closeprox)
	{
		j++;
	}

	return j;
}

/*
		get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
	char *choices[18];

	struct full_screen_values uvalues[23];
	int i, j, k;

	int old_rotate_lo, old_rotate_hi;
	int old_distestwidth;
	double old_potparam[3], old_inversion[3];
	long old_usr_distest;

	/* fill up the choices (and previous values) arrays */
	k = -1;

	choices[++k] = "Look for finite attractor (0=no,>0=yes,<0=phase)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ch.val = g_finite_attractor;

	choices[++k] = "Potential Max Color (0 means off)";
	uvalues[k].type = 'i';
	old_potparam[0] = g_potential_parameter[0];
	uvalues[k].uval.ival = (int) old_potparam[0];

	choices[++k] = "          Slope";
	uvalues[k].type = 'd';
	uvalues[k].uval.dval = old_potparam[1] = g_potential_parameter[1];

	choices[++k] = "          Bailout";
	uvalues[k].type = 'i';
	old_potparam[2] = g_potential_parameter[2];
	uvalues[k].uval.ival = (int) old_potparam[2];

	choices[++k] = "          16 bit values";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_potential_16bit;

	choices[++k] = "Distance Estimator (0=off, <0=edge, >0=on):";
	uvalues[k].type = 'L';
	uvalues[k].uval.Lval = old_usr_distest = g_user_distance_test;

	choices[++k] = "          width factor:";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_distestwidth = g_distance_test_width;

	choices[++k] = "Inversion radius or \"auto\" (0 means off)";
	choices[++k] = "          center X coordinate or \"auto\"";
	choices[++k] = "          center Y coordinate or \"auto\"";
	k = k - 3;
	for (i = 0; i < 3; i++)
	{
		uvalues[++k].type = 's';
		old_inversion[i] = g_inversion[i];
		if (g_inversion[i] == AUTOINVERT)
		{
			sprintf(uvalues[k].uval.sval, "auto");
		}
		else
		{
			sprintf(uvalues[k].uval.sval, "%-1.15lg", g_inversion[i]);
		}
	}
	choices[++k] = "  (use fixed radius & center when zooming)";
	uvalues[k].type = '*';

	choices[++k] = "Color cycling from color (0 ... 254)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_rotate_lo = g_rotate_lo;

	choices[++k] = "              to   color (1 ... 255)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_rotate_hi = g_rotate_hi;

	i = full_screen_prompt_help(HELPYOPTS, "Extended Options\n"
		"(not all combinations make sense)",
		k + 1, choices, uvalues, 0, NULL);
	if (i < 0)
	{
		return -1;
	}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;
	j = 0;   /* return code */

	if (uvalues[++k].uval.ch.val != g_finite_attractor)
	{
		g_finite_attractor = uvalues[k].uval.ch.val;
		j = 1;
	}

	g_potential_parameter[0] = uvalues[++k].uval.ival;
	if (g_potential_parameter[0] != old_potparam[0])
	{
		j = 1;
	}

	g_potential_parameter[1] = uvalues[++k].uval.dval;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[1] != old_potparam[1])
	{
		j = 1;
	}

	g_potential_parameter[2] = uvalues[++k].uval.ival;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != old_potparam[2])
	{
		j = 1;
	}

	if (uvalues[++k].uval.ch.val != g_potential_16bit)
	{
		g_potential_16bit = uvalues[k].uval.ch.val;
		if (g_potential_16bit)  /* turned it on */
		{
			if (g_potential_parameter[0] != 0.0)
			{
				j = 1;
			}
		}
		else /* turned it off */
		{
			if (!driver_diskp()) /* ditch the disk video */
			{
				disk_end();
			}
			else /* keep disk video, but ditch the fraction part at end */
			{
				g_disk_16bit = 0;
			}
		}
	}

	++k;
	g_user_distance_test = uvalues[k].uval.Lval;
	if (g_user_distance_test != old_usr_distest)
	{
		j = 1;
	}
	++k;
	g_distance_test_width = uvalues[k].uval.ival;
	if (g_user_distance_test && g_distance_test_width != old_distestwidth)
	{
		j = 1;
	}

	for (i = 0; i < 3; i++)
	{
		if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
		{
			g_inversion[i] = AUTOINVERT;
		}
		else
		{
			g_inversion[i] = atof(uvalues[k].uval.sval);
		}
		if (old_inversion[i] != g_inversion[i]
			&& (i == 0 || g_inversion[0] != 0.0))
		{
			j = 1;
		}
	}
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
	++k;

	g_rotate_lo = uvalues[++k].uval.ival;
	g_rotate_hi = uvalues[++k].uval.ival;
	if (g_rotate_lo < 0 || g_rotate_hi > 255 || g_rotate_lo > g_rotate_hi)
	{
		g_rotate_lo = old_rotate_lo;
		g_rotate_hi = old_rotate_hi;
	}

	return j;
}


/*
     passes_options invoked by <p> key
*/

int passes_options()
{
	char *choices[20];
	char *passcalcmodes[] = {"rect", "line"};

	struct full_screen_values uvalues[25];
	int i, j, k;
	int ret;

	int old_periodicity, old_orbit_delay, old_orbit_interval;
	int old_keep_scrn_coords;
	int old_drawmode;

	ret = 0;

pass_option_restart:
	/* fill up the choices (and previous values) arrays */
	k = -1;

	choices[++k] = "Periodicity (0=off, <0=show, >0=on, -255..+255)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_periodicity = g_user_periodicity_check;

	choices[++k] = "Orbit delay (0 = none)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_orbit_delay = g_orbit_delay;

	choices[++k] = "Orbit interval (1 ... 255)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = old_orbit_interval = (int)g_orbit_interval;

	choices[++k] = "Maintain screen coordinates";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = old_keep_scrn_coords = g_keep_screen_coords;

	choices[++k] = "Orbit pass shape (rect,line)";
	/* TODO: change to below when function mode works: */
	/* choices[++k] = "Orbit pass shape (rect,line,func)"; */
	uvalues[k].type = 'l';
	uvalues[k].uval.ch.vlen = 5;
	uvalues[k].uval.ch.llen = sizeof(passcalcmodes)/sizeof(*passcalcmodes);
	uvalues[k].uval.ch.list = passcalcmodes;
	uvalues[k].uval.ch.val = g_orbit_draw_mode;
	old_drawmode = g_orbit_draw_mode;

	i = full_screen_prompt_help(HELPPOPTS, "Passes Options\n"
		"(not all combinations make sense)\n"
		"(Press "FK_F2" for corner parameters)\n"
		"(Press "FK_F6" for calculation parameters)", k + 1, choices, uvalues, 0x44, NULL);
	if (i < 0)
	{
		return -1;
	}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;
	j = 0;   /* return code */

	g_user_periodicity_check = uvalues[++k].uval.ival;
	if (g_user_periodicity_check > 255)
	{
		g_user_periodicity_check = 255;
	}
	if (g_user_periodicity_check < -255)
	{
		g_user_periodicity_check = -255;
	}
	if (g_user_periodicity_check != old_periodicity)
	{
		j = 1;
	}


	g_orbit_delay = uvalues[++k].uval.ival;
	if (g_orbit_delay != old_orbit_delay)
	{
		j = 1;
	}


	g_orbit_interval = uvalues[++k].uval.ival;
	if (g_orbit_interval > 255)
	{
		g_orbit_interval = 255;
	}
	if (g_orbit_interval < 1)
	{
		g_orbit_interval = 1;
	}
	if (g_orbit_interval != old_orbit_interval)
	{
		j = 1;
	}

	g_keep_screen_coords = uvalues[++k].uval.ch.val;
	if (g_keep_screen_coords != old_keep_scrn_coords)
	{
		j = 1;
	}
	if (g_keep_screen_coords == 0)
	{
		g_set_orbit_corners = 0;
	}

	g_orbit_draw_mode = uvalues[++k].uval.ch.val;
	if (g_orbit_draw_mode != old_drawmode)
	{
		j = 1;
	}

	if (i == FIK_F2)
	{
		if (get_screen_corners() > 0)
		{
			ret = 1;
		}
		if (j)
		{
			ret = 1;
		}
		goto pass_option_restart;
	}

	if (i == FIK_F6)
	{
		if (get_corners() > 0)
		{
			ret = 1;
		}
		if (j)
		{
			ret = 1;
		}
		goto pass_option_restart;
	}

	return j + ret;
}


/* for videomodes added new options "virtual x/y" that change "sx/g_y_dots" */
/* for diskmode changed "viewx/g_y_dots" to "virtual x/y" that do as above  */
/* (since for diskmode they were updated by x/g_y_dots that should be the   */
/* same as sx/g_y_dots for that mode)                                       */
/* g_video_table and g_video_entry are now updated even for non-disk modes     */

/* --------------------------------------------------------------------- */
/*
	get_view_params() is called from FRACTINT.C whenever the 'v' key
	is pressed.  Return codes are:
		-1  routine was ESCAPEd - no need to re-generate the image.
			0  minor variable changed.  No need to re-generate the image.
			1  View changed.  Re-generate the image.
*/

int get_view_params()
{
	char *choices[16];
	struct full_screen_values uvalues[25];
	int i, k;
	float old_viewreduction, old_aspectratio;
	int old_viewwindow, old_viewxdots, old_viewydots, old_sxdots, old_sydots;
	int x_max, y_max;

	driver_get_max_screen(x_max, y_max);

	old_viewwindow    = g_view_window;
	old_viewreduction = g_view_reduction;
	old_aspectratio   = g_final_aspect_ratio;
	old_viewxdots     = g_view_x_dots;
	old_viewydots     = g_view_y_dots;
	old_sxdots        = g_screen_width;
	old_sydots        = g_screen_height;

get_view_restart:
	/* fill up the previous values arrays */
	k = -1;

	if (!driver_diskp())
	{
		choices[++k] = "Preview display? (no for full screen)";
		uvalues[k].type = 'y';
		uvalues[k].uval.ch.val = g_view_window;

		choices[++k] = "Auto window size reduction factor";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_view_reduction;

		choices[++k] = "Final media overall aspect ratio, y/x";
		uvalues[k].type = 'f';
		uvalues[k].uval.dval = g_final_aspect_ratio;

		choices[++k] = "Crop starting coordinates to new aspect ratio?";
		uvalues[k].type = 'y';
		uvalues[k].uval.ch.val = g_view_crop;

		choices[++k] = "Explicit size x pixels (0 for auto size)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_view_x_dots;

		choices[++k] = "              y pixels (0 to base on aspect ratio)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_view_y_dots;
	}
	else
	{
		choices[++k] = "Disk Video x pixels";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_screen_width;

		choices[++k] = "           y pixels";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_screen_height;
	}

	choices[++k] = "";
	uvalues[k].type = '*';

	if (!driver_diskp())
	{
		choices[++k] = "Press F4 to reset view parameters to defaults.";
		uvalues[k].type = '*';
	}

	i = full_screen_prompt_help(HELPVIEW, "View Window Options", k + 1, choices, uvalues, 16, NULL);
	if (i < 0)
	{
		return -1;
	}

	if (i == FIK_F4 && !driver_diskp())
	{
		g_view_window = g_view_x_dots = g_view_y_dots = 0;
		g_view_reduction = 4.2f;
		g_view_crop = 1;
		g_final_aspect_ratio = g_screen_aspect_ratio;
		g_screen_width = old_sxdots;
		g_screen_height = old_sydots;
		goto get_view_restart;
	}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;

	if (!driver_diskp())
	{
		g_view_window = uvalues[++k].uval.ch.val;
		g_view_reduction = (float) uvalues[++k].uval.dval;
		g_final_aspect_ratio = (float) uvalues[++k].uval.dval;
		g_view_crop = uvalues[++k].uval.ch.val;
		g_view_x_dots = uvalues[++k].uval.ival;
		g_view_y_dots = uvalues[++k].uval.ival;
	}
	else
	{
		g_screen_width = uvalues[++k].uval.ival;
		g_screen_height = uvalues[++k].uval.ival;
		if ((x_max != -1) && (g_screen_width > x_max))
		{
			g_screen_width = (int) x_max;
		}
		if (g_screen_width < 2)
		{
			g_screen_width = 2;
		}
		if ((y_max != -1) && (g_screen_height > y_max))
		{
			g_screen_height = y_max;
		}
		if (g_screen_height < 2)
		{
			g_screen_height = 2;
		}
	}
	++k;

	if (!driver_diskp())
	{
		if (g_view_x_dots != 0 && g_view_y_dots != 0 && g_view_window && g_final_aspect_ratio == 0.0)
		{
			g_final_aspect_ratio = ((float) g_view_y_dots)/((float) g_view_x_dots);
		}
		else if (g_final_aspect_ratio == 0.0 && (g_view_x_dots == 0 || g_view_y_dots == 0))
		{
			g_final_aspect_ratio = old_aspectratio;
		}

		if (g_final_aspect_ratio != old_aspectratio && g_view_crop)
		{
			aspect_ratio_crop(old_aspectratio, g_final_aspect_ratio);
		}
	}
	else
	{
		g_video_entry.x_dots = g_screen_width;
		g_video_entry.y_dots = g_screen_height;
		g_final_aspect_ratio = ((float) g_screen_height)/((float) g_screen_width);
		memcpy(&g_video_table[g_adapter], &g_video_entry, sizeof(g_video_entry));
	}

	return (g_view_window != old_viewwindow
		|| g_screen_width != old_sxdots || g_screen_height != old_sydots
		|| (g_view_window
			&& (g_view_reduction != old_viewreduction
				|| g_final_aspect_ratio != old_aspectratio
				|| g_view_x_dots != old_viewxdots
				|| (g_view_y_dots != old_viewydots && g_view_x_dots)))) ? 1 : 0;
}

/*
	get_command_string() is called from FRACTINT.C whenever the 'g' key
	is pressed.  Return codes are:
		-1  routine was ESCAPEd - no need to re-generate the image.
			0  parameter changed, no need to regenerate
		>0  parameter changed, regenerate
*/

int get_command_string()
{
	int i;
	static char cmdbuf[61];

	i = field_prompt_help(HELPCOMMANDS, "Enter command string to use.", NULL, cmdbuf, 60, NULL);
	if (i >= 0 && cmdbuf[0] != 0)
	{
		i = process_command(cmdbuf, CMDFILE_AT_AFTER_STARTUP);
		if (DEBUGFLAG_REAL_POPCORN == g_debug_flag)
		{
			backwards_v18();
			backwards_v19();
			backwards_v20();
		}
	}

	return i;
}


/* --------------------------------------------------------------------- */

int g_gaussian_distribution = 30, g_gaussian_offset = 0, g_gaussian_slope = 25;
long g_gaussian_constant;


double starfield_values[4] =
{
	30.0, 100.0, 5.0, 0.0
};

char g_grey_file[] = "altern.map";

int starfield()
{
	int c;
	BusyMarker marker;
	if (starfield_values[0] <   1.0)
	{
		starfield_values[0] =   1.0;
	}
	if (starfield_values[0] > 100.0)
	{
		starfield_values[0] = 100.0;
	}
	if (starfield_values[1] <   1.0)
	{
		starfield_values[1] =   1.0;
	}
	if (starfield_values[1] > 100.0)
	{
		starfield_values[1] = 100.0;
	}
	if (starfield_values[2] <   1.0)
	{
		starfield_values[2] =   1.0;
	}
	if (starfield_values[2] > 100.0)
	{
		starfield_values[2] = 100.0;
	}

	g_gaussian_distribution = (int)(starfield_values[0]);
	g_gaussian_constant  = (long)(((starfield_values[1]) / 100.0)*(1L << 16));
	g_gaussian_slope = (int)(starfield_values[2]);

	if (validate_luts(g_grey_file) != 0)
	{
		stop_message(0, "Unable to load ALTERN.MAP");
		return -1;
	}
	spindac(0, 1);                 /* load it, but don't spin */
	for (g_row = 0; g_row < g_y_dots; g_row++)
	{
		for (g_col = 0; g_col < g_x_dots; g_col++)
		{
			if (driver_key_pressed())
			{
				driver_buzzer(BUZZER_INTERRUPT);
				return 1;
			}
			c = getcolor(g_col, g_row);
			if (c == g_inside)
			{
				c = g_colors-1;
			}
			g_put_color(g_col, g_row, gaussian_number(c, g_colors));
		}
	}
	driver_buzzer(BUZZER_COMPLETE);
	return 0;
}

int get_starfield_params()
{
	struct full_screen_values uvalues[3];
	int i;
	char *starfield_prompts[3] =
	{
		"Star Density in Pixels per Star",
		"Percent Clumpiness",
		"Ratio of Dim stars to Bright"
	};

	if (g_colors < 255)
	{
		stop_message(0, "starfield requires 256 color mode");
		return -1;
	}
	for (i = 0; i < 3; i++)
	{
		uvalues[i].uval.dval = starfield_values[i];
		uvalues[i].type = 'f';
	}
	driver_stack_screen();
	i = full_screen_prompt_help(HELPSTARFLD, "Starfield Parameters", 3, starfield_prompts, uvalues, 0, NULL);
	driver_unstack_screen();
	if (i < 0)
	{
		return -1;
	}
	for (i = 0; i < 3; i++)
	{
		starfield_values[i] = uvalues[i].uval.dval;
	}

	return 0;
}

static char *masks[] = {"*.pot", "*.gif"};

int get_random_dot_stereogram_parameters()
{
	char rds6[60];
	char *stereobars[] = {"none", "middle", "top"};
	struct full_screen_values uvalues[7];
	char *rds_prompts[7] =
	{
		"Depth Effect (negative reverses front and back)",
		"Image width in inches",
		"Use grayscale value for depth? (if \"no\" uses color number)",
		"Calibration bars",
		"Use image map? (if \"no\" uses random dots)",
		"  If yes, use current image map name? (see below)",
		rds6
	};
	int i, k;
	int ret;
	static char reuse = 0;
	driver_stack_screen();
	while (1)
	{
		ret = 0;

		k = 0;
		uvalues[k].uval.ival = g_auto_stereo_depth;
		uvalues[k++].type = 'i';

		uvalues[k].uval.dval = g_auto_stereo_width;
		uvalues[k++].type = 'f';

		uvalues[k].uval.ch.val = g_grayscale_depth;
		uvalues[k++].type = 'y';

		uvalues[k].type = 'l';
		uvalues[k].uval.ch.list = stereobars;
		uvalues[k].uval.ch.vlen = 6;
		uvalues[k].uval.ch.llen = 3;
		uvalues[k++].uval.ch.val  = g_calibrate;

		uvalues[k].uval.ch.val = g_image_map;
		uvalues[k++].type = 'y';


		if (*g_stereo_map_name != 0 && g_image_map)
		{
			char *p;
			uvalues[k].uval.ch.val = reuse;
			uvalues[k++].type = 'y';

			uvalues[k++].type = '*';
			for (i = 0; i < sizeof(rds6); i++)
			{
				rds6[i] = ' ';
			}
			p = strrchr(g_stereo_map_name, SLASHC);
			if (p == NULL ||
				(int) strlen(g_stereo_map_name) < sizeof(rds6)-2)
			{
				p = strlwr(g_stereo_map_name);
			}
			else
			{
				p++;
			}
			/* center file name */
			rds6[(sizeof(rds6)-(int) strlen(p) + 2)/2] = 0;
			strcat(rds6, "[");
			strcat(rds6, p);
			strcat(rds6, "]");
		}
		else
		{
			*g_stereo_map_name = 0;
		}
		i = full_screen_prompt_help(HELPRDS, "Random Dot Stereogram Parameters", k, rds_prompts, uvalues, 0, NULL);
		if (i < 0)
		{
			ret = -1;
			break;
		}
		else
		{
			k = 0;
			g_auto_stereo_depth = uvalues[k++].uval.ival;
			g_auto_stereo_width = uvalues[k++].uval.dval;
			g_grayscale_depth = uvalues[k++].uval.ch.val;
			g_calibrate        = (char)uvalues[k++].uval.ch.val;
			g_image_map        = (char)uvalues[k++].uval.ch.val;
			if (*g_stereo_map_name && g_image_map)
			{
				reuse         = (char)uvalues[k++].uval.ch.val;
			}
			else
			{
				reuse = 0;
			}
			if (g_image_map && !reuse)
			{
				if (get_a_filename("Select an Imagemap File", masks[1], g_stereo_map_name))
				{
					continue;
				}
			}
		}
		break;
	}
	driver_unstack_screen();
	return ret;
}

int get_a_number(double *x, double *y)
{
	char *choices[2];

	struct full_screen_values uvalues[2];
	int i, k;

	driver_stack_screen();

	/* fill up the previous values arrays */
	k = -1;

	choices[++k] = "X coordinate at cursor";
	uvalues[k].type = 'd';
	uvalues[k].uval.dval = *x;

	choices[++k] = "Y coordinate at cursor";
	uvalues[k].type = 'd';
	uvalues[k].uval.dval = *y;

	i = full_screen_prompt("Set Cursor Coordinates", k + 1, choices, uvalues, 25, NULL);
	if (i < 0)
	{
		driver_unstack_screen();
		return -1;
		}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;

	*x = uvalues[++k].uval.dval;
	*y = uvalues[++k].uval.dval;

	driver_unstack_screen();
	return i;
}

/* --------------------------------------------------------------------- */

int get_commands()              /* execute commands from file */
{
	int ret;
	FILE *parmfile;
	long point;
	static char commandmask[13] = {"*.par"};

	ret = 0;
	point = get_file_entry_help(HELPPARMFILE, GETFILE_PARAMETER, "Parameter Set",
		commandmask, g_command_file, g_command_name);
	if (point >= 0)
	{
		parmfile = fopen(g_command_file, "rb");
		if (parmfile != NULL)
		{
			fseek(parmfile, point, SEEK_SET);
			ret = load_commands(parmfile);
		}
	}
	return ret;
}

/* --------------------------------------------------------------------- */

void goodbye()                  /* we done.  Bail out */
{
	char goodbyemessage[40] = "   Thank You for using "FRACTINT;
	int ret;

	line_3d_free();
	if (g_map_dac_box)
	{
		free(g_map_dac_box);
		g_map_dac_box = NULL;
	}
	if (g_resume_info != NULL)
	{
		end_resume();
	}
	if (g_evolve_handle != NULL)
	{
		free(g_evolve_handle);
	}
	release_parameter_box();
	history_free();
	if (g_ifs_definition != NULL)
	{
		free(g_ifs_definition);
		g_ifs_definition = NULL;
	}
	free_ant_storage();
	disk_end();
	ExitCheck();
#ifdef WINFRACT
	return;
#endif
	if (*g_make_par != 0)
	{
		driver_set_for_text();
	}
#ifdef XFRACT
	UnixDone();
	printf("\n\n\n%s\n", goodbyemessage); /* printf takes pointer */
#endif
	if (*g_make_par != 0)
	{
		driver_move_cursor(6, 0);
	}
	stop_slide_show();
	end_help();
	ret = 0;
	if (g_initialize_batch == INITBATCH_BAILOUT_ERROR) /* exit with error code for batch file */
	{
		ret = 2;
	}
	else if (g_initialize_batch == INITBATCH_BAILOUT_INTERRUPTED)
	{
		ret = 1;
	}
	DriverManager::close_drivers();
#if defined(_WIN32)
	_CrtDumpMemoryLeaks();
#endif
	exit(ret);
}

#if 0
void heap_sort(void *ra1, int n, unsigned sz, int (__cdecl *fct)(VOIDPTR arg1, VOIDPTR arg2))
{
	int ll, j, ir, i;
	void *rra;
	char *ra;
	ra = (char *)ra1;
	ra -= sz;
	ll = (n >> 1) + 1;
	ir = n;

	while (1)
	{
		if (ll > 1)
		{
			rra = *((char **)(ra + (--ll)*sz));
		}
		else
		{
			rra = *((char **)(ra + ir*sz));
			*((char **)(ra + ir*sz)) = *((char **)(ra + sz));
			if (--ir == 1)
			{
				*((char **)(ra + sz)) = rra;
				return;
			}
		}
		i = ll;
		j = ll <<1;
		while (j <= ir)
		{
			if (j < ir && (fct(ra + j*sz, ra + (j + 1)*sz) < 0))
			{
				++j;
			}
			if (fct(&rra, ra + j*sz) < 0)
			{
				*((char **)(ra + i*sz)) = *((char **)(ra + j*sz));
				j += (i = j);
			}
			else
			{
				j = ir + 1;
			}
		}
		*((char **)(ra + i*sz)) = rra;
	}
}
#endif

struct tagCHOICE
{
	char name[13];
	char full_name[FILE_MAX_PATH];
	char type;
};
typedef struct tagCHOICE CHOICE;

int lccompare(VOIDPTR arg1, VOIDPTR arg2) /* for sort */
{
	char **choice1 = (char **) arg1;
	char **choice2 = (char **) arg2;

	return stricmp(*choice1, *choice2);
}


static int speedstate;
int get_a_filename(char *hdg, char *file_template, char *flname)
{
	int rds;  /* if getting an RDS image map */
	char instr[80];
	int masklen;
	char filename[FILE_MAX_PATH]; /* 13 is big enough for Fractint, but not Xfractint */
	char speedstr[81];
	char tmpmask[FILE_MAX_PATH];   /* used to locate next file in list */
	char old_flname[FILE_MAX_PATH];
	int i, j;
	int out;
	int retried;
	/* Only the first 13 characters of file names are displayed... */
	CHOICE storage[MAXNUMFILES];
	CHOICE *choices[MAXNUMFILES];
	int attributes[MAXNUMFILES];
	int filecount;   /* how many files */
	int dircount;    /* how many directories */
	int notroot;     /* not the root directory */
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char temp1[256];

	static int numtemplates = 1;
	static int dosort = 1;

	rds = (g_stereo_map_name == flname) ? 1 : 0;
	for (i = 0; i < MAXNUMFILES; i++)
	{
		attributes[i] = 1;
		choices[i] = &storage[i];
	}
	/* save filename */
	strcpy(old_flname, flname);

restart:  /* return here if template or directory changes */
	tmpmask[0] = 0;
	if (flname[0] == 0)
	{
		strcpy(flname, DOTSLASH);
	}
	split_path(flname , drive, dir, fname, ext);
	make_path(filename, ""   , "" , fname, ext);
	retried = 0;

retry_dir:
	if (dir[0] == 0)
	{
		strcpy(dir, ".");
	}
	expand_dirname(dir, drive);
	make_path(tmpmask, drive, dir, "", "");
	fix_dir_name(tmpmask);
	if (retried == 0 && strcmp(dir, SLASH) && strcmp(dir, DOTSLASH))
	{
		j = (int) strlen(tmpmask) - 1;
		tmpmask[j] = 0; /* strip trailing \ */
		if (strchr(tmpmask, '*') || strchr(tmpmask, '?')
			|| fr_find_first(tmpmask) != 0
			|| (g_dta.attribute & SUBDIR) == 0)
		{
			strcpy(dir, DOTSLASH);
			++retried;
			goto retry_dir;
		}
		tmpmask[j] = SLASHC;
	}
	if (file_template[0])
	{
		numtemplates = 1;
		split_path(file_template, NULL, NULL, fname, ext);
	}
	else
	{
		numtemplates = sizeof(masks)/sizeof(masks[0]);
	}
	filecount = -1;
	dircount  = 0;
	notroot   = 0;
	j = 0;
	masklen = (int) strlen(tmpmask);
	strcat(tmpmask, "*.*");
	out = fr_find_first(tmpmask);
	while (out == 0 && filecount < MAXNUMFILES)
	{
		if ((g_dta.attribute & SUBDIR) && strcmp(g_dta.filename, "."))
		{
			if (strcmp(g_dta.filename, ".."))
			{
				strcat(g_dta.filename, SLASH);
			}
			strncpy(choices[++filecount]->name, g_dta.filename, 13);
			choices[filecount]->name[12] = 0;
			choices[filecount]->type = 1;
			strcpy(choices[filecount]->full_name, g_dta.filename);
			dircount++;
			if (strcmp(g_dta.filename, "..") == 0)
			{
				notroot = 1;
			}
		}
		out = fr_find_next();
	}
	tmpmask[masklen] = 0;
	if (file_template[0])
	{
		make_path(tmpmask, drive, dir, fname, ext);
	}
	do
	{
		if (numtemplates > 1)
		{
			strcpy(&(tmpmask[masklen]), masks[j]);
		}
		out = fr_find_first(tmpmask);
		while (out == 0 && filecount < MAXNUMFILES)
		{
			if (!(g_dta.attribute & SUBDIR))
			{
				if (rds)
				{
					put_string_center(2, 0, 80, C_GENERAL_INPUT, g_dta.filename);

					split_path(g_dta.filename, NULL, NULL, fname, ext);
					/* just using speedstr as a handy buffer */
					make_path(speedstr, drive, dir, fname, ext);
					strncpy(choices[++filecount]->name, g_dta.filename, 13);
					choices[filecount]->type = 0;
				}
				else
				{
					strncpy(choices[++filecount]->name, g_dta.filename, 13);
					choices[filecount]->type = 0;
					strcpy(choices[filecount]->full_name, g_dta.filename);
				}
			}
			out = fr_find_next();
		}
	}
	while (++j < numtemplates);
	if (++filecount == 0)
	{
		strcpy(choices[filecount]->name, "*nofiles*");
		choices[filecount]->type = 0;
		++filecount;
	}

	strcpy(instr, "Press " FK_F6 " for default directory, " FK_F4 " to toggle sort ");
	if (dosort)
	{
		strcat(instr, "off");
		shell_sort(&choices, filecount, sizeof(CHOICE *), lccompare); /* sort file list */
	}
	else
	{
		strcat(instr, "on");
	}
	if (notroot == 0 && dir[0] && dir[0] != SLASHC) /* must be in root directory */
	{
		split_path(tmpmask, drive, dir, fname, ext);
		strcpy(dir, SLASH);
		make_path(tmpmask, drive, dir, fname, ext);
	}
	if (numtemplates > 1)
	{
		strcat(tmpmask, " ");
		strcat(tmpmask, masks[0]);
	}
	sprintf(temp1, "%s\nTemplate: %s", hdg, tmpmask);
	strcpy(speedstr, filename);
	if (speedstr[0] == 0)
	{
		for (i = 0; i < filecount; i++) /* find first file */
		{
			if (choices[i]->type == 0)
			{
				break;
			}
		}
		if (i >= filecount)
		{
			i = 0;
		}
	}


	i = full_screen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
		temp1, NULL, instr, filecount, (char **) choices,
		attributes, 5, 99, 12, i, NULL, speedstr, filename_speedstr, check_f6_key);
	if (i == -FIK_F4)
	{
		dosort = 1 - dosort;
		goto restart;
	}
	if (i == -FIK_F6)
	{
		static int lastdir = 0;
		if (lastdir == 0)
		{
			strcpy(dir, g_fract_dir1);
		}
		else
		{
			strcpy(dir, g_fract_dir2);
		}
		fix_dir_name(dir);
		make_path(flname, drive, dir, "", "");
		lastdir = 1 - lastdir;
		goto restart;
	}
	if (i < 0)
	{
		/* restore filename */
		strcpy(flname, old_flname);
		return -1;
	}
	if (speedstr[0] == 0 || speedstate == MATCHING)
	{
		if (choices[i]->type)
		{
			if (strcmp(choices[i]->name, "..") == 0) /* go up a directory */
			{
				if (strcmp(dir, DOTSLASH) == 0)
				{
					strcpy(dir, DOTDOTSLASH);
				}
				else
				{
					char *s = strrchr(dir, SLASHC);
					if (s != NULL) /* trailing slash */
					{
						*s = 0;
						s = strrchr(dir, SLASHC);
						if (s != NULL)
						{
							*(s + 1) = 0;
						}
					}
				}
			}
			else  /* go down a directory */
			{
				strcat(dir, choices[i]->full_name);
			}
			fix_dir_name(dir);
			make_path(flname, drive, dir, "", "");
			goto restart;
		}
		split_path(choices[i]->full_name, NULL, NULL, fname, ext);
		make_path(flname, drive, dir, fname, ext);
	}
	else
	{
		if (speedstate == SEARCHPATH
			&& strchr(speedstr, '*') == 0 && strchr(speedstr, '?') == 0
			&& ((fr_find_first(speedstr) == 0
			&& (g_dta.attribute & SUBDIR))|| strcmp(speedstr, SLASH) == 0)) /* it is a directory */
		{
			speedstate = TEMPLATE;
		}

		if (speedstate == TEMPLATE)
		{
			/* extract from tempstr the pathname and template information,
				being careful not to overwrite drive and directory if not
				newly specified */
			char drive1[FILE_MAX_DRIVE];
			char dir1[FILE_MAX_DIR];
			char fname1[FILE_MAX_FNAME];
			char ext1[FILE_MAX_EXT];
			split_path(speedstr, drive1, dir1, fname1, ext1);
			if (drive1[0])
			{
				strcpy(drive, drive1);
			}
			if (dir1[0])
			{
				strcpy(dir, dir1);
			}
			make_path(flname, drive, dir, fname1, ext1);
			if (strchr(fname1, '*') || strchr(fname1, '?') ||
				strchr(ext1,   '*') || strchr(ext1,   '?'))
			{
				make_path(file_template, "", "", fname1, ext1);
			}
			else if (is_a_directory(flname))
			{
				fix_dir_name(flname);
			}
			goto restart;
		}
		else /* speedstate == SEARCHPATH */
		{
			char fullpath[FILE_MAX_DIR];
			findpath(speedstr, fullpath);
			if (fullpath[0])
			{
				strcpy(flname, fullpath);
			}
			else
			{  /* failed, make diagnostic useful: */
				strcpy(flname, speedstr);
				if (strchr(speedstr, SLASHC) == NULL)
				{
					split_path(speedstr, NULL, NULL, fname, ext);
					make_path(flname, drive, dir, fname, ext);
				}
			}
		}
	}
	make_path(g_browse_name, "", "", fname, ext);
	return 0;
}

#ifdef __CLINT__
#pragma argsused
#endif

static int check_f6_key(int curkey, int choice)
{ /* choice is dummy used by other routines called by full_screen_choice() */
	choice = 0; /* to suppress warning only */
	if (curkey == FIK_F6)
	{
		return -FIK_F6;
	}
	else if (curkey == FIK_F4)
	{
		return -FIK_F4;
	}
	return 0;
}

static int filename_speedstr(int row, int col, int vid,
							char *speedstring, int speed_match)
{
	char *prompt;
	if (strchr(speedstring, ':')
		|| strchr(speedstring, '*') || strchr(speedstring, '*')
		|| strchr(speedstring, '?'))
	{
		speedstate = TEMPLATE;  /* template */
		prompt = "File Template";
	}
	else if (speed_match)
	{
		speedstate = SEARCHPATH; /* does not match list */
		prompt = "Search Path for";
	}
	else
	{
		speedstate = MATCHING;
		prompt = "Speed key string";
	}
	driver_put_string(row, col, vid, prompt);
	return (int) strlen(prompt);
}

#if !defined(_WIN32)
int is_a_directory(char *s)
{
	int len;
	char sv;
#ifdef _MSC_VER
	unsigned attrib = 0;
#endif
	if (strchr(s, '*') || strchr(s, '?'))
	{
		return 0; /* for my purposes, not a directory */
	}

	len = (int) strlen(s);
	if (len > 0)
	{
		sv = s[len-1];   /* last char */
	}
	else
	{
		sv = 0;
	}

#ifdef _MSC_VER
	if (_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
	{
		return 1;  /* not a directory or doesn't exist */
	}
	else if (sv == SLASHC)
	{
		/* strip trailing slash and try again */
		s[len-1] = 0;
		if (_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
		{
			s[len-1] = sv;
			return 1;
		}
		s[len-1] = sv;
	}
	return 0;
#else
	if (fr_find_first(s) != 0) /* couldn't find it */
	{
		/* any better ideas?? */
		if (sv == SLASHC) /* we'll guess it is a directory */
		{
			return 1;
		}
		else
		{
			return 0;  /* no slashes - we'll guess it's a file */
		}
	}
	else if ((g_dta.attribute & SUBDIR) != 0)
	{
		if (sv == SLASHC)
		{
			/* strip trailing slash and try again */
			s[len-1] = 0;
			if (fr_find_first(s) != 0) /* couldn't find it */
			{
				return 0;
			}
			else if ((g_dta.attribute & SUBDIR) != 0)
			{
				return 1;   /* we're SURE it's a directory */
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 1;   /* we're SURE it's a directory */
		}
	}
	return 0;
#endif
}
#endif

#ifndef XFRACT  /* This routine moved to unix.c so we can use it in hc.c */

int split_path(const char *file_template, char *drive, char *dir, char *fname, char *ext)
{
	int length;
	int len;
	int offset;
	const char *tmp;
	if (drive)
	{
		drive[0] = 0;
	}
	if (dir)
	{
		dir[0]   = 0;
	}
	if (fname)
	{
		fname[0] = 0;
	}
	if (ext)
	{
		ext[0]   = 0;
	}

	length = (int) strlen(file_template);
	if (length == 0)
	{
		return 0;
	}

	offset = 0;

	/* get drive */
	if (length >= 2)
	{
		if (file_template[1] == ':')
		{
			if (drive)
			{
				drive[0] = file_template[offset++];
				drive[1] = file_template[offset++];
				drive[2] = 0;
			}
			else
			{
				offset++;
				offset++;
			}
		}
	}

	/* get dir */
	if (offset < length)
	{
		tmp = strrchr(file_template, SLASHC);
		if (tmp)
		{
			tmp++;  /* first character after slash */
			len = (int) (tmp - (char *)&file_template[offset]);
			if (len >= 0 && len < FILE_MAX_DIR && dir)
			{
				strncpy(dir, &file_template[offset], min(len, FILE_MAX_DIR));
			}
			if (len < FILE_MAX_DIR && dir)
			{
				dir[len] = 0;
			}
			offset += len;
		}
	}
	else
	{
		return 0;
	}

	/* get fname */
	if (offset < length)
	{
		tmp = strrchr(file_template, '.');
		if (tmp < strrchr(file_template, SLASHC) || tmp < strrchr(file_template, ':'))
		{
			tmp = 0; /* in this case the '.' must be a directory */
		}
		if (tmp)
		{
			/* tmp++; */ /* first character past "." */
			len = (int) (tmp - (char *)&file_template[offset]);
			if ((len > 0) && (offset + len < length) && fname)
			{
				strncpy(fname, &file_template[offset], min(len, FILE_MAX_FNAME));
				if (len < FILE_MAX_FNAME)
				{
					fname[len] = 0;
				}
				else
				{
					fname[FILE_MAX_FNAME-1] = 0;
				}
			}
			offset += len;
			if ((offset < length) && ext)
			{
				strncpy(ext, &file_template[offset], FILE_MAX_EXT);
				ext[FILE_MAX_EXT-1] = 0;
			}
		}
		else if ((offset < length) && fname)
		{
			strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
			fname[FILE_MAX_FNAME-1] = 0;
		}
	}
	return 0;
}
#endif

int make_path(char *template_str, char *drive, char *dir, char *fname, char *ext)
{
	if (template_str)
	{
		*template_str = 0;
	}
	else
	{
		return -1;
	}
#ifndef XFRACT
	if (drive)
	{
		strcpy(template_str, drive);
	}
#endif
	if (dir)
	{
		strcat(template_str, dir);
	}
	if (fname)
	{
		strcat(template_str, fname);
	}
	if (ext)
	{
		strcat(template_str, ext);
	}
	return 0;
}


/* fix up directory names */
void fix_dir_name(char *dirname)
{
	int length = (int) strlen(dirname); /* index of last character */

	/* make sure dirname ends with a slash */
	if (length > 0)
	{
		if (dirname[length-1] == SLASHC)
		{
			return;
		}
	}
	strcat(dirname, SLASH);
}

static void dir_name(char *target, const char *dir, const char *name)
{
	*target = 0;
	if (*dir != 0)
	{
		strcpy(target, dir);
	}
	strcat(target, name);
}

/* removes file in dir directory */
int dir_remove(char *dir, char *filename)
{
	char tmp[FILE_MAX_PATH];
	dir_name(tmp, dir, filename);
	return remove(tmp);
}

/* fopens file in dir directory */
FILE *dir_fopen(const char *dir, const char *filename, const char *mode)
{
	char tmp[FILE_MAX_PATH];
	dir_name(tmp, dir, filename);
	return fopen(tmp, mode);
}

/*
	See if double value was changed by input screen. Problem is that the
	conversion from double to string and back can make small changes
	in the value, so will it twill test as "different" even though it
	is not
*/
int cmpdbl(double old, double new_value)
{
	char buf[81];
	struct full_screen_values val;

	/* change the old value with the same torture the new value had */
	val.type = 'd'; /* most values on this screen are type d */
	val.uval.dval = old;
	prompt_value_string(buf, &val);   /* convert "old" to string */

	old = atof(buf);                /* convert back */
	return fabs(old-new_value) < DBL_EPSILON?0:1;  /* zero if same */
}

int get_corners()
{
	struct full_screen_values values[15];
	char *prompts[15];
	char xprompt[] = "          X";
	char yprompt[] = "          Y";
	char zprompt[] = "          Z";
	int i, nump, prompt_ret;
	int cmag;
	double Xctr, Yctr;
	LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
	double Xmagfactor, Rotation, Skew;
	BYTE ousemag;
	double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;

	ousemag = g_use_center_mag;
	oxxmin = g_escape_time_state.m_grid_fp.x_min();
	oxxmax = g_escape_time_state.m_grid_fp.x_max();
	oyymin = g_escape_time_state.m_grid_fp.y_min();
	oyymax = g_escape_time_state.m_grid_fp.y_max();
	oxx3rd = g_escape_time_state.m_grid_fp.x_3rd();
	oyy3rd = g_escape_time_state.m_grid_fp.y_3rd();

gc_loop:
	for (i = 0; i < 15; ++i)
	{
		values[i].type = 'd'; /* most values on this screen are type d */
	}
	cmag = g_use_center_mag;
	if (g_orbit_draw_mode == ORBITDRAW_LINE)
	{
		cmag = 0;
	}
	convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

	nump = -1;
	if (cmag)
	{
		prompts[++nump]= "Center X";
		values[nump].uval.dval = Xctr;
		prompts[++nump]= "Center Y";
		values[nump].uval.dval = Yctr;
		prompts[++nump]= "Magnification";
		values[nump].uval.dval = (double)Magnification;
		prompts[++nump]= "X Magnification Factor";
		values[nump].uval.dval = Xmagfactor;
		prompts[++nump]= "Rotation Angle (degrees)";
		values[nump].uval.dval = Rotation;
		prompts[++nump]= "Skew Angle (degrees)";
		values[nump].uval.dval = Skew;
		prompts[++nump]= "";
		values[nump].type = '*';
		prompts[++nump]= "Press "FK_F7" to switch to \"corners\" mode";
		values[nump].type = '*';
		}

	else
	{
		if (g_orbit_draw_mode == ORBITDRAW_LINE)
		{
			prompts[++nump]= "Left End Point";
			values[nump].type = '*';
			prompts[++nump] = xprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.x_min();
			prompts[++nump] = yprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.y_max();
			prompts[++nump]= "Right End Point";
			values[nump].type = '*';
			prompts[++nump] = xprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.x_max();
			prompts[++nump] = yprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.y_min();
		}
		else
		{
			prompts[++nump]= "Top-Left Corner";
			values[nump].type = '*';
			prompts[++nump] = xprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.x_min();
			prompts[++nump] = yprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.y_max();
			prompts[++nump]= "Bottom-Right Corner";
			values[nump].type = '*';
			prompts[++nump] = xprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.x_max();
			prompts[++nump] = yprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.y_min();
			if (g_escape_time_state.m_grid_fp.x_min() == g_escape_time_state.m_grid_fp.x_3rd() && g_escape_time_state.m_grid_fp.y_min() == g_escape_time_state.m_grid_fp.y_3rd())
			{
				g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.y_3rd() = 0;
			}
			prompts[++nump]= "Bottom-left (zeros for top-left X, bottom-right Y)";
			values[nump].type = '*';
			prompts[++nump] = xprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.x_3rd();
			prompts[++nump] = yprompt;
			values[nump].uval.dval = g_escape_time_state.m_grid_fp.y_3rd();
			prompts[++nump]= "Press "FK_F7" to switch to \"center-mag\" mode";
			values[nump].type = '*';
		}
	}

	prompts[++nump]= "Press "FK_F4" to reset to type default values";
	values[nump].type = '*';

	prompt_ret = full_screen_prompt_help(HELPCOORDS, "Image Coordinates", nump + 1, prompts, values, 0x90, NULL);
	if (prompt_ret < 0)
	{
		g_use_center_mag = ousemag;
		g_escape_time_state.m_grid_fp.x_min() = oxxmin;
		g_escape_time_state.m_grid_fp.x_max() = oxxmax;
		g_escape_time_state.m_grid_fp.y_min() = oyymin;
		g_escape_time_state.m_grid_fp.y_max() = oyymax;
		g_escape_time_state.m_grid_fp.x_3rd() = oxx3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = oyy3rd;
		return -1;
	}

	if (prompt_ret == FIK_F4)  /* reset to type defaults */
	{
		g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
		g_escape_time_state.m_grid_fp.x_max()         = g_current_fractal_specific->x_max;
		g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
		g_escape_time_state.m_grid_fp.y_max()         = g_current_fractal_specific->y_max;
		if (g_view_crop && g_final_aspect_ratio != g_screen_aspect_ratio)
		{
			aspect_ratio_crop(g_screen_aspect_ratio, g_final_aspect_ratio);
		}
		if (g_bf_math != 0)
		{
			fractal_float_to_bf();
		}
		goto gc_loop;
	}

	if (cmag)
	{
		if (cmpdbl(Xctr         , values[0].uval.dval)
			|| cmpdbl(Yctr         , values[1].uval.dval)
			|| cmpdbl((double)Magnification, values[2].uval.dval)
			|| cmpdbl(Xmagfactor   , values[3].uval.dval)
			|| cmpdbl(Rotation     , values[4].uval.dval)
			|| cmpdbl(Skew         , values[5].uval.dval))
		{
			Xctr          = values[0].uval.dval;
			Yctr          = values[1].uval.dval;
			Magnification = values[2].uval.dval;
			Xmagfactor    = values[3].uval.dval;
			Rotation      = values[4].uval.dval;
			Skew          = values[5].uval.dval;
			if (Xmagfactor == 0)
			{
				Xmagfactor = 1;
			}
			convert_corners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
		}
	}

	else
	{
		if (g_orbit_draw_mode == ORBITDRAW_LINE)
		{
			nump = 1;
			g_escape_time_state.m_grid_fp.x_min() = values[nump++].uval.dval;
			g_escape_time_state.m_grid_fp.y_max() = values[nump++].uval.dval;
			nump++;
			g_escape_time_state.m_grid_fp.x_max() = values[nump++].uval.dval;
			g_escape_time_state.m_grid_fp.y_min() = values[nump++].uval.dval;
		}
		else
		{
			nump = 1;
			g_escape_time_state.m_grid_fp.x_min() = values[nump++].uval.dval;
			g_escape_time_state.m_grid_fp.y_max() = values[nump++].uval.dval;
			nump++;
			g_escape_time_state.m_grid_fp.x_max() = values[nump++].uval.dval;
			g_escape_time_state.m_grid_fp.y_min() = values[nump++].uval.dval;
			nump++;
			g_escape_time_state.m_grid_fp.x_3rd() = values[nump++].uval.dval;
			g_escape_time_state.m_grid_fp.y_3rd() = values[nump++].uval.dval;
			if (g_escape_time_state.m_grid_fp.x_3rd() == 0 && g_escape_time_state.m_grid_fp.y_3rd() == 0)
			{
				g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
				g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
			}
		}
	}

	if (prompt_ret == FIK_F7 && g_orbit_draw_mode != ORBITDRAW_LINE)  /* toggle corners/center-mag mode */
	{
		if (!g_use_center_mag)
		{
			convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
			g_use_center_mag = TRUE;
		}
		else
		{
			g_use_center_mag = FALSE;
		}
		goto gc_loop;
	}

	if (!cmpdbl(oxxmin, g_escape_time_state.m_grid_fp.x_min()) && !cmpdbl(oxxmax, g_escape_time_state.m_grid_fp.x_max()) && !cmpdbl(oyymin, g_escape_time_state.m_grid_fp.y_min()) &&
		!cmpdbl(oyymax, g_escape_time_state.m_grid_fp.y_max()) && !cmpdbl(oxx3rd, g_escape_time_state.m_grid_fp.x_3rd()) && !cmpdbl(oyy3rd, g_escape_time_state.m_grid_fp.y_3rd()))
	{
		/* no change, restore values to avoid drift */
		g_escape_time_state.m_grid_fp.x_min() = oxxmin;
		g_escape_time_state.m_grid_fp.x_max() = oxxmax;
		g_escape_time_state.m_grid_fp.y_min() = oyymin;
		g_escape_time_state.m_grid_fp.y_max() = oyymax;
		g_escape_time_state.m_grid_fp.x_3rd() = oxx3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = oyy3rd;
		return 0;
	}
	else
	{
		return 1;
	}
}

static int get_screen_corners()
{
	struct full_screen_values values[15];
	char *prompts[15];
	char xprompt[] = "          X";
	char yprompt[] = "          Y";
	char zprompt[] = "          Z";
	int i, nump, prompt_ret;
	int cmag;
	double Xctr, Yctr;
	LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
	double Xmagfactor, Rotation, Skew;
	BYTE ousemag;
	double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;
	double svxxmin, svxxmax, svyymin, svyymax, svxx3rd, svyy3rd;

	ousemag = g_use_center_mag;

	svxxmin = g_escape_time_state.m_grid_fp.x_min();  /* save these for later since convert_corners modifies them */
	svxxmax = g_escape_time_state.m_grid_fp.x_max();  /* and we need to set them for convert_center_mag to work */
	svxx3rd = g_escape_time_state.m_grid_fp.x_3rd();
	svyymin = g_escape_time_state.m_grid_fp.y_min();
	svyymax = g_escape_time_state.m_grid_fp.y_max();
	svyy3rd = g_escape_time_state.m_grid_fp.y_3rd();

	if (!g_set_orbit_corners && !g_keep_screen_coords)
	{
		g_orbit_x_min = g_escape_time_state.m_grid_fp.x_min();
		g_orbit_x_max = g_escape_time_state.m_grid_fp.x_max();
		g_orbit_x_3rd = g_escape_time_state.m_grid_fp.x_3rd();
		g_orbit_y_min = g_escape_time_state.m_grid_fp.y_min();
		g_orbit_y_max = g_escape_time_state.m_grid_fp.y_max();
		g_orbit_y_3rd = g_escape_time_state.m_grid_fp.y_3rd();
	}

	oxxmin = g_orbit_x_min;
	oxxmax = g_orbit_x_max;
	oyymin = g_orbit_y_min;
	oyymax = g_orbit_y_max;
	oxx3rd = g_orbit_x_3rd;
	oyy3rd = g_orbit_y_3rd;

	g_escape_time_state.m_grid_fp.x_min() = g_orbit_x_min;
	g_escape_time_state.m_grid_fp.x_max() = g_orbit_x_max;
	g_escape_time_state.m_grid_fp.y_min() = g_orbit_y_min;
	g_escape_time_state.m_grid_fp.y_max() = g_orbit_y_max;
	g_escape_time_state.m_grid_fp.x_3rd() = g_orbit_x_3rd;
	g_escape_time_state.m_grid_fp.y_3rd() = g_orbit_y_3rd;

gsc_loop:
	for (i = 0; i < 15; ++i)
	{
		values[i].type = 'd'; /* most values on this screen are type d */
	}
	cmag = g_use_center_mag;
	convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

	nump = -1;
	if (cmag)
	{
		prompts[++nump]= "Center X";
		values[nump].uval.dval = Xctr;
		prompts[++nump]= "Center Y";
		values[nump].uval.dval = Yctr;
		prompts[++nump]= "Magnification";
		values[nump].uval.dval = (double)Magnification;
		prompts[++nump]= "X Magnification Factor";
		values[nump].uval.dval = Xmagfactor;
		prompts[++nump]= "Rotation Angle (degrees)";
		values[nump].uval.dval = Rotation;
		prompts[++nump]= "Skew Angle (degrees)";
		values[nump].uval.dval = Skew;
		prompts[++nump]= "";
		values[nump].type = '*';
		prompts[++nump]= "Press "FK_F7" to switch to \"corners\" mode";
		values[nump].type = '*';
	}
	else
	{
		prompts[++nump]= "Top-Left Corner";
		values[nump].type = '*';
		prompts[++nump] = xprompt;
		values[nump].uval.dval = g_orbit_x_min;
		prompts[++nump] = yprompt;
		values[nump].uval.dval = g_orbit_y_max;
		prompts[++nump]= "Bottom-Right Corner";
		values[nump].type = '*';
		prompts[++nump] = xprompt;
		values[nump].uval.dval = g_orbit_x_max;
		prompts[++nump] = yprompt;
		values[nump].uval.dval = g_orbit_y_min;
		if (g_orbit_x_min == g_orbit_x_3rd && g_orbit_y_min == g_orbit_y_3rd)
		{
			g_orbit_x_3rd = g_orbit_y_3rd = 0;
		}
		prompts[++nump]= "Bottom-left (zeros for top-left X, bottom-right Y)";
		values[nump].type = '*';
		prompts[++nump] = xprompt;
		values[nump].uval.dval = g_orbit_x_3rd;
		prompts[++nump] = yprompt;
		values[nump].uval.dval = g_orbit_y_3rd;
		prompts[++nump]= "Press "FK_F7" to switch to \"center-mag\" mode";
		values[nump].type = '*';
	}

	prompts[++nump]= "Press "FK_F4" to reset to type default values";
	values[nump].type = '*';

	prompt_ret = full_screen_prompt_help(HELPSCRNCOORDS, "Screen Coordinates",
		nump + 1, prompts, values, 0x90, NULL);
	if (prompt_ret < 0)
	{
		g_use_center_mag = ousemag;
		g_orbit_x_min = oxxmin;
		g_orbit_x_max = oxxmax;
		g_orbit_y_min = oyymin;
		g_orbit_y_max = oyymax;
		g_orbit_x_3rd = oxx3rd;
		g_orbit_y_3rd = oyy3rd;
		/* restore corners */
		g_escape_time_state.m_grid_fp.x_min() = svxxmin;
		g_escape_time_state.m_grid_fp.x_max() = svxxmax;
		g_escape_time_state.m_grid_fp.y_min() = svyymin;
		g_escape_time_state.m_grid_fp.y_max() = svyymax;
		g_escape_time_state.m_grid_fp.x_3rd() = svxx3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = svyy3rd;
		return -1;
		}

	if (prompt_ret == FIK_F4)  /* reset to type defaults */
	{
		g_orbit_x_3rd = g_orbit_x_min = g_current_fractal_specific->x_min;
		g_orbit_x_max = g_current_fractal_specific->x_max;
		g_orbit_y_3rd = g_orbit_y_min = g_current_fractal_specific->y_min;
		g_orbit_y_max = g_current_fractal_specific->y_max;
		g_escape_time_state.m_grid_fp.x_min() = g_orbit_x_min;
		g_escape_time_state.m_grid_fp.x_max() = g_orbit_x_max;
		g_escape_time_state.m_grid_fp.y_min() = g_orbit_y_min;
		g_escape_time_state.m_grid_fp.y_max() = g_orbit_y_max;
		g_escape_time_state.m_grid_fp.x_3rd() = g_orbit_x_3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = g_orbit_y_3rd;
		if (g_view_crop && g_final_aspect_ratio != g_screen_aspect_ratio)
		{
			aspect_ratio_crop(g_screen_aspect_ratio, g_final_aspect_ratio);
		}

		g_orbit_x_min = g_escape_time_state.m_grid_fp.x_min();
		g_orbit_x_max = g_escape_time_state.m_grid_fp.x_max();
		g_orbit_y_min = g_escape_time_state.m_grid_fp.y_min();
		g_orbit_y_max = g_escape_time_state.m_grid_fp.y_max();
		g_orbit_x_3rd = g_escape_time_state.m_grid_fp.x_min();
		g_orbit_y_3rd = g_escape_time_state.m_grid_fp.y_min();
		goto gsc_loop;
		}

	if (cmag)
	{
		if (cmpdbl(Xctr         , values[0].uval.dval)
		|| cmpdbl(Yctr         , values[1].uval.dval)
		|| cmpdbl((double)Magnification, values[2].uval.dval)
		|| cmpdbl(Xmagfactor   , values[3].uval.dval)
		|| cmpdbl(Rotation     , values[4].uval.dval)
		|| cmpdbl(Skew         , values[5].uval.dval))
		{
			Xctr          = values[0].uval.dval;
			Yctr          = values[1].uval.dval;
			Magnification = values[2].uval.dval;
			Xmagfactor    = values[3].uval.dval;
			Rotation      = values[4].uval.dval;
			Skew          = values[5].uval.dval;
			if (Xmagfactor == 0)
			{
				Xmagfactor = 1;
			}
			convert_corners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
			/* set screen corners */
			g_orbit_x_min = g_escape_time_state.m_grid_fp.x_min();
			g_orbit_x_max = g_escape_time_state.m_grid_fp.x_max();
			g_orbit_y_min = g_escape_time_state.m_grid_fp.y_min();
			g_orbit_y_max = g_escape_time_state.m_grid_fp.y_max();
			g_orbit_x_3rd = g_escape_time_state.m_grid_fp.x_3rd();
			g_orbit_y_3rd = g_escape_time_state.m_grid_fp.y_3rd();
		}
	}
	else
	{
		nump = 1;
		g_orbit_x_min = values[nump++].uval.dval;
		g_orbit_y_max = values[nump++].uval.dval;
		nump++;
		g_orbit_x_max = values[nump++].uval.dval;
		g_orbit_y_min = values[nump++].uval.dval;
		nump++;
		g_orbit_x_3rd = values[nump++].uval.dval;
		g_orbit_y_3rd = values[nump++].uval.dval;
		if (g_orbit_x_3rd == 0 && g_orbit_y_3rd == 0)
		{
			g_orbit_x_3rd = g_orbit_x_min;
			g_orbit_y_3rd = g_orbit_y_min;
		}
	}

	if (prompt_ret == FIK_F7)  /* toggle corners/center-mag mode */
	{
		if (!g_use_center_mag)
		{
			convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
			g_use_center_mag = TRUE;
		}
		else
		{
			g_use_center_mag = FALSE;
		}
		goto gsc_loop;
	}

	if (!cmpdbl(oxxmin, g_orbit_x_min) && !cmpdbl(oxxmax, g_orbit_x_max) && !cmpdbl(oyymin, g_orbit_y_min) &&
		!cmpdbl(oyymax, g_orbit_y_max) && !cmpdbl(oxx3rd, g_orbit_x_3rd) && !cmpdbl(oyy3rd, g_orbit_y_3rd))
	{
		/* no change, restore values to avoid drift */
		g_orbit_x_min = oxxmin;
		g_orbit_x_max = oxxmax;
		g_orbit_y_min = oyymin;
		g_orbit_y_max = oyymax;
		g_orbit_x_3rd = oxx3rd;
		g_orbit_y_3rd = oyy3rd;
		/* restore corners */
		g_escape_time_state.m_grid_fp.x_min() = svxxmin;
		g_escape_time_state.m_grid_fp.x_max() = svxxmax;
		g_escape_time_state.m_grid_fp.y_min() = svyymin;
		g_escape_time_state.m_grid_fp.y_max() = svyymax;
		g_escape_time_state.m_grid_fp.x_3rd() = svxx3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = svyy3rd;
		return 0;
	}
	else
	{
		g_set_orbit_corners = 1;
		g_keep_screen_coords = 1;
		/* restore corners */
		g_escape_time_state.m_grid_fp.x_min() = svxxmin;
		g_escape_time_state.m_grid_fp.x_max() = svxxmax;
		g_escape_time_state.m_grid_fp.y_min() = svyymin;
		g_escape_time_state.m_grid_fp.y_max() = svyymax;
		g_escape_time_state.m_grid_fp.x_3rd() = svxx3rd;
		g_escape_time_state.m_grid_fp.y_3rd() = svyy3rd;
		return 1;
	}
}

/* get browse parameters , called from fractint.c and loadfile.c
	returns 3 if anything changes.  code pinched from get_view_params */

int get_browse_parameters()
{
	char *choices[10];
	struct full_screen_values uvalues[25];
	int i, k;
	int old_autobrowse, old_brwschecktype, old_brwscheckparms;
	bool old_doublecaution;
	int old_minbox;
	double old_toosmall;
	char old_browsemask[FILE_MAX_FNAME];

	old_autobrowse     = g_auto_browse;
	old_brwschecktype  = g_browse_check_type;
	old_brwscheckparms = g_browse_check_parameters;
	old_doublecaution  = g_ui_state.double_caution;
	old_minbox         = g_cross_hair_box_size;
	old_toosmall       = g_too_small;
	strcpy(old_browsemask, g_browse_mask);

get_brws_restart:
	/* fill up the previous values arrays */
	k = -1;

	choices[++k] = "Autobrowsing? (y/n)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_auto_browse;

	choices[++k] = "Ask about GIF video mode? (y/n)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_ui_state.ask_video ? 1 : 0;

	choices[++k] = "Check fractal type? (y/n)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_browse_check_type;

	choices[++k] = "Check fractal parameters (y/n)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_browse_check_parameters;

	choices[++k] = "Confirm file deletes (y/n)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_ui_state.double_caution ? 1 : 0;

	choices[++k] = "Smallest window to display (size in pixels)";
	uvalues[k].type = 'f';
	uvalues[k].uval.dval = g_too_small;

	choices[++k] = "Smallest box size shown before crosshairs used (pix)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_cross_hair_box_size;
	choices[++k] = "Browse search filename mask ";
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval, g_browse_mask);

	choices[++k] = "";
	uvalues[k].type = '*';

	choices[++k] = "Press "FK_F4" to reset browse parameters to defaults.";
	uvalues[k].type = '*';

	i = full_screen_prompt_help(HELPBRWSPARMS, "Browse ('L'ook) Mode Options",
		k + 1, choices, uvalues, 16, NULL);
	if (i < 0)
	{
		return 0;
	}

	if (i == FIK_F4)
	{
		g_too_small = 6;
		g_auto_browse = FALSE;
		g_ui_state.ask_video = true;
		g_browse_check_parameters = TRUE;
		g_browse_check_type  = TRUE;
		g_ui_state.double_caution  = true;
		g_cross_hair_box_size = 3;
		strcpy(g_browse_mask, "*.gif");
		goto get_brws_restart;
	}

	/* now check out the results (*hopefully* in the same order <grin>) */
	k = -1;

	g_auto_browse = uvalues[++k].uval.ch.val;
	g_ui_state.ask_video = (uvalues[++k].uval.ch.val != 0);
	g_browse_check_type = (char) uvalues[++k].uval.ch.val;
	g_browse_check_parameters = (char) uvalues[++k].uval.ch.val;
	g_ui_state.double_caution = (uvalues[++k].uval.ch.val != 0);
	g_too_small  = uvalues[++k].uval.dval;
	if (g_too_small < 0)
	{
		g_too_small = 0 ;
	}
	g_cross_hair_box_size = uvalues[++k].uval.ival;
	if (g_cross_hair_box_size < 1)
	{
		g_cross_hair_box_size = 1;
	}
	if (g_cross_hair_box_size > 10)
	{
		g_cross_hair_box_size = 10;
	}

	strcpy(g_browse_mask, uvalues[++k].uval.sval);

	i = 0;
	if (g_auto_browse != old_autobrowse ||
			g_browse_check_type != old_brwschecktype ||
			g_browse_check_parameters != old_brwscheckparms ||
			g_ui_state.double_caution != old_doublecaution ||
			g_too_small != old_toosmall ||
			g_cross_hair_box_size != old_minbox ||
			!stricmp(g_browse_mask, old_browsemask))
	{
		i = -3;
	}

	if (g_evolving)  /* can't browse */
	{
		g_auto_browse = 0;
		i = 0;
	}

	return i;
}

/* merge existing full path with new one  */
/* attempt to detect if file or directory */

#define ATFILENAME 0
#define SSTOOLSINI 1
#define ATCOMMANDINTERACTIVE 2
#define ATFILENAMESETNAME  3

#define GETPATH (mode < 2)

#ifndef XFRACT
#include <direct.h>
#endif

/* copies the proposed new filename to the fullpath variable */
/* does not copy directories for PAR files (modes 2 and 3)   */
/* attempts to extract directory and test for existence (modes 0 and 1) */
int merge_path_names(char *oldfullpath, char *newfilename, int mode)
{
	int isadir = 0;
	int isafile = 0;
	int len;
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char temp_path[FILE_MAX_PATH];

	char drive1[FILE_MAX_DRIVE];
	char dir1[FILE_MAX_DIR];
	char fname1[FILE_MAX_FNAME];
	char ext1[FILE_MAX_EXT];

	/* no dot or slash so assume a file */
	if (strchr(newfilename, '.') == NULL && strchr(newfilename, SLASHC) == NULL)
	{
		isafile = 1;
	}
	isadir = is_a_directory(newfilename);
	if (isadir != 0)
	{
		fix_dir_name(newfilename);
	}
#if 0
	/* if slash by itself, it's a directory */
	if (strcmp(newfilename, SLASH) == 0)
	{
		isadir = 1;
	}
#endif
#ifndef XFRACT
	/* if drive, colon, slash, is a directory */
	if ((int) strlen(newfilename) == 3 &&
			newfilename[1] == ':' &&
			newfilename[2] == SLASHC)
		isadir = 1;
	/* if drive, colon, with no slash, is a directory */
	if ((int) strlen(newfilename) == 2 && newfilename[1] == ':')
	{
		newfilename[2] = SLASHC;
		newfilename[3] = 0;
		isadir = 1;
	}
	/* if dot, slash, '0', its the current directory, set up full path */
	if (newfilename[0] == '.' && newfilename[1] == SLASHC && newfilename[2] == 0)
	{
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		expand_dirname(newfilename, temp_path);
		strcat(temp_path, newfilename);
		strcpy(newfilename, temp_path);
		isadir = 1;
	}
	/* if dot, slash, its relative to the current directory, set up full path */
	if (newfilename[0] == '.' && newfilename[1] == SLASHC)
	{
		int len, test_dir = 0;
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		if (strrchr(newfilename, '.') == newfilename)
		{
			test_dir = 1;  /* only one '.' assume its a directory */
		}
		expand_dirname(newfilename, temp_path);
		strcat(temp_path, newfilename);
		strcpy(newfilename, temp_path);
		if (!test_dir)
		{
			len = (int) strlen(newfilename);
			newfilename[len-1] = 0; /* get rid of slash added by expand_dirname */
		}
	}
#else
	findpath(newfilename, temp_path);
	strcpy(newfilename, temp_path);
#endif
	/* check existence */
	if (isadir == 0 || isafile == 1)
	{
		if (fr_find_first(newfilename) == 0)
		{
			if (g_dta.attribute & SUBDIR) /* exists and is dir */
			{
				fix_dir_name(newfilename);  /* add trailing slash */
				isadir = 1;
				isafile = 0;
			}
			else
			{
				isafile = 1;
			}
		}
	}

	split_path(newfilename, drive, dir, fname, ext);
	split_path(oldfullpath, drive1, dir1, fname1, ext1);
	if ((int) strlen(drive) != 0 && GETPATH)
	{
		strcpy(drive1, drive);
	}
	if ((int) strlen(dir) != 0 && GETPATH)
	{
		strcpy(dir1, dir);
	}
	if ((int) strlen(fname) != 0)
	{
		strcpy(fname1, fname);
	}
	if ((int) strlen(ext) != 0)
	{
		strcpy(ext1, ext);
	}
	if (isadir == 0 && isafile == 0 && GETPATH)
	{
		make_path(oldfullpath, drive1, dir1, NULL, NULL);
		len = (int) strlen(oldfullpath);
		if (len > 0)
		{
			char save;
			/* strip trailing slash */
			save = oldfullpath[len-1];
			if (save == SLASHC)
			{
				oldfullpath[len-1] = 0;
			}
			if (access(oldfullpath, 0))
			{
				isadir = -1;
			}
			oldfullpath[len-1] = save;
		}
	}
	make_path(oldfullpath, drive1, dir1, fname1, ext1);
	return isadir;
}

/* extract just the filename/extension portion of a path */
void extract_filename(char *target, char *source)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	split_path(source, NULL, NULL, fname, ext);
	make_path(target, "", "", fname, ext);
}

/* tells if filename has extension */
/* returns pointer to period or NULL */
char *has_extension(char *source)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char *ret = NULL;
	split_path(source, NULL, NULL, fname, ext);
	if (ext != NULL)
	{
		if (*ext != 0)
		{
			ret = strrchr(source, '.');
		}
	}
	return ret;
}


/* I tried heap sort also - this is faster! */
void shell_sort(void *v1, int n, unsigned sz, int (__cdecl *fct)(VOIDPTR arg1, VOIDPTR arg2))
{
	int gap, i, j;
	char *temp;
	char *v;
	v = (char *)v1;
	for (gap = n/2; gap > 0; gap /= 2)
	{
		for (i = gap; i < n; i++)
		{
			for (j = i-gap; j >= 0; j -= gap)
			{
				if (fct((char **)(v + j*sz), (char **)(v + (j + gap)*sz)) <= 0)
				{
					break;
				}
				temp = *(char **)(v + j*sz);
				*(char **)(v + j*sz) = *(char **)(v + (j + gap)*sz);
				*(char **)(v + (j + gap)*sz) = temp;
			}
		}
	}
}

int integer_unsupported()
{
	static int last_fractype = -1;
	if (g_fractal_type != last_fractype)
	{
		last_fractype = g_fractal_type;
		stop_message(0, "This integer fractal type is unimplemented;\n"
			"Use float=yes or the <X> screen to get a real image.");
	}

	return 0;
}
