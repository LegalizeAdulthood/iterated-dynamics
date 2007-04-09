/*
	loadfdos.c - subroutine of loadfile.c (read_overlay) which sets
				up video (mode, screen size).
	This module is linked as an overlay, should only be called from loadfile.c

	This code was split to a separate module to isolate the DOS only aspects
	of loading an image.  get_video_mode should return with:
		return code 0 for ok, -1 for error or cancelled by user
		video parameters setup for the mainline, in the dos case this means
		setting g_init_mode to video mode, based on this fractint.c will set up
		for and call setvideomode
		set viewwindow on if file going to be loaded into a view smaller than
		physical screen, in this case also set viewreduction, viewxdots,
		viewydots, and g_final_aspect_ratio
		set skipxdots and skipydots, to 0 if all pixels are to be loaded,
		to 1 for every 2nd pixel, 2 for every 3rd, etc

	In WinFract, at least initially, get_video_mode can do just the
	following:
		set overall image x & y dimensions (sxdots and sydots) to g_file_x_dots
		and g_file_y_dots (note that g_file_colors is the number of g_colors in the
		gif, not sure if that is of any use...)
		if current window smaller than new sxdots and sydots, use scroll bars,
		if larger perhaps reduce the window size? whatever
		set viewwindow to 0 (no need? it always is for now in windows vsn?)
		set g_final_aspect_ratio to .75 (ditto?)
		set skipxdots and skipydots to 0
		return 0

*/

#include <string.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

/* routines in this module      */

#ifndef XFRACT
static int    vidcompare(VOIDCONSTPTR , VOIDCONSTPTR);
static void   format_item(int, char *);
static int    check_modekey(int, int);
static void   format_vid_inf(int i, char *err, char *buf);
#endif
static double vid_aspect(int tryxdots, int tryydots);

