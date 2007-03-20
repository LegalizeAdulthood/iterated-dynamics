#include <string.h>
#include <time.h>

#ifndef XFRACT
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <ctype.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

#if 0
/* makes a handly list of jul-man pairs, not for release */
static void julman()
{
   FILE *fp;
   int i;
   fp = dir_fopen(workdir,"toggle.txt","w");
   i = -1;
   while (fractalspecific[++i].name)
   {
      if (fractalspecific[i].tojulia != NOFRACTAL && fractalspecific[i].name[0] != '*')
         fprintf(fp,"%s  %s\n",fractalspecific[i].name,
             fractalspecific[fractalspecific[i].tojulia].name);
   }
   fclose(fp);
}
#endif

/* routines in this module      */

int main_menu_switch(int*,int*,int*,char*,int);
int evolver_menu_switch(int*,int*,int*,char*);
int big_while_loop(int *kbdmore, char *stacked, int resumeflag);
static void move_zoombox(int);
char fromtext_flag = 0;         /* = 1 if we're in graphics mode */
static int call_line3d(BYTE *pixels, int linelen);
static  void note_zoom(void);
static  void restore_zoom(void);
static  void move_zoombox(int keynum);
static  void cmp_line_cleanup(void);

void *evolve_handle = NULL;
char old_stdcalcmode;
static char *savezoom;
void (*outln_cleanup) (void);

