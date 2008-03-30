/*
		Various routines that prompt for things.
*/
#include <sstream>
#include <string>

#include <string.h>
#include <ctype.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "Browse.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "evolve.h"
#include "Externals.h"
#include "fihelp.h"
#include "filesystem.h"
#include "fimain.h"
#include "FiniteAttractor.h"
#include "fracsubr.h"
#include "GaussianDistribution.h"
#include "history.h"
#include "line3d.h"
#include "loadfile.h"
#include "loadmap.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "resume.h"
#include "slideshw.h"
#include "stereo.h"
#include "TextColors.h"
#include "zoom.h"

#include "busy.h"
#include "EscapeTime.h"
#include "FileNameGetter.h"
#include "MathUtil.h"
#include "SoundState.h"
#include "UIChoices.h"
#include "ViewWindow.h"

static  int get_screen_corners();

static int calculation_mode()
{
	switch (g_externs.UserStandardCalculationMode())
	{
	case CALCMODE_SINGLE_PASS:			return 0;
	case CALCMODE_DUAL_PASS:			return 1;
	case CALCMODE_TRIPLE_PASS:			return 2;
	case CALCMODE_SOLID_GUESS:			return 3 + g_externs.StopPass();
	case CALCMODE_BOUNDARY_TRACE:		return 10;
	case CALCMODE_SYNCHRONOUS_ORBITS:	return 11;
	case CALCMODE_TESSERAL:				return 12;
	case CALCMODE_DIFFUSION:			return 13;
	case CALCMODE_ORBITS:				return 14;
	default:
		assert(!"Bad g_user_standard_calculation_mode");
	}
	return -1;
}

static int inside_mode()
{
	if (g_externs.Inside() >= 0)  // numb 
	{
		return 0;
	}
	
	switch (g_externs.Inside())
	{
	case COLORMODE_ITERATION:					return 1;
	case COLORMODE_Z_MAGNITUDE:					return 2;
	case COLORMODE_BEAUTY_OF_FRACTALS_60:		return 3;
	case COLORMODE_BEAUTY_OF_FRACTALS_61:		return 4;
	case COLORMODE_EPSILON_CROSS:				return 5;
	case COLORMODE_STAR_TRAIL:					return 6;
	case COLORMODE_PERIOD:						return 7;
	case COLORMODE_INVERSE_TANGENT_INTEGER:		return 8;
	case COLORMODE_FLOAT_MODULUS_INTEGER:		return 9;
	default:
		assert(!"Bad inside value");
	}
	return -1;
}

static std::string save_name()
{
	std::string::size_type pos = g_save_name.find_last_of(SLASHC);
	return (pos == std::string::npos) ? g_save_name : g_save_name.substr(pos + 1);
}

