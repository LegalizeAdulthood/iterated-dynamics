// Command-line / Command-File Parser Routines
//
#include "cmdfiles_test.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "calc_frac_init.h"
#include "check_orbit_name.h"
#include "comments.h"
#include "convert_corners.h"
#include "debug_flags.h"
#include "do_pause.h"
#include "drivers.h"
#include "engine_timer.h"
#include "extract_filename.h"
#include "file_gets.h"
#include "file_item.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "fracsuba.h"
#include "fractalb.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "framain2.h"
#include "get_fract_type.h"
#include "get_prec_big_float.h"
#include "goodbye.h"
#include "has_ext.h"
#include "helpcom.h"
#include "history.h"
#include "id.h"
#include "id_data.h"
#include "is_directory.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "loadmap.h"
#include "load_params.h"
#include "lorenz.h"
#include "lowerize_parameter.h"
#include "make_batch_file.h"
#include "make_mig.h"
#include "merge_path_names.h"
#include "parser.h"
#include "plot3d.h"
#include "rotate.h"
#include "slideshw.h"
#include "soi.h"
#include "sound.h"
#include "stereo.h"
#include "sticky_orbits.h"
#include "stop_msg.h"
#include "trig_fns.h"
#include "video_mode.h"

#include <array>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <numeric>
#include <string>
#include <system_error>
#include <vector>

#define DEFAULT_ASPECT_DRIFT 0.02F  // drift of < 2% is forced to 0%

static int get_max_curarg_len(char const *floatvalstr[], int totparm);
static cmdarg_flags cmdfile(std::FILE *handle, cmd_file mode);
static int  next_command(
    char *cmdbuf,
    int maxlen,
    std::FILE *handle,
    char *linebuf,
    int *lineoffset,
    cmd_file mode);
static bool next_line(std::FILE *handle, char *linebuf, cmd_file mode);
static void argerror(char const *);
static void initvars_run();
static void initvars_restart();
static void initvars_fractal();
static void initvars_3d();
static void reset_ifs_defn();
static void parse_textcolors(char const *value);
static cmdarg_flags parse_colors(char const *value);
static int  get_bf(bf_t bf, char const *curarg);
static bool isabigfloat(char const *str);

// variables defined by the command line/files processor
int     g_stop_pass{};           // stop at this guessing pass early
int     g_distance_estimator_x_dots{};            // xdots to use for video independence
int     g_distance_estimator_y_dots{};            // ydots to use for video independence
int     g_bf_digits{};           // digits to use (force) for bf_math
int     g_show_dot = -1;           // color to show crawling graphics cursor
int     g_size_dot{};            // size of dot crawling cursor
record_colors_mode g_record_colors = record_colors_mode::none;       // default PAR color-writing method
char    g_auto_show_dot{};        // dark, medium, bright
bool    g_start_show_orbit{};        // show orbits on at start of fractal
std::string g_read_filename;           // name of fractal input file
std::string g_temp_dir;            // name of temporary directory
std::string g_working_dir;            // name of directory for misc files
std::string g_organize_formulas_dir;          // name of directory for orgfrm files
std::string g_gif_filename_mask;
std::string g_save_filename{"fract001"}; // save files using this name
bool    g_potential_flag{};        // continuous potential enabled?
bool    g_potential_16bit{};               // store 16 bit continuous potential values
bool    g_dither_flag{};    // true if want to dither GIFs
bool    g_ask_video{};       // flag for video prompting
bool    g_float_flag{};
int     g_biomorph{};           // flag for biomorph
int     g_user_biomorph_value{};
int     g_show_file{};           // zero if file display pending
bool    g_random_seed_flag{};
int     g_random_seed{};              // Random number seeding flag and value
int     g_decomp[2] = { 0 };      // Decomposition coloring
long    g_distance_estimator{};
int     g_distance_estimator_width_factor{};
bool    g_overwrite_file{};// true if file overwrite allowed
int     g_sound_flag{};          // sound control bitfield... see sound.c for useage
int     g_base_hertz{};          // sound=x/y/x hertz value
int     g_cycle_limit{};         // color-rotator upper limit
int     g_fill_color{};          // fillcolor: -1=normal
bool g_finite_attractor{};        // finite attractor logic
display_3d_modes g_display_3d = display_3d_modes::NONE; // 3D display flag: 0 = OFF
bool    g_overlay_3d{};      // 3D overlay flag
bool    g_check_cur_dir{};    // flag to check current dir for files
batch_modes g_init_batch = batch_modes::NONE; // 1 if batch run (no kbd)
int     g_init_save_time{};       // autosave minutes
DComplex  g_init_orbit = { 0.0 };  // initial orbitvalue
init_orbit_mode g_use_init_orbit = init_orbit_mode::normal;       // flag for initorbit
int     g_init_mode{};        // initial video mode
int     g_init_cycle_limit{};     // initial cycle limit
bool    g_use_center_mag{};         // use center-mag corners
long    g_bail_out{};            // user input bailout value
double  g_inversion[3] = { 0.0 }; // radius, xcenter, ycenter
int     g_color_cycle_range_lo{};
int     g_color_cycle_range_hi{};          // cycling color range
std::vector<int> g_iteration_ranges;        // iter->color ranges mapping
int     g_iteration_ranges_len{};          // size of ranges array
BYTE g_map_clut[256][3];          // map= (default colors)
bool g_map_specified{};     // map= specified
BYTE *mapdacbox = nullptr;      // map= (default colors)
color_state g_color_state{color_state::DEFAULT}; // g_dac_box matches default (bios or map=)
bool    g_colors_preloaded{}; // if g_dac_box preloaded for next mode select
bool    g_read_color = true;  // flag for reading color from GIF
double  g_math_tol[2] = {.05, .05}; // For math transition
bool g_targa_out{};                 // 3D fullcolor flag
bool g_truecolor{};                 // escape time truecolor flag
true_color_mode g_true_mode = true_color_mode::default_color;               // truecolor coloring scheme
std::string g_color_file;          // from last <l> <s> or colors=@filename
bool g_new_bifurcation_functions_loaded{}; // if function loaded for new bifs
float   g_screen_aspect = DEFAULT_ASPECT;   // aspect ratio of the screen
float   g_aspect_drift = DEFAULT_ASPECT_DRIFT;  // how much drift is allowed and
                                // still forced to screenaspect

// true - reset viewwindows prior to a restore and
// do not display warnings when video mode changes during restore
bool g_fast_restore{};

// true: user has specified a directory for Orgform formula compilation files
bool g_organize_formulas_search{};

int     g_orbit_save_flags{};          // for IFS and LORENZ to output acrospin file
std::string g_orbit_save_name{"orbits.raw"};
int g_orbit_delay{};            // clock ticks delating orbit release
int g_transparent_color_3d[2] = { 0 }; // transparency min/max values
long    g_log_map_flag{};            // Logarithmic palette flag: 0 = no

int     g_log_map_fly_calculate{};       // calculate logmap on-the-fly
bool    g_log_map_auto_calculate{};          // auto calculate logmap
bool    g_bof_match_book_images = true;                  // Flag to make inside=bof options not duplicate bof images

bool    g_escape_exit{};    // set to true to avoid the "are you sure?" screen
bool g_first_init = true;                 // first time into cmdfiles?
static int init_rseed{};
static bool s_init_corners{};   // corners set via corners= or center-mag=?
static bool s_init_params{};    // params set via params=?
static bool s_init_functions{}; // trig functions set via function=?
fractalspecificstuff *g_cur_fractal_specific = nullptr;

std::string g_formula_filename;      // file to find (type=)formulas in
std::string g_formula_name;          // Name of the Formula (if not null)
std::string g_l_system_filename;     // file to find (type=)L-System's in
std::string g_l_system_name;         // Name of L-System
std::string g_command_file;          // file to find command sets in
std::string g_command_name;          // Name of Command set
std::string g_ifs_filename;          // file to find (type=)IFS in
std::string g_ifs_name;              // Name of the IFS def'n (if not null)
id::SearchPath g_search_for;         //
std::vector<float> g_ifs_definition; // ifs parameters
bool g_ifs_type{};             // false=2d, true=3d

BYTE g_text_color[] =
{
    BLUE*16+L_WHITE,    // C_TITLE           title background
    BLUE*16+L_GREEN,    // C_TITLE_DEV       development vsn foreground
    GREEN*16+YELLOW,    // C_HELP_HDG        help page title line
    WHITE*16+BLACK,     // C_HELP_BODY       help page body
    GREEN*16+GRAY,      // C_HELP_INSTR      help page instr at bottom
    WHITE*16+BLUE,      // C_HELP_LINK       help page links
    CYAN*16+BLUE,       // C_HELP_CURLINK    help page current link
    WHITE*16+GRAY,      // C_PROMPT_BKGRD    prompt/choice background
    WHITE*16+BLACK,     // C_PROMPT_TEXT     prompt/choice extra info
    BLUE*16+WHITE,      // C_PROMPT_LO       prompt/choice text
    BLUE*16+L_WHITE,    // C_PROMPT_MED      prompt/choice hdg2/...
    BLUE*16+YELLOW,     // C_PROMPT_HI       prompt/choice hdg/cur/...
    GREEN*16+L_WHITE,   // C_PROMPT_INPUT    fullscreen_prompt input
    CYAN*16+L_WHITE,    // C_PROMPT_CHOOSE   fullscreen_prompt choice
    MAGENTA*16+L_WHITE, // C_CHOICE_CURRENT  fullscreen_choice input
    BLACK*16+WHITE,     // C_CHOICE_SP_INSTR speed key bar & instr
    BLACK*16+L_MAGENTA, // C_CHOICE_SP_KEYIN speed key value
    WHITE*16+BLUE,      // C_GENERAL_HI      tab, thinking, IFS
    WHITE*16+BLACK,     // C_GENERAL_MED
    WHITE*16+GRAY,      // C_GENERAL_LO
    BLACK*16+L_WHITE,   // C_GENERAL_INPUT
    WHITE*16+BLACK,     // C_DVID_BKGRD      disk video
    BLACK*16+YELLOW,    // C_DVID_HI
    BLACK*16+L_WHITE,   // C_DVID_LO
    RED*16+L_WHITE,     // C_STOP_ERR        stop message, error
    GREEN*16+BLACK,     // C_STOP_INFO       stop message, info
    BLUE*16+WHITE,      // C_TITLE_LOW       bottom lines of title screen
    GREEN*16+BLACK,     // C_AUTHDIV1        title screen dividers
    GREEN*16+GRAY,      // C_AUTHDIV2        title screen dividers
    BLACK*16+L_WHITE,   // C_PRIMARY         primary authors
    BLACK*16+WHITE      // C_CONTRIB         contributing authors
};

// cmdfiles(argc,argv) process the command-line arguments
// it also processes the 'sstools.ini' file and any
// indirect files ('id @myfile')

// This probably ought to go somewhere else, but it's used here.
// getpower10(x) returns the magnitude of x.  This rounds
// a little so 9.95 rounds to 10, but we're using a binary base anyway,
// so there's nothing magic about changing to the next power of 10.
int getpower10(LDBL x)
{
    char string[11]; // space for "+x.xe-xxxx"
    int p;

    std::snprintf(string, std::size(string), "%+.1Le", x);
    p = std::atoi(string+5);
    return p;
}

static void process_sstools_ini()
{
    std::string const sstools_ini = find_path("sstools.ini"); // look for SSTOOLS.INI
    if (!sstools_ini.empty())              // found it!
    {
        std::FILE *initfile = std::fopen(sstools_ini.c_str(), "r");
        if (initfile != nullptr)
        {
            cmdfile(initfile, cmd_file::SSTOOLS_INI);           // process it
        }
    }
}