struct vidinf
{
	int entnum;     /* g_video_entry subscript */
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
#define VI_CSMALL    8  /* mode has too few g_colors */
#define VI_CBIG      4  /* mode has excess g_colors */
#define VI_ASPECT    1  /* aspect ratio bad */

#ifndef XFRACT
static int vidcompare(VOIDCONSTPTR p1, VOIDCONSTPTR p2)
{
	struct vidinf CONST *ptr1, *ptr2;
	ptr1 = (struct vidinf CONST *)p1;
	ptr2 = (struct vidinf CONST *)p2;
	if (ptr1->flags < ptr2->flags)
	{
		return -1;
	}
	if (ptr1->flags > ptr2->flags)
	{
		return 1;
	}
	if (g_video_table[ptr1->entnum].keynum < g_video_table[ptr2->entnum].keynum)
	{
		return -1;
	}
	if (g_video_table[ptr1->entnum].keynum > g_video_table[ptr2->entnum].keynum)
	{
		return 1;
	}
	if (ptr1->entnum < ptr2->entnum)
	{
		return -1;
	}
	return 1;
}

static void format_vid_inf(int i, char *err, char *buf)
{
	char kname[5];
	memcpy((char *)&g_video_entry, (char *)&g_video_table[i],
				sizeof(g_video_entry));
	vidmode_keyname(g_video_entry.keynum, kname);
	sprintf(buf, "%-5s %-25s %-4s %5d %5d %3d %-25s",  /* 78 chars */
			kname, g_video_entry.name, err,
			g_video_entry.xdots, g_video_entry.ydots,
			g_video_entry.g_colors, g_video_entry.comment);
	g_video_entry.xdots = 0; /* so tab_display knows to display nothing */
}
#endif

static double vid_aspect(int tryxdots, int tryydots)
{  /* calc resulting aspect ratio for specified dots in current mode */
	return (double)tryydots / (double)tryxdots
		*(double)g_video_entry.xdots / (double)g_video_entry.ydots
		*g_screen_aspect_ratio;
	}

#ifndef XFRACT
static struct vidinf *vidptr;
#endif

int get_video_mode(struct fractal_info *info, struct ext_blk_formula_info *formula_info)
{
	struct vidinf vid[MAXVIDEOMODES];
	int i, j;
	int gotrealmode;
	double ftemp, ftemp2;
	unsigned tmpflags;

	int tmpxdots, tmpydots;
	float tmpreduce;
#ifndef XFRACT
	char *nameptr;
	int  *attributes;
	int oldhelpmode;
#endif
	VIDEOINFO *vident;

	g_init_mode = -1;

	/* try to find exact match for vid mode */
	for (i = 0; i < g_video_table_len; ++i)
	{
		vident = &g_video_table[i];
		if (info->xdots == vident->xdots && info->ydots == vident->ydots
			&& g_file_colors == vident->g_colors)
		{
			g_init_mode = i;
			break;
		}
	}

	/* exit in makepar mode if no exact match of video mode in file */
	if (*g_make_par == '\0' && g_init_mode == -1)
	{
		return 0;
	}

	if (g_init_mode == -1) /* try to find very good match for vid mode */
	{
		for (i = 0; i < g_video_table_len; ++i)
		{
			vident = &g_video_table[i];
			if (info->xdots == vident->xdots && info->ydots == vident->ydots
				&& g_file_colors == vident->g_colors)
			{
				g_init_mode = i;
				break;
			}
		}
	}

	/* setup table entry for each vid mode, flagged for how well it matches */
	for (i = 0; i < g_video_table_len; ++i)
	{
		memcpy((char *)&g_video_entry, (char *)&g_video_table[i],
					sizeof(g_video_entry));
		tmpflags = VI_EXACT;
		if (g_video_entry.keynum == 0)
		{
			tmpflags |= VI_NOKEY;
		}
		if (info->xdots > g_video_entry.xdots || info->ydots > g_video_entry.ydots)
		{
			tmpflags |= VI_SSMALL;
		}
		else if (info->xdots < g_video_entry.xdots || info->ydots < g_video_entry.ydots)
		{
			tmpflags |= VI_SBIG;
		}
		if (g_file_x_dots > g_video_entry.xdots || g_file_y_dots > g_video_entry.ydots)
		{
			tmpflags |= VI_VSMALL;
		}
		else if (g_file_x_dots < g_video_entry.xdots || g_file_y_dots < g_video_entry.ydots)
		{
			tmpflags |= VI_VBIG;
		}
		if (g_file_colors > g_video_entry.g_colors)
		{
			tmpflags |= VI_CSMALL;
		}
		if (g_file_colors < g_video_entry.g_colors)
		{
			tmpflags |= VI_CBIG;
		}
		if (i == g_init_mode)
		{
			tmpflags -= VI_EXACT;
		}
		if (g_file_aspect_ratio != 0 && (tmpflags & VI_VSMALL) == 0)
		{
			ftemp = vid_aspect(g_file_x_dots, g_file_y_dots);
			if (ftemp < g_file_aspect_ratio*0.98 ||
				ftemp > g_file_aspect_ratio*1.02)
			{
				tmpflags |= VI_ASPECT;
			}
		}
		vid[i].entnum = i;
		vid[i].flags  = tmpflags;
	}

	if (g_fast_restore  && !g_ask_video)
	{
		g_init_mode = g_adapter;
	}

#ifndef XFRACT
	gotrealmode = 0;
	if ((g_init_mode < 0 || (g_ask_video && !g_initialize_batch)) && *g_make_par != '\0')
	{
		char temp1[256];
		/* no exact match or (askvideo=yes and batch=no), and not
			in makepar mode, talk to user */

		qsort(vid, g_video_table_len, sizeof(vid[0]), vidcompare); /* sort modes */

		attributes = (int *)&g_stack[1000];
		for (i = 0; i < g_video_table_len; ++i)
		{
			attributes[i] = 1;
		}
		vidptr = &vid[0]; /* for format_item */

		/* format heading */
		if (info->info_id[0] == 'G')
		{
			strcpy(temp1, "      Non-fractal GIF");
		}
		else
		{
			nameptr = g_current_fractal_specific->name;
			if (*nameptr == '*')
			{
				++nameptr;
			}
			if (g_display_3d)
			{
				nameptr = "3D Transform";
			}
			if ((!strcmp(nameptr, "formula")) ||
				(!strcmp(nameptr, "lsystem")) ||
				(!strncmp(nameptr, "ifs", 3))) /* for ifs and ifs3d */
			{
				sprintf(temp1, "Type: %s -> %s", nameptr, formula_info->form_name);
			}
			else
			{
				sprintf(temp1, "Type: %s", nameptr);
			}
		}
		sprintf((char *)g_stack, "File: %-44s  %d x %d x %d\n%-52s",
				g_read_name, g_file_x_dots, g_file_y_dots, g_file_colors, temp1);
		if (info->info_id[0] != 'G')
		{
			if (save_system)
			{
				strcat((char *)g_stack, "WinFract ");
			}
			sprintf(temp1, "v%d.%01d", g_save_release/100, (g_save_release%100)/10);
			if (g_save_release % 100)
			{
				i = (int) strlen(temp1);
				temp1[i] = (char)((g_save_release % 10) + '0');
				temp1[i + 1] = 0;
			}
			if (save_system == 0 && g_save_release <= 1410)
			{
				strcat(temp1, " or earlier");
			}
			strcat((char *)g_stack, temp1);
		}
		strcat((char *)g_stack, "\n");
		if (info->info_id[0] != 'G' && save_system == 0)
		{
			if (g_init_mode < 0)
			{
				strcat((char *)g_stack, "Saved in unknown video mode.");
			}
			else
			{
				format_vid_inf(g_init_mode, "", temp1);
				strcat((char *)g_stack, temp1);
			}
		}
		if (g_file_aspect_ratio != 0 && g_file_aspect_ratio != g_screen_aspect_ratio)
		{
			strcat((char *)g_stack,
				"\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings");
		}
		strcat((char *)g_stack, "\n");
		/* set up instructions */
		strcpy(temp1, "Select a video mode.  Use the cursor keypad to move the pointer.\n"
				"Press ENTER for selected mode, or use a video mode function key.\n"
				"Press F1 for help, ");
		if (info->info_id[0] != 'G')
		{
			strcat(temp1, "TAB for fractal information, ");
		}
		strcat(temp1, "ESCAPE to back out.");

		oldhelpmode = helpmode;
		helpmode = HELPLOADFILE;
		i = fullscreen_choice(0, (char *) g_stack,
			"key...name......................err...xdot..ydot.clr.comment..................",
			temp1, g_video_table_len, NULL, attributes,
			1, 13, 78, 0, format_item, NULL, NULL, check_modekey);
		helpmode = oldhelpmode;
		if (i == -1)
		{
			return -1;
		}
		if (i < 0)  /* returned -100 - g_video_table entry number */
		{
			g_init_mode = -100 - i;
			gotrealmode = 1;
		}
		else
		{
			g_init_mode = vid[i].entnum;
		}
	}
#else
	g_init_mode = 0;
	j = g_video_table[0].keynum;
	gotrealmode = 0;
#endif

	if (gotrealmode == 0)  /* translate from temp table to permanent */
	{
		i = g_init_mode;
		j = g_video_table[i].keynum;
		if (j != 0)
		{
			for (g_init_mode = 0; g_init_mode < MAXVIDEOMODES-1; ++g_init_mode)
			{
				if (g_video_table[g_init_mode].keynum == j)
				{
					break;
				}
			}
			if (g_init_mode >= MAXVIDEOMODES-1)
			{
				j = 0;
			}
		}
		if (j == 0) /* mode has no key, add to reserved slot at end */
		{
			memcpy((char *)&g_video_table[g_init_mode = MAXVIDEOMODES-1],
						(char *)&g_video_table[i], sizeof(*g_video_table));
		}
	}

	/* ok, we're going to return with a video mode */
	memcpy((char *)&g_video_entry, (char *)&g_video_table[g_init_mode],
				sizeof(g_video_entry));

	if (viewwindow &&
		g_file_x_dots == g_video_entry.xdots && g_file_y_dots == g_video_entry.ydots)
	{
		/* pull image into a view window */
		if (g_calculation_status != CALCSTAT_COMPLETED) /* if not complete */
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;  /* can't resume anyway */
		}
		if (viewxdots)
		{
			viewreduction = (float) (g_video_entry.xdots / viewxdots);
			viewxdots = viewydots = 0; /* easier to use auto reduction */
		}
		viewreduction = (float)((int)(viewreduction + 0.5)); /* need integer value */
		skipxdots = skipydots = (short)(viewreduction - 1);
		return 0;
	}

