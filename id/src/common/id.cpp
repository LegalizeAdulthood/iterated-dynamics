// i d . c p p
//
// Iterated Dynamics -- The Next Generation Ultimate Fractal Generator
//
#include <csignal>
#include <cstdarg>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "Browse.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "CommandParser.h"
#include "decoder.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "Externals.h"
#include "filesystem.h"
#include "Formula.h"
#include "idhelp.h"
#include "idmain.h"
#include "IteratedDynamics.h"
#include "IteratedDynamicsImpl.h"
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
#include "ViewWindow.h"

static void set_exe_path(const char *path);
static void check_same_name();

/* #include hierarchy is a follows:
		Each module should include port.h as the first specific
				include. port.h includes <stdlib.h>, <math.h>, <float.h>
		Most modules should include prototyp.h, which incorporates by
				direct or indirect reference the following header files:
				mpmath.h
				cmplx.h
				id.h
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
long g_timer_interval;        // timer(...) start & total
std::string g_fract_dir1;
std::string g_fract_dir2;
boost::filesystem::path g_exe_path;

/*
	the following variables are out here only so
	that the calculate_fractal() and assembler routines can get at them easily
*/
int     textsafe2;              // textsafe override from g_video_table
bool g_ok_to_print;              // 0 if printf() won't work
int     g_screen_width;
int		g_screen_height;          // # of dots on the physical screen
int     g_screen_x_offset;
int		g_screen_y_offset;          // physical top left of logical screen
int     g_x_dots;
int		g_y_dots;           // # of dots on the logical screen
double  g_dx_size;
double	g_dy_size;         // g_x_dots-1, g_y_dots-1
int     g_colors = 256;           // maximum colors available
long    g_max_iteration;                  // try this many iterations
int     g_z_rotate;                // zoombox rotation
double  g_zbx;
double	g_zby;                // topleft of zoombox
double  g_z_width;
double	g_z_depth;
double	g_z_skew;    // zoombox size & shape
int     g_fractal_type;               // if == 0, use Mandelbrot
long    g_c_real;
long	g_c_imag;           // real, imag'ry parts of C
long    g_delta_min;                 // for calcfrac/calculate_mandelbrot_l
double  g_delta_min_fp;                // same as a double
double  g_parameters[MAX_PARAMETERS];       // parameters
double  g_potential_parameter[3];            // three potential parameters
long    g_fudge;                  // 2**fudgefactor
long    g_attractor_radius_l;               // finite attractor radius
double  g_attractor_radius_fp;               // finite attractor radius
int     g_bit_shift;               // fudgefactor

bool g_has_inverse = false;
// note that integer grid is set when g_integer_fractal && !invert;
// otherwise the floating point grid is set; never both at once
long    *g_x0_l;
long	*g_y0_l;     // x, y grid
long    *g_x1_l;
long	*g_y1_l;     // adjustment for rotate
// note that g_x1_l & g_y1_l values can overflow into sign bit; since
// they're used only to add to g_x0_l/g_y0_l, 2s comp straightens it out
double	*g_x0;
double	*g_y0;      // floating pt equivs
double	*g_x1;
double	*g_y1;
int     g_integer_fractal;         // true if fractal uses integer math
// usr_xxx is what the user wants, vs what we may be forced to do
CalculationMode g_user_standard_calculation_mode;
int     g_user_periodicity_check;
long    g_user_distance_test;
bool g_user_float_flag;
int		g_max_history = 10;
// variables defined by the command line/files processor
bool g_compare_gif = false;					// compare two gif files flag
TimedSaveType g_timed_save = TIMEDSAVE_DONE;// when doing a timed save
int     g_resave_mode = RESAVE_NO;			// tells encoder not to incr filename
bool g_started_resaves = false;				// but incr on first resave
bool g_tab_display_enabled = true;			// tab display enabled
// for historical reasons (before rotation):
// top    left  corner of screen is (g_xx_min, g_yy_max)
// bottom left  corner of screen is (g_xx_3rd, g_yy_3rd)
// bottom right corner of screen is (g_xx_max, g_yy_min)
long    g_x_min;
long	g_x_max;
long	g_y_min;
long	g_y_max;
long	g_x_3rd;
long	g_y_3rd;  // integer equivs
double  g_sx_min;
double	g_sx_max;
double	g_sy_min;
double	g_sy_max;
double	g_sx_3rd;
double	g_sy_3rd; // displayed screen corners
double  g_plot_mx1;
double	g_plot_mx2;
double	g_plot_my1;
double	g_plot_my2;     // real->screen multipliers
long	g_calculation_time;
int		g_max_colors;                         // maximum palette size
bool g_zoom_off;                     // = 0 when zoom is disabled
std::string g_file_name_stack[16];		// array of file names used while browsing
int		g_name_stack_ptr;

UserInterfaceState g_ui_state;