static void process_simple_command(char *curarg)
{
    bool processed{};
    if (std::strchr(curarg, '=') == nullptr)
    {
        // not xxx=yyy, so check for gif
        std::string filename = curarg;
        if (has_ext(curarg) == nullptr)
        {
            filename += ".gif";
        }
        if (std::FILE *initfile = std::fopen(filename.c_str(), "rb"))
        {
            char signature[6];
            if (std::fread(signature, 6, 1, initfile) != 6)
            {
                throw std::system_error(errno, std::system_category(), "process_simple_command failed fread");
            }
            if (signature[0] == 'G'
                && signature[1] == 'I'
                && signature[2] == 'F'
                && signature[3] >= '8' && signature[3] <= '9'
                && signature[4] >= '0' && signature[4] <= '9')
            {
                g_read_filename = curarg;
                g_browse_name = extract_filename(g_read_filename.c_str());
                g_show_file = 0;
                processed = true;
            }
            std::fclose(initfile);
        }
    }
    if (!processed)
    {
        cmdarg(curarg, cmd_file::AT_CMD_LINE);           // process simple command
    }
}

static void process_file_setname(const char *curarg, char *sptr)
{
    *sptr = 0;
    if (merge_pathnames(g_command_file, &curarg[1], cmd_file::AT_CMD_LINE) < 0)
    {
        init_msg("", g_command_file.c_str(), cmd_file::AT_CMD_LINE);
    }
    g_command_name = &sptr[1];
    std::FILE *initfile = nullptr;
    if (find_file_item(g_command_file, g_command_name.c_str(), &initfile, gfe_type::PARM) || initfile == nullptr)
    {
        argerror(curarg);
    }
    cmdfile(initfile, cmd_file::AT_CMD_LINE_SET_NAME);
}

static void process_file(char *curarg)
{
    std::FILE *initfile = std::fopen(&curarg[1], "r");
    if (initfile == nullptr)
    {
        argerror(curarg);
    }
    cmdfile(initfile, cmd_file::AT_CMD_LINE);
}

int cmdfiles(int argc, char const *const *argv)
{
    if (g_first_init)
    {
        initvars_run();                 // once per run initialization
    }
    initvars_restart();                  // <ins> key initialization
    initvars_fractal();                  // image initialization

    process_sstools_ini();

    // cycle through args
    for (int i = 1; i < argc; i++)
    {
        char curarg[141];
        std::strcpy(curarg, argv[i]);
        if (curarg[0] == ';')             // start of comments?
        {
            break;
        }
        if (curarg[0] != '@')
        {
            process_simple_command(curarg);
        }
        // @filename/setname?
        else if (char *sptr = std::strchr(curarg, '/'))
        {
            process_file_setname(curarg, sptr);
        }
        // @filename
        else
        {
            process_file(curarg);
        }
    }

    if (!g_first_init)
    {
        g_init_mode = -1; // don't set video when <ins> key used
        g_show_file = 1;  // nor startup image file
    }

    init_msg("", nullptr, cmd_file::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    if (g_debug_flag != debug_flags::allow_init_commands_anytime)
    {
        g_first_init = false;
    }

    // PAR reads a file and sets color, don't read colors from GIF
    g_read_color = !g_colors_preloaded || g_show_file != 0;

    //set structure of search directories
    g_search_for.par = g_command_file;
    g_search_for.frm = g_formula_filename;
    g_search_for.lsys = g_l_system_filename;
    g_search_for.ifs = g_ifs_filename;
    return 0;
}

static void init_param_flags()
{
    s_init_corners = false;
    s_init_params = false;
    s_init_functions = false;
}

// when called, file is open in binary mode, positioned at the
// '(' or '{' following the desired parameter set's name
cmdarg_flags load_commands(std::FILE *infile)
{
    init_param_flags(); // reset flags for type=
    const cmdarg_flags ret = cmdfile(infile, cmd_file::AT_AFTER_STARTUP);

    // PAR reads a file and sets color, don't read colors from GIF
    g_read_color = !(g_colors_preloaded && g_show_file == 0);

    return ret;
}


static void initvars_run()              // once per run init
{
    init_rseed = (int)std::time(nullptr);
    init_comments();
    char const *p = getenv("TMP");
    if (p == nullptr)
    {
        p = getenv("TEMP");
    }
    if (p != nullptr)
    {
        if (isadirectory(p))
        {
            g_temp_dir = p;
            fix_dirname(g_temp_dir);
        }
    }
    else
    {
        g_temp_dir.clear();
    }
}

static void initvars_restart() // <ins> key init
{
    g_record_colors = record_colors_mode::automatic;   // use mapfiles in PARs
    g_dither_flag = false;                             // no dithering
    g_ask_video = true;                                // turn on video-prompt flag
    g_overwrite_file = false;                          // don't overwrite
    g_sound_flag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; // sound is on to PC speaker
    g_init_batch = batch_modes::NONE;                  // not in batch mode
    g_check_cur_dir = false;                           // flag to check current dire for files
    g_init_save_time = 0;                              // no auto-save
    g_init_mode = -1;                                  // no initial video mode
    g_view_window = false;                             // no view window
    g_view_reduction = 4.2F;                           //
    g_view_crop = true;                                //
    g_virtual_screens = true;                          // virtual screen modes on
    g_final_aspect_ratio = g_screen_aspect;            //
    g_view_y_dots = 0;                                 //
    g_view_x_dots = 0;                                 //
    g_keep_aspect_ratio = true;                        // keep virtual aspect
    g_z_scroll = true;                                 // relaxed screen scrolling
    g_orbit_delay = 0;                                 // full speed orbits
    g_orbit_interval = 1;                              // plot all orbits
    g_debug_flag = debug_flags::none;                  // debugging flag(s) are off
    g_timer_flag = false;                              // timer flags are off
    g_formula_filename = "id.frm";                     // default formula file
    g_formula_name.clear();                            //
    g_l_system_filename = "id.l";                      //
    g_l_system_name.clear();                           //
    g_command_file = "id.par";                         //
    g_command_name.clear();                            //
    clear_command_comments();                          //
    g_ifs_filename = "id.ifs";                         //
    g_ifs_name.clear();                                //
    reset_ifs_defn();                                  //
    g_random_seed_flag = false;                        // not a fixed srand() seed
    g_random_seed = init_rseed;                        //
    g_read_filename = DOTSLASH;                        // initially current directory
    g_show_file = 1;                                   //
    // next should perhaps be fractal re-init, not just <ins> ?
    g_init_cycle_limit = 55;                          // spin-DAC default speed limit
    g_map_set = false;                                // no map= name active
    g_map_specified = false;                          //
    g_major_method = Major::breadth_first;            // default inverse julia methods
    g_inverse_julia_minor_method = Minor::left_first; // default inverse julia methods
    g_truecolor = false;                              // truecolor output flag
    g_true_mode = true_color_mode::default_color;     //
}

// init vars affecting calculation
static void initvars_fractal()
{
    g_escape_exit = false;                               // don't disable the "are you sure?" screen
    g_user_periodicity_value = 1;                        // turn on periodicity
    g_inside_color = 1;                                  // inside color = blue
    g_fill_color = -1;                                   // no special fill color
    g_user_biomorph_value = -1;                          // turn off biomorph flag
    g_outside_color = ITER;                              // outside color = -1 (not used)
    g_max_iterations = 150;                              // initial max iter
    g_user_std_calc_mode = 'g';                          // initial solid-guessing
    g_stop_pass = 0;                                     // initial guessing stop pass
    g_quick_calc = false;                                //
    g_close_proximity = 0.01;                            //
    g_is_mandelbrot = true;                              // default formula mand/jul toggle
    g_user_float_flag = true;                            // turn on the float flag
    g_finite_attractor = false;                          // disable finite attractor logic
    g_fractal_type = fractal_type::MANDEL;               // initial type Set flag
    g_cur_fractal_specific = &g_fractal_specific[0];     //
    init_param_flags();                                  //
    g_bail_out = 0;                                      // no user-entered bailout
    g_bof_match_book_images = true;                      // use normal bof initialization to make bof images
    g_use_init_orbit = init_orbit_mode::normal;          //
    std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0); // initial parameter values
    std::fill(&g_potential_params[0], &g_potential_params[3], 0.0); // initial potential values
    std::fill(std::begin(g_inversion), std::end(g_inversion), 0.0); // initial invert values
    g_init_orbit.y = 0.0;                                           //
    g_init_orbit.x = 0.0;                                           // initial orbit values
    g_invert = 0;                                                   //
    g_decomp[0] = 0;                                                //
    g_decomp[1] = 0;                                                //
    g_user_distance_estimator_value = 0;                            //
    g_distance_estimator_x_dots = 0;                                //
    g_distance_estimator_y_dots = 0;                                //
    g_distance_estimator_width_factor = 71;                         //
    g_force_symmetry = symmetry_type::NOT_FORCED;                   //
    g_x_min = -2.5;                                                 //
    g_x_3rd = g_x_min;                                              //
    g_x_max = 1.5;                                                  // initial corner values
    g_y_min = -1.5;                                                 //
    g_y_3rd = g_y_min;                                              //
    g_y_max = 1.5;                                                  // initial corner values
    bf_math = bf_math_type::NONE;                                   //
    g_potential_16bit = false;                                      //
    g_potential_flag = false;                                       //
    g_log_map_flag = 0;                                             // no logarithmic palette
    set_trig_array(0, "sin");                                       // trigfn defaults
    set_trig_array(1, "sqr");                                       //
    set_trig_array(2, "sinh");                                      //
    set_trig_array(3, "cosh");                                      //
    g_iteration_ranges.clear();                                     //
    g_iteration_ranges_len = 0;                                     //
    g_use_center_mag = true;                                        // use center-mag, not corners
    g_color_state = color_state::DEFAULT;                           //
    g_colors_preloaded = false;                                     //
    g_color_cycle_range_lo = 1;                                     //
    g_color_cycle_range_hi = 255;                                   // color cycling default range
    g_orbit_delay = 0;                                              // full speed orbits
    g_orbit_interval = 1;                                           // plot all orbits
    g_keep_screen_coords = false;                                   //
    g_draw_mode = 'r';                                              // passes=orbits draw mode
    g_set_orbit_corners = false;                                    //
    g_orbit_corner_min_x = g_cur_fractal_specific->xmin;            //
    g_orbit_corner_max_x = g_cur_fractal_specific->xmax;            //
    g_orbit_corner_3_x = g_cur_fractal_specific->xmin;              //
    g_orbit_corner_min_y = g_cur_fractal_specific->ymin;            //
    g_orbit_corner_max_y = g_cur_fractal_specific->ymax;            //
    g_orbit_corner_3_y = g_cur_fractal_specific->ymin;              //
    g_math_tol[0] = 0.05;                                           //
    g_math_tol[1] = 0.05;                                           //
    g_display_3d = display_3d_modes::NONE;                          // 3D display is off
    g_overlay_3d = false;                                           // 3D overlay is off
    g_old_demm_colors = false;                                      //
    g_bail_out_test = bailouts::Mod;                                //
    g_bailout_float = fpMODbailout;                                 //
    g_bailout_long = asmlMODbailout;                                //
    g_bailout_bignum = bnMODbailout;                                //
    g_bailout_bigfloat = bfMODbailout;                              //
    g_new_bifurcation_functions_loaded = false;                     // for old bifs
    g_julibrot_x_min = -.83;                                        //
    g_julibrot_y_min = -.25;                                        //
    g_julibrot_x_max = -.83;                                        //
    g_julibrot_y_max = .25;                                         //
    g_julibrot_origin_fp = 8;                                       //
    g_julibrot_height_fp = 7;                                       //
    g_julibrot_width_fp = 10;                                       //
    g_julibrot_dist_fp = 24;                                        //
    g_eyes_fp = 2.5F;                                               //
    g_julibrot_depth_fp = 8;                                        //
    g_new_orbit_type = fractal_type::JULIA;                         //
    g_julibrot_z_dots = 128;                                        //
    initvars_3d();                                                  //
    g_base_hertz = 440;                                             // basic hertz rate
    g_fm_volume = 63;                                               // full volume on soundcard o/p
    g_hi_attenuation = 0;                                           // no attenuation of hi notes
    g_fm_attack = 5;                                                // fast attack
    g_fm_decay = 10;                                                // long decay
    g_fm_sustain = 13;                                              // fairly high sustain level
    g_fm_release = 5;                                               // short release
    g_fm_wavetype = 0;                                              // sin wave
    g_polyphony = 0;                                                // no polyphony
    std::iota(&g_scale_map[0], &g_scale_map[11], 1);                // straight mapping of notes in octave
}