int big_while_loop(int *kbdmore, char *stacked, int resumeflag)
{
   int     frommandel;                  /* if julia entered from mandel */
   int     axmode = 0; /* video mode (BIOS ##)    */
   double  ftemp;                       /* fp temp                      */
   int     i = 0;                           /* temporary loop counters      */
   int kbdchar;
   int mms_value;

#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif
   frommandel = 0;
   if (resumeflag)
      goto resumeloop;

   while (1)                    /* eternal loop */
   {
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		if (calc_status != CALCSTAT_RESUMABLE || showfile == 0)
		{
			memcpy((char *)&g_video_entry, (char *)&g_video_table[g_adapter],
					sizeof(g_video_entry));
			axmode  = g_video_entry.videomodeax; /* video mode (BIOS call)   */
			dotmode = g_video_entry.dotmode;     /* assembler dot read/write */
			xdots   = g_video_entry.xdots;       /* # dots across the screen */
			ydots   = g_video_entry.ydots;       /* # dots down the screen   */
			colors  = g_video_entry.colors;      /* # colors available */
			dotmode  %= 100;
			sxdots  = xdots;
			sydots  = ydots;
			sxoffs = syoffs = 0;
			rotate_hi = (rotate_hi < colors) ? rotate_hi : colors - 1;

			memcpy(olddacbox, g_dac_box, 256*3); /* save the DAC */

			if (overlay3d && !initbatch)
			{
				driver_unstack_screen();            /* restore old graphics image */
				overlay3d = 0;
			}
			else
			{
				driver_set_video_mode(&g_video_entry); /* switch video modes */
				/* switching video modes may have changed drivers or disk flag... */
				if (g_good_mode == 0)
				{
					if (driver_diskp())
					{
						askvideo = TRUE;
					}
					else
					{
						stopmsg(0, "That video mode is not available with your adapter.");
						askvideo = TRUE;
					}
					g_init_mode = -1;
					driver_set_for_text(); /* switch to text mode */
					/* goto restorestart; */
					return RESTORESTART;
				}

				xdots = sxdots;
				ydots = sydots;
				g_video_entry.xdots = xdots;
				g_video_entry.ydots = ydots;
			}

			if (savedac || colorpreloaded)
			{
				memcpy(g_dac_box, olddacbox, 256*3); /* restore the DAC */
				spindac(0, 1);
				colorpreloaded = 0;
			}
			else
			{	/* reset DAC to defaults, which setvideomode has done for us */
				if (mapdacbox)
				{	/* but there's a map=, so load that */
					memcpy((char *)g_dac_box, mapdacbox, 768);
					spindac(0, 1);
				}
				else if ((driver_diskp() && colors == 256) || !colors)
				{
					/* disk video, setvideomode via bios didn't get it right, so: */
#if !defined(XFRACT) && !defined(_WIN32)
					ValidateLuts("default"); /* read the default palette file */
#endif
				}
				colorstate = 0;
            }
			if (viewwindow)
			{
				/* bypass for VESA virtual screen */
				ftemp = finalaspectratio*(((double) sydots)/((double) sxdots)/screenaspect);
				if ((xdots = viewxdots) != 0)
				{	/* xdots specified */
					ydots = viewydots;
					if (ydots == 0) /* calc ydots? */
					{
						ydots = (int)((double)xdots * ftemp + 0.5);
					}
				}
				else if (finalaspectratio <= screenaspect)
				{
					xdots = (int)((double)sxdots / viewreduction + 0.5);
					ydots = (int)((double)xdots * ftemp + 0.5);
				}
				else
				{
					ydots = (int)((double)sydots / viewreduction + 0.5);
					xdots = (int)((double)ydots / ftemp + 0.5);
				}
				if (xdots > sxdots || ydots > sydots)
				{
					stopmsg(0, "View window too large; using full screen.");
					viewwindow = 0;
					xdots = viewxdots = sxdots;
					ydots = viewydots = sydots;
				}
				else if (((xdots <= 1) /* changed test to 1, so a 2x2 window will */
					|| (ydots <= 1)) /* work with the sound feature */
					&& !(evolving&1))
				{	/* so ssg works */
					/* but no check if in evolve mode to allow lots of small views*/
					stopmsg(0, "View window too small; using full screen.");
					viewwindow = 0;
					xdots = sxdots;
					ydots = sydots;
				}
				if ((evolving & 1) && (curfractalspecific->flags & INFCALC))
				{
					stopmsg(0, "Fractal doesn't terminate! switching off evolution.");
					evolving = evolving -1;
					viewwindow = FALSE;
					xdots = sxdots;
					ydots = sydots;
				}
				if (evolving & 1)
				{
					xdots = (sxdots / gridsz)-!((evolving & NOGROUT)/NOGROUT);
					xdots = xdots - (xdots % 4); /* trim to multiple of 4 for SSG */
					ydots = (sydots / gridsz)-!((evolving & NOGROUT)/NOGROUT);
					ydots = ydots - (ydots % 4);
				}
				else
				{
					sxoffs = (sxdots - xdots) / 2;
					syoffs = (sydots - ydots) / 3;
				}
			}
			dxsize = xdots - 1;            /* convert just once now */
			dysize = ydots - 1;
		}
		/* assume we save next time (except jb) */
		savedac = (savedac == 0) ? 2 : 1;
		if (initbatch == INIT_BATCH_NONE)
		{
			lookatmouse = -FIK_PAGE_UP;        /* mouse left button == pgup */
		}

		if (showfile == 0)
		{               /* loading an image */
			outln_cleanup = NULL;          /* outln routine can set this */
			if (display3d)                 /* set up 3D decoding */
			{
				outln = call_line3d;
			}
			else if (filetype >= 1)         /* old .tga format input file */
			{
				outln = outlin16;
			}
			else if (comparegif)            /* debug 50 */
			{
				outln = cmp_line;
			}
			else if (pot16bit)
			{            /* .pot format input file */
				if (pot_startdisk() < 0)
				{                           /* pot file failed?  */
					showfile = 1;
					potflag  = 0;
					pot16bit = 0;
					g_init_mode = -1;
					calc_status = CALCSTAT_RESUMABLE;         /* "resume" without 16-bit */
					driver_set_for_text();
					get_fracttype();
					/* goto imagestart; */
					return IMAGESTART;
				}
				outln = pot_line;
			}
			else if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !evolving) /* regular gif/fra input file */
			{
				outln = sound_line;      /* sound decoding */
			}
			else
			{
				outln = out_line;        /* regular decoding */
			}
			if (filetype == 0)
			{
				if (debugflag == 2224)
				{
					char msg[MSGLEN];
					sprintf(msg, "floatflag=%d", usr_floatflag);
					stopmsg(STOPMSG_NO_BUZZER, (char *)msg);
				}
				i = funny_glasses_call(gifview);
			}
			else
			{
				i = funny_glasses_call(tgaview);
			}
			if (outln_cleanup)              /* cleanup routine defined? */
			{
				(*outln_cleanup)();
			}
			if (i == 0)
			{
				driver_buzzer(BUZZER_COMPLETE);
			}
			else
			{
				calc_status = CALCSTAT_NO_FRACTAL;
				if (driver_key_pressed())
				{
					driver_buzzer(BUZZER_INTERRUPT);
					while (driver_key_pressed()) driver_get_key();
					texttempmsg("*** load incomplete ***");
				}
			}
        }

		zoomoff = TRUE;                      /* zooming is enabled */
		if (driver_diskp() || (curfractalspecific->flags&NOZOOM) != 0)
		{
			zoomoff = FALSE;                   /* for these cases disable zooming */
		}
		if (!evolving)
		{
			calcfracinit();
		}
		driver_schedule_alarm(1);

		sxmin = xxmin; /* save 3 corners for zoom.c ref points */
		sxmax = xxmax;
		sx3rd = xx3rd;
		symin = yymin;
		symax = yymax;
		sy3rd = yy3rd;

		if (bf_math)
		{
			copy_bf(bfsxmin, bfxmin);
			copy_bf(bfsxmax, bfxmax);
			copy_bf(bfsymin, bfymin);
			copy_bf(bfsymax, bfymax);
			copy_bf(bfsx3rd, bfx3rd);
			copy_bf(bfsy3rd, bfy3rd);
		}
		save_history_info();

		if (showfile == 0)
		{               /* image has been loaded */
			showfile = 1;
			if (initbatch == INIT_BATCH_NORMAL && calc_status == CALCSTAT_RESUMABLE)
			{
				initbatch = INIT_BATCH_FINISH_CALC; /* flag to finish calc before save */
			}
			if (loaded3d)      /* 'r' of image created with '3' */
			{
				display3d = 1;  /* so set flag for 'b' command */
			}
		}
		else
		{                            /* draw an image */
			if (initsavetime != 0          /* autosave and resumable? */
					&& (curfractalspecific->flags&NORESUME) == 0)
			{
				savebase = readticker(); /* calc's start time */
				saveticks = initsavetime*60*1000; /* in milliseconds */
				finishrow = -1;
            }
			browsing = FALSE;      /* regenerate image, turn off browsing */
			/*rb*/
			name_stack_ptr = -1;   /* reset pointer */
			browsename[0] = '\0';  /* null */
			if (viewwindow && (evolving&1) && (calc_status != CALCSTAT_COMPLETED))
			{
				/* generate a set of images with varied parameters on each one */
				int grout, ecount, tmpxdots, tmpydots, gridsqr;
				struct evolution_info resume_e_info;

				if ((evolve_handle != NULL) && (calc_status == CALCSTAT_RESUMABLE))
				{
					memcpy(&resume_e_info, evolve_handle, sizeof(resume_e_info));
					paramrangex  = resume_e_info.paramrangex;
					paramrangey  = resume_e_info.paramrangey;
					opx = newopx = resume_e_info.opx;
					opy = newopy = resume_e_info.opy;
					odpx = newodpx = (char)resume_e_info.odpx;
					odpy = newodpy = (char)resume_e_info.odpy;
					px           = resume_e_info.px;
					py           = resume_e_info.py;
					sxoffs       = resume_e_info.sxoffs;
					syoffs       = resume_e_info.syoffs;
					xdots        = resume_e_info.xdots;
					ydots        = resume_e_info.ydots;
					gridsz       = resume_e_info.gridsz;
					this_gen_rseed = resume_e_info.this_gen_rseed;
					fiddlefactor   = resume_e_info.fiddlefactor;
					evolving     = viewwindow = resume_e_info.evolving;
					ecount       = resume_e_info.ecount;
					free(evolve_handle);  /* We're done with it, release it. */
					evolve_handle = NULL;
				}
				else
				{ /* not resuming, start from the beginning */
					int mid = gridsz / 2;
					if ((px != mid) || (py != mid))
					{
						this_gen_rseed = (unsigned int)clock_ticks(); /* time for new set */
					}
					param_history(0); /* save old history */
					ecount = 0;
					fiddlefactor = fiddlefactor * fiddle_reduction;
					opx = newopx; opy = newopy;
					odpx = newodpx; odpy = newodpy; /*odpx used for discrete parms like
														inside, outside, trigfn etc */
				}
				prmboxcount = 0;
				dpx = paramrangex/(gridsz-1);
				dpy = paramrangey/(gridsz-1);
				grout  = !((evolving & NOGROUT)/NOGROUT);
				tmpxdots = xdots+grout;
				tmpydots = ydots+grout;
				gridsqr = gridsz * gridsz;
				while (ecount < gridsqr)
				{
					spiralmap(ecount); /* sets px & py */
					sxoffs = tmpxdots * px;
					syoffs = tmpydots * py;
					param_history(1); /* restore old history */
					fiddleparms(g_genes, ecount);
					calcfracinit();
					if (calcfract() == -1)
					{
						goto done;
					}
					ecount ++;
				}
done:
#if defined(_WIN32)
				_ASSERTE(_CrtCheckMemory());
#endif

				if (ecount == gridsqr)
				{
					i = 0;
					driver_buzzer(BUZZER_COMPLETE); /* finished!! */
				}
				else
				{	/* interrupted screen generation, save info */
					if (evolve_handle == NULL)
					{
						evolve_handle = malloc(sizeof(resume_e_info));
					}
					resume_e_info.paramrangex     = paramrangex;
					resume_e_info.paramrangey     = paramrangey;
					resume_e_info.opx             = opx;
					resume_e_info.opy             = opy;
					resume_e_info.odpx            = (short)odpx;
					resume_e_info.odpy            = (short)odpy;
					resume_e_info.px              = (short)px;
					resume_e_info.py              = (short)py;
					resume_e_info.sxoffs          = (short)sxoffs;
					resume_e_info.syoffs          = (short)syoffs;
					resume_e_info.xdots           = (short)xdots;
					resume_e_info.ydots           = (short)ydots;
					resume_e_info.gridsz          = (short)gridsz;
					resume_e_info.this_gen_rseed  = (short)this_gen_rseed;
					resume_e_info.fiddlefactor    = fiddlefactor;
					resume_e_info.evolving        = (short)evolving;
					resume_e_info.ecount          = (short) ecount;
					memcpy(evolve_handle, &resume_e_info, sizeof(resume_e_info));
				}
				sxoffs = syoffs = 0;
				xdots = sxdots;
				ydots = sydots; /* otherwise save only saves a sub image and boxes get clipped */

				/* set up for 1st selected image, this reuses px and py */
				px = py = gridsz/2;
				unspiralmap(); /* first time called, w/above line sets up array */
				param_history(1); /* restore old history */
				fiddleparms(g_genes, 0);
			}
			/* end of evolution loop */
			else
			{
				i = calcfract();       /* draw the fractal using "C" */
				if (i == 0)
				{
					driver_buzzer(BUZZER_COMPLETE); /* finished!! */
				}
			}

			saveticks = 0;                 /* turn off autosave timer */
			if (driver_diskp() && i == 0) /* disk-video */
			{
				dvid_status(0, "Image has been completed");
			}
		}
#ifndef XFRACT
		boxcount = 0;                     /* no zoom box yet  */
		zwidth = 0;
#else
		if (!XZoomWaiting)
		{
			boxcount = 0;                 /* no zoom box yet  */
			zwidth = 0;
		}
#endif

		if (fractype == PLASMA && cpu > 88)
		{
			cyclelimit = 256;              /* plasma clouds need quick spins */
			g_dac_count = 256;
			g_dac_learn = 1;
         }

resumeloop:                             /* return here on failed overlays */
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		*kbdmore = 1;
		while (*kbdmore == 1)
		{           /* loop through command keys */
			if (timedsave != 0)
			{
				if (timedsave == 1)
				{       /* woke up for timed save */
					driver_get_key();     /* eat the dummy char */
					kbdchar = 's'; /* do the save */
					resave_flag = RESAVE_YES;
					timedsave = 2;
				}
				else
				{                      /* save done, resume */
					timedsave = 0;
					resave_flag = RESAVE_DONE;
					kbdchar = FIK_ENTER;
				}
			}
			else if (initbatch == INIT_BATCH_NONE)      /* not batch mode */
			{
#ifndef XFRACT
				lookatmouse = (zwidth == 0 && !g_video_scroll) ? -FIK_PAGE_UP : LOOK_MOUSE_ZOOM_BOX;
#else
				lookatmouse = (zwidth == 0) ? -FIK_PAGE_UP : LOOK_MOUSE_ZOOM_BOX;
#endif
				if (calc_status == CALCSTAT_RESUMABLE && zwidth == 0 && !driver_key_pressed())
				{
					kbdchar = FIK_ENTER ;  /* no visible reason to stop, continue */
				}
				else      /* wait for a real keystroke */
				{
					if (autobrowse && !no_sub_images)
					{
						kbdchar = 'l';
					}
					else
					{
						driver_wait_key_pressed(0);
						kbdchar = driver_get_key();
					}
					if (kbdchar == FIK_ESC || kbdchar == 'm' || kbdchar == 'M')
					{
						if (kbdchar == FIK_ESC && escape_exit != 0)
						{
							/* don't ask, just get out */
							goodbye();
						}
						driver_stack_screen();
#ifndef XFRACT
						kbdchar = main_menu(1);
#else
						if (XZoomWaiting)
						{
							kbdchar = FIK_ENTER;
						}
						else
						{
							kbdchar = main_menu(1);
							if (XZoomWaiting)
							{
								kbdchar = FIK_ENTER;
							}
						}
#endif
						if (kbdchar == '\\' || kbdchar == FIK_CTL_BACKSLASH ||
							kbdchar == 'h' || kbdchar == 8 ||
							check_vidmode_key(0, kbdchar) >= 0)
						{
							driver_discard_screen();
						}
						else if (kbdchar == 'x' || kbdchar == 'y' ||
								kbdchar == 'z' || kbdchar == 'g' ||
								kbdchar == 'v' || kbdchar == 2 ||
								kbdchar == 5 || kbdchar == 6)
						{
							fromtext_flag = 1;
						}
						else
						{
							driver_unstack_screen();
						}
					}
				}
			}
			else          /* batch mode, fake next keystroke */
			{
				/* initbatch == -1  flag to finish calc before save */
				/* initbatch == 0   not in batch mode */
				/* initbatch == 1   normal batch mode */
				/* initbatch == 2   was 1, now do a save */
				/* initbatch == 3   bailout with errorlevel == 2, error occurred, no save */
				/* initbatch == 4   bailout with errorlevel == 1, interrupted, try to save */
				/* initbatch == 5   was 4, now do a save */

				if (initbatch == INIT_BATCH_FINISH_CALC)       /* finish calc */
				{
					kbdchar = FIK_ENTER;
					initbatch = INIT_BATCH_NORMAL;
				}
				else if (initbatch == INIT_BATCH_NORMAL || initbatch == INIT_BATCH_BAILOUT_INTERRUPTED) /* save-to-disk */
				{
/*
					while (driver_key_pressed())
						driver_get_key();
*/
					kbdchar = (debugflag == 50) ? 'r' : 's';
					if (initbatch == INIT_BATCH_NORMAL)
					{
						initbatch = INIT_BATCH_SAVE;
					}
					if (initbatch == INIT_BATCH_BAILOUT_INTERRUPTED)
					{
						initbatch = INIT_BATCH_BAILOUT_SAVE;
					}
				}
				else
				{
					if (calc_status != CALCSTAT_COMPLETED)
					{
						initbatch = INIT_BATCH_BAILOUT_ERROR; /* bailout with error */
					}
					goodbye();               /* done, exit */
				}
			}

#ifndef XFRACT
			if ('A' <= kbdchar && kbdchar <= 'Z')
			{
				kbdchar = tolower(kbdchar);
			}
#endif
			if (evolving)
			{
				mms_value = evolver_menu_switch(&kbdchar, &frommandel, kbdmore, stacked);
			}
			else
			{
				mms_value = main_menu_switch(&kbdchar, &frommandel, kbdmore, stacked, axmode);
			}
			if (quick_calc && (mms_value == IMAGESTART ||
                            mms_value == RESTORESTART ||
                            mms_value == RESTART))
			{
				quick_calc = 0;
				usr_stdcalcmode = old_stdcalcmode;
			}
			if (quick_calc && calc_status != CALCSTAT_COMPLETED)
			{
				usr_stdcalcmode = '1';
			}
			switch (mms_value)
			{
			case IMAGESTART:	return IMAGESTART;
			case RESTORESTART:	return RESTORESTART;
			case RESTART:		return RESTART;
			case CONTINUE:		continue;
			default:			break;
			}
			if (zoomoff == TRUE && *kbdmore == 1) /* draw/clear a zoom box? */
			{
				drawbox(1);
			}
			if (driver_resize())
			{
				calc_status = CALCSTAT_NO_FRACTAL;
			}
		}
	}
}

