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

#include <string>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "fihelp.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "evolve.h"
#include "fracsubr.h"
#include "history.h"
#include "line3d.h"
#include "loadfile.h"
#include "loadmap.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "slideshw.h"
#include "zoom.h"

#include "busy.h"
#include "EscapeTime.h"
#include "SoundState.h"
#include "UIChoices.h"
#include "MathUtil.h"

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

static int calculation_mode()
{
	switch (g_user_standard_calculation_mode)
	{
	case '1': return 0;
	case '2': return 1;
	case '3': return 2;
	case 'g': return 3 + g_stop_pass;
	case 'b': return 10;
	case 's': return 11;
	case 't': return 12;
	case 'd': return 13;
	case 'o': return 14;
	default:
		assert(false && "Bad g_user_standard_calculation_mode");
	}
	return -1;
}

static int inside_mode()
{
	if (g_inside >= 0)  /* numb */
	{
		return 0;
	}
	
	switch (g_inside)
	{
	case -1:		return 1;
	case ZMAG:		return 2;
	case BOF60:		return 3;
	case BOF61:		return 4;
	case EPSCROSS:	return 5;
	case STARTRAIL: return 6;
	case PERIOD:	return 7;
	case ATANI:		return 8;
	case FMODI:		return 9;
	default:
		assert(false && "Bad g_inside");
	}
	return -1;
}

