/*
	loadfdos.cpp - Stuff split out of loadfile.cpp, for ancient crusty DOS
	reasons a long, long time ago in a code base far, far away.

	get_video_mode should return with:
		return code 0 for ok, -1 for error or cancelled by user
		video parameters setup for the mainline, in the dos case this means
		setting g_.InitialVideoMode to video mode, based on this will set up
		for and call setvideomode
		set view window on if file going to be loaded into a view smaller than
		physical screen, in this case also set view reduction, view x dots,
		view y dots, and view aspect ratio.
		set g_skip_x_dots and g_skip_y_dots, to 0 if all pixels are to be loaded,
		to 1 for every 2nd pixel, 2 for every 3rd, etc

*/
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "cmdfiles.h"
#include "Externals.h"
#include "FullScreenChooser.h"
#include "idhelp.h"
#include "loadfdos.h"
#include "realdos.h"
#include "StopMessage.h"
#include "VideoModeKeyName.h"
#include "ViewWindow.h"

// routines in this module

static int video_mode_compare(const void *, const void *);
static void format_item(int, char *);
static int check_mode_key(int, int);
static void format_video_info(int i, const char *err, char *buf);
static std::string format_video_info(int i, const char *err);

struct video_mode_sort_info
{
	int index;     // g_video_entry subscript
	unsigned flags; // flags for sort's compare, defined below
};
/* defines for flags; done this way instead of bit union to ensure ordering;
	these bits represent the sort sequence for video mode list */
enum
{
	VI_EXACT   = 0x8000, // unless the one and only exact match
	VI_NOKEY   = 512,  // if no function key assigned
	VI_SSMALL  = 128,  // screen smaller than file's screen
	VI_SBIG    =  64,  // screen bigger than file's screen
	VI_VSMALL  =  32,  // screen smaller than file's view
	VI_VBIG    =  16,  // screen bigger than file's view
	VI_CSMALL  =   8,  // mode has too few colors
	VI_CBIG    =   4,  // mode has excess colors
	VI_ASPECT  =   1  // aspect ratio bad
};

static int video_mode_compare(const void *p1, const void *p2)
{
	const video_mode_sort_info *ptr1 = (const video_mode_sort_info *) p1;
	const video_mode_sort_info *ptr2 = (const video_mode_sort_info *) p2;
	if (ptr1->flags < ptr2->flags)
	{
		return -1;
	}
	if (ptr1->flags > ptr2->flags)
	{
		return 1;
	}
	if (g_.VideoTable(ptr1->index).keynum < g_.VideoTable(ptr2->index).keynum)
	{
		return -1;
	}
	if (g_.VideoTable(ptr1->index).keynum > g_.VideoTable(ptr2->index).keynum)
	{
		return 1;
	}
	if (ptr1->index < ptr2->index)
	{
		return -1;
	}
	return 1;
}

static std::string format_video_info(int i, const char *err)
{
	g_.SetVideoEntry(i);
	std::string key_name = video_mode_key_name(g_.VideoEntry().keynum);
	std::string result = str(boost::format("%-5s %-25s %-4s %5d %5d %3d %-25s")  // 78 chars
			% key_name.c_str() % g_.VideoEntry().name % err
			% g_.VideoEntry().x_dots % g_.VideoEntry().y_dots
			% g_.VideoEntry().colors % g_.VideoEntry().comment);
	g_.SetVideoEntryXDots(0);
	return result;
}

static void format_video_info(int i, const char *err, char *buf)
{
	std::string result = format_video_info(i, err);
	strcpy(buf, result.c_str());
}

double video_mode_aspect_ratio(int width, int height)
{  // calc resulting aspect ratio for specified dots in current mode
	return double(height)/double(width)
		*double(g_.VideoEntry().x_dots)/double(g_.VideoEntry().y_dots)
		*g_screen_aspect_ratio;
}

static video_mode_sort_info *vidptr;