static int look(char *stacked)
{
    int oldhelpmode;
    oldhelpmode = helpmode;
    helpmode = HELPBROWSE;
    switch (fgetwindow())
    {
    case FIK_ENTER:
    case FIK_ENTER_2:
        showfile = 0;       /* trigger load */
        browsing = TRUE;    /* but don't ask for the file name as it's
                                * just been selected */
        if (name_stack_ptr == 15)
        {                   /* about to run off the end of the file
                                * history stack so shift it all back one to
                                * make room, lose the 1st one */
            int tmp;
            for (tmp = 1; tmp < 16; tmp++)
			{
                strcpy(file_name_stack[tmp - 1], file_name_stack[tmp]);
			}
            name_stack_ptr = 14;
        }
        name_stack_ptr++;
        strcpy(file_name_stack[name_stack_ptr], browsename);
        /*
        splitpath(browsename, NULL, NULL, fname, ext);
        splitpath(readname, drive, dir, NULL, NULL);
        makepath(readname, drive, dir, fname, ext);
        */
        merge_pathnames(readname, browsename, 2);
        if (askvideo)
        {
            driver_stack_screen();   /* save graphics image */
            *stacked = 1;
        }
        return 1;       /* hop off and do it!! */

	case '\\':
        if (name_stack_ptr >= 1)
        {
            /* go back one file if somewhere to go (ie. browsing) */
            name_stack_ptr--;
            while (file_name_stack[name_stack_ptr][0] == '\0' 
                    && name_stack_ptr >= 0)
			{
                name_stack_ptr--;
			}
            if (name_stack_ptr < 0) /* oops, must have deleted first one */
			{
                break;
			}
            strcpy(browsename, file_name_stack[name_stack_ptr]);
            merge_pathnames(readname,browsename,2);
            browsing = TRUE;
            showfile = 0;
            if (askvideo)
            {
                driver_stack_screen();/* save graphics image */
                *stacked = 1;
            }
            return 1;
        }                   /* otherwise fall through and turn off
                             * browsing */
	case FIK_ESC:
	case 'l':              /* turn it off */
	case 'L':
		browsing = FALSE;
		helpmode = oldhelpmode;
		break;

	case 's':
		browsing = FALSE;
		helpmode = oldhelpmode;
		savetodisk(savename);
		break;

	default:               /* or no files found, leave the state of browsing alone */
		break;
	}

	return 0;
}

static int handle_fractal_type(int *frommandel)
{
	int i;

	julibrot = 0;
	clear_zoombox();
	driver_stack_screen();
	i = get_fracttype();
	if (i >= 0)
	{
		driver_discard_screen();
		savedac = 0;
		save_release = g_release;
		no_mag_calc = 0;
		use_old_period = 0;
		bad_outside = 0;
		ldcheck = 0;
		set_current_params();
		odpx = odpy = newodpx = newodpy = 0;
		fiddlefactor = 1;           /* reset param evolution stuff */
		set_orbit_corners = 0;
		param_history(0); /* save history */
		if (i == 0)
		{
			g_init_mode = g_adapter;
			*frommandel = 0;
		}
		else if (g_init_mode < 0) /* it is supposed to be... */
		{
			driver_set_for_text();     /* reset to text mode      */
		}
		return TRUE;
	}
	driver_unstack_screen();
	return FALSE;
}

static void handle_options(int kbdchar, int *kbdmore, int *old_maxit)
{
	int i;
	*old_maxit = maxit;
	clear_zoombox();
	if (fromtext_flag == 1)
	{
		fromtext_flag = 0;
	}
	else
	{
		driver_stack_screen();
	}
	switch (kbdchar)
	{
	case 'x':		i = get_toggles(); break;
	case 'y':		i = get_toggles2(); break;
	case 'p':		i = passes_options(); break;
	case 'z':		i = get_fract_params(1); break;
	case 'v':		i = get_view_params(); break;
	case FIK_CTL_B:	i = get_browse_params(); break;
	case FIK_CTL_E:
        i = get_evolve_Parms();
        if (i > 0)
		{
			start_showorbit = 0;
			soundflag &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); /* turn off only x,y,z */
			Log_Auto_Calc = 0; /* turn it off */
        }
		break;
	case FIK_CTL_F:	i = get_sound_params(); break;
	default:
        i = get_cmd_string();
		break;
	}
    driver_unstack_screen();
    if (evolving && truecolor)
	{
        truecolor = 0; /* truecolor doesn't play well with the evolver */
	}
    if (maxit > *old_maxit
		&& inside >= 0
		&& calc_status == CALCSTAT_COMPLETED
		&& curfractalspecific->calctype == StandardFractal
		&& !LogFlag
		&& !truecolor /* recalc not yet implemented with truecolor */
		&& !(usr_stdcalcmode == 't' && fillcolor > -1) /* tesseral with fill doesn't work */
		&& !(usr_stdcalcmode == 'o')
		&& i == 1 /* nothing else changed */
		&& outside != ATAN)
	{
		quick_calc = 1;
		old_stdcalcmode = usr_stdcalcmode;
		usr_stdcalcmode = '1';
		*kbdmore = 0;
		calc_status = CALCSTAT_RESUMABLE;
		i = 0;
	}
	else if (i > 0)
	{              /* time to redraw? */
		quick_calc = 0;
		param_history(0); /* save history */
		*kbdmore = 0;
		calc_status = CALCSTAT_PARAMS_CHANGED;
	}
}