static char *save_name()
{
	char *result = ::strrchr(g_save_name, SLASHC);
	return (result == NULL) ? g_save_name : result+1;
}

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
	char old_user_standard_calculation_mode = g_user_standard_calculation_mode;
	int old_stop_pass = g_stop_pass;
	long old_max_iteration = g_max_iteration;
	int old_inside = g_inside;
	int old_outside = g_outside;
	char previous_save_name[FILE_MAX_DIR + 1];
	strcpy(previous_save_name, g_save_name);
	int old_sound_flags = g_sound_state.flags();
	long old_log_palette_flag = g_log_palette_mode;
	int old_biomorph = g_user_biomorph;
	int old_decomposition = g_decomposition[0];
	int old_fill_color = g_fill_color;
	double old_proximity = g_proximity;

	UIChoices dialog(HELPXOPTS, "Basic Options\n(not all combinations make sense)", 0);
	const char *calculation_modes[] =
	{
		"1", "2", "3",
		"g", "g1", "g2", "g3", "g4", "g5", "g6",
		"b", "s", "t", "d", "o"
	};
	dialog.push("Passes (1,2,3, g[uess], b[ound], t[ess], d[iffu], o[rbit])",
		calculation_modes, NUM_OF(calculation_modes), calculation_mode());
	dialog.push("Floating Point Algorithm", g_user_float_flag);
	dialog.push("Maximum Iterations (2 to 2,147,483,647)", g_max_iteration);
	dialog.push("Inside Color (0-# of colors, if Inside=numb)", (g_inside >= 0) ? g_inside : 0);
	const char *insidemodes[] =
	{
		"numb", "maxiter", "zmag", "bof60", "bof61",
		"epsiloncross", "startrail", "period", "atan", "fmod"
	};
	dialog.push("Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)",
		insidemodes, NUM_OF(insidemodes), inside_mode());
	dialog.push("Outside Color (0-# of colors, if Outside=numb)", g_outside >= 0 ? g_outside : 0);
	const char *outsidemodes[] =
	{
		"numb", "iter", "real", "imag", "mult",
		"summ", "atan", "fmod", "tdis"
	};
	dialog.push("Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)",
		outsidemodes, NUM_OF(outsidemodes), g_outside >= 0 ? 0 : -g_outside);
	char *savenameptr = save_name();
	dialog.push("Savename (.GIF implied)", savenameptr);
	dialog.push("File Overwrite ('overwrite=')", g_fractal_overwrite);
	const char *soundmodes[5] =
	{
		"off", "beep", "x", "y", "z"
	};
	dialog.push("Sound (off, beep, x, y, z)",
		soundmodes, NUM_OF(soundmodes), old_sound_flags & SOUNDFLAG_ORBITMASK);
	if (g_ranges_length == 0)
	{
		dialog.push("Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)", g_log_palette_mode);
	}
	else
	{
		dialog.push("Log Palette (n/a, ranges= parameter is in effect)");
	}
	dialog.push("Biomorph Color (-1 means OFF)", g_user_biomorph);
	dialog.push("Decomp Option (2,4,8,..,256, 0=OFF)", g_decomposition[0]);
	char fill_buffer[80] = "normal";
	if (g_fill_color >= 0)
	{
		sprintf(fill_buffer, "%d", g_fill_color);
	}
	dialog.push("Fill Color (normal,#) (works with passes=t, b and d)", fill_buffer);
	dialog.push("Proximity value for inside=epscross and fmod", static_cast<float>(g_proximity));
	if (dialog.prompt() < 0)
	{
		return -1;
	}

	int k = -1;
	int j = 0;
	g_user_standard_calculation_mode = calculation_modes[dialog.values(++k).uval.ch.val][0];
	g_stop_pass = (int) calculation_modes[dialog.values(k).uval.ch.val][1] - (int) '0';
	if (g_stop_pass < 0 || g_stop_pass > 6 || g_user_standard_calculation_mode != 'g')
	{
		g_stop_pass = 0;
	}
	/* Oops, lyapunov type doesn't use 'new' & breaks orbits */
	if (g_user_standard_calculation_mode == 'o'
		&& g_fractal_type == FRACTYPE_LYAPUNOV)
	{
		g_user_standard_calculation_mode = old_user_standard_calculation_mode;
	}
	if (old_user_standard_calculation_mode != g_user_standard_calculation_mode)
	{
		j++;
	}
	if (old_stop_pass != g_stop_pass)
	{
		j++;
	}

	if (dialog.values(++k).uval.ch.val != (g_user_float_flag ? 1 : 0))
	{
		g_user_float_flag = (dialog.values(k).uval.ch.val != 0);
		j++;
	}

	++k;
	g_max_iteration = dialog.values(k).uval.Lval;
	if (g_max_iteration < 0)
	{
		g_max_iteration = old_max_iteration;
	}
	if (g_max_iteration < 2)
	{
		g_max_iteration = 2;
	}
	if (g_max_iteration != old_max_iteration)
	{
		j++;
	}

	g_inside = dialog.values(++k).uval.ival;
	if (g_inside < 0)
	{
		g_inside = -g_inside;
	}
	if (g_inside >= g_colors)
	{
		g_inside = (g_inside % g_colors) + (g_inside / g_colors);
	}

	{
		int tmp = dialog.values(++k).uval.ch.val;
		int insides[] =
		{
			0, -1, ZMAG, BOF60, BOF61, EPSCROSS, STARTRAIL, PERIOD, ATANI, FMODI
		};
		if (tmp > 0 && tmp < NUM_OF(insides))
		{
			g_inside = insides[tmp];
		}
	}
	if (g_inside != old_inside)
	{
		j++;
	}

	g_outside = dialog.values(++k).uval.ival;
	if (g_outside < 0)
	{
		g_outside = -g_outside;
	}
	if (g_outside >= g_colors)
	{
		g_outside = (g_outside % g_colors) + (g_outside / g_colors);
	}
	{
		int tmp = dialog.values(++k).uval.ch.val;
		if (tmp > 0)
		{
			g_outside = -tmp;
		}
	}
	if (g_outside != old_outside)
	{
		j++;
	}

	::strcpy(savenameptr, dialog.values(++k).uval.sval);
	if (::strcmp(g_save_name, previous_save_name))
	{
		g_resave_mode = RESAVE_NO;
		g_started_resaves = false; /* forget pending increment */
	}

	g_fractal_overwrite = (dialog.values(++k).uval.ch.val != 0);

	g_sound_state.set_flags((g_sound_state.flags() & ~SOUNDFLAG_ORBITMASK) | dialog.values(++k).uval.ch.val);
	if ((g_sound_state.flags() != old_sound_flags)
		&& ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP
			|| (old_sound_flags & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP))
	{
		j++;
	}

	g_log_palette_mode = dialog.values(++k).uval.Lval;
	if (g_log_palette_mode != old_log_palette_flag)
	{
		j++;
		g_log_automatic_flag = false;  /* turn it off, use the supplied value */
	}

	g_user_biomorph = dialog.values(++k).uval.ival;
	if (g_user_biomorph >= g_colors)
	{
		g_user_biomorph = (g_user_biomorph % g_colors) + (g_user_biomorph / g_colors);
	}
	if (g_user_biomorph != old_biomorph)
	{
		j++;
	}

	g_decomposition[0] = dialog.values(++k).uval.ival;
	if (g_decomposition[0] != old_decomposition)
	{
		j++;
	}

	if (::strncmp(::strlwr(const_cast<char *>(dialog.values(++k).uval.sval)), "normal", 4) == 0)
	{
		g_fill_color = -1;
	}
	else
	{
		g_fill_color = atoi(dialog.values(k).uval.sval);
	}
	if (g_fill_color < 0)
	{
		g_fill_color = -1;
	}
	if (g_fill_color >= g_colors)
	{
		g_fill_color = (g_fill_color % g_colors) + (g_fill_color / g_colors);
	}
	if (g_fill_color != old_fill_color)
	{
		j++;
	}

	g_proximity = dialog.values(++k).uval.dval;
	if (g_proximity != old_proximity)
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
	double old_potparam[3];
	old_potparam[0] = g_potential_parameter[0];
	old_potparam[1] = g_potential_parameter[1];
	old_potparam[2] = g_potential_parameter[2];
	long old_usr_distest = g_user_distance_test;
	int old_distestwidth = g_distance_test_width;
	int old_rotate_lo = g_rotate_lo;
	int old_rotate_hi = g_rotate_hi;
	double old_inversion[3];
	for (int i = 0; i < NUM_OF(old_inversion); i++)
	{
		old_inversion[i] = g_inversion[i];
	}

	UIChoices dialog(HELPYOPTS, "Extended Options\n(not all combinations make sense)", 0);
	dialog.push("Look for finite attractor (0=no,>0=yes,<0=phase)", g_finite_attractor);
	dialog.push("Potential Max Color (0 means off)", (int) old_potparam[0]);
	dialog.push("          Slope", old_potparam[1]);
	dialog.push("          Bailout", (int) old_potparam[2]);
	dialog.push("          16 bit values", g_potential_16bit);
	dialog.push("Distance Estimator (0=off, <0=edge, >0=on):", g_user_distance_test);
	dialog.push("          width factor:", g_distance_test_width);
	{
		const char *prompts[] =
		{
			"Inversion radius or \"auto\" (0 means off)",
			"          center X coordinate or \"auto\"",
			"          center Y coordinate or \"auto\""
		};
		for (int i = 0; i < 3; i++)
		{
			char buffer[80];
			if (g_inversion[i] == AUTOINVERT)
			{
				::strcpy(buffer, "auto");
			}
			else
			{
				sprintf(buffer, "%-1.15lg", g_inversion[i]);
			}
			dialog.push(prompts[i], buffer);
		}
	}
	dialog.push("  (use fixed radius & center when zooming)");
	dialog.push("Color cycling from color (0 ... 254)", g_rotate_lo);
	dialog.push("              to   color (1 ... 255)", g_rotate_hi);
	if (dialog.prompt() < 0)
	{
		return -1;
	}

	int k = -1;
	int j = 0;   /* return code */
	if (dialog.values(++k).uval.ch.val != g_finite_attractor)
	{
		g_finite_attractor = dialog.values(k).uval.ch.val;
		j = 1;
	}

	g_potential_parameter[0] = dialog.values(++k).uval.ival;
	if (g_potential_parameter[0] != old_potparam[0])
	{
		j = 1;
	}

	g_potential_parameter[1] = dialog.values(++k).uval.dval;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[1] != old_potparam[1])
	{
		j = 1;
	}

	g_potential_parameter[2] = dialog.values(++k).uval.ival;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != old_potparam[2])
	{
		j = 1;
	}

	if (dialog.values(++k).uval.ch.val != (g_potential_16bit ? 1 : 0))
	{
		g_potential_16bit = (dialog.values(k).uval.ch.val != 0);
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
				g_disk_16bit = false;
			}
		}
	}

	++k;
	g_user_distance_test = dialog.values(k).uval.Lval;
	if (g_user_distance_test != old_usr_distest)
	{
		j = 1;
	}
	++k;
	g_distance_test_width = dialog.values(k).uval.ival;
	if (g_user_distance_test && g_distance_test_width != old_distestwidth)
	{
		j = 1;
	}

	for (int i = 0; i < 3; i++)
	{
		if (dialog.values(++k).uval.sval[0] == 'a' || dialog.values(k).uval.sval[0] == 'A')
		{
			g_inversion[i] = AUTOINVERT;
		}
		else
		{
			g_inversion[i] = atof(dialog.values(k).uval.sval);
		}
		if (old_inversion[i] != g_inversion[i]
			&& (i == 0 || g_inversion[0] != 0.0))
		{
			j = 1;
		}
	}
	g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
	++k;

	g_rotate_lo = dialog.values(++k).uval.ival;
	g_rotate_hi = dialog.values(++k).uval.ival;
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
	int old_periodicity;
	old_periodicity = g_user_periodicity_check;
	int old_orbit_delay;
	old_orbit_delay = g_orbit_delay;
	int old_orbit_interval;
	old_orbit_interval = (int)g_orbit_interval;
	int old_keep_scrn_coords;
	old_keep_scrn_coords = g_keep_screen_coords;
	int old_drawmode;
	old_drawmode = g_orbit_draw_mode;

