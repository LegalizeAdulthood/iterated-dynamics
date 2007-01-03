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
        viewydots, and finalaspectratio
      set skipxdots and skipydots, to 0 if all pixels are to be loaded,
        to 1 for every 2nd pixel, 2 for every 3rd, etc

    In WinFract, at least initially, get_video_mode can do just the
    following:
      set overall image x & y dimensions (sxdots and sydots) to filexdots
        and fileydots (note that filecolors is the number of colors in the
        gif, not sure if that is of any use...)
      if current window smaller than new sxdots and sydots, use scroll bars,
        if larger perhaps reduce the window size? whatever
      set viewwindow to 0 (no need? it always is for now in windows vsn?)
      set finalaspectratio to .75 (ditto?)
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
static int    vidcompare(VOIDCONSTPTR ,VOIDCONSTPTR );
static void   format_item(int,char *);
static int    check_modekey(int,int);
static void   format_vid_inf(int i,char *err,char *buf);
#endif
static double vid_aspect(int tryxdots,int tryydots);

struct vidinf {
   int entnum;     /* g_video_entry subscript */
   unsigned flags; /* flags for sort's compare, defined below */
   };
/* defines for flags; done this way instead of bit union to ensure ordering;
   these bits represent the sort sequence for video mode list */
#define VI_EXACT 0x8000 /* unless the one and only exact match */
#define VI_NOKEY   512  /* if no function key assigned */
#define VI_DISK1   256  /* disk video and size not exact */
#define VI_SSMALL  128  /* screen smaller than file's screen */
#define VI_SBIG     64  /* screen bigger than file's screen */
#define VI_VSMALL   32  /* screen smaller than file's view */
#define VI_VBIG     16  /* screen bigger than file's view */
#define VI_CSMALL    8  /* mode has too few colors */
#define VI_CBIG      4  /* mode has excess colors */
#define VI_DISK2     2  /* disk video */
#define VI_ASPECT    1  /* aspect ratio bad */

#ifndef XFRACT
static int vidcompare(VOIDCONSTPTR p1,VOIDCONSTPTR p2)
{
   struct vidinf CONST *ptr1,*ptr2;
   ptr1 = (struct vidinf CONST *)p1;
   ptr2 = (struct vidinf CONST *)p2;
   if (ptr1->flags < ptr2->flags) return(-1);
   if (ptr1->flags > ptr2->flags) return(1);
   if (g_video_table[ptr1->entnum].keynum < g_video_table[ptr2->entnum].keynum) return(-1);
   if (g_video_table[ptr1->entnum].keynum > g_video_table[ptr2->entnum].keynum) return(1);
   if (ptr1->entnum < ptr2->entnum) return(-1);
   return(1);
}

static void format_vid_inf(int i,char *err,char *buf)
{
   char kname[5];
   memcpy((char *)&g_video_entry,(char *)&g_video_table[i],
              sizeof(g_video_entry));
   vidmode_keyname(g_video_entry.keynum,kname);
   sprintf(buf,"%-5s %-25s %-4s %5d %5d %3d %-25s",  /* 78 chars */
           kname, g_video_entry.name, err,
           g_video_entry.xdots, g_video_entry.ydots,
           g_video_entry.colors, g_video_entry.comment);
   g_video_entry.xdots = 0; /* so tab_display knows to display nothing */
}
#endif

static double vid_aspect(int tryxdots,int tryydots)
{  /* calc resulting aspect ratio for specified dots in current mode */
   return (double)tryydots / (double)tryxdots
        * (double)g_video_entry.xdots / (double)g_video_entry.ydots
        * screenaspect;
   }

#ifndef XFRACT
static struct vidinf *vidptr;
#endif

