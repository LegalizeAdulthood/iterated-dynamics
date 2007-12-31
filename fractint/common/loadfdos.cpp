/*
	loadfdos.cpp - Stuff split out of loadfile.cpp, for ancient crusty DOS
	reasons a long, long time ago in a code base far, far away.

	get_video_mode should return with:
		return code 0 for ok, -1 for error or cancelled by user
		video parameters setup for the mainline, in the dos case this means
		setting g_.InitialAdapter to video mode, based on this fractint.c will set up
		for and call setvideomode
		set g_view_window on if file going to be loaded into a view smaller than
		physical screen, in this case also set g_view_reduction, g_view_x_dots,
		g_view_y_dots, and g_final_aspect_ratio
		set g_skip_x_dots and g_skip_y_dots, to 0 if all pixels are to be loaded,
		to 1 for every 2nd pixel, 2 for every 3rd, etc

*/
#include <string>

#include <string.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "cmdfiles.h"
#include "fihelp.h"
#include "FullScreenChooser.h"
#include "loadfdos.h"
#include "realdos.h"

/* routines in this module      */

static int video_mode_compare(const void *, const void *);
static void format_item(int, char *);
static int check_mode_key(int, int);
static void format_video_info(int i, const char *err, char *buf);
static std::string format_video_info(int i, const char *err);
static double video_mode_aspect_ratio(int width, int height);

struct video_mode_info
{
	int index;     /* g_video_entry subscript */
	unsigned flags; /* flags for sort's compare, defined below */
};
/* defines for flags; done this way instead of bit union to ensure ordering;
	these bits represent the sort sequence for video mode list */
#define VI_EXACT 0x8000 /* unless the one and only exact match */
#define VI_NOKEY   512  /* if no function key assigned */
#define VI_SSMALL  128  /* screen smaller than file's screen */
#define VI_SBIG     64  /* screen bigger than file's screen */
#define VI_VSMALL   32  /* screen smaller than file's view */
#define VI_VBIG     16  /* screen bigger than file's view */
#define VI_CSMALL    8  /* mode has too few colors */
#define VI_CBIG      4  /* mode has excess colors */
#define VI_ASPECT    1  /* aspect ratio bad */

