/*
		FRACTINT - The Ultimate Fractal Generator
                        Main Routine
*/
#include <fstream>
#include <string>

#include <string.h>
#include <time.h>
#include <signal.h>
/* for getcwd() */
#if defined(LINUX)
#include <unistd.h>
#endif
#include <stdarg.h>
#include <ctype.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "Browse.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "decoder.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "fihelp.h"
#include "filesystem.h"
#include "fimain.h"
#include "fracsubr.h"
#include "framain2.h"
#include "history.h"
#include "intro.h"
#include "loadfile.h"
#include "miscovl.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"

#include "SoundState.h"
#include "CommandParser.h"

/* #include hierarchy for fractint is a follows:
		Each module should include port.h as the first fractint specific
				include. port.h includes <stdlib.h>, <math.h>, <float.h>
		Most modules should include prototyp.h, which incorporates by
				direct or indirect reference the following header files:
				mpmath.h
				cmplx.h
				fractint.h
				big.h
				biginit.h
				helpcom.h
				externs.h
		Other modules may need the following, which must be included
				separately:
				fractype.h
				helpdefs.h
				lsys.y
		If included separately from prototyp.h, big.h includes cmplx.h
		and biginit.h; and mpmath.h includes cmplx.h
*/

VIDEOINFO g_video_entry;
long g_timer_start;
long g_timer_interval;        /* timer(...) start & total */
int     g_adapter;                /* Video Adapter chosen from list in ...h */
std::string g_fract_dir1;
std::string g_fract_dir2;
/*
	the following variables are out here only so
	that the calculate_fractal() and assembler routines can get at them easily
*/
int     textsafe2;              /* textsafe override from g_video_table */
bool g_ok_to_print;              /* 0 if printf() won't work */
int     g_screen_width;
int		g_screen_height;          /* # of dots on the physical screen    */
int     g_sx_offset;
int		g_sy_offset;          /* physical top left of logical screen */
int     g_x_dots;
int		g_y_dots;           /* # of dots on the logical screen     */
double  g_dx_size;
double	g_dy_size;         /* g_x_dots-1, g_y_dots-1         */
int     g_colors = 256;           /* maximum colors available */
long    g_max_iteration;                  /* try this many iterations */
int     g_z_rotate;                /* zoombox rotation         */
double  g_zbx;
double	g_zby;                /* topleft of zoombox       */
double  g_z_width;
double	g_z_depth;
double	g_z_skew;    /* zoombox size & shape     */
int     g_fractal_type;               /* if == 0, use Mandelbrot  */
CalculationMode g_standard_calculation_mode;            /* '1', '2', 'g', 'b'       */
long    g_c_real;
long	g_c_imag;           /* real, imag'ry parts of C */
long    g_delta_min;                 /* for calcfrac/calculate_mandelbrot_l */
double  g_delta_min_fp;                /* same as a double         */
double  g_parameters[MAX_PARAMETERS];       /* parameters               */
double  g_potential_parameter[3];            /* three potential parameters*/
long    g_fudge;                  /* 2**fudgefactor           */
long    g_attractor_radius_l;               /* finite attractor radius  */
double  g_attractor_radius_fp;               /* finite attractor radius  */
int     g_bit_shift;               /* fudgefactor              */

