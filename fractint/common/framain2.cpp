#include <fstream>
#include <string>

#include <string.h>
#include <time.h>
#include <ctype.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "ant.h"
#include "Browse.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "fihelp.h"
#include "filesystem.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "gifview.h"
#include "history.h"
#include "jiim.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscovl.h"
#include "miscfrac.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"
#include "zoom.h"
#include "ZoomBox.h"

#include "EscapeTime.h"
#include "CommandParser.h"
#include "FrothyBasin.h"
#include "SoundState.h"
#include "ViewWindow.h"

/* displays differences between current image file and new image */
class LineCompare
{
public:
	LineCompare() : _file(), _error_count(0)
	{
	}
	~LineCompare()
	{
	}

	int compare(BYTE *pixels, int line_length);
	void cleanup();

private:
	std::ofstream _file;
	int _error_count;
};

class ZoomSaver
{
public:
	ZoomSaver() : m_save_zoom(0)
	{
	}
	~ZoomSaver()
	{
	}

	void save();
	void restore();

private:
	int *m_save_zoom;
};

bool g_from_text_flag = false;         /* = 1 if we're in graphics mode */
evolution_info *g_evolve_handle = 0;
CalculationMode g_standard_calculation_mode_old;
void (*g_out_line_cleanup)();

/* routines in this module      */

ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked);
ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked);
ApplicationStateType big_while_loop(bool &kbdmore, bool &screen_stacked, bool resume_flag);
static void move_zoombox(int keynum);
static int out_line_compare(BYTE *pixels, int line_length);
static void out_line_cleanup_compare();

static LineCompare s_line_compare;
static ZoomSaver s_zoom_saver;

int LineCompare::compare(BYTE *pixels, int line_length)
{
	int row = g_row_count++;
	if (row == 0)
	{
		_error_count = 0;
		_file.open((g_work_dir / "cmperr.txt").string().c_str(),
			std::ios::out | (g_initialize_batch ? std::ios::ate : 0));
		g_out_line_cleanup = out_line_cleanup_compare;
	}
	if (g_potential_16bit)  /* 16 bit info, ignore odd numbered rows */
	{
		if (row & 1)
		{
			return 0;
		}
		row /= 2;
	}
	for (int col = 0; col < line_length; col++)
	{
		int old_color = get_color(col, row);
		if (old_color == int(pixels[col]))
		{
			g_plot_color_put_color(col, row, 0);
		}
		else
		{
			if (old_color == 0)
			{
				g_plot_color_put_color(col, row, 1);
			}
			++_error_count;
			if (g_initialize_batch == INITBATCH_NONE)
			{
				_file << boost::format("#%5d col %3d row %3d old %3d new %3d\n")
					% _error_count % col % row % old_color % pixels[col];
			}
		}
	}
	return 0;
}

void LineCompare::cleanup()
{
	if (g_initialize_batch)
	{
		time_t ltime;
		time(&ltime);
		char *timestring = ctime(&ltime);
		timestring[24] = 0; /*clobber newline in time string */
		_file << boost::format("%s compare to %s has %5d errs\n")
			% timestring % g_read_name.c_str() % _error_count;
	}
	_file.close();
}

static int out_line_compare(BYTE *pixels, int line_length)
{
	return s_line_compare.compare(pixels, line_length);
}

static void out_line_cleanup_compare()
{
	s_line_compare.cleanup();
}

void ZoomSaver::save()
{
	if (g_zoomBox.count() > 0)  /* save zoombox stuff in mem before encode (mem reused) */
	{
		m_save_zoom = new int[3*g_zoomBox.count()];
		if (m_save_zoom == 0)
		{
			clear_zoom_box(); /* not enuf mem so clear the box */
		}
		else
		{
			reset_zoom_corners(); /* reset these to overall image, not box */
			g_zoomBox.save(m_save_zoom,
				m_save_zoom + g_zoomBox.count(),
				m_save_zoom + g_zoomBox.count()*2, g_zoomBox.count());
		}
	}
}

void ZoomSaver::restore()
{
	if (g_zoomBox.count() > 0)  /* restore zoombox arrays */
	{
		g_zoomBox.restore(m_save_zoom,
			m_save_zoom + g_zoomBox.count(),
			m_save_zoom + g_zoomBox.count()*2, g_zoomBox.count());
		delete[] m_save_zoom;
		zoom_box_draw(true); /* get the g_xx_min etc variables recalc'd by redisplaying */
	}
}