enum
{
	CONTINUE = 4
};

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
		if (key == 'o' || key == 'O')
		{
			driver_get_key();
			if (!driver_diskp())
			{
				g_show_orbit = !g_show_orbit;
			}
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

// timer type values
enum TimerType
{
	TIMER_ENGINE	= 0,
	TIMER_DECODER	= 1,
	TIMER_ENCODER	= 2
};

/* timer function:
	timer(TIMER_ENGINE, fractal())		fractal engine
	timer(TIMER_DECODER, 0, int width)	decoder
	timer(TIMER_ENCODER)					encoder
*/
static int timer(TimerType timertype, int (*engine)(), ...)
{
	va_list arg_marker;  // variable arg list
	va_start(arg_marker, engine);

	bool do_bench = g_timer_flag; // record time?
	if (timertype == TIMER_ENCODER)
	{
		do_bench = (DEBUGMODE_TIME_ENCODER == g_debug_mode);
	}
	std::ofstream benchmark_file;
	if (do_bench)
	{
		benchmark_file.open((g_work_dir / "bench.txt").string().c_str(), std::ios::ate);
		if (!benchmark_file.is_open())
		{
			do_bench = false;
		}
	}
	g_timer_start = clock_ticks();
	int out = 0;
	switch (timertype)
	{
	case TIMER_ENGINE:
		out = engine();
		break;
	case TIMER_DECODER:
		out = int(decoder(short(va_arg(arg_marker, int)))); // not indirect, safer with overlays
		break;
	case TIMER_ENCODER:
		out = encoder();            // not indirect, safer with overlays
		break;
	}
	// next assumes CLK_TCK is 10^n, n >= 2
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

bool operator==(const VIDEOINFO &lhs, const VIDEOINFO &rhs)
{
	return strcmp(lhs.name, rhs.name) == 0
		&& strcmp(lhs.comment, rhs.comment) == 0
		&& lhs.keynum == rhs.keynum
		&& lhs.x_dots == rhs.x_dots
		&& lhs.y_dots == rhs.y_dots
		&& lhs.colors == rhs.colors
		&& lhs.driver == rhs.driver;
}

static void set_exe_path(const char *path)
{
	g_exe_path = boost::filesystem::path(path).branch_path();
}

static void check_same_name()
{
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char path[FILE_MAX_PATH];
	split_path(g_save_name, drive, dir, fname, ext);
	if (strcmp(fname, "fract001"))
	{
		make_path(path, drive, dir, fname, "gif");
		if (!exists(path))
		{
			exit(0);
		}
	}
}

class ProductionIteratedDynamicsApp : public IteratedDynamicsApp
{
public:
	virtual ~ProductionIteratedDynamicsApp() {}

	virtual void set_exe_path(const char *path)
	{ ::set_exe_path(path); }
	virtual IteratedDynamicsApp::SignalHandler *signal(int number, SignalHandler *handler)
	{ return ::signal(number, handler); }
	virtual void InitMemory()
	{ return ::InitMemory(); }
	virtual void init_failure(std::string const &message)
	{ ::init_failure(message); }
	virtual void init_help()
	{ ::init_help(); }
	virtual void save_parameter_history()
	{ ::save_parameter_history(); }
	virtual ApplicationStateType big_while_loop(bool &keyboardMore, bool &screenStacked, bool resumeFlag)
	{ return ::big_while_loop(keyboardMore, screenStacked, resumeFlag); }
	virtual void srand(unsigned int seed)
	{ ::srand(seed); }
	virtual void command_files(int argc, char **argv)
	{ ::command_files(argc, argv); }
	virtual void pause_error(int action)
	{ ::pause_error(action); }
	virtual int init_msg(const char *cmdstr, const char *bad_filename, int mode)
	{ return ::init_msg(cmdstr, bad_filename, mode); }
	virtual void history_allocate()
	{ ::history_allocate(); }
	virtual void check_same_name()
	{ ::check_same_name(); }
	virtual void intro()
	{ ::intro(); }
	virtual void goodbye()
	{ ::goodbye(); }
	virtual void set_if_old_bif()
	{ ::set_if_old_bif(); }
	virtual void set_help_mode(int new_mode)
	{ ::set_help_mode(new_mode); }
	virtual int get_commands()
	{ return ::get_commands(); }
	virtual int get_fractal_type()
	{ return ::get_fractal_type(); }
	virtual int get_toggles()
	{ return ::get_toggles(); }
	virtual int get_toggles2()
	{ return ::get_toggles2(); }
	virtual int get_fractal_parameters(bool type_specific)
	{ return ::get_fractal_parameters(type_specific); }
	virtual int get_view_params()
	{ return ::get_view_params(); }
	virtual int get_fractal_3d_parameters()
	{ return ::get_fractal_3d_parameters(); }
	virtual int get_command_string()
	{ return ::get_command_string(); }
	virtual int open_drivers(int &argc, char **argv)
	{ return DriverManager::open_drivers(argc, argv); }
	virtual void exit(int code)
	{ ::exit(code); }
	virtual int main_menu(bool full_menu)
	{ return ::main_menu(full_menu); }
	virtual int select_video_mode(int curmode)
	{ return ::select_video_mode(curmode); }
	virtual int check_video_mode_key(int k)
	{ return ::check_video_mode_key(k); }
	virtual int read_overlay()
	{ return ::read_overlay(); }
	virtual int clock_ticks()
	{ return ::clock_ticks(); }
	virtual int get_a_filename(const std::string &heading, std::string &fileTemplate, std::string &filename)
	{ return ::get_a_filename(heading, fileTemplate, filename); }
};

int application_main(int argc, char **argv)
{
	ProductionIteratedDynamicsApp app;
	return IteratedDynamicsImpl(app, DriverManager::current(), g_externs, g_, argc, argv).Main();
}