bool g_has_inverse = false;
/* note that integer grid is set when g_integer_fractal && !invert;    */
/* otherwise the floating point grid is set; never both at once     */
long    *g_x0_l;
long	*g_y0_l;     /* x, y grid                */
long    *g_x1_l;
long	*g_y1_l;     /* adjustment for rotate    */
/* note that g_x1_l & g_y1_l values can overflow into sign bit; since     */
/* they're used only to add to g_x0_l/g_y0_l, 2s comp straightens it out  */
double	*g_x0;
double	*g_y0;      /* floating pt equivs */
double	*g_x1;
double	*g_y1;
int     g_integer_fractal;         /* true if fractal uses integer math */
/* usr_xxx is what the user wants, vs what we may be forced to do */
CalculationMode g_user_standard_calculation_mode;
int     g_user_periodicity_check;
long    g_user_distance_test;
bool g_user_float_flag;
bool g_view_window;             /* 0 for full screen, 1 for window */
float   g_view_reduction;          /* window auto-sizing */
bool g_view_crop;               /* nonzero to crop default coords */
float   g_final_aspect_ratio;       /* for view shape and rotation */
int     g_view_x_dots;
int		g_view_y_dots;    /* explicit view sizing */
int		g_max_history = 10;
/* variables defined by the command line/files processor */
bool g_compare_gif = false;					/* compare two gif files flag */
TimedSaveType g_timed_save = TIMEDSAVE_DONE;/* when doing a timed save */
int     g_resave_mode = RESAVE_NO;			/* tells encoder not to incr filename */
bool g_started_resaves = false;				/* but incr on first resave */
bool g_tab_display_enabled = true;			/* tab display enabled */
/* for historical reasons (before rotation):         */
/*    top    left  corner of screen is (g_xx_min, g_yy_max) */
/*    bottom left  corner of screen is (g_xx_3rd, g_yy_3rd) */
/*    bottom right corner of screen is (g_xx_max, g_yy_min) */
long    g_x_min;
long	g_x_max;
long	g_y_min;
long	g_y_max;
long	g_x_3rd;
long	g_y_3rd;  /* integer equivs           */
double  g_sx_min;
double	g_sx_max;
double	g_sy_min;
double	g_sy_max;
double	g_sx_3rd;
double	g_sy_3rd; /* displayed screen corners */
double  g_plot_mx1;
double	g_plot_mx2;
double	g_plot_my1;
double	g_plot_my2;     /* real->screen multipliers */
int		g_calculation_status = CALCSTAT_NO_FRACTAL;
					  /* -1 no fractal                   */
					  /*  0 parms changed, recalc reqd   */
					  /*  1 actively calculating         */
					  /*  2 interrupted, resumable       */
					  /*  3 interrupted, not resumable   */
					  /*  4 completed                    */
long	g_calculation_time;
int		g_max_colors;                         /* maximum palette size */
bool g_zoom_off;                     /* = 0 when zoom is disabled    */
int		g_save_dac;                     /* save-the-Video DAC flag      */
std::string g_file_name_stack[16];		/* array of file names used while browsing */
int		g_name_stack_ptr;
double	g_too_small;
int		g_cross_hair_box_size;

UserInterfaceState g_ui_state;

boost::filesystem::path g_exe_path;

#define CONTINUE          4

class IteratedDynamics
{
public:
	IteratedDynamics();
	~IteratedDynamics();
	void Main(int argc, char **argv);

private:
	void Initialize(int argc, char **argv);
	void Restart(int argc, char *argv[], bool &screen_stacked);
	void RestoreStart(bool &screen_stacked, bool &resume_flag);
	void ImageStart(bool &screen_stacked, bool &resume_flag);

	ApplicationStateType m_state;
};

static IteratedDynamics s_id;

IteratedDynamics::IteratedDynamics()
	: m_state(APPSTATE_RESTART)
{
}

IteratedDynamics::~IteratedDynamics()
{
}

void check_same_name()
{
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char path[FILE_MAX_PATH];
	split_path(g_save_name.c_str(), drive, dir, fname, ext);
	if (strcmp(fname, "fract001"))
	{
		make_path(path, drive, dir, fname, "gif");
		if (!exists(path))
		{
			exit(0);
		}
	}
}

/* Do nothing if math error */
static void my_floating_point_err(int sig)
{
	if (sig != 0)
	{
		g_overflow = true;
	}
}

static void set_exe_path(const char *path)
{
	g_exe_path = path;
	if (g_exe_path.has_relative_path())
	{
		/* relative path */
		g_exe_path = boost::filesystem::current_path() / g_exe_path;
	}
}

void IteratedDynamics::Restart(int argc, char *argv[], bool &screen_stacked)
{
	g_browse_state.set_auto_browse(false);
	g_browse_state.set_check_type(true);
	g_browse_state.set_check_parameters(true);
	g_ui_state.double_caution = true;
	g_browse_state.set_sub_images(true);
	g_too_small = 6;
	g_cross_hair_box_size   = 3;
	g_browse_state.set_mask("*.gif");
	g_browse_state.set_name("");
	g_name_stack_ptr = -1; /* init loaded files stack */

	g_evolving_flags = EVOLVE_NONE;
	g_parameter_range_x = 4;
	g_parameter_offset_x = -2.0;
	g_new_parameter_offset_x = -2.0;
	g_parameter_range_y = 3;
	g_parameter_offset_y = -1.5;
	g_new_parameter_offset_y = -1.5;
	g_discrete_parameter_offset_x = 0;
	g_discrete_parameter_offset_y = 0;
	g_grid_size = 9;
	g_fiddle_factor = 1;
	g_fiddle_reduction = 1.0;
	g_this_generation_random_seed = (unsigned int)clock_ticks();
	srand(g_this_generation_random_seed);
	g_start_show_orbit = false;
	g_show_dot = -1; /* turn off g_show_dot if entered with <g> command */
	g_calculation_status = CALCSTAT_NO_FRACTAL;                    /* no active fractal image */

	command_files(argc, argv);         /* process the command-line */
	pause_error(PAUSE_ERROR_NO_BATCH); /* pause for error msg if not batch */
	init_msg("", 0, 0);  /* this causes driver_get_key if init_msg called on runup */

	history_allocate();

	if (DEBUGMODE_ABORT_SAVENAME == g_debug_mode && g_initialize_batch == INITBATCH_NORMAL)   /* abort if savename already exists */
	{
		check_same_name();
	}
	driver_window();
	memcpy(g_old_dac_box, g_dac_box, 256*3);      /* save in case colors= present */

	driver_set_for_text();                      /* switch to text mode */
	g_save_dac = SAVEDAC_NO;                         /* don't save the VGA DAC */

	g_max_colors = 256;
	g_max_input_counter = 80;				/* check the keyboard this often */

	if ((g_show_file != SHOWFILE_PENDING) && g_initial_adapter < 0)
	{
		// TODO: refactor to IInputContext
		intro();                          /* display the credits screen */
		if (driver_key_pressed() == FIK_ESC)
		{
			driver_get_key();
			goodbye();
		}
	}

	g_browse_state.set_browsing(false);

	if (!g_function_preloaded)
	{
		set_if_old_bif();
	}
	screen_stacked = false;

	m_state = APPSTATE_RESTORE_START;
}