// init vars affecting 3d
static void initvars_3d()
{
    g_raytrace_format = raytrace_formats::none;
    g_brief   = false;
    SPHERE = FALSE;
    g_preview = false;
    g_show_box = false;
    g_converge_x_adjust = 0;
    g_converge_y_adjust = 0;
    g_eye_separation = 0;
    g_glasses_type = 0;
    g_preview_factor = 20;
    g_red_crop_left   = 4;
    g_red_crop_right  = 0;
    g_blue_crop_left  = 0;
    g_blue_crop_right = 4;
    g_red_bright     = 80;
    g_blue_bright   = 100;
    g_transparent_color_3d[0] = 0;
    g_transparent_color_3d[1] = 0; // no min/max transparency
    set_3d_defaults();
}

static void reset_ifs_defn()
{
    g_ifs_definition.clear();
}


// mode = AT_CMD_LINE           command line @filename
//        SSTOOLS_INI           sstools.ini
//        AT_AFTER_STARTUP      <@> command after startup
//        AT_CMD_LINE_SET_NAME  command line @filename/setname
// note that cmdfile could be open as text OR as binary
// binary is used in @ command processing for reasonable speed note/point
static cmdarg_flags cmdfile(std::FILE *handle, cmd_file mode)
{
    if (mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME)
    {
        int i;
        while ((i = getc(handle)) != '{' && i != EOF)
        {
        }
        clear_command_comments();
    }

    char cmdbuf[10000] = { 0 };
    char linebuf[513];
    linebuf[0] = 0;
    int lineoffset{};
    cmdarg_flags changeflag{}; // &1 fractal stuff chgd, &2 3d stuff chgd
    while (next_command(cmdbuf, std::size(cmdbuf), handle, linebuf, &lineoffset, mode) > 0)
    {
        if ((mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME) && std::strcmp(cmdbuf, "}") == 0)
        {
            break;
        }
        const cmdarg_flags i = cmdarg(cmdbuf, mode);
        if (i == cmdarg_flags::ERROR)
        {
            break;
        }
        changeflag |= i;
    }
    std::fclose(handle);

    if (bit_set(changeflag, cmdarg_flags::FRACTAL_PARAM))
    {
        backwards_v18();
        backwards_v19();
        backwards_v20();
    }
    return changeflag;
}

static int next_command(
    char *cmdbuf,
    int maxlen,
    std::FILE *handle,
    char *linebuf,
    int *lineoffset,
    cmd_file mode)
{
    int cmdlen{};
    char *lineptr = linebuf + *lineoffset;
    while (true)
    {
        while (*lineptr <= ' ' || *lineptr == ';')
        {
            if (cmdlen)                 // space or ; marks end of command
            {
                cmdbuf[cmdlen] = 0;
                *lineoffset = (int)(lineptr - linebuf);
                return cmdlen;
            }
            while (*lineptr && *lineptr <= ' ')
            {
                ++lineptr;                  // skip spaces and tabs
            }
            if (*lineptr == ';' || *lineptr == 0)
            {
                if (*lineptr == ';'
                    && (mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME)
                    && (g_command_comment[0].empty() || g_command_comment[1].empty()
                        || g_command_comment[2].empty() || g_command_comment[3].empty()))
                {
                    // save comment
                    while (*(++lineptr) && (*lineptr == ' ' || *lineptr == '\t'))
                    {
                    }
                    if (*lineptr)
                    {
                        if ((int)std::strlen(lineptr) >= MAX_COMMENT_LEN)
                        {
                            *(lineptr+MAX_COMMENT_LEN-1) = 0;
                        }
                        for (std::string &elem : g_command_comment)
                        {
                            if (elem.empty())
                            {
                                elem = lineptr;
                                break;
                            }
                        }
                    }
                }
                if (next_line(handle, linebuf, mode))
                {
                    return -1; // eof
                }
                lineptr = linebuf; // start new line
            }
        }
        if (*lineptr == '\\' && *(lineptr+1) == 0)              // continuation onto next line?
        {
            if (next_line(handle, linebuf, mode))
            {
                argerror(cmdbuf);           // missing continuation
                return -1;
            }
            lineptr = linebuf;
            while (*lineptr && *lineptr <= ' ')
            {
                ++lineptr;                  // skip white space @ start next line
            }
            continue;                      // loop to check end of line again
        }
        cmdbuf[cmdlen] = *(lineptr++);    // copy character to command buffer
        if (++cmdlen >= maxlen)         // command too long?
        {
            argerror(cmdbuf);
            return -1;
        }
    }
}

static bool next_line(std::FILE *handle, char *linebuf, cmd_file mode)
{
    bool tools_section = true;
    while (file_gets(linebuf, 512, handle) >= 0)
    {
        if (mode == cmd_file::SSTOOLS_INI && linebuf[0] == '[')   // check for [id]
        {
            char tmpbuf[11];
            std::strncpy(tmpbuf, &linebuf[1], 4);
            tmpbuf[4] = 0;
            strlwr(tmpbuf);
            tools_section = std::strncmp(tmpbuf, "id]", 3) == 0;
            continue;                              // skip tools section heading
        }
        if (tools_section)
        {
            return false;
        }
    }
    return true;
}

inline cmdarg_flags bad_arg(const char *curarg)
{
    argerror(curarg);
    return cmdarg_flags::ERROR;
}

enum
{
    NONNUMERIC = -32767
};

namespace cmd_arg
{

static StopMsgFn s_stop_msg{static_cast<StopMsg *>(stopmsg)};
static GoodbyeFn s_goodbye{goodbye};
static PrintDocFn s_print_document{print_document};

StopMsgFn get_stop_msg()
{
    return s_stop_msg;
}

void set_stop_msg(const StopMsgFn &fn)
{
    s_stop_msg = fn;
}

GoodbyeFn get_goodbye()
{
    return s_goodbye;
}

void set_goodbye(const GoodbyeFn &fn)
{
    s_goodbye = fn;
}

PrintDocFn get_print_document()
{
    return s_print_document;
}

void set_print_document(const PrintDocFn &fn)
{
    s_print_document = fn;
}

} // namespace cmd_arg

