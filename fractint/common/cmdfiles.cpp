/*
		Command-line / Command-File Parser Routines
*/
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "Browse.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "fracsuba.h"
#include "fracsubr.h"
#include "fractalb.h"
#include "fractals.h"
#include "history.h"
#include "loadfile.h"
#include "loadmap.h"
#include "miscovl.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"

#include "CommandParser.h"
#include "EscapeTime.h"
#include "FiniteAttractor.h"
#include "Formula.h"
#include "SoundState.h"
#include "ThreeDimensionalState.h"

#define INIT_GIF87      0						/* Turn on GIF 89a processing  */

/* variables defined by the command line/files processor */
int     g_stop_pass = 0;						/* stop at this guessing pass early */
int     g_pseudo_x = 0;							/* g_x_dots to use for video independence */
int     g_pseudo_y = 0;							/* g_y_dots to use for video independence */
int     g_bf_digits = 0;						/* digits to use (force) for g_bf_math */
int     g_show_dot = -1;						/* color to show crawling graphics cursor */
int     g_size_dot;								/* size of dot crawling cursor */
char    g_record_colors;						/* default PAR color-writing method */
char    g_auto_show_dot = 0;					/* dark, medium, bright */
bool g_start_show_orbit = false;				/* show orbits on at start of fractal */
char    g_read_name[FILE_MAX_PATH];				/* name of fractal input file */
char    g_temp_dir[FILE_MAX_DIR] = {""};		/* name of temporary directory */
char    g_work_dir[FILE_MAX_DIR] = {""};		/* name of directory for misc files */
char    g_organize_formula_dir[FILE_MAX_DIR] = {""}; /*name of directory for orgfrm files*/
char    g_gif_mask[FILE_MAX_PATH] = {""};
char    g_save_name[FILE_MAX_PATH] = {"fract001"};  /* save files using this name */
char    g_autokey_name[FILE_MAX_PATH] = {"auto.key"}; /* record auto keystrokes here */
bool g_potential_flag = false;					/* continuous potential enabled? */
bool g_potential_16bit;							/* store 16 bit continuous potential values */
bool g_gif87a_flag;								/* 1 if GIF87a format, 0 otherwise */
bool g_dither_flag;								/* 1 if want to dither GIFs */
bool g_float_flag;
int     g_biomorph;								/* flag for g_biomorph */
int     g_user_biomorph;
int     g_force_symmetry;						/* force symmetry */
int     g_show_file;							/* zero if file display pending */
bool g_use_fixed_random_seed;
int g_random_seed;								/* Random number seeding flag and value */
int     g_decomposition[2];						/* Decomposition coloring */
long    g_distance_test;
int     g_distance_test_width;
bool g_fractal_overwrite = false;				/* 0 if file overwrite not allowed */
int     g_debug_mode;							/* internal use only - you didn't see this */
bool g_timer_flag;								/* you didn't see this, either */
int     g_cycle_limit;							/* color-rotator upper limit */
int     g_inside;								/* inside color: 1=blue     */
int     g_fill_color;							/* fillcolor: -1=normal     */
int     g_outside;								/* outside color    */
int     g_finite_attractor;						/* finite attractor logic */
int     g_display_3d;							/* 3D display flag: 0 = OFF */
int     g_overlay_3d;							/* 3D overlay flag: 0 = OFF */
int     g_init_3d[20];							/* '3d=nn/nn/nn/...' values */
bool g_check_current_dir;						/* flag to check current dir for files */
int     g_initialize_batch = INITBATCH_NONE;	/* 1 if batch run (no kbd)  */
int     g_save_time;							/* autosave minutes         */
ComplexD  g_initial_orbit_z;					/* initial orbitvalue */
InitialZType g_use_initial_orbit_z;				/* flag for g_initial_orbit_z */
int     g_initial_adapter;							/* initial video mode       */
int     g_initial_cycle_limit;					/* initial cycle limit      */
bool g_use_center_mag;							/* use center-mag corners   */
long    g_bail_out;								/* user input bailout value */
enum bailouts g_bail_out_test;					/* test used for determining bailout */
double  g_inversion[3];							/* radius, xcenter, ycenter */
int g_rotate_lo;
int g_rotate_hi;								/* cycling color range      */
int		*g_ranges;								/* iter->color ranges mapping */
int     g_ranges_length = 0;					/* size of ranges array     */
BYTE	*g_map_dac_box = NULL;					/* map= (default colors)    */
int     g_color_state;							/* 0, g_dac_box matches default (bios or map=) */
												/* 1, g_dac_box matches no known defined map   */
												/* 2, g_dac_box matches the g_color_file map      */
bool g_color_preloaded;							/* if g_dac_box preloaded for next mode select */
int     g_save_release;							/* release creating PAR file*/
bool g_dont_read_color = false;					/* flag for reading color from GIF */
double  g_math_tolerance[2] = {.05, .05};		/* For math transition */
bool g_targa_output = false;					/* 3D fullcolor flag */
bool g_true_color = false;						/* escape time truecolor flag */
bool g_true_mode_iterates = false;				/* truecolor iterates coloring scheme */
char    g_color_file[FILE_MAX_PATH];			/* from last <l> <s> or colors=@filename */
bool g_function_preloaded;						/* if function loaded for new bifurcations */
float   g_screen_aspect_ratio = DEFAULT_ASPECT_RATIO;	/* aspect ratio of the screen */
float   g_aspect_drift = DEFAULT_ASPECT_DRIFT;	/* how much drift is allowed and */
												/* still forced to g_screen_aspect_ratio  */
bool g_fast_restore = false;					/* true - reset viewwindows prior to a restore
													and do not display warnings when video
													mode changes during restore */
bool g_organize_formula_search = false;			/* 1 - user has specified a directory for
													Orgform formula compilation files */
int     g_orbit_save = ORBITSAVE_NONE;			/* for IFS and LORENZ to output acrospin file */
int		g_orbit_delay;							/* clock ticks delating orbit release */
int     g_transparent[2];						/* transparency min/max values */
long    g_log_palette_mode;						/* Logarithmic palette flag: 0 = no */
int     g_log_dynamic_calculate = LOGDYNAMIC_NONE;	/* calculate logmap on-the-fly */
bool g_log_automatic_flag = false;				/* auto calculate logmap */
bool g_no_bof = false;							/* Flag to make inside=bof options not duplicate bof images */
bool g_escape_exit_flag;						/* set to 1 to avoid the "are you sure?" screen */
bool g_command_initialize = true;				/* first time into command_files? */
FractalTypeSpecificData *g_current_fractal_specific = NULL;
char	g_l_system_filename[FILE_MAX_PATH];		/* file to find (type=)L-System's in */
char	g_l_system_name[ITEMNAMELEN + 1];		/* Name of L-System */
char	g_command_file[FILE_MAX_PATH];			/* file to find command sets in */
char	g_command_name[ITEMNAMELEN + 1];		/* Name of Command set */
char	g_command_comment[4][MAX_COMMENT];		/* comments for command set */
char	g_ifs_filename[FILE_MAX_PATH];			/* file to find (type=)IFS in */
char	g_ifs_name[ITEMNAMELEN + 1];			/* Name of the IFS def'n (if not null) */
struct search_path g_search_for;
float	*g_ifs_definition = NULL;				/* ifs parameters */
int		g_ifs_type;								/* 0 = 2d, 1 = 3d */
int		g_slides = SLIDES_OFF;					/* 1 autokey=play, 2 autokey=record */
BYTE	g_text_colors[]=
{
		BLUE*16 + L_WHITE,    /* C_TITLE           title background */
		BLUE*16 + L_GREEN,    /* C_TITLE_DEV       development vsn foreground */
		GREEN*16 + YELLOW,    /* C_HELP_HDG        help page title line */
		WHITE*16 + BLACK,     /* C_HELP_BODY       help page body */
		GREEN*16 + GRAY,      /* C_HELP_INSTR      help page instr at bottom */
		WHITE*16 + BLUE,      /* C_HELP_LINK       help page links */
		CYAN*16 + BLUE,       /* C_HELP_CURLINK    help page current link */
		WHITE*16 + GRAY,      /* C_PROMPT_BKGRD    prompt/choice background */
		WHITE*16 + BLACK,     /* C_PROMPT_TEXT     prompt/choice extra info */
		BLUE*16 + WHITE,      /* C_PROMPT_LO       prompt/choice text */
		BLUE*16 + L_WHITE,    /* C_PROMPT_MED      prompt/choice hdg2/... */
		BLUE*16 + YELLOW,     /* C_PROMPT_HI       prompt/choice hdg/cur/... */
		GREEN*16 + L_WHITE,   /* C_PROMPT_INPUT    full_screen_prompt input */
		CYAN*16 + L_WHITE,    /* C_PROMPT_CHOOSE   full_screen_prompt choice */
		MAGENTA*16 + L_WHITE, /* C_CHOICE_CURRENT  full_screen_choice input */
		BLACK*16 + WHITE,     /* C_CHOICE_SP_INSTR speed key bar & instr */
		BLACK*16 + L_MAGENTA, /* C_CHOICE_SP_KEYIN speed key value */
		WHITE*16 + BLUE,      /* C_GENERAL_HI      tab, thinking, IFS */
		WHITE*16 + BLACK,     /* C_GENERAL_MED */
		WHITE*16 + GRAY,      /* C_GENERAL_LO */
		BLACK*16 + L_WHITE,   /* C_GENERAL_INPUT */
		WHITE*16 + BLACK,     /* C_DVID_BKGRD      disk video */
		BLACK*16 + YELLOW,    /* C_DVID_HI */
		BLACK*16 + L_WHITE,   /* C_DVID_LO */
		RED*16 + L_WHITE,     /* C_STOP_ERR        stop message, error */
		GREEN*16 + BLACK,     /* C_STOP_INFO       stop message, info */
		BLUE*16 + WHITE,      /* C_TITLE_LOW       bottom lines of title screen */
		GREEN*16 + BLACK,     /* C_AUTHDIV1        title screen dividers */
		GREEN*16 + GRAY,      /* C_AUTHDIV2        title screen dividers */
		BLACK*16 + L_WHITE,   /* C_PRIMARY         primary authors */
		BLACK*16 + WHITE      /* C_CONTRIB         contributing authors */
};
char	g_make_par[] =          "makepar";

int  process_command(char *, int);

static int s_init_random_seed;
static bool s_initial_corners = false;
static bool s_initial_parameters = false;

static int  command_file(FILE *, int);
static int  next_command(char *, int, FILE *, char *, int *, int);
static int  next_line(FILE *, char *, int);
static void arg_error(const char *);
static void initialize_variables_once();
static void initialize_variables_restart();
static void initialize_variables_fractal();
static void initialize_variables_3d();
static void reset_ifs_definition();
static void parse_text_colors(char *value);
static int  parse_colors(char *value);
static int  get_bf(bf_t, char *);
static int is_a_big_float(char *str);

/*
		command_files(argc, argv) process the command-line arguments
				it also processes the 'sstools.ini' file and any
				indirect files ('fractint @myfile')
*/

/* This probably ought to go somewhere else, but it's used here.        */
/* get_power_10(x) returns the magnitude of x.  This rounds               */
/* a little so 9.95 rounds to 10, but we're using a binary base anyway, */
/* so there's nothing magic about changing to the next power of 10.     */
int get_power_10(LDBL x)
{
	char string[11]; /* space for "+x.xe-xxxx" */
	int p;

#ifdef USE_LONG_DOUBLE
	sprintf(string, "%+.1Le", x);
#else
	sprintf(string, "%+.1le", x);
#endif
	p = atoi(string + 5);
	return p;
}

int command_files(int argc, char **argv)
{
	int     i;
	char    curarg[141];
	char    tempstring[101];
	char    *sptr;
	FILE    *initfile;

	if (g_command_initialize) /* once per run initialization  */
	{
		initialize_variables_once();
	}
	initialize_variables_restart();                  /* <ins> key initialization */
	initialize_variables_fractal();                  /* image initialization */

	strcpy(curarg, "sstools.ini");
	find_path(curarg, tempstring); /* look for SSTOOLS.INI */
	if (tempstring[0] != 0)              /* found it! */
	{
		initfile = fopen(tempstring, "r");
		if (initfile != NULL)
		{
			command_file(initfile, CMDFILE_SSTOOLS_INI);           /* process it */
		}
	}

	for (i = 1; i < argc; i++)  /* cycle through args */
	{
#ifdef XFRACT
		/* Let the xfract code take a look at the argument */
		if (unixarg(argc, argv, &i))
		{
			continue;
		}
#endif
		strcpy(curarg, argv[i]);
		if (curarg[0] == ';')             /* start of comments? */
		{
			break;
		}
		if (curarg[0] != '@')  /* simple command? */
		{
			if (strchr(curarg, '=') == NULL)  /* not xxx = yyy, so check for gif */
			{
				strcpy(tempstring, curarg);
				if (has_extension(curarg) == NULL)
				{
					strcat(tempstring, ".gif");
				}
				initfile = fopen(tempstring, "rb");
				if (initfile != NULL)
				{
					fread(tempstring, 6, 1, initfile);
					if (tempstring[0] == 'G'
						&& tempstring[1] == 'I'
						&& tempstring[2] == 'F'
						&& tempstring[3] >= '8' && tempstring[3] <= '9'
						&& tempstring[4] >= '0' && tempstring[4] <= '9')
					{
						strcpy(g_read_name, curarg);
						g_browse_state.extract_read_name();
						g_show_file = SHOWFILE_PENDING;
						curarg[0] = 0;
					}
					fclose(initfile);
				}
			}
			if (curarg[0])
			{
				process_command(curarg, CMDFILE_AT_CMDLINE);           /* process simple command */
			}
		}
		else
		{
			sptr = strchr(curarg, '/');
			if (sptr != NULL)  /* @filename/setname? */
			{
				*sptr = 0;
				if (merge_path_names(g_command_file, &curarg[1], true) < 0)
				{
					init_msg("", g_command_file, 0);
				}
				strcpy(g_command_name, sptr + 1);
				if (find_file_item(g_command_file, g_command_name, &initfile, ITEMTYPE_PARAMETER) < 0 || initfile == NULL)
				{
					arg_error(curarg);
				}
				command_file(initfile, CMDFILE_AT_CMDLINE_SETNAME);
			}
			else  /* @filename */
			{
				initfile = fopen(&curarg[1], "r");
				if (initfile == NULL)
				{
					arg_error(curarg);
				}
				command_file(initfile, CMDFILE_AT_CMDLINE);
			}
		}
	}

	if (!g_command_initialize)
	{
		g_initial_adapter = -1;			/* don't set video when <ins> key used */
		g_show_file = SHOWFILE_DONE;	/* nor startup image file              */
	}

	init_msg("", NULL, 0);

	if (g_debug_mode != DEBUGMODE_NO_FIRST_INIT)
	{
		g_command_initialize = false;
	}
	/* PAR reads a file and sets color */
	g_dont_read_color = (g_color_preloaded && (g_show_file == SHOWFILE_PENDING));

	/*set structure of search directories*/
	strcpy(g_search_for.par, g_command_file);
	strcpy(g_search_for.frm, g_formula_state.get_filename());
	strcpy(g_search_for.lsys, g_l_system_filename);
	strcpy(g_search_for.ifs, g_ifs_filename);
	return 0;
}