void IteratedDynamics::RestoreStart(bool &screen_stacked, bool &resume_flag)
{
	if (g_color_preloaded)
	{
		memcpy(g_dac_box, g_old_dac_box, 256*3);   /* restore in case colors= present */
	}

	driver_set_mouse_mode(LOOK_MOUSE_NONE);			/* ignore mouse */

	/* image is to be loaded */
	while (g_show_file == SHOWFILE_PENDING || g_show_file == SHOWFILE_CANCELLED)
	{
		char *hdg;
		g_tab_display_enabled = false;
		if (!g_browse_state.browsing())     /*RB*/
		{
			if (g_overlay_3d)
			{
				hdg = "Select File for 3D Overlay";
				set_help_mode(HELP3DOVLY);
			}
			else if (g_display_3d)
			{
				hdg = "Select File for 3D Transform";
				set_help_mode(HELP3D);
			}
			else
			{
				hdg = "Select File to Restore";
				set_help_mode(HELPSAVEREST);
			}
			if (g_show_file == SHOWFILE_CANCELLED && get_a_filename(hdg, g_gif_mask, g_read_name) < 0)
			{
				g_show_file = SHOWFILE_DONE;               /* cancelled */
				g_initial_adapter = -1;
				break;
			}

			g_name_stack_ptr = 0; /* 'r' reads first filename for browsing */
			g_file_name_stack[g_name_stack_ptr] = g_browse_state.name();
		}

		g_evolving_flags = EVOLVE_NONE;
		g_view_window = false;
		g_show_file = SHOWFILE_DONE;
		set_help_mode(-1);
		g_tab_display_enabled = true;
		if (screen_stacked)
		{
			driver_discard_screen();
			driver_set_for_text();
			screen_stacked = false;
		}
		if (read_overlay() == 0)       /* read hdr, get video mode */
		{
			break;                      /* got it, exit */
		}
		g_show_file = g_browse_state.browsing() ? SHOWFILE_DONE : SHOWFILE_CANCELLED;
	}

	set_help_mode(HELPMENU);                 /* now use this help mode */
	g_tab_display_enabled = true;
	driver_set_mouse_mode(LOOK_MOUSE_NONE);                     /* ignore mouse */

	if (((g_overlay_3d && !g_initialize_batch) || screen_stacked) && g_initial_adapter < 0)        /* overlay command failed */
	{
		driver_unstack_screen();                  /* restore the graphics screen */
		screen_stacked = false;
		g_overlay_3d = 0;                    /* forget overlays */
		g_display_3d = DISPLAY3D_NONE;
		if (g_calculation_status == CALCSTAT_NON_RESUMABLE)
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
		resume_flag = true;
		m_state = APPSTATE_RESUME_LOOP;
	}

	g_save_dac = SAVEDAC_NO;                         /* don't save the VGA DAC */
	m_state = APPSTATE_IMAGE_START;
}

