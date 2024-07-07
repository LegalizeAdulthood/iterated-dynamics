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
#include <functional>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#define DEFAULT_ASPECT_DRIFT 0.02F  // drift of < 2% is forced to 0%

static int get_max_curarg_len(char const *const floatvalstr[], int num_args);
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
static int  get_bf(bf_t bf, char const *curarg);
static bool isabigfloat(char const *str);

// variables defined by the command line/files processor
int g_stop_pass{};                                            // stop at this guessing pass early
int g_distance_estimator_x_dots{};                            // xdots to use for video independence
int g_distance_estimator_y_dots{};                            // ydots to use for video independence
int g_bf_digits{};                                            // digits to use (force) for g_bf_math
int g_show_dot{-1};                                           // color to show crawling graphics cursor
int g_size_dot{};                                             // size of dot crawling cursor
record_colors_mode g_record_colors{record_colors_mode::none}; // default PAR color-writing method
char g_auto_show_dot{};                                       // dark, medium, bright
bool g_start_show_orbit{};                                    // show orbits on at start of fractal
std::string g_read_filename;                                  // name of fractal input file
std::string g_temp_dir;                                       // name of temporary directory
std::string g_working_dir;                                    // name of directory for misc files
std::string g_organize_formulas_dir;                          // name of directory for orgfrm files
std::string g_gif_filename_mask;                              //
std::string g_save_filename{"fract001"};                      // save files using this name
bool g_potential_flag{};                                      // continuous potential enabled?
bool g_potential_16bit{};                                     // store 16 bit continuous potential values
bool g_dither_flag{};                                         // true if want to dither GIFs
bool g_ask_video{};                                           // flag for video prompting
bool g_float_flag{};                                          //
int g_biomorph{};                                             // flag for biomorph
int g_user_biomorph_value{};                                  //
int g_show_file{};                                            // zero if file display pending
bool g_random_seed_flag{};                                    //
int g_random_seed{};                                          // Random number seeding flag and value
int g_decomp[2]{};                                            // Decomposition coloring
long g_distance_estimator{};                                  //
int g_distance_estimator_width_factor{};                      //
bool g_overwrite_file{};                                      // true if file overwrite allowed
int g_sound_flag{};                                    // sound control bitfield... see sound.c for useage
int g_base_hertz{};                                    // sound=x/y/x hertz value
int g_cycle_limit{};                                   // color-rotator upper limit
int g_fill_color{};                                    // fillcolor: -1=normal
bool g_finite_attractor{};                             // finite attractor logic
display_3d_modes g_display_3d{display_3d_modes::NONE}; // 3D display flag: 0 = OFF
bool g_overlay_3d{};                                   // 3D overlay flag
bool g_check_cur_dir{};                                // flag to check current dir for files
batch_modes g_init_batch{batch_modes::NONE};           // 1 if batch run (no kbd)
int g_init_save_time{};                                // autosave minutes
DComplex g_init_orbit{};                               // initial orbitvalue
init_orbit_mode g_use_init_orbit{init_orbit_mode::normal};    // flag for initorbit
int g_init_mode{};                                            // initial video mode
int g_init_cycle_limit{};                                     // initial cycle limit
bool g_use_center_mag{};                                      // use center-mag corners
long g_bail_out{};                                            // user input bailout value
double g_inversion[3]{};                                      // radius, xcenter, ycenter
int g_color_cycle_range_lo{};                                 //
int g_color_cycle_range_hi{};                                 // cycling color range
std::vector<int> g_iteration_ranges;                          // iter->color ranges mapping
int g_iteration_ranges_len{};                                 // size of ranges array
BYTE g_map_clut[256][3];                                      // map= (default colors)
bool g_map_specified{};                                       // map= specified
BYTE *mapdacbox{};                                            // map= (default colors)
color_state g_color_state{color_state::DEFAULT};              // g_dac_box matches default (bios or map=)
bool g_colors_preloaded{};                                    // if g_dac_box preloaded for next mode select
bool g_read_color{true};                                      // flag for reading color from GIF
double g_math_tol[2]{.05, .05};                               // For math transition
bool g_targa_out{};                                           // 3D fullcolor flag
bool g_truecolor{};                                           // escape time truecolor flag
true_color_mode g_true_mode{true_color_mode::default_color};  // truecolor coloring scheme
std::string g_color_file;                                     // from last <l> <s> or colors=@filename
bool g_new_bifurcation_functions_loaded{};                    // if function loaded for new bifs
float g_screen_aspect{DEFAULT_ASPECT};                        // aspect ratio of the screen
float g_aspect_drift{DEFAULT_ASPECT_DRIFT}; // how much drift is allowed and still forced to g_screen_aspect

// true - reset viewwindows prior to a restore and
// do not display warnings when video mode changes during restore
bool g_fast_restore{};

// true: user has specified a directory for Orgform formula compilation files
bool g_organize_formulas_search{};

int g_orbit_save_flags{};                    // for IFS and LORENZ to output acrospin file
std::string g_orbit_save_name{"orbits.raw"}; //
int g_orbit_delay{};                         // clock ticks delating orbit release
int g_transparent_color_3d[2]{};             // transparency min/max values
long g_log_map_flag{};                       // Logarithmic palette flag: 0 = no
int g_log_map_fly_calculate{};               // calculate logmap on-the-fly
bool g_log_map_auto_calculate{};             // auto calculate logmap
bool g_bof_match_book_images{true};          // Flag to make inside=bof options not duplicate bof images
bool g_escape_exit{};                        // set to true to avoid the "are you sure?" screen
bool g_first_init{true};                     // first time into cmdfiles?

static int init_rseed{};        //
static bool s_init_corners{};   // corners set via corners= or center-mag=?
static bool s_init_params{};    // params set via params=?
static bool s_init_functions{}; // trig functions set via function=?
fractalspecificstuff *g_cur_fractal_specific{};

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
bool g_ifs_type{};                   // false=2d, true=3d

BYTE g_text_color[31] =
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

static_assert(std::size(g_text_color) == 31);

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
            if (std::fread(signature, 1, 6, initfile) != 6)
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
    g_bf_math = bf_math_type::NONE;                                   //
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

struct Command
{
    Command(char *curarg, cmd_file a_mode);
    cmdarg_flags bad_arg() const;

    char *arg;
    cmd_file mode{};
    char *value{};
    std::string variable;
    int valuelen{};                // length of value
    int numval{};                  // numeric value of arg
    char charval[16]{};            // first character of arg
    int yesnoval[16]{};            // 0 if 'n', 1 if 'y', -1 if not
    int totparms{};                // # of / delimited parms
    int intparms{};                // # of / delimited ints
    int floatparms{};              // # of / delimited floats
    int intval[64]{};              // pre-parsed integer parms
    double floatval[16]{};         // pre-parsed floating parms
    char const *floatvalstr[16]{}; // pointers to float vals
    cmdarg_flags status{cmdarg_flags::NONE};
};