static int handle_execute_commands(int *kbdchar, int *kbdmore)
{
	int i;
    driver_stack_screen();
    i = get_commands();
    if (g_init_mode != -1)
	{                         /* video= was specified */
		g_adapter = g_init_mode;
		g_init_mode = -1;
		i |= CMDARG_FRACTAL_PARAM;
		savedac = 0;
	}
	else if (colorpreloaded)
	{                         /* colors= was specified */
		spindac(0, 1);
		colorpreloaded = 0;
	}
	else if (i & CMDARG_RESET)         /* reset was specified */
	{
		savedac = 0;
	}
	if (i & CMDARG_3D_YES)
	{                         /* 3d = was specified */
		*kbdchar = '3';
		driver_unstack_screen();
		return TRUE;
	}
	if (i & CMDARG_FRACTAL_PARAM)
	{                         /* fractal parameter changed */
		driver_discard_screen();
		/* backwards_v18();*/  /* moved this to cmdfiles.c */
		/* backwards_v19();*/
		*kbdmore = 0;
		calc_status = CALCSTAT_PARAMS_CHANGED;
	}
	else
	{
		driver_unstack_screen();
	}

	return FALSE;
}

static int handle_toggle_float(void)
{
    if (usr_floatflag == 0)
	{
        usr_floatflag = 1;
	}
    else if (stdcalcmode != 'o') /* don't go there */
	{
        usr_floatflag = 0;
	}
    g_init_mode = g_adapter;
    return IMAGESTART;
}

