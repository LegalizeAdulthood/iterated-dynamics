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

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"
#include "EscapeTime.h"
#include "SoundState.h"
#include "CommandParser.h"

#if 0
/* makes a handly list of jul-man pairs, not for release */
static void julman()
{
	FILE *fp;
	int i;
	fp = dir_fopen(g_work_dir, "toggle.txt", "w");
	i = -1;
	while (g_fractal_specific[++i].name)
	{
		if (g_fractal_specific[i].tojulia != FRACTYPE_NO_FRACTAL && g_fractal_specific[i].name[0] != '*')
		{
			fprintf(fp, "%s  %s\n", g_fractal_specific[i].name,
				g_fractal_specific[g_fractal_specific[i].tojulia].name);
		}
	}
	fclose(fp);
}
#endif

char g_from_text_flag = 0;         /* = 1 if we're in graphics mode */
void *g_evolve_handle = NULL;
char g_standard_calculation_mode_old;
void (*g_out_line_cleanup)();

/* routines in this module      */

int main_menu_switch(int*, int*, int*, int *stacked, int);
int evolver_menu_switch(int*, int*, int*, int *stacked);
int big_while_loop(int *kbdmore, int *stacked, int resumeflag);
static void move_zoombox(int);
static  void note_zoom();
static  void restore_zoom();
static  void move_zoombox(int keynum);
static  void cmp_line_cleanup();

static char *savezoom;