int load_commands(FILE *infile)
{
	/* when called, file is open in binary mode, positioned at the */
	/* '(' or '{' following the desired parameter set's name       */
	int ret;
	s_initial_corners = false;
	s_initial_parameters = false; /* reset flags for type= */
	ret = command_file(infile, CMDFILE_AT_AFTER_STARTUP);
	/* PAR reads a file and sets color */
	g_dont_read_color = (g_color_preloaded && (g_show_file == SHOWFILE_PENDING));
	return ret;
}

static void initialize_variables_once()              /* once per run init */
{
	char *p;
	s_init_random_seed = (int) time(NULL);
	init_comments();
	p = getenv("TMP");
	if (p == NULL)
	{
		p = getenv("TEMP");
	}
	if (p != NULL)
	{
		if (is_a_directory(p) != 0)
		{
			strcpy(g_temp_dir, p);
			ensure_slash_on_directory(g_temp_dir);
		}
	}
	else
	{
		*g_temp_dir = 0;
	}
}

static void initialize_variables_restart()          /* <ins> key init */
{
	int i;
	g_record_colors = 'a';                  /* don't use mapfiles in PARs */
	g_save_release = g_release;            /* this release number */
	g_gif87a_flag = false;            /* turn on GIF89a processing */
	g_dither_flag = false;                     /* no dithering */
	g_ui_state.ask_video = true;                        /* turn on video-prompt flag */
	g_fractal_overwrite = false;                 /* don't overwrite           */
	g_sound_state.set_speaker_beep();		/* sound is on to PC speaker */
	g_initialize_batch = INITBATCH_NONE;			/* not in batch mode         */
	g_check_current_dir = false;		/* flag to check current dir for files */
	g_save_time = 0;                    /* no auto-save              */
	g_initial_adapter = -1;                       /* no initial video mode     */
	g_view_window = false;
	g_view_reduction = 4.2f;
	g_view_crop = true;
	g_final_aspect_ratio = g_screen_aspect_ratio;
	g_view_x_dots = 0;
	g_view_y_dots = 0;
	g_orbit_delay = 0;                     /* full speed orbits */
	g_orbit_interval = 1;                  /* plot all orbits */
	g_debug_mode = DEBUGMODE_NONE;				/* debugging flag(s) are off */
	g_timer_flag = false;                       /* timer flags are off       */
	g_formula_state.set_filename("fractint.frm"); /* default formula file      */
	g_formula_state.set_formula(NULL);
	strcpy(g_l_system_filename, "fractint.l");
	g_l_system_name[0] = 0;
	strcpy(g_command_file, "fractint.par");
	g_command_name[0] = 0;
	for (i = 0; i < 4; i++)
	{
		g_command_comment[i][0] = 0;
	}
	strcpy(g_ifs_filename, "fractint.ifs");
	g_ifs_name[0] = 0;
	reset_ifs_definition();
	g_use_fixed_random_seed = false;                           /* not a fixed srand() seed */
	g_random_seed = s_init_random_seed;
	strcpy(g_read_name, DOTSLASH);           /* initially current directory */
	g_show_file = SHOWFILE_DONE;
	/* next should perhaps be fractal re-init, not just <ins> ? */
	g_initial_cycle_limit = 55;						/* spin-DAC default speed limit */
	g_map_set = false;								/* no map= name active */
	if (g_map_dac_box)
	{
		free(g_map_dac_box);
		g_map_dac_box = NULL;
	}

	g_major_method = MAJORMETHOD_BREADTH_FIRST;		/* default inverse julia methods */
	g_minor_method = MINORMETHOD_LEFT_FIRST;		/* default inverse julia methods */
	g_true_color = false;							/* truecolor output flag */
	g_true_mode_iterates = false;					/* set to default color scheme */
}

static void initialize_variables_fractal()          /* init vars affecting calculation */
{
	int i;
	g_escape_exit_flag = false;                     /* don't disable the "are you sure?" screen */
	g_user_periodicity_check = 1;            /* turn on periodicity    */
	g_inside = 1;                          /* inside color = blue    */
	g_fill_color = -1;                      /* no special fill color */
	g_user_biomorph = -1;                   /* turn off g_biomorph flag */
	g_outside = COLORMODE_ITERATION;				/* outside color = -1 (not used) */
	g_max_iteration = 150;                         /* initial maxiter        */
	g_user_standard_calculation_mode = 'g';               /* initial solid-guessing */
	g_stop_pass = 0;                        /* initial guessing g_stop_pass */
	g_quick_calculate = false;
	g_proximity = 0.01;
	g_is_mand = true;                          /* default formula mand/jul toggle */
	g_user_float_flag = false;
	g_finite_attractor = FINITE_ATTRACTOR_NO;
	g_fractal_type = 0;                        /* initial type Set flag  */
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	s_initial_corners = false;
	s_initial_parameters = false;
	g_bail_out = 0;                         /* no user-entered bailout */
	g_no_bof = false;  /* use normal bof initialization to make bof images */
	g_use_initial_orbit_z = INITIALZ_NONE;
	for (i = 0; i < MAX_PARAMETERS; i++)
	{
		g_parameters[i] = 0.0;     /* initial parameter values */
	}
	for (i = 0; i < 3; i++)
	{
		g_potential_parameter[i]  = 0.0; /* initial potential values */
		g_inversion[i] = 0.0;  /* initial invert values */
	}
	g_initial_orbit_z.x = g_initial_orbit_z.y = 0.0;     /* initial orbit values */
	g_invert = 0;
	g_decomposition[0] = g_decomposition[1] = 0;
	g_user_distance_test = 0;
	g_pseudo_x = 0;
	g_pseudo_y = 0;
	g_distance_test_width = 71;
	g_force_symmetry = FORCESYMMETRY_NONE;                 /* symmetry not forced */
	g_escape_time_state.m_grid_fp.x_3rd() = -2.5;
	g_escape_time_state.m_grid_fp.x_min() = -2.5;
	g_escape_time_state.m_grid_fp.x_max() = 1.5;						/* initial corner values  */
	g_escape_time_state.m_grid_fp.y_3rd() = -1.5;
	g_escape_time_state.m_grid_fp.y_min() = -1.5;
	g_escape_time_state.m_grid_fp.y_max() = 1.5;						/* initial corner values  */
	g_bf_math = 0;
	g_potential_16bit = false;
	g_potential_flag = false;
	g_log_palette_mode = LOGPALETTE_NONE;                         /* no logarithmic palette */
	set_function_array(0, "sin");             /* trigfn defaults */
	set_function_array(1, "sqr");
	set_function_array(2, "sinh");
	set_function_array(3, "cosh");
	if (g_ranges_length)
	{
		free((char *)g_ranges);
		g_ranges_length = 0;
	}
	g_use_center_mag = true;                          /* use center-mag, not corners */

	g_color_state = COLORSTATE_DEFAULT;
	g_color_preloaded = false;
	g_rotate_lo = 1; g_rotate_hi = 255;      /* color cycling default range */
	g_orbit_delay = 0;                     /* full speed orbits */
	g_orbit_interval = 1;                  /* plot all orbits */
	g_keep_screen_coords = false;
	g_orbit_draw_mode = ORBITDRAW_RECTANGLE; /* passes=orbits draw mode */
	g_set_orbit_corners = false;
	g_orbit_x_min = g_current_fractal_specific->x_min;
	g_orbit_x_max = g_current_fractal_specific->x_max;
	g_orbit_x_3rd = g_current_fractal_specific->x_min;
	g_orbit_y_min = g_current_fractal_specific->y_min;
	g_orbit_y_max = g_current_fractal_specific->y_max;
	g_orbit_y_3rd = g_current_fractal_specific->y_min;

	g_math_tolerance[0] = 0.05;
	g_math_tolerance[1] = 0.05;

	g_display_3d = DISPLAY3D_NONE;
	g_overlay_3d = 0;                       /* 3D overlay is off        */

	g_old_demm_colors = false;
	g_bail_out_test    = BAILOUT_MODULUS;
	g_bail_out_fp  = bail_out_mod_fp;
	g_bail_out_l   = bail_out_mod_l;
	g_bail_out_bn = bail_out_mod_bn;
	g_bail_out_bf = bail_out_mod_bf;

	
	g_function_preloaded = false;			/* old bifurcation function support */
	g_m_x_min_fp = -.83;
	g_m_y_min_fp = -.25;
	g_m_x_max_fp = -.83;
	g_m_y_max_fp =  .25;
	g_origin_fp = 8;
	g_height_fp = 7;
	g_width_fp = 10;
	g_screen_distance_fp = 24.0f;
	g_eyes_fp = 2.5f;
	g_depth_fp = 8;
	g_new_orbit_type = FRACTYPE_JULIA;
	g_z_dots = 128;
	initialize_variables_3d();
	g_sound_state.initialize();
}

static void initialize_variables_3d()               /* init vars affecting 3d */
{
	g_3d_state.set_raytrace_output(RAYTRACE_NONE);
	g_3d_state.set_raytrace_brief(0);
	g_3d_state.set_sphere(false);
	g_3d_state.set_preview(false);
	g_3d_state.set_show_box(false);
	g_3d_state.set_x_adjust(0);
	g_3d_state.set_y_adjust(0);
	g_3d_state.set_eye_separation(0);
	g_3d_state.set_glasses_type(STEREO_NONE);
	g_3d_state.set_preview_factor(20);
	g_3d_state.set_red().set_crop_left(4);
	g_3d_state.set_red().set_crop_right(0);
	g_3d_state.set_blue().set_crop_left(0);
	g_3d_state.set_blue().set_crop_right(4);
	g_3d_state.set_red().set_bright(80);
	g_3d_state.set_blue().set_bright(100);
	g_3d_state.set_transparent0(0);
	g_3d_state.set_transparent1(0); /* no min/max transparency */
	g_3d_state.set_defaults();
}

static void reset_ifs_definition()
{
	if (g_ifs_definition)
	{
		free(g_ifs_definition);
		g_ifs_definition = NULL;
	}
}


static int command_file(FILE *handle, int mode)
	/* mode = 0 command line @filename         */
	/*        1 sstools.ini                    */
	/*        2 <@> command after startup      */
	/*        3 command line @filename/setname */
{
	/* note that command_file could be open as text OR as binary */
	/* binary is used in @ command processing for reasonable speed note/point */
	int i;
	int lineoffset = 0;
	int changeflag = 0; /* &1 fractal stuff chgd, &2 3d stuff chgd */
	char linebuf[513];
	char cmdbuf[10000] = { 0 };

	if (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME)
	{
		do
		{
			i = getc(handle);
		}
		while (i != '{' && i != EOF);
		for (i = 0; i < 4; i++)
		{
			g_command_comment[i][0] = 0;
		}
	}
	linebuf[0] = 0;
	while (next_command(cmdbuf, 10000, handle, linebuf, &lineoffset, mode) > 0)
	{
		if ((mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME) && strcmp(cmdbuf, "}") == 0)
		{
			break;
		}
		i = process_command(cmdbuf, mode);
		if (i < 0)
		{
			break;
		}
		changeflag |= i;
	}
	fclose(handle);
#ifdef XFRACT
	g_initial_adapter = 0;                /* Skip credits if @file is used. */
#endif
	if (changeflag & COMMANDRESULT_FRACTAL_PARAMETER)
	{
		backwards_v18();
		backwards_v19();
		backwards_v20();
	}
	return changeflag;
}