	skipxdots = skipydots = 0; /* set for no reduction */
	if (g_video_entry.xdots < g_file_x_dots || g_video_entry.ydots < g_file_y_dots)
	{
		/* set up to load only every nth pixel to make image fit */
		if (g_calculation_status != CALCSTAT_COMPLETED) /* if not complete */
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;  /* can't resume anyway */
		}
		skipxdots = skipydots = 1;
		while (skipxdots*g_video_entry.xdots < g_file_x_dots)
		{
			++skipxdots;
		}
		while (skipydots*g_video_entry.ydots < g_file_y_dots)
		{
			++skipydots;
		}
		i = j = 0;
		while (1)
		{
			tmpxdots = (g_file_x_dots + skipxdots - 1) / skipxdots;
			tmpydots = (g_file_y_dots + skipydots - 1) / skipydots;
			/* reduce further if that improves aspect */
			ftemp = vid_aspect(tmpxdots, tmpydots);
			if (ftemp > g_file_aspect_ratio)
			{
				if (j)
				{
					break; /* already reduced x, don't reduce y */
				}
				ftemp2 = vid_aspect(tmpxdots, (g_file_y_dots + skipydots)/(skipydots + 1));
				if (ftemp2 < g_file_aspect_ratio &&
					ftemp/g_file_aspect_ratio *0.9 <= g_file_aspect_ratio/ftemp2)
				{
					break; /* further y reduction is worse */
				}
				++skipydots;
				++i;
			}
			else
			{
				if (i)
				{
					break; /* already reduced y, don't reduce x */
				}
				ftemp2 = vid_aspect((g_file_x_dots + skipxdots)/(skipxdots + 1), tmpydots);
				if (ftemp2 > g_file_aspect_ratio &&
					g_file_aspect_ratio/ftemp *0.9 <= ftemp2/g_file_aspect_ratio)
				{
					break; /* further x reduction is worse */
				}
				++skipxdots;
				++j;
			}
		}
		g_file_x_dots = tmpxdots;
		g_file_y_dots = tmpydots;
		--skipxdots;
		--skipydots;
	}

	g_final_aspect_ratio = g_file_aspect_ratio;
	if (g_final_aspect_ratio == 0) /* assume display correct */
	{
		g_final_aspect_ratio = (float)vid_aspect(g_file_x_dots, g_file_y_dots);
	}
	if (g_final_aspect_ratio >= g_screen_aspect_ratio-0.02
		&& g_final_aspect_ratio <= g_screen_aspect_ratio + 0.02)
	{
		g_final_aspect_ratio = g_screen_aspect_ratio;
	}
	i = (int)(g_final_aspect_ratio*1000.0 + 0.5);
	g_final_aspect_ratio = (float)(i/1000.0); /* chop precision to 3 decimals */

	/* setup view window stuff */
	viewwindow = viewxdots = viewydots = 0;
	if (g_file_x_dots != g_video_entry.xdots || g_file_y_dots != g_video_entry.ydots)
	{
		/* image not exactly same size as screen */
		viewwindow = 1;
		ftemp = g_final_aspect_ratio*
			(double)g_video_entry.ydots / (double)g_video_entry.xdots
				/ g_screen_aspect_ratio;
		if (g_final_aspect_ratio <= g_screen_aspect_ratio)
		{
			i = (int)((double)g_video_entry.xdots / (double)g_file_x_dots*20.0 + 0.5);
			tmpreduce = (float)(i/20.0); /* chop precision to nearest .05 */
			i = (int)((double)g_video_entry.xdots / tmpreduce + 0.5);
			j = (int)((double)i*ftemp + 0.5);
		}
		else
		{
			i = (int)((double)g_video_entry.ydots / (double)g_file_y_dots*20.0 + 0.5);
			tmpreduce = (float)(i/20.0); /* chop precision to nearest .05 */
			j = (int)((double)g_video_entry.ydots / tmpreduce + 0.5);
			i = (int)((double)j / ftemp + 0.5);
		}
		if (i != g_file_x_dots || j != g_file_y_dots)  /* too bad, must be explicit */
		{
			viewxdots = g_file_x_dots;
			viewydots = g_file_y_dots;
		}
		else
		{
			viewreduction = tmpreduce; /* ok, this works */
		}
	}
	if (*g_make_par && !g_fast_restore && !g_initialize_batch &&
			(fabs(g_final_aspect_ratio - g_screen_aspect_ratio) > .00001 || viewxdots != 0))
	{
		stopmsg(STOPMSG_NO_BUZZER,
			"Warning: <V>iew parameters are being set to non-standard values.\n"
			"Remember to reset them when finished with this image!");
	}
	return 0;
}

#ifndef XFRACT
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
	format_vid_inf(vidptr[choice].entnum, errbuf, buf);
}

static int check_modekey(int curkey, int choice)
{
	int i;
	i = choice; /* avoid warning */
	i = check_vidmode_key(0, curkey);
	return (i >= 0) ? -100-i : 0;
}
#endif