int big_while_loop(int *kbdmore, int *stacked, int resumeflag)
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
	{
		goto resumeloop;
	}

	while (1)                    /* eternal loop */
	{
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		if (g_calculation_status != CALCSTAT_RESUMABLE || g_show_file == 0)
		{
			memcpy((char *)&g_video_entry, (char *)&g_video_table[g_adapter],
					sizeof(g_video_entry));
			axmode  = g_video_entry.videomodeax; /* video mode (BIOS call)   */
			g_dot_mode = g_video_entry.dotmode;     /* assembler dot read/write */
			g_x_dots   = g_video_entry.x_dots;       /* # dots across the screen */
			g_y_dots   = g_video_entry.y_dots;       /* # dots down the screen   */
			g_colors  = g_video_entry.colors;      /* # g_colors available */
			g_dot_mode  %= 100;
			g_screen_width  = g_x_dots;
			g_screen_height  = g_y_dots;
			g_sx_offset = g_sy_offset = 0;
			g_rotate_hi = (g_rotate_hi < g_colors) ? g_rotate_hi : g_colors - 1;

			memcpy(g_old_dac_box, g_dac_box, 256*3); /* save the DAC */

			if (g_overlay_3d && !g_initialize_batch)
			{
				driver_unstack_screen();            /* restore old graphics image */
				g_overlay_3d = 0;
			}
			else
			{
				DriverManager::change_video_mode(g_video_entry); /* switch video modes */
				/* switching video modes may have changed drivers or disk flag... */
				if (g_good_mode == 0)
				{
					if (!driver_diskp())
					{
						stop_message(0, "That video mode is not available with your adapter.");
					}
					g_ui_state.ask_video = true;
					g_init_mode = -1;
					driver_set_for_text(); /* switch to text mode */
					/* goto restorestart; */
					return RESTORESTART;
				}

				g_x_dots = g_screen_width;
				g_y_dots = g_screen_height;
				g_video_entry.x_dots = g_x_dots;
				g_video_entry.y_dots = g_y_dots;
			}

			if (g_save_dac || g_color_preloaded)
			{
				memcpy(g_dac_box, g_old_dac_box, 256*3); /* restore the DAC */
				spindac(0, 1);
				g_color_preloaded = FALSE;
			}
			else
			{	/* reset DAC to defaults, which setvideomode has done for us */
				if (g_map_dac_box)
				{	/* but there's a map=, so load that */
					memcpy((char *)g_dac_box, g_map_dac_box, 768);
					spindac(0, 1);
				}
				else if ((driver_diskp() && g_colors == 256) || !g_colors)
				{
					/* disk video, setvideomode via bios didn't get it right, so: */
#if !defined(XFRACT) && !defined(_WIN32)
					validate_luts("default"); /* read the default palette file */
#endif
				}
				g_color_state = COLORSTATE_DEFAULT;
			}
			if (g_view_window)
			{
				/* bypass for VESA virtual screen */
				ftemp = g_final_aspect_ratio*(((double) g_screen_height)/((double) g_screen_width)/g_screen_aspect_ratio);
				g_x_dots = g_view_x_dots;
				if (g_x_dots != 0)
				{	/* g_x_dots specified */
					g_y_dots = g_view_y_dots;
					if (g_y_dots == 0) /* calc g_y_dots? */
					{
						g_y_dots = (int)((double)g_x_dots*ftemp + 0.5);
					}
				}
				else if (g_final_aspect_ratio <= g_screen_aspect_ratio)
				{
					g_x_dots = (int)((double)g_screen_width / g_view_reduction + 0.5);
					g_y_dots = (int)((double)g_x_dots*ftemp + 0.5);
				}
				else
				{
					g_y_dots = (int)((double)g_screen_height / g_view_reduction + 0.5);
					g_x_dots = (int)((double)g_y_dots / ftemp + 0.5);
				}
				if (g_x_dots > g_screen_width || g_y_dots > g_screen_height)
				{
					stop_message(0, "View window too large; using full screen.");
					g_view_window = 0;
					g_x_dots = g_view_x_dots = g_screen_width;
					g_y_dots = g_view_y_dots = g_screen_height;
				}
				else if (((g_x_dots <= 1) /* changed test to 1, so a 2x2 window will */
					|| (g_y_dots <= 1)) /* work with the sound feature */
					&& !(g_evolving & EVOLVE_FIELD_MAP))
				{	/* so ssg works */
					/* but no check if in evolve mode to allow lots of small views*/
					stop_message(0, "View window too small; using full screen.");
					g_view_window = 0;
					g_x_dots = g_screen_width;
					g_y_dots = g_screen_height;
				}
				if ((g_evolving & EVOLVE_FIELD_MAP) && (g_current_fractal_specific->flags & FRACTALFLAG_INFINITE_CALCULATION))
				{
					stop_message(0, "Fractal doesn't terminate! switching off evolution.");
					g_evolving &= ~EVOLVE_FIELD_MAP;
					g_view_window = FALSE;
					g_x_dots = g_screen_width;
					g_y_dots = g_screen_height;
				}
				if (g_evolving & EVOLVE_FIELD_MAP)
				{
					g_x_dots = (g_screen_width / g_grid_size)-!((g_evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
					g_x_dots = g_x_dots - (g_x_dots % 4); /* trim to multiple of 4 for SSG */
					g_y_dots = (g_screen_height / g_grid_size)-!((g_evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
					g_y_dots = g_y_dots - (g_y_dots % 4);
				}
				else
				{
					g_sx_offset = (g_screen_width - g_x_dots) / 2;
					g_sy_offset = (g_screen_height - g_y_dots) / 3;
				}
			}
			g_dx_size = g_x_dots - 1;            /* convert just once now */
			g_dy_size = g_y_dots - 1;
		}
		/* assume we save next time (except jb) */
		g_save_dac = (g_save_dac == SAVEDAC_NO) ? SAVEDAC_NEXT : SAVEDAC_YES;
		if (g_initialize_batch == INITBATCH_NONE)
		{
			driver_set_mouse_mode(-FIK_PAGE_UP);        /* mouse left button == pgup */
		}

		if (g_show_file == 0)
		{               /* loading an image */
			g_out_line_cleanup = NULL;          /* g_out_line routine can set this */
			if (g_display_3d)                 /* set up 3D decoding */
			{
				g_out_line = line3d;
			}
			else if (g_compare_gif)            /* debug 50 */
			{
				g_out_line = cmp_line;
			}
			else if (g_potential_16bit)
			{            /* .pot format input file */
				if (disk_start_potential() < 0)
				{                           /* pot file failed?  */
					g_show_file = 1;
					g_potential_flag  = FALSE;
					g_potential_16bit = FALSE;
					g_init_mode = -1;
					g_calculation_status = CALCSTAT_RESUMABLE;         /* "resume" without 16-bit */
					driver_set_for_text();
					get_fractal_type();
					/* goto imagestart; */
					return IMAGESTART;
				}
				g_out_line = potential_line;
			}
			else if ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !g_evolving) /* regular gif/fra input file */
			{
				g_out_line = sound_line;      /* sound decoding */
			}
			else
			{
				g_out_line = out_line;        /* regular decoding */
			}
			if (2224 == g_debug_flag)
			{
				char msg[MESSAGE_LEN];
				sprintf(msg, "floatflag=%d", g_user_float_flag);
				stop_message(STOPMSG_NO_BUZZER, (char *)msg);
			}
			i = funny_glasses_call(gifview);
			if (g_out_line_cleanup)              /* cleanup routine defined? */
			{
				(*g_out_line_cleanup)();
			}
			if (i == 0)
			{
				driver_buzzer(BUZZER_COMPLETE);
			}
			else
			{
				g_calculation_status = CALCSTAT_NO_FRACTAL;
				if (driver_key_pressed())
				{
					driver_buzzer(BUZZER_INTERRUPT);
					while (driver_key_pressed())
					{
						driver_get_key();
					}
					text_temp_message("*** load incomplete ***");
				}
			}
		}

		g_zoom_off = TRUE;                      /* zooming is enabled */
		if (driver_diskp() || (g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM) != 0)
		{
			g_zoom_off = FALSE;                   /* for these cases disable zooming */
		}
		if (!g_evolving)
		{
			calculate_fractal_initialize();
		}
		driver_schedule_alarm(1);

		g_sx_min = g_escape_time_state.m_grid_fp.x_min(); /* save 3 corners for zoom.c ref points */
		g_sx_max = g_escape_time_state.m_grid_fp.x_max();
		g_sx_3rd = g_escape_time_state.m_grid_fp.x_3rd();
		g_sy_min = g_escape_time_state.m_grid_fp.y_min();
		g_sy_max = g_escape_time_state.m_grid_fp.y_max();
		g_sy_3rd = g_escape_time_state.m_grid_fp.y_3rd();

		if (g_bf_math)
		{
			copy_bf(bfsxmin, g_escape_time_state.m_grid_bf.x_min());
			copy_bf(bfsxmax, g_escape_time_state.m_grid_bf.x_max());
			copy_bf(bfsymin, g_escape_time_state.m_grid_bf.y_min());
			copy_bf(bfsymax, g_escape_time_state.m_grid_bf.y_max());
			copy_bf(bfsx3rd, g_escape_time_state.m_grid_bf.x_3rd());
			copy_bf(bfsy3rd, g_escape_time_state.m_grid_bf.y_3rd());
		}
		history_save_info();

		if (g_show_file == 0)
		{               /* image has been loaded */
			g_show_file = 1;
			if (g_initialize_batch == INITBATCH_NORMAL && g_calculation_status == CALCSTAT_RESUMABLE)
			{
				g_initialize_batch = INITBATCH_FINISH_CALC; /* flag to finish calc before save */
			}
			if (g_loaded_3d)      /* 'r' of image created with '3' */
			{
				g_display_3d = DISPLAY3D_YES;  /* so set flag for 'b' command */
			}
		}
		else
		{                            /* draw an image */
			if (g_save_time != 0          /* autosave and resumable? */
				&& (g_current_fractal_specific->flags & FRACTALFLAG_NOT_RESUMABLE) == 0)
			{
				g_save_base = read_ticker(); /* calc's start time */
				g_save_ticks = g_save_time*60*1000; /* in milliseconds */
				g_finish_row = -1;
			}
			g_browsing = FALSE;      /* regenerate image, turn off browsing */
			/*rb*/
			g_name_stack_ptr = -1;   /* reset pointer */
			g_browse_name[0] = '\0';  /* null */
			if (g_view_window && (g_evolving & EVOLVE_FIELD_MAP) && (g_calculation_status != CALCSTAT_COMPLETED))
			{
				/* generate a set of images with varied parameters on each one */
				int grout, ecount, tmpxdots, tmpydots, gridsqr;
				struct evolution_info resume_e_info;

				if ((g_evolve_handle != NULL) && (g_calculation_status == CALCSTAT_RESUMABLE))
				{
					memcpy(&resume_e_info, g_evolve_handle, sizeof(resume_e_info));
					g_parameter_range_x  = resume_e_info.parameter_range_x;
					g_parameter_range_y  = resume_e_info.parameter_range_y;
					g_parameter_offset_x = g_new_parameter_offset_x = resume_e_info.opx;
					g_parameter_offset_y = g_new_parameter_offset_y = resume_e_info.opy;
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x = (char)resume_e_info.odpx;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y = (char)resume_e_info.odpy;
					g_px           = resume_e_info.px;
					g_py           = resume_e_info.py;
					g_sx_offset       = resume_e_info.sxoffs;
					g_sy_offset       = resume_e_info.syoffs;
					g_x_dots        = resume_e_info.x_dots;
					g_y_dots        = resume_e_info.y_dots;
					g_grid_size       = resume_e_info.gridsz;
					g_this_generation_random_seed = resume_e_info.this_generation_random_seed;
					g_fiddle_factor   = resume_e_info.fiddle_factor;
					g_evolving     = resume_e_info.evolving;
					if (g_evolving)
					{
						g_view_window = TRUE;
					}
					ecount       = resume_e_info.ecount;
					free(g_evolve_handle);  /* We're done with it, release it. */
					g_evolve_handle = NULL;
				}
				else
				{ /* not resuming, start from the beginning */
					int mid = g_grid_size / 2;
					if ((g_px != mid) || (g_py != mid))
					{
						g_this_generation_random_seed = (unsigned int)clock_ticks(); /* time for new set */
					}
					save_parameter_history();
					ecount = 0;
					g_fiddle_factor = g_fiddle_factor*g_fiddle_reduction;
					g_parameter_offset_x = g_new_parameter_offset_x;
					g_parameter_offset_y = g_new_parameter_offset_y;
					/* odpx used for discrete parms like inside, outside, trigfn etc */
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y; 
				}
				g_parameter_box_count = 0;
				g_delta_parameter_image_x = g_parameter_range_x/(g_grid_size-1);
				g_delta_parameter_image_y = g_parameter_range_y/(g_grid_size-1);
				grout  = !((g_evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
				tmpxdots = g_x_dots + grout;
				tmpydots = g_y_dots + grout;
				gridsqr = g_grid_size*g_grid_size;
				while (ecount < gridsqr)
				{
					spiral_map(ecount); /* sets px & py */
					g_sx_offset = tmpxdots*g_px;
					g_sy_offset = tmpydots*g_py;
					restore_parameter_history();
					fiddle_parameters(g_genes, ecount);
					calculate_fractal_initialize();
					if (calculate_fractal() == -1)
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
					if (g_evolve_handle == NULL)
					{
						g_evolve_handle = malloc(sizeof(resume_e_info));
					}
					resume_e_info.parameter_range_x     = g_parameter_range_x;
					resume_e_info.parameter_range_y     = g_parameter_range_y;
					resume_e_info.opx             = g_parameter_offset_x;
					resume_e_info.opy             = g_parameter_offset_y;
					resume_e_info.odpx            = (short) g_discrete_parameter_offset_x;
					resume_e_info.odpy            = (short) g_discrete_parameter_offset_y;
					resume_e_info.px              = (short) g_px;
					resume_e_info.py              = (short) g_py;
					resume_e_info.sxoffs          = (short) g_sx_offset;
					resume_e_info.syoffs          = (short) g_sy_offset;
					resume_e_info.x_dots           = (short) g_x_dots;
					resume_e_info.y_dots           = (short) g_y_dots;
					resume_e_info.gridsz          = (short) g_grid_size;
					resume_e_info.this_generation_random_seed  = (short) g_this_generation_random_seed;
					resume_e_info.fiddle_factor = g_fiddle_factor;
					resume_e_info.evolving        = (short) g_evolving;
					resume_e_info.ecount          = (short) ecount;
					memcpy(g_evolve_handle, &resume_e_info, sizeof(resume_e_info));
				}
				g_sx_offset = g_sy_offset = 0;
				g_x_dots = g_screen_width;
				g_y_dots = g_screen_height; /* otherwise save only saves a sub image and boxes get clipped */

				/* set up for 1st selected image, this reuses px and py */
				g_px = g_py = g_grid_size/2;
				unspiral_map(); /* first time called, w/above line sets up array */
				restore_parameter_history();
				fiddle_parameters(g_genes, 0);
			}
			/* end of evolution loop */
			else
			{
				i = calculate_fractal();       /* draw the fractal using "C" */
				if (i == 0)
				{
					driver_buzzer(BUZZER_COMPLETE); /* finished!! */
				}
			}

			g_save_ticks = 0;                 /* turn off autosave timer */
			if (driver_diskp() && i == 0) /* disk-video */
			{
				disk_video_status(0, "Image has been completed");
			}
		}
#ifndef XFRACT
		g_box_count = 0;                     /* no zoom box yet  */
		g_z_width = 0;
#else
		if (!XZoomWaiting)
		{
			g_box_count = 0;                 /* no zoom box yet  */
			g_z_width = 0;
		}
#endif

		if (g_fractal_type == FRACTYPE_PLASMA)
		{
			g_cycle_limit = 256;              /* plasma clouds need quick spins */
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
			if (g_timed_save != 0)
			{
				if (g_timed_save == 1)
				{       /* woke up for timed save */
					driver_get_key();     /* eat the dummy char */
					kbdchar = 's'; /* do the save */
					g_resave_flag = RESAVE_YES;
					g_timed_save = 2;
				}
				else
				{                      /* save done, resume */
					g_timed_save = 0;
					g_resave_flag = RESAVE_DONE;
					kbdchar = FIK_ENTER;
				}
			}
			else if (g_initialize_batch == INITBATCH_NONE)      /* not batch mode */
			{
				driver_set_mouse_mode((g_z_width == 0) ? -FIK_PAGE_UP : LOOK_MOUSE_ZOOM_BOX);
				if (g_calculation_status == CALCSTAT_RESUMABLE && g_z_width == 0 && !driver_key_pressed())
				{
					kbdchar = FIK_ENTER;  /* no visible reason to stop, continue */
				}
				else      /* wait for a real keystroke */
				{
					if (g_auto_browse && !g_no_sub_images)
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
						if (kbdchar == FIK_ESC && g_escape_exit_flag != 0)
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
							check_video_mode_key(0, kbdchar) >= 0)
						{
							driver_discard_screen();
						}
						else if (kbdchar == 'x' || kbdchar == 'y' ||
								kbdchar == 'z' || kbdchar == 'g' ||
								kbdchar == 'v' || kbdchar == 2 ||
								kbdchar == 5 || kbdchar == 6)
						{
							g_from_text_flag = 1;
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
				/* g_initialize_batch == -1  flag to finish calc before save */
				/* g_initialize_batch == 0   not in batch mode */
				/* g_initialize_batch == 1   normal batch mode */
				/* g_initialize_batch == 2   was 1, now do a save */
				/* g_initialize_batch == 3   bailout with errorlevel == 2, error occurred, no save */
				/* g_initialize_batch == 4   bailout with errorlevel == 1, interrupted, try to save */
				/* g_initialize_batch == 5   was 4, now do a save */

				if (g_initialize_batch == INITBATCH_FINISH_CALC)       /* finish calc */
				{
					kbdchar = FIK_ENTER;
					g_initialize_batch = INITBATCH_NORMAL;
				}
				else if (g_initialize_batch == INITBATCH_NORMAL || g_initialize_batch == INITBATCH_BAILOUT_INTERRUPTED) /* save-to-disk */
				{
					kbdchar = (DEBUGFLAG_COMPARE_RESTORED == g_debug_flag) ? 'r' : 's';
					if (g_initialize_batch == INITBATCH_NORMAL)
					{
						g_initialize_batch = INITBATCH_SAVE;
					}
					if (g_initialize_batch == INITBATCH_BAILOUT_INTERRUPTED)
					{
						g_initialize_batch = INITBATCH_BAILOUT_SAVE;
					}
				}
				else
				{
					if (g_calculation_status != CALCSTAT_COMPLETED)
					{
						g_initialize_batch = INITBATCH_BAILOUT_ERROR; /* bailout with error */
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
			mms_value = g_evolving ?
				evolver_menu_switch(&kbdchar, &frommandel, kbdmore, stacked)
				: main_menu_switch(&kbdchar, &frommandel, kbdmore, stacked, axmode);
			if (g_quick_calculate && (mms_value == IMAGESTART ||
				mms_value == RESTORESTART ||
				mms_value == RESTART))
			{
				g_quick_calculate = FALSE;
				g_user_standard_calculation_mode = g_standard_calculation_mode_old;
			}
			if (g_quick_calculate && g_calculation_status != CALCSTAT_COMPLETED)
			{
				g_user_standard_calculation_mode = '1';
			}
			switch (mms_value)
			{
			case IMAGESTART:	return IMAGESTART;
			case RESTORESTART:	return RESTORESTART;
			case RESTART:		return RESTART;
			case CONTINUE:		continue;
			default:			break;
			}
			if (g_zoom_off == TRUE && *kbdmore == 1) /* draw/clear a zoom box? */
			{
				zoom_box_draw(1);
			}
			if (driver_resize())
			{
				g_calculation_status = CALCSTAT_NO_FRACTAL;
			}
		}
	}
}

static int look(int *stacked)
{
	switch (look_get_window())
	{
	case FIK_ENTER:
	case FIK_ENTER_2:
		g_show_file = 0;       /* trigger load */
		g_browsing = TRUE;    /* but don't ask for the file name as it's
							* just been selected */
		if (g_name_stack_ptr == 15)
		{					/* about to run off the end of the file
							* history stack so shift it all back one to
							* make room, lose the 1st one */
			int tmp;
			for (tmp = 1; tmp < 16; tmp++)
			{
				strcpy(g_file_name_stack[tmp - 1], g_file_name_stack[tmp]);
			}
			g_name_stack_ptr = 14;
		}
		g_name_stack_ptr++;
		strcpy(g_file_name_stack[g_name_stack_ptr], g_browse_name);
		merge_path_names(g_read_name, g_browse_name, 2);
		if (g_ui_state.ask_video)
		{
				driver_stack_screen();   /* save graphics image */
				*stacked = 1;
		}
		return 1;       /* hop off and do it!! */

	case '\\':
		if (g_name_stack_ptr >= 1)
		{
			/* go back one file if somewhere to go (ie. browsing) */
			g_name_stack_ptr--;
			while (g_file_name_stack[g_name_stack_ptr][0] == '\0'
					&& g_name_stack_ptr >= 0)
			{
				g_name_stack_ptr--;
			}
			if (g_name_stack_ptr < 0) /* oops, must have deleted first one */
			{
				break;
			}
			strcpy(g_browse_name, g_file_name_stack[g_name_stack_ptr]);
			merge_path_names(g_read_name, g_browse_name, 2);
			g_browsing = TRUE;
			g_show_file = 0;
			if (g_ui_state.ask_video)
			{
				driver_stack_screen(); /* save graphics image */
				*stacked = 1;
			}
			return 1;
		}                   /* otherwise fall through and turn off
							* browsing */
	case FIK_ESC:
	case 'l':              /* turn it off */
	case 'L':
		g_browsing = FALSE;
		break;

	case 's':
		g_browsing = FALSE;
		save_to_disk(g_save_name);
		break;

	default:               /* or no files found, leave the state of browsing alone */
		break;
	}

	return 0;
}

static int handle_fractal_type(int *frommandel)
{
	int i;

	g_julibrot = FALSE;
	clear_zoom_box();
	driver_stack_screen();
	i = get_fractal_type();
	if (i >= 0)
	{
		driver_discard_screen();
		g_save_dac = SAVEDAC_NO;
		g_save_release = g_release;
		g_no_magnitude_calculation = FALSE;
		g_use_old_periodicity = FALSE;
		g_bad_outside = false;
		g_use_old_complex_power = true;
		set_current_parameters();
		g_discrete_parameter_offset_x = g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_x = g_new_discrete_parameter_offset_y = 0;
		g_fiddle_factor = 1;           /* reset param evolution stuff */
		g_set_orbit_corners = 0;
		save_parameter_history();
		if (i == 0)
		{
			g_init_mode = g_adapter;
			*frommandel = 0;
		}
		else if (g_init_mode < 0) /* it is supposed to be... */
		{
			driver_set_for_text();     /* reset to text mode      */
		}
		return IMAGESTART;
	}
	driver_unstack_screen();
	return 0;
}

static void handle_options(int kbdchar, int *kbdmore, long *old_maxit)
{
	int i;
	*old_maxit = g_max_iteration;
	clear_zoom_box();
	if (g_from_text_flag == 1)
	{
		g_from_text_flag = 0;
	}
	else
	{
		driver_stack_screen();
	}
	switch (kbdchar)
	{
	case 'x':		i = get_toggles();			break;
	case 'y':		i = get_toggles2();			break;
	case 'p':		i = passes_options();		break;
	case 'z':		i = get_fractal_parameters(1);	break;
	case 'v':		i = get_view_params();		break;
	case FIK_CTL_B:	i = get_browse_parameters();	break;

	case FIK_CTL_E:
		i = get_evolve_parameters();
		if (i > 0)
		{
			g_start_show_orbit = 0;
			g_sound_state.silence_xyz();
			g_log_automatic_flag = FALSE; /* turn it off */
		}
		break;

	case FIK_CTL_F:
		i = g_sound_state.get_parameters();
		break;

	default:
		i = get_command_string();
		break;
	}
	driver_unstack_screen();
	if (g_evolving && g_true_color)
	{
		g_true_color = 0; /* truecolor doesn't play well with the evolver */
	}
	if (g_max_iteration > *old_maxit
		&& g_inside >= 0
		&& g_calculation_status == CALCSTAT_COMPLETED
		&& g_current_fractal_specific->calculate_type == standard_fractal
		&& !g_log_palette_flag
		&& !g_true_color /* recalc not yet implemented with truecolor */
		&& !(g_user_standard_calculation_mode == 't' && g_fill_color > -1) /* tesseral with fill doesn't work */
		&& !(g_user_standard_calculation_mode == 'o')
		&& i == Command::FractalParameter /* nothing else changed */
		&& g_outside != ATAN)
	{
		g_quick_calculate = TRUE;
		g_standard_calculation_mode_old = g_user_standard_calculation_mode;
		g_user_standard_calculation_mode = '1';
		*kbdmore = 0;
		g_calculation_status = CALCSTAT_RESUMABLE;
		i = 0;
	}
	else if (i > 0)
	{              /* time to redraw? */
		g_quick_calculate = FALSE;
		save_parameter_history();
		*kbdmore = 0;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
}

static void handle_evolver_options(int kbdchar, int *kbdmore)
{
	int i;
	clear_zoom_box();
	if (g_from_text_flag == 1)
	{
		g_from_text_flag = 0;
	}
	else
	{
		driver_stack_screen();
	}
	switch (kbdchar)
	{
	case 'x': i = get_toggles(); break;
	case 'y': i = get_toggles2(); break;
	case 'p': i = passes_options(); break;
	case 'z': i = get_fractal_parameters(1); break;

	case FIK_CTL_E:
	case FIK_SPACE:
		i = get_evolve_parameters();
		break;

	default:
		i = get_command_string();
		break;
	}
	driver_unstack_screen();
	if (g_evolving && g_true_color)
	{
		g_true_color = 0; /* truecolor doesn't play well with the evolver */
	}
	if (i > Command::OK)              /* time to redraw? */
	{
		save_parameter_history();
		*kbdmore = 0;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
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
		i |= Command::FractalParameter;
		g_save_dac = SAVEDAC_NO;
	}
	else if (g_color_preloaded)
	{                         /* g_colors= was specified */
		spindac(0, 1);
		g_color_preloaded = FALSE;
	}
	else if (i & Command::Reset)         /* reset was specified */
	{
		g_save_dac = SAVEDAC_NO;
	}
	if (i & Command::ThreeDYes)
	{                         /* 3d = was specified */
		*kbdchar = '3';
		driver_unstack_screen();
		return TRUE;
	}
	if (i & Command::FractalParameter)
	{                         /* fractal parameter changed */
		driver_discard_screen();
		*kbdmore = 0;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
	else
	{
		driver_unstack_screen();
	}

	return FALSE;
}

static int handle_toggle_float()
{
	if (g_user_float_flag == 0)
	{
		g_user_float_flag = 1;
	}
	else if (g_standard_calculation_mode != 'o') /* don't go there */
	{
		g_user_float_flag = 0;
	}
	g_init_mode = g_adapter;
	return IMAGESTART;
}

static int handle_ant()
{
	int oldtype, err, i;
	double oldparm[MAX_PARAMETERS];

	clear_zoom_box();
	oldtype = g_fractal_type;
	for (i = 0; i < MAX_PARAMETERS; i++)
	{
		oldparm[i] = g_parameters[i];
	}
	if (g_fractal_type != FRACTYPE_ANT)
	{
		g_fractal_type = FRACTYPE_ANT;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		load_parameters(g_fractal_type);
	}
	if (!g_from_text_flag)
	{
		driver_stack_screen();
	}
	g_from_text_flag = 0;
	err = get_fractal_parameters(2);
	if (err >= 0)
	{
		driver_unstack_screen();
		if (ant() >= 0)
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
	else
	{
		driver_unstack_screen();
	}
	g_fractal_type = oldtype;
	for (i = 0; i < MAX_PARAMETERS; i++)
	{
		g_parameters[i] = oldparm[i];
	}
	return (err >= 0) ? CONTINUE : 0;
}

static int handle_recalc(int (*continue_check)(), int (*recalc_check)())
{
#if defined(_WIN32)
	_ASSERTE(continue_check && recalc_check);
#endif
	clear_zoom_box();
	if ((*continue_check)() >= 0)
	{
		if ((*recalc_check)() >= 0)
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
		return CONTINUE;
	}
	return 0;
}

static void handle_3d_params(int *kbdmore)
{
	if (get_fractal_3d_parameters() >= 0)    /* get the parameters */
	{
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;    /* time to redraw */
	}
}

static void handle_orbits()
{
	/* must use standard fractal and have a float variant */
	if ((g_fractal_specific[g_fractal_type].calculate_type == standard_fractal
			|| g_fractal_specific[g_fractal_type].calculate_type == froth_calc)
		&& (g_fractal_specific[g_fractal_type].isinteger == FALSE
			|| g_fractal_specific[g_fractal_type].tofloat != FRACTYPE_NO_FRACTAL)
		&& !g_bf_math /* for now no arbitrary precision support */
		&& !(g_is_true_color && g_true_mode))
	{
		clear_zoom_box();
		Jiim(ORBIT);
	}
}

static void handle_mandelbrot_julia_toggle(int *kbdmore, int *frommandel)
{
	static double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
	static double  jxx3rd, jyy3rd;

	if (g_bf_math || g_evolving)
	{
		return;
	}
	if (g_fractal_type == FRACTYPE_CELLULAR)
	{
		g_next_screen_flag = !g_next_screen_flag;
		g_calculation_status = CALCSTAT_RESUMABLE;
		*kbdmore = 0;
		return;
	}

	if (g_fractal_type == FRACTYPE_FORMULA || g_fractal_type == FRACTYPE_FORMULA_FP)
	{
		if (g_is_mand)
		{
			g_fractal_specific[g_fractal_type].tojulia = g_fractal_type;
			g_fractal_specific[g_fractal_type].tomandel = FRACTYPE_NO_FRACTAL;
			g_is_mand = false;
		}
		else
		{
			g_fractal_specific[g_fractal_type].tojulia = FRACTYPE_NO_FRACTAL;
			g_fractal_specific[g_fractal_type].tomandel = g_fractal_type;
			g_is_mand = true;
		}
	}

	if (g_current_fractal_specific->tojulia != FRACTYPE_NO_FRACTAL
		&& g_parameters[0] == 0.0
		&& g_parameters[1] == 0.0)
	{
		/* switch to corresponding Julia set */
		int key;
		g_has_inverse = (g_fractal_type == FRACTYPE_MANDELBROT || g_fractal_type == FRACTYPE_MANDELBROT_FP)
			&& (g_bf_math == 0) ? TRUE : FALSE;
		clear_zoom_box();
		Jiim(JIIM);
		key = driver_get_key();    /* flush keyboard buffer */
		if (key != FIK_SPACE)
		{
			driver_unget_key(key);
			return;
		}
		g_fractal_type = g_current_fractal_specific->tojulia;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		if (g_julia_c_x == BIG || g_julia_c_y == BIG)
		{
			g_parameters[0] = g_escape_time_state.m_grid_fp.x_center();
			g_parameters[1] = g_escape_time_state.m_grid_fp.y_center();
		}
		else
		{
			g_parameters[0] = g_julia_c_x;
			g_parameters[1] = g_julia_c_y;
			g_julia_c_x = g_julia_c_y = BIG;
		}
		jxxmin = g_sx_min;
		jxxmax = g_sx_max;
		jyymax = g_sy_max;
		jyymin = g_sy_min;
		jxx3rd = g_sx_3rd;
		jyy3rd = g_sy_3rd;
		*frommandel = 1;
		g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
		g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
		g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
		g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
		g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
		g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
		if (g_user_distance_test == 0
			&& g_user_biomorph != -1
			&& g_bit_shift != 29)
		{
			g_escape_time_state.m_grid_fp.x_min() *= 3.0;
			g_escape_time_state.m_grid_fp.x_max() *= 3.0;
			g_escape_time_state.m_grid_fp.y_min() *= 3.0;
			g_escape_time_state.m_grid_fp.y_max() *= 3.0;
			g_escape_time_state.m_grid_fp.x_3rd() *= 3.0;
			g_escape_time_state.m_grid_fp.y_3rd() *= 3.0;
		}
		g_zoom_off = TRUE;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
	}
	else if (g_current_fractal_specific->tomandel != FRACTYPE_NO_FRACTAL)
	{
		/* switch to corresponding Mandel set */
		g_fractal_type = g_current_fractal_specific->tomandel;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		if (*frommandel)
		{
			g_escape_time_state.m_grid_fp.x_min() = jxxmin;
			g_escape_time_state.m_grid_fp.x_max() = jxxmax;
			g_escape_time_state.m_grid_fp.y_min() = jyymin;
			g_escape_time_state.m_grid_fp.y_max() = jyymax;
			g_escape_time_state.m_grid_fp.x_3rd() = jxx3rd;
			g_escape_time_state.m_grid_fp.y_3rd() = jyy3rd;
		}
		else
		{
			g_escape_time_state.m_grid_fp.x_min() = g_escape_time_state.m_grid_fp.x_3rd() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
			g_escape_time_state.m_grid_fp.y_min() = g_escape_time_state.m_grid_fp.y_3rd() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
		}
		g_save_c.x = g_parameters[0];
		g_save_c.y = g_parameters[1];
		g_parameters[0] = 0;
		g_parameters[1] = 0;
		g_zoom_off = TRUE;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
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
	if (g_fractal_type == FRACTYPE_JULIA || g_fractal_type == FRACTYPE_JULIA_FP || g_fractal_type == FRACTYPE_INVERSE_JULIA)
	{
		static int oldtype = -1;
		if (g_fractal_type == FRACTYPE_JULIA || g_fractal_type == FRACTYPE_JULIA_FP)
		{
			oldtype = g_fractal_type;
			g_fractal_type = FRACTYPE_INVERSE_JULIA;
		}
		else if (g_fractal_type == FRACTYPE_INVERSE_JULIA)
		{
			g_fractal_type = (oldtype != -1) ? oldtype : FRACTYPE_JULIA;
		}
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		g_zoom_off = TRUE;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
	}
	else
	{
		driver_buzzer(BUZZER_ERROR);
	}
}

static int handle_history(int *stacked, int kbdchar)
{
	if (g_name_stack_ptr >= 1)
	{
		/* go back one file if somewhere to go (ie. browsing) */
		g_name_stack_ptr--;
		while (g_file_name_stack[g_name_stack_ptr][0] == '\0'
			&& g_name_stack_ptr >= 0)
		{
			g_name_stack_ptr--;
		}
		if (g_name_stack_ptr < 0) /* oops, must have deleted first one */
		{
			return 0;
		}
		strcpy(g_browse_name, g_file_name_stack[g_name_stack_ptr]);
		/*
		split_path(g_browse_name, NULL, NULL, fname, ext);
		split_path(g_read_name, drive, dir, NULL, NULL);
		make_path(g_read_name, drive, dir, fname, ext);
		*/
		merge_path_names(g_read_name, g_browse_name, 2);
		g_browsing = TRUE;
		g_no_sub_images = FALSE;
		g_show_file = 0;
		if (g_ui_state.ask_video)
		{
			driver_stack_screen();      /* save graphics image */
			*stacked = 1;
		}
		return RESTORESTART;
	}
	else if (g_max_history > 0 && g_bf_math == 0)
	{
		if (kbdchar == '\\' || kbdchar == 'h')
		{
			history_back();
		}
		else if (kbdchar == FIK_CTL_BACKSLASH || kbdchar == FIK_BACKSPACE)
		{
			history_forward();
		}
		history_restore_info();
		g_zoom_off = TRUE;
		g_init_mode = g_adapter;
		if (g_current_fractal_specific->isinteger != 0
			&& g_current_fractal_specific->tofloat != FRACTYPE_NO_FRACTAL)
		{
			g_user_float_flag = 0;
		}
		if (g_current_fractal_specific->isinteger == 0
			&& g_current_fractal_specific->tofloat != FRACTYPE_NO_FRACTAL)
		{
			g_user_float_flag = 1;
		}
		return IMAGESTART;
	}

	return 0;
}

static int handle_color_cycling(int kbdchar)
{
	clear_zoom_box();
	memcpy(g_old_dac_box, g_dac_box, 256*3);
	rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
	if (memcmp(g_old_dac_box, g_dac_box, 256*3))
	{
		g_color_state = COLORSTATE_UNKNOWN;
		history_save_info();
	}
	return CONTINUE;
}

static int handle_color_editing(int *kbdmore)
{
	if (g_is_true_color && !g_initialize_batch) /* don't enter palette editor */
	{
		if (load_palette() >= 0)
		{
			*kbdmore = 0;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
			return 0;
		}
		else
		{
			return CONTINUE;
		}
	}
	clear_zoom_box();
	if (g_dac_box[0][0] != 255
		&& g_colors >= 16
		&& !driver_diskp())
	{
		memcpy(g_old_dac_box, g_dac_box, 256*3);
		palette_edit();
		if (memcmp(g_old_dac_box, g_dac_box, 256*3))
		{
			g_color_state = COLORSTATE_UNKNOWN;
			history_save_info();
		}
	}
	return CONTINUE;
}

static int handle_save_to_disk()
{
	if (driver_diskp() && g_disk_targa)
	{
		return CONTINUE;  /* disk video and targa, nothing to save */
	}
	note_zoom();
	save_to_disk(g_save_name);
	restore_zoom();
	return CONTINUE;
}

static int handle_evolver_save_to_disk()
{
	int oldsxoffs, oldsyoffs, oldxdots, oldydots, oldpx, oldpy;

	if (driver_diskp() && g_disk_targa)
	{
		return CONTINUE;  /* disk video and targa, nothing to save */
	}

	oldsxoffs = g_sx_offset;
	oldsyoffs = g_sy_offset;
	oldxdots = g_x_dots;
	oldydots = g_y_dots;
	oldpx = g_px;
	oldpy = g_py;
	g_sx_offset = g_sy_offset = 0;
	g_x_dots = g_screen_width;
	g_y_dots = g_screen_height; /* for full screen save and pointer move stuff */
	g_px = g_py = g_grid_size / 2;
	restore_parameter_history();
	fiddle_parameters(g_genes, 0);
	draw_parameter_box(1);
	save_to_disk(g_save_name);
	g_px = oldpx;
	g_py = oldpy;
	restore_parameter_history();
	fiddle_parameters(g_genes, unspiral_map());
	g_sx_offset = oldsxoffs;
	g_sy_offset = oldsyoffs;
	g_x_dots = oldxdots;
	g_y_dots = oldydots;
	return CONTINUE;
}

static int handle_restore_from(int *frommandel, int kbdchar, int *stacked)
{
	g_compare_gif = 0;
	*frommandel = 0;
	if (g_browsing)
	{
		g_browsing = FALSE;
	}
	if (kbdchar == 'r')
	{
		if (DEBUGFLAG_COMPARE_RESTORED == g_debug_flag)
		{
			g_compare_gif = g_overlay_3d = 1;
			if (g_initialize_batch == INITBATCH_SAVE)
			{
				driver_stack_screen();   /* save graphics image */
				strcpy(g_read_name, g_save_name);
				g_show_file = 0;
				return RESTORESTART;
			}
		}
		else
		{
			g_compare_gif = g_overlay_3d = 0;
		}
		g_display_3d = DISPLAY3D_NONE;
	}
	driver_stack_screen();            /* save graphics image */
	*stacked = g_overlay_3d ? 0 : 1;
	if (g_resave_flag)
	{
		update_save_name(g_save_name);      /* do the pending increment */
		g_resave_flag = RESAVE_NO;
		g_started_resaves = FALSE;
	}
	g_show_file = -1;
	return RESTORESTART;
}

static int handle_look_for_files(int *stacked)
{
	if ((g_z_width != 0) || driver_diskp())
	{
		g_browsing = FALSE;
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
	if (g_z_width != 0.0)
	{                         /* do a zoom */
		init_pan_or_recalc(0);
		*kbdmore = 0;
	}
	if (g_calculation_status != CALCSTAT_COMPLETED)     /* don't restart if image complete */
	{
		*kbdmore = 0;
	}
}

static void handle_zoom_out(int *kbdmore)
{
	if (g_z_width != 0.0)
	{
		init_pan_or_recalc(1);
		*kbdmore = 0;
		zoom_box_out();                /* calc corners for zooming out */
	}
}

static void handle_zoom_skew(int negative)
{
	if (negative)
	{
		if (g_box_count && (g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM_BOX_ROTATE) == 0)
		{
			int i = key_count(FIK_CTL_HOME);
			g_z_skew -= 0.02*i;
			if (g_z_skew < -0.48)
			{
				g_z_skew = -0.48;
			}
		}
	}
	else
	{
		if (g_box_count && (g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM_BOX_ROTATE) == 0)
		{
			int i = key_count(FIK_CTL_END);
			g_z_skew += 0.02*i;
			if (g_z_skew > 0.48)
			{
				g_z_skew = 0.48;
			}
		}
	}
}

static void handle_select_video(int *kbdchar)
{
	driver_stack_screen();
	*kbdchar = select_video_mode(g_adapter);
	if (check_video_mode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
	{
		driver_discard_screen();
	}
	else
	{
		driver_unstack_screen();
	}
}

static void handle_mutation_level(int forward, int amount, int *kbdmore)
{
	g_view_window = TRUE;
	g_evolving = EVOLVE_FIELD_MAP;
	set_mutation_level(amount);
	if (forward)
	{
		restore_parameter_history();
	}
	else
	{
		save_parameter_history();
	}
	*kbdmore = 0;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static int handle_video_mode(int kbdchar, int *kbdmore)
{
	int k = check_video_mode_key(0, kbdchar);
	if (k >= 0)
	{
		g_adapter = k;
		if (g_video_table[g_adapter].colors != g_colors)
		{
			g_save_dac = SAVEDAC_NO;
		}
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		*kbdmore = 0;
		return CONTINUE;
	}
	return 0;
}

static void handle_z_rotate(int increase)
{
	if (g_box_count && (g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM_BOX_ROTATE) == 0)
	{
		if (increase)
		{
			g_z_rotate += key_count(FIK_CTL_MINUS);
		}
		else
		{
			g_z_rotate -= key_count(FIK_CTL_PLUS);
		}
	}
}

static void handle_box_color(int increase)
{
	if (increase)
	{
		g_box_color += key_count(FIK_CTL_INSERT);
	}
	else
	{
		g_box_color -= key_count(FIK_CTL_DEL);
	}
}

static void handle_zoom_resize(int zoom_in)
{
	if (zoom_in)
	{
		if (g_zoom_off == TRUE)
		{
			if (g_z_width == 0)
			{                      /* start zoombox */
				g_z_width = g_z_depth = 1.0;
				g_z_skew = 0.0;
				g_z_rotate = 0;
				g_zbx = g_zby = 0.0;
				find_special_colors();
				g_box_color = g_color_bright;
				g_px = g_py = g_grid_size/2;
				zoom_box_move(0.0, 0.0); /* force scrolling */
			}
			else
			{
				zoom_box_resize(-key_count(FIK_PAGE_UP));
			}
		}
	}
	else
	{
		/* zoom out */
		if (g_box_count)
		{
			if (g_z_width >= 0.999 && g_z_depth >= 0.999) /* end zoombox */
			{
				g_z_width = 0.0;
			}
			else
			{
				zoom_box_resize(key_count(FIK_PAGE_DOWN));
			}
		}
	}
}

static void handle_zoom_stretch(int narrower)
{
	if (g_box_count)
	{
		zoom_box_change_i(0, narrower ?
			-2*key_count(FIK_CTL_PAGE_UP) : 2*key_count(FIK_CTL_PAGE_DOWN));
	}
}

static int handle_restart()
{
	driver_set_for_text();           /* force text mode */
	return RESTART;
}

int main_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, int *stacked, int axmode)
{
	long old_maxit;

	if (g_quick_calculate)
	{
		if (CALCSTAT_COMPLETED == g_calculation_status)
		{
			g_quick_calculate = FALSE;
		}
		g_user_standard_calculation_mode = g_standard_calculation_mode_old;
	}
	switch (*kbdchar)
	{
	case 't':                    /* new fractal type             */
		return handle_fractal_type(frommandel);

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
		return handle_ant();

	case 'k':                    /* ^s is irritating, give user a single key */
	case FIK_CTL_S:                     /* ^s RDS */
		return handle_recalc(get_random_dot_stereogram_parameters, auto_stereo);

	case 'a':                    /* starfield parms               */
		return handle_recalc(get_starfield_params, starfield);

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
		return handle_history(stacked, *kbdchar);

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
		return handle_color_editing(kbdmore);

	case 's':                    /* save-to-disk                 */
		return handle_save_to_disk();

	case '#':                    /* 3D overlay                   */
#ifdef XFRACT
	case FIK_F3:                     /* 3D overlay                   */
#endif
		clear_zoom_box();
		g_overlay_3d = 1;
		/* fall through */

do_3d_transform:
	case '3':                    /* restore-from (3d)            */
		g_display_3d = g_overlay_3d ? DISPLAY3D_OVERLAY : DISPLAY3D_YES; /* for <b> command               */
		/* fall through */

	case 'r':                    /* restore-from                 */
		return handle_restore_from(frommandel, *kbdchar, stacked);

	case 'l':
	case 'L':                    /* Look for other files within this view */
		return handle_look_for_files(stacked);
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
		return handle_restart();

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
		handle_mutation_level(FALSE, *kbdchar - FIK_ALT_1 + 1, kbdmore);
		break;

	case FIK_DELETE:         /* select video mode from list */
		handle_select_video(kbdchar);
		/* fall through */

	default:                     /* other (maybe a valid Fn key) */
		return handle_video_mode(*kbdchar, kbdmore);
	}                            /* end of the big switch */

	return 0;
}

static void handle_evolver_exit(int *kbdmore)
{
	g_evolving = EVOLVE_NONE;
	g_view_window = FALSE;
	save_parameter_history();
	*kbdmore = 0;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static int handle_evolver_history(int *stacked, int *kbdchar)
{
	if (g_max_history > 0 && g_bf_math == 0)
	{
		if (*kbdchar == '\\' || *kbdchar == 'h')
		{
			history_back();
		}
		else if (*kbdchar == FIK_CTL_BACKSLASH || *kbdchar == FIK_BACKSPACE)
		{
			history_forward();
		}
		history_restore_info();
		g_zoom_off = TRUE;
		g_init_mode = g_adapter;
		if (g_current_fractal_specific->isinteger != 0
			&& g_current_fractal_specific->tofloat != FRACTYPE_NO_FRACTAL)
		{
			g_user_float_flag = 0;
		}
		if (g_current_fractal_specific->isinteger == 0
			&& g_current_fractal_specific->tofloat != FRACTYPE_NO_FRACTAL)
		{
			g_user_float_flag = 1;
		}
		return IMAGESTART;
	}
	return 0;
}

static void handle_evolver_move_selection(int *kbdchar)
{
	/* borrow ctrl cursor keys for moving selection box */
	/* in evolver mode */
	if (g_box_count)
	{
		int grout;
		if (g_evolving & EVOLVE_FIELD_MAP)
		{
			if (*kbdchar == FIK_CTL_LEFT_ARROW)
			{
				g_px--;
			}
			if (*kbdchar == FIK_CTL_RIGHT_ARROW)
			{
				g_px++;
			}
			if (*kbdchar == FIK_CTL_UP_ARROW)
			{
				g_py--;
			}
			if (*kbdchar == FIK_CTL_DOWN_ARROW)
			{
				g_py++;
			}
			if (g_px < 0)
			{
				g_px = g_grid_size-1;
			}
			if (g_px > (g_grid_size-1))
			{
				g_px = 0;
			}
			if (g_py < 0)
			{
				g_py = g_grid_size-1;
			}
			if (g_py > (g_grid_size-1))
			{
				g_py = 0;
			}
			grout = !((g_evolving & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
			g_sx_offset = g_px*(int)(g_dx_size + 1 + grout);
			g_sy_offset = g_py*(int)(g_dy_size + 1 + grout);

			restore_parameter_history();
			fiddle_parameters(g_genes, unspiral_map()); /* change all parameters */
						/* to values appropriate to the image selected */
			set_evolve_ranges();
			zoom_box_change_i(0, 0);
			draw_parameter_box(0);
		}
	}
	else                       /* if no zoombox, scroll by arrows */
	{
		move_zoombox(*kbdchar);
	}
}

static void handle_evolver_param_zoom(int zoom_out)
{
	if (g_parameter_box_count)
	{
		if (zoom_out)
		{
			g_parameter_zoom -= 1.0;
			if (g_parameter_zoom < 1.0)
			{
				g_parameter_zoom = 1.0;
			}
			draw_parameter_box(0);
			set_evolve_ranges();
		}
		else
		{
			g_parameter_zoom += 1.0;
			if (g_parameter_zoom > (double) g_grid_size/2.0)
			{
				g_parameter_zoom = (double) g_grid_size/2.0;
			}
			draw_parameter_box(0);
			set_evolve_ranges();
		}
	}
}

static void handle_evolver_zoom(int zoom_in)
{
	if (zoom_in)
	{
		if (g_zoom_off == TRUE)
		{
			if (g_z_width == 0)
			{                      /* start zoombox */
				g_z_width = g_z_depth = 1;
				g_z_skew = g_z_rotate = 0;
				g_zbx = g_zby = 0;
				find_special_colors();
				g_box_color = g_color_bright;
				if (g_evolving & EVOLVE_FIELD_MAP) /*rb*/
				{
					/* set screen view params back (previously changed to allow
					   full screen saves in g_view_window mode) */
					int grout = !((g_evolving & EVOLVE_NO_GROUT) / EVOLVE_NO_GROUT);
					g_sx_offset = g_px*(int) (g_dx_size + 1 + grout);
					g_sy_offset = g_py*(int) (g_dy_size + 1 + grout);
					setup_parameter_box();
					draw_parameter_box(0);
				}
				zoom_box_move(0.0, 0.0); /* force scrolling */
			}
			else
			{
				zoom_box_resize(-key_count(FIK_PAGE_UP));
			}
		}
	}
	else
	{
		if (g_box_count)
		{
			if (g_z_width >= 0.999 && g_z_depth >= 0.999) /* end zoombox */
			{
				g_z_width = 0;
				if (g_evolving & EVOLVE_FIELD_MAP)
				{
					draw_parameter_box(1); /* clear boxes off screen */
					release_parameter_box();
				}
			}
			else
			{
				zoom_box_resize(key_count(FIK_PAGE_DOWN));
			}
		}
	}
}

static void handle_evolver_mutation(int halve, int *kbdmore)
{
	if (halve)
	{
		g_fiddle_factor /= 2;
		g_parameter_range_x /= 2;
		g_new_parameter_offset_x = g_parameter_offset_x + g_parameter_range_x / 2;
		g_parameter_range_y /= 2;
		g_new_parameter_offset_y = g_parameter_offset_y + g_parameter_range_y / 2;
	}
	else
	{
		double centerx, centery;
		g_fiddle_factor *= 2;
		centerx = g_parameter_offset_x + g_parameter_range_x / 2;
		g_parameter_range_x *= 2;
		g_new_parameter_offset_x = centerx - g_parameter_range_x / 2;
		centery = g_parameter_offset_y + g_parameter_range_y / 2;
		g_parameter_range_y *= 2;
		g_new_parameter_offset_y = centery - g_parameter_range_y / 2;
	}
	*kbdmore = 0;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static void handle_evolver_grid_size(int decrement, int *kbdmore)
{
	if (decrement)
	{
		if (g_grid_size > 3)
		{
			g_grid_size -= 2;  /* g_grid_size must have odd value only */
			*kbdmore = 0;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
	else
	{
		if (g_grid_size < g_screen_width/(MIN_PIXELS << 1))
		{
			g_grid_size += 2;
			*kbdmore = 0;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
}

static void handle_evolver_toggle(int *kbdmore)
{
	int i;
	for (i = 0; i < NUMGENES; i++)
	{
		if (g_genes[i].mutate == 5)
		{
			g_genes[i].mutate = 6;
			continue;
		}
		if (g_genes[i].mutate == 6)
		{
			g_genes[i].mutate = 5;
		}
	}
	*kbdmore = 0;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static void handle_mutation_off(int *kbdmore)
{
	g_evolving = EVOLVE_NONE;
	g_view_window = FALSE;
	*kbdmore = 0;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

int evolver_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, int *stacked)
{
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
		handle_evolver_options(*kbdchar, kbdmore);
		break;

	case 'b': /* quick exit from evolve mode */
		handle_evolver_exit(kbdmore);
		break;

	case 'f':                    /* floating pt toggle           */
		return handle_toggle_float();

	case '\\':                   /* return to prev image    */
	case FIK_CTL_BACKSLASH:
	case 'h':
	case FIK_BACKSPACE:
		return handle_evolver_history(stacked, kbdchar);

	case 'c':                    /* switch to color cycling      */
	case '+':                    /* rotate palette               */
	case '-':                    /* rotate palette               */
		return handle_color_cycling(*kbdchar);

	case 'e':                    /* switch to color editing      */
		return handle_color_editing(kbdmore);

	case 's':                    /* save-to-disk                 */
		return handle_evolver_save_to_disk();

	case 'r':                    /* restore-from                 */
		return handle_restore_from(frommandel, *kbdchar, stacked);

	case FIK_ENTER:                  /* Enter                        */
	case FIK_ENTER_2:                /* Numeric-Keypad Enter         */
		handle_zoom_in(kbdmore);
		break;

	case FIK_CTL_ENTER:              /* control-Enter                */
	case FIK_CTL_ENTER_2:            /* Control-Keypad Enter         */
		handle_zoom_out(kbdmore);
		break;

	case FIK_INSERT:         /* insert                       */
		return handle_restart();

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
		handle_evolver_move_selection(kbdchar);
		break;

	case FIK_CTL_HOME:               /* Ctrl-home                    */
	case FIK_CTL_END:                /* Ctrl-end                     */
		handle_zoom_skew(*kbdchar == FIK_CTL_HOME);
		break;

	case FIK_CTL_PAGE_UP:
	case FIK_CTL_PAGE_DOWN:
		handle_evolver_param_zoom(FIK_CTL_PAGE_UP == *kbdchar);
		break;

	case FIK_PAGE_UP:                /* page up                      */
	case FIK_PAGE_DOWN:              /* page down                    */
		handle_evolver_zoom(FIK_PAGE_UP == *kbdchar);
		break;

	case FIK_CTL_MINUS:              /* Ctrl-kpad-                  */
	case FIK_CTL_PLUS:               /* Ctrl-kpad+               */
		handle_z_rotate(FIK_CTL_MINUS == *kbdchar);
		break;

	case FIK_CTL_INSERT:             /* Ctrl-ins                 */
	case FIK_CTL_DEL:                /* Ctrl-del                 */
		handle_box_color(FIK_CTL_INSERT == *kbdchar);
		break;

	/* grabbed a couple of video mode keys, user can change to these using
		delete and the menu if necessary */

	case FIK_F2: /* halve mutation params and regen */
	case FIK_F3: /*double mutation parameters and regenerate */
		handle_evolver_mutation(FIK_F2 == *kbdchar, kbdmore);
		break;

	case FIK_F4: /*decrement  gridsize and regen */
	case FIK_F5: /* increment gridsize and regen */
		handle_evolver_grid_size(FIK_F4 == *kbdchar, kbdmore);
		break;

	case FIK_F6: /* toggle all variables selected for random variation to
				center weighted variation and vice versa */
		handle_evolver_toggle(kbdmore);
		break;

	case FIK_ALT_1: /* alt + number keys set mutation level */
	case FIK_ALT_2:
	case FIK_ALT_3:
	case FIK_ALT_4:
	case FIK_ALT_5:
	case FIK_ALT_6:
	case FIK_ALT_7:
		handle_mutation_level(TRUE, *kbdchar - FIK_ALT_1 + 1, kbdmore);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		handle_mutation_level(TRUE, *kbdchar - (int) '1' + 1, kbdmore);
		break;

	case '0': /* mutation level 0 == turn off evolving */
		handle_mutation_off(kbdmore);
		break;

	case FIK_DELETE:         /* select video mode from list */
		handle_select_video(kbdchar);
		/* fall through */

	default:             /* other (maybe valid Fn key */
		return handle_video_mode(*kbdchar, kbdmore);
	}                            /* end of the big evolver switch */

	return 0;
}

static void note_zoom()
{
	if (g_box_count)  /* save zoombox stuff in mem before encode (mem reused) */
	{
		savezoom = (char *)malloc((long)(5*g_box_count));
		if (savezoom == NULL)
		{
			clear_zoom_box(); /* not enuf mem so clear the box */
		}
		else
		{
			reset_zoom_corners(); /* reset these to overall image, not box */
			memcpy(savezoom, g_box_x, g_box_count*2);
			memcpy(savezoom + g_box_count*2, g_box_y, g_box_count*2);
			memcpy(savezoom + g_box_count*4, g_box_values, g_box_count);
		}
	}
}

static void restore_zoom()
{
	if (g_box_count)  /* restore zoombox arrays */
	{
		memcpy(g_box_x, savezoom, g_box_count*2);
		memcpy(g_box_y, savezoom + g_box_count*2, g_box_count*2);
		memcpy(g_box_values, savezoom + g_box_count*4, g_box_count);
		free(savezoom);
		zoom_box_draw(1); /* get the g_xx_min etc variables recalc'd by redisplaying */
		}
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{  int vertical, horizontal, getmore;
	vertical = horizontal = 0;
	getmore = 1;
	while (getmore)
	{
		switch (keynum)
		{
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
		if (getmore)
		{
			if (getmore == 2)              /* eat last key used */
			{
				driver_get_key();
			}
			getmore = 2;
			keynum = driver_key_pressed();         /* next pending key */
		}
	}
	if (g_box_count)
	{
		zoom_box_move((double)horizontal/g_dx_size, (double)vertical/g_dy_size);
	}
}

/* displays differences between current image file and new image */
static FILE *cmp_fp;
static int errcount;
int cmp_line(BYTE *pixels, int linelen)
{
	int row, col;
	int oldcolor;
	row = g_row_count++;
	if (row == 0)
	{
		errcount = 0;
		cmp_fp = dir_fopen(g_work_dir, "cmperr", g_initialize_batch ? "a" : "w");
		g_out_line_cleanup = cmp_line_cleanup;
		}
	if (g_potential_16bit)  /* 16 bit info, ignore odd numbered rows */
	{
		if ((row & 1) != 0)
		{
			return 0;
		}
		row >>= 1;
	}
	for (col = 0; col < linelen; col++)
	{
		oldcolor = getcolor(col, row);
		if (oldcolor == (int)pixels[col])
		{
			g_put_color(col, row, 0);
		}
		else
		{
			if (oldcolor == 0)
			{
				g_put_color(col, row, 1);
			}
			++errcount;
			if (g_initialize_batch == INITBATCH_NONE)
			{
				fprintf(cmp_fp, "#%5d col %3d row %3d old %3d new %3d\n",
					errcount, col, row, oldcolor, pixels[col]);
			}
		}
	}
	return 0;
}

static void cmp_line_cleanup()
{
	char *timestring;
	time_t ltime;
	if (g_initialize_batch)
	{
		time(&ltime);
		timestring = ctime(&ltime);
		timestring[24] = 0; /*clobber newline in time string */
		fprintf(cmp_fp, "%s compare to %s has %5d errs\n",
							timestring, g_read_name, errcount);
		}
	fclose(cmp_fp);
}

void clear_zoom_box()
{
	g_z_width = 0;
	zoom_box_draw(0);
	reset_zoom_corners();
}

void reset_zoom_corners()
{
	g_escape_time_state.m_grid_fp.x_min() = g_sx_min;
	g_escape_time_state.m_grid_fp.x_max() = g_sx_max;
	g_escape_time_state.m_grid_fp.x_3rd() = g_sx_3rd;
	g_escape_time_state.m_grid_fp.y_max() = g_sy_max;
	g_escape_time_state.m_grid_fp.y_min() = g_sy_min;
	g_escape_time_state.m_grid_fp.y_3rd() = g_sy_3rd;
	if (g_bf_math)
	{
		copy_bf(g_escape_time_state.m_grid_bf.x_min(), bfsxmin);
		copy_bf(g_escape_time_state.m_grid_bf.x_max(), bfsxmax);
		copy_bf(g_escape_time_state.m_grid_bf.y_min(), bfsymin);
		copy_bf(g_escape_time_state.m_grid_bf.y_max(), bfsymax);
		copy_bf(g_escape_time_state.m_grid_bf.x_3rd(), bfsx3rd);
		copy_bf(g_escape_time_state.m_grid_bf.y_3rd(), bfsy3rd);
	}
}

/* read keystrokes while = specified key, return 1 + count;       */
/* used to catch up when moving zoombox is slower than keyboard */
int key_count(int keynum)
{
	int ctr;
	ctr = 1;
	while (driver_key_pressed() == keynum)
	{
		driver_get_key();
		++ctr;
	}
	return ctr;
}