void IteratedDynamics::ImageStart(bool &screen_stacked, bool &resume_flag)
{
	if (screen_stacked)
	{
		driver_discard_screen();
		screen_stacked = false;
	}
	g_got_status = GOT_STATUS_NONE;                     /* for tab_display */

	if (g_show_file != SHOWFILE_PENDING)
	{
		if (g_calculation_status > CALCSTAT_PARAMS_CHANGED)              /* goto imagestart implies re-calc */
		{
			g_calculation_status = CALCSTAT_PARAMS_CHANGED;
		}
	}

	if (g_initialize_batch == INITBATCH_NONE)
	{
		driver_set_mouse_mode(-FIK_PAGE_UP);           /* just mouse left button, == pgup */
	}

	g_cycle_limit = g_initial_cycle_limit;         /* default cycle limit   */
	g_adapter = g_initial_adapter;                  /* set the video adapter up */
	g_initial_adapter = -1;                       /* (once)                   */

	while (g_adapter < 0)                /* cycle through instructions */
	{
		if (g_initialize_batch)                          /* batch, nothing to do */
		{
			g_initialize_batch = INITBATCH_BAILOUT_INTERRUPTED; /* exit with error condition set */
			goodbye();
		}
		int kbdchar = main_menu(false);
		if (kbdchar == FIK_INSERT) /* restart pgm on Insert Key  */
		{
			m_state = APPSTATE_RESTART;
			return;
		}
		if (kbdchar == FIK_DELETE)                    /* select video mode list */
		{
			kbdchar = select_video_mode(-1);
		}
		g_adapter = check_video_mode_key(0, kbdchar);
		if (g_adapter >= 0)
		{
			break;                                 /* got a video mode now */
		}
#ifndef XFRACT
		if ('A' <= kbdchar && kbdchar <= 'Z')
		{
			kbdchar = tolower(kbdchar);
		}
#endif
		if (kbdchar == 'd')  /* shell to DOS */
		{
			driver_set_clear();
			driver_shell();
			m_state = APPSTATE_IMAGE_START;
			return;
		}

#ifndef XFRACT
		if (kbdchar == '@' || kbdchar == '2')  /* execute commands */
#else
		if (kbdchar == FIK_F2 || kbdchar == '@')  /* We mapped @ to F2 */
#endif
		{
			if ((get_commands() & COMMANDRESULT_3D_YES) == 0)
			{
				m_state = APPSTATE_IMAGE_START;
				return;
			}
			kbdchar = '3';                         /* 3d=y so fall thru '3' code */
		}
#ifndef XFRACT
		if (kbdchar == 'r' || kbdchar == '3' || kbdchar == '#')
#else
		if (kbdchar == 'r' || kbdchar == '3' || kbdchar == FIK_F3)
#endif
		{
			g_display_3d = DISPLAY3D_NONE;
			if (kbdchar == '3' || kbdchar == '#' || kbdchar == FIK_F3)
			{
				g_display_3d = DISPLAY3D_YES;
			}
			if (g_color_preloaded)
			{
				memcpy(g_old_dac_box, g_dac_box, 256*3);     /* save in case colors= present */
			}
			driver_set_for_text(); /* switch to text mode */
			g_show_file = SHOWFILE_CANCELLED;
			m_state = APPSTATE_RESTORE_START;
			return;
		}
		if (kbdchar == 't')  /* set fractal type */
		{
			g_julibrot = false;
			get_fractal_type();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'x')  /* generic toggle switch */
		{
			get_toggles();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'y')  /* generic toggle switch */
		{
			get_toggles2();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'z')  /* type specific parms */
		{
			get_fractal_parameters(true);
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'v')  /* view parameters */
		{
			get_view_params();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == FIK_CTL_B)  /* ctrl B = browse parms*/
		{
			get_browse_parameters();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == FIK_CTL_F)  /* ctrl f = sound parms*/
		{
			g_sound_state.get_parameters();
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'f')  /* floating pt toggle */
		{
			g_user_float_flag = !g_user_float_flag;
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'i')  /* set 3d fractal parms */
		{
			get_fractal_3d_parameters(); /* get the parameters */
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		if (kbdchar == 'g')
		{
			get_command_string(); /* get command string */
			m_state = APPSTATE_IMAGE_START;
			return;
		}
		/* buzzer(2); */                          /* unrecognized key */
	}

	g_zoom_off = true;                 /* zooming is enabled */
	set_help_mode(HELPMAIN);         /* now use this help mode */
	resume_flag = false;  /* allows taking goto inside big_while_loop() */

	m_state = APPSTATE_RESUME_LOOP;
}

void str_assign(std::string &str, const char *ptr)
{
	if (!ptr)
	{
		str = "";
	}
	else
	{
		str = ptr;
	}
}

void IteratedDynamics::Initialize(int argc, char **argv)
{
	set_exe_path(argv[0]);

	str_assign(g_fract_dir1, getenv("FRACTDIR"));
	if (g_fract_dir1.length() == 0)
	{
		g_fract_dir1 = ".";
	}
#ifdef SRCDIR
	g_fract_dir2 = SRCDIR;
#else
	g_fract_dir2 = ".";
#endif

	/* this traps non-math library floating point errors */
	signal(SIGFPE, my_floating_point_err);

	g_overflow = false;
	InitMemory();

	/* let drivers add their video modes */
	if (!DriverManager::open_drivers(argc, argv))
	{
		init_failure("Sorry, I couldn't find any working video drivers for your system\n");
		exit(-1);
	}
	init_help();
}

int application_main(int argc, char **argv)
{
	s_id.Main(argc, argv);
	return 0;
}

void IteratedDynamics::Main(int argc, char **argv)
{
	Initialize(argc, argv);

	bool resume_flag = false;
	bool screen_stacked = false;
	bool kbdmore;						/* continuation variable  */
	m_state = APPSTATE_RESTART;

	while (m_state != APPSTATE_NO_CHANGE)
	{
#if defined(_WIN32)
		_ASSERTE(_CrtCheckMemory());
#endif

		switch (m_state)
		{
		case APPSTATE_RESTART:
			Restart(argc, argv, screen_stacked);
			break;

		case APPSTATE_RESTORE_START:
			RestoreStart(screen_stacked, resume_flag);
			break;

		case APPSTATE_IMAGE_START:
			ImageStart(screen_stacked, resume_flag);
			break;

		case APPSTATE_RESUME_LOOP:
			save_parameter_history();
			m_state = big_while_loop(kbdmore, screen_stacked, resume_flag);
			break;
		}
	}
}

int check_key()
{
	// TODO: refactor to IInputContext
	int key = driver_key_pressed();
	if (key != 0)
	{
		if (g_show_orbit)
		{
			orbit_scrub();
		}
		if (key != 'o' && key != 'O')
		{
			return -1;
		}
		driver_get_key();
		if (!driver_diskp())
		{
			g_show_orbit = !g_show_orbit;
		}
	}
	return 0;
}

/* timer type values */
enum TimerType
{
	TIMER_ENGINE	= 0,
	TIMER_DECODER	= 1,
	TIMER_ENCODER	= 2
};

/* timer function:
	timer(TIMER_ENGINE, (*fractal)())		fractal engine
	timer(TIMER_DECODER, 0, int width)	decoder
	timer(TIMER_ENCODER)					encoder
*/
static int timer(TimerType timertype, int (*engine)(), ...)
{
	va_list arg_marker;  /* variable arg list */
	va_start(arg_marker, engine);

	bool do_bench = g_timer_flag; /* record time? */
	if (timertype == TIMER_ENCODER)
	{
		do_bench = (DEBUGMODE_TIME_ENCODER == g_debug_mode);
	}
	std::ofstream benchmark_file;
	if (do_bench)
	{
		benchmark_file.open((boost::filesystem::path(g_work_dir) / "bench.txt").string().c_str(), std::ios::ate);
		if (!benchmark_file)
		{
			do_bench = false;
		}
	}
	g_timer_start = clock_ticks();
	int out = 0;
	switch (timertype)
	{
	case TIMER_ENGINE:
		out = (*(int (*)()) engine)();
		break;
	case TIMER_DECODER:
		out = int(decoder(short(va_arg(arg_marker, int)))); /* not indirect, safer with overlays */
		break;
	case TIMER_ENCODER:
		out = encoder();            /* not indirect, safer with overlays */
		break;
	}
	/* next assumes CLK_TCK is 10^n, n >= 2 */
	g_timer_interval = (clock_ticks() - g_timer_start)/(CLK_TCK/100);

	if (do_bench)
	{
		time_t ltime;
		time(&ltime);
		char *timestring = ctime(&ltime);
		timestring[24] = 0; /*clobber newline in time string */
		switch (timertype)
		{
		case TIMER_DECODER:
			benchmark_file << "decode ";
			break;
		case TIMER_ENCODER:
			benchmark_file << "encode ";
			break;
		}
		benchmark_file << boost::format("%s type=%s resolution = %dx%d maxiter=%ld")
				% timestring
				% g_current_fractal_specific->name
				% g_x_dots
				% g_y_dots
				% g_max_iteration
			<< boost::format(" time= %ld.%02ld secs\n") % (g_timer_interval/100) % (g_timer_interval % 100);
		benchmark_file.close();
	}
	return out;
}

int timer_engine(int (*engine)())
{
	return timer(TIMER_ENGINE, engine);
}

int timer_decoder(int line_width)
{
	return timer(TIMER_DECODER, 0, line_width);
}

int timer_encoder()
{
	return timer(TIMER_ENCODER, 0);
}