void initialize_video_sort_table(fractal_info const *info, video_mode_sort_info sort_table[MAXVIDEOMODES])
{
	for (int i = 0; i < g_.VideoTableLength(); ++i)
	{
		const VIDEOINFO &video = g_.VideoTable(i);
		unsigned tmpflags = VI_EXACT;
		if (video.keynum == 0)
		{
			tmpflags |= VI_NOKEY;
		}

		if (info->x_dots > video.x_dots || info->y_dots > video.y_dots)
		{
			tmpflags |= VI_SSMALL;
		}
		else if (info->x_dots < video.x_dots || info->y_dots < video.y_dots)
		{
			tmpflags |= VI_SBIG;
		}

		if (g_file_x_dots > video.x_dots || g_file_y_dots > video.y_dots)
		{
			tmpflags |= VI_VSMALL;
		}
		else if (g_file_x_dots < video.x_dots || g_file_y_dots < video.y_dots)
		{
			tmpflags |= VI_VBIG;
		}

		if (g_file_colors > video.colors)
		{
			tmpflags |= VI_CSMALL;
		}

		if (g_file_colors < video.colors)
		{
			tmpflags |= VI_CBIG;
		}

		if (i == g_.InitialVideoMode())
		{
			tmpflags -= VI_EXACT;
		}

		if (g_file_aspect_ratio != 0 && (tmpflags & VI_VSMALL) == 0)
		{
			double ftemp = video_mode_aspect_ratio(g_file_x_dots, g_file_y_dots);
			if (ftemp < g_file_aspect_ratio*0.98 ||
				ftemp > g_file_aspect_ratio*1.02)
			{
				tmpflags |= VI_ASPECT;
			}
		}

		sort_table[i].index = i;
		sort_table[i].flags  = tmpflags;
	}
}

static std::string GetInstructions(fractal_info const *info)
{
	std::string instructions =
		"Select a video mode.  Use the cursor keypad to move the pointer.\n"
		"Press ENTER for selected mode, or use a video mode function key.\n"
		"Press F1 for help, ";
	if (info->info_id[0] != 'G')
	{
		instructions += "TAB for fractal information, ";
	}
	instructions += "ESCAPE to back out.";
	return instructions;
}

static std::string GetHeading(fractal_info const *info, formula_info_extension_block const *formula_info)
{
	std::string heading;
	if (info->info_id[0] == 'G')
	{
		heading = "      Non-fractal GIF";
	}
	else
	{
		const char *nameptr = g_current_fractal_specific->get_type();
		if (g_display_3d)
		{
			nameptr = "3D Transform";
		}
		heading = "Type: " + std::string(nameptr);
		if ((!strcmp(nameptr, "formula")) ||
			(!strcmp(nameptr, "lsystem")) ||
			(!strncmp(nameptr, "ifs", 3))) // for ifs and ifs3d
		{
			heading += std::string(" -> ") + formula_info->form_name;
		}
	}
	heading = str(boost::format("File: %-44s  %d x %d x %d\n%-52s")
		% g_read_name % g_file_x_dots % g_file_y_dots % g_file_colors % heading);
	if (info->info_id[0] != 'G')
	{
		std::string version = str(boost::format("v%d.%01d") % (g_save_release/100) % ((g_save_release%100)/10));
		if (g_save_release % 100)
		{
			version += char((g_save_release % 10) + '0');
		}
		heading += version;
	}
	heading += "\n";
	if (info->info_id[0] != 'G')
	{
		if (g_.InitialVideoMode() < 0)
		{
			heading += "Saved in unknown video mode.";
		}
		else
		{
			heading += format_video_info(g_.InitialVideoMode(), "");
		}
	}
	if (g_file_aspect_ratio != 0 && g_file_aspect_ratio != g_screen_aspect_ratio)
	{
		heading += "\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings";
	}
	heading += "\n";
	return heading;
}

int get_skip_factor(int video_size, int file_size)
{
	int skip_factor = 1;
	while (skip_factor*video_size < file_size)
	{
		++skip_factor;
	}
	return skip_factor;
}