static int next_command(char *cmdbuf, int maxlen,
	FILE *handle, char *linebuf, int *lineoffset, int mode)
{
	int i;
	int cmdlen = 0;
	char *lineptr;
	lineptr = linebuf + *lineoffset;
	while (true)
	{
		while (*lineptr <= ' ' || *lineptr == ';')
		{
			if (cmdlen)  /* space or ; marks end of command */
			{
				cmdbuf[cmdlen] = 0;
				*lineoffset = (int) (lineptr - linebuf);
				return cmdlen;
			}
			while (*lineptr && *lineptr <= ' ')
			{
				++lineptr;                  /* skip spaces and tabs */
			}
			if (*lineptr == ';' || *lineptr == 0)
			{
				if (*lineptr == ';'
					&& (mode == CMDFILE_AT_AFTER_STARTUP || mode == CMDFILE_AT_CMDLINE_SETNAME)
					&& (g_command_comment[0][0] == 0 || g_command_comment[1][0] == 0 ||
						g_command_comment[2][0] == 0 || g_command_comment[3][0] == 0))
				{
					/* save comment */
					while (*(++lineptr)
						&& (*lineptr == ' ' || *lineptr == '\t'))
					{
					}
					if (*lineptr)
					{
						if ((int)strlen(lineptr) >= MAX_COMMENT)
						{
							*(lineptr + MAX_COMMENT-1) = 0;
						}
						for (i = 0; i < 4; i++)
						{
							if (g_command_comment[i][0] == 0)
							{
								strcpy(g_command_comment[i], lineptr);
								break;
							}
						}
					}
				}
				if (next_line(handle, linebuf, mode) != 0)
				{
					return -1; /* eof */
				}
				lineptr = linebuf; /* start new line */
			}
		}
		if (*lineptr == '\\'              /* continuation onto next line? */
			&& *(lineptr + 1) == 0)
		{
			if (next_line(handle, linebuf, mode) != 0)
			{
				arg_error(cmdbuf);           /* missing continuation */
				return -1;
			}
			lineptr = linebuf;
			while (*lineptr && *lineptr <= ' ')
			{
				++lineptr;                  /* skip white space @ start next line */
			}
			continue;                      /* loop to check end of line again */
		}
		cmdbuf[cmdlen] = *(lineptr++);    /* copy character to command buffer */
		if (++cmdlen >= maxlen)  /* command too long? */
		{
			arg_error(cmdbuf);
			return -1;
		}
	}
}

static int next_line(FILE *handle, char *linebuf, int mode)
{
	int toolssection;
	char tmpbuf[11];
	toolssection = 0;
	while (file_gets(linebuf, 512, handle) >= 0)
	{
		if (mode == CMDFILE_SSTOOLS_INI && linebuf[0] == '[')  /* check for [fractint] */
		{
			strncpy(tmpbuf, &linebuf[1], 9);
			tmpbuf[9] = 0;
			strlwr(tmpbuf);
			toolssection = strncmp(tmpbuf, "fractint]", 9);
			continue;                              /* skip tools section heading */
		}
		if (toolssection == 0)
		{
			return 0;
		}
	}
	return -1;
}

int bad_arg(const char *curarg)
{
	arg_error(curarg);
	return COMMANDRESULT_ERROR;
}

struct named_int
{
	const char *name;
	int value;
};

static bool named_value(const named_int *args, int num_args, const char *name, int *value)
{
	for (int i = 0; i < num_args; i++)
	{
		if (strcmp(name, args[i].name) == 0)
		{
			*value = args[i].value;
			return true;
		}
	}

	return false;
}

static int batch_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
#ifdef XFRACT
	g_initial_adapter = context.yesnoval[0] ? 0 : -1; /* skip credits for batch mode */
#endif
	g_initialize_batch = context.yesnoval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int max_history_arg(const cmd_context &context)
{
	if (context.numval == NON_NUMERIC)
	{
		return bad_arg(context.curarg);
	}
	else if (context.numval < 0 /* || context.numval > 1000 */)
	{
		return bad_arg(context.curarg);
	}
	else
	{
		g_max_history = context.numval;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int adapter_arg(const cmd_context &context)
{
	/* adapter parameter no longer used; check for bad argument anyway */
	named_int args[] =
	{
		{ "egamono", -1 },
		{ "hgc", -1 },
		{ "ega", -1 },
		{ "cga", -1 },
		{ "mcga", -1 },
		{ "vga", -1 }
	};
	int adapter = 0;
	if (named_value(args, NUM_OF(args), context.value, &adapter))
	{
		assert(adapter == -1);
		return bad_arg(context.curarg);
	}

	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int text_safe_arg(const cmd_context &context)
{
	/* textsafe no longer used, do validity checking, but gobble argument */
	if (g_command_initialize)
	{
		if (!((context.charval[0] == 'n')	/* no */
				|| (context.charval[0] == 'y')	/* yes */
				|| (context.charval[0] == 'b')	/* bios */
				|| (context.charval[0] == 's'))) /* save */
		{
			return bad_arg(context.curarg);
		}
	}
	return COMMANDRESULT_OK;
}

static int gobble_flag_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_OK;
}

static int exit_no_ask_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_escape_exit_flag,
		COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER).parse(context);
}

static int fpu_arg(const cmd_context &context)
{
	if (strcmp(context.value, "387") == 0)
	{
		return COMMANDRESULT_OK;
	}
	return bad_arg(context.curarg);
}

static int make_doc_arg(const cmd_context &context)
{
	print_document(context.value ? context.value : "fractint.doc", makedoc_msg_func, 0);
#ifndef WINFRACT
	goodbye();
#endif
	return 0;
}

static int make_par_arg(const cmd_context &context)
{
	char *slash;
	char *next = NULL;
	if (context.totparms < 1 || context.totparms > 2)
	{
		return bad_arg(context.curarg);
	}
	slash = strchr(context.value, '/');
	if (slash != NULL)
	{
		*slash = 0;
		next = slash + 1;
	}

	strcpy(g_command_file, context.value);
	if (strchr(g_command_file, '.') == NULL)
	{
		strcat(g_command_file, ".par");
	}
	if (strcmp(g_read_name, DOTSLASH) == 0)
	{
		*g_read_name = 0;
	}
	if (next == NULL)
	{
		if (*g_read_name != 0)
		{
			extract_filename(g_command_name, g_read_name);
		}
		else if (*g_map_name != 0)
		{
			extract_filename(g_command_name, g_map_name);
		}
		else
		{
			return bad_arg(context.curarg);
		}
	}
	else
	{
		strncpy(g_command_name, next, ITEMNAMELEN);
		g_command_name[ITEMNAMELEN] = 0;
	}
	*g_make_par = 0; /* used as a flag for makepar case */
	if (*g_read_name != 0)
	{
		if (read_overlay() != 0)
		{
			goodbye();
		}
	}
	else if (*g_map_name != 0)
	{
		g_make_par[1] = 0; /* second char is flag for map */
	}
	g_x_dots = g_file_x_dots;
	g_y_dots = g_file_y_dots;
	g_dx_size = g_x_dots - 1;
	g_dy_size = g_y_dots - 1;
	calculate_fractal_initialize();
	make_batch_file();
#ifndef WINFRACT
#if !defined(XFRACT)
#if defined(_WIN32)
	ABORT(0, "Don't call standard I/O without a console on Windows");
	_ASSERTE(0 && "Don't call standard I/O without a console on Windows");
#else
	if (*g_read_name != 0)
	{
		printf("copying fractal info in GIF %s to PAR %s/%s\n",
			g_read_name, g_command_file, g_command_name);
	}
	else if (*g_map_name != 0)
	{
		printf("copying color info in map %s to PAR %s/%s\n",
			g_map_name, g_command_file, g_command_name);
	}
#endif
#endif
	goodbye();
#endif
	return 0;
}

static int reset_arg(const cmd_context &context)
{
	initialize_variables_fractal();

	/* PAR release unknown unless specified */
	if (context.numval >= 0)
	{
		g_save_release = context.numval;
	}
	else
	{
		return bad_arg(context.curarg);
	}
	if (g_save_release == 0)
	{
		g_save_release = 1730; /* before start of lyapunov wierdness */
	}
	return COMMANDRESULT_RESET | COMMANDRESULT_FRACTAL_PARAMETER;
}

static int filename_arg(const cmd_context &context)
{
	int existdir;
	if (context.charval[0] == '.' && context.value[1] != SLASHC)
	{
		if (context.valuelen > 4)
		{
			return bad_arg(context.curarg);
		}
		g_gif_mask[0] = '*';
		g_gif_mask[1] = 0;
		strcat(g_gif_mask, context.value);
		return COMMANDRESULT_OK;
	}
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (context.mode == CMDFILE_AT_AFTER_STARTUP && g_display_3d == DISPLAY3D_NONE) /* can't do this in @ command */
	{
		return bad_arg(context.curarg);
	}

	existdir = merge_path_names(g_read_name, context.value, context.mode);
	if (existdir == 0)
	{
		g_show_file = SHOWFILE_PENDING;
	}
	else if (existdir < 0)
	{
		init_msg(context.variable, context.value, context.mode);
	}
	else
	{
		g_browse_state.extract_read_name();
	}
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int video_arg(const cmd_context &context)
{
	int k = check_vidmode_keyname(context.value);
	int i;

	if (k == 0)
	{
		return bad_arg(context.curarg);
	}
	g_initial_adapter = -1;
	for (i = 0; i < MAXVIDEOMODES; ++i)
	{
		if (g_video_table[i].keynum == k)
		{
			g_initial_adapter = i;
			break;
		}
	}
	if (g_initial_adapter == -1)
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int map_arg(const cmd_context &context)
{
	int existdir;
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	existdir = merge_path_names(g_map_name, context.value, context.mode);
	if (existdir > 0)
	{
		return COMMANDRESULT_OK;    /* got a directory */
	}
	else if (existdir < 0)
	{
		init_msg(context.variable, context.value, context.mode);
		return COMMANDRESULT_OK;
	}
	set_color_palette_name(g_map_name);
	return COMMANDRESULT_OK;
}

static int colors_arg(const cmd_context &context)
{
	if (parse_colors(context.value) < 0)
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_OK;
}

static int record_colors_arg(const cmd_context &context)
{
	if (*context.value != 'y' && *context.value != 'c' && *context.value != 'a')
	{
		return bad_arg(context.curarg);
	}
	g_record_colors = *context.value;
	return COMMANDRESULT_OK;
}

static int max_line_length_arg(const cmd_context &context)
{
	if (context.numval < MIN_MAX_LINE_LENGTH || context.numval > MAX_MAX_LINE_LENGTH)
	{
		return bad_arg(context.curarg);
	}
	g_max_line_length = context.numval;
	return COMMANDRESULT_OK;
}

static int parse_arg(const cmd_context &context)
{
	parse_comments(context.value);
	return COMMANDRESULT_OK;
}

/* maxcolorres no longer used, validate value and gobble argument */
static int max_color_res_arg(const cmd_context &context)
{
	if (context.numval == 1
		|| context.numval == 4
		|| context.numval == 8
		|| context.numval == 16
		|| context.numval == 24)
	{
		return COMMANDRESULT_OK;
	}
	return bad_arg(context.curarg);
}

/* pixelzoom no longer used, validate value and gobble argument */
static int pixel_zoom_arg(const cmd_context &context)
{
	if (context.numval >= 5)
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_OK;
}

/* keep this for backward compatibility */
static int warn_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_fractal_overwrite = (context.yesnoval[0] == 0);
	return COMMANDRESULT_OK;
}

static int overwrite_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_fractal_overwrite = (context.yesnoval[0] != 0);
	return COMMANDRESULT_OK;
}

static int save_time_arg(const cmd_context &context)
{
	g_save_time = context.numval;
	return COMMANDRESULT_OK;
}

static int auto_key_arg(const cmd_context &context)
{
	named_int args[] =
	{
		{ "record", SLIDES_RECORD },
		{ "play", SLIDES_PLAY }
	};
	return named_value(args, NUM_OF(args), context.value, &g_slides)
		? COMMANDRESULT_OK : bad_arg(context.curarg);
}

static int auto_key_name_arg(const cmd_context &context)
{
	if (merge_path_names(g_autokey_name, context.value, context.mode) < 0)
	{
		init_msg(context.variable, context.value, context.mode);
	}
	return COMMANDRESULT_OK;
}