// cmdarg(string,mode) processes a single command-line/command-file argument
//  return:
//    -1 error, >= 0 ok
//    if ok, return value:
//      | 1 means fractal parm has been set
//      | 2 means 3d parm has been set
//      | 4 means 3d=yes specified
//      | 8 means reset specified
//
cmdarg_flags cmdarg(char *curarg, cmd_file mode) // process a single argument
{
    int     valuelen{};               // length of value
    int     numval{};                 // numeric value of arg
    char    charval[16] = { 0 };        // first character of arg
    int     yesnoval[16] = { 0 };       // 0 if 'n', 1 if 'y', -1 if not
    double  ftemp{};
    char    *argptr2 = nullptr;
    int     totparms{};               // # of / delimited parms
    int     intparms{};               // # of / delimited ints
    int     floatparms{};             // # of / delimited floats
    int     intval[64] = { 0 };         // pre-parsed integer parms
    double  floatval[16] = { 0.0 };     // pre-parsed floating parms
    char const *floatvalstr[16];        // pointers to float vals
    char    tmpc{};
    int     lastarg{};
    double Xctr{};
    double Yctr{};
    double Xmagfactor{};
    double Rotation{};
    double Skew{};
    LDBL Magnification{};
    bf_t bXctr;
    bf_t bYctr;

    lowerize_parameter(curarg);

    int j;
    char *value = std::strchr(&curarg[1], '=');
    if (value != nullptr)
    {
        j = (int)(value++ - curarg);
    }
    else
    {
        j = (int) std::strlen(curarg);
        value = curarg + j;
    }
    if (j > 20)
    {
        argerror(curarg);               // keyword too long
        return cmdarg_flags::ERROR;
    }
    std::string const variable(curarg, j);
    valuelen = (int) std::strlen(value);            // note value's length
    charval[0] = value[0];               // first letter of value
    yesnoval[0] = -1;                    // note yes|no value
    if (charval[0] == 'n')
    {
        yesnoval[0] = 0;
    }
    if (charval[0] == 'y')
    {
        yesnoval[0] = 1;
    }

    char *argptr = value;
    floatparms = 0;
    intparms = 0;
    totparms = 0;
    numval = 0;
    while (*argptr)                    // count and pre-parse parms
    {
        long ll;
        lastarg = 0;
        argptr2 = std::strchr(argptr, '/');
        if (argptr2 == nullptr)     // find next '/'
        {
            argptr2 = argptr + std::strlen(argptr);
            *argptr2 = '/';
            lastarg = 1;
        }
        if (totparms == 0)
        {
            numval = NONNUMERIC;
        }
        if (totparms < 16)
        {
            charval[totparms] = *argptr;                      // first letter of value
            if (charval[totparms] == 'n')
            {
                yesnoval[totparms] = 0;
            }
            if (charval[totparms] == 'y')
            {
                yesnoval[totparms] = 1;
            }
        }
        char next{};
        if (std::sscanf(argptr, "%c%c", &next, &tmpc) > 0    // NULL entry
            && (next == '/' || next == '=') && tmpc == '/')
        {
            j = 0;
            ++floatparms;
            ++intparms;
            if (totparms < 16)
            {
                floatval[totparms] = j;
                floatvalstr[totparms] = "0";
            }
            if (totparms < 64)
            {
                intval[totparms] = j;
            }
            if (totparms == 0)
            {
                numval = j;
            }
        }
        else if (std::sscanf(argptr, "%ld%c", &ll, &tmpc) > 0       // got an integer
            && tmpc == '/')        // needs a long int, ll, here for lyapunov
        {
            ++floatparms;
            ++intparms;
            if (totparms < 16)
            {
                floatval[totparms] = ll;
                floatvalstr[totparms] = argptr;
            }
            if (totparms < 64)
            {
                intval[totparms] = (int) ll;
            }
            if (totparms == 0)
            {
                numval = (int) ll;
            }
        }
        else if (std::sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0  // got a float
            && tmpc == '/')
        {
            ++floatparms;
            if (totparms < 16)
            {
                floatval[totparms] = ftemp;
                floatvalstr[totparms] = argptr;
            }
        }
        // using arbitrary precision and above failed
        else if (((int) std::strlen(argptr) > 513)  // very long command
            || (totparms > 0 && floatval[totparms-1] == FLT_MAX && totparms < 6)
            || isabigfloat(argptr))
        {
            ++floatparms;
            floatval[totparms] = FLT_MAX;
            floatvalstr[totparms] = argptr;
        }
        ++totparms;
        argptr = argptr2;                                 // on to the next
        if (lastarg)
        {
            *argptr = 0;
        }
        else
        {
            ++argptr;
        }
    }

    if (mode != cmd_file::AT_AFTER_STARTUP || g_debug_flag == debug_flags::allow_init_commands_anytime)
    {
        // these commands are allowed only at startup
        if (variable == "batch")     // batch=?
        {
            if (yesnoval[0] < 0)
            {
                return bad_arg(curarg);
            }
            g_init_batch = yesnoval[0] == 0 ? batch_modes::NONE : batch_modes::NORMAL;
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }
        if (variable == "maxhistory")       // maxhistory=?
        {
            if (numval == NONNUMERIC)
            {
                return bad_arg(curarg);
            }
            if (numval < 0)
            {
                return bad_arg(curarg);
            }
            g_max_image_history = numval;
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        // adapter= no longer used
        if (variable == "adapter")    // adapter==?
        {
            // adapter parameter no longer used; check for bad argument anyway
            if ((std::strcmp(value, "egamono") != 0) && (std::strcmp(value, "hgc") != 0)
                && (std::strcmp(value, "ega") != 0) && (std::strcmp(value, "cga") != 0)
                && (std::strcmp(value, "mcga") != 0) && (std::strcmp(value, "vga") != 0))
            {
                return bad_arg(curarg);
            }
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        // 8514 API no longer used; silently gobble any argument
        if (variable == "afi")
        {
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        if (variable == "textsafe")   // textsafe==?
        {
            // textsafe no longer used, do validity checking, but gobble argument
            if (g_first_init)
            {
                if (!((charval[0] == 'n')   // no
                    || (charval[0] == 'y')  // yes
                    || (charval[0] == 'b')  // bios
                    || (charval[0] == 's'))) // save
                {
                    return bad_arg(curarg);
                }
            }
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        if (variable == "vesadetect")
        {
            // vesadetect no longer used, do validity checks, but gobble argument
            if (yesnoval[0] < 0)
            {
                return bad_arg(curarg);
            }
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        // biospalette no longer used, do validity checks, but gobble argument
        if (variable == "biospalette")
        {
            if (yesnoval[0] < 0)
            {
                return bad_arg(curarg);
            }
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        // fpu deprecated; validate arg and gobble
        if (variable == "fpu")
        {
            if (std::strcmp(value, "387") == 0)
            {
                return cmdarg_flags::NONE;
            }
            return bad_arg(curarg);
        }

        // exitnoask deprecated; validate arg and gobble
        if (variable == "exitnoask")
        {
            if (yesnoval[0] < 0)
            {
                return bad_arg(curarg);
            }
            g_escape_exit = yesnoval[0] != 0;
            return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
        }

        if (variable == "makedoc")
        {
            cmd_arg::s_print_document(*value ? value : "id.txt", makedoc_msg_func);
            cmd_arg::s_goodbye();
            return cmdarg_flags::GOODBYE;
        }

        if (variable == "makepar")
        {
            if (totparms < 1 || totparms > 2)
            {
                return bad_arg(curarg);
            }
            char *slash = std::strchr(value, '/');
            char *next = nullptr;
            if (slash != nullptr)
            {
                *slash = 0;
                next = slash+1;
            }

            g_command_file = value;
            if (std::strchr(g_command_file.c_str(), '.') == nullptr)
            {
                g_command_file += ".par";
            }
            if (g_read_filename == DOTSLASH)
            {
                g_read_filename = "";
            }
            if (next == nullptr)
            {
                if (!g_read_filename.empty())
                {
                    g_command_name = extract_filename(g_read_filename.c_str());
                }
                else if (!g_map_name.empty())
                {
                    g_command_name = extract_filename(g_map_name.c_str());
                }
                else
                {
                    return bad_arg(curarg);
                }
            }
            else
            {
                g_command_name = next;
                assert(g_command_name.length() <= ITEM_NAME_LEN);
                if (g_command_name.length() > ITEM_NAME_LEN)
                {
                    g_command_name.resize(ITEM_NAME_LEN);
                }
            }
            g_make_parameter_file = true;
            if (!g_read_filename.empty())
            {
                if (read_overlay() != 0)
                {
                    cmd_arg::s_goodbye();
                    return cmdarg_flags::GOODBYE;
                }
            }
            else if (!g_map_name.empty())
            {
                g_make_parameter_file_map = true;
            }
            g_logical_screen_x_dots = g_file_x_dots;
            g_logical_screen_y_dots = g_file_y_dots;
            g_logical_screen_x_size_dots = g_logical_screen_x_dots - 1;
            g_logical_screen_y_size_dots = g_logical_screen_y_dots - 1;
            calcfracinit();
            make_batch_file();
            cmd_arg::s_goodbye();
            return cmdarg_flags::GOODBYE;
        }
    } // end of commands allowed only at startup

    if (variable == "reset")
    {
        // PAR release unknown unless specified
        if (numval < 0)
        {
            return bad_arg(curarg);
        }

        initvars_fractal();
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::RESET;
    }

    if (variable == "filename")      // filename=?
    {
        if (charval[0] == '.' && value[1] != SLASHC)
        {
            if (valuelen > 4)
            {
                return bad_arg(curarg);
            }
            g_gif_filename_mask = std::string{"*"} + value;
            return cmdarg_flags::NONE;
        }
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        if (mode == cmd_file::AT_AFTER_STARTUP && g_display_3d == display_3d_modes::NONE) // can't do this in @ command
        {
            return bad_arg(curarg);
        }

        const int exist_dir = merge_pathnames(g_read_filename, value, mode);
        if (exist_dir == 0)
        {
            g_show_file = 0;
        }
        else if (exist_dir < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        else
        {
            g_browse_name = extract_filename(g_read_filename.c_str());
        }
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "video")         // video=?
    {
        const int k = check_vidmode_keyname(value);
        if (k == 0)
        {
            return bad_arg(curarg);
        }
        g_init_mode = -1;
        for (int i = 0; i < MAX_VIDEO_MODES; ++i)
        {
            if (g_video_table[i].keynum == k)
            {
                g_init_mode = i;
                break;
            }
        }
        if (g_init_mode == -1)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "map")         // map=, set default colors
    {
        if (valuelen > FILE_MAX_PATH - 1)
        {
            return bad_arg(curarg);
        }

        const int exist_dir = merge_pathnames(g_map_name, value, mode);
        if (exist_dir > 0)
        {
            return cmdarg_flags::NONE;    // got a directory
        }
        if (exist_dir < 0) // error
        {
            init_msg(variable.c_str(), value, mode);
            return cmdarg_flags::NONE;
        }
        SetColorPaletteName(g_map_name.c_str());
        return cmdarg_flags::NONE;
    }

    if (variable == "colors")       // colors=, set current colors
    {
        if (parse_colors(value) == cmdarg_flags::ERROR)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "recordcolors")       // recordcolors=
    {
        if (*value != 'y' && *value != 'c' && *value != 'a')
        {
            return bad_arg(curarg);
        }
        g_record_colors = static_cast<record_colors_mode>(*value);
        return cmdarg_flags::NONE;
    }

    if (variable == "maxlinelength")  // maxlinelength=
    {
        if (numval < MIN_MAX_LINE_LENGTH || numval > MAX_MAX_LINE_LENGTH)
        {
            return bad_arg(curarg);
        }
        g_max_line_length = numval;
        return cmdarg_flags::NONE;
    }

    if (variable == "comment")       // comment=
    {
        parse_comments(value);
        return cmdarg_flags::NONE;
    }

    // tplus no longer used, validate value and gobble argument
    if (variable == "tplus")
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    // noninterlaced no longer used, validate value and gobble argument
    if (variable == "noninterlaced")
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    // maxcolorres no longer used, validate value and gobble argument
    if (variable == "maxcolorres") // Change default color resolution
    {
        if (numval == 1 || numval == 4 || numval == 8 || numval == 16 || numval == 24)
        {
            return cmdarg_flags::NONE;
        }
        return bad_arg(curarg);
    }

    // pixelzoom no longer used, validate value and gobble argument
    if (variable == "pixelzoom")
    {
        if (numval >= 5)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    // keep this for backward compatibility
    if (variable == "warn")         // warn=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_overwrite_file = (yesnoval[0] ^ 1) != 0;
        return cmdarg_flags::NONE;
    }
    if (variable == "overwrite")    // overwrite=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_overwrite_file = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "gif87a")       // gif87a=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "dither") // dither=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_dither_flag = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "savetime")      // savetime=?
    {
        g_init_save_time = numval;
        return cmdarg_flags::NONE;
    }

    if (variable == "autokey")       // autokey=?
    {
        if (std::strcmp(value, "record") == 0)
        {
            g_slides = slides_mode::RECORD;
        }
        else if (std::strcmp(value, "play") == 0)
        {
            g_slides = slides_mode::PLAY;
        }
        else
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "autokeyname")   // autokeyname=?
    {
        if (merge_pathnames(g_auto_name, value, mode) < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "type")         // type=?
    {
        if (value[valuelen-1] == '*')
        {
            value[--valuelen] = 0;
        }
        // kludge because type ifs3d has an asterisk in front
        if (std::strcmp(value, "ifs3d") == 0)
        {
            value[3] = 0;
        }
        int k;
        for (k = 0; g_fractal_specific[k].name != nullptr; k++)
        {
            if (std::strcmp(value, g_fractal_specific[k].name) == 0)
            {
                break;
            }
        }
        if (g_fractal_specific[k].name == nullptr)
        {
            return bad_arg(curarg);
        }
        const fractal_type old_fractal_type{g_fractal_type};
        g_fractal_type = static_cast<fractal_type>(k);
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        if (!s_init_corners)
        {
            g_x_min = g_cur_fractal_specific->xmin;
            g_x_3rd = g_x_min;
            g_x_max = g_cur_fractal_specific->xmax;
            g_y_min = g_cur_fractal_specific->ymin;
            g_y_3rd = g_y_min;
            g_y_max = g_cur_fractal_specific->ymax;
        }
        if (!s_init_functions)
        {
            set_fractal_default_functions(old_fractal_type);
        }
        if (!s_init_params)
        {
            load_params(g_fractal_type);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "inside")       // inside=?
    {
        struct
        {
            char const *arg;
            int inside;
        }
        const args[] =
        {
            { "zmag", ZMAG },
            { "bof60", BOF60 },
            { "bof61", BOF61 },
            { "epsiloncross", EPSCROSS },
            { "startrail", STARTRAIL },
            { "period", PERIOD },
            { "fmod", FMODI },
            { "atan", ATANI },
            { "maxiter", -1 }
        };
        for (auto &arg : args)
        {
            if (std::strcmp(value, arg.arg) == 0)
            {
                g_inside_color = arg.inside;
                return cmdarg_flags::FRACTAL_PARAM;
            }
        }
        if (numval == NONNUMERIC)
        {
            return bad_arg(curarg);
        }
        else
        {
            g_inside_color = numval;
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "proximity")       // proximity=?
    {
        g_close_proximity = floatval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "fillcolor")       // fillcolor
    {
        if (std::strcmp(value, "normal") == 0)
        {
            g_fill_color = -1;
        }
        else if (numval == NONNUMERIC)
        {
            return bad_arg(curarg);
        }
        else
        {
            g_fill_color = numval;
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "finattract")   // finattract=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_finite_attractor = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "nobof")   // nobof=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_bof_match_book_images = yesnoval[0] == 0;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "function")      // function=?,?
    {
        int k = 0;
        while (*value && k < 4)
        {
            if (set_trig_array(k++, value))
            {
                return bad_arg(curarg);
            }
            value = std::strchr(value, '/');
            if (value == nullptr)
            {
                break;
            }
            ++value;
        }
        g_new_bifurcation_functions_loaded = true; // for old bifs
        s_init_functions = true;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "outside")      // outside=?
    {
        struct
        {
            char const *arg;
            int outside;
        }
        args[] =
        {
            { "iter", ITER },
            { "real", REAL },
            { "imag", IMAG },
            { "mult", MULT },
            { "summ", SUM },
            { "atan", ATAN },
            { "fmod", FMOD },
            { "tdis", TDIS }
        };
        for (auto &arg : args)
        {
            if (std::strcmp(value, arg.arg) == 0)
            {
                g_outside_color = arg.outside;
                return cmdarg_flags::FRACTAL_PARAM;
            }
        }
        if ((numval == NONNUMERIC) || (numval < TDIS || numval > 255))
        {
            return bad_arg(curarg);
        }
        g_outside_color = numval;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "bfdigits")      // bfdigits=?
    {
        if ((numval == NONNUMERIC) || (numval < 0 || numval > 2000))
        {
            return bad_arg(curarg);
        }
        g_bf_digits = numval;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "maxiter")       // maxiter=?
    {
        if (floatval[0] < 2)
        {
            return bad_arg(curarg);
        }
        g_max_iterations = (long) floatval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "iterincr")        // iterincr=?
    {
        return cmdarg_flags::NONE;
    }

    if (variable == "passes")        // passes=?
    {
        if (std::strchr("123gbtsdo", charval[0]) == nullptr)
        {
            return bad_arg(curarg);
        }
        g_user_std_calc_mode = charval[0];
        if (charval[0] == 'g')
        {
            g_stop_pass = ((int)value[1] - (int)'0');
            if (g_stop_pass < 0 || g_stop_pass > 6)
            {
                g_stop_pass = 0;
            }
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "ismand")        // ismand=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_is_mandelbrot = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "cyclelimit")   // cyclelimit=?
    {
        if (numval <= 1 || numval > 256)
        {
            return bad_arg(curarg);
        }
        g_init_cycle_limit = numval;
        return cmdarg_flags::NONE;
    }

    if (variable == "makemig")
    {
        int xmult;
        int ymult;
        if (totparms < 2)
        {
            return bad_arg(curarg);
        }
        xmult = intval[0];
        ymult = intval[1];
        make_mig(xmult, ymult);
        exit(0);
    }

    if (variable == "cyclerange")
    {
        if (totparms < 2)
        {
            intval[1] = 255;
        }
        if (totparms < 1)
        {
            intval[0] = 1;
        }
        if (totparms != intparms
            || intval[0] < 0
            || intval[1] > 255
            || intval[0] > intval[1])
        {
            return bad_arg(curarg);
        }
        g_color_cycle_range_lo = intval[0];
        g_color_cycle_range_hi = intval[1];
        return cmdarg_flags::NONE;
    }

    if (variable == "ranges")
    {
        int i;
        int k;
        int entries;
        int prev;
        int tmpranges[128];

        if (totparms != intparms)
        {
            return bad_arg(curarg);
        }
        i = 0;
        prev = i;
        entries = prev;
        g_log_map_flag = 0; // ranges overrides logmap
        while (i < totparms)
        {
            k = intval[i++];
            if (k < 0) // striping
            {
                k = -k;
                if (k < 1 || k >= 16384 || i >= totparms)
                {
                    return bad_arg(curarg);
                }
                tmpranges[entries++] = -1; // {-1,width,limit} for striping
                tmpranges[entries++] = k;
                k = intval[i++];
            }
            if (k < prev)
            {
                return bad_arg(curarg);
            }
            prev = k;
            tmpranges[entries++] = prev;
        }
        if (prev == 0)
        {
            return bad_arg(curarg);
        }
        bool resized{};
        try
        {
            g_iteration_ranges.resize(entries);
            resized = true;
        }
        catch (std::bad_alloc const &)
        {
        }
        if (!resized)
        {
            stopmsg(stopmsg_flags::NO_STACK, "Insufficient memory for ranges=");
            return cmdarg_flags::ERROR;
        }
        g_iteration_ranges_len = entries;
        for (int i2 = 0; i2 < g_iteration_ranges_len; ++i2)
        {
            g_iteration_ranges[i2] = tmpranges[i2];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "savename")      // savename=?
    {
        if (valuelen > FILE_MAX_PATH - 1)
        {
            return bad_arg(curarg);
        }
        if (g_first_init || mode == cmd_file::AT_AFTER_STARTUP)
        {
            if (merge_pathnames(g_save_filename, value, mode) < 0)
            {
                init_msg(variable.c_str(), value, mode);
            }
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "tweaklzw")      // tweaklzw=?
    {
        // TODO: deprecated
        return cmdarg_flags::NONE;
    }

    if (variable == "minstack")      // minstack=?
    {
        if (totparms != 1)
        {
            return bad_arg(curarg);
        }
        g_soi_min_stack = intval[0];
        return cmdarg_flags::NONE;
    }

    if (variable == "mathtolerance")      // mathtolerance=?
    {
        if (charval[0] == '/')
        {
            ; // leave math_tol[0] at the default value
        }
        else if (totparms >= 1)
        {
            g_math_tol[0] = floatval[0];
        }
        if (totparms >= 2)
        {
            g_math_tol[1] = floatval[1];
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "tempdir")      // tempdir=?
    {
        if (valuelen > (FILE_MAX_DIR-1))
        {
            return bad_arg(curarg);
        }
        if (!isadirectory(value))
        {
            return bad_arg(curarg);
        }
        g_temp_dir = value;
        fix_dirname(g_temp_dir);
        return cmdarg_flags::NONE;
    }

    if (variable == "workdir")      // workdir=?
    {
        if (valuelen > (FILE_MAX_DIR-1))
        {
            return bad_arg(curarg);
        }
        if (!isadirectory(value))
        {
            return bad_arg(curarg);
        }
        g_working_dir = value;
        fix_dirname(g_working_dir);
        return cmdarg_flags::NONE;
    }

    if (variable == "exitmode")      // exitmode=? (deprecated)
    {
        return cmdarg_flags::NONE;
    }

    if (variable == "textcolors")
    {
        parse_textcolors(value);
        return cmdarg_flags::NONE;
    }

    if (variable == "potential")     // potential=?
    {
        int k{};
        while (k < 3 && *value)
        {
            if (k == 1)
            {
                g_potential_params[k] = std::atof(value);
            }
            else
            {
                g_potential_params[k] = std::atoi(value);
            }
            k++;
            value = std::strchr(value, '/');
            if (value == nullptr)
            {
                k = 99;
            }
            ++value;
        }
        g_potential_16bit = false;
        if (k < 99)
        {
            if (std::strcmp(value, "16bit"))
            {
                return bad_arg(curarg);
            }
            g_potential_16bit = true;
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "params")        // params=?,?
    {
        if (totparms != floatparms || totparms > MAX_PARAMS)
        {
            return bad_arg(curarg);
        }
        s_init_params = true;
        for (int k = 0; k < MAX_PARAMS; ++k)
        {
            g_params[k] = (k < totparms) ? floatval[k] : 0.0;
        }
        if (bf_math != bf_math_type::NONE)
        {
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                floattobf(bfparms[k], g_params[k]);
            }
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "miim")          // miim=?[/?[/?[/?]]]
    {
        if (totparms > 6)
        {
            return bad_arg(curarg);
        }
        if (charval[0] == 'b')
        {
            g_major_method = Major::breadth_first;
        }
        else if (charval[0] == 'd')
        {
            g_major_method = Major::depth_first;
        }
        else if (charval[0] == 'w')
        {
            g_major_method = Major::random_walk;
        }
#ifdef RANDOM_RUN
        else if (charval[0] == 'r')
        {
            major_method = Major::random_run;
        }
#endif
        else
        {
            return bad_arg(curarg);
        }

        if (charval[1] == 'l')
        {
            g_inverse_julia_minor_method = Minor::left_first;
        }
        else if (charval[1] == 'r')
        {
            g_inverse_julia_minor_method = Minor::right_first;
        }
        else
        {
            return bad_arg(curarg);
        }

        // keep this next part in for backwards compatibility with old PARs ???

        if (totparms > 2)
        {
            for (int k = 2; k < 6; ++k)
            {
                g_params[k-2] = (k < totparms) ? floatval[k] : 0.0;
            }
        }

        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "initorbit")     // initorbit=?,?
    {
        if (std::strcmp(value, "pixel") == 0)
        {
            g_use_init_orbit = init_orbit_mode::pixel;
        }
        else
        {
            if (totparms != 2 || floatparms != 2)
            {
                return bad_arg(curarg);
            }
            g_init_orbit.x = floatval[0];
            g_init_orbit.y = floatval[1];
            g_use_init_orbit = init_orbit_mode::value;
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitname")         // orbitname=?
    {
        if (check_orbit_name(value))
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "3dmode")         // 3dmode=?
    {
        int julibrot_mode = -1;
        for (int i = 0; i < 4; i++)
        {
            if (g_julibrot_3d_options[i] == std::string{value})
            {
                julibrot_mode = i;
                break;
            }
        }
        if (julibrot_mode < 0)
        {
            return bad_arg(curarg);
        }
        g_julibrot_3d_mode = static_cast<julibrot_3d_mode>(julibrot_mode);
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "julibrot3d")       // julibrot3d=?,?,?,?
    {
        if (floatparms != totparms)
        {
            return bad_arg(curarg);
        }
        if (totparms > 0)
        {
            g_julibrot_z_dots = (int)floatval[0];
        }
        if (totparms > 1)
        {
            g_julibrot_origin_fp = (float)floatval[1];
        }
        if (totparms > 2)
        {
            g_julibrot_depth_fp = (float)floatval[2];
        }
        if (totparms > 3)
        {
            g_julibrot_height_fp = (float)floatval[3];
        }
        if (totparms > 4)
        {
            g_julibrot_width_fp = (float)floatval[4];
        }
        if (totparms > 5)
        {
            g_julibrot_dist_fp = (float)floatval[5];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "julibroteyes")       // julibroteyes=?,?,?,?
    {
        if (floatparms != totparms || totparms != 1)
        {
            return bad_arg(curarg);
        }
        g_eyes_fp = (float)floatval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "julibrotfromto")       // julibrotfromto=?,?,?,?
    {
        if (floatparms != totparms || totparms != 4)
        {
            return bad_arg(curarg);
        }
        g_julibrot_x_max = floatval[0];
        g_julibrot_x_min = floatval[1];
        g_julibrot_y_max = floatval[2];
        g_julibrot_y_min = floatval[3];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "corners")       // corners=?,?,?,?
    {
        int dec;
        if (g_fractal_type == fractal_type::CELLULAR)
        {
            return cmdarg_flags::FRACTAL_PARAM; // skip setting the corners
        }
        if (floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
        {
            return bad_arg(curarg);
        }
        g_use_center_mag = false;
        if (totparms == 0)
        {
            return cmdarg_flags::NONE; // turns corners mode on
        }
        s_init_corners = true;
        // good first approx, but dec could be too big
        dec = get_max_curarg_len(floatvalstr, totparms) + 1;
        if ((dec > DBL_DIG+1 || g_debug_flag == debug_flags::force_arbitrary_precision_math)
            && g_debug_flag != debug_flags::prevent_arbitrary_precision_math)
        {
            bf_math_type old_bf_math = bf_math;
            if (bf_math == bf_math_type::NONE || dec > g_decimals)
            {
                init_bf_dec(dec);
            }
            if (old_bf_math == bf_math_type::NONE)
            {
                for (int k = 0; k < MAX_PARAMS; k++)
                {
                    floattobf(bfparms[k], g_params[k]);
                }
            }

            // xx3rd = xxmin = floatval[0];
            get_bf(g_bf_x_min, floatvalstr[0]);
            get_bf(g_bf_x_3rd, floatvalstr[0]);

            // xxmax = floatval[1];
            get_bf(g_bf_x_max, floatvalstr[1]);

            // yy3rd = yymin = floatval[2];
            get_bf(g_bf_y_min, floatvalstr[2]);
            get_bf(g_bf_y_3rd, floatvalstr[2]);

            // yymax = floatval[3];
            get_bf(g_bf_y_max, floatvalstr[3]);

            if (totparms == 6)
            {
                // xx3rd = floatval[4];
                get_bf(g_bf_x_3rd, floatvalstr[4]);

                // yy3rd = floatval[5];
                get_bf(g_bf_y_3rd, floatvalstr[5]);
            }

            // now that all the corners have been read in, get a more
            // accurate value for dec and do it all again

            dec = getprecbf_mag();
            if (dec < 0)
            {
                return bad_arg(curarg);     // ie: Magnification is +-1.#INF
            }

            if (dec > g_decimals)  // get corners again if need more precision
            {
                init_bf_dec(dec);

                // now get parameters and corners all over again at new
                // decimal setting
                for (int k = 0; k < MAX_PARAMS; k++)
                {
                    floattobf(bfparms[k], g_params[k]);
                }

                // xx3rd = xxmin = floatval[0];
                get_bf(g_bf_x_min, floatvalstr[0]);
                get_bf(g_bf_x_3rd, floatvalstr[0]);

                // xxmax = floatval[1];
                get_bf(g_bf_x_max, floatvalstr[1]);

                // yy3rd = yymin = floatval[2];
                get_bf(g_bf_y_min, floatvalstr[2]);
                get_bf(g_bf_y_3rd, floatvalstr[2]);

                // yymax = floatval[3];
                get_bf(g_bf_y_max, floatvalstr[3]);

                if (totparms == 6)
                {
                    // xx3rd = floatval[4];
                    get_bf(g_bf_x_3rd, floatvalstr[4]);

                    // yy3rd = floatval[5];
                    get_bf(g_bf_y_3rd, floatvalstr[5]);
                }
            }
        }
        g_x_min = floatval[0];
        g_x_3rd = floatval[0];
        g_x_max = floatval[1];
        g_y_min = floatval[2];
        g_y_3rd = floatval[2];
        g_y_max = floatval[3];

        if (totparms == 6)
        {
            g_x_3rd =      floatval[4];
            g_y_3rd =      floatval[5];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitcorners")  // orbit corners=?,?,?,?
    {
        g_set_orbit_corners = false;
        if (floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
        {
            return bad_arg(curarg);
        }
        g_orbit_corner_min_x = floatval[0];
        g_orbit_corner_3_x = floatval[0];
        g_orbit_corner_max_x = floatval[1];
        g_orbit_corner_min_y = floatval[2];
        g_orbit_corner_3_y = floatval[2];
        g_orbit_corner_max_y = floatval[3];

        if (totparms == 6)
        {
            g_orbit_corner_3_x =      floatval[4];
            g_orbit_corner_3_y =      floatval[5];
        }
        g_set_orbit_corners = true;
        g_keep_screen_coords = true;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "screencoords")     // screencoords=?
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_keep_screen_coords = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitdrawmode")     // orbitdrawmode=?
    {
        if (charval[0] != 'l' && charval[0] != 'r' && charval[0] != 'f')
        {
            return bad_arg(curarg);
        }
        g_draw_mode = charval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "viewwindows")
    {
        // viewwindows=?,?,?,?,?
        if (totparms > 5 || floatparms-intparms > 2 || intparms > 4)
        {
            return bad_arg(curarg);
        }
        g_view_window = true;
        g_view_reduction = 4.2F;  // reset default values
        g_final_aspect_ratio = g_screen_aspect;
        g_view_crop = true;
        g_view_y_dots = 0;
        g_view_x_dots = g_view_y_dots;

        if ((totparms > 0) && (floatval[0] > 0.001))
        {
            g_view_reduction = (float)floatval[0];
        }
        if ((totparms > 1) && (floatval[1] > 0.001))
        {
            g_final_aspect_ratio = (float)floatval[1];
        }
        if ((totparms > 2) && (yesnoval[2] == 0))
        {
            g_view_crop = yesnoval[2] != 0;
        }
        if ((totparms > 3) && (intval[3] > 0))
        {
            g_view_x_dots = intval[3];
        }
        if ((totparms == 5) && (intval[4] > 0))
        {
            g_view_y_dots = intval[4];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "center-mag")
    {
        // center-mag=?,?,?[,?,?,?]
        int dec;

        if ((totparms != floatparms)
            || (totparms != 0 && totparms < 3)
            || (totparms >= 3 && floatval[2] == 0.0))
        {
            return bad_arg(curarg);
        }
        if (g_fractal_type == fractal_type::CELLULAR)
        {
            return cmdarg_flags::FRACTAL_PARAM; // skip setting the corners
        }
        g_use_center_mag = true;
        if (totparms == 0)
        {
            return cmdarg_flags::NONE; // turns center-mag mode on
        }
        s_init_corners = true;
        // dec = get_max_curarg_len(floatvalstr, totparms);
        std::sscanf(floatvalstr[2], "%Lf", &Magnification);

        // I don't know if this is portable, but something needs to
        // be used in case compiler's LDBL_MAX is not big enough
        if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
        {
            return bad_arg(curarg);     // ie: Magnification is +-1.#INF
        }

        dec = getpower10(Magnification) + 4; // 4 digits of padding sounds good

        if ((dec <= DBL_DIG+1 && g_debug_flag != debug_flags::force_arbitrary_precision_math)
            || g_debug_flag == debug_flags::prevent_arbitrary_precision_math)
        {
            // rough estimate that double is OK
            Xctr = floatval[0];
            Yctr = floatval[1];
            Xmagfactor = 1;
            Rotation = 0;
            Skew = 0;
            if (floatparms > 3)
            {
                Xmagfactor = floatval[3];
            }
            if (Xmagfactor == 0)
            {
                Xmagfactor = 1;
            }
            if (floatparms > 4)
            {
                Rotation = floatval[4];
            }
            if (floatparms > 5)
            {
                Skew = floatval[5];
            }
            // calculate bounds
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
            return cmdarg_flags::FRACTAL_PARAM;
        }


        // use arbitrary precision
        int saved;
        s_init_corners = true;
        bf_math_type old_bf_math = bf_math;
        if (bf_math == bf_math_type::NONE || dec > g_decimals)
        {
            init_bf_dec(dec);
        }
        if (old_bf_math == bf_math_type::NONE)
        {
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                floattobf(bfparms[k], g_params[k]);
            }
        }
        g_use_center_mag = true;
        saved = save_stack();
        bXctr            = alloc_stack(bflength+2);
        bYctr            = alloc_stack(bflength+2);
        get_bf(bXctr, floatvalstr[0]);
        get_bf(bYctr, floatvalstr[1]);
        Xmagfactor = 1;
        Rotation = 0;
        Skew = 0;
        if (floatparms > 3)
        {
            Xmagfactor = floatval[3];
        }
        if (Xmagfactor == 0)
        {
            Xmagfactor = 1;
        }
        if (floatparms > 4)
        {
            Rotation = floatval[4];
        }
        if (floatparms > 5)
        {
            Skew = floatval[5];
        }
        // calculate bounds
        cvtcornersbf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
        bfcornerstofloat();
        restore_stack(saved);
        return cmdarg_flags::FRACTAL_PARAM;

    }

    if (variable == "aspectdrift")
    {
        // aspectdrift=?
        if (floatparms != 1 || floatval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_aspect_drift = (float)floatval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "invert")
    {
        // invert=?,?,?
        if (totparms != floatparms || (totparms != 1 && totparms != 3))
        {
            return bad_arg(curarg);
        }
        g_inversion[0] = floatval[0];
        g_invert = (g_inversion[0] != 0.0) ? totparms : 0;
        if (totparms == 3)
        {
            g_inversion[1] = floatval[1];
            g_inversion[2] = floatval[2];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "olddemmcolors")
    {
        // olddemmcolors=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_old_demm_colors = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "askvideo")
    {
        // askvideo=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_ask_video = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "ramvideo")          // ramvideo=?
    {
        return cmdarg_flags::NONE; // just ignore and return, for old time's sake
    }

    if (variable == "float")
    {
        // float=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_user_float_flag = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "fastrestore")
    {
        // fastrestore=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_fast_restore = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "orgfrmdir")
    {
        // orgfrmdir=?
        if (valuelen > (FILE_MAX_DIR-1))
        {
            return bad_arg(curarg);
        }
        if (!isadirectory(value))
        {
            return bad_arg(curarg);
        }
        g_organize_formulas_search = true;
        g_organize_formulas_dir = value;
        fix_dirname(g_organize_formulas_dir);
        return cmdarg_flags::NONE;
    }

    if (variable == "biomorph")
    {
        // biomorph=?
        g_user_biomorph_value = numval;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitsave")
    {
        // orbitsave=?
        if (charval[0] == 's')
        {
            g_orbit_save_flags |= osf_midi;
        }
        else if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_orbit_save_flags |= (yesnoval[0] ? osf_raw : 0);
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitsavename")
    {
        // orbitsavename=?
        g_orbit_save_name = value;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "bailout")
    {
        // bailout=?
        if (floatval[0] < 1 || floatval[0] > 2100000000L)
        {
            return bad_arg(curarg);
        }
        g_bail_out = (long)floatval[0];
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "bailoutest")
    {
        // bailoutest=?
        if (std::strcmp(value, "mod") == 0)
        {
            g_bail_out_test = bailouts::Mod;
        }
        else if (std::strcmp(value, "real") == 0)
        {
            g_bail_out_test = bailouts::Real;
        }
        else if (std::strcmp(value, "imag") == 0)
        {
            g_bail_out_test = bailouts::Imag;
        }
        else if (std::strcmp(value, "or") == 0)
        {
            g_bail_out_test = bailouts::Or;
        }
        else if (std::strcmp(value, "and") == 0)
        {
            g_bail_out_test = bailouts::And;
        }
        else if (std::strcmp(value, "manh") == 0)
        {
            g_bail_out_test = bailouts::Manh;
        }
        else if (std::strcmp(value, "manr") == 0)
        {
            g_bail_out_test = bailouts::Manr;
        }
        else
        {
            return bad_arg(curarg);
        }
        set_bailout_formula(g_bail_out_test);
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "symmetry")
    {
        // symmetry=?
        if (std::strcmp(value, "xaxis") == 0)
        {
            g_force_symmetry = symmetry_type::X_AXIS;
        }
        else if (std::strcmp(value, "yaxis") == 0)
        {
            g_force_symmetry = symmetry_type::Y_AXIS;
        }
        else if (std::strcmp(value, "xyaxis") == 0)
        {
            g_force_symmetry = symmetry_type::XY_AXIS;
        }
        else if (std::strcmp(value, "origin") == 0)
        {
            g_force_symmetry = symmetry_type::ORIGIN;
        }
        else if (std::strcmp(value, "pi") == 0)
        {
            g_force_symmetry = symmetry_type::PI_SYM;
        }
        else if (std::strcmp(value, "none") == 0)
        {
            g_force_symmetry = symmetry_type::NONE;
        }
        else
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    // deprecated print parameters
    if ((variable == "printer")
        || (variable == "printfile")
        || (variable == "rleps")
        || (variable == "colorps")
        || (variable == "epsf")
        || (variable == "title")
        || (variable == "translate")
        || (variable == "plotstyle")
        || (variable == "halftone")
        || (variable == "linefeed")
        || (variable == "comport"))
    {
        return cmdarg_flags::NONE;
    }

    if (variable == "sound")
    {
        // sound=?,?,?
        if (totparms > 5)
        {
            return bad_arg(curarg);
        }
        g_sound_flag = SOUNDFLAG_OFF; // start with a clean slate, add bits as we go
        if (totparms == 1)
        {
            g_sound_flag = SOUNDFLAG_SPEAKER; // old command, default to PC speaker
        }

        // soundflag is used as a bitfield... bit 0,1,2 used for whether sound
        // is modified by an orbits x,y,or z component. and also to turn it on
        // or off (0==off, 1==beep (or yes), 2==x, 3==y, 4==z),
        // Bit 3 is used for flagging the PC speaker sound,
        // Bit 4 for OPL3 FM soundcard output,
        // Bit 5 will be for midi output (not yet),
        // Bit 6 for whether the tone is quantised to the nearest 'proper' note
        //  (according to the western, even tempered system anyway)
        if (charval[0] == 'n' || charval[0] == 'o')
        {
            g_sound_flag &= ~SOUNDFLAG_ORBITMASK;
        }
        else if ((std::strncmp(value, "ye", 2) == 0) || (charval[0] == 'b'))
        {
            g_sound_flag |= SOUNDFLAG_BEEP;
        }
        else if (charval[0] == 'x')
        {
            g_sound_flag |= SOUNDFLAG_X;
        }
        else if (charval[0] == 'y' && std::strncmp(value, "ye", 2) != 0)
        {
            g_sound_flag |= SOUNDFLAG_Y;
        }
        else if (charval[0] == 'z')
        {
            g_sound_flag |= SOUNDFLAG_Z;
        }
        else
        {
            return bad_arg(curarg);
        }
        if (totparms > 1)
        {
            g_sound_flag &= SOUNDFLAG_ORBITMASK; // reset options
            for (int i = 1; i < totparms; i++)
            {
                // this is for 2 or more options at the same time
                if (charval[i] == 'f')
                {
                    // (try to)switch on opl3 fm synth
                    if (driver_init_fm())
                    {
                        g_sound_flag |= SOUNDFLAG_OPL3_FM;
                    }
                    else
                    {
                        g_sound_flag &= ~SOUNDFLAG_OPL3_FM;
                    }
                }
                else if (charval[i] == 'p')
                {
                    g_sound_flag |= SOUNDFLAG_SPEAKER;
                }
                else if (charval[i] == 'm')
                {
                    g_sound_flag |= SOUNDFLAG_MIDI;
                }
                else if (charval[i] == 'q')
                {
                    g_sound_flag |= SOUNDFLAG_QUANTIZED;
                }
                else
                {
                    return bad_arg(curarg);
                }
            } // end for
        }    // end totparms > 1
        return cmdarg_flags::NONE;
    }

    if (variable == "hertz")
    {
        // Hertz=?
        g_base_hertz = numval;
        return cmdarg_flags::NONE;
    }

    if (variable == "volume")
    {
        // Volume =?
        g_fm_volume = numval & 0x3F; // 63
        return cmdarg_flags::NONE;
    }

    if (variable == "attenuate")
    {
        if (charval[0] == 'n')
        {
            g_hi_attenuation = 0;
        }
        else if (charval[0] == 'l')
        {
            g_hi_attenuation = 1;
        }
        else if (charval[0] == 'm')
        {
            g_hi_attenuation = 2;
        }
        else if (charval[0] == 'h')
        {
            g_hi_attenuation = 3;
        }
        else
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "polyphony")
    {
        if (numval > 9)
        {
            return bad_arg(curarg);
        }
        g_polyphony = std::abs(numval-1);
        return cmdarg_flags::NONE;
    }

    if (variable == "wavetype")
    {
        // wavetype = ?
        g_fm_wavetype = numval & 0x0F;
        return cmdarg_flags::NONE;
    }

    if (variable == "attack")
    {
        // attack = ?
        g_fm_attack = numval & 0x0F;
        return cmdarg_flags::NONE;
    }

    if (variable == "decay")
    {
        // decay = ?
        g_fm_decay = numval & 0x0F;
        return cmdarg_flags::NONE;
    }

    if (variable == "sustain")
    {
        // sustain = ?
        g_fm_sustain = numval & 0x0F;
        return cmdarg_flags::NONE;
    }

    if (variable == "srelease")
    {
        // release = ?
        g_fm_release = numval & 0x0F;
        return cmdarg_flags::NONE;
    }

    if (variable == "scalemap")
    {
        // Scalemap=?,?,?,?,?,?,?,?,?,?,?
        if (totparms != intparms)
        {
            return bad_arg(curarg);
        }
        for (int counter = 0; counter <= 11; counter++)
        {
            if ((totparms > counter) && (intval[counter] > 0)
               && (intval[counter] < 13))
            {
                g_scale_map[counter] = intval[counter];
            }
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "periodicity")
    {
        // periodicity=?
        g_user_periodicity_value = 1;
        if ((charval[0] == 'n') || (numval == 0))
        {
            g_user_periodicity_value = 0;
        }
        else if (charval[0] == 'y')
        {
            g_user_periodicity_value = 1;
        }
        else if (charval[0] == 's')       // 's' for 'show'
        {
            g_user_periodicity_value = -1;
        }
        else if (numval == NONNUMERIC)
        {
            return bad_arg(curarg);
        }
        else if (numval != 0)
        {
            g_user_periodicity_value = numval;
            if (g_user_periodicity_value > 255)
            {
                g_user_periodicity_value = 255;
            }
            if (g_user_periodicity_value < -255)
            {
                g_user_periodicity_value = -255;
            }
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "logmap")
    {
        // logmap=?
        g_log_map_auto_calculate = false;          // turn this off if loading a PAR
        if (charval[0] == 'y')
        {
            g_log_map_flag = 1;                           // palette is logarithmic
        }
        else if (charval[0] == 'n')
        {
            g_log_map_flag = 0;
        }
        else if (charval[0] == 'o')
        {
            g_log_map_flag = -1;                          // old log palette
        }
        else
        {
            g_log_map_flag = (long)floatval[0];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "logmode")
    {
        // logmode=?
        g_log_map_fly_calculate = 0;                         // turn off if error
        g_log_map_auto_calculate = false;
        if (charval[0] == 'f')
        {
            g_log_map_fly_calculate = 1;                      // calculate on the fly
        }
        else if (charval[0] == 't')
        {
            g_log_map_fly_calculate = 2;                      // force use of LogTable
        }
        else if (charval[0] == 'a')
        {
            g_log_map_auto_calculate = true;        // force auto calc of logmap
        }
        else
        {
            return bad_arg(curarg);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "debugflag" || variable == "debug")
    {
        // internal use only
        g_debug_flag = static_cast<debug_flags>(numval);
        g_timer_flag = (g_debug_flag & debug_flags::benchmark_timer) != debug_flags::none; // separate timer flag
        g_debug_flag &= ~debug_flags::benchmark_timer;
        return cmdarg_flags::NONE;
    }

    if (variable == "rseed")
    {
        g_random_seed = numval;
        g_random_seed_flag = true;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "orbitdelay")
    {
        g_orbit_delay = numval;
        return cmdarg_flags::NONE;
    }

    if (variable == "orbitinterval")
    {
        g_orbit_interval = numval;
        if (g_orbit_interval < 1)
        {
            g_orbit_interval = 1;
        }
        if (g_orbit_interval > 255)
        {
            g_orbit_interval = 255;
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "showdot")
    {
        g_show_dot = 15;
        if (totparms > 0)
        {
            g_auto_show_dot = (char)0;
            if (std::isalpha(charval[0]))
            {
                if (std::strchr("abdm", (int)charval[0]) != nullptr)
                {
                    g_auto_show_dot = charval[0];
                }
                else
                {
                    return bad_arg(curarg);
                }
            }
            else
            {
                g_show_dot = numval;
                if (g_show_dot < 0)
                {
                    g_show_dot = -1;
                }
            }
            if (totparms > 1 && intparms > 0)
            {
                g_size_dot = intval[1];
            }
            if (g_size_dot < 0)
            {
                g_size_dot = 0;
            }
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "showorbit") // showorbit=yes|no
    {
        g_start_show_orbit = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "decomp")
    {
        if (totparms != intparms || totparms < 1)
        {
            return bad_arg(curarg);
        }
        g_decomp[0] = intval[0];
        g_decomp[1] = 0;
        if (totparms > 1) // backward compatibility
        {
            g_decomp[1] = intval[1];
            g_bail_out = g_decomp[1];
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "distest")
    {
        if (totparms != intparms || totparms < 1)
        {
            return bad_arg(curarg);
        }
        g_user_distance_estimator_value = (long)floatval[0];
        g_distance_estimator_width_factor = 71;
        if (totparms > 1)
        {
            g_distance_estimator_width_factor = intval[1];
        }
        if (totparms > 3 && intval[2] > 0 && intval[3] > 0)
        {
            g_distance_estimator_x_dots = intval[2];
            g_distance_estimator_y_dots = intval[3];
        }
        else
        {
            g_distance_estimator_y_dots = 0;
            g_distance_estimator_x_dots = 0;
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "formulafile")
    {
        // formulafile=?
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        if (merge_pathnames(g_formula_filename, value, mode) < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "formulaname")
    {
        // formulaname=?
        if (valuelen > ITEM_NAME_LEN)
        {
            return bad_arg(curarg);
        }
        g_formula_name = value;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "lfile")
    {
        // lfile=?
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        if (merge_pathnames(g_l_system_filename, value, mode) < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "lname")
    {
        if (valuelen > ITEM_NAME_LEN)
        {
            return bad_arg(curarg);
        }
        g_l_system_name = value;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "ifsfile")
    {
        // ifsfile=??
        int existdir;
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        existdir = merge_pathnames(g_ifs_filename, value, mode);
        if (existdir == 0)
        {
            reset_ifs_defn();
        }
        else if (existdir < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }


    if (variable == "ifs" || variable == "ifs3d")
    {
        // ifs3d for old time's sake
        if (valuelen > ITEM_NAME_LEN)
        {
            return bad_arg(curarg);
        }
        g_ifs_name = value;
        reset_ifs_defn();
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "parmfile")
    {
        // parmfile=?
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        if (merge_pathnames(g_command_file, value, mode) < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
        return cmdarg_flags::FRACTAL_PARAM;
    }

    if (variable == "stereo")
    {
        // stereo=?
        if ((numval < 0) || (numval > 4))
        {
            return bad_arg(curarg);
        }
        g_glasses_type = numval;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "rotation")
    {
        // rotation=?/?/?
        if (totparms != 3 || intparms != 3)
        {
            return bad_arg(curarg);
        }
        XROT = intval[0];
        YROT = intval[1];
        ZROT = intval[2];
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "perspective")
    {
        // perspective=?
        if (numval == NONNUMERIC)
        {
            return bad_arg(curarg);
        }
        ZVIEWER = numval;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "xyshift")
    {
        // xyshift=?/?
        if (totparms != 2 || intparms != 2)
        {
            return bad_arg(curarg);
        }
        XSHIFT = intval[0];
        YSHIFT = intval[1];
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "interocular")
    {
        // interocular=?
        g_eye_separation = numval;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "converge")
    {
        // converg=?
        g_converge_x_adjust = numval;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "crop")
    {
        // crop=?
        if (totparms != 4 || intparms != 4
            || intval[0] < 0 || intval[0] > 100
            || intval[1] < 0 || intval[1] > 100
            || intval[2] < 0 || intval[2] > 100
            || intval[3] < 0 || intval[3] > 100)
        {
            return bad_arg(curarg);
        }
        g_red_crop_left   = intval[0];
        g_red_crop_right  = intval[1];
        g_blue_crop_left  = intval[2];
        g_blue_crop_right = intval[3];
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "bright")
    {
        // bright=?
        if (totparms != 2 || intparms != 2)
        {
            return bad_arg(curarg);
        }
        g_red_bright  = intval[0];
        g_blue_bright = intval[1];
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "xyadjust")
    {
        // trans=?
        if (totparms != 2 || intparms != 2)
        {
            return bad_arg(curarg);
        }
        g_adjust_3d_x = intval[0];
        g_adjust_3d_y = intval[1];
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "3d")
    {
        // 3d=?/?/..
        if (std::strcmp(value, "overlay") == 0)
        {
            yesnoval[0] = 1;
            if (g_calc_status > calc_status_value::NO_FRACTAL)   // if no image, treat same as 3D=yes
            {
                g_overlay_3d = true;
            }
        }
        else if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_display_3d = yesnoval[0] != 0 ? display_3d_modes::YES : display_3d_modes::NONE;
        initvars_3d();
        return g_display_3d != display_3d_modes::NONE ? (cmdarg_flags::PARAM_3D | cmdarg_flags::YES_3D) : cmdarg_flags::PARAM_3D;
    }

    if (variable == "sphere")
    {
        // sphere=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        SPHERE = yesnoval[0];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "scalexyz")
    {
        // scalexyz=?/?/?
        if (totparms < 2 || intparms != totparms)
        {
            return bad_arg(curarg);
        }
        XSCALE = intval[0];
        YSCALE = intval[1];
        if (totparms > 2)
        {
            ROUGH = intval[2];
        }
        return cmdarg_flags::PARAM_3D;
    }

    // "rough" is really scale z, but we add it here for convenience
    if (variable == "roughness")
    {
        // roughness=?
        ROUGH = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "waterline")
    {
        // waterline=?
        if (numval < 0)
        {
            return bad_arg(curarg);
        }
        WATERLINE = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "filltype")
    {
        // filltype=?
        if (numval < -1 || numval > 6)
        {
            return bad_arg(curarg);
        }
        FILLTYPE = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "lightsource")
    {
        // lightsource=?/?/?
        if (totparms != 3 || intparms != 3)
        {
            return bad_arg(curarg);
        }
        XLIGHT = intval[0];
        YLIGHT = intval[1];
        ZLIGHT = intval[2];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "smoothing")
    {
        // smoothing=?
        if (numval < 0)
        {
            return bad_arg(curarg);
        }
        LIGHTAVG = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "latitude")
    {
        // latitude=?/?
        if (totparms != 2 || intparms != 2)
        {
            return bad_arg(curarg);
        }
        THETA1 = intval[0];
        THETA2 = intval[1];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "longitude")
    {
        // longitude=?/?
        if (totparms != 2 || intparms != 2)
        {
            return bad_arg(curarg);
        }
        PHI1 = intval[0];
        PHI2 = intval[1];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "radius")
    {
        // radius=?
        if (numval < 0)
        {
            return bad_arg(curarg);
        }
        RADIUS = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "transparent")
    {
        // transparent?
        if (totparms != intparms || totparms < 1)
        {
            return bad_arg(curarg);
        }
        g_transparent_color_3d[0] = intval[0];
        g_transparent_color_3d[1] = g_transparent_color_3d[0];
        if (totparms > 1)
        {
            g_transparent_color_3d[1] = intval[1];
        }
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "preview")
    {
        // preview?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_preview = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "showbox")
    {
        // showbox?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_show_box = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "coarse")
    {
        // coarse=?
        if (numval < 3 || numval > 2000)
        {
            return bad_arg(curarg);
        }
        g_preview_factor = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "randomize")
    {
        // RANDOMIZE=?
        if (numval < 0 || numval > 7)
        {
            return bad_arg(curarg);
        }
        g_randomize_3d = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "ambient")
    {
        // ambient=?
        if (numval < 0 || numval > 100)
        {
            return bad_arg(curarg);
        }
        g_ambient = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "haze")
    {
        // haze=?
        if (numval < 0 || numval > 100)
        {
            return bad_arg(curarg);
        }
        g_haze = numval;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "fullcolor")
    {
        // fullcolor=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_targa_out = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "truecolor")
    {
        // truecolor=?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_truecolor = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "truemode")
    {
        // truemode=?
        g_true_mode = true_color_mode::default_color;
        if (charval[0] == 'd')
        {
            g_true_mode = true_color_mode::default_color;
        }
        if (charval[0] == 'i' || intval[0] == 1)
        {
            g_true_mode = true_color_mode::iterate;
        }
        return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
    }

    if (variable == "usegrayscale")
    {
        // usegrayscale?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_gray_flag = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    // TODO: deprecate monitorwidth parameter
    if (variable == "monitorwidth" || variable == "stereowidth")
    {
        // monitorwidth/stereowidth=?
        if (totparms != 1 || floatparms != 1)
        {
            return bad_arg(curarg);
        }
        g_auto_stereo_width  = floatval[0];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "targa_overlay")
    {
        // Targa Overlay?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_targa_overlay = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "background")
    {
        // background=?/?
        if (totparms != 3 || intparms != 3)
        {
            return bad_arg(curarg);
        }
        for (int i = 0; i < 3; i++)
        {
            if (intval[i] & ~0xff)
            {
                return bad_arg(curarg);
            }
        }
        g_background_color[0] = (BYTE)intval[0];
        g_background_color[1] = (BYTE)intval[1];
        g_background_color[2] = (BYTE)intval[2];
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "lightname")
    {
        // lightname=?
        if (valuelen > (FILE_MAX_PATH-1))
        {
            return bad_arg(curarg);
        }
        if (g_first_init || mode == cmd_file::AT_AFTER_STARTUP)
        {
            g_light_name = value;
        }
        return cmdarg_flags::NONE;
    }

    if (variable == "ray")
    {
        // RAY=?
        if (numval < 0 || numval > 6)
        {
            return bad_arg(curarg);
        }
        g_raytrace_format = static_cast<raytrace_formats>(numval);
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "brief")
    {
        // BRIEF?
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_brief = yesnoval[0] != 0;
        return cmdarg_flags::PARAM_3D;
    }

    if (variable == "release")
    {
        // release
        return bad_arg(curarg);
    }

    if (variable == "curdir")
    {
        // curdir=
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_check_cur_dir = yesnoval[0] != 0;
        return cmdarg_flags::NONE;
    }

    if (variable == "virtual")         // virtual=
    {
        if (yesnoval[0] < 0)
        {
            return bad_arg(curarg);
        }
        g_virtual_screens = yesnoval[0] != 0;
        return cmdarg_flags::FRACTAL_PARAM;
    }

    return bad_arg(curarg);
}

static void parse_textcolors(char const *value)
{
    if (std::strcmp(value, "mono") == 0)
    {
        for (auto & elem : g_text_color)
        {
            elem = BLACK*16 + WHITE;
        }
        g_text_color[28] = WHITE*16 + BLACK;
        g_text_color[27] = WHITE*16 + BLACK;
        g_text_color[20] = WHITE*16 + BLACK;
        g_text_color[14] = WHITE*16 + BLACK;
        g_text_color[13] = WHITE*16 + BLACK;
        g_text_color[12] = WHITE*16 + BLACK;
        g_text_color[6] = WHITE*16 + BLACK;
        g_text_color[25] = BLACK*16 + L_WHITE;
        g_text_color[24] = BLACK*16 + L_WHITE;
        g_text_color[22] = BLACK*16 + L_WHITE;
        g_text_color[17] = BLACK*16 + L_WHITE;
        g_text_color[16] = BLACK*16 + L_WHITE;
        g_text_color[11] = BLACK*16 + L_WHITE;
        g_text_color[5] = BLACK*16 + L_WHITE;
        g_text_color[2] = BLACK*16 + L_WHITE;
        g_text_color[0] = BLACK*16 + L_WHITE;
    }
    else
    {
        int k{};
        while (k < sizeof(g_text_color))
        {
            if (*value == 0)
            {
                break;
            }
            if (*value != '/')
            {
                unsigned int hexval;
                std::sscanf(value, "%x", &hexval);
                unsigned int i = (hexval / 16) & 7;
                unsigned int j = hexval & 15;
                if (i == j || (i == 0 && j == 8))   // force contrast
                {
                    j = 15;
                }
                g_text_color[k] = (BYTE)(i * 16 + j);
                value = std::strchr(value, '/');
                if (value == nullptr)
                {
                    break;
                }
            }
            ++value;
            ++k;
        }
    }
}

static cmdarg_flags parse_colors(char const *value)
{
    if (*value == '@')
    {
        if (merge_pathnames(g_map_name, &value[1], cmd_file::AT_CMD_LINE_SET_NAME) < 0)
        {
            init_msg("", &value[1], cmd_file::AT_CMD_LINE_SET_NAME);
        }
        if ((int)std::strlen(value) > FILE_MAX_PATH || ValidateLuts(g_map_name.c_str()))
        {
            goto badcolor;
        }
        if (g_display_3d != display_3d_modes::NONE)
        {
            g_map_set = true;
        }
        else
        {
            if (merge_pathnames(g_color_file, &value[1], cmd_file::AT_CMD_LINE_SET_NAME) < 0)
            {
                init_msg("", &value[1], cmd_file::AT_CMD_LINE_SET_NAME);
            }
            g_color_state = color_state::MAP_FILE;
        }
    }
    else
    {
        int smooth{};
        int i{};
        while (*value)
        {
            if (i >= 256)
            {
                goto badcolor;
            }
            if (*value == '<')
            {
                if (i == 0
                    || smooth
                    || (smooth = std::atoi(value+1)) < 2
                    || (value = std::strchr(value, '>')) == nullptr)
                {
                    goto badcolor;
                }
                i += smooth;
                ++value;
            }
            else
            {
                for (int j = 0; j < 3; ++j)
                {
                    int k = *(value++);
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
                    g_dac_box[i][j] = (BYTE)k;
                    if (smooth)
                    {
                        int spread = smooth + 1;
                        int start = i - spread;
                        int cnum{};
                        if ((k - (int)g_dac_box[start][j]) == 0)
                        {
                            while (++cnum < spread)
                            {
                                g_dac_box[start+cnum][j] = (BYTE)k;
                            }
                        }
                        else
                        {
                            while (++cnum < spread)
                            {
                                g_dac_box[start+cnum][j] =
                                    (BYTE)((cnum *g_dac_box[i][j]
                                            + (i-(start+cnum))*g_dac_box[start][j]
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
        while (i < 256)
        {
            // zap unset entries
            g_dac_box[i][2] = 40;
            g_dac_box[i][1] = 40;
            g_dac_box[i][0] = 40;
            ++i;
        }
        g_color_state = color_state::UNKNOWN;
    }
    g_colors_preloaded = true;
    std::memcpy(g_old_dac_box, g_dac_box, 256*3);
    return cmdarg_flags::NONE;
badcolor:
    return cmdarg_flags::ERROR;
}

static void argerror(char const *badarg)      // oops. couldn't decode this
{
    std::string spillover;
    if ((int) std::strlen(badarg) > 70)
    {
        spillover = std::string(&badarg[0], &badarg[70]);
        badarg = spillover.c_str();
    }
    std::string msg{"Oops. I couldn't understand the argument:\n  "};
    msg += badarg;

    if (g_first_init)       // this is 1st call to cmdfiles
    {
        msg += "\n"
               "\n"
               "(see the Startup Help screens or documentation for a complete\n"
               " argument list with descriptions)";
    }
    cmd_arg::s_stop_msg(stopmsg_flags::NONE, msg);
    if (g_init_batch != batch_modes::NONE)
    {
        g_init_batch = batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE;
        cmd_arg::s_goodbye();
    }
}

void set_3d_defaults()
{
    ROUGH     = 30;
    WATERLINE = 0;
    ZVIEWER   = 0;
    XSHIFT    = 0;
    YSHIFT    = 0;
    g_adjust_3d_x    = 0;
    g_adjust_3d_y    = 0;
    LIGHTAVG  = 0;
    g_ambient   = 20;
    g_randomize_3d = 0;
    g_haze      = 0;
    g_background_color[0] = 51;
    g_background_color[1] = 153;
    g_background_color[2] = 200;
    if (SPHERE)
    {
        PHI1      =  180;
        PHI2      =  0;
        THETA1    =  -90;
        THETA2    =  90;
        RADIUS    =  100;
        FILLTYPE  = 2;
        XLIGHT    = 1;
        YLIGHT    = 1;
        ZLIGHT    = 1;
    }
    else
    {
        XROT      = 60;
        YROT      = 30;
        ZROT      = 0;
        XSCALE    = 90;
        YSCALE    = 90;
        FILLTYPE  = 0;
        XLIGHT    = 1;
        YLIGHT    = -1;
        ZLIGHT    = 1;
    }
}

// copy a big number from a string, up to slash
static int get_bf(bf_t bf, char const *curarg)
{
    char const *s;
    s = std::strchr(curarg, '/');
    if (s)
    {
        std::string buff(curarg, s);
        strtobf(bf, buff.c_str());
    }
    else
    {
        strtobf(bf, curarg);
    }
    return 0;
}

// Get length of current args
static int get_curarg_len(char const *curarg)
{
    char const *s;
    s = std::strchr(curarg, '/');
    if (s)
    {
        return static_cast<int>(s - curarg);
    }

    return static_cast<int>(std::strlen(curarg));
}

// Get max length of current args
static int get_max_curarg_len(char const *floatvalstr[], int totparms)
{
    int tmp;
    int max_str;
    max_str = 0;
    for (int i = 0; i < totparms; i++)
    {
        tmp = get_curarg_len(floatvalstr[i]);
        if (tmp > max_str)
        {
            max_str = tmp;
        }
    }
    return max_str;
}

static std::string to_string(cmd_file value)
{
    static char const *const modestr[4] = {"command line", "sstools.ini", "PAR file", "PAR file"};
    return modestr[static_cast<int>(value)];
}

// mode = 0 command line @filename
//        1 sstools.ini
//        2 <@> command after startup
//        3 command line @filename/setname
// this is like stopmsg() but can be used in cmdfiles()
// call with NULL for badfilename to get pause for driver_get_key()
int init_msg(char const *cmdstr, char const *badfilename, cmd_file mode)
{
    static int row = 1;

    if (g_init_batch == batch_modes::NORMAL)
    {
        // in batch mode
        if (badfilename)
        {
            return -1;
        }
    }
    char cmd[80];
    std::strncpy(cmd, cmdstr, 30);
    cmd[29] = 0;

    if (*cmd)
    {
        std::strcat(cmd, "=");
    }
    std::string msg;
    if (badfilename)
    {
        msg = std::string {"Can't find "} + cmd + badfilename + ", please check " + to_string(mode);
    }
    if (g_first_init)
    {
        // & cmdfiles hasn't finished 1st try
        if (row == 1 && badfilename)
        {
            driver_set_for_text();
            driver_put_string(0, 0, 15, "Id found the following problems when parsing commands: ");
        }
        if (badfilename)
        {
            driver_put_string(row++, 0, 7, msg);
        }
        else if (row > 1)
        {
            driver_put_string(++row, 0, 15, "Press Escape to abort, any other key to continue");
            driver_move_cursor(row+1, 0);
            dopause(2);  // defer getakeynohelp until after parsing
        }
    }
    else if (badfilename)
    {
        stopmsg(msg);
    }
    return 0;
}

// Crude function to detect a floating point number. Intended for
// use with arbitrary precision.
static bool isabigfloat(char const *str)
{
    // [+|-]numbers][.]numbers[+|-][e|g]numbers
    bool result = true;
    char const *s = str;
    int numdot{};
    int nume{};
    int numsign{};
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
        else if (!std::isdigit(*s))
        {
            result = false;
            break;
        }
        s++;
    }
    if (numdot > 1 || numsign > 2 || nume > 1)
    {
        result = false;
    }
    return result;
}