int get_video_mode(fractal_info const *info, formula_info_extension_block const *formula_info)
{
	int tmpxdots;
	int tmpydots;
	g_.SetInitialVideoModeNone();

	// try to find exact match for vid mode
	for (int i = 0; i < g_.VideoTableLength(); ++i)
	{
		const VIDEOINFO &vident = g_.VideoTable(i);
		if (info->x_dots == vident.x_dots && info->y_dots == vident.y_dots
			&& g_file_colors == vident.colors)
		{
			g_.SetInitialVideoMode(i);
			break;
		}
	}

	// exit in makepar mode if no exact match of video mode in file
	if (!g_make_par_flag && g_.InitialVideoMode() == -1)
	{
		return 0;
	}

	if (g_.InitialVideoMode() == -1) // try to find very good match for vid mode
	{
		for (int i = 0; i < g_.VideoTableLength(); ++i)
		{
			const VIDEOINFO &vident = g_.VideoTable(i);
			if (info->x_dots == vident.x_dots && info->y_dots == vident.y_dots
				&& g_file_colors == vident.colors)
			{
				g_.SetInitialVideoMode(i);
				break;
			}
		}
	}

	// setup table entry for each vid mode, flagged for how well it matches
	video_mode_sort_info sort_table[MAXVIDEOMODES];
	initialize_video_sort_table(info, sort_table);
	if (g_fast_restore  && !g_ui_state.ask_video)
	{
		g_.SetInitialVideoMode(g_.Adapter());
	}

	bool got_real_mode = false;
	if ((g_.InitialVideoMode() < 0 || (g_ui_state.ask_video && !g_initialize_batch)) && g_make_par_flag)
	{
		/* no exact match or (askvideo=yes and batch=no), and not
		in makepar mode, talk to user */

		qsort(sort_table, g_.VideoTableLength(), sizeof(sort_table[0]), video_mode_compare); // sort modes

		std::vector<int> attributes(g_.VideoTableLength());
		for (int i = 0; i < g_.VideoTableLength(); ++i)
		{
			attributes[i] = 1;
		}
		vidptr = &sort_table[0]; // for format_item

		// format heading
		int i = full_screen_choice_help(FIHELP_LOAD_FILE, 0, GetHeading(info, formula_info).c_str(),
			"key...name......................err...xdot..ydot.clr.comment..................",
			GetInstructions(info).c_str(), g_.VideoTableLength(), 0, &attributes[0],
			1, 13, 78, 0, format_item, 0, 0, check_mode_key);
		if (i == -1)
		{
			return -1;
		}
		if (i < 0)  // returned -100 - g_video_table entry number
		{
			g_.SetInitialVideoMode(-100 - i);
			got_real_mode = true;
		}
		else
		{
			g_.SetInitialVideoMode(sort_table[i].index);
		}
	}

	if (!got_real_mode)  // translate from temp table to permanent
	{
		int i = g_.InitialVideoMode();
		int key = g_.VideoTable(i).keynum;
		if (key != 0)
		{
			bool found = false;
			for (int k = 0; k < g_.VideoTableLength(); k++)
			{
				if (g_.VideoTable(k).keynum == key)
				{
					g_.SetInitialVideoMode(k);
					found = true;
					break;
				}
			}
			if (!found)
			{
				key = 0;
			}
		}
		if (key == 0) // mode has no key, add to reserved slot at end
		{
			g_.SetInitialVideoMode(MAXVIDEOMODES-1);
			g_.SetVideoTable(g_.InitialVideoMode(), g_.VideoTable(i));
		}
	}

	// ok, we're going to return with a video mode
	g_.SetVideoEntry(g_.InitialVideoMode());

	if (g_viewWindow.Visible()
		&& g_file_x_dots == g_.VideoEntry().x_dots
		&& g_file_y_dots == g_.VideoEntry().y_dots)
	{
		// pull image into a view window
		if (g_externs.CalculationStatus() != CALCSTAT_COMPLETED) // if not complete
		{
			g_externs.SetCalculationStatus(CALCSTAT_PARAMS_CHANGED);  // can't resume anyway
		}
		g_viewWindow.SetReductionFromVideoEntry(g_.VideoEntry());
		g_skip_x_dots = short(g_viewWindow.Reduction() - 1);
		g_skip_y_dots = g_skip_x_dots;
		return 0;
	}

	g_skip_x_dots = 0;
	g_skip_y_dots = 0; // set for no reduction
	if (g_.VideoEntry().x_dots < g_file_x_dots || g_.VideoEntry().y_dots < g_file_y_dots)
	{
		// set up to load only every nth pixel to make image fit
		if (g_externs.CalculationStatus() != CALCSTAT_COMPLETED) // if not complete
		{
			g_externs.SetCalculationStatus(CALCSTAT_PARAMS_CHANGED);  // can't resume anyway
		}
		g_skip_x_dots = get_skip_factor(g_.VideoEntry().x_dots, g_file_x_dots);
		g_skip_y_dots = get_skip_factor(g_.VideoEntry().y_dots, g_file_y_dots);
		bool reduced_y = false;
		bool reduced_x = false;
		while (true)
		{
			tmpxdots = (g_file_x_dots + g_skip_x_dots - 1)/g_skip_x_dots;
			tmpydots = (g_file_y_dots + g_skip_y_dots - 1)/g_skip_y_dots;
			// reduce further if that improves aspect
			double ftemp = video_mode_aspect_ratio(tmpxdots, tmpydots);
			double ftemp2;
			if (ftemp > g_file_aspect_ratio)
			{
				if (reduced_x)
				{
					break; // already reduced x, don't reduce y
				}
				ftemp2 = video_mode_aspect_ratio(tmpxdots, (g_file_y_dots + g_skip_y_dots)/(g_skip_y_dots + 1));
				if (ftemp2 < g_file_aspect_ratio &&
					ftemp/g_file_aspect_ratio *0.9 <= g_file_aspect_ratio/ftemp2)
				{
					break; // further y reduction is worse
				}
				++g_skip_y_dots;
				reduced_y = true;
			}
			else
			{
				if (reduced_y)
				{
					break; // already reduced y, don't reduce x
				}
				ftemp2 = video_mode_aspect_ratio((g_file_x_dots + g_skip_x_dots)/(g_skip_x_dots + 1), tmpydots);
				if (ftemp2 > g_file_aspect_ratio &&
					g_file_aspect_ratio/ftemp *0.9 <= ftemp2/g_file_aspect_ratio)
				{
					break; // further x reduction is worse
				}
				++g_skip_x_dots;
				reduced_x = true;
			}
		}
		g_file_x_dots = tmpxdots;
		g_file_y_dots = tmpydots;
		--g_skip_x_dots;
		--g_skip_y_dots;
	}

	g_viewWindow.SetFromVideoEntry();

	if (g_make_par_flag && !g_fast_restore && !g_initialize_batch &&
		(std::abs(g_viewWindow.AspectRatio() - g_screen_aspect_ratio) > .00001 || g_viewWindow.Width() != 0))
	{
		stop_message(STOPMSG_NO_BUZZER,
			"Warning: <V>iew parameters are being set to non-standard values.\n"
			"Remember to reset them when finished with this image!");
	}
	return 0;
}

static void format_item(int choice, char *buf)
{
	std::string errors = "";
	unsigned tmpflags = vidptr[choice].flags;
	if (tmpflags & (VI_VSMALL + VI_CSMALL + VI_ASPECT))
	{
		errors.append(1, '*');
	}
	if (tmpflags & VI_VSMALL)
	{
		errors.append(1, 'R');
	}
	if (tmpflags & VI_CSMALL)
	{
		errors.append(1, 'C');
	}
	if (tmpflags & VI_ASPECT)
	{
		errors.append(1, 'A');
	}
	if (tmpflags & VI_VBIG)
	{
		errors.append(1, 'v');
	}
	if (tmpflags & VI_CBIG)
	{
		errors.append(1, 'c');
	}
	format_video_info(vidptr[choice].index, errors.c_str(), buf);
}

static int check_mode_key(int curkey, int)
{
	int i = check_video_mode_key(curkey);
	return (i >= 0) ? -100-i : 0;
}