pass_option_restart:
	{
		UIChoices dialog(HELPPOPTS, "Passes Options\n"
			"(not all combinations make sense)\n"
			"(Press "FK_F2" for corner parameters)\n"
			"(Press "FK_F6" for calculation parameters)", 0x44);
		dialog.push("Periodicity (0=off, <0=show, >0=on, -255..+255)", g_user_periodicity_check);
		dialog.push("Orbit delay (0 = none)", g_orbit_delay);
		dialog.push("Orbit interval (1 ... 255)", static_cast<int>(g_orbit_interval));
		dialog.push("Maintain screen coordinates", g_keep_screen_coords);
		const char *passcalcmodes[] =
		{
			"rect", "line"
		};
		dialog.push("Orbit pass shape (rect,line)", passcalcmodes, NUM_OF(passcalcmodes), g_orbit_draw_mode);

		int i = dialog.prompt();
		if (i < 0)
		{
			return -1;
		}

		int j = 0;
		int ret = 0;

		int k = -1;
		g_user_periodicity_check = MathUtil::Clamp(dialog.values(++k).uval.ival, -255, 255);
		if (g_user_periodicity_check != old_periodicity)
		{
			j = 1;
		}
		g_orbit_delay = dialog.values(++k).uval.ival;
		if (g_orbit_delay != old_orbit_delay)
		{
			j = 1;
		}
		g_orbit_interval = MathUtil::Clamp(dialog.values(++k).uval.ival, 1, 255);
		if (g_orbit_interval != old_orbit_interval)
		{
			j = 1;
		}
		g_keep_screen_coords = dialog.values(++k).uval.ch.val;
		if (g_keep_screen_coords != old_keep_scrn_coords)
		{
			j = 1;
		}
		if (g_keep_screen_coords == 0)
		{
			g_set_orbit_corners = 0;
		}
		g_orbit_draw_mode = dialog.values(++k).uval.ch.val;
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
	int x_max;
	int y_max;
	driver_get_max_screen(x_max, y_max);
	bool old_viewwindow = g_view_window;
	float old_viewreduction = g_view_reduction;
	float old_aspectratio = g_final_aspect_ratio;
	int old_viewxdots = g_view_x_dots;
	int old_viewydots = g_view_y_dots;
	int old_sxdots = g_screen_width;
	int old_sydots = g_screen_height;

get_view_restart:
	{
		UIChoices dialog(HELPVIEW, "View Window Options", 16);

		if (!driver_diskp())
		{
			dialog.push("Preview display? (no for full screen)", g_view_window);
			dialog.push("Auto window size reduction factor", g_view_reduction);
			dialog.push("Final media overall aspect ratio, y/x", g_final_aspect_ratio);
			dialog.push("Crop starting coordinates to new aspect ratio?", g_view_crop);
			dialog.push("Explicit size x pixels (0 for auto size)", g_view_x_dots);
			dialog.push("              y pixels (0 to base on aspect ratio)", g_view_y_dots);
		}
		else
		{
			dialog.push("Disk Video x pixels", g_screen_width);
			dialog.push("           y pixels", g_screen_height);
		}
		dialog.push("");
		if (!driver_diskp())
		{
			dialog.push("Press F4 to reset view parameters to defaults.");
		}

		int i = dialog.prompt();
		if (i < 0)
		{
			return -1;
		}

		if (i == FIK_F4 && !driver_diskp())
		{
			g_view_window = false;
			g_view_x_dots = 0;
			g_view_y_dots = 0;
			g_view_reduction = 4.2f;
			g_view_crop = true;
			g_final_aspect_ratio = g_screen_aspect_ratio;
			g_screen_width = old_sxdots;
			g_screen_height = old_sydots;
			goto get_view_restart;
		}

		int k = -1;
		if (!driver_diskp())
		{
			g_view_window = (dialog.values(++k).uval.ch.val != 0);
			g_view_reduction = (float) dialog.values(++k).uval.dval;
			g_final_aspect_ratio = (float) dialog.values(++k).uval.dval;
			g_view_crop = (dialog.values(++k).uval.ch.val != 0);
			g_view_x_dots = dialog.values(++k).uval.ival;
			g_view_y_dots = dialog.values(++k).uval.ival;
		}
		else
		{
			g_screen_width = dialog.values(++k).uval.ival;
			g_screen_height = dialog.values(++k).uval.ival;
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
		if (DEBUGMODE_REAL_POPCORN == g_debug_mode)
		{
			backwards_v18();
			backwards_v19();
			backwards_v20();
		}
	}

	return i;
}


/* --------------------------------------------------------------------- */

int g_gaussian_distribution = 30;
int g_gaussian_offset = 0;
int g_gaussian_slope = 25;
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
	if (g_colors < 255)
	{
		stop_message(0, "starfield requires 256 color mode");
		return -1;
	}

	UIChoices dialog(HELPSTARFLD, "Starfield Parameters", 0);
	const char *starfield_prompts[3] =
	{
		"Star Density in Pixels per Star",
		"Percent Clumpiness",
		"Ratio of Dim stars to Bright"
	};
	for (int i = 0; i < 3; i++)
	{
		dialog.push(starfield_prompts[i], static_cast<float>(starfield_values[i]));
	}
	ScreenStacker stacker;
	if (dialog.prompt() < 0)
	{
		return -1;
	}
	for (int i = 0; i < 3; i++)
	{
		starfield_values[i] = dialog.values(i).uval.dval;
	}

	return 0;
}