static int video_mode_compare(const void *p1, const void *p2)
{
	const video_mode_info *ptr1 = (const video_mode_info *) p1;
	const video_mode_info *ptr2 = (const video_mode_info *) p2;
	if (ptr1->flags < ptr2->flags)
	{
		return -1;
	}
	if (ptr1->flags > ptr2->flags)
	{
		return 1;
	}
	if (g_video_table[ptr1->index].keynum < g_video_table[ptr2->index].keynum)
	{
		return -1;
	}
	if (g_video_table[ptr1->index].keynum > g_video_table[ptr2->index].keynum)
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
	g_.SetVideoEntry(g_video_table[i]);
	std::string key_name = video_mode_key_name(g_.VideoEntry().keynum);
	std::string result = str(boost::format("%-5s %-25s %-4s %5d %5d %3d %-25s")  /* 78 chars */
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

static double video_mode_aspect_ratio(int width, int height)
{  /* calc resulting aspect ratio for specified dots in current mode */
	return double(height)/double(width)
		*double(g_.VideoEntry().x_dots)/double(g_.VideoEntry().y_dots)
		*g_screen_aspect_ratio;
	}

static video_mode_info *vidptr;

int get_video_mode(const fractal_info *info, struct ext_blk_formula_info *formula_info)
{
	int j;
	int gotrealmode;
	double ftemp;
	double ftemp2;
	unsigned tmpflags;
	int tmpxdots;
	int tmpydots;
	float tmpreduce;
#if !defined(XFRACT)
#endif
	VIDEOINFO *vident;

	g_.SetInitialAdapter(-1);

	/* try to find exact match for vid mode */
	for (int i = 0; i < g_.VideoTableLength(); ++i)
	{
		vident = &g_video_table[i];
		if (info->x_dots == vident->x_dots && info->y_dots == vident->y_dots
			&& g_file_colors == vident->colors)
		{
			g_.SetInitialAdapter(i);
			break;
		}
	}

	/* exit in makepar mode if no exact match of video mode in file */
	if (!g_make_par_flag && g_.InitialAdapter() == -1)
	{
		return 0;
	}

	if (g_.InitialAdapter() == -1) /* try to find very good match for vid mode */
	{
		for (int i = 0; i < g_.VideoTableLength(); ++i)
		{
			vident = &g_video_table[i];
			if (info->x_dots == vident->x_dots && info->y_dots == vident->y_dots
				&& g_file_colors == vident->colors)
			{
				g_.SetInitialAdapter(i);
				break;
			}
		}
	}

	/* setup table entry for each vid mode, flagged for how well it matches */
	video_mode_info vid[MAXVIDEOMODES];
	for (int i = 0; i < g_.VideoTableLength(); ++i)
	{
		g_.SetVideoEntry(g_video_table[i]);
		tmpflags = VI_EXACT;
		if (g_.VideoEntry().keynum == 0)
		{
			tmpflags |= VI_NOKEY;
		}
		if (info->x_dots > g_.VideoEntry().x_dots || info->y_dots > g_.VideoEntry().y_dots)
		{
			tmpflags |= VI_SSMALL;
		}
		else if (info->x_dots < g_.VideoEntry().x_dots || info->y_dots < g_.VideoEntry().y_dots)
		{
			tmpflags |= VI_SBIG;
		}
		if (g_file_x_dots > g_.VideoEntry().x_dots || g_file_y_dots > g_.VideoEntry().y_dots)
		{
			tmpflags |= VI_VSMALL;
		}
		else if (g_file_x_dots < g_.VideoEntry().x_dots || g_file_y_dots < g_.VideoEntry().y_dots)
		{
			tmpflags |= VI_VBIG;
		}
		if (g_file_colors > g_.VideoEntry().colors)
		{
			tmpflags |= VI_CSMALL;
		}
		if (g_file_colors < g_.VideoEntry().colors)
		{
			tmpflags |= VI_CBIG;
		}
		if (i == g_.InitialAdapter())
		{
			tmpflags -= VI_EXACT;
		}
		if (g_file_aspect_ratio != 0 && (tmpflags & VI_VSMALL) == 0)
		{
			ftemp = video_mode_aspect_ratio(g_file_x_dots, g_file_y_dots);
			if (ftemp < g_file_aspect_ratio*0.98 ||
				ftemp > g_file_aspect_ratio*1.02)
			{
				tmpflags |= VI_ASPECT;
			}
		}
		vid[i].index = i;
		vid[i].flags  = tmpflags;
	}

	if (g_fast_restore  && !g_ui_state.ask_video)
	{
		g_.SetInitialAdapter(g_.Adapter());
	}

#ifndef XFRACT
	gotrealmode = 0;
	if ((g_.InitialAdapter() < 0 || (g_ui_state.ask_video && !g_initialize_batch)) && g_make_par_flag)
	{
		/* no exact match or (askvideo=yes and batch=no), and not
			in makepar mode, talk to user */

		qsort(vid, g_.VideoTableLength(), sizeof(vid[0]), video_mode_compare); /* sort modes */

		int *attributes = new int[g_.VideoTableLength()];
		for (int i = 0; i < g_.VideoTableLength(); ++i)
		{
			attributes[i] = 1;
		}
		vidptr = &vid[0]; /* for format_item */

		/* format heading */
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
				(!strncmp(nameptr, "ifs", 3))) /* for ifs and ifs3d */
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
			if (g_.InitialAdapter() < 0)
			{
				heading += "Saved in unknown video mode.";
			}
			else
			{
				heading += format_video_info(g_.InitialAdapter(), "");
			}
		}
		if (g_file_aspect_ratio != 0 && g_file_aspect_ratio != g_screen_aspect_ratio)
		{
			heading += "\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings";
		}
		heading += "\n";
		std::string instructions =
			"Select a video mode.  Use the cursor keypad to move the pointer.\n"
			"Press ENTER for selected mode, or use a video mode function key.\n"
			"Press F1 for help, ";
		if (info->info_id[0] != 'G')
		{
			instructions += "TAB for fractal information, ";
		}
		instructions += "ESCAPE to back out.";

		int i = full_screen_choice_help(HELPLOADFILE, 0, heading.c_str(),
			"key...name......................err...xdot..ydot.clr.comment..................",
			instructions.c_str(), g_.VideoTableLength(), 0, attributes,
			1, 13, 78, 0, format_item, 0, 0, check_mode_key);
		delete[] attributes;
		if (i == -1)
		{
			return -1;
		}
		if (i < 0)  /* returned -100 - g_video_table entry number */
		{
			g_.SetInitialAdapter(-100 - i);
			gotrealmode = 1;
		}
		else
		{
			g_.SetInitialAdapter(vid[i].index);
		}
	}
#else
	g_.SetInitialAdapter(0);
	j = g_video_table[0].keynum;
	gotrealmode = 0;
#endif

	if (gotrealmode == 0)  /* translate from temp table to permanent */
	{
		int i = g_.InitialAdapter();
		int key = g_video_table[i].keynum;
		if (key != 0)
		{
			int k;
			for (k = 0; k < MAXVIDEOMODES-1; k++)
			{
				if (g_video_table[k].keynum == key)
				{
					g_.SetInitialAdapter(k);
					break;
				}
			}
			if (k >= MAXVIDEOMODES-1)
			{
				key = 0;
			}
		}
		if (key == 0) /* mode has no key, add to reserved slot at end */
		{
			g_.SetInitialAdapter(MAXVIDEOMODES-1);
			g_video_table[g_.InitialAdapter()] = g_video_table[i];
		}
	}

	/* ok, we're going to return with a video mode */
	g_.SetVideoEntry(g_video_table[g_.InitialAdapter()]);

	if (g_view_window
		&& g_file_x_dots == g_.VideoEntry().x_dots
		&& g_file_y_dots == g_.VideoEntry().y_dots)
	{
		/* pull image into a view window */
		if (g_calculation_status != CALCSTAT_COMPLETED) /* if not complete */
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;  /* can't resume anyway */
		}
		if (g_view_x_dots)
		{
			g_view_reduction = float(g_.VideoEntry().x_dots/g_view_x_dots);
			g_view_x_dots = g_view_y_dots = 0; /* easier to use auto reduction */
		}
		g_view_reduction = float(int(g_view_reduction + 0.5)); /* need integer value */
		g_skip_x_dots = g_skip_y_dots = short(g_view_reduction - 1);
		return 0;
	}

	g_skip_x_dots = 0;
	g_skip_y_dots = 0; /* set for no reduction */
	if (g_.VideoEntry().x_dots < g_file_x_dots || g_.VideoEntry().y_dots < g_file_y_dots)
	{
		/* set up to load only every nth pixel to make image fit */
		if (g_calculation_status != CALCSTAT_COMPLETED) /* if not complete */
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;  /* can't resume anyway */
		}
		g_skip_x_dots = 1;
		g_skip_y_dots = 1;
		while (g_skip_x_dots*g_.VideoEntry().x_dots < g_file_x_dots)
		{
			++g_skip_x_dots;
		}
		while (g_skip_y_dots*g_.VideoEntry().y_dots < g_file_y_dots)
		{
			++g_skip_y_dots;
		}
		bool reduced_y = false;
		bool reduced_x = false;
		while (true)
		{
			tmpxdots = (g_file_x_dots + g_skip_x_dots - 1)/g_skip_x_dots;
			tmpydots = (g_file_y_dots + g_skip_y_dots - 1)/g_skip_y_dots;
			/* reduce further if that improves aspect */
			ftemp = video_mode_aspect_ratio(tmpxdots, tmpydots);
			if (ftemp > g_file_aspect_ratio)
			{
				if (reduced_x)
				{
					break; /* already reduced x, don't reduce y */
				}
				ftemp2 = video_mode_aspect_ratio(tmpxdots, (g_file_y_dots + g_skip_y_dots)/(g_skip_y_dots + 1));
				if (ftemp2 < g_file_aspect_ratio &&
					ftemp/g_file_aspect_ratio *0.9 <= g_file_aspect_ratio/ftemp2)
				{
					break; /* further y reduction is worse */
				}
				++g_skip_y_dots;
				reduced_y = true;
			}
			else
			{
				if (reduced_y)
				{
					break; /* already reduced y, don't reduce x */
				}
				ftemp2 = video_mode_aspect_ratio((g_file_x_dots + g_skip_x_dots)/(g_skip_x_dots + 1), tmpydots);
				if (ftemp2 > g_file_aspect_ratio &&
					g_file_aspect_ratio/ftemp *0.9 <= ftemp2/g_file_aspect_ratio)
				{
					break; /* further x reduction is worse */
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

	g_final_aspect_ratio = g_file_aspect_ratio;
	if (g_final_aspect_ratio == 0) /* assume display correct */
	{
		g_final_aspect_ratio = float(video_mode_aspect_ratio(g_file_x_dots, g_file_y_dots));
	}
	if (g_final_aspect_ratio >= g_screen_aspect_ratio-0.02
		&& g_final_aspect_ratio <= g_screen_aspect_ratio + 0.02)
	{
		g_final_aspect_ratio = g_screen_aspect_ratio;
	}
	int i = int(g_final_aspect_ratio*1000.0 + 0.5);
	g_final_aspect_ratio = float(i/1000.0); /* chop precision to 3 decimals */

	/* setup view window stuff */
	g_view_window = false;
	g_view_x_dots = 0;
	g_view_y_dots = 0;
	if (g_file_x_dots != g_.VideoEntry().x_dots || g_file_y_dots != g_.VideoEntry().y_dots)
	{
		/* image not exactly same size as screen */
		g_view_window = true;
		ftemp = g_final_aspect_ratio*
			double(g_.VideoEntry().y_dots)/double(g_.VideoEntry().x_dots)
				/g_screen_aspect_ratio;
		if (g_final_aspect_ratio <= g_screen_aspect_ratio)
		{
			i = int(double(g_.VideoEntry().x_dots)/double(g_file_x_dots)*20.0 + 0.5);
			tmpreduce = float(i/20.0); /* chop precision to nearest .05 */
			i = int(double(g_.VideoEntry().x_dots)/tmpreduce + 0.5);
			j = int(double(i)*ftemp + 0.5);
		}
		else
		{
			i = int(double(g_.VideoEntry().y_dots)/double(g_file_y_dots)*20.0 + 0.5);
			tmpreduce = float(i/20.0); /* chop precision to nearest .05 */
			j = int(double(g_.VideoEntry().y_dots)/tmpreduce + 0.5);
			i = int(double(j)/ftemp + 0.5);
		}
		if (i != g_file_x_dots || j != g_file_y_dots)  /* too bad, must be explicit */
		{
			g_view_x_dots = g_file_x_dots;
			g_view_y_dots = g_file_y_dots;
		}
		else
		{
			g_view_reduction = tmpreduce; /* ok, this works */
		}
	}
	if (g_make_par_flag && !g_fast_restore && !g_initialize_batch &&
			(fabs(g_final_aspect_ratio - g_screen_aspect_ratio) > .00001 || g_view_x_dots != 0))
	{
		stop_message(STOPMSG_NO_BUZZER,
			"Warning: <V>iew parameters are being set to non-standard values.\n"
			"Remember to reset them when finished with this image!");
	}
	return 0;
}

static void format_item(int choice, char *buf)
{
	char errbuf[10];
	unsigned tmpflags;
	errbuf[0] = 0;
	tmpflags = vidptr[choice].flags;
	if (tmpflags & (VI_VSMALL + VI_CSMALL + VI_ASPECT))
	{
		strcat(errbuf, "*");
	}
	if (tmpflags & VI_VSMALL)
	{
		strcat(errbuf, "R");
	}
	if (tmpflags & VI_CSMALL)
	{
		strcat(errbuf, "C");
	}
	if (tmpflags & VI_ASPECT)
	{
		strcat(errbuf, "A");
	}
	if (tmpflags & VI_VBIG)
	{
		strcat(errbuf, "v");
	}
	if (tmpflags & VI_CBIG)
	{
		strcat(errbuf, "c");
	}
	format_video_info(vidptr[choice].index, errbuf, buf);
}

static int check_mode_key(int curkey, int choice)
{
	int i;
	i = choice; /* avoid warning */
	i = check_video_mode_key(0, curkey);
	return (i >= 0) ? -100-i : 0;
}