// --------------------------------------------------------------------- 
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
	CalculationMode old_user_standard_calculation_mode = g_externs.UserStandardCalculationMode();
	int old_stop_pass = g_externs.StopPass();
	long old_max_iteration = g_max_iteration;
	int old_inside = g_externs.Inside();
	int old_outside = g_externs.Outside();
	std::string previous_save_name = g_save_name;
	int old_sound_flags = g_sound_state.flags();
	long old_log_palette_flag = g_log_palette_mode;
	int old_biomorph = g_externs.UserBiomorph();
	int old_decomposition = g_decomposition[0];
	int old_fill_color = g_fill_color;
	double old_proximity = g_proximity;

	UIChoices dialog(FIHELP_TOGGLES, "Basic Options\n(not all combinations make sense)", 0);
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
	dialog.push("Inside Color (0-# of colors, if Inside=numb)", (g_externs.Inside() >= 0) ? g_externs.Inside() : 0);
	const char *insidemodes[] =
	{
		"numb", "maxiter", "zmag", "bof60", "bof61",
		"epsiloncross", "startrail", "period", "atan", "fmod"
	};
	dialog.push("Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)",
		insidemodes, NUM_OF(insidemodes), inside_mode());
	dialog.push("Outside Color (0-# of colors, if Outside=numb)", g_externs.Outside() >= 0 ? g_externs.Outside() : 0);
	const char *outsidemodes[] =
	{
		"numb", "iter", "real", "imag", "mult",
		"summ", "atan", "fmod", "tdis"
	};
	dialog.push("Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)",
		outsidemodes, NUM_OF(outsidemodes), g_externs.Outside() >= 0 ? 0 : -g_externs.Outside());
	dialog.push("Savename (.GIF implied)", save_name().c_str());
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
	dialog.push("Biomorph Color (-1 means OFF)", g_externs.UserBiomorph());
	dialog.push("Decomp Option (2,4,8,..,256, 0=OFF)", g_decomposition[0]);
	std::string fill_buffer = "normal";
	if (g_fill_color >= 0)
	{
		fill_buffer = str(boost::format("%d") % g_fill_color);
	}
	dialog.push("Fill Color (normal,#) (works with passes=t, b and d)", fill_buffer);
	dialog.push("Proximity value for inside=epscross and fmod", float(g_proximity));
	if (dialog.prompt() < 0)
	{
		return -1;
	}

	int k = -1;
	g_externs.SetUserStandardCalculationMode(CalculationMode(calculation_modes[dialog.values(++k).uval.ch.val][0]));
	g_externs.SetStopPass(int(calculation_modes[dialog.values(k).uval.ch.val][1]) - int('0'));
	if (g_externs.StopPass() < 0 || g_externs.StopPass() > 6 || g_externs.UserStandardCalculationMode() != CALCMODE_SOLID_GUESS)
	{
		g_externs.SetStopPass(0);
	}
	// Oops, lyapunov type doesn't use 'new' & breaks orbits 
	if (g_externs.UserStandardCalculationMode() == CALCMODE_ORBITS
		&& g_fractal_type == FRACTYPE_LYAPUNOV)
	{
		g_externs.SetUserStandardCalculationMode(old_user_standard_calculation_mode);
	}
	int j = 0;
	if (old_user_standard_calculation_mode != g_externs.UserStandardCalculationMode())
	{
		j++;
	}
	if (old_stop_pass != g_externs.StopPass())
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

	{
		int value = dialog.values(++k).uval.ival;
		if (value < 0)
		{
			value = -value;
		}
		if (value >= g_colors)
		{
			value = (value % g_colors) + (value/g_colors);
		}
		g_externs.SetInside(value);
	}

	{
		int tmp = dialog.values(++k).uval.ch.val;
		int insides[] =
		{
			0,
			COLORMODE_ITERATION,
			COLORMODE_Z_MAGNITUDE,
			COLORMODE_BEAUTY_OF_FRACTALS_60,
			COLORMODE_BEAUTY_OF_FRACTALS_61,
			COLORMODE_EPSILON_CROSS,
			COLORMODE_STAR_TRAIL,
			COLORMODE_PERIOD,
			COLORMODE_INVERSE_TANGENT_INTEGER,
			COLORMODE_FLOAT_MODULUS_INTEGER
		};
		if (tmp > 0 && tmp < NUM_OF(insides))
		{
			g_externs.SetInside(insides[tmp]);
		}
	}
	if (g_externs.Inside() != old_inside)
	{
		j++;
	}

	{
		int value = dialog.values(++k).uval.ival;
		if (value < 0)
		{
			value = -value;
		}
		if (value >= g_colors)
		{
			value = (value % g_colors) + (value/g_colors);
		}
		{
			int tmp = dialog.values(++k).uval.ch.val;
			if (tmp > 0)
			{
				value = -tmp;
			}
		}
		if (value != old_outside)
		{
			g_externs.SetOutside(value);
			j++;
		}
	}
	{
		std::string::size_type pos = g_save_name.find_last_of(SLASHC);
		if (pos != std::string::npos)
		{
			g_save_name.replace(pos+1, std::string::npos, dialog.values(++k).uval.sval);
		}
		else
		{
			g_save_name = dialog.values(++k).uval.sval;
		}
	}
	if (g_save_name != previous_save_name)
	{
		g_resave_mode = RESAVE_NO;
		g_started_resaves = false; // forget pending increment 
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
		g_log_automatic_flag = false;  // turn it off, use the supplied value 
	}

	{
		int value = dialog.values(++k).uval.ival;
		if (value >= g_colors)
		{
			value = (value % g_colors) + (value/g_colors);
		}
		if (value != old_biomorph)
		{
			g_externs.SetUserBiomorph(value);
			j++;
		}
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
		g_fill_color = (g_fill_color % g_colors) + (g_fill_color/g_colors);
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
	double old_potential_parameter[3];
	old_potential_parameter[0] = g_potential_parameter[0];
	old_potential_parameter[1] = g_potential_parameter[1];
	old_potential_parameter[2] = g_potential_parameter[2];
	long old_user_distance_test = g_user_distance_test;
	int old_distance_test_width = g_distance_test_width;
	int old_rotate_lo = g_rotate_lo;
	int old_rotate_hi = g_rotate_hi;
	double old_inversion[3];
	for (int i = 0; i < NUM_OF(old_inversion); i++)
	{
		old_inversion[i] = g_inversion[i];
	}

	UIChoices dialog(FIHELP_TOGGLES2, "Extended Options\n(not all combinations make sense)", 0);
	dialog.push("Look for finite attractor (0=no,>0=yes,<0=phase)", g_finite_attractor);
	dialog.push("Potential Max Color (0 means off)", int(old_potential_parameter[0]));
	dialog.push("          Slope", old_potential_parameter[1]);
	dialog.push("          Bailout", int(old_potential_parameter[2]));
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
			dialog.push(prompts[i], g_inversion[i] == AUTO_INVERT ?
				"auto" : str(boost::format("%-1.15lg") % g_inversion[i]));
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
	int j = 0;   // return code 
	if (dialog.values(++k).uval.ch.val != g_finite_attractor)
	{
		g_finite_attractor = dialog.values(k).uval.ch.val;
		if (g_finite_attractor > 0)
		{
			g_finite_attractor = FINITE_ATTRACTOR_YES;
		}
		else if (g_finite_attractor < 0)
		{
			g_finite_attractor = FINITE_ATTRACTOR_PHASE;
		}
		j = 1;
	}

	g_potential_parameter[0] = dialog.values(++k).uval.ival;
	if (g_potential_parameter[0] != old_potential_parameter[0])
	{
		j = 1;
	}

	g_potential_parameter[1] = dialog.values(++k).uval.dval;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[1] != old_potential_parameter[1])
	{
		j = 1;
	}

	g_potential_parameter[2] = dialog.values(++k).uval.ival;
	if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != old_potential_parameter[2])
	{
		j = 1;
	}

	if (dialog.values(++k).uval.ch.val != (g_potential_16bit ? 1 : 0))
	{
		g_potential_16bit = (dialog.values(k).uval.ch.val != 0);
		if (g_potential_16bit)  // turned it on 
		{
			if (g_potential_parameter[0] != 0.0)
			{
				j = 1;
			}
		}
		else // turned it off 
		{
			if (!driver_diskp()) // ditch the disk video 
			{
				disk_end();
			}
			else // keep disk video, but ditch the fraction part at end 
			{
				g_disk_16bit = false;
			}
		}
	}

	++k;
	g_user_distance_test = dialog.values(k).uval.Lval;
	if (g_user_distance_test != old_user_distance_test)
	{
		j = 1;
	}
	++k;
	g_distance_test_width = dialog.values(k).uval.ival;
	if (g_user_distance_test && g_distance_test_width != old_distance_test_width)
	{
		j = 1;
	}

	for (int i = 0; i < 3; i++)
	{
		if (dialog.values(++k).uval.sval[0] == 'a' || dialog.values(k).uval.sval[0] == 'A')
		{
			g_inversion[i] = AUTO_INVERT;
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
	old_orbit_interval = int(g_orbit_interval);
	bool old_keep_scrn_coords = g_keep_screen_coords;
	int old_drawmode;
	old_drawmode = g_orbit_draw_mode;

pass_option_restart:
	{
		UIChoices dialog(FIHELP_PASSES_OPTIONS, "Passes Options\n"
			"(not all combinations make sense)\n"
			"(Press "FK_F2" for corner parameters)\n"
			"(Press "FK_F6" for calculation parameters)", 0x44);
		dialog.push("Periodicity (0=off, <0=show, >0=on, -255..+255)", g_user_periodicity_check);
		dialog.push("Orbit delay (0 = none)", g_orbit_delay);
		dialog.push("Orbit interval (1 ... 255)", int(g_orbit_interval));
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
		g_keep_screen_coords = (dialog.values(++k).uval.ch.val != 0);
		if (g_keep_screen_coords != old_keep_scrn_coords)
		{
			j = 1;
		}
		if (!g_keep_screen_coords)
		{
			g_set_orbit_corners = false;
		}
		g_orbit_draw_mode = dialog.values(++k).uval.ch.val;
		if (g_orbit_draw_mode != old_drawmode)
		{
			j = 1;
		}

		if (i == IDK_F2)
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
		if (i == IDK_F6)
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


// for videomodes added new options "virtual x/y" that change "sx/g_y_dots" 
// for diskmode changed "viewx/g_y_dots" to "virtual x/y" that do as above  
// (since for diskmode they were updated by x/g_y_dots that should be the   
// same as sx/g_y_dots for that mode)                                       
// g_video_table and g_.VideoEntry() are now updated even for non-disk modes     

int get_disk_view_params()
{
	int x_max;
	int y_max;
	driver_get_max_screen(x_max, y_max);
	float old_aspectratio = g_viewWindow.AspectRatio();
	int old_sxdots = g_screen_width;
	int old_sydots = g_screen_height;

	UIChoices dialog(FIHELP_VIEW_WINDOW, "View Window Options", 16);

	dialog.push("Disk Video x pixels", g_screen_width);
	dialog.push("           y pixels", g_screen_height);
	dialog.push("");

	if (dialog.prompt() < 0)
	{
		return -1;
	}

	g_screen_width = dialog.values(0).uval.ival;
	g_screen_height = dialog.values(1).uval.ival;
	if (x_max != -1)
	{
		g_screen_width = std::min(x_max, g_screen_width);
	}
	g_screen_width = std::max(2, g_screen_width);
	if (y_max != -1)
	{
		g_screen_height = std::min(y_max, g_screen_height);
	}
	g_screen_height = std::max(2, g_screen_height);

	g_.SetVideoEntrySize(g_screen_width, g_screen_height);
	g_viewWindow.SetAspectRatio(float(g_screen_height)/float(g_screen_width));
	g_.SetVideoTable(g_.Adapter(), g_.VideoEntry());

	return (g_screen_width != old_sxdots || g_screen_height != old_sydots) ? 1 : 0;
}

// --------------------------------------------------------------------- 
/*
	get_view_params() is called from FRACTINT.C whenever the 'v' key
	is pressed.  Return codes are:
		-1  routine was ESCAPEd - no need to re-generate the image.
			0  minor variable changed.  No need to re-generate the image.
			1  View changed.  Re-generate the image.
*/

int get_view_params()
{
	return driver_diskp() ? get_disk_view_params() : g_viewWindow.GetParameters();
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

	i = field_prompt_help(FIHELP_COMMANDS, "Enter command string to use.", 0, cmdbuf, 60, 0);
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


// --------------------------------------------------------------------- 



double starfield_values[4] =
{
	30.0, 100.0, 5.0, 0.0
};

const std::string GREY_MAP = "altern.map";

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

	GaussianDistribution::SetDistribution(int(starfield_values[0]));
	GaussianDistribution::SetConstant(long(((starfield_values[1])/100.0)*(1L << 16)));
	GaussianDistribution::SetSlope(int(starfield_values[2]));

	if (validate_luts(GREY_MAP))
	{
		stop_message(STOPMSG_NORMAL, "Unable to load ALTERN.MAP");
		return -1;
	}
	load_dac();
	for (g_row = 0; g_row < g_y_dots; g_row++)
	{
		for (g_col = 0; g_col < g_x_dots; g_col++)
		{
			if (driver_key_pressed())
			{
				driver_buzzer(BUZZER_INTERRUPT);
				return 1;
			}
			c = get_color(g_col, g_row);
			if (c == g_externs.Inside())
			{
				c = g_colors-1;
			}
			g_plot_color_put_color(g_col, g_row, GaussianDistribution::Evaluate(c, g_colors));
		}
	}
	driver_buzzer(BUZZER_COMPLETE);
	return 0;
}

int get_starfield_params()
{
	UIChoices dialog(FIHELP_STARFIELDS, "Starfield Parameters", 0);
	const char *starfield_prompts[3] =
	{
		"Star Density in Pixels per Star",
		"Percent Clumpiness",
		"Ratio of Dim stars to Bright"
	};
	for (int i = 0; i < 3; i++)
	{
		dialog.push(starfield_prompts[i], float(starfield_values[i]));
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

// --------------------------------------------------------------------- 

int get_commands()              // execute commands from file 
{
	std::ifstream::pos_type point;
	static char commandmask[13] = {"*.par"};

	point = std::ifstream::pos_type(get_file_entry_help(FIHELP_PARAMETER_FILES, GETFILE_PARAMETER, "Parameter Set",
		commandmask, g_command_file, g_command_name));
	if (point >= 0)
	{
		std::ifstream parmfile(g_command_file.c_str(), std::ios::in | std::ios::binary);
		if (parmfile)
		{
			parmfile.seekg(point, SEEK_SET);
			return load_commands(parmfile);
		}
	}
	return 0;
}

// --------------------------------------------------------------------- 

void goodbye()
{
	char *goodbyemessage = "   Thank You for using Iterated Dynamics";
	int ret;

	delete g_.MapDAC();
	g_.SetMapDAC(0);
	if (g_resume_info != 0)
	{
		end_resume();
	}
	delete g_evolve_info;
	g_evolve_info = 0;
	release_parameter_box();
	history_free();
	delete[] g_ifs_definition;
	g_ifs_definition = 0;
	disk_end();
	ExitCheck();
#ifdef WINFRACT
	return;
#endif
	if (g_make_par_flag)
	{
		driver_set_for_text();
	}
#ifdef XFRACT
	UnixDone();
#endif
	if (g_make_par_flag)
	{
		driver_move_cursor(6, 0);
	}
	g_slideShow.Stop();
	end_help();
	ret = 0;
	if (g_initialize_batch == INITBATCH_BAILOUT_ERROR) // exit with error code for batch file 
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

	while (true)
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

int lccompare(VOIDPTR arg1, VOIDPTR arg2) // for sort 
{
	char **choice1 = (char **) arg1;
	char **choice2 = (char **) arg2;

	return stricmp(*choice1, *choice2);
}

int get_a_filename(const std::string &hdg, char *file_template, char *flname)
{
	std::string fileTemplate = file_template;
	std::string fileName = flname;
	int result = FileNameGetter(hdg, fileTemplate, fileName).Execute();
	strcpy(file_template, fileTemplate.c_str());
	strcpy(flname, fileName.c_str());
	return result;
}

int get_a_filename(const std::string &heading, std::string &fileTemplate, std::string &filename)
{
	return FileNameGetter(heading, fileTemplate, filename).Execute();
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

	// change the old value with the same torture the new value had 
	val.type = 'd'; // most values on this screen are type d 
	val.uval.dval = old;
	prompt_value_string(buf, &val);   // convert "old" to string 

	old = atof(buf);                // convert back 
	return fabs(old-new_value) < DBL_EPSILON?0:1;  // zero if same 
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
		UIChoices dialog(FIHELP_IMAGE_COORDINATES, "Image Coordinates", 0x90);
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
			dialog.push("Magnification", double(Magnification));
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

		if (result == IDK_F4)  // reset to type defaults 
		{
			g_escape_time_state.m_grid_fp.x_3rd() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
			g_escape_time_state.m_grid_fp.y_3rd() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
			if (g_viewWindow.Crop() && g_viewWindow.AspectRatio() != g_screen_aspect_ratio)
			{
				aspect_ratio_crop(g_screen_aspect_ratio, g_viewWindow.AspectRatio());
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
				|| cmpdbl(double(Magnification), dialog.values(2).uval.dval)
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

		if (result == IDK_F7 && g_orbit_draw_mode != ORBITDRAW_LINE)  // toggle corners/center-mag mode 
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
			// no change, restore values to avoid drift 
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
	double svxxmin = g_escape_time_state.m_grid_fp.x_min();  // save these for later since convert_corners modifies them 
	double svxxmax = g_escape_time_state.m_grid_fp.x_max();  // and we need to set them for convert_center_mag to work 
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
		UIChoices dialog(FIHELP_SCREEN_COORDINATES, "Screen Coordinates", 0x90);
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
			dialog.push("Magnification", double(Magnification));
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
			// restore corners 
			g_escape_time_state.m_grid_fp.x_min() = svxxmin;
			g_escape_time_state.m_grid_fp.x_max() = svxxmax;
			g_escape_time_state.m_grid_fp.y_min() = svyymin;
			g_escape_time_state.m_grid_fp.y_max() = svyymax;
			g_escape_time_state.m_grid_fp.x_3rd() = svxx3rd;
			g_escape_time_state.m_grid_fp.y_3rd() = svyy3rd;
			return -1;
		}

		if (prompt_ret == IDK_F4)  // reset to type defaults 
		{
			g_orbit_x_min = g_current_fractal_specific->x_min;
			g_orbit_x_max = g_current_fractal_specific->x_max;
			g_orbit_y_min = g_current_fractal_specific->y_min;
			g_orbit_y_max = g_current_fractal_specific->y_max;
			g_orbit_x_3rd = g_current_fractal_specific->x_min;
			g_orbit_y_3rd = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.x_min() = g_orbit_x_min;
			g_escape_time_state.m_grid_fp.x_max() = g_orbit_x_max;
			g_escape_time_state.m_grid_fp.y_min() = g_orbit_y_min;
			g_escape_time_state.m_grid_fp.y_max() = g_orbit_y_max;
			g_escape_time_state.m_grid_fp.x_3rd() = g_orbit_x_3rd;
			g_escape_time_state.m_grid_fp.y_3rd() = g_orbit_y_3rd;
			if (g_viewWindow.Crop() && g_viewWindow.AspectRatio() != g_screen_aspect_ratio)
			{
				aspect_ratio_crop(g_screen_aspect_ratio, g_viewWindow.AspectRatio());
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
				|| cmpdbl(double(Magnification), dialog.values(2).uval.dval)
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
				// set screen corners 
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

		if (prompt_ret == IDK_F7)  // toggle corners/center-mag mode 
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
			// no change, restore values to avoid drift 
			g_orbit_x_min = oxxmin;
			g_orbit_x_max = oxxmax;
			g_orbit_y_min = oyymin;
			g_orbit_y_max = oyymax;
			g_orbit_x_3rd = oxx3rd;
			g_orbit_y_3rd = oyy3rd;
			// restore corners 
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
			g_set_orbit_corners = true;
			g_keep_screen_coords = true;
			// restore corners 
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

// merge existing full path with new one  
// attempt to detect if file or directory 

// I tried heap sort also - this is faster! 
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
		stop_message(STOPMSG_NORMAL, "This integer fractal type is unimplemented;\n"
			"Use float=yes or the <X> screen to get a real image.");
	}

	return 0;
}