static char *masks[] = {"*.pot", "*.gif"};

int get_random_dot_stereogram_parameters()
{
	char rds6[60];
	const char *stereobars[] =
	{
		"none", "middle", "top"
	};
	struct full_screen_values uvalues[7];
	const char *rds_prompts[] =
	{
		"Depth Effect (negative reverses front and back)",
		"Image width in inches",
		"Use grayscale value for depth? (if \"no\" uses color number)",
		"Calibration bars",
		"Use image map? (if \"no\" uses random dots)",
		"  If yes, use current image map name? (see below)",
		rds6
	};
	int i;
	int k;
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

		uvalues[k].uval.ch.val = g_image_map ? 1 : 0;
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
			g_calibrate = (char) uvalues[k++].uval.ch.val;
			g_image_map = (uvalues[k++].uval.ch.val != 0);
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
	ScreenStacker stacker;

	UIChoices dialog("Set Cursor Coordinates", 25);
	dialog.push("X coordinate at cursor", *x);
	dialog.push("Y coordinate at cursor", *y);

	int i = dialog.prompt();
	if (i < 0)
	{
		return -1;
	}

	*x = dialog.values(0).uval.dval;
	*y = dialog.values(1).uval.dval;

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
	char filename[FILE_MAX_PATH];
	char speedstr[81];
	char tmpmask[FILE_MAX_PATH];   /* used to locate next file in list */
	char old_flname[FILE_MAX_PATH];
	int i;
	int j;
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
bool is_a_directory(char *s)
{
	int len;
	char sv;
#ifdef _MSC_VER
	unsigned attrib = 0;
#endif
	if (strchr(s, '*') || strchr(s, '?'))
	{
		return false; /* for my purposes, not a directory */
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
		return true;  /* not a directory or doesn't exist */
	}
	else if (sv == SLASHC)
	{
		/* strip trailing slash and try again */
		s[len-1] = 0;
		if (_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
		{
			s[len-1] = sv;
			return true;
		}
		s[len-1] = sv;
	}
	return false;
#else
	if (fr_find_first(s) != 0) /* couldn't find it */
	{
		/* any better ideas?? */
		if (sv == SLASHC) /* we'll guess it is a directory */
		{
			return true;
		}
		else
		{
			return false;  /* no slashes - we'll guess it's a file */
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
				return false;
			}
			else if ((g_dta.attribute & SUBDIR) != 0)
			{
				return true;   /* we're SURE it's a directory */
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;   /* we're SURE it's a directory */
		}
	}
	return false;
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
				strncpy(dir, &file_template[offset], std::min(len, FILE_MAX_DIR));
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
				strncpy(fname, &file_template[offset], std::min(len, FILE_MAX_FNAME));
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
	full_screen_values val;

	/* change the old value with the same torture the new value had */
	val.type = 'd'; /* most values on this screen are type d */
	val.uval.dval = old;
	prompt_value_string(buf, &val);   /* convert "old" to string */

	old = atof(buf);                /* convert back */
	return fabs(old-new_value) < DBL_EPSILON?0:1;  /* zero if same */
}

int get_corners()
{
	const char *xprompt = "          X";
	const char *yprompt = "          Y";
	const char *zprompt = "          Z";
	bool ousemag = g_use_center_mag;
	double oxxmin = g_escape_time_state.m_grid_fp.x_min();
	double oxxmax = g_escape_time_state.m_grid_fp.x_max();
	double oyymin = g_escape_time_state.m_grid_fp.y_min();
	double oyymax = g_escape_time_state.m_grid_fp.y_max();
	double oxx3rd = g_escape_time_state.m_grid_fp.x_3rd();
	double oyy3rd = g_escape_time_state.m_grid_fp.y_3rd();

gc_loop:
	{
		UIChoices dialog(HELPCOORDS, "Image Coordinates", 0x90);
		bool center_mag = (g_orbit_draw_mode == ORBITDRAW_LINE) ? false : g_use_center_mag;
		double Yctr;
		double Xctr;
		double Skew;
		double Rotation;
		LDBL Magnification;
		double Xmagfactor;
		convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

		if (center_mag)
		{
			dialog.push("Center X", Xctr);
			dialog.push("Center Y", Yctr);
			dialog.push("Magnification", static_cast<double>(Magnification));
			dialog.push("X Magnification Factor", Xmagfactor);
			dialog.push("Rotation Angle (degrees)", Rotation);
			dialog.push("Skew Angle (degrees)", Skew);
			dialog.push("");
			dialog.push("Press "FK_F7" to switch to \"corners\" mode");
		}
		else
		{
			if (g_orbit_draw_mode == ORBITDRAW_LINE)
			{
				dialog.push("Left End Point");
				dialog.push(xprompt, g_escape_time_state.m_grid_fp.x_min());
				dialog.push(yprompt, g_escape_time_state.m_grid_fp.y_max());
				dialog.push("Right End Point");
				dialog.push(xprompt, g_escape_time_state.m_grid_fp.x_max());
				dialog.push(yprompt, g_escape_time_state.m_grid_fp.y_min());
			}
			else
			{
				dialog.push("Top-Left Corner");
				dialog.push(xprompt, g_escape_time_state.m_grid_fp.x_min());
				dialog.push(yprompt, g_escape_time_state.m_grid_fp.y_max());
				dialog.push("Bottom-Right Corner");
				dialog.push(xprompt, g_escape_time_state.m_grid_fp.x_max());
				dialog.push(yprompt, g_escape_time_state.m_grid_fp.y_min());
				if (g_escape_time_state.m_grid_fp.x_min() == g_escape_time_state.m_grid_fp.x_3rd()
					&& g_escape_time_state.m_grid_fp.y_min() == g_escape_time_state.m_grid_fp.y_3rd())
				{
					g_escape_time_state.m_grid_fp.x_3rd() = 0;
					g_escape_time_state.m_grid_fp.y_3rd() = 0;
				}
				dialog.push("Bottom-left (zeros for top-left X, bottom-right Y)");
				dialog.push(xprompt, g_escape_time_state.m_grid_fp.x_3rd());
				dialog.push(yprompt, g_escape_time_state.m_grid_fp.y_3rd());
				dialog.push("Press "FK_F7" to switch to \"center-mag\" mode");
			}
		}
		dialog.push("Press "FK_F4" to reset to type default values");

		int result = dialog.prompt();
		if (result < 0)
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

		if (result == FIK_F4)  /* reset to type defaults */
		{
			g_escape_time_state.m_grid_fp.x_3rd() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
			g_escape_time_state.m_grid_fp.y_3rd() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
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

		if (center_mag)
		{
			if (cmpdbl(Xctr, dialog.values(0).uval.dval)
				|| cmpdbl(Yctr, dialog.values(1).uval.dval)
				|| cmpdbl(static_cast<double>(Magnification), dialog.values(2).uval.dval)
				|| cmpdbl(Xmagfactor, dialog.values(3).uval.dval)
				|| cmpdbl(Rotation, dialog.values(4).uval.dval)
				|| cmpdbl(Skew, dialog.values(5).uval.dval))
			{
				Xctr = dialog.values(0).uval.dval;
				Yctr = dialog.values(1).uval.dval;
				Magnification = dialog.values(2).uval.dval;
				Xmagfactor = dialog.values(3).uval.dval;
				Rotation = dialog.values(4).uval.dval;
				Skew = dialog.values(5).uval.dval;
				if (Xmagfactor == 0)
				{
					Xmagfactor = 1;
				}
				convert_corners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
			}
		}
		else
		{
			int i = 1;
			if (g_orbit_draw_mode == ORBITDRAW_LINE)
			{
				g_escape_time_state.m_grid_fp.x_min() = dialog.values(i++).uval.dval;
				g_escape_time_state.m_grid_fp.y_max() = dialog.values(i++).uval.dval;
				i++;
				g_escape_time_state.m_grid_fp.x_max() = dialog.values(i++).uval.dval;
				g_escape_time_state.m_grid_fp.y_min() = dialog.values(i++).uval.dval;
			}
			else
			{
				g_escape_time_state.m_grid_fp.x_min() = dialog.values(i++).uval.dval;
				g_escape_time_state.m_grid_fp.y_max() = dialog.values(i++).uval.dval;
				i++;
				g_escape_time_state.m_grid_fp.x_max() = dialog.values(i++).uval.dval;
				g_escape_time_state.m_grid_fp.y_min() = dialog.values(i++).uval.dval;
				i++;
				g_escape_time_state.m_grid_fp.x_3rd() = dialog.values(i++).uval.dval;
				g_escape_time_state.m_grid_fp.y_3rd() = dialog.values(i++).uval.dval;
				if (g_escape_time_state.m_grid_fp.x_3rd() == 0 && g_escape_time_state.m_grid_fp.y_3rd() == 0)
				{
					g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
					g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
				}
			}
		}

		if (result == FIK_F7 && g_orbit_draw_mode != ORBITDRAW_LINE)  /* toggle corners/center-mag mode */
		{
			if (!g_use_center_mag)
			{
				convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
				g_use_center_mag = true;
			}
			else
			{
				g_use_center_mag = false;
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
}

static int get_screen_corners()
{
	const char *xprompt = "          X";
	const char *yprompt = "          Y";
	const char *zprompt = "          Z";
	bool ousemag = g_use_center_mag;
	double svxxmin = g_escape_time_state.m_grid_fp.x_min();  /* save these for later since convert_corners modifies them */
	double svxxmax = g_escape_time_state.m_grid_fp.x_max();  /* and we need to set them for convert_center_mag to work */
	double svxx3rd = g_escape_time_state.m_grid_fp.x_3rd();
	double svyymin = g_escape_time_state.m_grid_fp.y_min();
	double svyymax = g_escape_time_state.m_grid_fp.y_max();
	double svyy3rd = g_escape_time_state.m_grid_fp.y_3rd();
	if (!g_set_orbit_corners && !g_keep_screen_coords)
	{
		g_orbit_x_min = g_escape_time_state.m_grid_fp.x_min();
		g_orbit_x_max = g_escape_time_state.m_grid_fp.x_max();
		g_orbit_x_3rd = g_escape_time_state.m_grid_fp.x_3rd();
		g_orbit_y_min = g_escape_time_state.m_grid_fp.y_min();
		g_orbit_y_max = g_escape_time_state.m_grid_fp.y_max();
		g_orbit_y_3rd = g_escape_time_state.m_grid_fp.y_3rd();
	}
	double oxxmin = g_orbit_x_min;
	double oxxmax = g_orbit_x_max;
	double oyymin = g_orbit_y_min;
	double oyymax = g_orbit_y_max;
	double oxx3rd = g_orbit_x_3rd;
	double oyy3rd = g_orbit_y_3rd;
	g_escape_time_state.m_grid_fp.x_min() = g_orbit_x_min;
	g_escape_time_state.m_grid_fp.x_max() = g_orbit_x_max;
	g_escape_time_state.m_grid_fp.y_min() = g_orbit_y_min;
	g_escape_time_state.m_grid_fp.y_max() = g_orbit_y_max;
	g_escape_time_state.m_grid_fp.x_3rd() = g_orbit_x_3rd;
	g_escape_time_state.m_grid_fp.y_3rd() = g_orbit_y_3rd;

gsc_loop:
	{
		UIChoices dialog(HELPSCRNCOORDS, "Screen Coordinates", 0x90);
		double Skew;
		double Rotation;
		LDBL Magnification;
		double Xmagfactor;
		double Yctr;
		double Xctr;
		convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

		if (g_use_center_mag)
		{
			dialog.push("Center X", Xctr);
			dialog.push("Center Y", Yctr);
			dialog.push("Magnification", static_cast<double>(Magnification));
			dialog.push("X Magnification Factor", Xmagfactor);
			dialog.push("Rotation Angle (degrees)", Rotation);
			dialog.push("Skew Angle (degrees)", Skew);
			dialog.push("");
			dialog.push("Press "FK_F7" to switch to \"corners\" mode");
		}
		else
		{
			dialog.push("Top-Left Corner");
			dialog.push(xprompt, g_orbit_x_min);
			dialog.push(yprompt, g_orbit_y_max);
			dialog.push("Bottom-Right Corner");
			dialog.push(xprompt, g_orbit_x_max);
			dialog.push(yprompt, g_orbit_y_min);
			if (g_orbit_x_min == g_orbit_x_3rd && g_orbit_y_min == g_orbit_y_3rd)
			{
				g_orbit_x_3rd = 0;
				g_orbit_y_3rd = 0;
			}
			dialog.push("Bottom-left (zeros for top-left X, bottom-right Y)");
			dialog.push(xprompt, g_orbit_x_3rd);
			dialog.push(yprompt, g_orbit_y_3rd);
			dialog.push("Press "FK_F7" to switch to \"center-mag\" mode");
		}
		dialog.push("Press "FK_F4" to reset to type default values");

		int prompt_ret = dialog.prompt();
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

		if (g_use_center_mag)
		{
			if (cmpdbl(Xctr, dialog.values(0).uval.dval)
				|| cmpdbl(Yctr, dialog.values(1).uval.dval)
				|| cmpdbl(static_cast<double>(Magnification), dialog.values(2).uval.dval)
				|| cmpdbl(Xmagfactor, dialog.values(3).uval.dval)
				|| cmpdbl(Rotation, dialog.values(4).uval.dval)
				|| cmpdbl(Skew, dialog.values(5).uval.dval))
			{
				Xctr = dialog.values(0).uval.dval;
				Yctr = dialog.values(1).uval.dval;
				Magnification = dialog.values(2).uval.dval;
				Xmagfactor = dialog.values(3).uval.dval;
				Rotation = dialog.values(4).uval.dval;
				Skew = dialog.values(5).uval.dval;
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
			int nump = 1;
			g_orbit_x_min = dialog.values(nump++).uval.dval;
			g_orbit_y_max = dialog.values(nump++).uval.dval;
			nump++;
			g_orbit_x_max = dialog.values(nump++).uval.dval;
			g_orbit_y_min = dialog.values(nump++).uval.dval;
			nump++;
			g_orbit_x_3rd = dialog.values(nump++).uval.dval;
			g_orbit_y_3rd = dialog.values(nump++).uval.dval;
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
				g_use_center_mag = true;
			}
			else
			{
				g_use_center_mag = false;
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
}

/* get browse parameters , called from fractint.c and loadfile.c
	returns 3 if anything changes.  code pinched from get_view_params */

int get_browse_parameters()
{
	bool old_auto_browse = g_auto_browse;
	bool old_browse_check_type = g_browse_check_type;
	bool old_browse_check_parameters = g_browse_check_parameters;
	bool old_double_caution = g_ui_state.double_caution;
	int old_cross_hair_box_size = g_cross_hair_box_size;
	double old_too_small = g_too_small;
	char old_browse_mask[FILE_MAX_FNAME];
	strcpy(old_browse_mask, g_browse_mask);

get_brws_restart:
	{
		UIChoices dialog(HELPBRWSPARMS, "Browse ('L'ook) Mode Options", 16);

		dialog.push("Autobrowsing? (y/n)", g_auto_browse);
		dialog.push("Ask about GIF video mode? (y/n)", g_ui_state.ask_video);
		dialog.push("Check fractal type? (y/n)", g_browse_check_type);
		dialog.push("Check fractal parameters (y/n)", g_browse_check_parameters);
		dialog.push("Confirm file deletes (y/n)", g_ui_state.double_caution);
		dialog.push("Smallest window to display (size in pixels)", static_cast<float>(g_too_small));
		dialog.push("Smallest box size shown before crosshairs used (pix)", g_cross_hair_box_size);
		dialog.push("Browse search filename mask ", g_browse_mask);
		dialog.push("");
		dialog.push("Press "FK_F4" to reset browse parameters to defaults.");

		int result = dialog.prompt();
		if (result < 0)
		{
			return 0;
		}

		if (result == FIK_F4)
		{
			g_too_small = 6;
			g_auto_browse = false;
			g_ui_state.ask_video = true;
			g_browse_check_parameters = true;
			g_browse_check_type  = true;
			g_ui_state.double_caution  = true;
			g_cross_hair_box_size = 3;
			strcpy(g_browse_mask, "*.gif");
			goto get_brws_restart;
		}

		int k = -1;
		g_auto_browse = (dialog.values(++k).uval.ch.val != 0);
		g_ui_state.ask_video = (dialog.values(++k).uval.ch.val != 0);
		g_browse_check_type = (dialog.values(++k).uval.ch.val != 0);
		g_browse_check_parameters = (dialog.values(++k).uval.ch.val != 0);
		g_ui_state.double_caution = (dialog.values(++k).uval.ch.val != 0);
		g_too_small  = dialog.values(++k).uval.dval;
		if (g_too_small < 0)
		{
			g_too_small = 0;
		}
		g_cross_hair_box_size = dialog.values(++k).uval.ival;
		if (g_cross_hair_box_size < 1)
		{
			g_cross_hair_box_size = 1;
		}
		if (g_cross_hair_box_size > 10)
		{
			g_cross_hair_box_size = 10;
		}
		strcpy(g_browse_mask, dialog.values(++k).uval.sval);

		int i = 0;
		if (g_auto_browse != old_auto_browse ||
			g_browse_check_type != old_browse_check_type ||
			g_browse_check_parameters != old_browse_check_parameters ||
			g_ui_state.double_caution != old_double_caution ||
			g_too_small != old_too_small ||
			g_cross_hair_box_size != old_cross_hair_box_size ||
			!stricmp(g_browse_mask, old_browse_mask))
		{
			i = -3;
		}
		if (g_evolving_flags)  /* can't browse */
		{
			g_auto_browse = false;
			i = 0;
		}

		return i;
	}
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
		int len;
		int test_dir = 0;
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
	int gap;
	int i;
	int j;
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