static int type_arg(const cmd_context &context)
{
	char value[256];
	int valuelen = context.valuelen;
	int k;

	strcpy(value, context.value);
	if (value[valuelen-1] == '*')
	{
		value[--valuelen] = 0;
	}
	/* kludge because type ifs3d has an asterisk in front */
	if (strcmp(value, "ifs3d") == 0)
	{
		value[3] = 0;
	}
	for (k = 0; g_fractal_specific[k].name != NULL; k++)
	{
		if (strcmp(value, g_fractal_specific[k].name) == 0)
		{
			break;
		}
	}
	if (g_fractal_specific[k].name == NULL)
	{
		return bad_arg(context.curarg);
	}
	g_fractal_type = k;
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	if (!s_initial_corners)
	{
		g_escape_time_state.m_grid_fp.x_3rd() = g_current_fractal_specific->x_min;
		g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
		g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
		g_escape_time_state.m_grid_fp.y_3rd() = g_current_fractal_specific->y_min;
		g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
		g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
	}
	if (!s_initial_parameters)
	{
		load_parameters(g_fractal_type);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int inside_arg(const cmd_context &context)
{
	named_int args[] =
	{
		{ "zmag", COLORMODE_Z_MAGNITUDE },
		{ "bof60", COLORMODE_BEAUTY_OF_FRACTALS_60 },
		{ "bof61", COLORMODE_BEAUTY_OF_FRACTALS_61 },
		{ "epsiloncross", COLORMODE_EPSILON_CROSS },
		{ "startrail", COLORMODE_STAR_TRAIL },
		{ "period", COLORMODE_PERIOD },
		{ "fmod", COLORMODE_FLOAT_MODULUS_INTEGER },
		{ "atan", COLORMODE_INVERSE_TANGENT_INTEGER },
		{ "maxiter", COLORMODE_ITERATION }
	};
	if (named_value(args, NUM_OF(args), context.value, &g_inside))
	{
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}
	if (context.numval == NON_NUMERIC)
	{
		return bad_arg(context.curarg);
	}
	else
	{
		g_inside = context.numval;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int proximity_arg(const cmd_context &context)
{
	g_proximity = context.floatval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int fill_color_arg(const cmd_context &context)
{
	if (strcmp(context.value, "normal") == 0)
	{
		g_fill_color = -1;
	}
	else if (context.numval == NON_NUMERIC)
	{
		return bad_arg(context.curarg);
	}
	else
	{
		g_fill_color = context.numval;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int function_arg(const cmd_context &context)
{
	int k = 0;
	const char *value = context.value;

	while (*value && k < 4)
	{
		if (set_function_array(k++, value))
		{
			return bad_arg(context.curarg);
		}
		value = strchr(value, '/');
		if (value == NULL)
		{
			break;
		}
		++value;
	}
	g_function_preloaded = true;			/* old bifurcation function support */
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int outside_arg(const cmd_context &context)
{
	named_int args[] =
	{
		{ "iter", COLORMODE_ITERATION },
		{ "real", COLORMODE_REAL },
		{ "imag", COLORMODE_IMAGINARY },
		{ "mult", COLORMODE_MULTIPLY },
		{ "summ", COLORMODE_SUM },
		{ "atan", COLORMODE_INVERSE_TANGENT },
		{ "fmod", COLORMODE_FLOAT_MODULUS },
		{ "tdis", COLORMODE_TOTAL_DISTANCE }
	};
	if (named_value(args, NUM_OF(args), context.value, &g_outside))
	{
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}
	if ((context.numval == NON_NUMERIC) || (context.numval < COLORMODE_TOTAL_DISTANCE || context.numval > 255))
	{
		return bad_arg(context.curarg);
	}
	g_outside = context.numval;
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int bf_digits_arg(const cmd_context &context)
{
	if ((context.numval == NON_NUMERIC) || (context.numval < 0 || context.numval > 2000))
	{
		return bad_arg(context.curarg);
	}
	g_bf_digits = context.numval;
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int max_iter_arg(const cmd_context &context)
{
	if (context.floatval[0] < 2)
	{
		return bad_arg(context.curarg);
	}
	g_max_iteration = (long) context.floatval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int passes_arg(const cmd_context &context)
{
	if (context.charval[0] != '1' && context.charval[0] != '2' && context.charval[0] != '3'
		&& context.charval[0] != 'g' && context.charval[0] != 'b'
		&& context.charval[0] != 't' && context.charval[0] != 's'
		&& context.charval[0] != 'd' && context.charval[0] != 'o')
	{
		return bad_arg(context.curarg);
	}
	g_user_standard_calculation_mode = context.charval[0];
	if (context.charval[0] == 'g')
	{
		g_stop_pass = ((int)context.value[1] - (int)'0');
		if (g_stop_pass < 0 || g_stop_pass > 6)
		{
			g_stop_pass = 0;
		}
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int cycle_limit_arg(const cmd_context &context)
{
	if (context.numval <= 1 || context.numval > 256)
	{
		return bad_arg(context.curarg);
	}
	g_initial_cycle_limit = context.numval;
	return COMMANDRESULT_OK;
}

static int make_mig_arg(const cmd_context &context)
{
	int xmult;
	int ymult;
	if (context.totparms < 2)
	{
		return bad_arg(context.curarg);
	}
	xmult = context.intval[0];
	ymult = context.intval[1];
	make_mig(xmult, ymult);
#ifndef WINFRACT
	exit(0);
#endif
	return COMMANDRESULT_OK;
}

static int cycle_range_arg(const cmd_context &context)
{
	int lo;
	int hi;
	if (context.totparms < 2)
	{
		hi = 255;
	}
	else
	{
		hi = context.intval[1];
	}
	if (context.totparms < 1)
	{
		lo = 1;
	}
	else
	{
		lo = context.intval[0];
	}
	if (context.totparms != context.intparms
		|| lo < 0 || hi > 255 || lo > hi)
	{
		return bad_arg(context.curarg);
	}
	g_rotate_lo = lo;
	g_rotate_hi = hi;
	return COMMANDRESULT_OK;
}

static int ranges_arg(const cmd_context &context)
{
	int i;
	int j;
	int entries;
	int prev;
	int tmpranges[128];

	if (context.totparms != context.intparms)
	{
		return bad_arg(context.curarg);
	}
	entries = prev = i = 0;
	g_log_palette_mode = LOGPALETTE_NONE; /* ranges overrides logmap */
	while (i < context.totparms)
	{
		j = context.intval[i++];
		if (j < 0) /* striping */
		{
			j = -j;
			if (j < 1 || j >= 16384 || i >= context.totparms)
			{
				return bad_arg(context.curarg);
			}
			tmpranges[entries++] = -1; /* {-1,width,limit} for striping */
			tmpranges[entries++] = j;
			j = context.intval[i++];
		}
		if (j < prev)
		{
			return bad_arg(context.curarg);
		}
		tmpranges[entries++] = prev = j;
	}
	if (prev == 0)
	{
		return bad_arg(context.curarg);
	}
	g_ranges = (int *)malloc(sizeof(int)*entries);
	if (g_ranges == NULL)
	{
		stop_message(STOPMSG_NO_STACK, "Insufficient memory for ranges=");
		return -1;
	}
	g_ranges_length = entries;
	for (i = 0; i < g_ranges_length; ++i)
	{
		g_ranges[i] = tmpranges[i];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int save_name_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (g_command_initialize || context.mode == CMDFILE_AT_AFTER_STARTUP)
	{
		if (merge_path_names(g_save_name, context.value, context.mode) < 0)
		{
			init_msg(context.variable, context.value, context.mode);
		}
	}
	return COMMANDRESULT_OK;
}

static int min_stack_arg(const cmd_context &context)
{
	if (context.totparms != 1)
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_OK;
}

static int math_tolerance_arg(const cmd_context &context)
{
	if (context.charval[0] == '/')
	{
		; /* leave g_math_tolerance[0] at the default value */
	}
	else if (context.totparms >= 1)
	{
		g_math_tolerance[0] = context.floatval[0];
	}
	if (context.totparms >= 2)
	{
		g_math_tolerance[1] = context.floatval[1];
	}
	return COMMANDRESULT_OK;
}

static int temp_dir_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_DIR-1))
	{
		return bad_arg(context.curarg);
	}
	if (is_a_directory(context.value) == 0)
	{
		return bad_arg(context.curarg);
	}
	strcpy(g_temp_dir, context.value);
	ensure_slash_on_directory(g_temp_dir);
	return COMMANDRESULT_OK;
}

static int work_dir_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_DIR-1))
	{
		return bad_arg(context.curarg);
	}
	if (is_a_directory(context.value) == 0)
	{
		return bad_arg(context.curarg);
	}
	strcpy(g_work_dir, context.value);
	ensure_slash_on_directory(g_work_dir);
	return COMMANDRESULT_OK;
}

static int text_colors_arg(const cmd_context &context)
{
	parse_text_colors(context.value);
	return COMMANDRESULT_OK;
}

static int potential_arg(const cmd_context &context)
{
	int k = 0;
	char *value = context.value;
	while (k < 3 && *value)
	{
		g_potential_parameter[k] = (k == 1) ? atof(value) : atoi(value);
		k++;
		value = strchr(value, '/');
		if (value == NULL)
		{
			k = 99;
		}
		++value;
	}
	g_potential_16bit = false;
	if (k < 99)
	{
		if (strcmp(value, "16bit"))
		{
			return bad_arg(context.curarg);
		}
		g_potential_16bit = true;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int params_arg(const cmd_context &context)
{
	int k;

	if (context.totparms != context.floatparms || context.totparms > MAX_PARAMETERS)
	{
		return bad_arg(context.curarg);
	}
	s_initial_parameters = true;
	for (k = 0; k < MAX_PARAMETERS; ++k)
	{
		g_parameters[k] = (k < context.totparms) ? context.floatval[k] : 0.0;
	}
	if (g_bf_math)
	{
		for (k = 0; k < MAX_PARAMETERS; k++)
		{
			floattobf(bfparms[k], g_parameters[k]);
		}
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int miim_arg(const cmd_context &context)
{
	if (context.totparms > 6)
	{
		return bad_arg(context.curarg);
	}
	if (context.charval[0] == 'b')
	{
		g_major_method = MAJORMETHOD_BREADTH_FIRST;
	}
	else if (context.charval[0] == 'd')
	{
		g_major_method = MAJORMETHOD_DEPTH_FIRST;
	}
	else if (context.charval[0] == 'w')
	{
		g_major_method = MAJORMETHOD_RANDOM_WALK;
	}
#ifdef RANDOM_RUN
	else if (context.charval[0] == 'r')
	{
		g_major_method = MAJORMETHOD_RANDOM_RUN;
	}
#endif
	else
	{
		return bad_arg(context.curarg);
	}

	if (context.charval[1] == 'l')
	{
		g_minor_method = MINORMETHOD_LEFT_FIRST;
	}
	else if (context.charval[1] == 'r')
	{
		g_minor_method = MINORMETHOD_RIGHT_FIRST;
	}
	else
	{
		return bad_arg(context.curarg);
	}

	/* keep this next part in for backwards compatibility with old PARs ??? */

	if (context.totparms > 2)
	{
		int k;
		for (k = 2; k < 6; ++k)
		{
			g_parameters[k-2] = (k < context.totparms) ? context.floatval[k] : 0.0;
		}
	}

	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int init_orbit_arg(const cmd_context &context)
{
	if (strcmp(context.value, "pixel") == 0)
	{
		g_use_initial_orbit_z = INITIALZ_PIXEL;
	}
	else
	{
		if (context.totparms != 2 || context.floatparms != 2)
		{
			return bad_arg(context.curarg);
		}
		g_initial_orbit_z.x = context.floatval[0];
		g_initial_orbit_z.y = context.floatval[1];
		g_use_initial_orbit_z = INITIALZ_ORBIT;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int orbit_name_arg(const cmd_context &context)
{
	if (check_orbit_name(context.value))
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int threed_mode_arg(const cmd_context &context)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if (strcmp(context.value, g_juli_3d_options[i]) == 0)
		{
			g_juli_3d_mode = i;
			return COMMANDRESULT_FRACTAL_PARAMETER;
		}
	}
	return bad_arg(context.curarg);
}

static int julibrot_3d_arg(const cmd_context &context)
{
	if (context.floatparms != context.totparms)
	{
		return bad_arg(context.curarg);
	}
	if (context.totparms > 0)
	{
		g_z_dots = (int)context.floatval[0];
	}
	if (context.totparms > 1)
	{
		g_origin_fp = (float)context.floatval[1];
	}
	if (context.totparms > 2)
	{
		g_depth_fp = (float)context.floatval[2];
	}
	if (context.totparms > 3)
	{
		g_height_fp = (float)context.floatval[3];
	}
	if (context.totparms > 4)
	{
		g_width_fp = (float)context.floatval[4];
	}
	if (context.totparms > 5)
	{
		g_screen_distance_fp = (float)context.floatval[5];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int julibrot_eyes_arg(const cmd_context &context)
{
	if (context.floatparms != context.totparms || context.totparms != 1)
	{
		return bad_arg(context.curarg);
	}
	g_eyes_fp =  (float)context.floatval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int julibrot_from_to_arg(const cmd_context &context)
{
	if (context.floatparms != context.totparms || context.totparms != 4)
	{
		return bad_arg(context.curarg);
	}
	g_m_x_max_fp = context.floatval[0];
	g_m_x_min_fp = context.floatval[1];
	g_m_y_max_fp = context.floatval[2];
	g_m_y_min_fp = context.floatval[3];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int corners_arg(const cmd_context &context)
{
	int dec;
	if (g_fractal_type == FRACTYPE_CELLULAR)
	{
		return COMMANDRESULT_FRACTAL_PARAMETER; /* skip setting the corners */
	}
#if 0
	/* use a debugger and OutputDebugString instead of standard I/O on Windows */
	printf("totparms %d floatparms %d\n", totparms, context.floatparms);
	getch();
#endif
	if (context.floatparms != context.totparms
		|| (context.totparms != 0 && context.totparms != 4 && context.totparms != 6))
	{
		return bad_arg(context.curarg);
	}
	g_use_center_mag = false;
	if (context.totparms == 0)
	{
		return COMMANDRESULT_OK; /* turns corners mode on */
	}
	s_initial_corners = true;
	/* good first approx, but dec could be too big */
	dec = get_max_curarg_len((char **) context.floatvalstr, context.totparms) + 1;
	if ((dec > DBL_DIG + 1 || DEBUGMODE_NO_BIG_TO_FLOAT == g_debug_mode) && g_debug_mode != DEBUGMODE_NO_INT_TO_FLOAT)
	{
		int old_bf_math;

		old_bf_math = g_bf_math;
		if (!g_bf_math || dec > g_decimals)
		{
			init_bf_dec(dec);
		}
		if (old_bf_math == 0)
		{
			int k;
			for (k = 0; k < MAX_PARAMETERS; k++)
			{
				floattobf(bfparms[k], g_parameters[k]);
			}
		}

		/* x3rd = xmin = floatval[0]; */
		get_bf(g_escape_time_state.m_grid_bf.x_min(), context.floatvalstr[0]);
		get_bf(g_escape_time_state.m_grid_bf.x_3rd(), context.floatvalstr[0]);

		/* xmax = floatval[1]; */
		get_bf(g_escape_time_state.m_grid_bf.x_max(), context.floatvalstr[1]);

		/* y3rd = ymin = floatval[2]; */
		get_bf(g_escape_time_state.m_grid_bf.y_min(), context.floatvalstr[2]);
		get_bf(g_escape_time_state.m_grid_bf.y_3rd(), context.floatvalstr[2]);

		/* ymax = floatval[3]; */
		get_bf(g_escape_time_state.m_grid_bf.y_max(), context.floatvalstr[3]);

		if (context.totparms == 6)
		{
			/* x3rd = floatval[4]; */
			get_bf(g_escape_time_state.m_grid_bf.x_3rd(), context.floatvalstr[4]);

			/* y3rd = floatval[5]; */
			get_bf(g_escape_time_state.m_grid_bf.y_3rd(), context.floatvalstr[5]);
		}

		/* now that all the corners have been read in, get a more */
		/* accurate value for dec and do it all again             */

		dec = get_precision_mag_bf();
		if (dec < 0)
		{
			return bad_arg(context.curarg);     /* ie: Magnification is +-1.#INF */
		}

		if (dec > g_decimals)  /* get corners again if need more precision */
		{
			int k;

			init_bf_dec(dec);

			/* now get parameters and corners all over again at new
			decimal setting */
			for (k = 0; k < MAX_PARAMETERS; k++)
			{
				floattobf(bfparms[k], g_parameters[k]);
			}

			/* x3rd = xmin = floatval[0]; */
			get_bf(g_escape_time_state.m_grid_bf.x_min(), context.floatvalstr[0]);
			get_bf(g_escape_time_state.m_grid_bf.x_3rd(), context.floatvalstr[0]);

			/* xmax = floatval[1]; */
			get_bf(g_escape_time_state.m_grid_bf.x_max(), context.floatvalstr[1]);

			/* y3rd = ymin = floatval[2]; */
			get_bf(g_escape_time_state.m_grid_bf.y_min(), context.floatvalstr[2]);
			get_bf(g_escape_time_state.m_grid_bf.y_3rd(), context.floatvalstr[2]);

			/* ymax = floatval[3]; */
			get_bf(g_escape_time_state.m_grid_bf.y_max(), context.floatvalstr[3]);

			if (context.totparms == 6)
			{
				/* x3rd = floatval[4]; */
				get_bf(g_escape_time_state.m_grid_bf.x_3rd(), context.floatvalstr[4]);

				/* y3rd = floatval[5]; */
				get_bf(g_escape_time_state.m_grid_bf.y_3rd(), context.floatvalstr[5]);
			}
		}
	}
	g_escape_time_state.m_grid_fp.x_3rd() = context.floatval[0];
	g_escape_time_state.m_grid_fp.x_min() = context.floatval[0];
	g_escape_time_state.m_grid_fp.x_max() = context.floatval[1];
	g_escape_time_state.m_grid_fp.y_3rd() = context.floatval[2];
	g_escape_time_state.m_grid_fp.y_min() = context.floatval[2];
	g_escape_time_state.m_grid_fp.y_max() = context.floatval[3];

	if (context.totparms == 6)
	{
		g_escape_time_state.m_grid_fp.x_3rd() = context.floatval[4];
		g_escape_time_state.m_grid_fp.y_3rd() = context.floatval[5];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int orbit_corners_arg(const cmd_context &context)
{
	g_set_orbit_corners = false;
	if (context.floatparms != context.totparms
		|| (context.totparms != 0 && context.totparms != 4 && context.totparms != 6))
	{
		return bad_arg(context.curarg);
	}
	g_orbit_x_3rd = g_orbit_x_min = context.floatval[0];
	g_orbit_x_max =         context.floatval[1];
	g_orbit_y_3rd = g_orbit_y_min = context.floatval[2];
	g_orbit_y_max =         context.floatval[3];

	if (context.totparms == 6)
	{
		g_orbit_x_3rd =      context.floatval[4];
		g_orbit_y_3rd =      context.floatval[5];
	}
	g_set_orbit_corners = true;
	g_keep_screen_coords = true;
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int orbit_draw_mode_arg(const cmd_context &context)
{
	switch (context.charval[0])
	{
	case 'l': g_orbit_draw_mode = ORBITDRAW_LINE;		break;
	case 'r': g_orbit_draw_mode = ORBITDRAW_RECTANGLE;	break;
	case 'f': g_orbit_draw_mode = ORBITDRAW_FUNCTION;	break;

	default:
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int view_windows_arg(const cmd_context &context)
{
	if (context.totparms > 5 || context.floatparms-context.intparms > 2 || context.intparms > 4)
	{
		return bad_arg(context.curarg);
	}
	g_view_window = true;
	g_view_reduction = 4.2f;  /* reset default values */
	g_final_aspect_ratio = g_screen_aspect_ratio;
	g_view_crop = true;
	g_view_x_dots = g_view_y_dots = 0;

	if ((context.totparms > 0) && (context.floatval[0] > 0.001))
	{
		g_view_reduction = (float)context.floatval[0];
	}
	if ((context.totparms > 1) && (context.floatval[1] > 0.001))
	{
		g_final_aspect_ratio = (float)context.floatval[1];
	}
	if ((context.totparms > 2) && (context.yesnoval[2] == 0))
	{
		g_view_crop = false;
	}
	if ((context.totparms > 3) && (context.intval[3] > 0))
	{
		g_view_x_dots = context.intval[3];
	}
	if ((context.totparms == 5) && (context.intval[4] > 0))
	{
		g_view_y_dots = context.intval[4];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int center_mag_arg(const cmd_context &context)
{
	int dec;
	double Xctr;
	double Yctr;
	double Xmagfactor;
	double Rotation;
	double Skew;
	LDBL Magnification;
	big_t bXctr;
	big_t bYctr;

	if ((context.totparms != context.floatparms)
		|| (context.totparms != 0 && context.totparms < 3)
		|| (context.totparms >= 3 && context.floatval[2] == 0.0))
	{
		return bad_arg(context.curarg);
	}
	if (g_fractal_type == FRACTYPE_CELLULAR)
	{
		return COMMANDRESULT_FRACTAL_PARAMETER; /* skip setting the corners */
	}
	g_use_center_mag = true;
	if (context.totparms == 0)
	{
		return COMMANDRESULT_OK; /* turns center-mag mode on */
	}
	s_initial_corners = true;
	/* dec = get_max_curarg_len(floatvalstr, context.totparms); */
#ifdef USE_LONG_DOUBLE
	sscanf(context.floatvalstr[2], "%Lf", &Magnification);
#else
	sscanf(context.floatvalstr[2], "%lf", &Magnification);
#endif

	/* I don't know if this is portable, but something needs to */
	/* be used in case compiler's LDBL_MAX is not big enough    */
	if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
	{
		return bad_arg(context.curarg);     /* ie: Magnification is +-1.#INF */
	}

	dec = get_power_10(Magnification) + 4; /* 4 digits of padding sounds good */

	if ((dec <= DBL_DIG + 1 && g_debug_mode != DEBUGMODE_NO_BIG_TO_FLOAT) || DEBUGMODE_NO_INT_TO_FLOAT == g_debug_mode)  /* rough estimate that double is OK */
	{
		Xctr = context.floatval[0];
		Yctr = context.floatval[1];
		/* Magnification = context.floatval[2]; */  /* already done above */
		Xmagfactor = 1;
		Rotation = 0;
		Skew = 0;
		if (context.floatparms > 3)
		{
			Xmagfactor = context.floatval[3];
		}
		if (Xmagfactor == 0)
		{
			Xmagfactor = 1;
		}
		if (context.floatparms > 4)
		{
			Rotation = context.floatval[4];
		}
		if (context.floatparms > 5)
		{
			Skew = context.floatval[5];
		}
		/* calculate bounds */
		convert_corners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}
	else  /* use arbitrary precision */
	{
		int old_bf_math;
		int saved;
		s_initial_corners = true;
		old_bf_math = g_bf_math;
		if (!g_bf_math || dec > g_decimals)
		{
			init_bf_dec(dec);
		}
		if (old_bf_math == 0)
		{
			int k;
			for (k = 0; k < MAX_PARAMETERS; k++)
			{
				floattobf(bfparms[k], g_parameters[k]);
			}
		}
		g_use_center_mag = true;
		saved = save_stack();
		bXctr            = alloc_stack(g_bf_length + 2);
		bYctr            = alloc_stack(g_bf_length + 2);
		/* Xctr = context.floatval[0]; */
		get_bf(bXctr, context.floatvalstr[0]);
		/* Yctr = context.floatval[1]; */
		get_bf(bYctr, context.floatvalstr[1]);
		/* Magnification = context.floatval[2]; */  /* already done above */
		Xmagfactor = 1;
		Rotation = 0;
		Skew = 0;
		if (context.floatparms > 3)
		{
			Xmagfactor = context.floatval[3];
		}
		if (Xmagfactor == 0)
		{
			Xmagfactor = 1;
		}
		if (context.floatparms > 4)
		{
			Rotation = context.floatval[4];
		}
		if (context.floatparms > 5)
		{
			Skew = context.floatval[5];
		}
		/* calculate bounds */
		convert_corners_bf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
		corners_bf_to_float();
		restore_stack(saved);
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}
}

static int aspect_drift_arg(const cmd_context &context)
{
	if (context.floatparms != 1 || context.floatval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_aspect_drift = (float)context.floatval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int invert_arg(const cmd_context &context)
{
	if (context.totparms != context.floatparms || (context.totparms != 1 && context.totparms != 3))
	{
		return bad_arg(context.curarg);
	}
	g_inversion[0] = context.floatval[0];
	g_invert = (g_inversion[0] != 0.0) ? context.totparms : 0;
	if (context.totparms == 3)
	{
		g_inversion[1] = context.floatval[1];
		g_inversion[2] = context.floatval[2];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int ignore_arg(const cmd_context &context)
{
	return COMMANDRESULT_OK; /* just ignore and return, for old time's sake */
}

static int float_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_user_float_flag = (context.yesnoval[0] != 0);
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int fast_restore_arg(const cmd_context &context)
{
	if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_fast_restore = (context.yesnoval[0] != 0);
	return COMMANDRESULT_OK;
}

static int organize_formula_dir_arg(const cmd_context &context)
{
	if ((context.valuelen > (FILE_MAX_DIR-1))
		|| (is_a_directory(context.value) == 0))
	{
		return bad_arg(context.curarg);
	}
	g_organize_formula_search = true;
	strcpy(g_organize_formula_dir, context.value);
	ensure_slash_on_directory(g_organize_formula_dir);
	return COMMANDRESULT_OK;
}

static int biomorph_arg(const cmd_context &context)
{
	g_user_biomorph = context.numval;
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int orbit_save_arg(const cmd_context &context)
{
	if (context.charval[0] == 's')
	{
		g_orbit_save |= ORBITSAVE_SOUND;
	}
	else if (context.yesnoval[0] < 0)
	{
		return bad_arg(context.curarg);
	}
	g_orbit_save |= context.yesnoval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int bail_out_arg(const cmd_context &context)
{
	if (context.floatval[0] < 1 || context.floatval[0] > 2100000000L)
	{
		return bad_arg(context.curarg);
	}
	g_bail_out = (long)context.floatval[0];
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int bail_out_test_arg(const cmd_context &context)
{
	named_int args[] =
	{
		{ "mod", BAILOUT_MODULUS },
		{ "real", BAILOUT_REAL },
		{ "imag", BAILOUT_IMAGINARY },
		{ "or", BAILOUT_OR },
		{ "and", BAILOUT_AND },
		{ "manh", BAILOUT_MANHATTAN },
		{ "manr", BAILOUT_MANHATTAN_R }
	};
	int value;
	if (named_value(args, NUM_OF(args), context.value, &value))
	{
		g_bail_out_test = (enum bailouts) value;
		set_bail_out_formula(g_bail_out_test);
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}

	return bad_arg(context.curarg);
}

static int symmetry_arg(const cmd_context &context)
{
	named_int args[] =
	{
		{ "xaxis", SYMMETRY_X_AXIS },
		{ "yaxis", SYMMETRY_Y_AXIS },
		{ "xyaxis", SYMMETRY_XY_AXIS },
		{ "origin", SYMMETRY_ORIGIN },
		{ "pi", SYMMETRY_PI },
		{ "none", SYMMETRY_NONE }
	};
	if (named_value(args, NUM_OF(args), context.value, &g_force_symmetry))
	{
		return COMMANDRESULT_FRACTAL_PARAMETER;
	}
	return bad_arg(context.curarg);
}

static int sphere_arg(const cmd_context &context)
{
	return g_3d_state.parse_sphere(context);
}

static int sound_arg(const cmd_context &context)
{
	return g_sound_state.parse_sound(context);
}

static int hertz_arg(const cmd_context &context)
{
	return g_sound_state.parse_hertz(context);
}

static int volume_arg(const cmd_context &context)
{
	return g_sound_state.parse_volume(context);
}

static int attenuate_arg(const cmd_context &context)
{
	return g_sound_state.parse_attenuation(context);
}

static int polyphony_arg(const cmd_context &context)
{
	return g_sound_state.parse_polyphony(context);
}

static int wave_type_arg(const cmd_context &context)
{
	return g_sound_state.parse_wave_type(context);
}

static int attack_arg(const cmd_context &context)
{
	return g_sound_state.parse_attack(context);
}

static int decay_arg(const cmd_context &context)
{
	return g_sound_state.parse_decay(context);
}

static int sustain_arg(const cmd_context &context)
{
	return g_sound_state.parse_sustain(context);
}

static int sustain_release_arg(const cmd_context &context)
{
	return g_sound_state.parse_release(context);
}

static int scale_map_arg(const cmd_context &context)
{
	return g_sound_state.parse_scale_map(context);
}

static int periodicity_arg(const cmd_context &context)
{
	g_user_periodicity_check = 1;
	if ((context.charval[0] == 'n') || (context.numval == 0))
	{
		g_user_periodicity_check = 0;
	}
	else if (context.charval[0] == 'y')
	{
		g_user_periodicity_check = 1;
	}
	else if (context.charval[0] == 's')   /* 's' for 'show' */
	{
		g_user_periodicity_check = -1;
	}
	else if (context.numval == NON_NUMERIC)
	{
		return bad_arg(context.curarg);
	}
	else if (context.numval != 0)
	{
		g_user_periodicity_check = context.numval;
	}
	if (g_user_periodicity_check > 255)
	{
		g_user_periodicity_check = 255;
	}
	if (g_user_periodicity_check < -255)
	{
		g_user_periodicity_check = -255;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int log_map_arg(const cmd_context &context)
{
	g_log_automatic_flag = false;   /* turn this off if loading a PAR */
	if (context.charval[0] == 'y')
	{
		g_log_palette_mode = LOGPALETTE_STANDARD;                           /* palette is logarithmic */
	}
	else if (context.charval[0] == 'n')
	{
		g_log_palette_mode = LOGPALETTE_NONE;
	}
	else if (context.charval[0] == 'o')
	{
		g_log_palette_mode = LOGPALETTE_OLD;                          /* old log palette */
	}
	else
	{
		g_log_palette_mode = (long)context.floatval[0];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int log_mode_arg(const cmd_context &context)
{
	g_log_dynamic_calculate = LOGDYNAMIC_NONE;                         /* turn off if error */
	g_log_automatic_flag = false;
	if (context.charval[0] == 'f')
	{
		g_log_dynamic_calculate = LOGDYNAMIC_DYNAMIC;                      /* calculate on the fly */
	}
	else if (context.charval[0] == 't')
	{
		g_log_dynamic_calculate = LOGDYNAMIC_TABLE;                      /* force use of g_log_table */
	}
	else if (context.charval[0] == 'a')
	{
		g_log_automatic_flag = true;                     /* force auto calc of logmap */
	}
	else
	{
		return bad_arg(context.curarg);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int debug_flag_arg(const cmd_context &context)
{
	g_debug_mode = context.numval;
	g_timer_flag = ((g_debug_mode & 1) != 0);                /* separate timer flag */
	g_debug_mode &= ~1;
	return COMMANDRESULT_OK;
}

static int random_seed_arg(const cmd_context &context)
{
	g_random_seed = context.numval;
	g_use_fixed_random_seed = true;
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int orbit_delay_arg(const cmd_context &context)
{
	g_orbit_delay = context.numval;
	return COMMANDRESULT_OK;
}

static int orbit_interval_arg(const cmd_context &context)
{
	g_orbit_interval = context.numval;
	if (g_orbit_interval < 1)
	{
		g_orbit_interval = 1;
	}
	if (g_orbit_interval > 255)
	{
		g_orbit_interval = 255;
	}
	return COMMANDRESULT_OK;
}

static int show_dot_arg(const cmd_context &context)
{
	g_show_dot = 15;
	if (context.totparms > 0)
	{
		g_auto_show_dot = 0;
		if (isalpha(context.charval[0]))
		{
			if (strchr("abdm", (int)context.charval[0]) != NULL)
			{
				g_auto_show_dot = context.charval[0];
			}
			else
			{
				return bad_arg(context.curarg);
			}
		}
		else
		{
			g_show_dot = context.numval;
			if (g_show_dot < 0)
			{
				g_show_dot = -1;
			}
		}
		if (context.totparms > 1 && context.intparms > 0)
		{
			g_size_dot = context.intval[1];
		}
		if (g_size_dot < 0)
		{
			g_size_dot = 0;
		}
	}
	return COMMANDRESULT_OK;
}

static int show_orbit_arg(const cmd_context &context)
{
	g_start_show_orbit = (context.yesnoval[0] != 0);
	return COMMANDRESULT_OK;
}

static int decomposition_arg(const cmd_context &context)
{
	if (context.totparms != context.intparms || context.totparms < 1)
	{
		return bad_arg(context.curarg);
	}
	g_decomposition[0] = context.intval[0];
	g_decomposition[1] = 0;
	if (context.totparms > 1) /* backward compatibility */
	{
		g_bail_out = g_decomposition[1] = context.intval[1];
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int distance_test_arg(const cmd_context &context)
{
	if (context.totparms != context.intparms || context.totparms < 1)
	{
		return bad_arg(context.curarg);
	}
	g_user_distance_test = (long) context.floatval[0];
	g_distance_test_width = 71;
	if (context.totparms > 1)
	{
		g_distance_test_width = context.intval[1];
	}
	if (context.totparms > 3 && context.intval[2] > 0 && context.intval[3] > 0)
	{
		g_pseudo_x = context.intval[2];
		g_pseudo_y = context.intval[3];
	}
	else
	{
		g_pseudo_x = g_pseudo_y = 0;
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int formula_file_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (g_formula_state.merge_formula_filename(context.value, context.mode))
	{
		init_msg(context.variable, context.value, context.mode);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int formula_name_arg(const cmd_context &context)
{
	if (context.valuelen > ITEMNAMELEN)
	{
		return bad_arg(context.curarg);
	}
	g_formula_state.set_formula(context.value);
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int l_file_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (merge_path_names(g_l_system_filename, context.value, context.mode) < 0)
	{
		init_msg(context.variable, context.value, context.mode);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int l_name_arg(const cmd_context &context)
{
	if (context.valuelen > ITEMNAMELEN)
	{
		return bad_arg(context.curarg);
	}
	strcpy(g_l_system_name, context.value);
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int ifs_file_arg(const cmd_context &context)
{
	int existdir;
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	existdir = merge_path_names(g_ifs_filename, context.value, context.mode);
	if (existdir == 0)
	{
		reset_ifs_definition();
	}
	else if (existdir < 0)
	{
		init_msg(context.variable, context.value, context.mode);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int ifs_arg(const cmd_context &context)
{
	if (context.valuelen > ITEMNAMELEN)
	{
		return bad_arg(context.curarg);
	}
	strcpy(g_ifs_name, context.value);
	reset_ifs_definition();
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int parm_file_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (merge_path_names(g_command_file, context.value, context.mode) < 0)
	{
		init_msg(context.variable, context.value, context.mode);
	}
	return COMMANDRESULT_FRACTAL_PARAMETER;
}

static int stereo_arg(const cmd_context &context)
{
	if ((context.numval < 0) || (context.numval > 4))
	{
		return bad_arg(context.curarg);
	}
	g_3d_state.set_glasses_type(GlassesType(context.numval));
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int rotation_arg(const cmd_context &context)
{
	return g_3d_state.parse_rotation(context);
}

static int perspective_arg(const cmd_context &context)
{
	return g_3d_state.parse_perspective(context);
}

static int xy_shift_arg(const cmd_context &context)
{
	return g_3d_state.parse_xy_shift(context);
}

static int inter_ocular_arg(const cmd_context &context)
{
	g_3d_state.set_eye_separation(context.numval);
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int converge_arg(const cmd_context &context)
{
	g_3d_state.set_x_adjust(context.numval);
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int crop_arg(const cmd_context &context)
{
	if (context.totparms != 4 || context.intparms != 4
		|| context.intval[0] < 0 || context.intval[0] > 100
		|| context.intval[1] < 0 || context.intval[1] > 100
		|| context.intval[2] < 0 || context.intval[2] > 100
		|| context.intval[3] < 0 || context.intval[3] > 100)
	{
		return bad_arg(context.curarg);
	}
	g_3d_state.set_red().set_crop_left(context.intval[0]);
	g_3d_state.set_red().set_crop_right(context.intval[1]);
	g_3d_state.set_blue().set_crop_left(context.intval[2]);
	g_3d_state.set_blue().set_crop_right(context.intval[3]);
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int bright_arg(const cmd_context &context)
{
	if (context.totparms != 2 || context.intparms != 2)
	{
		return bad_arg(context.curarg);
	}
	g_3d_state.set_red().set_bright(context.intval[0]);
	g_3d_state.set_blue().set_bright(context.intval[1]);
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int xy_adjust_arg(const cmd_context &context)
{
	return g_3d_state.parse_xy_translate(context);
}

static int threed_arg(const cmd_context &context)
{
	int yesno = context.yesnoval[0];
	if (strcmp(context.value, "overlay") == 0)
	{
		yesno = 1;
		if (g_calculation_status > CALCSTAT_NO_FRACTAL) /* if no image, treat same as 3D=yes */
		{
			g_overlay_3d = 1;
		}
	}
	else if (yesno < 0)
	{
		return bad_arg(context.curarg);
	}
	g_display_3d = yesno ? DISPLAY3D_YES : DISPLAY3D_NONE;
	initialize_variables_3d();
	return COMMANDRESULT_3D_PARAMETER | (g_display_3d ? COMMANDRESULT_3D_YES : 0);
}

static int scale_xyz_arg(const cmd_context &context)
{
	return g_3d_state.parse_xyz_scale(context);
}

static int roughness_arg(const cmd_context &context)
{
	return g_3d_state.parse_roughness(context);
}

static int water_line_arg(const cmd_context &context)
{
	return g_3d_state.parse_water_line(context);
}

static int fill_type_arg(const cmd_context &context)
{
	return g_3d_state.parse_fill_type(context);
}

static int light_source_arg(const cmd_context &context)
{
	return g_3d_state.parse_light_source(context);
}

static int smoothing_arg(const cmd_context &context)
{
	return g_3d_state.parse_smoothing(context);
}

static int lattitude_arg(const cmd_context &context)
{
	return g_3d_state.parse_lattitude(context);
}

static int longitude_arg(const cmd_context &context)
{
	return g_3d_state.parse_longitude(context);
}

static int radius_arg(const cmd_context &context)
{
	return g_3d_state.parse_radius(context);
}

static int transparent_arg(const cmd_context &context)
{
	if (context.totparms != context.intparms || context.totparms < 1)
	{
		return bad_arg(context.curarg);
	}
	g_3d_state.set_transparent0(context.intval[0]);
	g_3d_state.set_transparent1((context.totparms > 1) ? context.intval[1] : context.intval[0]);
	return COMMANDRESULT_3D_PARAMETER;
}

static int coarse_arg(const cmd_context &context)
{
	if (context.numval < 3 || context.numval > 2000)
	{
		return bad_arg(context.curarg);
	}
	g_3d_state.set_preview_factor(context.numval);
	return COMMANDRESULT_3D_PARAMETER;
}

static int randomize_arg(const cmd_context &context)
{
	return g_3d_state.parse_randomize_colors(context);
}

static int ambient_arg(const cmd_context &context)
{
	return g_3d_state.parse_ambient(context);
}

static int haze_arg(const cmd_context &context)
{
	return g_3d_state.parse_haze(context);
}

static int true_mode_arg(const cmd_context &context)
{
	g_true_mode_iterates = false;				/* use default if error */
	if (context.charval[0] == 'd')
	{
		g_true_mode_iterates = false;			/* use default color output */
	}
	if (context.charval[0] == 'i' || context.intval[0] == 1)
	{
		g_true_mode_iterates = true;			/* use iterates output */
	}
	return COMMANDRESULT_FRACTAL_PARAMETER | COMMANDRESULT_3D_PARAMETER;
}

static int monitor_width_arg(const cmd_context &context)
{
	if (context.totparms != 1 || context.floatparms != 1)
	{
		return bad_arg(context.curarg);
	}
	g_auto_stereo_width  = context.floatval[0];
	return COMMANDRESULT_3D_PARAMETER;
}

static int background_arg(const cmd_context &context)
{
	return g_3d_state.parse_background_color(context);
}

static int light_name_arg(const cmd_context &context)
{
	if (context.valuelen > (FILE_MAX_PATH-1))
	{
		return bad_arg(context.curarg);
	}
	if (g_command_initialize || context.mode == CMDFILE_AT_AFTER_STARTUP)
	{
		strcpy(g_light_name, context.value);
	}
	return COMMANDRESULT_OK;
}

static int ray_arg(const cmd_context &context)
{
	return g_3d_state.parse_raytrace_output(context);
}

static int release_arg(const cmd_context &context)
{
	if (context.numval < 0)
	{
		return bad_arg(context.curarg);
	}

	g_save_release = context.numval;
	return COMMANDRESULT_3D_PARAMETER;
}

struct tag_command_processor
{
	const char *command;
	int (*processor)(const cmd_context &context);
};
typedef struct tag_command_processor command_processor;

static bool named_processor(const command_processor *processors,
						   int num_processors,
						   const cmd_context &context,
						   const char *command, int *result)
{
	int i;
	for (i = 0; i < num_processors; i++)
	{
		if (strcmp(processors[i].command, command) == 0)
		{
			*result = processors[i].processor(context);
			return true;
		}
	}
	return false;
}

static int gif87a_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_gif87a_flag, COMMANDRESULT_OK).parse(context);
}

static int dither_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_dither_flag, COMMANDRESULT_OK).parse(context);
}

static int finite_attractor_arg(const cmd_context &context)
{
	// TODO: this isn't quite right, should allow FINITE_ATTRACTOR_PHASE
	return FlagParser<int>(g_finite_attractor, COMMANDRESULT_FRACTAL_PARAMETER).parse(context);
}

static int no_bof_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_no_bof, COMMANDRESULT_FRACTAL_PARAMETER).parse(context);
}

static int is_mand_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_is_mand, COMMANDRESULT_FRACTAL_PARAMETER).parse(context);
}

static int preview_arg(const cmd_context &context)
{
	return g_3d_state.parse_preview(context);
}

static int showbox_arg(const cmd_context &context)
{
	return g_3d_state.parse_show_box(context);
}

static int fullcolor_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_targa_output, COMMANDRESULT_3D_PARAMETER).parse(context);
}

static int truecolor_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_true_color, COMMANDRESULT_3D_PARAMETER | COMMANDRESULT_FRACTAL_PARAMETER).parse(context);
}

static int use_grayscale_depth_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_grayscale_depth, COMMANDRESULT_3D_PARAMETER).parse(context);
}

static int targa_overlay_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_targa_overlay, COMMANDRESULT_3D_PARAMETER).parse(context);
}

static int brief_arg(const cmd_context &context)
{
	return g_3d_state.parse_raytrace_brief(context);
}

static int screencoords_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_keep_screen_coords, COMMANDRESULT_FRACTAL_PARAMETER).parse(context);
}

static int olddemmcolors_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_old_demm_colors, COMMANDRESULT_OK).parse(context);
}

static int ask_video_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_ui_state.ask_video, COMMANDRESULT_OK).parse(context);
}

static int cur_dir_arg(const cmd_context &context)
{
	return FlagParser<bool>(g_check_current_dir, COMMANDRESULT_OK).parse(context);
}

/*
	process_command(string, mode) processes a single command-line/command-file argument
	return:
		-1 error, >= 0 ok
		if ok, return value:
		| 1 means fractal parm has been set
		| 2 means 3d parm has been set
		| 4 means 3d=yes specified
		| 8 means reset specified
*/

int process_command(char *curarg, int mode) /* process a single argument */
{
	cmd_context context;
	char    variable[21];                /* variable name goes here   */
	double  ftemp;
	int i;
	int j;
	char *argptr;
	char *argptr2;
	char    tmpc;
	int     lastarg;

	argptr = curarg;
	context.curarg = curarg;
	while (*argptr)
	{                    /* convert to lower case */
		if (*argptr >= 'A' && *argptr <= 'Z')
		{
			*argptr += 'a' - 'A';
		}
		else if (*argptr == '=')
		{
			/* don't convert colors=value or comment=value */
			if ((strncmp(curarg, "colors=", 7) == 0) || (strncmp(curarg, "comment", 7) == 0))
			{
				break;
			}
		}
		++argptr;
	}

	context.value = strchr(&curarg[1], '=');
	if (context.value != NULL)
	{
		j = (int) ((context.value++) - curarg);
		if (j > 1 && curarg[j-1] == ':')
		{
			--j;                           /* treat := same as =     */
		}
	}
	else
	{
		j = (int) strlen(curarg);
		context.value = curarg + j;
	}
	if (j > 20)
	{
		return bad_arg(context.curarg);             /* keyword too long */
	}
	strncpy(variable, curarg, j);          /* get the variable name  */
	variable[j] = 0;                     /* truncate variable name */
	context.valuelen = (int) strlen(context.value);            /* note value's length    */
	context.charval[0] = context.value[0];               /* first letter of value  */
	context.yesnoval[0] = -1;                    /* note yes|no value      */
	if (context.charval[0] == 'n')
	{
		context.yesnoval[0] = 0;
	}
	if (context.charval[0] == 'y')
	{
		context.yesnoval[0] = 1;
	}

	argptr = context.value;
	context.numval = context.totparms = context.intparms = context.floatparms = 0;
	while (*argptr)                    /* count and pre-parse parms */
	{
		long ll;
		lastarg = 0;
		argptr2 = strchr(argptr, '/');
		if (argptr2 == NULL)     /* find next '/' */
		{
			argptr2 = argptr + strlen(argptr);
			*argptr2 = '/';
			lastarg = 1;
		}
		if (context.totparms == 0)
		{
			context.numval = NON_NUMERIC;
		}
		i = -1;
		if (context.totparms < 16)
		{
			context.charval[context.totparms] = *argptr;                      /* first letter of value  */
			if (context.charval[context.totparms] == 'n')
			{
				context.yesnoval[context.totparms] = 0;
			}
			if (context.charval[context.totparms] == 'y')
			{
				context.yesnoval[context.totparms] = 1;
			}
		}
		j = 0;
		if (sscanf(argptr, "%c%c", (char *) &j, &tmpc) > 0    /* NULL entry */
			&& ((char) j == '/' || (char) j == '=') && tmpc == '/')
		{
			j = 0;
			++context.floatparms;
			++context.intparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = j;
				context.floatvalstr[context.totparms] = "0";
			}
			if (context.totparms < 64)
			{
				context.intval[context.totparms] = j;
			}
			if (context.totparms == 0)
			{
				context.numval = j;
			}
		}
		else if (sscanf(argptr, "%ld%c", &ll, &tmpc) > 0       /* got an integer */
			&& tmpc == '/')        /* needs a long int, ll, here for lyapunov */
		{
			++context.floatparms;
			++context.intparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = ll;
				context.floatvalstr[context.totparms] = argptr;
			}
			if (context.totparms < 64)
			{
				context.intval[context.totparms] = (int) ll;
			}
			if (context.totparms == 0)
			{
				context.numval = (int) ll;
			}
		}
#ifndef XFRACT
		else if (sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0  /* got a float */
#else
		else if (sscanf(argptr, "%lf%c", &ftemp, &tmpc) > 0  /* got a float */
#endif
				&& tmpc == '/')
		{
			++context.floatparms;
			if (context.totparms < 16)
			{
				context.floatval[context.totparms] = ftemp;
				context.floatvalstr[context.totparms] = argptr;
			}
		}
		/* using arbitrary precision and above failed */
		else if (((int) strlen(argptr) > 513)  /* very long command */
					|| (context.totparms > 0 && context.floatval[context.totparms-1] == FLT_MAX
						&& context.totparms < 6)
					|| is_a_big_float(argptr))
		{
			++context.floatparms;
			context.floatval[context.totparms] = FLT_MAX;
			context.floatvalstr[context.totparms] = argptr;
		}
		++context.totparms;
		argptr = argptr2;                                 /* on to the next */
		if (lastarg)
		{
			*argptr = 0;
		}
		else
		{
			++argptr;
		}
	}

	context.mode = mode;
	context.variable = variable;
	/* these commands are allowed only at startup */
	if (mode != CMDFILE_AT_AFTER_STARTUP || DEBUGMODE_NO_FIRST_INIT == g_debug_mode)
	{
		command_processor processors[] =
		{
			{ "batch",		batch_arg },
			{ "maxhistory", max_history_arg },
			{ "adapter",	adapter_arg },
			{ "afi",		ignore_arg }, /* 8514 API no longer used; silently gobble any argument */
			{ "textsafe",	text_safe_arg },
			{ "vesadetect", gobble_flag_arg },
			{ "biospalette", gobble_flag_arg },
			{ "fpu",		fpu_arg },
			{ "exitnoask",	exit_no_ask_arg },
			{ "makedoc",	make_doc_arg },
			{ "makepar",	make_par_arg }
		};
		int result = COMMANDRESULT_OK;
		if (named_processor(processors, NUM_OF(processors), context, variable, &result))
		{
			return result;
		}
	}

	{
		command_processor processors[] =
		{
			{ "reset",			reset_arg },
			{ "filename",		filename_arg },			/* filename=?     */
			{ "video",			video_arg },			/* video=? */
 			{ "map",			map_arg },				/* map=, set default colors */
			{ "colors",			colors_arg },			/* colors=, set current colors */
			{ "recordcolors",	record_colors_arg },	/* recordcolors= */
			{ "maxlinelength",	max_line_length_arg },	/* maxlinelength= */
			{ "comment",		parse_arg },			/* comment= */
			{ "tplus",			gobble_flag_arg },		/* tplus no longer used */
			{ "noninterlaced",	gobble_flag_arg },		/* noninterlaced no longer used */
			{ "maxcolorres",	max_color_res_arg },	/* Change default color resolution */
			{ "pixelzoom",		pixel_zoom_arg },
			{ "warn",			warn_arg },				/* warn=? */
			{ "overwrite",		overwrite_arg },		/* overwrite=? */
			{ "gif87a",			gif87a_arg },
			{ "dither",			dither_arg },
			{ "savetime",		save_time_arg },		/* savetime=? */
			{ "autokey",		auto_key_arg },			/* autokey=? */
			{ "autokeyname",	auto_key_name_arg },	/* autokeyname=? */
			{ "type",			type_arg },				/* type=? */
			{ "inside",			inside_arg },			/* inside=? */
			{ "proximity",		proximity_arg },		/* proximity=? */
			{ "fillcolor",		fill_color_arg },		/* fillcolor */
			{ "finattract",		finite_attractor_arg },
			{ "nobof",			no_bof_arg },
			{ "function",		function_arg },			/* function=?,? */
			{ "outside",		outside_arg },			/* outside=? */
			{ "bfdigits",		bf_digits_arg },		/* bfdigits=? */
			{ "maxiter",		max_iter_arg },			/* maxiter=? */
			{ "iterincr",		ignore_arg },			/* iterincr=? */
			{ "passes",			passes_arg },			/* passes=? */
			{ "ismand",			is_mand_arg },
			{ "cyclelimit",		cycle_limit_arg },		/* cyclelimit=? */
			{ "makemig",		make_mig_arg },
			{ "cyclerange",		cycle_range_arg },
			{ "ranges",			ranges_arg },
			{ "savename", 		save_name_arg },		/* savename=? */
			{ "tweaklzw", 		ignore_arg },			/* tweaklzw=? */
			{ "minstack", 		min_stack_arg },		/* minstack=? */
			{ "mathtolerance", 	math_tolerance_arg },	/* mathtolerance=? */
			{ "tempdir", 		temp_dir_arg },			/* tempdir=? */
			{ "workdir", 		work_dir_arg },			/* workdir=? */
			{ "exitmode", 		ignore_arg },			/* exitmode=? */
			{ "textcolors", 	text_colors_arg },
			{ "potential", 		potential_arg },		/* potential=? */
			{ "params", 		params_arg },			/* params=?,? */
			{ "miim", 			miim_arg },				/* miim=?[/?[/?[/?]]] */
			{ "initorbit", 		init_orbit_arg },		/* initorbit=?,? */
			{ "orbitname", 		orbit_name_arg },		/* orbitname=? */
			{ "3dmode", 		threed_mode_arg },		/* orbitname=? */
			{ "julibrot3d", 	julibrot_3d_arg },		/* julibrot3d=?,?,?,? */
			{ "julibroteyes", 	julibrot_eyes_arg },	/* julibroteyes=?,?,?,? */
			{ "julibrotfromto", julibrot_from_to_arg },	/* julibrotfromto=?,?,?,? */
			{ "corners", 		corners_arg },			/* corners=?,?,?,? */
			{ "orbitcorners", 	orbit_corners_arg },	/* orbit corners=?,?,?,? */
			{ "screencoords", 	screencoords_arg },
			{ "orbitdrawmode", 	orbit_draw_mode_arg },	/* orbitdrawmode=? */
			{ "viewwindows", 	view_windows_arg },		/* viewwindows=?,?,?,?,? */
			{ "center-mag", 	center_mag_arg },		/* center-mag=?,?,?[,?,?,?] */
			{ "aspectdrift", 	aspect_drift_arg },		/* aspectdrift=? */
			{ "invert", 		invert_arg },			/* invert=?,?,? */
			{ "olddemmcolors", 	olddemmcolors_arg },
			{ "askvideo", 		ask_video_arg },
			{ "ramvideo", 		ignore_arg },			/* ramvideo=?   */
			{ "float", 			float_arg },			/* float=? */
			{ "fastrestore", 	fast_restore_arg },		/* fastrestore=? */
			{ "orgfrmdir", 		organize_formula_dir_arg },		/* orgfrmdir=? */
			{ "biomorph", 		biomorph_arg },			/* biomorph=? */
			{ "orbitsave", 		orbit_save_arg },		/* orbitsave=? */
			{ "bailout", 		bail_out_arg },			/* bailout=? */
			{ "bailoutest", 	bail_out_test_arg },	/* bailoutest=? */
			{ "symmetry", 		symmetry_arg },			/* symmetry=? */
			{ "printer", 		ignore_arg },			/* deprecated print parameters */
			{ "printfile", 		ignore_arg },
			{ "rleps", 			ignore_arg },
			{ "colorps", 		ignore_arg },
			{ "epsf", 			ignore_arg },
			{ "title", 			ignore_arg },
			{ "translate", 		ignore_arg },
			{ "plotstyle", 		ignore_arg },
			{ "halftone", 		ignore_arg },
			{ "linefeed", 		ignore_arg },
			{ "comport", 		ignore_arg },
			{ "sound", 			sound_arg },			/* sound=?,?,? */
			{ "hertz", 			hertz_arg },			/* Hertz=? */
			{ "volume", 		volume_arg },			/* Volume =? */
			{ "attenuate", 		attenuate_arg },
			{ "polyphony", 		polyphony_arg },
			{ "wavetype", 		wave_type_arg },		/* wavetype = ? */
			{ "attack", 		attack_arg },			/* attack = ? */
			{ "decay", 			decay_arg },			/* decay = ? */
			{ "sustain", 		sustain_arg },			/* sustain = ? */
			{ "srelease", 		sustain_release_arg },	/* srelease = ? */
			{ "scalemap", 		scale_map_arg },		/* Scalemap=?,?,?,?,?,?,?,?,?,?,? */
			{ "periodicity", 	periodicity_arg },		/* periodicity=? */
			{ "logmap", 		log_map_arg },			/* logmap=? */
			{ "logmode", 		log_mode_arg },			/* logmode=? */
			{ "debugflag", 		debug_flag_arg },
			{ "debug", 			debug_flag_arg },		/* internal use only */
			{ "rseed", 			random_seed_arg },
			{ "orbitdelay", 	orbit_delay_arg },
			{ "orbitinterval", 	orbit_interval_arg },
			{ "showdot", 		show_dot_arg },
			{ "showorbit", 		show_orbit_arg },		/* showorbit=yes|no */
			{ "decomp", 		decomposition_arg },
			{ "distest", 		distance_test_arg },
			{ "formulafile", 	formula_file_arg },		/* formulafile=? */
			{ "formulaname", 	formula_name_arg },		/* formulaname=? */
			{ "lfile", 			l_file_arg },			/* lfile=? */
			{ "lname", 			l_name_arg },
			{ "ifsfile", 		ifs_file_arg },			/* ifsfile=?? */
			{ "ifs", 			ifs_arg },
			{ "ifs3d", 			ifs_arg },				/* ifs3d for old time's sake */
			{ "parmfile", 		parm_file_arg },		/* parmfile=? */
			{ "stereo", 		stereo_arg },			/* stereo=? */
			{ "rotation", 		rotation_arg },			/* rotation=?/?/? */
			{ "perspective", 	perspective_arg },		/* perspective=? */
			{ "xyshift", 		xy_shift_arg },			/* xyshift=?/?  */
			{ "interocular", 	inter_ocular_arg },		/* interocular=? */
			{ "converge", 		converge_arg },			/* converg=? */
			{ "crop", 			crop_arg },				/* crop=? */
			{ "bright", 		bright_arg },			/* bright=? */
			{ "xyadjust", 		xy_adjust_arg },		/* xyadjust=?/? */
			{ "3d", 			threed_arg },			/* 3d=?/?/..    */
			{ "sphere", 		sphere_arg },
			{ "scalexyz", 		scale_xyz_arg },		/* scalexyz=?/?/? */
			{ "roughness", 		roughness_arg },		/* roughness=?  */
			{ "waterline", 		water_line_arg },		/* waterline=?  */
			{ "filltype", 		fill_type_arg },		/* filltype=?   */
			{ "lightsource", 	light_source_arg },		/* lightsource=?/?/? */
			{ "smoothing", 		smoothing_arg },		/* smoothing=?  */
			{ "latitude", 		lattitude_arg },			/* latitude=?/? */
			{ "longitude", 		longitude_arg },		/* longitude=?/? */
			{ "radius", 		radius_arg },			/* radius=? */
			{ "transparent", 	transparent_arg },		/* transparent? */
			{ "preview", 		preview_arg },
			{ "showbox", 		showbox_arg },
			{ "coarse", 		coarse_arg },			/* coarse=? */
			{ "randomize", 		randomize_arg },		/* randomize=? */
			{ "ambient", 		ambient_arg },			/* ambient=? */
			{ "haze", 			haze_arg },				/* haze=? */
			{ "fullcolor", 		fullcolor_arg },
			{ "truecolor", 		truecolor_arg },
			{ "truemode", 		true_mode_arg },		/* truemode=? */
			{ "usegrayscale", 	use_grayscale_depth_arg },
			{ "monitorwidth", 	monitor_width_arg },	/* monitorwidth=? */
			{ "targa_overlay", 	targa_overlay_arg },
			{ "background", 	background_arg },		/* background=?/? */
			{ "lightname", 		light_name_arg },		/* lightname=?   */
			{ "ray", 			ray_arg },				/* ray=? */
			{ "brief", 			brief_arg },
			{ "release", 		release_arg },			/* release */
			{ "curdir", 		cur_dir_arg },
			{ "virtual", 		gobble_flag_arg }
		};
		int result = COMMANDRESULT_OK;
		if (named_processor(processors, NUM_OF(processors), context, variable, &result))
		{
			return result;
		}
	}

	return bad_arg(context.curarg);
}

#ifdef _MSC_VER
#if (_MSC_VER >= 600)
#pragma optimize("el", on)
#endif
#endif

static void parse_text_colors(char *value)
{
	if (strcmp(value, "mono") == 0)
	{
		for (int k = 0; k < sizeof(g_text_colors); ++k)
		{
			g_text_colors[k] = BLACK*16 + WHITE;
		}
		/* C_HELP_CURLINK =
			C_PROMPT_INPUT =
			C_CHOICE_CURRENT =
			C_GENERAL_INPUT =
			C_AUTHDIV1 =
			C_AUTHDIV2 = WHITE*16 + BLACK; */
		g_text_colors[6] =
			g_text_colors[12] =
			g_text_colors[13] =
			g_text_colors[14] =
			g_text_colors[20] =
			g_text_colors[27] =
			g_text_colors[28] = WHITE*16 + BLACK;
		/* C_TITLE =
			C_HELP_HDG =
			C_HELP_LINK =
			C_PROMPT_HI =
			C_CHOICE_SP_KEYIN =
			C_GENERAL_HI =
			C_DVID_HI =
			C_STOP_ERR =
			C_STOP_INFO = BLACK*16 + L_WHITE; */
		g_text_colors[0] =
			g_text_colors[2] =
			g_text_colors[5] =
			g_text_colors[11] =
			g_text_colors[16] =
			g_text_colors[17] =
			g_text_colors[22] =
			g_text_colors[24] =
			g_text_colors[25] = BLACK*16 + L_WHITE;
	}
	else
	{
		int k = 0;
		while (k < sizeof(g_text_colors))
		{
			if (*value == 0)
			{
				break;
			}
			if (*value != '/')
			{
				int hexval;
				sscanf(value, "%x", &hexval);
				int i = (hexval / 16) & 7;
				int j = hexval & 15;
				if (i == j || (i == 0 && j == 8)) /* force contrast */
				{
					j = 15;
				}
				g_text_colors[k] = (BYTE) (i*16 + j);
				value = strchr(value, '/');
				if (value == NULL)
				{
					break;
				}
			}
			++value;
			++k;
		}
	}
}

static int parse_colors(char *value)
{
	int i;
	int j;
	int k;
	if (*value == '@')
	{
		if (merge_path_names(g_map_name, &value[1], false) < 0)
		{
			init_msg("", &value[1], 3);
		}
		if ((int)strlen(value) > FILE_MAX_PATH || validate_luts(g_map_name) != 0)
		{
			goto badcolor;
		}
		if (g_display_3d)
		{
			g_map_set = true;
		}
		else
		{
			if (merge_path_names(g_color_file, &value[1], false) < 0)
			{
				init_msg("", &value[1], 3);
			}
			g_color_state = COLORSTATE_MAP;
		}
	}
	else
	{
		i = 0;
		int smooth = 0;
		while (*value)
		{
			if (i >= 256)
			{
				goto badcolor;
			}
			if (*value == '<')
			{
				if (i == 0 || smooth
					|| (smooth = atoi(value + 1)) < 2
					|| (value = strchr(value, '>')) == NULL)
				{
					goto badcolor;
				}
				i += smooth;
				++value;
			}
			else
			{
				for (j = 0; j < 3; ++j)
				{
					k = *(value++);
					if (k < '0')
					{
						goto badcolor;
					}
					else if (k <= '9')
					{
						k -= '0';
					}
					else if (k < 'A')
					{
						goto badcolor;
					}
					else if (k <= 'Z')
					{
						k -= ('A'-10);
					}
					else if (k < '_' || k > 'z')
					{
						goto badcolor;
					}
					else
					{
						k -= ('_'-36);
					}
					g_dac_box[i][j] = (BYTE) k;
					if (smooth)
					{
						int spread = smooth + 1;
						int start = i - spread;
						int cnum = 0;
						if ((k - (int) g_dac_box[start][j]) == 0)
						{
							while (++cnum < spread)
							{
								g_dac_box[start + cnum][j] = (BYTE) k;
							}
						}
						else
						{
							while (++cnum < spread)
							{
								g_dac_box[start + cnum][j] =
									(BYTE) ((cnum*g_dac_box[i][j]
												+ (i - (start + cnum))*g_dac_box[start][j]
												+ spread/2)
											/ (BYTE) spread);
							}
						}
					}
				}
				smooth = 0;
				++i;
			}
		}
		if (smooth)
		{
			goto badcolor;
		}
		while (i < 256)   /* zap unset entries */
		{
			g_dac_box[i][0] = g_dac_box[i][1] = g_dac_box[i][2] = 63*COLOR_CHANNEL_MAX/100;
			++i;
		}
		g_color_state = COLORSTATE_UNKNOWN;
	}
	g_color_preloaded = true;
	memcpy(g_old_dac_box, g_dac_box, 256*3);
	return 0;

badcolor:
	return -1;
}

static void arg_error(const char *bad_arg)      /* oops. couldn't decode this */
{
	char msg[300];
	char spillover[71];
	if ((int) strlen(bad_arg) > 70)
	{
		strncpy(spillover, bad_arg, 70);
		spillover[70] = 0;
		bad_arg = spillover;
	}
	sprintf(msg, "Oops. I couldn't understand the argument:\n  %s", bad_arg);

	if (g_command_initialize)       /* this is 1st call to command_files */
	{
		strcat(msg, "\n"
			"\n"
			"(see the Startup Help screens or documentation for a complete\n"
			" argument list with descriptions)");
	}
	stop_message(0, msg);
	if (g_initialize_batch)
	{
		g_initialize_batch = INITBATCH_BAILOUT_INTERRUPTED;
		goodbye();
	}
}

/* copy a big number from a string, up to slash */
static int get_bf(bf_t bf, char *curarg)
{
	char *s = strchr(curarg, '/');
	if (s)
	{
		*s = 0;
	}
	strtobf(bf, curarg);
	if (s)
	{
		*s = '/';
	}
	return 0;
}

/* Get length of current args */
/* TODO: this shouldn't modify the curarg! */
int get_curarg_len(char *curarg)
{
	int len;
	char *s = strchr(curarg, '/');
	if (s)
	{
		*s = 0;
	}
	len = (int) strlen(curarg);
	if (s)
	{
		*s = '/';
	}
	return len;
}

/* Get max length of current args */
int get_max_curarg_len(char *floatvalstr[], int totparms)
{
	int i;
	int tmp;
	int max_str;
	max_str = 0;
	for (i = 0; i < totparms; i++)
	{
		tmp = get_curarg_len(floatvalstr[i]);
		if (tmp > max_str)
		{
			max_str = tmp;
		}
	}
	return max_str;
}

/* mode = 0 command line @filename         */
/*        1 sstools.ini                    */
/*        2 <@> command after startup      */
/*        3 command line @filename/setname */
/* this is like stop_message() but can be used in command_files()      */
/* call with NULL for badfilename to get pause for driver_get_key() */
int init_msg(const char *cmdstr, char *bad_filename, int mode)
{
	char *modestr[4] =
	{
		"command line", "sstools.ini", "PAR file", "PAR file"
	};
	char msg[256];
	char cmd[80];
	static int row = 1;

	if (g_initialize_batch == INITBATCH_NORMAL)  /* in batch mode */
	{
		if (bad_filename)
		{
			return -1;
		}
	}
	strncpy(cmd, cmdstr, 30);
	cmd[29] = 0;

	if (*cmd)
	{
		strcat(cmd, "=");
	}
	if (bad_filename)
	{
		sprintf(msg, "Can't find %s%s, please check %s", cmd, bad_filename, modestr[mode]);
	}
	if (g_command_initialize)  /* & command_files hasn't finished 1st try */
	{
		if (row == 1 && bad_filename)
		{
			driver_set_for_text();
			driver_put_string(0, 0, 15, "Fractint found the following problems when parsing commands: ");
		}
		if (bad_filename)
		{
			driver_put_string(row++, 0, 7, msg);
		}
		else if (row > 1)
		{
			driver_put_string(++row, 0, 15, "Press Escape to abort, any other key to continue");
			driver_move_cursor(row + 1, 0);
			pause_error(PAUSE_ERROR_GOODBYE);  /* defer getakeynohelp until after parsing */
		}
	}
	else if (bad_filename)
	{
		stop_message(0, msg);
	}
	return 0;
}

/* defer pause until after parsing so we know if in batch mode */
void pause_error(int action)
{
	static int needpause = PAUSE_ERROR_NO_BATCH;
	switch (action)
	{
	case PAUSE_ERROR_NO_BATCH:
		if (g_initialize_batch == INITBATCH_NONE)
		{
			if (needpause == PAUSE_ERROR_ANY)
			{
				driver_get_key();
			}
			else if (needpause == PAUSE_ERROR_GOODBYE)
			{
				if (getakeynohelp() == FIK_ESC)
				{
					goodbye();
				}
			}
		}
		needpause = PAUSE_ERROR_NO_BATCH;
		break;

	case PAUSE_ERROR_ANY:
	case PAUSE_ERROR_GOODBYE:
		needpause = action;
		break;
	default:
		break;
	}
}

/*
	Crude function to detect a floating point number. Intended for
	use with arbitrary precision.
*/
static int is_a_big_float(char *str)
{
	/* [+|-]numbers][.]numbers[+|-][e|g]numbers */
	int result = 1;
	char *s = str;
	int numdot = 0;
	int nume = 0;
	int numsign = 0;
	while (*s != 0 && *s != '/' && *s != ' ')
	{
		if (*s == '-' || *s == '+')
		{
			numsign++;
		}
		else if (*s == '.')
		{
			numdot++;
		}
		else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G')
		{
			nume++;
		}
		else if (!isdigit(*s))
		{
			result = 0;
			break;
		}
		s++;
	}
	if (numdot > 1 || numsign > 2 || nume > 1)
	{
		result = 0;
	}
	return result;
}