ApplicationStateType big_while_loop(bool &kbdmore, bool &screen_stacked, bool resume_flag)
{
	double  ftemp;                       /* fp temp                      */
	int     i = 0;                           /* temporary loop counters      */
	int kbdchar;

#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif
	bool julia_entered_from_mandelbrot = false;
	if (resume_flag)
	{
		goto resumeloop;
	}

	while (true)                    /* eternal loop */
	{
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		if (g_calculation_status != CALCSTAT_RESUMABLE || g_show_file == SHOWFILE_PENDING)
		{
			g_.SetVideoEntry(g_.Adapter());
			g_x_dots   = g_.VideoEntry().x_dots;       /* # dots across the screen */
			g_y_dots   = g_.VideoEntry().y_dots;       /* # dots down the screen   */
			g_colors  = g_.VideoEntry().colors;      /* # colors available */
			g_screen_width  = g_x_dots;
			g_screen_height  = g_y_dots;
			g_screen_x_offset = 0;
			g_screen_y_offset = 0;
			g_rotate_hi = (g_rotate_hi < g_colors) ? g_rotate_hi : g_colors - 1;

			g_.OldDAC() = g_.DAC(); /* save the DAC */

			if (g_overlay_3d && !g_initialize_batch)
			{
				driver_unstack_screen();            /* restore old graphics image */
				g_overlay_3d = 0;
			}
			else
			{
				DriverManager::change_video_mode(g_.VideoEntry()); /* switch video modes */
				/* switching video modes may have changed drivers or disk flag... */
				if (!g_.GoodMode())
				{
					if (!driver_diskp())
					{
						stop_message(STOPMSG_NORMAL, "That video mode is not available with your adapter.");
					}
					g_ui_state.ask_video = true;
					g_.SetInitialVideoModeNone();
					driver_set_for_text(); /* switch to text mode */
					/* goto restorestart; */
					return APPSTATE_RESTORE_START;
				}

				g_x_dots = g_screen_width;
				g_y_dots = g_screen_height;
				g_.SetVideoEntrySize(g_x_dots, g_y_dots);
			}

			if (g_.SaveDAC() || g_color_preloaded)
			{
				g_.DAC() = g_.OldDAC(); /* restore the DAC */
				load_dac();
				g_color_preloaded = false;
			}
			else
			{	/* reset DAC to defaults, which setvideomode has done for us */
				if (g_.MapDAC())
				{	/* but there's a map=, so load that */
					g_.DAC() = *g_.MapDAC();
					load_dac();
				}
				g_.SetColorState(COLORSTATE_DEFAULT);
			}
			if (g_viewWindow.Visible())
			{
				/* bypass for VESA virtual screen */
				ftemp = g_viewWindow.AspectRatio()*((double(g_screen_height))/(double(g_screen_width))/g_screen_aspect_ratio);
				g_x_dots = g_viewWindow.Width();
				if (g_x_dots != 0)
				{	/* g_x_dots specified */
					g_y_dots = g_viewWindow.Height();
					if (g_y_dots == 0) /* calc g_y_dots? */
					{
						g_y_dots = int(double(g_x_dots)*ftemp + 0.5);
					}
				}
				else if (g_viewWindow.AspectRatio() <= g_screen_aspect_ratio)
				{
					g_x_dots = int(double(g_screen_width)/g_viewWindow.Reduction() + 0.5);
					g_y_dots = int(double(g_x_dots)*ftemp + 0.5);
				}
				else
				{
					g_y_dots = int(double(g_screen_height)/g_viewWindow.Reduction() + 0.5);
					g_x_dots = int(double(g_y_dots)/ftemp + 0.5);
				}
				if (g_x_dots > g_screen_width || g_y_dots > g_screen_height)
				{
					stop_message(STOPMSG_NORMAL, "View window too large; using full screen.");
					g_viewWindow.FullScreen(g_screen_width, g_screen_height);
					g_x_dots = g_screen_width;
					g_y_dots = g_screen_height;
				}
				else if (((g_x_dots <= 1) /* changed test to 1, so a 2x2 window will */
					|| (g_y_dots <= 1)) /* work with the sound feature */
					&& !(g_evolving_flags & EVOLVE_FIELD_MAP))
				{	/* so ssg works */
					/* but no check if in evolve mode to allow lots of small views*/
					stop_message(STOPMSG_NORMAL, "View window too small; using full screen.");
					g_viewWindow.Hide();
					g_x_dots = g_screen_width;
					g_y_dots = g_screen_height;
				}
				if ((g_evolving_flags & EVOLVE_FIELD_MAP) && (g_current_fractal_specific->flags & FRACTALFLAG_INFINITE_CALCULATION))
				{
					stop_message(STOPMSG_NORMAL, "Fractal doesn't terminate! switching off evolution.");
					g_evolving_flags &= ~EVOLVE_FIELD_MAP;
					g_viewWindow.Hide();
					g_x_dots = g_screen_width;
					g_y_dots = g_screen_height;
				}
				if (g_evolving_flags & EVOLVE_FIELD_MAP)
				{
					g_x_dots = (g_screen_width/g_grid_size) - !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
					g_x_dots -= g_x_dots % 4; /* trim to multiple of 4 for SSG */
					g_y_dots = (g_screen_height/g_grid_size) - !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
					g_y_dots -= g_y_dots % 4;
				}
				else
				{
					g_screen_x_offset = (g_screen_width - g_x_dots)/2;
					g_screen_y_offset = (g_screen_height - g_y_dots)/3;
				}
			}
			g_dx_size = g_x_dots - 1;            /* convert just once now */
			g_dy_size = g_y_dots - 1;
		}
		/* assume we save next time (except jb) */
		g_.SetSaveDAC((g_.SaveDAC() == SAVEDAC_NO) ? SAVEDAC_NEXT : SAVEDAC_YES);
		if (g_initialize_batch == INITBATCH_NONE)
		{
			driver_set_mouse_mode(-FIK_PAGE_UP);        /* mouse left button == pgup */
		}

		if (g_show_file == SHOWFILE_PENDING)
		{               /* loading an image */
			g_out_line_cleanup = 0;          /* g_out_line routine can set this */
			if (g_display_3d)                 /* set up 3D decoding */
			{
				g_out_line = out_line_3d;
			}
			else if (g_compare_gif)            /* debug 50 */
			{
				g_out_line = out_line_compare;
			}
			else if (g_potential_16bit)
			{            /* .pot format input file */
				if (disk_start_potential() < 0)
				{                           /* pot file failed?  */
					g_show_file = SHOWFILE_DONE;
					g_potential_flag  = false;
					g_potential_16bit = false;
					g_.SetInitialVideoModeNone();
					g_calculation_status = CALCSTAT_RESUMABLE;         /* "resume" without 16-bit */
					driver_set_for_text();
					get_fractal_type();
					/* goto imagestart; */
					return APPSTATE_IMAGE_START;
				}
				g_out_line = out_line_potential;
			}
			else if ((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !g_evolving_flags) /* regular gif/fra input file */
			{
				g_out_line = out_line_sound;      /* sound decoding */
			}
			else
			{
				g_out_line = out_line;        /* regular decoding */
			}
			if (2224 == g_debug_mode)
			{
				stop_message(STOPMSG_NO_BUZZER,
					str(boost::format("floatflag=%d") % (g_user_float_flag ? 1 : 0)));
			}
			i = funny_glasses_call(gifview);
			if (g_out_line_cleanup)              /* cleanup routine defined? */
			{
				g_out_line_cleanup();
			}
			if (i == 0)
			{
				driver_buzzer(BUZZER_COMPLETE);
			}
			else
			{
				g_calculation_status = CALCSTAT_NO_FRACTAL;
				// TODO: don't support aborting of load
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

		g_zoom_off = true;                      /* zooming is enabled */
		if (driver_diskp() || (g_current_fractal_specific->flags & FRACTALFLAG_NO_ZOOM) != 0)
		{
			g_zoom_off = false;                   /* for these cases disable zooming */
		}
		if (!g_evolving_flags)
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
			copy_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_min());
			copy_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_max());
			copy_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_min());
			copy_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_max());
			copy_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_3rd());
			copy_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_3rd());
		}
		history_save_info();

		if (g_show_file == SHOWFILE_PENDING)
		{               /* image has been loaded */
			g_show_file = SHOWFILE_DONE;
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
			g_browse_state.SetBrowsing(false);      /* regenerate image, turn off browsing */
			/*rb*/
			g_name_stack_ptr = -1;   /* reset pointer */
			g_browse_state.SetName("");
			if (g_viewWindow.Visible() && (g_evolving_flags & EVOLVE_FIELD_MAP) && (g_calculation_status != CALCSTAT_COMPLETED))
			{
				/* generate a set of images with varied parameters on each one */
				int grout;
				int ecount;
				int tmpxdots;
				int tmpydots;
				int gridsqr;
				evolution_info resume_e_info;

				if ((g_evolve_handle != 0) && (g_calculation_status == CALCSTAT_RESUMABLE))
				{
					memcpy(&resume_e_info, g_evolve_handle, sizeof(resume_e_info));
					g_parameter_range_x  = resume_e_info.parameter_range_x;
					g_parameter_range_y  = resume_e_info.parameter_range_y;
					g_parameter_offset_x = resume_e_info.opx;
					g_parameter_offset_y = resume_e_info.opy;
					g_new_parameter_offset_x = resume_e_info.opx;
					g_new_parameter_offset_y = resume_e_info.opy;
					g_new_discrete_parameter_offset_x = resume_e_info.odpx;
					g_new_discrete_parameter_offset_y = resume_e_info.odpy;
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y;
					g_px           = resume_e_info.px;
					g_py           = resume_e_info.py;
					g_screen_x_offset       = resume_e_info.sxoffs;
					g_screen_y_offset       = resume_e_info.syoffs;
					g_x_dots        = resume_e_info.x_dots;
					g_y_dots        = resume_e_info.y_dots;
					g_grid_size       = resume_e_info.grid_size;
					g_this_generation_random_seed = resume_e_info.this_generation_random_seed;
					g_fiddle_factor   = resume_e_info.fiddle_factor;
					g_evolving_flags     = resume_e_info.evolving;
					if (g_evolving_flags)
					{
						g_viewWindow.Show();
					}
					ecount       = resume_e_info.ecount;
					delete g_evolve_handle;
					g_evolve_handle = 0;
				}
				else
				{ /* not resuming, start from the beginning */
					int mid = g_grid_size/2;
					if ((g_px != mid) || (g_py != mid))
					{
						g_this_generation_random_seed = (unsigned int)clock_ticks(); /* time for new set */
					}
					save_parameter_history();
					ecount = 0;
					g_fiddle_factor *= g_fiddle_reduction;
					g_parameter_offset_x = g_new_parameter_offset_x;
					g_parameter_offset_y = g_new_parameter_offset_y;
					/* odpx used for discrete parms like inside, outside, trigfn etc */
					g_discrete_parameter_offset_x = g_new_discrete_parameter_offset_x;
					g_discrete_parameter_offset_y = g_new_discrete_parameter_offset_y; 
				}
				g_parameter_box_count = 0;
				g_delta_parameter_image_x = g_parameter_range_x/(g_grid_size-1);
				g_delta_parameter_image_y = g_parameter_range_y/(g_grid_size-1);
				grout  = !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
				tmpxdots = g_x_dots + grout;
				tmpydots = g_y_dots + grout;
				gridsqr = g_grid_size*g_grid_size;
				while (ecount < gridsqr)
				{
					spiral_map(ecount); /* sets px & py */
					g_screen_x_offset = tmpxdots*g_px;
					g_screen_y_offset = tmpydots*g_py;
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
					if (g_evolve_handle == 0)
					{
						g_evolve_handle = new evolution_info;
					}
					resume_e_info.parameter_range_x     = g_parameter_range_x;
					resume_e_info.parameter_range_y     = g_parameter_range_y;
					resume_e_info.opx             = g_parameter_offset_x;
					resume_e_info.opy             = g_parameter_offset_y;
					resume_e_info.odpx            = short(g_discrete_parameter_offset_x);
					resume_e_info.odpy            = short(g_discrete_parameter_offset_y);
					resume_e_info.px              = short(g_px);
					resume_e_info.py              = short(g_py);
					resume_e_info.sxoffs          = short(g_screen_x_offset);
					resume_e_info.syoffs          = short(g_screen_y_offset);
					resume_e_info.x_dots           = short(g_x_dots);
					resume_e_info.y_dots           = short(g_y_dots);
					resume_e_info.grid_size          = short(g_grid_size);
					resume_e_info.this_generation_random_seed  = short(g_this_generation_random_seed);
					resume_e_info.fiddle_factor = g_fiddle_factor;
					resume_e_info.evolving        = short(g_evolving_flags);
					resume_e_info.ecount          = short(ecount);
					memcpy(g_evolve_handle, &resume_e_info, sizeof(resume_e_info));
				}
				g_screen_x_offset = 0;
				g_screen_y_offset = 0;
				g_x_dots = g_screen_width;
				g_y_dots = g_screen_height; /* otherwise save only saves a sub image and boxes get clipped */

				/* set up for 1st selected image, this reuses px and py */
				g_px = g_grid_size/2;
				g_py = g_grid_size/2;
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
		g_zoomBox.set_count(0);                     /* no zoom box yet  */
		g_z_width = 0;
#else
		if (!XZoomWaiting)
		{
			g_zoomBox.set_count(0);                 /* no zoom box yet  */
			g_z_width = 0;
		}
#endif

		if (g_fractal_type == FRACTYPE_PLASMA)
		{
			g_cycle_limit = 256;              /* plasma clouds need quick spins */
			g_.SetDACSleepCount(256);
		}

resumeloop:
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		kbdmore = true;
		while (kbdmore)
		{           /* loop through command keys */
			if (g_timed_save != TIMEDSAVE_DONE)
			{
				if (g_timed_save == TIMEDSAVE_START)
				{       /* woke up for timed save */
					driver_get_key();     /* eat the dummy char */
					kbdchar = 's'; /* do the save */
					g_resave_mode = RESAVE_YES;
					g_timed_save = TIMEDSAVE_PENDING;
				}
				else
				{                      /* save done, resume */
					g_timed_save = TIMEDSAVE_DONE;
					g_resave_mode = RESAVE_DONE;
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
					if (g_browse_state.AutoBrowse() && g_browse_state.SubImages())
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
						if (kbdchar == FIK_ESC && g_escape_exit_flag)
						{
							/* don't ask, just get out */
							goodbye();
						}
						driver_stack_screen();
#ifndef XFRACT
						kbdchar = main_menu(true);
#else
						if (XZoomWaiting)
						{
							kbdchar = FIK_ENTER;
						}
						else
						{
							kbdchar = main_menu(true);
							if (XZoomWaiting)
							{
								kbdchar = FIK_ENTER;
							}
						}
#endif
						if (kbdchar == '\\' || kbdchar == FIK_CTL_BACKSLASH ||
							kbdchar == 'h' || kbdchar == FIK_BACKSPACE ||
							check_video_mode_key(kbdchar) >= 0)
						{
							driver_discard_screen();
						}
						else if (kbdchar == 'x' || kbdchar == 'y' ||
								kbdchar == 'z' || kbdchar == 'g' ||
								kbdchar == 'v' || kbdchar == FIK_CTL_B ||
								kbdchar == FIK_CTL_E || kbdchar == FIK_CTL_F)
						{
							g_from_text_flag = true;
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
					kbdchar = (DEBUGMODE_COMPARE_RESTORED == g_debug_mode) ? 'r' : 's';
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

			if ('A' <= kbdchar && kbdchar <= 'Z')
			{
				kbdchar = tolower(kbdchar);
			}

			ApplicationStateType mms_value = g_evolving_flags ?
				evolver_menu_switch(kbdchar, julia_entered_from_mandelbrot, kbdmore, screen_stacked)
				: main_menu_switch(kbdchar, julia_entered_from_mandelbrot, kbdmore, screen_stacked);
			if (g_quick_calculate
				&& (mms_value == APPSTATE_IMAGE_START ||
					mms_value == APPSTATE_RESTORE_START ||
					mms_value == APPSTATE_RESTART))
			{
				g_quick_calculate = false;
				g_user_standard_calculation_mode = g_standard_calculation_mode_old;
			}
			if (g_quick_calculate && g_calculation_status != CALCSTAT_COMPLETED)
			{
				g_user_standard_calculation_mode = CALCMODE_SINGLE_PASS;
			}
			switch (mms_value)
			{
			case APPSTATE_IMAGE_START:		return APPSTATE_IMAGE_START;
			case APPSTATE_RESTORE_START:	return APPSTATE_RESTORE_START;
			case APPSTATE_RESTART:			return APPSTATE_RESTART;
			case APPSTATE_CONTINUE:			continue;
			default:						break;
			}
			if (g_zoom_off && kbdmore) /* draw/clear a zoom box? */
			{
				zoom_box_draw(true);
			}
			if (driver_resize())
			{
				g_calculation_status = CALCSTAT_NO_FRACTAL;
			}
		}
	}
}

static ApplicationStateType handle_fractal_type(bool &frommandel)
{
	int i;

	g_julibrot = false;
	clear_zoom_box();
	driver_stack_screen();
	i = get_fractal_type();
	if (i >= 0)
	{
		driver_discard_screen();
		g_.SetSaveDAC(SAVEDAC_NO);
		g_save_release = g_release;
		g_no_magnitude_calculation = false;
		g_use_old_periodicity = false;
		g_bad_outside = false;
		g_use_old_complex_power = true;
		set_current_parameters();
		g_discrete_parameter_offset_x = 0;
		g_discrete_parameter_offset_y = 0;
		g_new_discrete_parameter_offset_x = 0;
		g_new_discrete_parameter_offset_y = 0;
		g_fiddle_factor = 1;           /* reset param evolution stuff */
		g_set_orbit_corners = false;
		save_parameter_history();
		if (i == 0)
		{
			g_.SetInitialVideoMode(g_.Adapter());
			frommandel = false;
		}
		else if (g_.InitialVideoMode() < 0) /* it is supposed to be... */
		{
			driver_set_for_text();     /* reset to text mode      */
		}
		return APPSTATE_IMAGE_START;
	}
	driver_unstack_screen();
	return APPSTATE_NO_CHANGE;
}

static void handle_options(int kbdchar, bool &kbdmore, long *old_maxit)
{
	*old_maxit = g_max_iteration;
	clear_zoom_box();
	if (g_from_text_flag)
	{
		g_from_text_flag = false;
	}
	else
	{
		driver_stack_screen();
	}
	int i;
	switch (kbdchar)
	{
	case 'x':		i = get_toggles();						break;
	case 'y':		i = get_toggles2();						break;
	case 'p':		i = passes_options();					break;
	case 'z':		i = get_fractal_parameters(true);		break;
	case 'v':		i = get_view_params();					break;
	case FIK_CTL_B:	i = g_browse_state.GetParameters();		break;

	case FIK_CTL_E:
		i = get_evolve_parameters();
		if (i > 0)
		{
			g_start_show_orbit = false;
			g_sound_state.silence_xyz();
			g_log_automatic_flag = false;
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
	if (g_evolving_flags && g_true_color)
	{
		g_true_color = false; /* truecolor doesn't play well with the evolver */
	}
	if (g_max_iteration > *old_maxit
		&& g_inside >= 0
		&& g_calculation_status == CALCSTAT_COMPLETED
		&& g_current_fractal_specific->calculate_type == standard_fractal
		&& !g_log_palette_mode
		&& !g_true_color /* recalc not yet implemented with truecolor */
		&& !(g_user_standard_calculation_mode == CALCMODE_TESSERAL && g_fill_color > -1) /* tesseral with fill doesn't work */
		&& !(g_user_standard_calculation_mode == CALCMODE_ORBITS)
		&& i == COMMANDRESULT_FRACTAL_PARAMETER /* nothing else changed */
		&& g_outside != COLORMODE_INVERSE_TANGENT)
	{
		g_quick_calculate = true;
		g_standard_calculation_mode_old = g_user_standard_calculation_mode;
		g_user_standard_calculation_mode = CALCMODE_SINGLE_PASS;
		kbdmore = false;
		g_calculation_status = CALCSTAT_RESUMABLE;
	}
	else if (i > 0)
	{              /* time to redraw? */
		g_quick_calculate = false;
		save_parameter_history();
		kbdmore = false;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
}

static void handle_evolver_options(int kbdchar, bool &kbdmore)
{
	int i;
	clear_zoom_box();
	if (g_from_text_flag)
	{
		g_from_text_flag = false;
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
	case 'z': i = get_fractal_parameters(true); break;

	case FIK_CTL_E:
	case FIK_SPACE:
		i = get_evolve_parameters();
		break;

	default:
		i = get_command_string();
		break;
	}
	driver_unstack_screen();
	if (g_evolving_flags && g_true_color)
	{
		g_true_color = false; /* truecolor doesn't play well with the evolver */
	}
	if (i > COMMANDRESULT_OK)              /* time to redraw? */
	{
		save_parameter_history();
		kbdmore = false;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
}

static bool handle_execute_commands(int &kbdchar, bool &kbdmore)
{
	int i;
	driver_stack_screen();
	i = get_commands();
	if (g_.InitialVideoMode() != -1)
	{                         /* video= was specified */
		g_.SetAdapter(g_.InitialVideoMode());
		g_.SetInitialVideoModeNone();
		i |= COMMANDRESULT_FRACTAL_PARAMETER;
		g_.SetSaveDAC(SAVEDAC_NO);
	}
	else if (g_color_preloaded)
	{                         /* colors= was specified */
		load_dac();
		g_color_preloaded = false;
	}
	else if (i & COMMANDRESULT_RESET)         /* reset was specified */
	{
		g_.SetSaveDAC(SAVEDAC_NO);
	}
	if (i & COMMANDRESULT_3D_YES)
	{                         /* 3d = was specified */
		kbdchar = '3';
		driver_unstack_screen();
		return true;
	}
	if (i & COMMANDRESULT_FRACTAL_PARAMETER)
	{                         /* fractal parameter changed */
		driver_discard_screen();
		kbdmore = false;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
	else
	{
		driver_unstack_screen();
	}

	return false;
}

static ApplicationStateType handle_toggle_float()
{
	if (!g_user_float_flag)
	{
		g_user_float_flag = true;
	}
	else if (g_standard_calculation_mode != 'o') /* don't go there */
	{
		g_user_float_flag = false;
	}
	g_.SetInitialVideoMode(g_.Adapter());
	return APPSTATE_IMAGE_START;
}

static ApplicationStateType handle_ant()
{
	int oldtype;
	int err;
	int i;
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
	g_from_text_flag = false;
	err = get_fractal_parameters(true);
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
	return (err >= 0) ? APPSTATE_CONTINUE : APPSTATE_NO_CHANGE;
}

static ApplicationStateType handle_recalc(int (*continue_check)(), int (*recalc_check)())
{
#if defined(_WIN32)
	_ASSERTE(continue_check && recalc_check);
#endif
	clear_zoom_box();
	if (continue_check() >= 0)
	{
		if (recalc_check() >= 0)
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
		return APPSTATE_CONTINUE;
	}
	return APPSTATE_NO_CHANGE;
}

static void handle_3d_params(bool &kbdmore)
{
	if (get_fractal_3d_parameters() >= 0)    /* get the parameters */
	{
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		kbdmore = false;    /* time to redraw */
	}
}

static void handle_orbits()
{
	/* must use standard fractal and have a float variant */
	if ((g_fractal_specific[g_fractal_type].calculate_type == standard_fractal
			|| g_fractal_specific[g_fractal_type].calculate_type == froth_calc)
		&& (g_fractal_specific[g_fractal_type].isinteger == 0
			|| !fractal_type_none(g_fractal_specific[g_fractal_type].tofloat))
		&& !g_bf_math /* for now no arbitrary precision support */
		&& !(g_is_true_color && g_true_mode_iterates))
	{
		clear_zoom_box();
		Jiim(true);
	}
}

static void set_fractal_specific_to_julia_mandelbrot(int to_julia_type, int to_mandelbrot_type)
{
	// TODO: eliminate writing to g_fractal_specific
	FractalTypeSpecificData &target = g_fractal_specific[g_fractal_type];
	target.tojulia = to_julia_type;
	target.tomandel = to_mandelbrot_type;
	g_is_mandelbrot = (to_mandelbrot_type == FRACTYPE_NO_FRACTAL);
}

static void handle_mandelbrot_julia_toggle(bool &kbdmore, bool &frommandel)
{
	static double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
	static double  jxx3rd, jyy3rd;

	if (g_bf_math || g_evolving_flags)
	{
		return;
	}
	if (g_fractal_type == FRACTYPE_CELLULAR)
	{
		g_next_screen_flag = !g_next_screen_flag;
		g_calculation_status = CALCSTAT_RESUMABLE;
		kbdmore = false;
		return;
	}

	if (fractal_type_formula(g_fractal_type))
	{
		if (g_is_mandelbrot)
		{
			set_fractal_specific_to_julia_mandelbrot(g_fractal_type, FRACTYPE_NO_FRACTAL);
		}
		else
		{
			set_fractal_specific_to_julia_mandelbrot(FRACTYPE_NO_FRACTAL, g_fractal_type);
		}
	}

	if (!fractal_type_none(g_current_fractal_specific->tojulia)
		&& g_parameters[0] == 0.0
		&& g_parameters[1] == 0.0)
	{
		/* switch to corresponding Julia set */
		g_has_inverse = fractal_type_mandelbrot(g_fractal_type) && (g_bf_math == 0);
		clear_zoom_box();
		Jiim(false);
		int key = driver_get_key();    /* flush keyboard buffer */
		if (key != FIK_SPACE)
		{
			driver_unget_key(key);
			return;
		}
		g_fractal_type = g_current_fractal_specific->tojulia;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		if (g_julia_c.real() == BIG || g_julia_c.imag() == BIG)
		{
			g_parameters[0] = g_escape_time_state.m_grid_fp.x_center();
			g_parameters[1] = g_escape_time_state.m_grid_fp.y_center();
		}
		else
		{
			g_parameters[0] = g_julia_c.real();
			g_parameters[1] = g_julia_c.imag();
			g_julia_c = std::complex<double>(BIG, BIG);
		}
		jxxmin = g_sx_min;
		jxxmax = g_sx_max;
		jyymax = g_sy_max;
		jyymin = g_sy_min;
		jxx3rd = g_sx_3rd;
		jyy3rd = g_sy_3rd;
		frommandel = true;
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
		g_zoom_off = true;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		kbdmore = false;
	}
	else if (!fractal_type_none(g_current_fractal_specific->tomandel))
	{
		/* switch to corresponding Mandel set */
		g_fractal_type = g_current_fractal_specific->tomandel;
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		if (frommandel)
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
			g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
			g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
			g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
			g_escape_time_state.m_grid_fp.x_3rd() = g_current_fractal_specific->x_min;
			g_escape_time_state.m_grid_fp.y_3rd() = g_current_fractal_specific->y_min;
		}
		g_save_c.x = g_parameters[0];
		g_save_c.y = g_parameters[1];
		g_parameters[0] = 0;
		g_parameters[1] = 0;
		g_zoom_off = true;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		kbdmore = false;
	}
	else
	{
		driver_buzzer(BUZZER_ERROR);          /* can't switch */
	}
}

static void handle_inverse_julia_toggle(bool &kbdmore)
{
	/* if the inverse types proliferate, something more elegant will be
	* needed */
	if (fractal_type_julia_or_inverse(g_fractal_type))
	{
		static int oldtype = -1;
		if (fractal_type_julia(g_fractal_type))
		{
			oldtype = g_fractal_type;
			g_fractal_type = FRACTYPE_INVERSE_JULIA;
		}
		else if (fractal_type_inverse_julia(g_fractal_type))
		{
			g_fractal_type = (oldtype != -1) ? oldtype : FRACTYPE_JULIA;
		}
		g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		g_zoom_off = true;
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		kbdmore = false;
	}
	else
	{
		driver_buzzer(BUZZER_ERROR);
	}
}

static ApplicationStateType handle_history(bool &stacked, int kbdchar)
{
	if (g_name_stack_ptr >= 1)
	{
		/* go back one file if somewhere to go (ie. browsing) */
		g_name_stack_ptr--;
		while (g_file_name_stack[g_name_stack_ptr].length() == 0
			&& g_name_stack_ptr >= 0)
		{
			g_name_stack_ptr--;
		}
		if (g_name_stack_ptr < 0) /* oops, must have deleted first one */
		{
			return APPSTATE_NO_CHANGE;
		}
		g_browse_state.SetName(g_file_name_stack[g_name_stack_ptr]);
		g_browse_state.MergePathNames(g_read_name);
		g_browse_state.SetBrowsing(true);
		g_browse_state.SetSubImages(true);
		g_show_file = SHOWFILE_PENDING;
		if (g_ui_state.ask_video)
		{
			driver_stack_screen();      /* save graphics image */
			stacked = true;
		}
		return APPSTATE_RESTORE_START;
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
		g_zoom_off = true;
		g_.SetInitialVideoMode(g_.Adapter());
		if (g_current_fractal_specific->isinteger != 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_user_float_flag = false;
		}
		if (g_current_fractal_specific->isinteger == 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_user_float_flag = true;
		}
		return APPSTATE_IMAGE_START;
	}

	return APPSTATE_NO_CHANGE;
}

static ApplicationStateType handle_color_cycling(int kbdchar)
{
	clear_zoom_box();
	g_.OldDAC() = g_.DAC();
	rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
	if (g_.OldDAC() != g_.DAC())
	{
		g_.SetColorState(COLORSTATE_UNKNOWN);
		history_save_info();
	}
	return APPSTATE_CONTINUE;
}

static ApplicationStateType handle_color_editing(bool &kbdmore)
{
	if (g_is_true_color && !g_initialize_batch) /* don't enter palette editor */
	{
		if (load_palette() >= 0)
		{
			kbdmore = false;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
			return APPSTATE_NO_CHANGE;
		}
		else
		{
			return APPSTATE_CONTINUE;
		}
	}
	clear_zoom_box();
	if (g_.DAC().Red(0) != 255
		&& !driver_diskp())
	{
		g_.OldDAC() = g_.DAC();
		palette_edit();
		if (g_.OldDAC() != g_.DAC())
		{
			g_.SetColorState(COLORSTATE_UNKNOWN);
			history_save_info();
		}
	}
	return APPSTATE_CONTINUE;
}

static ApplicationStateType handle_save_to_disk()
{
	if (driver_diskp() && g_disk_targa)
	{
		return APPSTATE_CONTINUE;  /* disk video and targa, nothing to save */
	}
	s_zoom_saver.save();
	save_to_disk(g_save_name);
	s_zoom_saver.restore();
	return APPSTATE_CONTINUE;
}

static ApplicationStateType handle_evolver_save_to_disk()
{
	int oldsxoffs;
	int oldsyoffs;
	int oldxdots;
	int oldydots;
	int oldpx;
	int oldpy;

	if (driver_diskp() && g_disk_targa)
	{
		return APPSTATE_CONTINUE;  /* disk video and targa, nothing to save */
	}

	oldsxoffs = g_screen_x_offset;
	oldsyoffs = g_screen_y_offset;
	oldxdots = g_x_dots;
	oldydots = g_y_dots;
	oldpx = g_px;
	oldpy = g_py;
	g_screen_x_offset = 0;
	g_screen_y_offset = 0;
	g_x_dots = g_screen_width;
	g_y_dots = g_screen_height; /* for full screen save and pointer move stuff */
	g_px = g_grid_size/2;
	g_py = g_grid_size/2;
	restore_parameter_history();
	fiddle_parameters(g_genes, 0);
	draw_parameter_box(true);
	save_to_disk(g_save_name);
	g_px = oldpx;
	g_py = oldpy;
	restore_parameter_history();
	fiddle_parameters(g_genes, unspiral_map());
	g_screen_x_offset = oldsxoffs;
	g_screen_y_offset = oldsyoffs;
	g_x_dots = oldxdots;
	g_y_dots = oldydots;
	return APPSTATE_CONTINUE;
}

static ApplicationStateType handle_restore_from(bool &frommandel, int kbdchar, bool &stacked)
{
	g_compare_gif = false;
	frommandel = false;
	g_browse_state.SetBrowsing(false);
	if (kbdchar == 'r')
	{
		if (DEBUGMODE_COMPARE_RESTORED == g_debug_mode)
		{
			g_compare_gif = true;
			g_overlay_3d = 1;
			if (g_initialize_batch == INITBATCH_SAVE)
			{
				driver_stack_screen();   /* save graphics image */
				g_read_name = g_save_name;
				g_show_file = SHOWFILE_PENDING;
				return APPSTATE_RESTORE_START;
			}
		}
		else
		{
			g_compare_gif = false;
			g_overlay_3d = 0;
		}
		g_display_3d = DISPLAY3D_NONE;
	}
	driver_stack_screen();            /* save graphics image */
	stacked = (g_overlay_3d == 0);
	if (g_resave_mode)
	{
		update_save_name(g_save_name);      /* do the pending increment */
		g_resave_mode = RESAVE_NO;
		g_started_resaves = false;
	}
	g_show_file = SHOWFILE_CANCELLED;
	return APPSTATE_RESTORE_START;
}

static void handle_zoom_in(bool &kbdmore)
{
#ifdef XFRACT
	XZoomWaiting = 0;
#endif
	if (g_z_width != 0.0)
	{                         /* do a zoom */
		init_pan_or_recalc(false);
		kbdmore = false;
	}
	if (g_calculation_status != CALCSTAT_COMPLETED)     /* don't restart if image complete */
	{
		kbdmore = false;
	}
}

static void handle_zoom_out(bool &kbdmore)
{
	if (g_z_width != 0.0)
	{
		init_pan_or_recalc(true);
		kbdmore = false;
		zoom_box_out();                /* calc corners for zooming out */
	}
}

static void handle_zoom_skew(bool negative)
{
	if (negative)
	{
		if (g_zoomBox.count() && !g_current_fractal_specific->no_zoom_box_rotate())
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
		if (g_zoomBox.count() && !g_current_fractal_specific->no_zoom_box_rotate())
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

static void handle_select_video(int &kbdchar)
{
	driver_stack_screen();
	kbdchar = select_video_mode(g_.Adapter());
	if (check_video_mode_key(kbdchar) >= 0)  /* picked a new mode? */
	{
		driver_discard_screen();
	}
	else
	{
		driver_unstack_screen();
	}
}

static void handle_mutation_level(bool forward, int amount, bool &kbdmore)
{
	g_viewWindow.Show();
	g_evolving_flags = EVOLVE_FIELD_MAP;
	set_mutation_level(amount);
	if (forward)
	{
		restore_parameter_history();
	}
	else
	{
		save_parameter_history();
	}
	kbdmore = false;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static ApplicationStateType handle_video_mode(int kbdchar, bool &kbdmore)
{
	int k = check_video_mode_key(kbdchar);
	if (k >= 0)
	{
		g_.SetAdapter(k);
		if (g_.VideoTable(g_.Adapter()).colors != g_colors)
		{
			g_.SetSaveDAC(SAVEDAC_NO);
		}
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		kbdmore = false;
		return APPSTATE_CONTINUE;
	}
	return APPSTATE_NO_CHANGE;
}

static void handle_z_rotate(bool increase)
{
	if (g_zoomBox.count() && !g_current_fractal_specific->no_zoom_box_rotate())
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

static void handle_box_color(bool increase)
{
	if (increase)
	{
		g_zoomBox.set_color(g_zoomBox.color() + key_count(FIK_CTL_INSERT));
	}
	else
	{
		g_zoomBox.set_color(g_zoomBox.color() - key_count(FIK_CTL_DEL));
	}
}

static void handle_zoom_resize(bool zoom_in)
{
	if (zoom_in)
	{
		if (g_zoom_off)
		{
			if (g_z_width == 0)
			{                      /* start zoombox */
				g_z_width = 1.0;
				g_z_depth = 1.0;
				g_z_skew = 0.0;
				g_z_rotate = 0;
				g_zbx = 0.0;
				g_zby = 0.0;
				g_.DAC().FindSpecialColors();
				g_zoomBox.set_color(g_.DAC().Bright());
				g_px = g_grid_size/2;
				g_py = g_grid_size/2;
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
		if (g_zoomBox.count())
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

static void handle_zoom_stretch(bool narrower)
{
	if (g_zoomBox.count())
	{
		zoom_box_change_i(0, narrower ?
			-2*key_count(FIK_CTL_PAGE_UP) : 2*key_count(FIK_CTL_PAGE_DOWN));
	}
}

static ApplicationStateType handle_restart()
{
	driver_set_for_text();           /* force text mode */
	return APPSTATE_RESTART;
}

ApplicationStateType main_menu_switch(int &kbdchar, bool &frommandel, bool &kbdmore, bool &screen_stacked)
{
	long old_maxit;

	if (g_quick_calculate)
	{
		if (CALCSTAT_COMPLETED == g_calculation_status)
		{
			g_quick_calculate = false;
		}
		g_user_standard_calculation_mode = g_standard_calculation_mode_old;
	}
	switch (kbdchar)
	{
	case 't':
		return handle_fractal_type(frommandel);

	case FIK_CTL_X:
	case FIK_CTL_Y:
	case FIK_CTL_Z:
		flip_image(kbdchar);
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
		handle_options(kbdchar, kbdmore, &old_maxit);
		break;

	case '@':
	case '2':
		if (handle_execute_commands(kbdchar, kbdmore))
		{
			goto do_3d_transform;  /* pretend '3' was keyed */
		}
		break;

	case 'f':
		return handle_toggle_float();

	case 'i':
		handle_3d_params(kbdmore);
		break;

	case FIK_CTL_A:
		return handle_ant();

	case 'k':
	case FIK_CTL_S:
		return handle_recalc(get_random_dot_stereogram_parameters, auto_stereo);

	case 'a':
		return handle_recalc(get_starfield_params, starfield);

	case FIK_CTL_O:
	case 'o':
		handle_orbits();
		break;

	case FIK_SPACE:
		handle_mandelbrot_julia_toggle(kbdmore, frommandel);
		break;

	case 'j':
		handle_inverse_julia_toggle(kbdmore);
		break;

	case '\\':
	case FIK_CTL_BACKSLASH:
	case 'h':
	case FIK_BACKSPACE:
		return handle_history(screen_stacked, kbdchar);

	case 'd':
		{
			ScreenStacker stacker;
			driver_shell();
		}
		break;

	case 'c':
	case '+':
	case '-':
		return handle_color_cycling(kbdchar);

	case 'e':
		return handle_color_editing(kbdmore);

	case 's':
		return handle_save_to_disk();

	case '#':
		clear_zoom_box();
		g_overlay_3d = 1;
		/* fall through */

do_3d_transform:
	case '3':                    /* restore-from (3d)            */
		g_display_3d = g_overlay_3d ? DISPLAY3D_OVERLAY : DISPLAY3D_YES; /* for <b> command               */
		/* fall through */

	case 'r':                    /* restore-from                 */
		return handle_restore_from(frommandel, kbdchar, screen_stacked);

	case 'l':
	case 'L':
		return handle_look_for_files(screen_stacked);
		break;

	case 'b':
		make_batch_file();
		break;

	case FIK_ENTER:
	case FIK_ENTER_2:
		handle_zoom_in(kbdmore);
		break;

	case FIK_CTL_ENTER:
	case FIK_CTL_ENTER_2:
		handle_zoom_out(kbdmore);
		break;

	case FIK_INSERT:
		return handle_restart();

	case FIK_LEFT_ARROW:
	case FIK_RIGHT_ARROW:
	case FIK_UP_ARROW:
	case FIK_DOWN_ARROW:
	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_DOWN_ARROW:
		move_zoombox(kbdchar);
		break;

	case FIK_CTL_HOME:
	case FIK_CTL_END:
		handle_zoom_skew(kbdchar == FIK_CTL_HOME);
		break;

	case FIK_CTL_PAGE_UP:
	case FIK_CTL_PAGE_DOWN:
		handle_zoom_stretch(FIK_CTL_PAGE_UP == kbdchar);
		break;

	case FIK_PAGE_UP:
	case FIK_PAGE_DOWN:
		handle_zoom_resize(FIK_PAGE_UP == kbdchar);
		break;

	case FIK_CTL_MINUS:
	case FIK_CTL_PLUS:
		handle_z_rotate(FIK_CTL_MINUS == kbdchar);
		break;

	case FIK_CTL_INSERT:
	case FIK_CTL_DEL:
		handle_box_color(FIK_CTL_INSERT == kbdchar);
		break;

	case FIK_ALT_1: /* alt + number keys set mutation level and start evolution engine */
	case FIK_ALT_2:
	case FIK_ALT_3:
	case FIK_ALT_4:
	case FIK_ALT_5:
	case FIK_ALT_6:
	case FIK_ALT_7:
		handle_mutation_level(false, kbdchar - FIK_ALT_1 + 1, kbdmore);
		break;

	case FIK_DELETE:
		handle_select_video(kbdchar);
		/* fall through */

	default:                     /* other (maybe a valid Fn key) */
		return handle_video_mode(kbdchar, kbdmore);
	}                            /* end of the big switch */

	return APPSTATE_NO_CHANGE;
}

static void handle_evolver_exit(bool &kbdmore)
{
	g_evolving_flags = EVOLVE_NONE;
	g_viewWindow.Hide();
	save_parameter_history();
	kbdmore = false;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static ApplicationStateType handle_evolver_history(int kbdchar)
{
	if (g_max_history > 0 && g_bf_math == 0)
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
		g_zoom_off = true;
		g_.SetInitialVideoMode(g_.Adapter());
		if (g_current_fractal_specific->isinteger != 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_user_float_flag = false;
		}
		if (g_current_fractal_specific->isinteger == 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_user_float_flag = true;
		}
		return APPSTATE_IMAGE_START;
	}
	return APPSTATE_NO_CHANGE;
}

static void handle_evolver_move_selection(int kbdchar)
{
	/* borrow ctrl cursor keys for moving selection box */
	/* in evolver mode */
	if (g_zoomBox.count())
	{
		int grout;
		if (g_evolving_flags & EVOLVE_FIELD_MAP)
		{
			if (kbdchar == FIK_CTL_LEFT_ARROW)
			{
				g_px--;
			}
			if (kbdchar == FIK_CTL_RIGHT_ARROW)
			{
				g_px++;
			}
			if (kbdchar == FIK_CTL_UP_ARROW)
			{
				g_py--;
			}
			if (kbdchar == FIK_CTL_DOWN_ARROW)
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
			grout = !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
			g_screen_x_offset = g_px*int(g_dx_size + 1 + grout);
			g_screen_y_offset = g_py*int(g_dy_size + 1 + grout);

			restore_parameter_history();
			fiddle_parameters(g_genes, unspiral_map()); /* change all parameters */
						/* to values appropriate to the image selected */
			set_evolve_ranges();
			zoom_box_change_i(0, 0);
			draw_parameter_box(false);
		}
	}
	else                       /* if no zoombox, scroll by arrows */
	{
		move_zoombox(kbdchar);
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
			draw_parameter_box(false);
			set_evolve_ranges();
		}
		else
		{
			g_parameter_zoom += 1.0;
			if (g_parameter_zoom > double(g_grid_size)/2.0)
			{
				g_parameter_zoom = double(g_grid_size)/2.0;
			}
			draw_parameter_box(false);
			set_evolve_ranges();
		}
	}
}

static void handle_evolver_zoom(int zoom_in)
{
	if (zoom_in)
	{
		if (g_zoom_off)
		{
			if (g_z_width == 0)
			{                      /* start zoombox */
				g_z_width = 1;
				g_z_depth = 1;
				g_z_skew = 0;
				g_z_rotate = 0;
				g_zbx = 0;
				g_zby = 0;
				g_.DAC().FindSpecialColors();
				g_zoomBox.set_color(g_.DAC().Bright());
				if (g_evolving_flags & EVOLVE_FIELD_MAP) /*rb*/
				{
					/* set screen view params back (previously changed to allow
					   full screen saves in view window mode) */
					int grout = !((g_evolving_flags & EVOLVE_NO_GROUT)/EVOLVE_NO_GROUT);
					g_screen_x_offset = g_px*int(g_dx_size + 1 + grout);
					g_screen_y_offset = g_py*int(g_dy_size + 1 + grout);
					setup_parameter_box();
					draw_parameter_box(false);
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
		if (g_zoomBox.count())
		{
			if (g_z_width >= 0.999 && g_z_depth >= 0.999) /* end zoombox */
			{
				g_z_width = 0;
				if (g_evolving_flags & EVOLVE_FIELD_MAP)
				{
					draw_parameter_box(true); /* clear boxes off screen */
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

static void handle_evolver_mutation(int halve, bool &kbdmore)
{
	if (halve)
	{
		g_fiddle_factor /= 2;
		g_parameter_range_x /= 2;
		g_new_parameter_offset_x = g_parameter_offset_x + g_parameter_range_x/2;
		g_parameter_range_y /= 2;
		g_new_parameter_offset_y = g_parameter_offset_y + g_parameter_range_y/2;
	}
	else
	{
		double centerx;
		double centery;
		g_fiddle_factor *= 2;
		centerx = g_parameter_offset_x + g_parameter_range_x/2;
		g_parameter_range_x *= 2;
		g_new_parameter_offset_x = centerx - g_parameter_range_x/2;
		centery = g_parameter_offset_y + g_parameter_range_y/2;
		g_parameter_range_y *= 2;
		g_new_parameter_offset_y = centery - g_parameter_range_y/2;
	}
	kbdmore = false;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static void handle_evolver_grid_size(int decrement, bool &kbdmore)
{
	if (decrement)
	{
		if (g_grid_size > 3)
		{
			g_grid_size -= 2;  /* g_grid_size must have odd value only */
			kbdmore = false;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
	else
	{
		if (g_grid_size < g_screen_width/(MIN_PIXELS << 1))
		{
			g_grid_size += 2;
			kbdmore = false;
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}
}

static void handle_evolver_toggle(bool &kbdmore)
{
	int i;
	for (i = 0; i < NUM_GENES; i++)
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
	kbdmore = false;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static void handle_mutation_off(bool &kbdmore)
{
	g_evolving_flags = EVOLVE_NONE;
	g_viewWindow.Hide();
	kbdmore = false;
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

ApplicationStateType evolver_menu_switch(int &kbdchar, bool &julia_entered_from_manelbrot, bool &kbdmore, bool &stacked)
{
	switch (kbdchar)
	{
	case 't':                    /* new fractal type             */
		if (handle_fractal_type(julia_entered_from_manelbrot))
		{
			return APPSTATE_IMAGE_START;
		}
		break;

	case 'x':                    /* invoke options screen        */
	case 'y':
	case 'p':                    /* passes options      */
	case 'z':                    /* type specific parms */
	case 'g':
	case FIK_CTL_E:
	case FIK_SPACE:
		handle_evolver_options(kbdchar, kbdmore);
		break;

	case 'b':
		handle_evolver_exit(kbdmore);
		break;

	case 'f':
		return handle_toggle_float();

	case '\\':
	case FIK_CTL_BACKSLASH:
	case 'h':
	case FIK_BACKSPACE:
		return handle_evolver_history(kbdchar);

	case 'c':
	case '+':
	case '-':
		return handle_color_cycling(kbdchar);

	case 'e':
		return handle_color_editing(kbdmore);

	case 's':
		return handle_evolver_save_to_disk();

	case 'r':
		return handle_restore_from(julia_entered_from_manelbrot, kbdchar, stacked);

	case FIK_ENTER:
	case FIK_ENTER_2:
		handle_zoom_in(kbdmore);
		break;

	case FIK_CTL_ENTER:
	case FIK_CTL_ENTER_2:
		handle_zoom_out(kbdmore);
		break;

	case FIK_INSERT:
		return handle_restart();

	case FIK_LEFT_ARROW:
	case FIK_RIGHT_ARROW:
	case FIK_UP_ARROW:
	case FIK_DOWN_ARROW:
		move_zoombox(kbdchar);
		break;

	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_DOWN_ARROW:
		handle_evolver_move_selection(kbdchar);
		break;

	case FIK_CTL_HOME:
	case FIK_CTL_END:
		handle_zoom_skew(kbdchar == FIK_CTL_HOME);
		break;

	case FIK_CTL_PAGE_UP:
	case FIK_CTL_PAGE_DOWN:
		handle_evolver_param_zoom(FIK_CTL_PAGE_UP == kbdchar);
		break;

	case FIK_PAGE_UP:
	case FIK_PAGE_DOWN:
		handle_evolver_zoom(FIK_PAGE_UP == kbdchar);
		break;

	case FIK_CTL_MINUS:
	case FIK_CTL_PLUS:
		handle_z_rotate(FIK_CTL_MINUS == kbdchar);
		break;

	case FIK_CTL_INSERT:
	case FIK_CTL_DEL:
		handle_box_color(FIK_CTL_INSERT == kbdchar);
		break;

	/* grabbed a couple of video mode keys, user can change to these using
		delete and the menu if necessary */

	case FIK_F2: /* halve mutation params and regen */
	case FIK_F3: /*double mutation parameters and regenerate */
		handle_evolver_mutation(FIK_F2 == kbdchar, kbdmore);
		break;

	case FIK_F4: /*decrement  gridsize and regen */
	case FIK_F5: /* increment gridsize and regen */
		handle_evolver_grid_size(FIK_F4 == kbdchar, kbdmore);
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
		handle_mutation_level(true, kbdchar - FIK_ALT_1 + 1, kbdmore);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		handle_mutation_level(true, kbdchar - int('1') + 1, kbdmore);
		break;

	case '0':
		handle_mutation_off(kbdmore);
		break;

	case FIK_DELETE:
		handle_select_video(kbdchar);
		/* fall through */

	default:             /* other (maybe valid Fn key */
		return handle_video_mode(kbdchar, kbdmore);
	}                            /* end of the big evolver switch */

	return APPSTATE_NO_CHANGE;
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{
	int vertical = 0;
	int horizontal = 0;
	int getmore = 1;
	// TODO: refactor to IInputContext
	while (getmore)
	{
		switch (keynum)
		{
		case FIK_LEFT_ARROW:
			--horizontal;
			break;
		case FIK_RIGHT_ARROW:
			++horizontal;
			break;
		case FIK_UP_ARROW:
			--vertical;
			break;
		case FIK_DOWN_ARROW:
			++vertical;
			break;
		case FIK_CTL_LEFT_ARROW:
			horizontal -= 8;
			break;
		case FIK_CTL_RIGHT_ARROW:
			horizontal += 8;
			break;
		case FIK_CTL_UP_ARROW:
			vertical -= 8;
			break;
		case FIK_CTL_DOWN_ARROW:
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
	if (g_zoomBox.count())
	{
		zoom_box_move(double(horizontal)/g_dx_size, double(vertical)/g_dy_size);
	}
}

void clear_zoom_box()
{
	g_z_width = 0;
	zoom_box_draw(false);
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
		copy_bf(g_escape_time_state.m_grid_bf.x_min(), g_sx_min_bf);
		copy_bf(g_escape_time_state.m_grid_bf.x_max(), g_sx_max_bf);
		copy_bf(g_escape_time_state.m_grid_bf.y_min(), g_sy_min_bf);
		copy_bf(g_escape_time_state.m_grid_bf.y_max(), g_sy_max_bf);
		copy_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_sx_3rd_bf);
		copy_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_sy_3rd_bf);
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