static int handle_ant(void)
{
	int oldtype, err, i;
	double oldparm[MAXPARAMS];

	clear_zoombox();
	oldtype = fractype;
	for (i = 0; i < MAXPARAMS; i++)
	{
		oldparm[i] = param[i];
	}
	if (fractype != ANT)
	{
		fractype = ANT;
		curfractalspecific = &fractalspecific[fractype];
		load_params(fractype);
	}
	if (!fromtext_flag)
	{
		driver_stack_screen();
	}
	fromtext_flag = 0;
	err = get_fract_params(2);
	if (err >= 0)
	{
		driver_unstack_screen();
		if (ant() >= 0)
		{
			calc_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
	else
	{
		driver_unstack_screen();
	}
	fractype = oldtype;
	for (i = 0; i < MAXPARAMS; i++)
	{
		param[i] = oldparm[i];
	}
	return (err >= 0);
}

static int handle_recalc(int (*continue_check)(void), int (*recalc_check)(void))
{
	_ASSERTE(continue_check && recalc_check);
	clear_zoombox();
	if ((*continue_check)() >= 0)
	{
		if ((*recalc_check)() >= 0)
		{
			calc_status = CALCSTAT_PARAMS_CHANGED;
		}
		return TRUE;
	}
	return FALSE;
}

static void handle_3d_params(int *kbdmore)
{
	if (get_fract3d_params() >= 0)    /* get the parameters */
	{
		calc_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;    /* time to redraw */
	}
}

static void handle_orbits(void)
{
	/* must use standard fractal and have a float variant */
	if ((fractalspecific[fractype].calctype == StandardFractal
			|| fractalspecific[fractype].calctype == calcfroth)
		&& (fractalspecific[fractype].isinteger == FALSE
			|| fractalspecific[fractype].tofloat != NOFRACTAL)
		&& !bf_math /* for now no arbitrary precision support */
		&& !(g_is_true_color && truemode))
	{
		clear_zoombox();
		Jiim(ORBIT);
	}
}

static void handle_mandelbrot_julia_toggle(int *kbdmore, int *frommandel)
{
	static double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
	static double  jxx3rd, jyy3rd;

	if (bf_math || evolving)
	{
		return;
	}
	if (fractype == CELLULAR)
	{
		nxtscreenflag = !nxtscreenflag;
		calc_status = CALCSTAT_RESUMABLE;
		*kbdmore = 0;
		return;
	}

	if (fractype == FORMULA || fractype == FFORMULA)
	{
		if (ismand)
		{
			fractalspecific[fractype].tojulia = fractype;
			fractalspecific[fractype].tomandel = NOFRACTAL;
			ismand = 0;
		}
		else
		{
			fractalspecific[fractype].tojulia = NOFRACTAL;
			fractalspecific[fractype].tomandel = fractype;
			ismand = 1;
		}
	}

	if (curfractalspecific->tojulia != NOFRACTAL
		&& param[0] == 0.0
		&& param[1] == 0.0)
	{
		/* switch to corresponding Julia set */
		int key;
		hasinverse = (fractype == MANDEL || fractype == MANDELFP)
			&& (bf_math == 0) ? TRUE : FALSE;
		clear_zoombox();
		Jiim(JIIM);
		key = driver_get_key();    /* flush keyboard buffer */
		if (key != FIK_SPACE)
		{
			driver_unget_key(key);
			return;
		}
		fractype = curfractalspecific->tojulia;
		curfractalspecific = &fractalspecific[fractype];
		if (xcjul == BIG || ycjul == BIG)
		{
			param[0] = (xxmax + xxmin) / 2;
			param[1] = (yymax + yymin) / 2;
		}
		else
		{
			param[0] = xcjul;
			param[1] = ycjul;
			xcjul = ycjul = BIG;
		}
		jxxmin = sxmin;
		jxxmax = sxmax;
		jyymax = symax;
		jyymin = symin;
		jxx3rd = sx3rd;
		jyy3rd = sy3rd;
		*frommandel = 1;
		xxmin = curfractalspecific->xmin;
		xxmax = curfractalspecific->xmax;
		yymin = curfractalspecific->ymin;
		yymax = curfractalspecific->ymax;
		xx3rd = xxmin;
		yy3rd = yymin;
		if (usr_distest == 0
			&& usr_biomorph != -1
			&& bitshift != 29)
		{
			xxmin *= 3.0;
			xxmax *= 3.0;
			yymin *= 3.0;
			yymax *= 3.0;
			xx3rd *= 3.0;
			yy3rd *= 3.0;
		}
		zoomoff = TRUE;
		calc_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
	}
	else if (curfractalspecific->tomandel != NOFRACTAL)
	{
		/* switch to corresponding Mandel set */
		fractype = curfractalspecific->tomandel;
		curfractalspecific = &fractalspecific[fractype];
		if (*frommandel)
		{
			xxmin = jxxmin;
			xxmax = jxxmax;
			yymin = jyymin;
			yymax = jyymax;
			xx3rd = jxx3rd;
			yy3rd = jyy3rd;
		}
		else
		{
			xxmin = xx3rd = curfractalspecific->xmin;
			xxmax = curfractalspecific->xmax;
			yymin = yy3rd = curfractalspecific->ymin;
			yymax = curfractalspecific->ymax;
		}
		SaveC.x = param[0];
		SaveC.y = param[1];
		param[0] = 0;
		param[1] = 0;
		zoomoff = TRUE;
		calc_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
	}
	else
	{
		driver_buzzer(BUZZER_ERROR);          /* can't switch */
	}
}

static void handle_inverse_julia_toggle(int *kbdmore)
{
	/* if the inverse types proliferate, something more elegant will be
	* needed */
	if (fractype == JULIA || fractype == JULIAFP || fractype == INVERSEJULIA)
	{
		static int oldtype = -1;
		if (fractype == JULIA || fractype == JULIAFP)
		{
			oldtype = fractype;
			fractype = INVERSEJULIA;
		}
		else if (fractype == INVERSEJULIA)
		{
			fractype = (oldtype != -1) ? oldtype : JULIA;
		}
		curfractalspecific = &fractalspecific[fractype];
		zoomoff = TRUE;
		calc_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
	}
#if 0
	else if (fractype == MANDEL || fractype == MANDELFP)
	{
		clear_zoombox();
		Jiim(JIIM);
	}
#endif
	else
	{
		driver_buzzer(BUZZER_ERROR);
	}
}

static int handle_history(char *stacked, int kbdchar)
{
	if (name_stack_ptr >= 1)
	{
		/* go back one file if somewhere to go (ie. browsing) */
		name_stack_ptr--;
		while (file_name_stack[name_stack_ptr][0] == '\0' 
			&& name_stack_ptr >= 0)
		{
			name_stack_ptr--;
		}
		if (name_stack_ptr < 0) /* oops, must have deleted first one */
		{
			return 0;
		}
		strcpy(browsename, file_name_stack[name_stack_ptr]);
		/*
		splitpath(browsename, NULL, NULL, fname, ext);
		splitpath(readname, drive, dir, NULL, NULL);
		makepath(readname, drive, dir, fname, ext);
		*/
		merge_pathnames(readname, browsename, 2);
		browsing = TRUE;
		no_sub_images = FALSE;
		showfile = 0;
		if (askvideo)
		{
			driver_stack_screen();      /* save graphics image */
			*stacked = 1;
		}
		return RESTORESTART;
	}
	else if (maxhistory > 0 && bf_math == 0)
	{
		if (kbdchar == '\\' || kbdchar == 'h')
		{
			history_back();
		}
		else if (kbdchar == FIK_CTL_BACKSLASH || kbdchar == FIK_BACKSPACE)
		{
			history_forward();
		}
		restore_history_info();
		zoomoff = TRUE;
		g_init_mode = g_adapter;
		if (curfractalspecific->isinteger != 0
			&& curfractalspecific->tofloat != NOFRACTAL)
		{
			usr_floatflag = 0;
		}
		if (curfractalspecific->isinteger == 0
			&& curfractalspecific->tofloat != NOFRACTAL)
		{
			usr_floatflag = 1;
		}
		return IMAGESTART;
	}

	return 0;
}

static int handle_color_cycling(int kbdchar)
{
	clear_zoombox();
	memcpy(olddacbox, g_dac_box, 256 * 3);
	rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
	if (memcmp(olddacbox, g_dac_box, 256 * 3))
	{
		colorstate = 1;
		save_history_info();
	}
	return CONTINUE;
}

static int handle_color_editing(int *kbdmore)
{
	if (g_is_true_color && !initbatch) /* don't enter palette editor */
	{
		if (load_palette() >= 0)
		{
			*kbdmore = 0;
			calc_status = CALCSTAT_PARAMS_CHANGED;
			return 0;
		}
		else
		{
			return CONTINUE;
		}
	}
	clear_zoombox();
	if (g_dac_box[0][0] != 255
		&& colors >= 16
		&& !driver_diskp())
	{
		int oldhelpmode = helpmode;
		memcpy(olddacbox, g_dac_box, 256*3);
		helpmode = HELPXHAIR;
		EditPalette();
		helpmode = oldhelpmode;
		if (memcmp(olddacbox, g_dac_box, 256*3))
		{
			colorstate = 1;
			save_history_info();
		}
	}
	return CONTINUE;
}

static int handle_save_to_disk(void)
{
    if (driver_diskp() && disktarga == 1)
	{
        return CONTINUE;  /* disk video and targa, nothing to save */
	}
    note_zoom();
    savetodisk(savename);
    restore_zoom();
    return CONTINUE;
}

static int handle_restore_from(int *frommandel, int kbdchar, char *stacked)
{
	comparegif = 0;
	*frommandel = 0;
	if (browsing)
	{
		browsing = FALSE;
	}
	if (kbdchar == 'r')
	{
		if (debugflag == 50)
		{
			comparegif = overlay3d = 1;
			if (initbatch == INIT_BATCH_SAVE)
			{
				driver_stack_screen();   /* save graphics image */
				strcpy(readname, savename);
				showfile = 0;
				return RESTORESTART;
			}
		}
		else
		{
			comparegif = overlay3d = 0;
		}
		display3d = 0;
	}
	driver_stack_screen();            /* save graphics image */
	*stacked = overlay3d ? 0 : 1;
	if (resave_flag)
	{
		updatesavename(savename);      /* do the pending increment */
		resave_flag = RESAVE_NO;
		started_resaves = FALSE;
	}
	showfile = -1;
	return RESTORESTART;
}

static int handle_look_for_files(char *stacked)
{
	if ((zwidth != 0) || driver_diskp())
	{
		browsing = FALSE;
		driver_buzzer(BUZZER_ERROR);             /* can't browse if zooming or disk video */
	}
	else if (look(stacked))
	{
		return RESTORESTART;
	}
	return 0;
}

static void handle_zoom_in(int *kbdmore)
{
#ifdef XFRACT
	XZoomWaiting = 0;
#endif
	if (zwidth != 0.0)
	{                         /* do a zoom */
		init_pan_or_recalc(0);
		*kbdmore = 0;
	}
	if (calc_status != CALCSTAT_COMPLETED)     /* don't restart if image complete */
	{
		*kbdmore = 0;
	}
}

static void handle_zoom_out(int *kbdmore)
{
	if (zwidth != 0.0)
	{
		init_pan_or_recalc(1);
		*kbdmore = 0;
		zoomout();                /* calc corners for zooming out */
	}
}

static void handle_zoom_skew(int negative)
{
	if (negative)
	{
		if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
		{
			int i = key_count(FIK_CTL_HOME);
			if ((zskew -= 0.02 * i) < -0.48)
			{
				zskew = -0.48;
			}
		}
	}
	else
	{
		if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
		{
			int i = key_count(FIK_CTL_END);
			if ((zskew += 0.02 * i) > 0.48)
			{
				zskew = 0.48;
			}
		}
	}
}

static void handle_select_video(int *kbdchar)
{
	driver_stack_screen();
	*kbdchar = select_video_mode(g_adapter);
	if (check_vidmode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
	{
		driver_discard_screen();
	}
	else
	{
		driver_unstack_screen();
	}
}

static void handle_mutation_level(int kbdchar, int *kbdmore)
{
	viewwindow = evolving = 1;
	set_mutation_level(kbdchar-1119);
	param_history(0); /* save parameter history */
	*kbdmore = 0;
	calc_status = CALCSTAT_PARAMS_CHANGED;
}

static int handle_video_mode(int kbdchar, int *kbdmore)
{
	int k = check_vidmode_key(0, kbdchar);
	if (k >= 0)
	{
		g_adapter = k;
		if (g_video_table[g_adapter].colors != colors)
		{
			savedac = 0;
		}
		calc_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
		return CONTINUE;
	}
	return 0;
}

static void handle_z_rotate(int increase)
{
	if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
	{
		if (increase)
		{
			zrotate += key_count(FIK_CTL_MINUS);
		}
		else
		{
			zrotate -= key_count(FIK_CTL_PLUS);
		}
	}
}

static void handle_box_color(int increase)
{
	if (increase)
	{
		boxcolor += key_count(FIK_CTL_INSERT);
	}
	else
	{
		boxcolor -= key_count(FIK_CTL_DEL);
	}
}

static void handle_zoom_resize(int zoom_in)
{
	if (zoom_in)
	{
		if (zoomoff == TRUE)
		{
			if (zwidth == 0)
			{                      /* start zoombox */
				zwidth = zdepth = 1.0;
				zskew = 0.0;
				zrotate = 0;
				zbx = zby = 0.0;
				find_special_colors();
				boxcolor = g_color_bright;
				px = py = gridsz/2;
				moveboxf(0.0, 0.0); /* force scrolling */
			}
			else
			{
				resizebox(-key_count(FIK_PAGE_UP));
			}
		}
	}
	else
	{
		/* zoom out */
		if (boxcount)
		{
			if (zwidth >= 0.999 && zdepth >= 0.999) /* end zoombox */
			{
				zwidth = 0.0;
			}
			else
			{
				resizebox(key_count(FIK_PAGE_DOWN));
			}
		}
	}
}

static void handle_zoom_stretch(int narrower)
{
	if (boxcount)
	{
		chgboxi(0, narrower ?
			-2*key_count(FIK_CTL_PAGE_UP) : 2*key_count(FIK_CTL_PAGE_DOWN));
	}
}

int main_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, char *stacked, int axmode)
{
	int i;
	long old_maxit;

	if (quick_calc && calc_status == CALCSTAT_COMPLETED)
	{
		quick_calc = 0;
		usr_stdcalcmode = old_stdcalcmode;
	}
	if (quick_calc && calc_status != CALCSTAT_COMPLETED)
	{
		usr_stdcalcmode = old_stdcalcmode;
	}
	switch (*kbdchar)
	{
	case 't':                    /* new fractal type             */
		if (handle_fractal_type(frommandel))
		{
			return IMAGESTART;
		}
		break;

	case FIK_CTL_X:                     /* Ctl-X, Ctl-Y, CTL-Z do flipping */
	case FIK_CTL_Y:
	case FIK_CTL_Z:
		flip_image(*kbdchar);
		break;

	case 'x':                    /* invoke options screen        */
	case 'y':
	case 'p':                    /* passes options      */
	case 'z':                    /* type specific parms */
	case 'g':
	case 'v':
	case FIK_CTL_B:
	case FIK_CTL_E:
	case FIK_CTL_F:
		handle_options(*kbdchar, kbdmore, &old_maxit);
		break;

#ifndef XFRACT
	case '@':                    /* execute commands */
	case '2':                    /* execute commands */
#else
	case FIK_F2:                     /* execute commands */
#endif
		if (handle_execute_commands(kbdchar, kbdmore))
		{
			goto do_3d_transform;  /* pretend '3' was keyed */
		}
		break;

	case 'f':
		return handle_toggle_float();

	case 'i':                    /* 3d fractal parms */
		handle_3d_params(kbdmore);
		break;

	case FIK_CTL_A:                     /* ^a Ant */
		if (handle_ant())
		{
			return CONTINUE;
		}
		break;

	case 'k':                    /* ^s is irritating, give user a single key */
	case FIK_CTL_S:                     /* ^s RDS */
		if (handle_recalc(get_rds_params, do_AutoStereo))
		{
			return CONTINUE;
		}
		break;

	case 'a':                    /* starfield parms               */
		if (handle_recalc(get_starfield_params, starfield))
		{
			return CONTINUE;
		}
		break;

	case FIK_CTL_O:                     /* ctrl-o */
	case 'o':
		handle_orbits();
		break;

	case FIK_SPACE:                  /* spacebar, toggle mand/julia   */
		handle_mandelbrot_julia_toggle(kbdmore, frommandel);
		break;

	case 'j':                    /* inverse julia toggle */
		handle_inverse_julia_toggle(kbdmore);
		break;

	case '\\':                   /* return to prev image    */
	case FIK_CTL_BACKSLASH:
	case 'h':
	case FIK_BACKSPACE:
		i = handle_history(stacked, *kbdchar);
		if (i != 0)
		{
			return i;
		}
		break;

	case 'd':                    /* shell to MS-DOS              */
		driver_stack_screen();
		driver_shell();
		driver_unstack_screen();
		break;

	case 'c':                    /* switch to color cycling      */
	case '+':                    /* rotate palette               */
	case '-':                    /* rotate palette               */
		return handle_color_cycling(*kbdchar);

	case 'e':                    /* switch to color editing      */
		i = handle_color_editing(kbdmore);
		if (i != 0)
		{
			return i;
		}
		break;

	case 's':                    /* save-to-disk                 */
		return handle_save_to_disk();

	case '#':                    /* 3D overlay                   */
#ifdef XFRACT
	case FIK_F3:                     /* 3D overlay                   */
#endif
		clear_zoombox();
		overlay3d = 1;
		/* fall through */

do_3d_transform:
	case '3':                    /* restore-from (3d)            */
		display3d = overlay3d ? 2 : 1; /* for <b> command               */
		/* fall through */

	case 'r':                    /* restore-from                 */
		return handle_restore_from(frommandel, *kbdchar, stacked);

	case 'l':
	case 'L':                    /* Look for other files within this view */
		i = handle_look_for_files(stacked);
		if (i != 0)
		{
			return i;
		}
		break;

	case 'b':                    /* make batch file              */
		make_batch_file();
		break;

	case FIK_CTL_P:                    /* print current image          */
		driver_buzzer(BUZZER_INTERRUPT);
		return CONTINUE;

	case FIK_ENTER:                  /* Enter                        */
	case FIK_ENTER_2:                /* Numeric-Keypad Enter         */
		handle_zoom_in(kbdmore);
		break;

	case FIK_CTL_ENTER:              /* control-Enter                */
	case FIK_CTL_ENTER_2:            /* Control-Keypad Enter         */
		handle_zoom_out(kbdmore);
		break;

	case FIK_INSERT:         /* insert                       */
		driver_set_for_text();           /* force text mode */
		return RESTART;

	case FIK_LEFT_ARROW:             /* cursor left                  */
	case FIK_RIGHT_ARROW:            /* cursor right                 */
	case FIK_UP_ARROW:               /* cursor up                    */
	case FIK_DOWN_ARROW:             /* cursor down                  */
	case FIK_CTL_LEFT_ARROW:           /* Ctrl-cursor left             */
	case FIK_CTL_RIGHT_ARROW:          /* Ctrl-cursor right            */
	case FIK_CTL_UP_ARROW:             /* Ctrl-cursor up               */
	case FIK_CTL_DOWN_ARROW:           /* Ctrl-cursor down             */
		move_zoombox(*kbdchar);
		break;

	case FIK_CTL_HOME:               /* Ctrl-home                    */
	case FIK_CTL_END:                /* Ctrl-end                     */
		handle_zoom_skew(*kbdchar == FIK_CTL_HOME);
		break;

	case FIK_CTL_PAGE_UP:            /* Ctrl-pgup                    */
	case FIK_CTL_PAGE_DOWN:          /* Ctrl-pgdn                    */
		handle_zoom_stretch(FIK_CTL_PAGE_UP == *kbdchar);
		break;

	case FIK_PAGE_UP:                /* page up                      */
	case FIK_PAGE_DOWN:              /* page down                    */
		handle_zoom_resize(FIK_PAGE_UP == *kbdchar);
		break;

	case FIK_CTL_MINUS:              /* Ctrl-kpad-                  */
	case FIK_CTL_PLUS:               /* Ctrl-kpad+               */
		handle_z_rotate(FIK_CTL_MINUS == *kbdchar);
		break;

	case FIK_CTL_INSERT:             /* Ctrl-ins                 */
	case FIK_CTL_DEL:                /* Ctrl-del                 */
		handle_box_color(FIK_CTL_INSERT == *kbdchar);
		break;

	case FIK_ALT_1: /* alt + number keys set mutation level and start evolution engine */
	case FIK_ALT_2: 
	case FIK_ALT_3: 
	case FIK_ALT_4:
	case FIK_ALT_5:
	case FIK_ALT_6:
	case FIK_ALT_7:
		handle_mutation_level(*kbdchar, kbdmore);
		break;

	case FIK_DELETE:         /* select video mode from list */
		handle_select_video(kbdchar);
		/* fall through */

	default:                     /* other (maybe a valid Fn key) */
		i = handle_video_mode(*kbdchar, kbdmore);
		if (i != 0)
		{
			return i;
		}
		break;
   }                            /* end of the big switch */
   return 0;
}

int evolver_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, char *stacked)
{
	int i, k;

	switch (*kbdchar)
	{
	case 't':                    /* new fractal type             */
		if (handle_fractal_type(frommandel))
		{
			return IMAGESTART;
		}
		break;
   case 'x':                    /* invoke options screen        */
   case 'y':
   case 'p':                    /* passes options      */
   case 'z':                    /* type specific parms */
   case 'g':
   case FIK_CTL_E:
   case FIK_SPACE:
      clear_zoombox();
      if (fromtext_flag == 1)
         fromtext_flag = 0;
      else
         driver_stack_screen();
      if (*kbdchar == 'x')
         i = get_toggles();
      else if (*kbdchar == 'y')
         i = get_toggles2();
      else if (*kbdchar == 'p')
         i = passes_options();
      else if (*kbdchar == 'z')
         i = get_fract_params(1);
      else if (*kbdchar == 5 || *kbdchar == FIK_SPACE)
         i = get_evolve_Parms();
      else
         i = get_cmd_string();
      driver_unstack_screen();
      if (evolving && truecolor)
         truecolor = 0; /* truecolor doesn't play well with the evolver */
      if (i > 0) {              /* time to redraw? */
         param_history(0); /* save history */
         *kbdmore = 0;
		 calc_status = CALCSTAT_PARAMS_CHANGED;
      }
      break;
   case 'b': /* quick exit from evolve mode */
      evolving = viewwindow = 0;
      param_history(0); /* save history */
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;

   case 'f':                    /* floating pt toggle           */
      if (usr_floatflag == 0)
         usr_floatflag = 1;
      else if (stdcalcmode != 'o') /* don't go there */
         usr_floatflag = 0;
      g_init_mode = g_adapter;
      return IMAGESTART;
   case '\\':                   /* return to prev image    */
   case FIK_CTL_BACKSLASH:
   case 'h':
   case FIK_BACKSPACE:
      if (maxhistory > 0 && bf_math == 0)
      {
         if (*kbdchar == '\\' || *kbdchar == 'h')
			 history_back();
         if (*kbdchar == FIK_CTL_BACKSLASH || *kbdchar == 8)
			 history_forward();
         restore_history_info();
         zoomoff = TRUE;
         g_init_mode = g_adapter;
         if (curfractalspecific->isinteger != 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 0;
         if (curfractalspecific->isinteger == 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 1;
         return IMAGESTART;
      }
      break;
   case 'c':                    /* switch to color cycling      */
   case '+':                    /* rotate palette               */
   case '-':                    /* rotate palette               */
      clear_zoombox();
      memcpy(olddacbox, g_dac_box, 256 * 3);
      rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
      if (memcmp(olddacbox, g_dac_box, 256 * 3))
      {
         colorstate = 1;
         save_history_info();
      }
      return CONTINUE;
   case 'e':                    /* switch to color editing      */
      if (g_is_true_color && !initbatch) { /* don't enter palette editor */
         if (load_palette() >= 0) {
            *kbdmore = 0;
			calc_status = CALCSTAT_PARAMS_CHANGED;
            break;
         } else
            return CONTINUE;
      }
      clear_zoombox();
      if (g_dac_box[0][0] != 255 && colors >= 16
          && !driver_diskp())
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         memcpy(olddacbox, g_dac_box, 256 * 3);
         helpmode = HELPXHAIR;
         EditPalette();
         helpmode = oldhelpmode;
         if (memcmp(olddacbox, g_dac_box, 256 * 3))
         {
            colorstate = 1;
            save_history_info();
         }
      }
      return CONTINUE;
   case 's':                    /* save-to-disk                 */
{     int oldsxoffs, oldsyoffs, oldxdots, oldydots, oldpx, oldpy;

      if (driver_diskp() && disktarga == 1)
         return CONTINUE;  /* disk video and targa, nothing to save */

	  oldsxoffs = sxoffs;
      oldsyoffs = syoffs;
      oldxdots = xdots;
      oldydots = ydots;
      oldpx = px;
      oldpy = py;
      sxoffs = syoffs = 0;
      xdots = sxdots;
      ydots = sydots; /* for full screen save and pointer move stuff */
      px = py = gridsz / 2;
      param_history(1); /* restore old history */
      fiddleparms(g_genes, 0);
      drawparmbox(1);
      savetodisk(savename);
      px = oldpx;
      py = oldpy;
      param_history(1); /* restore old history */
      fiddleparms(g_genes, unspiralmap());
      sxoffs = oldsxoffs;
      syoffs = oldsyoffs;
      xdots = oldxdots;
      ydots = oldydots;
}
      return CONTINUE;
   case 'r':                    /* restore-from                 */
      comparegif = 0;
      *frommandel = 0;
      if (browsing)
      {
         browsing = FALSE;
      }
      if (*kbdchar == 'r')
      {
         if (debugflag == 50)
         {
            comparegif = overlay3d = 1;
            if (initbatch == INIT_BATCH_SAVE)
            {
               driver_stack_screen();   /* save graphics image */
               strcpy(readname, savename);
               showfile = 0;
               return RESTORESTART;
            }
         }
         else
            comparegif = overlay3d = 0;
         display3d = 0;
      }
      driver_stack_screen();            /* save graphics image */
      if (overlay3d)
         *stacked = 0;
      else
         *stacked = 1;
      if (resave_flag)
      {
         updatesavename(savename);      /* do the pending increment */
         resave_flag = RESAVE_NO;
		 started_resaves = FALSE;
      }
      showfile = -1;
      return RESTORESTART;
   case FIK_ENTER:                  /* Enter                        */
   case FIK_ENTER_2:                /* Numeric-Keypad Enter         */
#ifdef XFRACT
      XZoomWaiting = 0;
#endif
      if (zwidth != 0.0)
      {                         /* do a zoom */
         init_pan_or_recalc(0);
         *kbdmore = 0;
      }
      if (calc_status != CALCSTAT_COMPLETED)     /* don't restart if image complete */
         *kbdmore = 0;
      break;
   case FIK_CTL_ENTER:              /* control-Enter                */
   case FIK_CTL_ENTER_2:            /* Control-Keypad Enter         */
      init_pan_or_recalc(1);
      *kbdmore = 0;
      zoomout();                /* calc corners for zooming out */
      break;
   case FIK_INSERT:         /* insert                       */
      driver_set_for_text();           /* force text mode */
      return RESTART;
   case FIK_LEFT_ARROW:             /* cursor left                  */
   case FIK_RIGHT_ARROW:            /* cursor right                 */
   case FIK_UP_ARROW:               /* cursor up                    */
   case FIK_DOWN_ARROW:             /* cursor down                  */
        move_zoombox(*kbdchar);
        break;
   case FIK_CTL_LEFT_ARROW:           /* Ctrl-cursor left             */
   case FIK_CTL_RIGHT_ARROW:          /* Ctrl-cursor right            */
   case FIK_CTL_UP_ARROW:             /* Ctrl-cursor up               */
   case FIK_CTL_DOWN_ARROW:           /* Ctrl-cursor down             */
        /* borrow ctrl cursor keys for moving selection box */
        /* in evolver mode */
     if (boxcount) {
        int grout;
        if (evolving&1) {
             if (*kbdchar == FIK_CTL_LEFT_ARROW) {
                px--;
             }
             if (*kbdchar == FIK_CTL_RIGHT_ARROW) {
                px++;
             }
             if (*kbdchar == FIK_CTL_UP_ARROW) {
                py--;
             }
             if (*kbdchar == FIK_CTL_DOWN_ARROW) {
                py++;
             }
             if (px <0 ) px = gridsz-1;
             if (px >(gridsz-1)) px = 0;
             if (py <0) py = gridsz-1;
             if (py > (gridsz-1)) py = 0;
             grout = !((evolving & NOGROUT)/NOGROUT) ;
             sxoffs = px * (int)(dxsize+1+grout);
             syoffs = py * (int)(dysize+1+grout);

             param_history(1); /* restore old history */
             fiddleparms(g_genes, unspiralmap()); /* change all parameters */
                         /* to values appropriate to the image selected */
             set_evolve_ranges();
             chgboxi(0,0);
             drawparmbox(0);
        }
     }
     else                       /* if no zoombox, scroll by arrows */
        move_zoombox(*kbdchar);
     break;
   case FIK_CTL_HOME:               /* Ctrl-home                    */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(FIK_CTL_HOME);
         if ((zskew -= 0.02 * i) < -0.48)
            zskew = -0.48;
      }
      break;
   case FIK_CTL_END:                /* Ctrl-end                     */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(FIK_CTL_END);
         if ((zskew += 0.02 * i) > 0.48)
            zskew = 0.48;
      }
      break;
   case FIK_CTL_PAGE_UP:
      if (prmboxcount) {
        parmzoom -= 1.0;
        if (parmzoom<1.0) parmzoom=1.0;
        drawparmbox(0);
        set_evolve_ranges();
      }
      break;
   case FIK_CTL_PAGE_DOWN:
      if (prmboxcount) {
        parmzoom += 1.0;
        if (parmzoom>(double)gridsz/2.0) parmzoom=(double)gridsz/2.0;
        drawparmbox(0);
        set_evolve_ranges();
      }
      break;

   case FIK_PAGE_UP:                /* page up                      */
      if (zoomoff == TRUE)
      {
         if (zwidth == 0)
         {                      /* start zoombox */
            zwidth = zdepth = 1;
            zskew = zrotate = 0;
            zbx = zby = 0;
            find_special_colors();
            boxcolor = g_color_bright;
     /*rb*/ if (evolving&1) {
              /* set screen view params back (previously changed to allow
                          full screen saves in viewwindow mode) */
                   int grout = !((evolving & NOGROUT) / NOGROUT);
                   sxoffs = px * (int)(dxsize+1+grout);
                   syoffs = py * (int)(dysize+1+grout);
                   SetupParamBox();
                   drawparmbox(0);
            }
            moveboxf(0.0,0.0); /* force scrolling */
         }
         else
            resizebox(-key_count(FIK_PAGE_UP));
      }
      break;
   case FIK_PAGE_DOWN:              /* page down                    */
      if (boxcount)
      {
         if (zwidth >= .999 && zdepth >= 0.999) { /* end zoombox */
            zwidth = 0;
            if (evolving&1) {
               drawparmbox(1); /* clear boxes off screen */
               ReleaseParamBox();
            }
         }
         else
            resizebox(key_count(FIK_PAGE_DOWN));
      }
      break;
   case FIK_CTL_MINUS:              /* Ctrl-kpad-                  */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate += key_count(FIK_CTL_MINUS);
      break;
   case FIK_CTL_PLUS:               /* Ctrl-kpad+               */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate -= key_count(FIK_CTL_PLUS);
      break;
   case FIK_CTL_INSERT:             /* Ctrl-ins                 */
   case FIK_CTL_DEL:                /* Ctrl-del                 */
		handle_box_color(FIK_CTL_INSERT == *kbdchar);
      break;

   /* grabbed a couple of video mode keys, user can change to these using
       delete and the menu if necessary */

   case FIK_F2: /* halve mutation params and regen */
      fiddlefactor = fiddlefactor / 2;
      paramrangex = paramrangex / 2;
      newopx = opx + paramrangex / 2;
      paramrangey = paramrangey / 2;
      newopy = opy + paramrangey / 2;
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;

   case FIK_F3: /*double mutation parameters and regenerate */
   {
    double centerx, centery;
      fiddlefactor = fiddlefactor * 2;
      centerx = opx + paramrangex / 2;
      paramrangex = paramrangex * 2;
      newopx = centerx - paramrangex / 2;
      centery = opy + paramrangey / 2;
      paramrangey = paramrangey * 2;
      newopy = centery - paramrangey / 2;
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;
   }

   case FIK_F4: /*decrement  gridsize and regen */
      if (gridsz > 3) {
        gridsz = gridsz - 2;  /* gridsz must have odd value only */
        *kbdmore = 0;
		calc_status = CALCSTAT_PARAMS_CHANGED;
        }
      break;

   case FIK_F5: /* increment gridsize and regen */
      if (gridsz < (sxdots / (MINPIXELS<<1))) {
         gridsz = gridsz + 2;
         *kbdmore = 0;
		 calc_status = CALCSTAT_PARAMS_CHANGED;
         }
      break;

   case FIK_F6: /* toggle all variables selected for random variation to
               center weighted variation and vice versa */
         {
          int i;
          for (i =0;i < NUMGENES; i++) {
            if (g_genes[i].mutate == 5) {
               g_genes[i].mutate = 6;
               continue;
            }
            if (g_genes[i].mutate == 6) g_genes[i].mutate = 5;
          }
        }
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;

   case 1120: /* alt + number keys set mutation level */
   case 1121: 
   case 1122: 
   case 1123:
   case 1124:
   case 1125:
   case 1126:
/*
   case 1127:
   case 1128:
*/
      set_mutation_level(*kbdchar-1119);
      param_history(1); /* restore old history */
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;

   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
/*  add these in when more parameters can be varied
   case '8':
   case '9':
*/
      set_mutation_level(*kbdchar-(int)'0');
      param_history(1); /* restore old history */
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;
   case '0': /* mutation level 0 == turn off evolving */ 
      evolving = viewwindow = 0;
      *kbdmore = 0;
	  calc_status = CALCSTAT_PARAMS_CHANGED;
      break;

   case FIK_DELETE:         /* select video mode from list */
      driver_stack_screen();
      *kbdchar = select_video_mode(g_adapter);
      if (check_vidmode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
         driver_discard_screen();
      else
         driver_unstack_screen();
      /* fall through */
   default:             /* other (maybe valid Fn key */
      if ((k = check_vidmode_key(0, *kbdchar)) >= 0)
      {
         g_adapter = k;
         if (g_video_table[g_adapter].colors != colors)
            savedac = 0;
         calc_status = CALCSTAT_PARAMS_CHANGED;
         *kbdmore = 0;
         return CONTINUE;
      }
      break;
   }                            /* end of the big evolver switch */
   return 0;
}

static int call_line3d(BYTE *pixels, int linelen)
{
   /* this routine exists because line3d might be in an overlay */
   return line3d(pixels,linelen);
}

static void note_zoom()
{
   if (boxcount) { /* save zoombox stuff in mem before encode (mem reused) */
      savezoom = (char *)malloc((long)(5*boxcount));
	  if (savezoom == NULL)
         clear_zoombox(); /* not enuf mem so clear the box */
      else {
         reset_zoom_corners(); /* reset these to overall image, not box */
         memcpy(savezoom,boxx,boxcount*2);
         memcpy(savezoom+boxcount*2,boxy,boxcount*2);
         memcpy(savezoom+boxcount*4,boxvalues,boxcount);
         }
      }
}

static void restore_zoom()
{
   if (boxcount) { /* restore zoombox arrays */
      memcpy(boxx,savezoom,boxcount*2);
      memcpy(boxy,savezoom+boxcount*2,boxcount*2);
      memcpy(boxvalues,savezoom+boxcount*4,boxcount);
      free(savezoom);
      drawbox(1); /* get the xxmin etc variables recalc'd by redisplaying */
      }
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{  int vertical, horizontal, getmore;
   vertical = horizontal = 0;
   getmore = 1;
   while (getmore) {
      switch (keynum) {
         case FIK_LEFT_ARROW:               /* cursor left */
            --horizontal;
            break;
         case FIK_RIGHT_ARROW:              /* cursor right */
            ++horizontal;
            break;
         case FIK_UP_ARROW:                 /* cursor up */
            --vertical;
            break;
         case FIK_DOWN_ARROW:               /* cursor down */
            ++vertical;
            break;
         case FIK_CTL_LEFT_ARROW:             /* Ctrl-cursor left */
            horizontal -= 8;
            break;
         case FIK_CTL_RIGHT_ARROW:             /* Ctrl-cursor right */
            horizontal += 8;
            break;
         case FIK_CTL_UP_ARROW:               /* Ctrl-cursor up */
            vertical -= 8;
            break;
         case FIK_CTL_DOWN_ARROW:             /* Ctrl-cursor down */
            vertical += 8;
            break;                      /* += 8 needed by VESA scrolling */
         default:
            getmore = 0;
         }
      if (getmore) {
         if (getmore == 2)              /* eat last key used */
            driver_get_key();
         getmore = 2;
         keynum = driver_key_pressed();         /* next pending key */
         }
      }
   if (boxcount) {
/*
      if (horizontal != 0)
         moveboxf((double)horizontal/dxsize,0.0);
      if (vertical != 0)
         moveboxf(0.0,(double)vertical/dysize);
*/
      moveboxf((double)horizontal/dxsize,(double)vertical/dysize);
      }
#ifndef XFRACT
   else                                 /* if no zoombox, scroll by arrows */
      scroll_relative(horizontal,vertical);
#endif
}

/* displays differences between current image file and new image */
static FILE *cmp_fp;
static int errcount;
int cmp_line(BYTE *pixels, int linelen)
{
   int row,col;
   int oldcolor;
   row = g_row_count++;
   if (row == 0) {
      errcount = 0;
      cmp_fp = dir_fopen(workdir,"cmperr",(initbatch)?"a":"w");
      outln_cleanup = cmp_line_cleanup;
      }
   if (pot16bit) { /* 16 bit info, ignore odd numbered rows */
      if ((row & 1) != 0) return 0;
      row >>= 1;
      }
   for (col=0; col<linelen; col++) {
      oldcolor=getcolor(col,row);
      if (oldcolor==(int)pixels[col])
         putcolor(col,row,0);
      else {
         if (oldcolor==0)
            putcolor(col,row,1);
         ++errcount;
         if (initbatch == INIT_BATCH_NONE)
            fprintf(cmp_fp,"#%5d col %3d row %3d old %3d new %3d\n",
               errcount,col,row,oldcolor,pixels[col]);
         }
      }
   return 0;
}

static void cmp_line_cleanup(void)
{
   char *timestring;
   time_t ltime;
   if (initbatch) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(cmp_fp,"%s compare to %s has %5d errs\n",
                     timestring,readname,errcount);
      }
   fclose(cmp_fp);
}

void clear_zoombox()
{
   zwidth = 0;
   drawbox(0);
   reset_zoom_corners();
}

void reset_zoom_corners()
{
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = symax;
   yymin = symin;
   yy3rd = sy3rd;
   if (bf_math)
   {
      copy_bf(bfxmin,bfsxmin);
      copy_bf(bfxmax,bfsxmax);
      copy_bf(bfymin,bfsymin);
      copy_bf(bfymax,bfsymax);
      copy_bf(bfx3rd,bfsx3rd);
      copy_bf(bfy3rd,bfsy3rd);
   }
}

/*
   Function setup287code is called by main() when a 287
   or better fpu is detected.
*/
#define ORBPTR(x) fractalspecific[x].orbitcalc
void setup287code()
{
   ORBPTR(MANDELFP)       = ORBPTR(JULIAFP)      = FJuliafpFractal;
   ORBPTR(BARNSLEYM1FP)   = ORBPTR(BARNSLEYJ1FP) = FBarnsley1FPFractal;
   ORBPTR(BARNSLEYM2FP)   = ORBPTR(BARNSLEYJ2FP) = FBarnsley2FPFractal;
   ORBPTR(MANOWARFP)      = ORBPTR(MANOWARJFP)   = FManOWarfpFractal;
   ORBPTR(MANDELLAMBDAFP) = ORBPTR(LAMBDAFP)     = FLambdaFPFractal;
}

/* read keystrokes while = specified key, return 1+count;       */
/* used to catch up when moving zoombox is slower than keyboard */
int key_count(int keynum)
{  int ctr;
   ctr = 1;
   while (driver_key_pressed() == keynum) {
      driver_get_key();
      ++ctr;
      }
   return ctr;
}