int get_video_mode(struct fractal_info *info,struct ext_blk_3 *blk_3_info)
{
	struct vidinf vid[MAXVIDEOMODES];
	int i, j;
	int gotrealmode;
	double ftemp,ftemp2;
	unsigned tmpflags;

	int tmpxdots,tmpydots;
	float tmpreduce;
#ifndef XFRACT
	char *nameptr;
	int  *attributes;
	int oldhelpmode;
#endif
	VIDEOINFO *vident;

	g_init_mode = -1;

	/* try to change any VESA entries to fit the loaded image size */
	/* TODO: virtual screens? video ram? VESA/DOS be gone! */
	if (g_virtual_screens && g_video_vram && g_init_mode == -1)
	{
		unsigned long vram = (unsigned long)g_video_vram << 16,
						need = (unsigned long)info->xdots * info->ydots;
		if (need <= vram)
		{
			char over[25]; /* overwrite comments with original resolutions */
			int bppx;      /* bytesperpixel multiplier */
			for (i = 0; i < g_video_table_len; ++i)
			{
				vident = &g_video_table[i];
				if (vident->dotmode%100 == DOTMODE_VESA && vident->colors >= 256
				&& (info->xdots > vident->xdots || info->ydots > vident->ydots)
				&& vram >= (unsigned long)
					(info->xdots < vident->xdots ? vident->xdots : info->xdots)
					* (info->ydots < vident->ydots ? vident->ydots : info->ydots)
					* ((bppx = vident->dotmode/1000) < 2 ? ++bppx : bppx))
				{
					_ASSERTE(FALSE && "Trying to set video mode table from command-file?");
					sprintf(over, "<-VIRTUAL! at %4u x %4u",vident->xdots,vident->ydots);
					strcpy((char *)vident->comment, (char *)over);

					if (info->xdots > vident->xdots)
					{
						vident->xdots = info->xdots;
					}
					if (info->ydots > vident->ydots)
					{
						vident->ydots = info->ydots;
					}
				}  /* change entry to force VESA virtual scanline setup */
			}
		}
	}

	/* try to find exact match for vid mode */
	for (i = 0; i < g_video_table_len; ++i)
	{
		vident = &g_video_table[i];
		if (info->xdots == vident->xdots && info->ydots == vident->ydots
			&& filecolors == vident->colors
			&& info->videomodeax == vident->videomodeax
			&& info->videomodebx == vident->videomodebx
			&& info->videomodecx == vident->videomodecx
			&& info->videomodedx == vident->videomodedx
			&& info->dotmode%100 == vident->dotmode%100)
		{
			g_init_mode = i;
			break;
        }
    }

	/* exit in makepar mode if no exact match of video mode in file */
	if (*s_makepar == '\0' && g_init_mode == -1)
		return 0;

	if (g_init_mode == -1) /* try to find very good match for vid mode */
	{
		for (i = 0; i < g_video_table_len; ++i)
		{
			vident = &g_video_table[i];
			if (info->xdots == vident->xdots && info->ydots == vident->ydots
				&& filecolors == vident->colors)
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
		if (filexdots > g_video_entry.xdots || fileydots > g_video_entry.ydots)
		{
			tmpflags |= VI_VSMALL;
		}
		else if (filexdots < g_video_entry.xdots || fileydots < g_video_entry.ydots)
		{
			tmpflags |= VI_VBIG;
		}
		if (filecolors > g_video_entry.colors)
		{
			tmpflags |= VI_CSMALL;
		}
		if (filecolors < g_video_entry.colors)
		{
			tmpflags |= VI_CBIG;
		}
		if (i == g_init_mode)
		{
			tmpflags -= VI_EXACT;
		}
		if (g_video_entry.dotmode%100 == DOTMODE_RAMDISK)
		{
			tmpflags |= VI_DISK2;
			if ((tmpflags & (VI_SBIG+VI_SSMALL+VI_VBIG+VI_VSMALL)) != 0)
			{
				tmpflags |= VI_DISK1;
			}
		}
		if (fileaspectratio != 0 && g_video_entry.dotmode%100 != DOTMODE_RAMDISK
			&& (tmpflags & VI_VSMALL) == 0)
		{
			ftemp = vid_aspect(filexdots, fileydots);
			if (ftemp < fileaspectratio * 0.98 ||
				ftemp > fileaspectratio * 1.02)
			{
				tmpflags |= VI_ASPECT;
			}
        }
		vid[i].entnum = i;
		vid[i].flags  = tmpflags;
    }

	if (fastrestore  && !askvideo)
	{
		g_init_mode = g_adapter;
	}

#ifndef XFRACT
	gotrealmode = 0;
	if ((g_init_mode < 0 || (askvideo && !initbatch)) && *s_makepar != '\0')
	{
		/* no exact match or (askvideo=yes and batch=no), and not
			in makepar mode, talk to user */

		qsort(vid, g_video_table_len, sizeof(vid[0]), vidcompare); /* sort modes */

		attributes = (int *)&dstack[1000];
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
			nameptr = curfractalspecific->name;
			if (*nameptr == '*')
			{
				++nameptr;
			}
			if (display3d)
			{
				nameptr = "3D Transform";
			}
			if ((!strcmp(nameptr, "formula")) ||
				(!strcmp(nameptr, "lsystem")) ||
				(!strncmp(nameptr, "ifs", 3))) /* for ifs and ifs3d */
			{
				sprintf(temp1, "Type: %s -> %s", nameptr, blk_3_info->form_name);
			}
			else
			{
				sprintf(temp1, "Type: %s", nameptr);
			}
		}
		sprintf((char *)dstack, "File: %-44s  %d x %d x %d\n%-52s",
				readname, filexdots, fileydots, filecolors, temp1);
		if (info->info_id[0] != 'G')
		{
			if (save_system)
			{
				strcat((char *)dstack, "WinFract ");
			}
			sprintf(temp1, "v%d.%01d", save_release/100, (save_release%100)/10);
			if (save_release%100)
			{
				i = (int) strlen(temp1);
				temp1[i] = (char)((save_release%10) + '0');
				temp1[i+1] = 0;
			}
			if (save_system == 0 && save_release <= 1410)
			{
				strcat(temp1, " or earlier");
			}
			strcat((char *)dstack, temp1);
        }
		strcat((char *)dstack, "\n");
		if (info->info_id[0] != 'G' && save_system == 0)
		{
			if (g_init_mode < 0)
			{
				strcat((char *)dstack, "Saved in unknown video mode.");
			}
			else
			{
				format_vid_inf(g_init_mode, "", temp1);
				strcat((char *)dstack, temp1);
			}
		}
		if (fileaspectratio != 0 && fileaspectratio != screenaspect)
		{
			strcat((char *)dstack,
				"\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings");
		}
		strcat((char *)dstack, "\n");
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
		i = fullscreen_choice(0, (char *) dstack,
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
		if ((j = g_video_table[i=g_init_mode].keynum) != 0)
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
			memcpy((char *)&g_video_table[g_init_mode=MAXVIDEOMODES-1],
						(char *)&g_video_table[i], sizeof(*g_video_table));
		}
    }

	/* ok, we're going to return with a video mode */
	memcpy((char *)&g_video_entry, (char *)&g_video_table[g_init_mode],
				sizeof(g_video_entry));

	if (viewwindow &&
		filexdots == g_video_entry.xdots && fileydots == g_video_entry.ydots)
	{
		/* pull image into a view window */
		if (calc_status != 4) /* if not complete */
		{
			calc_status = 0;  /* can't resume anyway */
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
	if (g_video_entry.xdots < filexdots || g_video_entry.ydots < fileydots)
	{
		/* set up to load only every nth pixel to make image fit */
		if (calc_status != 4) /* if not complete */
		{
			calc_status = 0;  /* can't resume anyway */
		}
		skipxdots = skipydots = 1;
		while (skipxdots * g_video_entry.xdots < filexdots)
		{
			++skipxdots;
		}
		while (skipydots * g_video_entry.ydots < fileydots)
		{
			++skipydots;
		}
		i = j = 0;
		for (;;)
		{
			tmpxdots = (filexdots + skipxdots - 1) / skipxdots;
			tmpydots = (fileydots + skipydots - 1) / skipydots;
			if (fileaspectratio == 0 || g_video_entry.dotmode%100 == DOTMODE_RAMDISK)
			{
				break;
			}
			/* reduce further if that improves aspect */
			if ((ftemp = vid_aspect(tmpxdots, tmpydots)) > fileaspectratio)
			{
				if (j)
				{
					break; /* already reduced x, don't reduce y */
				}
				ftemp2 = vid_aspect(tmpxdots, (fileydots+skipydots)/(skipydots+1));
				if (ftemp2 < fileaspectratio &&
					ftemp/fileaspectratio *0.9 <= fileaspectratio/ftemp2)
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
				ftemp2 = vid_aspect((filexdots+skipxdots)/(skipxdots+1), tmpydots);
				if (ftemp2 > fileaspectratio &&
					fileaspectratio/ftemp *0.9 <= ftemp2/fileaspectratio)
				{
					break; /* further x reduction is worse */
				}
				++skipxdots;
				++j;
			}
        }
		filexdots = tmpxdots;
		fileydots = tmpydots;
		--skipxdots;
		--skipydots;
    }

	if ((finalaspectratio = fileaspectratio) == 0) /* assume display correct */
	{
		finalaspectratio = (float)vid_aspect(filexdots, fileydots);
	}
	if (finalaspectratio >= screenaspect-0.02
		&& finalaspectratio <= screenaspect+0.02)
	{
		finalaspectratio = screenaspect;
	}
	i = (int)(finalaspectratio * 1000.0 + 0.5);
	finalaspectratio = (float)(i/1000.0); /* chop precision to 3 decimals */

	/* setup view window stuff */
	viewwindow = viewxdots = viewydots = 0;
	if (filexdots != g_video_entry.xdots || fileydots != g_video_entry.ydots)
	{
		/* image not exactly same size as screen */
		viewwindow = 1;
		ftemp = finalaspectratio
				* (double)g_video_entry.ydots / (double)g_video_entry.xdots
				/ screenaspect;
		if (finalaspectratio <= screenaspect)
		{
			i = (int)((double)g_video_entry.xdots / (double)filexdots * 20.0 + 0.5);
			tmpreduce = (float)(i/20.0); /* chop precision to nearest .05 */
			i = (int)((double)g_video_entry.xdots / tmpreduce + 0.5);
			j = (int)((double)i * ftemp + 0.5);
        }
		else
		{
			i = (int)((double)g_video_entry.ydots / (double)fileydots * 20.0 + 0.5);
			tmpreduce = (float)(i/20.0); /* chop precision to nearest .05 */
			j = (int)((double)g_video_entry.ydots / tmpreduce + 0.5);
			i = (int)((double)j / ftemp + 0.5);
        }
		if (i != filexdots || j != fileydots)  /* too bad, must be explicit */
		{
			viewxdots = filexdots;
			viewydots = fileydots;
        }
		else
		{
			viewreduction = tmpreduce; /* ok, this works */
		}
	}
	if (*s_makepar && !fastrestore && !initbatch &&
			(fabs(finalaspectratio - screenaspect) > .00001 || viewxdots != 0))
	{
		stopmsg(STOPMSG_NO_BUZZER,
			"Warning: <V>iew parameters are being set to non-standard values.\n"
			"Remember to reset them when finished with this image!");
    }
	return 0;
}

#ifndef XFRACT
static void format_item(int choice,char *buf)
{
   char errbuf[10];
   unsigned tmpflags;
   errbuf[0] = 0;
   tmpflags = vidptr[choice].flags;
   if (tmpflags & (VI_VSMALL+VI_CSMALL+VI_ASPECT)) strcat(errbuf,"*");
   if (tmpflags & VI_VSMALL) strcat(errbuf,"R");
   if (tmpflags & VI_CSMALL) strcat(errbuf,"C");
   if (tmpflags & VI_ASPECT) strcat(errbuf,"A");
   if (tmpflags & VI_VBIG)   strcat(errbuf,"v");
   if (tmpflags & VI_CBIG)   strcat(errbuf,"c");
   format_vid_inf(vidptr[choice].entnum,errbuf,buf);
}

static int check_modekey(int curkey,int choice)
{
   int i;
   i=choice; /* avoid warning */
   return (((i = check_vidmode_key(0,curkey)) >= 0) ? -100-i : 0);
}
#endif