Command::Command(char *curarg, cmd_file a_mode) :
    arg(curarg),
    mode(a_mode)
{
    lowerize_parameter(curarg);
    int j;
    value = std::strchr(&curarg[1], '=');
    if (value != nullptr)
    {
        j = (int) (value++ - curarg);
    }
    else
    {
        j = (int) std::strlen(curarg);
        value = curarg + j;
    }
    if (j > 20)
    {
        argerror(curarg); // keyword too long
        status = cmdarg_flags::ERROR;
        return;
    }
    variable = std::string(curarg, j);
    valuelen = (int) std::strlen(value); // note value's length
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
    while (*argptr) // count and pre-parse parms
    {
        long ll;
        bool last_arg{};
        char *argptr2 = std::strchr(argptr, '/');
        if (argptr2 == nullptr) // find next '/'
        {
            argptr2 = argptr + std::strlen(argptr);
            *argptr2 = '/';
            last_arg = true;
        }
        if (totparms == 0)
        {
            numval = NONNUMERIC;
        }
        if (totparms < 16)
        {
            charval[totparms] = *argptr; // first letter of value
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
        char tmpc{};
        if (std::sscanf(argptr, "%c%c", &next, &tmpc) > 0 // NULL entry
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
        else if (std::sscanf(argptr, "%ld%c", &ll, &tmpc) > 0 // got an integer
            && tmpc == '/')                                   // needs a long int, ll, here for lyapunov
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
        else if (double ftemp{}; std::sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0 // got a float
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
        else if (((int) std::strlen(argptr) > 513) // very long command
            || (totparms > 0 && floatval[totparms - 1] == FLT_MAX && totparms < 6) || isabigfloat(argptr))
        {
            ++floatparms;
            floatval[totparms] = FLT_MAX;
            floatvalstr[totparms] = argptr;
        }
        ++totparms;
        argptr = argptr2; // on to the next
        if (last_arg)
        {
            *argptr = 0;
        }
        else
        {
            ++argptr;
        }
    }
}
cmdarg_flags Command::bad_arg() const
{
    argerror(arg);
    return cmdarg_flags::ERROR;
}

struct CommandHandler
{
    std::string_view name;
    std::function<cmdarg_flags(const Command &)> handler;
};

// For deprecated command parameters that are still parsed but ignored.
static cmdarg_flags cmd_deprecated(const Command &)
{
    return cmdarg_flags::NONE;
}

// adapter parameter no longer used; check for bad argument anyway
static cmdarg_flags cmd_adapter(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (std::strcmp(cmd.value, "egamono") != 0 && std::strcmp(cmd.value, "hgc") != 0 &&
        std::strcmp(cmd.value, "ega") != 0 && std::strcmp(cmd.value, "cga") != 0 &&
        std::strcmp(cmd.value, "mcga") != 0 && std::strcmp(cmd.value, "vga") != 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// 8514 API no longer used; silently gobble any argument
static cmdarg_flags cmd_afi(const Command &)
{
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_batch(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_init_batch = cmd.yesnoval[0] == 0 ? batch_modes::NONE : batch_modes::NORMAL;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// biospalette no longer used, do validity checks, but gobble argument
static cmdarg_flags cmd_bios_palette(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// exitnoask deprecated; validate arg and gobble
static cmdarg_flags cmd_exit_no_ask(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_escape_exit = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// fpu deprecated; validate arg and gobble
static cmdarg_flags cmd_fpu(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "387")
    {
        return cmdarg_flags::NONE;
    }
    return cmd.bad_arg();
}

static cmdarg_flags cmd_make_doc(const Command &cmd)
{
    cmd_arg::s_print_document(*cmd.value ? cmd.value : "id.txt", makedoc_msg_func);
    cmd_arg::s_goodbye();
    return cmdarg_flags::GOODBYE;
}

static cmdarg_flags cmd_make_par(const Command &cmd)
{
    if (cmd.totparms < 1 || cmd.totparms > 2)
    {
        return cmd.bad_arg();
    }
    char *slash = std::strchr(cmd.value, '/');
    char *next = nullptr;
    if (slash != nullptr)
    {
        *slash = 0;
        next = slash + 1;
    }

    g_command_file = cmd.value;
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
            return cmd.bad_arg();
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

static cmdarg_flags cmd_max_history(const Command &cmd)
{
    if (cmd.numval == NONNUMERIC)
    {
        return cmd.bad_arg();
    }
    if (cmd.numval < 0)
    {
        return cmd.bad_arg();
    }
    g_max_image_history = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// textsafe no longer used, do validity checking, but gobble argument
static cmdarg_flags cmd_text_safe(const Command &cmd)
{
    if (g_first_init)
    {
        if (!(cmd.charval[0] == 'n'        // no
                || cmd.charval[0] == 'y'   // yes
                || cmd.charval[0] == 'b'   // bios
                || cmd.charval[0] == 's')) // save
        {
            return cmd.bad_arg();
        }
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// vesadetect no longer used, do validity checks, but gobble argument
static cmdarg_flags cmd_vesa_detect(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// these commands are allowed only at startup
// Keep this sorted by parameter name for binary search to work correctly.
static std::array<CommandHandler, 11> s_startup_commands{
    CommandHandler{"adapter", cmd_adapter},          //
    CommandHandler{"afi", cmd_afi},                  //
    CommandHandler{"batch", cmd_batch},              //
    CommandHandler{"biospalette", cmd_bios_palette}, //
    CommandHandler{"exitnoask", cmd_exit_no_ask},    //
    CommandHandler{"fpu", cmd_fpu},                  //
    CommandHandler{"makedoc", cmd_make_doc},         //
    CommandHandler{"makepar", cmd_make_par},         //
    CommandHandler{"maxhistory", cmd_max_history},   //
    CommandHandler{"textsafe", cmd_text_safe},       //
    CommandHandler{"vesadetect", cmd_vesa_detect},   //
};

// 3d=?/?/..
static cmdarg_flags cmd_3d(const Command &cmd)
{
    const std::string_view value{cmd.value};
    int yes_no = cmd.yesnoval[0];
    if (value == "overlay")
    {
        yes_no = 1;
        if (g_calc_status > calc_status_value::NO_FRACTAL) // if no image, treat same as 3D=yes
        {
            g_overlay_3d = true;
        }
    }
    else if (yes_no < 0)
    {
        return cmd.bad_arg();
    }
    g_display_3d = yes_no != 0 ? display_3d_modes::YES : display_3d_modes::NONE;
    initvars_3d();
    return g_display_3d != display_3d_modes::NONE ? cmdarg_flags::PARAM_3D | cmdarg_flags::YES_3D
                                                  : cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_3d_mode(const Command &cmd)
{
    int julibrot_mode = -1;
    for (int i = 0; i < 4; i++)
    {
        if (g_julibrot_3d_options[i] == std::string{cmd.value})
        {
            julibrot_mode = i;
            break;
        }
    }
    if (julibrot_mode < 0)
    {
        return cmd.bad_arg();
    }
    g_julibrot_3d_mode = static_cast<julibrot_3d_mode>(julibrot_mode);
    return cmdarg_flags::FRACTAL_PARAM;
}

// ambient=?
static cmdarg_flags cmd_ambient(const Command &cmd)
{
    if (cmd.numval < 0 || cmd.numval > 100)
    {
        return cmd.bad_arg();
    }
    g_ambient = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// askvideo=?
static cmdarg_flags cmd_ask_video(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_ask_video = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

// aspectdrift=?
static cmdarg_flags cmd_aspect_drift(const Command &cmd)
{
    if (cmd.floatparms != 1 || cmd.floatval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_aspect_drift = (float) cmd.floatval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

// attack=?
static cmdarg_flags cmd_attack(const Command &cmd)
{
    g_fm_attack = cmd.numval & 0x0F;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_attenuate(const Command &cmd)
{
    if (cmd.charval[0] == 'n')
    {
        g_hi_attenuation = 0;
    }
    else if (cmd.charval[0] == 'l')
    {
        g_hi_attenuation = 1;
    }
    else if (cmd.charval[0] == 'm')
    {
        g_hi_attenuation = 2;
    }
    else if (cmd.charval[0] == 'h')
    {
        g_hi_attenuation = 3;
    }
    else
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_auto_key(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "record")
    {
        g_slides = slides_mode::RECORD;
    }
    else if (value == "play")
    {
        g_slides = slides_mode::PLAY;
    }
    else
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_auto_key_name(const Command &cmd)
{
    if (merge_pathnames(g_auto_name, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return cmdarg_flags::NONE;
}

// background=?/?/?
static cmdarg_flags cmd_background(const Command &cmd)
{
    if (cmd.totparms != 3 || cmd.intparms != 3)
    {
        return cmd.bad_arg();
    }
    for (int i = 0; i < 3; i++)
    {
        if (cmd.intval[i] & ~0xff)
        {
            return cmd.bad_arg();
        }
    }
    g_background_color[0] = (BYTE) cmd.intval[0];
    g_background_color[1] = (BYTE) cmd.intval[1];
    g_background_color[2] = (BYTE) cmd.intval[2];
    return cmdarg_flags::PARAM_3D;
}

// bailout=?
static cmdarg_flags cmd_bail_out(const Command &cmd)
{
    if (cmd.floatval[0] < 1 || cmd.floatval[0] > 2100000000L)
    {
        return cmd.bad_arg();
    }
    g_bail_out = (long) cmd.floatval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

// bailoutest=?
static cmdarg_flags cmd_bail_out_test(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "mod")
    {
        g_bail_out_test = bailouts::Mod;
    }
    else if (value == "real")
    {
        g_bail_out_test = bailouts::Real;
    }
    else if (value == "imag")
    {
        g_bail_out_test = bailouts::Imag;
    }
    else if (value == "or")
    {
        g_bail_out_test = bailouts::Or;
    }
    else if (value == "and")
    {
        g_bail_out_test = bailouts::And;
    }
    else if (value == "manh")
    {
        g_bail_out_test = bailouts::Manh;
    }
    else if (value == "manr")
    {
        g_bail_out_test = bailouts::Manr;
    }
    else
    {
        return cmd.bad_arg();
    }
    set_bailout_formula(g_bail_out_test);
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_bf_digits(const Command &cmd)
{
    if (cmd.numval == NONNUMERIC || (cmd.numval < 0 || cmd.numval > 2000))
    {
        return cmd.bad_arg();
    }
    g_bf_digits = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM;
}

// biomorph=?
static cmdarg_flags cmd_biomorph(const Command &cmd)
{
    g_user_biomorph_value = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM;
}

// brief=?
static cmdarg_flags cmd_brief(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_brief = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

// bright=?
static cmdarg_flags cmd_bright(const Command &cmd)
{
    if (cmd.totparms != 2 || cmd.intparms != 2)
    {
        return cmd.bad_arg();
    }
    g_red_bright = cmd.intval[0];
    g_blue_bright = cmd.intval[1];
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// center-mag=?/?/?[/?/?/?]
static cmdarg_flags cmd_center_mag(const Command &cmd)
{
    if ((cmd.totparms != cmd.floatparms) || (cmd.totparms != 0 && cmd.totparms < 3) ||
        (cmd.totparms >= 3 && cmd.floatval[2] == 0.0))
    {
        return cmd.bad_arg();
    }
    if (g_fractal_type == fractal_type::CELLULAR)
    {
        return cmdarg_flags::FRACTAL_PARAM; // skip setting the corners
    }

    g_use_center_mag = true;
    if (cmd.totparms == 0)
    {
        return cmdarg_flags::NONE; // turns center-mag mode on
    }
    s_init_corners = true;
    // dec = get_max_curarg_len(floatvalstr, totparms);
    LDBL Magnification;
    std::sscanf(cmd.floatvalstr[2], "%Lf", &Magnification);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
    {
        return cmd.bad_arg(); // ie: Magnification is +-1.#INF
    }

    const int dec = getpower10(Magnification) + 4; // 4 digits of padding sounds good

    if ((dec <= DBL_DIG + 1 && g_debug_flag != debug_flags::force_arbitrary_precision_math) ||
        g_debug_flag == debug_flags::prevent_arbitrary_precision_math)
    {
        // rough estimate that double is OK
        double Xctr = cmd.floatval[0];
        double Yctr = cmd.floatval[1];
        double Xmagfactor = 1;
        double Rotation = 0;
        double Skew = 0;
        if (cmd.floatparms > 3)
        {
            Xmagfactor = cmd.floatval[3];
        }
        if (Xmagfactor == 0)
        {
            Xmagfactor = 1;
        }
        if (cmd.floatparms > 4)
        {
            Rotation = cmd.floatval[4];
        }
        if (cmd.floatparms > 5)
        {
            Skew = cmd.floatval[5];
        }
        // calculate bounds
        cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        return cmdarg_flags::FRACTAL_PARAM;
    }

    // use arbitrary precision
    int saved;
    s_init_corners = true;
    bf_math_type old_bf_math = g_bf_math;
    if (g_bf_math == bf_math_type::NONE || dec > g_decimals)
    {
        init_bf_dec(dec);
    }
    if (old_bf_math == bf_math_type::NONE)
    {
        for (int k = 0; k < MAX_PARAMS; k++)
        {
            floattobf(g_bf_parms[k], g_params[k]);
        }
    }
    g_use_center_mag = true;
    saved = save_stack();
    bf_t bXctr = alloc_stack(g_bf_length + 2);
    bf_t bYctr = alloc_stack(g_bf_length + 2);
    get_bf(bXctr, cmd.floatvalstr[0]);
    get_bf(bYctr, cmd.floatvalstr[1]);
    double Xmagfactor = 1;
    double Rotation = 0;
    double Skew = 0;
    if (cmd.floatparms > 3)
    {
        Xmagfactor = cmd.floatval[3];
    }
    if (Xmagfactor == 0)
    {
        Xmagfactor = 1;
    }
    if (cmd.floatparms > 4)
    {
        Rotation = cmd.floatval[4];
    }
    if (cmd.floatparms > 5)
    {
        Skew = cmd.floatval[5];
    }
    // calculate bounds
    cvtcornersbf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
    bfcornerstofloat();
    restore_stack(saved);
    return cmdarg_flags::FRACTAL_PARAM;
}

// coarse=?
static cmdarg_flags cmd_coarse(const Command &cmd)
{
    if (cmd.numval < 3 || cmd.numval > 2000)
    {
        return cmd.bad_arg();
    }
    g_preview_factor = cmd.numval;
    return cmdarg_flags::PARAM_3D;
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

// colors=, set current colors
static cmdarg_flags cmd_colors(const Command &cmd)
{
    if (parse_colors(cmd.value) == cmdarg_flags::ERROR)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_comment(const Command &cmd)
{
    parse_comments(cmd.value);
    return cmdarg_flags::NONE;
}

// converge=?
static cmdarg_flags cmd_converge(const Command &cmd)
{
    g_converge_x_adjust = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// corners=?/?/?/?
static cmdarg_flags cmd_corners(const Command &cmd)
{
    if (g_fractal_type == fractal_type::CELLULAR)
    {
        return cmdarg_flags::FRACTAL_PARAM; // skip setting the corners
    }

    if (cmd.floatparms != cmd.totparms || (cmd.totparms != 0 && cmd.totparms != 4 && cmd.totparms != 6))
    {
        return cmd.bad_arg();
    }
    
    g_use_center_mag = false;
    if (cmd.totparms == 0)
    {
        return cmdarg_flags::NONE; // turns corners mode on
    }

    s_init_corners = true;
    // good first approx, but dec could be too big
    int dec = get_max_curarg_len(cmd.floatvalstr, cmd.totparms) + 1;
    if ((dec > DBL_DIG + 1 || g_debug_flag == debug_flags::force_arbitrary_precision_math) &&
        g_debug_flag != debug_flags::prevent_arbitrary_precision_math)
    {
        bf_math_type old_bf_math = g_bf_math;
        if (g_bf_math == bf_math_type::NONE || dec > g_decimals)
        {
            init_bf_dec(dec);
        }
        if (old_bf_math == bf_math_type::NONE)
        {
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                floattobf(g_bf_parms[k], g_params[k]);
            }
        }

        // xx3rd = xxmin = floatval[0];
        get_bf(g_bf_x_min, cmd.floatvalstr[0]);
        get_bf(g_bf_x_3rd, cmd.floatvalstr[0]);

        // xxmax = floatval[1];
        get_bf(g_bf_x_max, cmd.floatvalstr[1]);

        // yy3rd = yymin = floatval[2];
        get_bf(g_bf_y_min, cmd.floatvalstr[2]);
        get_bf(g_bf_y_3rd, cmd.floatvalstr[2]);

        // yymax = floatval[3];
        get_bf(g_bf_y_max, cmd.floatvalstr[3]);

        if (cmd.totparms == 6)
        {
            // xx3rd = floatval[4];
            get_bf(g_bf_x_3rd, cmd.floatvalstr[4]);

            // yy3rd = floatval[5];
            get_bf(g_bf_y_3rd, cmd.floatvalstr[5]);
        }

        // now that all the corners have been read in, get a more
        // accurate value for dec and do it all again

        dec = getprecbf_mag();
        if (dec < 0)
        {
            return cmd.bad_arg(); // ie: Magnification is +-1.#INF
        }

        if (dec > g_decimals) // get corners again if need more precision
        {
            init_bf_dec(dec);

            // now get parameters and corners all over again at new
            // decimal setting
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                floattobf(g_bf_parms[k], g_params[k]);
            }

            // xx3rd = xxmin = floatval[0];
            get_bf(g_bf_x_min, cmd.floatvalstr[0]);
            get_bf(g_bf_x_3rd, cmd.floatvalstr[0]);

            // xxmax = floatval[1];
            get_bf(g_bf_x_max, cmd.floatvalstr[1]);

            // yy3rd = yymin = floatval[2];
            get_bf(g_bf_y_min, cmd.floatvalstr[2]);
            get_bf(g_bf_y_3rd, cmd.floatvalstr[2]);

            // yymax = floatval[3];
            get_bf(g_bf_y_max, cmd.floatvalstr[3]);

            if (cmd.totparms == 6)
            {
                // xx3rd = floatval[4];
                get_bf(g_bf_x_3rd, cmd.floatvalstr[4]);

                // yy3rd = floatval[5];
                get_bf(g_bf_y_3rd, cmd.floatvalstr[5]);
            }
        }
    }
    g_x_min = cmd.floatval[0];
    g_x_3rd = cmd.floatval[0];
    g_x_max = cmd.floatval[1];
    g_y_min = cmd.floatval[2];
    g_y_3rd = cmd.floatval[2];
    g_y_max = cmd.floatval[3];

    if (cmd.totparms == 6)
    {
        g_x_3rd = cmd.floatval[4];
        g_y_3rd = cmd.floatval[5];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// crop=?/?/?/?
static cmdarg_flags cmd_crop(const Command &cmd)
{
    if (cmd.totparms != 4 || cmd.intparms != 4 || cmd.intval[0] < 0 || cmd.intval[0] > 100 ||
        cmd.intval[1] < 0 || cmd.intval[1] > 100 || cmd.intval[2] < 0 || cmd.intval[2] > 100 ||
        cmd.intval[3] < 0 || cmd.intval[3] > 100)
    {
        return cmd.bad_arg();
    }
    g_red_crop_left = cmd.intval[0];
    g_red_crop_right = cmd.intval[1];
    g_blue_crop_left = cmd.intval[2];
    g_blue_crop_right = cmd.intval[3];
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// curdir=?
static cmdarg_flags cmd_cur_dir(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_check_cur_dir = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_cycle_limit(const Command &cmd)
{
    if (cmd.numval <= 1 || cmd.numval > 256)
    {
        return cmd.bad_arg();
    }
    g_init_cycle_limit = cmd.numval;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_cycle_range(const Command &cmd)
{
    int end{cmd.intval[1]};
    if (cmd.totparms < 2)
    {
        end = 255;
    }
    int begin{cmd.intval[0]};
    if (cmd.totparms < 1)
    {
        begin = 1;
    }
    if (cmd.totparms != cmd.intparms || begin < 0 || end > 255 || begin > end)
    {
        return cmd.bad_arg();
    }
    g_color_cycle_range_lo = begin;
    g_color_cycle_range_hi = end;
    return cmdarg_flags::NONE;
}

// debugflag=? or debug=?
static cmdarg_flags cmd_debug_flag(const Command &cmd)
{
    // internal use only
    g_debug_flag = static_cast<debug_flags>(cmd.numval);
    g_timer_flag = (g_debug_flag & debug_flags::benchmark_timer) != debug_flags::none; // separate timer flag
    g_debug_flag &= ~debug_flags::benchmark_timer;
    return cmdarg_flags::NONE;
}

// decay=?
static cmdarg_flags cmd_decay(const Command &cmd)
{
    g_fm_decay = cmd.numval & 0x0F;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_decomp(const Command &cmd)
{
    if (cmd.totparms != cmd.intparms || cmd.totparms < 1)
    {
        return cmd.bad_arg();
    }
    g_decomp[0] = cmd.intval[0];
    g_decomp[1] = 0;
    if (cmd.totparms > 1) // backward compatibility
    {
        g_decomp[1] = cmd.intval[1];
        g_bail_out = g_decomp[1];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_dist_est(const Command &cmd)
{
    if (cmd.totparms != cmd.intparms || cmd.totparms < 1)
    {
        return cmd.bad_arg();
    }
    g_user_distance_estimator_value = (long) cmd.floatval[0];
    g_distance_estimator_width_factor = 71;
    if (cmd.totparms > 1)
    {
        g_distance_estimator_width_factor = cmd.intval[1];
    }
    if (cmd.totparms > 3 && cmd.intval[2] > 0 && cmd.intval[3] > 0)
    {
        g_distance_estimator_x_dots = cmd.intval[2];
        g_distance_estimator_y_dots = cmd.intval[3];
    }
    else
    {
        g_distance_estimator_y_dots = 0;
        g_distance_estimator_x_dots = 0;
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_dither(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_dither_flag = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

// fastrestore=?
static cmdarg_flags cmd_fast_restore(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_fast_restore = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_filename(const Command &cmd)
{
    if (cmd.charval[0] == '.' && cmd.value[1] != SLASHC)
    {
        if (cmd.valuelen > 4)
        {
            return cmd.bad_arg();
        }
        g_gif_filename_mask = std::string{"*"} + cmd.value;
        return cmdarg_flags::NONE;
    }
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (cmd.mode == cmd_file::AT_AFTER_STARTUP &&
        g_display_3d == display_3d_modes::NONE) // can't do this in @ command
    {
        return cmd.bad_arg();
    }

    const int exist_dir = merge_pathnames(g_read_filename, cmd.value, cmd.mode);
    if (exist_dir == 0)
    {
        g_show_file = 0;
    }
    else if (exist_dir < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    else
    {
        g_browse_name = extract_filename(g_read_filename.c_str());
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// fillcolor=?
static cmdarg_flags cmd_fill_color(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "normal")
    {
        g_fill_color = -1;
    }
    else if (cmd.numval == NONNUMERIC)
    {
        return cmd.bad_arg();
    }
    else
    {
        g_fill_color = cmd.numval;
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// filltype=?
static cmdarg_flags cmd_fill_type(const Command &cmd)
{
    if (cmd.numval < +fill_type::SURFACE_GRID || cmd.numval > +fill_type::LIGHT_SOURCE_AFTER)
    {
        return cmd.bad_arg();
    }
    FILLTYPE = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_fin_attract(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_finite_attractor = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM;
}

// float=?
static cmdarg_flags cmd_float(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_user_float_flag = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// formulafile=?
static cmdarg_flags cmd_formula_file(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_pathnames(g_formula_filename, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// formulaname=?
static cmdarg_flags cmd_formula_name(const Command &cmd)
{
    if (cmd.valuelen > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_formula_name = cmd.value;
    return cmdarg_flags::FRACTAL_PARAM;
}

// fullcolor=?
static cmdarg_flags cmd_full_color(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_targa_out = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_function(const Command &cmd)
{
    int k = 0;
    const char *value = cmd.value;
    while (*value && k < 4)
    {
        if (set_trig_array(k++, value))
        {
            return cmd.bad_arg();
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

// deprecated, validate argument and swallow value
static cmdarg_flags cmd_gif87a(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

// haze=?
static cmdarg_flags cmd_haze(const Command &cmd)
{
    if (cmd.numval < 0 || cmd.numval > 100)
    {
        return cmd.bad_arg();
    }
    g_haze = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// hertz=?
static cmdarg_flags cmd_hertz(const Command &cmd)
{
    g_base_hertz = cmd.numval;
    return cmdarg_flags::NONE;
}

// ifs=? or ifs3d=?
static cmdarg_flags cmd_ifs(const Command &cmd)
{
    // ifs3d for old time's sake
    if (cmd.valuelen > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_ifs_name = cmd.value;
    reset_ifs_defn();
    return cmdarg_flags::FRACTAL_PARAM;
}

// ifsfile=??
static cmdarg_flags cmd_ifs_file(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (const int exist_dir = merge_pathnames(g_ifs_filename, cmd.value, cmd.mode); exist_dir == 0)
    {
        reset_ifs_defn();
    }
    else if (exist_dir < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// initorbit=?/?
static cmdarg_flags cmd_init_orbit(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "pixel")
    {
        g_use_init_orbit = init_orbit_mode::pixel;
    }
    else
    {
        if (cmd.totparms != 2 || cmd.floatparms != 2)
        {
            return cmd.bad_arg();
        }
        g_init_orbit.x = cmd.floatval[0];
        g_init_orbit.y = cmd.floatval[1];
        g_use_init_orbit = init_orbit_mode::value;
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_inside(const Command &cmd)
{
    struct Inside
    {
        const char *arg;
        int inside;
    };
    const Inside inside_names[] = {
        {"zmag", ZMAG},             //
        {"bof60", BOF60},           //
        {"bof61", BOF61},           //
        {"epsiloncross", EPSCROSS}, //
        {"startrail", STARTRAIL},   //
        {"period", PERIOD},         //
        {"fmod", FMODI},            //
        {"atan", ATANI},            //
        {"maxiter", -1}             //
    };
    for (const Inside &arg : inside_names)
    {
        if (std::strcmp(cmd.value, arg.arg) == 0)
        {
            g_inside_color = arg.inside;
            return cmdarg_flags::FRACTAL_PARAM;
        }
    }
    if (cmd.numval == NONNUMERIC)
    {
        return cmd.bad_arg();
    }

    g_inside_color = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM;
}

// interocular=?
static cmdarg_flags cmd_interocular(const Command &cmd)
{
    g_eye_separation = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// invert=?/?/?
static cmdarg_flags cmd_invert(const Command &cmd)
{
    if (cmd.totparms != cmd.floatparms || (cmd.totparms != 1 && cmd.totparms != 3))
    {
        return cmd.bad_arg();
    }
    g_inversion[0] = cmd.floatval[0];
    g_invert = (g_inversion[0] != 0.0) ? cmd.totparms : 0;
    if (cmd.totparms == 3)
    {
        g_inversion[1] = cmd.floatval[1];
        g_inversion[2] = cmd.floatval[2];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_is_mand(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_is_mandelbrot = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM;
}

// julibrot3d=?/?/?/?
static cmdarg_flags cmd_julibrot_3d(const Command &cmd)
{
    if (cmd.floatparms != cmd.totparms)
    {
        return cmd.bad_arg();
    }
    if (cmd.totparms > 0)
    {
        g_julibrot_z_dots = (int) cmd.floatval[0];
    }
    if (cmd.totparms > 1)
    {
        g_julibrot_origin_fp = (float) cmd.floatval[1];
    }
    if (cmd.totparms > 2)
    {
        g_julibrot_depth_fp = (float) cmd.floatval[2];
    }
    if (cmd.totparms > 3)
    {
        g_julibrot_height_fp = (float) cmd.floatval[3];
    }
    if (cmd.totparms > 4)
    {
        g_julibrot_width_fp = (float) cmd.floatval[4];
    }
    if (cmd.totparms > 5)
    {
        g_julibrot_dist_fp = (float) cmd.floatval[5];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// julibroteyes=?
static cmdarg_flags cmd_julibrot_eyes(const Command &cmd)
{
    if (cmd.floatparms != cmd.totparms || cmd.totparms != 1)
    {
        return cmd.bad_arg();
    }
    g_eyes_fp = (float) cmd.floatval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

// julibrotfromto=?/?/?/?
static cmdarg_flags cmd_julibrot_from_to(const Command &cmd)
{
    if (cmd.floatparms != cmd.totparms || cmd.totparms != 4)
    {
        return cmd.bad_arg();
    }
    g_julibrot_x_max = cmd.floatval[0];
    g_julibrot_x_min = cmd.floatval[1];
    g_julibrot_y_max = cmd.floatval[2];
    g_julibrot_y_min = cmd.floatval[3];
    return cmdarg_flags::FRACTAL_PARAM;
}

// latitude=?/?
static cmdarg_flags cmd_latitude(const Command &cmd)
{
    if (cmd.totparms != 2 || cmd.intparms != 2)
    {
        return cmd.bad_arg();
    }
    THETA1 = cmd.intval[0];
    THETA2 = cmd.intval[1];
    return cmdarg_flags::PARAM_3D;
}

// lfile=?
static cmdarg_flags cmd_l_file(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_pathnames(g_l_system_filename, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// lightname=?
static cmdarg_flags cmd_light_name(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (g_first_init || cmd.mode == cmd_file::AT_AFTER_STARTUP)
    {
        g_light_name = cmd.value;
    }
    return cmdarg_flags::NONE;
}

// lightsource=?/?/?
static cmdarg_flags cmd_light_source(const Command &cmd)
{
    if (cmd.totparms != 3 || cmd.intparms != 3)
    {
        return cmd.bad_arg();
    }
    XLIGHT = cmd.intval[0];
    YLIGHT = cmd.intval[1];
    ZLIGHT = cmd.intval[2];
    return cmdarg_flags::PARAM_3D;
}

// lname=?
static cmdarg_flags cmd_l_name(const Command &cmd)
{
    if (cmd.valuelen > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_l_system_name = cmd.value;
    return cmdarg_flags::FRACTAL_PARAM;
}

// logmap=?
static cmdarg_flags cmd_log_map(const Command &cmd)
{
    g_log_map_auto_calculate = false; // turn this off if loading a PAR
    if (cmd.charval[0] == 'y')
    {
        g_log_map_flag = 1; // palette is logarithmic
    }
    else if (cmd.charval[0] == 'n')
    {
        g_log_map_flag = 0;
    }
    else if (cmd.charval[0] == 'o')
    {
        g_log_map_flag = -1; // old log palette
    }
    else
    {
        g_log_map_flag = (long) cmd.floatval[0];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// logmode=?
static cmdarg_flags cmd_log_mode(const Command &cmd)
{
    g_log_map_fly_calculate = 0; // turn off if error
    g_log_map_auto_calculate = false;
    if (cmd.charval[0] == 'f')
    {
        g_log_map_fly_calculate = 1; // calculate on the fly
    }
    else if (cmd.charval[0] == 't')
    {
        g_log_map_fly_calculate = 2; // force use of LogTable
    }
    else if (cmd.charval[0] == 'a')
    {
        g_log_map_auto_calculate = true; // force auto calc of logmap
    }
    else
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// longitude=?/?
static cmdarg_flags cmd_longitude(const Command &cmd)
{
    if (cmd.totparms != 2 || cmd.intparms != 2)
    {
        return cmd.bad_arg();
    }
    PHI1 = cmd.intval[0];
    PHI2 = cmd.intval[1];
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_make_mig(const Command &cmd)
{
    if (cmd.totparms < 2)
    {
        return cmd.bad_arg();
    }
    const int xmult = cmd.intval[0];
    const int ymult = cmd.intval[1];
    make_mig(xmult, ymult);
    exit(0);
}

// map=, set default colors
static cmdarg_flags cmd_map(const Command &cmd)
{
    if (cmd.valuelen > FILE_MAX_PATH - 1)
    {
        return cmd.bad_arg();
    }

    const int exist_dir = merge_pathnames(g_map_name, cmd.value, cmd.mode);
    if (exist_dir > 0)
    {
        return cmdarg_flags::NONE; // got a directory
    }
    if (exist_dir < 0) // error
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
        return cmdarg_flags::NONE;
    }
    SetColorPaletteName(g_map_name.c_str());
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_math_tolerance(const Command &cmd)
{
    if (cmd.charval[0] == '/')
    {
        // leave math_tol[0] at the default value
    }
    else if (cmd.totparms >= 1)
    {
        g_math_tol[0] = cmd.floatval[0];
    }

    if (cmd.totparms >= 2)
    {
        g_math_tol[1] = cmd.floatval[1];
    }
    return cmdarg_flags::NONE;
}

// maxcolorres deprecated, validate value and gobble argument
// Change default color resolution
static cmdarg_flags cmd_max_color_res(const Command &cmd)
{
    if (cmd.numval == 1 || cmd.numval == 4 || cmd.numval == 8 || cmd.numval == 16 || cmd.numval == 24)
    {
        return cmdarg_flags::NONE;
    }
    return cmd.bad_arg();
}

static cmdarg_flags cmd_max_iter(const Command &cmd)
{
    if (cmd.floatval[0] < 2)
    {
        return cmd.bad_arg();
    }
    g_max_iterations = (long) cmd.floatval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_max_line_length(const Command &cmd)
{
    if (cmd.numval < MIN_MAX_LINE_LENGTH || cmd.numval > MAX_MAX_LINE_LENGTH)
    {
        return cmd.bad_arg();
    }
    g_max_line_length = cmd.numval;
    return cmdarg_flags::NONE;
}

// miim=?[/?[/?[/?]]]
static cmdarg_flags cmd_miim(const Command &cmd)
{
    if (cmd.totparms > 6)
    {
        return cmd.bad_arg();
    }
    if (cmd.charval[0] == 'b')
    {
        g_major_method = Major::breadth_first;
    }
    else if (cmd.charval[0] == 'd')
    {
        g_major_method = Major::depth_first;
    }
    else if (cmd.charval[0] == 'w')
    {
        g_major_method = Major::random_walk;
    }
#ifdef RANDOM_RUN
    else if (cmd.charval[0] == 'r')
    {
        major_method = Major::random_run;
    }
#endif
    else
    {
        return cmd.bad_arg();
    }

    if (cmd.charval[1] == 'l')
    {
        g_inverse_julia_minor_method = Minor::left_first;
    }
    else if (cmd.charval[1] == 'r')
    {
        g_inverse_julia_minor_method = Minor::right_first;
    }
    else
    {
        return cmd.bad_arg();
    }

    // keep this next part in for backwards compatibility with old PARs ???

    if (cmd.totparms > 2)
    {
        for (int k = 2; k < 6; ++k)
        {
            g_params[k - 2] = (k < cmd.totparms) ? cmd.floatval[k] : 0.0;
        }
    }

    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_min_stack(const Command &cmd)
{
    if (cmd.totparms != 1)
    {
        return cmd.bad_arg();
    }
    g_soi_min_stack = cmd.intval[0];
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_no_bof(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_bof_match_book_images = cmd.yesnoval[0] == 0;
    return cmdarg_flags::FRACTAL_PARAM;
}

// noninterlaced no longer used, validate value and gobble argument
static cmdarg_flags cmd_non_interlaced(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

// olddemmcolors=?
static cmdarg_flags cmd_old_demm_colors(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_old_demm_colors = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_orbit_delay(const Command &cmd)
{
    g_orbit_delay = cmd.numval;
    return cmdarg_flags::NONE;
}

// orbit corners=?/?/?/?
static cmdarg_flags cmd_orbit_corners(const Command &cmd)
{
    g_set_orbit_corners = false;
    if (cmd.floatparms != cmd.totparms || (cmd.totparms != 0 && cmd.totparms != 4 && cmd.totparms != 6))
    {
        return cmd.bad_arg();
    }
    g_orbit_corner_min_x = cmd.floatval[0];
    g_orbit_corner_3_x = cmd.floatval[0];
    g_orbit_corner_max_x = cmd.floatval[1];
    g_orbit_corner_min_y = cmd.floatval[2];
    g_orbit_corner_3_y = cmd.floatval[2];
    g_orbit_corner_max_y = cmd.floatval[3];

    if (cmd.totparms == 6)
    {
        g_orbit_corner_3_x = cmd.floatval[4];
        g_orbit_corner_3_y = cmd.floatval[5];
    }
    g_set_orbit_corners = true;
    g_keep_screen_coords = true;
    return cmdarg_flags::FRACTAL_PARAM;
}

// orbitdrawmode=?
// rectangle, line, or function (not yet tested or documented)
static cmdarg_flags cmd_orbit_draw_mode(const Command &cmd)
{
    if (cmd.charval[0] != 'l' && cmd.charval[0] != 'r' && cmd.charval[0] != 'f')
    {
        return cmd.bad_arg();
    }
    g_draw_mode = cmd.charval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_orbit_interval(const Command &cmd)
{
    g_orbit_interval = cmd.numval;
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

static cmdarg_flags cmd_orbit_name(const Command &cmd)
{
    if (check_orbit_name(cmd.value))
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// orbitsave=?
static cmdarg_flags cmd_orbit_save(const Command &cmd)
{
    if (cmd.charval[0] == 's')
    {
        g_orbit_save_flags |= osf_midi;
    }
    else if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_orbit_save_flags |= (cmd.yesnoval[0] ? osf_raw : 0);
    return cmdarg_flags::FRACTAL_PARAM;
}

// orbitsavename=?
static cmdarg_flags cmd_orbit_save_name(const Command &cmd)
{
    g_orbit_save_name = cmd.value;
    return cmdarg_flags::FRACTAL_PARAM;
}

// orgfrmdir=?
static cmdarg_flags cmd_org_frm_dir(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!isadirectory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_organize_formulas_search = true;
    g_organize_formulas_dir = cmd.value;
    fix_dirname(g_organize_formulas_dir);
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_outside(const Command &cmd)
{
    struct Outside
    {
        char const *arg;
        int outside;
    };
    static const Outside args[] = {
        {"iter", ITER}, //
        {"real", REAL}, //
        {"imag", IMAG}, //
        {"mult", MULT}, //
        {"summ", SUM},  //
        {"atan", ATAN}, //
        {"fmod", FMOD}, //
        {"tdis", TDIS}  //
    };
    for (const Outside &arg : args)
    {
        if (std::strcmp(cmd.value, arg.arg) == 0)
        {
            g_outside_color = arg.outside;
            return cmdarg_flags::FRACTAL_PARAM;
        }
    }
    if (cmd.numval == NONNUMERIC || (cmd.numval < TDIS || cmd.numval > 255))
    {
        return cmd.bad_arg();
    }
    g_outside_color = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_overwrite(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_overwrite_file = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_params(const Command &cmd)
{
    if (cmd.totparms != cmd.floatparms || cmd.totparms > MAX_PARAMS)
    {
        return cmd.bad_arg();
    }
    s_init_params = true;
    for (int k = 0; k < MAX_PARAMS; ++k)
    {
        g_params[k] = (k < cmd.totparms) ? cmd.floatval[k] : 0.0;
    }
    if (g_bf_math != bf_math_type::NONE)
    {
        for (int k = 0; k < MAX_PARAMS; k++)
        {
            floattobf(g_bf_parms[k], g_params[k]);
        }
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// parmfile=?
static cmdarg_flags cmd_parm_file(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_pathnames(g_command_file, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_passes(const Command &cmd)
{
    if (std::strchr("123gbtsdo", cmd.charval[0]) == nullptr)
    {
        return cmd.bad_arg();
    }
    g_user_std_calc_mode = cmd.charval[0];
    if (cmd.charval[0] == 'g')
    {
        g_stop_pass = ((int) cmd.value[1] - (int) '0');
        if (g_stop_pass < 0 || g_stop_pass > 6)
        {
            g_stop_pass = 0;
        }
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// periodicity=?
static cmdarg_flags cmd_periodicity(const Command &cmd)
{
    g_user_periodicity_value = 1;
    if (cmd.charval[0] == 'n' || cmd.numval == 0)
    {
        g_user_periodicity_value = 0;
    }
    else if (cmd.charval[0] == 'y')
    {
        g_user_periodicity_value = 1;
    }
    else if (cmd.charval[0] == 's') // 's' for 'show'
    {
        g_user_periodicity_value = -1;
    }
    else if (cmd.numval == NONNUMERIC)
    {
        return cmd.bad_arg();
    }
    else
    {
        g_user_periodicity_value = cmd.numval;
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

// perspective=?
static cmdarg_flags cmd_perspective(const Command &cmd)
{
    if (cmd.numval == NONNUMERIC)
    {
        return cmd.bad_arg();
    }
    ZVIEWER = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// pixelzoom no longer used, validate value and gobble argument
static cmdarg_flags cmd_pixel_zoom(const Command &cmd)
{
    if (cmd.numval >= 5)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_polyphony(const Command &cmd)
{
    if (cmd.numval > 9)
    {
        return cmd.bad_arg();
    }
    g_polyphony = std::abs(cmd.numval - 1);
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_potential(const Command &cmd)
{
    int k{};
    const char *value = cmd.value;
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
        else
        {
            ++value;
        }
    }
    g_potential_16bit = false;
    if (k < 99)
    {
        if (std::strcmp(value, "16bit") != 0)
        {
            return cmd.bad_arg();
        }
        g_potential_16bit = true;
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// preview=?
static cmdarg_flags cmd_preview(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_preview = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_proximity(const Command &cmd)
{
    g_close_proximity = cmd.floatval[0];
    return cmdarg_flags::FRACTAL_PARAM;
}

// radius=?
static cmdarg_flags cmd_radius(const Command &cmd)
{
    if (cmd.numval < 0)
    {
        return cmd.bad_arg();
    }
    RADIUS = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// randomize=?
static cmdarg_flags cmd_randomize(const Command &cmd)
{
    if (cmd.numval < 0 || cmd.numval > 7)
    {
        return cmd.bad_arg();
    }
    g_randomize_3d = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// ranges=?/?/.../?
static cmdarg_flags cmd_ranges(const Command &cmd)
{
    if (cmd.totparms != cmd.intparms)
    {
        return cmd.bad_arg();
    }

    int i = 0;
    int prev = i;
    int entries = prev;
    g_log_map_flag = 0; // ranges overrides logmap
    int tmpranges[128];
    while (i < cmd.totparms)
    {
        int k = cmd.intval[i++];
        if (k < 0) // striping
        {
            k = -k;
            if (k >= 16384 || i >= cmd.totparms)
            {
                return cmd.bad_arg();
            }
            tmpranges[entries++] = -1; // {-1,width,limit} for striping
            tmpranges[entries++] = k;
            k = cmd.intval[i++];
        }
        if (k < prev)
        {
            return cmd.bad_arg();
        }
        prev = k;
        tmpranges[entries++] = k;
    }
    if (prev == 0)
    {
        return cmd.bad_arg();
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

// ray=?
static cmdarg_flags cmd_ray(const Command &cmd)
{
    if (cmd.numval < 0 || cmd.numval > 6)
    {
        return cmd.bad_arg();
    }
    g_raytrace_format = static_cast<raytrace_formats>(cmd.numval);
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_record_colors(const Command &cmd)
{
    if (*cmd.value != 'y' && *cmd.value != 'c' && *cmd.value != 'a')
    {
        return cmd.bad_arg();
    }
    g_record_colors = static_cast<record_colors_mode>(*cmd.value);
    return cmdarg_flags::NONE;
}

// release=?
static cmdarg_flags cmd_release(const Command &cmd)
{
    return cmd.bad_arg();
}

static cmdarg_flags cmd_reset(const Command &cmd)
{
    // PAR release unknown unless specified
    if (cmd.numval < 0)
    {
        return cmd.bad_arg();
    }

    initvars_fractal();
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::RESET;
}

// rotation=?/?/?
static cmdarg_flags cmd_rotation(const Command &cmd)
{
    if (cmd.totparms != 3 || cmd.intparms != 3)
    {
        return cmd.bad_arg();
    }
    XROT = cmd.intval[0];
    YROT = cmd.intval[1];
    ZROT = cmd.intval[2];
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// roughness=?
static cmdarg_flags cmd_roughness(const Command &cmd)
{
    // "rough" is really scale z, but we add it here for convenience
    ROUGH = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// rseed=?
static cmdarg_flags cmd_r_seed(const Command &cmd)
{
    g_random_seed = cmd.numval;
    g_random_seed_flag = true;
    return cmdarg_flags::FRACTAL_PARAM;
}

static cmdarg_flags cmd_save_name(const Command &cmd)
{
    if (cmd.valuelen > FILE_MAX_PATH - 1)
    {
        return cmd.bad_arg();
    }
    if (g_first_init || cmd.mode == cmd_file::AT_AFTER_STARTUP)
    {
        if (merge_pathnames(g_save_filename, cmd.value, cmd.mode) < 0)
        {
            init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
        }
    }
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_save_time(const Command &cmd)
{
    g_init_save_time = cmd.numval;
    return cmdarg_flags::NONE;
}

// scalemap=?/?/?/?/?/?/?/?/?/?/?
static cmdarg_flags cmd_scale_map(const Command &cmd)
{
    if (cmd.totparms != cmd.intparms)
    {
        return cmd.bad_arg();
    }
    for (int counter = 0; counter <= 11; counter++)
    {
        if ((cmd.totparms > counter) && (cmd.intval[counter] > 0) && (cmd.intval[counter] < 13))
        {
            g_scale_map[counter] = cmd.intval[counter];
        }
    }
    return cmdarg_flags::NONE;
}

// scalexyz=?/?[/?]
static cmdarg_flags cmd_scale_xyz(const Command &cmd)
{
    if (cmd.totparms < 2 || cmd.intparms != cmd.totparms)
    {
        return cmd.bad_arg();
    }
    XSCALE = cmd.intval[0];
    YSCALE = cmd.intval[1];
    if (cmd.totparms > 2)
    {
        ROUGH = cmd.intval[2];
    }
    return cmdarg_flags::PARAM_3D;
}

// screencoords=?
static cmdarg_flags cmd_screen_coords(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_keep_screen_coords = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM;
}

// showbox=?
static cmdarg_flags cmd_show_box(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_show_box = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

// showdot=?[/?]
static cmdarg_flags cmd_show_dot(const Command &cmd)
{
    g_show_dot = 15;
    if (cmd.totparms > 0)
    {
        g_auto_show_dot = (char) 0;
        if (std::isalpha(cmd.charval[0]))
        {
            if (std::strchr("abdm", (int) cmd.charval[0]) != nullptr)
            {
                g_auto_show_dot = cmd.charval[0];
            }
            else
            {
                return cmd.bad_arg();
            }
        }
        else
        {
            g_show_dot = cmd.numval;
            if (g_show_dot < 0)
            {
                g_show_dot = -1;
            }
        }
        if (cmd.totparms > 1 && cmd.intparms > 0)
        {
            g_size_dot = cmd.intval[1];
        }
        if (g_size_dot < 0)
        {
            g_size_dot = 0;
        }
    }
    return cmdarg_flags::NONE;
}

// showorbit=yes|no
static cmdarg_flags cmd_show_orbit(const Command &cmd)
{
    g_start_show_orbit = cmd.yesnoval[0] != 0;
    return cmdarg_flags::NONE;
}

// smoothing=?
static cmdarg_flags cmd_smoothing(const Command &cmd)
{
    if (cmd.numval < 0)
    {
        return cmd.bad_arg();
    }
    LIGHTAVG = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// sound=?[/?/?/?/?]
static cmdarg_flags cmd_sound(const Command &cmd)
{
    if (cmd.totparms > 5)
    {
        return cmd.bad_arg();
    }
    g_sound_flag = SOUNDFLAG_OFF; // start with a clean slate, add bits as we go
    if (cmd.totparms == 1)
    {
        g_sound_flag = SOUNDFLAG_SPEAKER; // old command, default to PC speaker
    }

    // g_sound_flag is used as a bitfield... bit 0,1,2 used for whether sound
    // is modified by an orbits x,y,or z component. and also to turn it on
    // or off (0==off, 1==beep (or yes), 2==x, 3==y, 4==z),
    // Bit 3 is used for flagging the PC speaker sound,
    // Bit 4 for OPL3 FM sound card output,
    // Bit 5 will be for midi output (not yet),
    // Bit 6 for whether the tone is quantised to the nearest 'proper' note
    //  (according to the western, even tempered system anyway)
    if (cmd.charval[0] == 'n' || cmd.charval[0] == 'o')
    {
        g_sound_flag &= ~SOUNDFLAG_ORBITMASK;
    }
    else if ((std::strncmp(cmd.value, "ye", 2) == 0) || (cmd.charval[0] == 'b'))
    {
        g_sound_flag |= SOUNDFLAG_BEEP;
    }
    else if (cmd.charval[0] == 'x')
    {
        g_sound_flag |= SOUNDFLAG_X;
    }
    else if (cmd.charval[0] == 'y' && std::strncmp(cmd.value, "ye", 2) != 0)
    {
        g_sound_flag |= SOUNDFLAG_Y;
    }
    else if (cmd.charval[0] == 'z')
    {
        g_sound_flag |= SOUNDFLAG_Z;
    }
    else
    {
        return cmd.bad_arg();
    }
    if (cmd.totparms > 1)
    {
        g_sound_flag &= SOUNDFLAG_ORBITMASK; // reset options
        for (int i = 1; i < cmd.totparms; i++)
        {
            // this is for 2 or more options at the same time
            if (cmd.charval[i] == 'f')
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
            else if (cmd.charval[i] == 'p')
            {
                g_sound_flag |= SOUNDFLAG_SPEAKER;
            }
            else if (cmd.charval[i] == 'm')
            {
                g_sound_flag |= SOUNDFLAG_MIDI;
            }
            else if (cmd.charval[i] == 'q')
            {
                g_sound_flag |= SOUNDFLAG_QUANTIZED;
            }
            else
            {
                return cmd.bad_arg();
            }
        } // end for
    }     // end totparms > 1
    return cmdarg_flags::NONE;
}

// sphere=?
static cmdarg_flags cmd_sphere(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    SPHERE = cmd.yesnoval[0];
    return cmdarg_flags::PARAM_3D;
}

// srelease=?
static cmdarg_flags cmd_s_release(const Command &cmd)
{
    g_fm_release = cmd.numval & 0x0F;
    return cmdarg_flags::NONE;
}

// stereo=?
static cmdarg_flags cmd_stereo(const Command &cmd)
{
    if ((cmd.numval < 0) || (cmd.numval > 4))
    {
        return cmd.bad_arg();
    }
    g_glasses_type = cmd.numval;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// stereowidth=?, monitorwidth=?
static cmdarg_flags cmd_stereo_width(const Command &cmd)
{
    if (cmd.totparms != 1 || cmd.floatparms != 1)
    {
        return cmd.bad_arg();
    }
    g_auto_stereo_width = cmd.floatval[0];
    return cmdarg_flags::PARAM_3D;
}

// sustain=?
static cmdarg_flags cmd_sustain(const Command &cmd)
{
    g_fm_sustain = cmd.numval & 0x0F;
    return cmdarg_flags::NONE;
}

// symmetry=?
static cmdarg_flags cmd_symmetry(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "xaxis")
    {
        g_force_symmetry = symmetry_type::X_AXIS;
    }
    else if (value == "yaxis")
    {
        g_force_symmetry = symmetry_type::Y_AXIS;
    }
    else if (value == "xyaxis")
    {
        g_force_symmetry = symmetry_type::XY_AXIS;
    }
    else if (value == "origin")
    {
        g_force_symmetry = symmetry_type::ORIGIN;
    }
    else if (value == "pi")
    {
        g_force_symmetry = symmetry_type::PI_SYM;
    }
    else if (value == "none")
    {
        g_force_symmetry = symmetry_type::NONE;
    }
    else
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// targaoverlay=?
static cmdarg_flags cmd_targa_overlay(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_targa_overlay = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_temp_dir(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!isadirectory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_temp_dir = cmd.value;
    fix_dirname(g_temp_dir);
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_text_colors(const Command &cmd)
{
    char const *value = cmd.value;
    if (std::string_view(value) == "mono")
    {
        for (BYTE &elem : g_text_color)
        {
            elem = BLACK * 16 + WHITE;
        }
        g_text_color[28] = WHITE * 16 + BLACK;
        g_text_color[27] = WHITE * 16 + BLACK;
        g_text_color[20] = WHITE * 16 + BLACK;
        g_text_color[14] = WHITE * 16 + BLACK;
        g_text_color[13] = WHITE * 16 + BLACK;
        g_text_color[12] = WHITE * 16 + BLACK;
        g_text_color[6] = WHITE * 16 + BLACK;
        g_text_color[25] = BLACK * 16 + L_WHITE;
        g_text_color[24] = BLACK * 16 + L_WHITE;
        g_text_color[22] = BLACK * 16 + L_WHITE;
        g_text_color[17] = BLACK * 16 + L_WHITE;
        g_text_color[16] = BLACK * 16 + L_WHITE;
        g_text_color[11] = BLACK * 16 + L_WHITE;
        g_text_color[5] = BLACK * 16 + L_WHITE;
        g_text_color[2] = BLACK * 16 + L_WHITE;
        g_text_color[0] = BLACK * 16 + L_WHITE;
    }
    else
    {
        std::size_t k{};
        while (k < std::size(g_text_color))
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
                if (i == j || (i == 0 && j == 8)) // force contrast
                {
                    j = 15;
                }
                g_text_color[k] = (BYTE) (i * 16 + j);
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

    return cmdarg_flags::NONE;
}

// tplus no longer used, validate value and gobble argument
static cmdarg_flags cmd_tplus(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::NONE;
}

// transparent=?
static cmdarg_flags cmd_transparent(const Command &cmd)
{
    if (cmd.totparms != cmd.intparms || cmd.totparms < 1)
    {
        return cmd.bad_arg();
    }
    g_transparent_color_3d[0] = cmd.intval[0];
    g_transparent_color_3d[1] = cmd.intval[0];
    if (cmd.totparms > 1)
    {
        g_transparent_color_3d[1] = cmd.intval[1];
    }
    return cmdarg_flags::PARAM_3D;
}

// truecolor=?
static cmdarg_flags cmd_true_color(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_truecolor = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// truemode=?
static cmdarg_flags cmd_true_mode(const Command &cmd)
{
    g_true_mode = true_color_mode::default_color;
    if (cmd.charval[0] == 'd')
    {
        g_true_mode = true_color_mode::default_color;
    }
    if (cmd.charval[0] == 'i' || cmd.intval[0] == 1)
    {
        g_true_mode = true_color_mode::iterate;
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_type(const Command &cmd)
{
    std::string value{cmd.value, static_cast<std::string::size_type>(cmd.valuelen)};
    if (!value.empty() && value.back() == '*')
    {
        value.pop_back();
    }

    // kludge because type ifs3d has an asterisk in front
    if (value == "ifs3d")
    {
        value = "ifs";
    }
    int k;
    for (k = 0; g_fractal_specific[k].name != nullptr; k++)
    {
        if (value == g_fractal_specific[k].name)
        {
            break;
        }
    }
    if (g_fractal_specific[k].name == nullptr)
    {
        return cmd.bad_arg();
    }
    const fractal_type previous{g_fractal_type};
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
        set_fractal_default_functions(previous);
    }
    if (!s_init_params)
    {
        load_params(g_fractal_type);
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// usegrayscale=?
static cmdarg_flags cmd_use_gray_scale(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_gray_flag = cmd.yesnoval[0] != 0;
    return cmdarg_flags::PARAM_3D;
}

static cmdarg_flags cmd_video(const Command &cmd)
{
    const int k = check_vidmode_keyname(cmd.value);
    if (k == 0)
    {
        return cmd.bad_arg();
    }
    g_init_mode = -1;
    for (int i = 0; i < g_video_table_len; ++i)
    {
        if (g_video_table[i].keynum == k)
        {
            g_init_mode = i;
            break;
        }
    }
    if (g_init_mode == -1)
    {
        return cmd.bad_arg();
    }
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// viewwindows=?/?/?/?/?
static cmdarg_flags cmd_view_windows(const Command &cmd)
{
    if (cmd.totparms > 5 || cmd.floatparms - cmd.intparms > 2 || cmd.intparms > 4)
    {
        return cmd.bad_arg();
    }

    g_view_window = true;
    g_view_reduction = 4.2F; // reset default values
    g_final_aspect_ratio = g_screen_aspect;
    g_view_crop = true;
    g_view_x_dots = 0;
    g_view_y_dots = 0;

    if (cmd.totparms > 0 && cmd.floatval[0] > 0.001)
    {
        g_view_reduction = (float) cmd.floatval[0];
    }
    if (cmd.totparms > 1 && cmd.floatval[1] > 0.001)
    {
        g_final_aspect_ratio = (float) cmd.floatval[1];
    }
    if (cmd.totparms > 2 && cmd.yesnoval[2] == 0)
    {
        g_view_crop = cmd.yesnoval[2] != 0;
    }
    if (cmd.totparms > 3 && cmd.intval[3] > 0)
    {
        g_view_x_dots = cmd.intval[3];
    }
    if (cmd.totparms == 5 && cmd.intval[4] > 0)
    {
        g_view_y_dots = cmd.intval[4];
    }
    return cmdarg_flags::FRACTAL_PARAM;
}

// virtual=?
static cmdarg_flags cmd_virtual(const Command &cmd)
{
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_virtual_screens = cmd.yesnoval[0] != 0;
    return cmdarg_flags::FRACTAL_PARAM;
}

// volume=?
static cmdarg_flags cmd_volume(const Command &cmd)
{
    g_fm_volume = cmd.numval & 0x3F; // 63
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_warn(const Command &cmd)
{
    // keep this for backward compatibility
    if (cmd.yesnoval[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_overwrite_file = cmd.yesnoval[0] == 0;
    return cmdarg_flags::NONE;
}

// waterline=?
static cmdarg_flags cmd_water_line(const Command &cmd)
{
    if (cmd.numval < 0)
    {
        return cmd.bad_arg();
    }
    WATERLINE = cmd.numval;
    return cmdarg_flags::PARAM_3D;
}

// wavetype=?
static cmdarg_flags cmd_wave_type(const Command &cmd)
{
    g_fm_wavetype = cmd.numval & 0x0F;
    return cmdarg_flags::NONE;
}

static cmdarg_flags cmd_work_dir(const Command &cmd)
{
    if (cmd.valuelen > (FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!isadirectory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_working_dir = cmd.value;
    fix_dirname(g_working_dir);
    return cmdarg_flags::NONE;
}

// xyadjust=?
static cmdarg_flags cmd_xy_adjust(const Command &cmd)
{
    if (cmd.totparms != 2 || cmd.intparms != 2)
    {
        return cmd.bad_arg();
    }
    g_adjust_3d_x = cmd.intval[0];
    g_adjust_3d_y = cmd.intval[1];
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// xyshift=?/?
static cmdarg_flags cmd_xy_shift(const Command &cmd)
{
    if (cmd.totparms != 2 || cmd.intparms != 2)
    {
        return cmd.bad_arg();
    }
    XSHIFT = cmd.intval[0];
    YSHIFT = cmd.intval[1];
    return cmdarg_flags::FRACTAL_PARAM | cmdarg_flags::PARAM_3D;
}

// Keep this sorted by parameter name for binary search to work correctly.
static std::array<CommandHandler, 156> s_commands{
    CommandHandler{"3d", cmd_3d},                           //
    CommandHandler{"3dmode", cmd_3d_mode},                  //
    CommandHandler{"ambient", cmd_ambient},                 //
    CommandHandler{"askvideo", cmd_ask_video},              //
    CommandHandler{"aspectdrift", cmd_aspect_drift},        //
    CommandHandler{"attack", cmd_attack},                   //
    CommandHandler{"attenuate", cmd_attenuate},             //
    CommandHandler{"autokey", cmd_auto_key},                //
    CommandHandler{"autokeyname", cmd_auto_key_name},       //
    CommandHandler{"background", cmd_background},           //
    CommandHandler{"bailout", cmd_bail_out},                //
    CommandHandler{"bailoutest", cmd_bail_out_test},        //
    CommandHandler{"bfdigits", cmd_bf_digits},              //
    CommandHandler{"biomorph", cmd_biomorph},               //
    CommandHandler{"brief", cmd_brief},                     //
    CommandHandler{"bright", cmd_bright},                   //
    CommandHandler{"center-mag", cmd_center_mag},           //
    CommandHandler{"coarse", cmd_coarse},                   //
    CommandHandler{"colorps", cmd_deprecated},              // deprecated print parameters
    CommandHandler{"colors", cmd_colors},                   //
    CommandHandler{"comment", cmd_comment},                 //
    CommandHandler{"comport", cmd_deprecated},              // deprecated print parameters
    CommandHandler{"converge", cmd_converge},               //
    CommandHandler{"corners", cmd_corners},                 //
    CommandHandler{"crop", cmd_crop},                       //
    CommandHandler{"curdir", cmd_cur_dir},                  //
    CommandHandler{"cyclelimit", cmd_cycle_limit},          //
    CommandHandler{"cyclerange", cmd_cycle_range},          //
    CommandHandler{"debug", cmd_debug_flag},                //
    CommandHandler{"debugflag", cmd_debug_flag},            //
    CommandHandler{"decay", cmd_decay},                     //
    CommandHandler{"decomp", cmd_decomp},                   //
    CommandHandler{"distest", cmd_dist_est},                //
    CommandHandler{"dither", cmd_dither},                   //
    CommandHandler{"epsf", cmd_deprecated},                 // deprecated print parameters
    CommandHandler{"exitmode", cmd_deprecated},             //
    CommandHandler{"fastrestore", cmd_fast_restore},        //
    CommandHandler{"filename", cmd_filename},               //
    CommandHandler{"fillcolor", cmd_fill_color},            //
    CommandHandler{"filltype", cmd_fill_type},              //
    CommandHandler{"finattract", cmd_fin_attract},          //
    CommandHandler{"float", cmd_float},                     //
    CommandHandler{"formulafile", cmd_formula_file},        //
    CommandHandler{"formulaname", cmd_formula_name},        //
    CommandHandler{"fullcolor", cmd_full_color},            //
    CommandHandler{"function", cmd_function},               //
    CommandHandler{"gif87a", cmd_gif87a},                   //
    CommandHandler{"halftone", cmd_deprecated},             // deprecated print parameters
    CommandHandler{"haze", cmd_haze},                       //
    CommandHandler{"hertz", cmd_hertz},                     //
    CommandHandler{"ifs", cmd_ifs},                         //
    CommandHandler{"ifs3d", cmd_ifs},                       //
    CommandHandler{"ifsfile", cmd_ifs_file},                //
    CommandHandler{"initorbit", cmd_init_orbit},            //
    CommandHandler{"inside", cmd_inside},                   //
    CommandHandler{"interocular", cmd_interocular},         //
    CommandHandler{"invert", cmd_invert},                   //
    CommandHandler{"ismand", cmd_is_mand},                  //
    CommandHandler{"iterincr", cmd_deprecated},             //
    CommandHandler{"julibrot3d", cmd_julibrot_3d},          //
    CommandHandler{"julibroteyes", cmd_julibrot_eyes},      //
    CommandHandler{"julibrotfromto", cmd_julibrot_from_to}, //
    CommandHandler{"latitude", cmd_latitude},               //
    CommandHandler{"lfile", cmd_l_file},                    //
    CommandHandler{"lightname", cmd_light_name},            //
    CommandHandler{"lightsource", cmd_light_source},        //
    CommandHandler{"linefeed", cmd_deprecated},             // deprecated print parameters
    CommandHandler{"lname", cmd_l_name},                    //
    CommandHandler{"logmap", cmd_log_map},                  //
    CommandHandler{"logmode", cmd_log_mode},                //
    CommandHandler{"longitude", cmd_longitude},             //
    CommandHandler{"makemig", cmd_make_mig},                //
    CommandHandler{"map", cmd_map},                         //
    CommandHandler{"mathtolerance", cmd_math_tolerance},    //
    CommandHandler{"maxcolorres", cmd_max_color_res},       //
    CommandHandler{"maxiter", cmd_max_iter},                //
    CommandHandler{"maxlinelength", cmd_max_line_length},   //
    CommandHandler{"miim", cmd_miim},                       //
    CommandHandler{"minstack", cmd_min_stack},              //
    CommandHandler{"monitorwidth", cmd_stereo_width},       //
    CommandHandler{"nobof", cmd_no_bof},                    //
    CommandHandler{"noninterlaced", cmd_non_interlaced},    //
    CommandHandler{"olddemmcolors", cmd_old_demm_colors},   //
    CommandHandler{"orbitcorners", cmd_orbit_corners},      //
    CommandHandler{"orbitdelay", cmd_orbit_delay},          //
    CommandHandler{"orbitdrawmode", cmd_orbit_draw_mode},   //
    CommandHandler{"orbitinterval", cmd_orbit_interval},    //
    CommandHandler{"orbitname", cmd_orbit_name},            //
    CommandHandler{"orbitsave", cmd_orbit_save},            //
    CommandHandler{"orbitsavename", cmd_orbit_save_name},   //
    CommandHandler{"orgfrmdir", cmd_org_frm_dir},           //
    CommandHandler{"outside", cmd_outside},                 //
    CommandHandler{"overwrite", cmd_overwrite},             //
    CommandHandler{"params", cmd_params},                   //
    CommandHandler{"parmfile", cmd_parm_file},              //
    CommandHandler{"passes", cmd_passes},                   //
    CommandHandler{"periodicity", cmd_periodicity},         //
    CommandHandler{"perspective", cmd_perspective},         //
    CommandHandler{"pixelzoom", cmd_pixel_zoom},            //
    CommandHandler{"plotstyle", cmd_deprecated},            // deprecated print parameters
    CommandHandler{"polyphony", cmd_polyphony},             //
    CommandHandler{"potential", cmd_potential},             //
    CommandHandler{"preview", cmd_preview},                 //
    CommandHandler{"printer", cmd_deprecated},              // deprecated print parameters
    CommandHandler{"printfile", cmd_deprecated},            // deprecated print parameters
    CommandHandler{"proximity", cmd_proximity},             //
    CommandHandler{"radius", cmd_radius},                   //
    CommandHandler{"ramvideo", cmd_deprecated},             //
    CommandHandler{"randomize", cmd_randomize},             //
    CommandHandler{"ranges", cmd_ranges},                   //
    CommandHandler{"ray", cmd_ray},                         //
    CommandHandler{"recordcolors", cmd_record_colors},      //
    CommandHandler{"release", cmd_release},                 //
    CommandHandler{"reset", cmd_reset},                     //
    CommandHandler{"rleps", cmd_deprecated},                // deprecated print parameters
    CommandHandler{"rotation", cmd_rotation},               //
    CommandHandler{"roughness", cmd_roughness},             //
    CommandHandler{"rseed", cmd_r_seed},                    //
    CommandHandler{"savename", cmd_save_name},              //
    CommandHandler{"savetime", cmd_save_time},              //
    CommandHandler{"scalemap", cmd_scale_map},              //
    CommandHandler{"scalexyz", cmd_scale_xyz},              //
    CommandHandler{"screencoords", cmd_screen_coords},      //
    CommandHandler{"showbox", cmd_show_box},                //
    CommandHandler{"showdot", cmd_show_dot},                //
    CommandHandler{"showorbit", cmd_show_orbit},            //
    CommandHandler{"smoothing", cmd_smoothing},             //
    CommandHandler{"sound", cmd_sound},                     //
    CommandHandler{"sphere", cmd_sphere},                   //
    CommandHandler{"srelease", cmd_s_release},              //
    CommandHandler{"stereo", cmd_stereo},                   //
    CommandHandler{"stereowidth", cmd_stereo_width},        //
    CommandHandler{"sustain", cmd_sustain},                 //
    CommandHandler{"symmetry", cmd_symmetry},               //
    CommandHandler{"targa_overlay", cmd_targa_overlay},     //
    CommandHandler{"tempdir", cmd_temp_dir},                //
    CommandHandler{"textcolors", cmd_text_colors},          //
    CommandHandler{"title", cmd_deprecated},                // deprecated print parameters
    CommandHandler{"tplus", cmd_tplus},                     //
    CommandHandler{"translate", cmd_deprecated},            // deprecated print parameters
    CommandHandler{"transparent", cmd_transparent},         //
    CommandHandler{"truecolor", cmd_true_color},            //
    CommandHandler{"truemode", cmd_true_mode},              //
    CommandHandler{"tweaklzw", cmd_deprecated},             //
    CommandHandler{"type", cmd_type},                       //
    CommandHandler{"usegrayscale", cmd_use_gray_scale},     //
    CommandHandler{"video", cmd_video},                     //
    CommandHandler{"viewwindows", cmd_view_windows},        //
    CommandHandler{"virtual", cmd_virtual},                 //
    CommandHandler{"volume", cmd_volume},                   //
    CommandHandler{"warn", cmd_warn},                       //
    CommandHandler{"waterline", cmd_water_line},            //
    CommandHandler{"wavetype", cmd_wave_type},              //
    CommandHandler{"workdir", cmd_work_dir},                //
    CommandHandler{"xyadjust", cmd_xy_adjust},              //
    CommandHandler{"xyshift", cmd_xy_shift},                //
};

template <size_t N>
std::optional<cmdarg_flags> handle_command(const std::array<CommandHandler, N> &handlers, Command &cmd)
{
    if (auto it = std::lower_bound(handlers.begin(), handlers.end(), cmd.variable,
            [](const CommandHandler &handler, const std::string &name) { return handler.name < name; });
        it != handlers.end() && it->name == cmd.variable)
    {
        return it->handler(cmd);
    }
    return {};
}

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
    Command cmd{curarg, mode};
    if (cmd.status != cmdarg_flags::NONE)
    {
        return cmd.status;
    }

    if (mode != cmd_file::AT_AFTER_STARTUP || g_debug_flag == debug_flags::allow_init_commands_anytime)
    {
        if (const std::optional handled{handle_command(s_startup_commands, cmd)}; handled.has_value())
        {
            return handled.value();
        }
    }

    if (const std::optional handled{handle_command(s_commands, cmd)}; handled.has_value())
    {
        return handled.value();
    }

    return cmd.bad_arg();
}

static void argerror(char const *badarg)      // oops. couldn't decode this
{
    std::string spillover{badarg};
    if (spillover.length() > 70)
    {
        spillover = spillover.substr(0, 70);
    }
    std::string msg{"Oops. I couldn't understand the argument:\n  "};
    msg += spillover;

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
        FILLTYPE  = +fill_type::SURFACE_INTERPOLATED;
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
        FILLTYPE  = +fill_type::POINTS;
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
static int get_max_curarg_len(const char *const floatvalstr[], int num_args)
{
    int max_str = 0;
    for (int i = 0; i < num_args; i++)
    {
        int tmp = get_curarg_len(floatvalstr[i]);
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
