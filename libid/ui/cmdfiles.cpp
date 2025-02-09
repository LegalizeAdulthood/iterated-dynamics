// SPDX-License-Identifier: GPL-3.0-only
//
// Command-line / Command-File Parser Routines
//
#include "ui/cmdfiles_test.h"

#include "3d/line3d.h"
#include "3d/plot3d.h"
#include "engine/bailout_formula.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/convert_corners.h"
#include "engine/engine_timer.h"
#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "engine/get_prec_big_float.h"
#include "engine/id_data.h"
#include "engine/log_map.h"
#include "engine/soi.h"
#include "engine/sticky_orbits.h"
#include "engine/perturbation.h"
#include "fractals/check_orbit_name.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "fractals/lorenz.h"
#include "fractals/parser.h"
#include "helpcom.h"
#include "io/extract_filename.h"
#include "io/file_gets.h"
#include "io/find_path.h"
#include "io/fix_dirname.h"
#include "io/has_ext.h"
#include "io/is_directory.h"
#include "io/load_config.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "io/merge_path_names.h"
#include "io/special_dirs.h"
#include "math/biginit.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/version.h"
#include "ui/comments.h"
#include "ui/do_pause.h"
#include "ui/file_item.h"
#include "ui/framain2.h"
#include "ui/get_fract_type.h"
#include "ui/goodbye.h"
#include "ui/help_title.h"
#include "ui/history.h"
#include "ui/load_params.h"
#include "ui/lowerize_parameter.h"
#include "ui/make_batch_file.h"
#include "ui/make_mig.h"
#include "ui/rotate.h"
#include "ui/slideshw.h"
#include "ui/sound.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"
#include "ui/trig_fns.h"
#include "ui/video_mode.h"

#include <config/path_limits.h>
#include <config/string_lower.h>

#include <algorithm>
#include <array>
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

static int get_max_cur_arg_len(const char *const float_val_str[], int num_args);
static CmdArgFlags command_file(std::FILE *handle, CmdFile mode);
static int  next_command(
    char *cmd_buf,
    int max_len,
    std::FILE *handle,
    char *line_buf,
    int *line_offset,
    CmdFile mode);
static bool next_line(std::FILE *handle, char *line_buf, CmdFile mode);
static void arg_error(const char *bad_arg);
static void init_vars_run();
static void init_vars_restart();
static void init_vars_fractal();
static void init_vars3d();
static void reset_ifs_definition();
static int  get_bf(BigFloat bf, const char *cur_arg);
static bool is_a_big_float(const char *str);

// variables defined by the command line/files processor
int g_stop_pass{};                                        // stop at this guessing pass early
int g_distance_estimator_x_dots{};                        // x dots to use for video independence
int g_distance_estimator_y_dots{};                        // y dots to use for video independence
int g_bf_digits{};                                        // digits to use (force) for g_bf_math
int g_show_dot{-1};                                       // color to show crawling graphics cursor
int g_size_dot{};                                         // size of dot crawling cursor
RecordColorsMode g_record_colors{RecordColorsMode::NONE}; // default PAR color-writing method
char g_auto_show_dot{};                                   // dark, medium, bright
bool g_start_show_orbit{};                                // show orbits on at start of fractal
std::string g_read_filename;                              // name of fractal input file
std::string g_temp_dir;                                   // name of temporary directory
std::string g_working_dir;                                // name of directory for misc files
std::string g_organize_formulas_dir;                      // name of directory for orgfrm files
std::string g_gif_filename_mask;                          //
std::string g_save_filename{"fract001"};                  // save files using this name
bool g_potential_flag{};                                  // continuous potential enabled?
bool g_potential_16bit{};                                 // store 16 bit continuous potential values
bool g_dither_flag{};                                     // true if we want to dither GIFs
bool g_ask_video{};                                       // flag for video prompting
int g_biomorph{};                                         // flag for biomorph
int g_user_biomorph_value{};                              //
int g_show_file{};                                        // zero if file display pending
bool g_random_seed_flag{};                                //
int g_random_seed{};                                      // Random number seeding flag and value
int g_decomp[2]{};                                        // Decomposition coloring
long g_distance_estimator{};                              //
int g_distance_estimator_width_factor{};                  //
bool g_overwrite_file{};                                  // true if file overwrite allowed
int g_sound_flag{};                                       // sound control bitfield... see sound.c for useage
int g_base_hertz{};                                       // sound=x/y/x hertz value
int g_cycle_limit{};                                      // color-rotator upper limit
int g_fill_color{};                                       // fill color: -1=normal
bool g_finite_attractor{};                                // finite attractor logic
Display3DMode g_display_3d{Display3DMode::NONE};          // 3D display flag: 0 = OFF
bool g_overlay_3d{};                                      // 3D overlay flag
bool g_check_cur_dir{};                                   // flag to check current dir for files
BatchMode g_init_batch{BatchMode::NONE};                  // 1 if batch run (no kbd)
int g_init_save_time{};                                   // autosave minutes
DComplex g_init_orbit{};                                  // initial orbit value
InitOrbitMode g_use_init_orbit{InitOrbitMode::NORMAL};    // flag for init orbit
int g_init_mode{};                                        // initial video mode
int g_init_cycle_limit{};                                 // initial cycle limit
bool g_use_center_mag{};                                  // use center-mag corners
long g_bailout{};                                         // user input bailout value
double g_inversion[3]{};                                  // radius, x center, y center
int g_color_cycle_range_lo{};                             //
int g_color_cycle_range_hi{};                             // cycling color range
std::vector<int> g_iteration_ranges;                      // iter->color ranges mapping
int g_iteration_ranges_len{};                             // size of ranges array
Byte g_map_clut[256][3];                                  // map= (default colors)
bool g_map_specified{};                                   // map= specified
ColorState g_color_state{ColorState::DEFAULT};            // g_dac_box matches default (bios or map=)
bool g_colors_preloaded{};                                // if g_dac_box preloaded for next mode select
bool g_read_color{true};                                  // flag for reading color from GIF
double g_math_tol[2]{.05, .05};                           // For math transition
bool g_targa_out{};                                       // 3D full color flag
bool g_true_color{};                                      // escape time true color flag
TrueColorMode g_true_mode{TrueColorMode::DEFAULT_COLOR};  // true color coloring scheme
std::string g_color_file;                                 // from last <l> <s> or colors=@filename
bool g_new_bifurcation_functions_loaded{};                // if function loaded for new bifs
float g_screen_aspect{DEFAULT_ASPECT};                    // aspect ratio of the screen
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
bool g_bof_match_book_images{true};          // Flag to make inside=bof options not duplicate bof images
bool g_escape_exit{};                        // set to true to avoid the "are you sure?" screen
bool g_first_init{true};                     // first time into cmdfiles?
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
Byte g_text_color[31] = {
    BLUE * 16 + LT_WHITE,    // C_TITLE           title background
    BLUE * 16 + LT_GREEN,    // C_TITLE_DEV       development vsn foreground
    GREEN * 16 + YELLOW,     // C_HELP_HDG        help page title line
    WHITE * 16 + BLACK,      // C_HELP_BODY       help page body
    GREEN * 16 + GRAY,       // C_HELP_INSTR      help page instr at bottom
    WHITE * 16 + BLUE,       // C_HELP_LINK       help page links
    CYAN * 16 + BLUE,        // C_HELP_CURLINK    help page current link
    WHITE * 16 + GRAY,       // C_PROMPT_BKGRD    prompt/choice background
    WHITE * 16 + BLACK,      // C_PROMPT_TEXT     prompt/choice extra info
    BLUE * 16 + WHITE,       // C_PROMPT_LO       prompt/choice text
    BLUE * 16 + LT_WHITE,    // C_PROMPT_MED      prompt/choice hdg2/...
    BLUE * 16 + YELLOW,      // C_PROMPT_HI       prompt/choice hdg/cur/...
    GREEN * 16 + LT_WHITE,   // C_PROMPT_INPUT    fullscreen_prompt input
    CYAN * 16 + LT_WHITE,    // C_PROMPT_CHOOSE   fullscreen_prompt choice
    MAGENTA * 16 + LT_WHITE, // C_CHOICE_CURRENT  fullscreen_choice input
    BLACK * 16 + WHITE,      // C_CHOICE_SP_INSTR speed key bar & instr
    BLACK * 16 + LT_MAGENTA, // C_CHOICE_SP_KEYIN speed key value
    WHITE * 16 + BLUE,       // C_GENERAL_HI      tab, thinking, IFS
    WHITE * 16 + BLACK,      // C_GENERAL_MED
    WHITE * 16 + GRAY,       // C_GENERAL_LO
    BLACK * 16 + LT_WHITE,   // C_GENERAL_INPUT
    WHITE * 16 + BLACK,      // C_DVID_BKGRD      disk video
    BLACK * 16 + YELLOW,     // C_DVID_HI
    BLACK * 16 + LT_WHITE,   // C_DVID_LO
    RED * 16 + LT_WHITE,     // C_STOP_ERR        stop message, error
    GREEN * 16 + BLACK,      // C_STOP_INFO       stop message, info
    BLUE * 16 + WHITE,       // C_TITLE_LOW       bottom lines of title screen
    GREEN * 16 + BLACK,      // C_AUTHDIV1        title screen dividers
    GREEN * 16 + GRAY,       // C_AUTHDIV2        title screen dividers
    BLACK * 16 + LT_WHITE,   // C_PRIMARY         primary authors
    BLACK * 16 + WHITE       // C_CONTRIB         contributing authors
};
static_assert(std::size(g_text_color) == 31);

static int s_init_random_seed{}; //
static bool s_init_corners{};    // corners set via corners= or center-mag=?
static bool s_init_params{};     // params set via params=?
static bool s_init_functions{};  // trig functions set via function=?

// cmdfiles(argc,argv) process the command-line arguments
// it also processes the 'sstools.ini' file and any
// indirect files ('id @myfile')

// This probably ought to go somewhere else, but it's used here.
// getpower10(x) returns the magnitude of x.  This rounds
// a little so 9.95 rounds to 10, but we're using a binary base anyway,
// so there's nothing magic about changing to the next power of 10.
int get_power10(LDouble x)
{
    char string[11]; // space for "+x.xe-xxxx"

    std::snprintf(string, std::size(string), "%+.1Le", x);
    int p = std::atoi(string + 5);
    return p;
}

static void process_sstools_ini()
{
    const std::string sstools_ini{locate_config_file("sstools.ini")}; // look for SSTOOLS.INI
    if (sstools_ini.empty())
    {
        return;
    }

    if (std::FILE *init_file = std::fopen(sstools_ini.c_str(), "r"); init_file != nullptr)
    {
        command_file(init_file, CmdFile::SSTOOLS_INI); // process it
    }
}

static void process_simple_command(char *cur_arg)
{
    bool processed{};
    if (std::strchr(cur_arg, '=') == nullptr)
    {
        // not xxx=yyy, so check for gif
        std::string filename = cur_arg;
        if (has_ext(cur_arg) == nullptr)
        {
            filename += ".gif";
        }
        if (std::FILE *init_file = std::fopen(filename.c_str(), "rb"))
        {
            char signature[6];
            if (std::fread(signature, 1, 6, init_file) != 6)
            {
                throw std::system_error(errno, std::system_category(), "process_simple_command failed fread");
            }
            if (signature[0] == 'G'
                && signature[1] == 'I'
                && signature[2] == 'F'
                && signature[3] >= '8' && signature[3] <= '9'
                && signature[4] >= '0' && signature[4] <= '9')
            {
                g_read_filename = cur_arg;
                g_browse_name = extract_file_name(g_read_filename.c_str());
                g_show_file = 0;
                processed = true;
            }
            std::fclose(init_file);
        }
    }
    if (!processed)
    {
        cmd_arg(cur_arg, CmdFile::AT_CMD_LINE);           // process simple command
    }
}

static void process_file_set_name(const char *cur_arg, char *slash)
{
    *slash = 0;
    if (merge_path_names(g_command_file, &cur_arg[1], CmdFile::AT_CMD_LINE) < 0)
    {
        init_msg("", g_command_file.c_str(), CmdFile::AT_CMD_LINE);
    }
    g_command_name = &slash[1];
    std::FILE *init_file = nullptr;
    if (find_file_item(g_command_file, g_command_name.c_str(), &init_file, ItemType::PAR_SET) || init_file == nullptr)
    {
        arg_error(cur_arg);
    }
    else
    {
        command_file(init_file, CmdFile::AT_CMD_LINE_SET_NAME);
    }
}

static void process_file(char *cur_arg)
{
    std::FILE *init_file = std::fopen(&cur_arg[1], "r");
    if (init_file == nullptr)
    {
        arg_error(cur_arg);
    }
    command_file(init_file, CmdFile::AT_CMD_LINE);
}

int cmd_files(int argc, const char *const *argv)
{
    if (g_first_init)
    {
        init_vars_run();                 // once per run initialization
    }
    init_vars_restart();                  // <ins> key initialization
    init_vars_fractal();                  // image initialization

    process_sstools_ini();

    // cycle through args
    for (int i = 1; i < argc; i++)
    {
        char cur_arg[141];
        std::strcpy(cur_arg, argv[i]);
        if (cur_arg[0] == ';')             // start of comments?
        {
            break;
        }
        if (cur_arg[0] != '@')
        {
            process_simple_command(cur_arg);
        }
        // @filename/setname?
        else if (char *slash = std::strchr(cur_arg, '/'))
        {
            process_file_set_name(cur_arg, slash);
        }
        // @filename
        else
        {
            process_file(cur_arg);
        }
    }

    if (!g_first_init)
    {
        g_init_mode = -1; // don't set video when <ins> key used
        g_show_file = 1;  // nor startup image file
    }

    init_msg("", nullptr, CmdFile::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    if (g_debug_flag != DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)
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
CmdArgFlags load_commands(std::FILE *infile)
{
    init_param_flags(); // reset flags for type=
    const CmdArgFlags ret = command_file(infile, CmdFile::AT_AFTER_STARTUP);

    // PAR reads a file and sets color, don't read colors from GIF
    g_read_color = !g_colors_preloaded || g_show_file != 0;

    return ret;
}

static void init_vars_run()              // once per run init
{
    s_init_random_seed = (int)std::time(nullptr);
    init_comments();
    const char *p = getenv("TMP");
    if (p == nullptr)
    {
        p = getenv("TEMP");
    }
    if (p != nullptr)
    {
        if (is_a_directory(p))
        {
            g_temp_dir = p;
            fix_dir_name(g_temp_dir);
        }
    }
    else
    {
        g_temp_dir.clear();
    }
}

static void init_vars_restart() // <ins> key init
{
    g_record_colors = RecordColorsMode::AUTOMATIC;     // use mapfiles in PARs
    g_dither_flag = false;                             // no dithering
    g_ask_video = true;                                // turn on video-prompt flag
    g_overwrite_file = false;                          // don't overwrite
    g_sound_flag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; // sound is on to PC speaker
    g_init_batch = BatchMode::NONE;                    // not in batch mode
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
    g_debug_flag = DebugFlags::NONE;                   // debugging flag(s) are off
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
    reset_ifs_definition();                            //
    g_random_seed_flag = false;                        // not a fixed srand() seed
    g_random_seed = s_init_random_seed;                //
    g_read_filename = DOT_SLASH;                       // initially current directory
    g_show_file = 1;                                   //
    // next should perhaps be fractal re-init, not just <ins> ?
    g_init_cycle_limit = 55;                          // spin-DAC default speed limit
    g_map_set = false;                                // no map= name active
    g_map_specified = false;                          //
    g_major_method = Major::BREADTH_FIRST;            // default inverse julia methods
    g_inverse_julia_minor_method = Minor::LEFT_FIRST; // default inverse julia methods
    g_true_color = false;                             // truecolor output flag
    g_true_mode = TrueColorMode::DEFAULT_COLOR;       //
}

// init vars affecting calculation
static void init_vars_fractal()
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
    g_finite_attractor = false;                          // disable finite attractor logic
    set_fractal_type(FractalType::MANDEL);               // initial fractal type
    init_param_flags();                                  //
    g_bailout = 0;                                       // no user-entered bailout
    g_bof_match_book_images = true;                      // use normal bof initialization to make bof images
    g_use_init_orbit = InitOrbitMode::NORMAL;            //
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
    g_force_symmetry = SymmetryType::NOT_FORCED;                    //
    g_x_min = -2.5;                                                 //
    g_x_3rd = g_x_min;                                              //
    g_x_max = 1.5;                                                  // initial corner values
    g_y_min = -1.5;                                                 //
    g_y_3rd = g_y_min;                                              //
    g_y_max = 1.5;                                                  // initial corner values
    g_bf_math = BFMathType::NONE;                                   //
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
    g_color_state = ColorState::DEFAULT;                            //
    g_colors_preloaded = false;                                     //
    g_color_cycle_range_lo = 1;                                     //
    g_color_cycle_range_hi = 255;                                   // color cycling default range
    g_orbit_delay = 0;                                              // full speed orbits
    g_orbit_interval = 1;                                           // plot all orbits
    g_keep_screen_coords = false;                                   //
    g_draw_mode = 'r';                                              // passes=orbits draw mode
    g_set_orbit_corners = false;                                    //
    g_orbit_corner_min_x = g_cur_fractal_specific->x_min;           //
    g_orbit_corner_max_x = g_cur_fractal_specific->x_max;           //
    g_orbit_corner_3rd_x = g_cur_fractal_specific->x_min;           //
    g_orbit_corner_min_y = g_cur_fractal_specific->y_min;           //
    g_orbit_corner_max_y = g_cur_fractal_specific->y_max;           //
    g_orbit_corner_3rd_y = g_cur_fractal_specific->y_min;           //
    g_math_tol[0] = 0.05;                                           //
    g_math_tol[1] = 0.05;                                           //
    g_display_3d = Display3DMode::NONE;                             // 3D display is off
    g_overlay_3d = false;                                           // 3D overlay is off
    g_old_demm_colors = false;                                      //
    g_bailout_test = Bailout::MOD;                                  //
    set_bailout_formula(Bailout::MOD);                              //
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
    g_new_orbit_type = FractalType::JULIA;                          //
    g_julibrot_z_dots = 128;                                        //
    init_vars3d();                                                  //
    g_base_hertz = 440;                                             // basic hertz rate
    g_fm_volume = 63;                                               // full volume on soundcard o/p
    g_hi_attenuation = 0;                                           // no attenuation of hi notes
    g_fm_attack = 5;                                                // fast attack
    g_fm_decay = 10;                                                // long decay
    g_fm_sustain = 13;                                              // fairly high sustain level
    g_fm_release = 5;                                               // short release
    g_fm_wave_type = 0;                                             // sin wave
    g_polyphony = 0;                                                // no polyphony
    std::iota(&g_scale_map[0], &g_scale_map[11], 1);                // straight mapping of notes in octave
}

// init vars affecting 3d
static void init_vars3d()
{
    g_raytrace_format = RayTraceFormat::NONE;
    g_brief   = false;
    g_sphere = false;
    g_preview = false;
    g_show_box = false;
    g_converge_x_adjust = 0;
    g_converge_y_adjust = 0;
    g_eye_separation = 0;
    g_glasses_type = GlassesType::NONE;
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

static void reset_ifs_definition()
{
    g_ifs_definition.clear();
}

// mode = AT_CMD_LINE           command line @filename
//        SSTOOLS_INI           sstools.ini
//        AT_AFTER_STARTUP      <@> command after startup
//        AT_CMD_LINE_SET_NAME  command line @filename/setname
// note that cmdfile could be open as text OR as binary.
// binary is used in @ command processing for reasonable speed note/point
static CmdArgFlags command_file(std::FILE *handle, CmdFile mode)
{
    if (mode == CmdFile::AT_AFTER_STARTUP || mode == CmdFile::AT_CMD_LINE_SET_NAME)
    {
        int i;
        while ((i = getc(handle)) != '{' && i != EOF)
        {
        }
        clear_command_comments();
    }

    char cmd_buf[10000]{};
    char line_buf[513];
    line_buf[0] = 0;
    int line_offset{};
    CmdArgFlags change_flag{}; // &1 fractal stuff chgd, &2 3d stuff chgd
    while (next_command(cmd_buf, std::size(cmd_buf), handle, line_buf, &line_offset, mode) > 0)
    {
        if ((mode == CmdFile::AT_AFTER_STARTUP || mode == CmdFile::AT_CMD_LINE_SET_NAME) && std::strcmp(cmd_buf, "}") == 0)
        {
            break;
        }
        const CmdArgFlags i = cmd_arg(cmd_buf, mode);
        if (i == CmdArgFlags::BAD_ARG)
        {
            break;
        }
        change_flag |= i;
    }
    std::fclose(handle);

    if (bit_set(change_flag, CmdArgFlags::FRACTAL_PARAM))
    {
        backwards_legacy_v18();
        backwards_legacy_v19();
        backwards_legacy_v20();
    }
    return change_flag;
}

static int next_command(
    char *cmd_buf,
    int max_len,
    std::FILE *handle,
    char *line_buf,
    int *line_offset,
    CmdFile mode)
{
    int cmd_len{};
    char *line_ptr = line_buf + *line_offset;
    while (true)
    {
        while (*line_ptr <= ' ' || *line_ptr == ';')
        {
            if (cmd_len)                 // space or ; marks end of command
            {
                cmd_buf[cmd_len] = 0;
                *line_offset = (int)(line_ptr - line_buf);
                return cmd_len;
            }
            while (*line_ptr && *line_ptr <= ' ')
            {
                ++line_ptr;                  // skip spaces and tabs
            }
            if (*line_ptr == ';' || *line_ptr == 0)
            {
                if (*line_ptr == ';'
                    && (mode == CmdFile::AT_AFTER_STARTUP || mode == CmdFile::AT_CMD_LINE_SET_NAME)
                    && (g_command_comment[0].empty() || g_command_comment[1].empty()
                        || g_command_comment[2].empty() || g_command_comment[3].empty()))
                {
                    // save comment
                    ++line_ptr;
                    while (*line_ptr && (*line_ptr == ' ' || *line_ptr == '\t'))
                    {
                        ++line_ptr;
                    }
                    if (*line_ptr)
                    {
                        if ((int)std::strlen(line_ptr) >= MAX_COMMENT_LEN)
                        {
                            *(line_ptr+MAX_COMMENT_LEN-1) = 0;
                        }
                        for (std::string &elem : g_command_comment)
                        {
                            if (elem.empty())
                            {
                                elem = line_ptr;
                                break;
                            }
                        }
                    }
                }
                if (next_line(handle, line_buf, mode))
                {
                    return -1; // eof
                }
                line_ptr = line_buf; // start new line
            }
        }
        if (*line_ptr == '\\' && *(line_ptr+1) == 0)              // continuation onto next line?
        {
            if (next_line(handle, line_buf, mode))
            {
                arg_error(cmd_buf);           // missing continuation
                return -1;
            }
            line_ptr = line_buf;
            while (*line_ptr && *line_ptr <= ' ')
            {
                ++line_ptr;                  // skip white space @ start next line
            }
            continue;                      // loop to check end of line again
        }
        cmd_buf[cmd_len] = *(line_ptr++);    // copy character to command buffer
        if (++cmd_len >= max_len)         // command too long?
        {
            arg_error(cmd_buf);
            return -1;
        }
    }
}

static bool next_line(std::FILE *handle, char *line_buf, CmdFile mode)
{
    bool tools_section = true;
    while (file_gets(line_buf, 512, handle) >= 0)
    {
        if (mode == CmdFile::SSTOOLS_INI && line_buf[0] == '[')   // check for [id]
        {
            char tmp_buf[11];
            std::strncpy(tmp_buf, &line_buf[1], 4);
            tmp_buf[4] = 0;
            string_lower(tmp_buf);
            tools_section = std::strncmp(tmp_buf, "id]", 3) == 0;
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
    NON_NUMERIC = -32767
};

namespace cmd_files_test
{

static StopMsgFn s_stop_msg{static_cast<StopMsg *>(stop_msg)};
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
    Command(char *cur_arg, CmdFile a_mode);
    CmdArgFlags bad_arg() const;

    char *arg;
    CmdFile mode{};
    char *value{};
    std::string variable;
    int value_len{};                  // length of value
    int num_val{};                    // numeric value of arg
    char char_val[16]{};              // first character of arg
    int yes_no_val[16]{};             // 0 if 'n', 1 if 'y', -1 if not
    int total_params{};               // # of / delimited parms
    int num_int_params{};             // # of / delimited ints
    int num_float_params{};           // # of / delimited floats
    int int_vals[64]{};               // pre-parsed integer parms
    double float_vals[16]{};          // pre-parsed floating parms
    const char *float_val_strs[16]{}; // pointers to float vals
    CmdArgFlags status{CmdArgFlags::NONE};
};

Command::Command(char *cur_arg, CmdFile a_mode) :
    arg(cur_arg),
    mode(a_mode),
    value(std::strchr(&cur_arg[1], '='))
{
    lowerize_parameter(cur_arg);
    int j;

    if (value != nullptr)
    {
        j = (int) (value++ - cur_arg);
    }
    else
    {
        j = (int) std::strlen(cur_arg);
        value = cur_arg + j;
    }
    if (j > 20)
    {
        arg_error(cur_arg); // keyword too long
        status = CmdArgFlags::BAD_ARG;
        return;
    }
    variable = std::string(cur_arg, j);
    value_len = (int) std::strlen(value); // note value's length
    char_val[0] = value[0];               // first letter of value
    yes_no_val[0] = -1;                    // note yes|no value
    if (char_val[0] == 'n')
    {
        yes_no_val[0] = 0;
    }
    if (char_val[0] == 'y')
    {
        yes_no_val[0] = 1;
    }

    char *arg_ptr = value;
    num_float_params = 0;
    num_int_params = 0;
    total_params = 0;
    num_val = 0;
    while (*arg_ptr) // count and pre-parse parms
    {
        long ll;
        bool last_arg{};
        char *arg_ptr2 = std::strchr(arg_ptr, '/');
        if (arg_ptr2 == nullptr) // find next '/'
        {
            arg_ptr2 = arg_ptr + std::strlen(arg_ptr);
            *arg_ptr2 = '/';
            last_arg = true;
        }
        if (total_params == 0)
        {
            num_val = NON_NUMERIC;
        }
        if (total_params < 16)
        {
            char_val[total_params] = *arg_ptr; // first letter of value
            if (char_val[total_params] == 'n')
            {
                yes_no_val[total_params] = 0;
            }
            if (char_val[total_params] == 'y')
            {
                yes_no_val[total_params] = 1;
            }
        }
        char next{};
        char tmp_c{};
        if (std::sscanf(arg_ptr, "%c%c", &next, &tmp_c) > 0 // NULL entry
            && (next == '/' || next == '=') && tmp_c == '/')
        {
            j = 0;
            ++num_float_params;
            ++num_int_params;
            if (total_params < 16)
            {
                float_vals[total_params] = j;
                float_val_strs[total_params] = "0";
            }
            if (total_params < 64)
            {
                int_vals[total_params] = j;
            }
            if (total_params == 0)
            {
                num_val = j;
            }
        }
        else if (std::sscanf(arg_ptr, "%ld%c", &ll, &tmp_c) > 0 // got an integer
            && tmp_c == '/')                                   // needs a long int, ll, here for lyapunov
        {
            ++num_float_params;
            ++num_int_params;
            if (total_params < 16)
            {
                float_vals[total_params] = ll;
                float_val_strs[total_params] = arg_ptr;
            }
            if (total_params < 64)
            {
                int_vals[total_params] = (int) ll;
            }
            if (total_params == 0)
            {
                num_val = (int) ll;
            }
        }
        else if (double f_temp{}; std::sscanf(arg_ptr, "%lg%c", &f_temp, &tmp_c) > 0 // got a float
                 && tmp_c == '/')
        {
            ++num_float_params;
            if (total_params < 16)
            {
                float_vals[total_params] = f_temp;
                float_val_strs[total_params] = arg_ptr;
            }
        }
        // using arbitrary precision and above failed
        else if (((int) std::strlen(arg_ptr) > 513) // very long command
            || (total_params > 0 && float_vals[total_params - 1] == FLT_MAX && total_params < 6) || is_a_big_float(arg_ptr))
        {
            ++num_float_params;
            float_vals[total_params] = FLT_MAX;
            float_val_strs[total_params] = arg_ptr;
        }
        ++total_params;
        arg_ptr = arg_ptr2; // on to the next
        if (last_arg)
        {
            *arg_ptr = 0;
        }
        else
        {
            ++arg_ptr;
        }
    }
}
CmdArgFlags Command::bad_arg() const
{
    arg_error(arg);
    return CmdArgFlags::BAD_ARG;
}

struct CommandHandler
{
    std::string_view name;
    std::function<CmdArgFlags(const Command &)> handler;
};

// For deprecated command parameters that are still parsed but ignored.
static CmdArgFlags cmd_deprecated(const Command &/*cmd*/)
{
    return CmdArgFlags::NONE;
}

// adapter parameter no longer used; check for bad argument anyway
static CmdArgFlags cmd_adapter(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value != "egamono" && value != "hgc" //
        && value != "ega" && value != "cga"  //
        && value != "mcga" && value != "vga")
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// 8514 API no longer used; silently gobble any argument
static CmdArgFlags cmd_afi(const Command &/*cmd*/)
{
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_batch(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_init_batch = cmd.yes_no_val[0] == 0 ? BatchMode::NONE : BatchMode::NORMAL;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// biospalette no longer used, do validity checks, but gobble argument
static CmdArgFlags cmd_bios_palette(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// exitnoask deprecated; validate arg and gobble
static CmdArgFlags cmd_exit_no_ask(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_escape_exit = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// fpu deprecated; validate arg and gobble
static CmdArgFlags cmd_fpu(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "387")
    {
        return CmdArgFlags::NONE;
    }
    return cmd.bad_arg();
}

static CmdArgFlags cmd_make_doc(const Command &cmd)
{
    enum
    {
        BOX_ROW = 6,
        BOX_COL = 11,
        BOX_WIDTH = 57,
        BOX_DEPTH = 12,
    };
    help_title();
    driver_set_attr(1, 0, C_DVID_BKGRD, 24 * 80); // init rest to background
                                                  // init box
    for (int i = 0; i < BOX_DEPTH; ++i)
    {
        driver_set_attr(BOX_ROW + i, BOX_COL, C_DVID_LO, BOX_WIDTH);
    }
    driver_put_string(BOX_ROW + 2, BOX_COL + 4, C_DVID_HI, "Creating Help Document");
    std::string file{*cmd.value ? cmd.value : "id.txt"};
    driver_put_string(BOX_ROW + 4, BOX_COL + 4, C_DVID_LO, "Save name: " + file);
    driver_put_string(BOX_ROW + 6, BOX_COL + 4, C_DVID_LO, "Status:");
    cmd_files_test::s_print_document(file.c_str(), make_doc_msg_func);
    cmd_files_test::s_goodbye();
    return CmdArgFlags::GOODBYE;
}

static CmdArgFlags cmd_make_par(const Command &cmd)
{
    if (cmd.total_params < 1 || cmd.total_params > 2)
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
    if (g_read_filename == DOT_SLASH)
    {
        g_read_filename.clear();
    }
    if (next == nullptr)
    {
        if (!g_read_filename.empty())
        {
            g_command_name = extract_file_name(g_read_filename.c_str());
        }
        else if (!g_map_name.empty())
        {
            g_command_name = extract_file_name(g_map_name.c_str());
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
            cmd_files_test::s_goodbye();
            return CmdArgFlags::GOODBYE;
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
    calc_frac_init();
    make_batch_file();
    cmd_files_test::s_goodbye();
    return CmdArgFlags::GOODBYE;
}

static CmdArgFlags cmd_max_history(const Command &cmd)
{
    if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }
    g_max_image_history = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// textsafe no longer used, do validity checking, but gobble argument
static CmdArgFlags cmd_text_safe(const Command &cmd)
{
    if (g_first_init)
    {
        // no, yes, bios, save
        if (std::string_view{"nybs"}.find(cmd.char_val[0]) == std::string_view::npos)
        {
            return cmd.bad_arg();
        }
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// vesadetect no longer used, do validity checks, but gobble argument
static CmdArgFlags cmd_vesa_detect(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
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
static CmdArgFlags cmd_3d(const Command &cmd)
{
    const std::string_view value{cmd.value};
    int yes_no = cmd.yes_no_val[0];
    if (value == "overlay")
    {
        yes_no = 1;
        if (g_calc_status > CalcStatus::NO_FRACTAL) // if no image, treat same as 3D=yes
        {
            g_overlay_3d = true;
        }
    }
    else if (yes_no < 0)
    {
        return cmd.bad_arg();
    }
    g_display_3d = yes_no != 0 ? Display3DMode::YES : Display3DMode::NONE;
    init_vars3d();
    return g_display_3d != Display3DMode::NONE ? CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D
                                                  : CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_3d_mode(const Command &cmd)
{
    int julibrot_mode = -1;
    for (int i = 0; i < 4; i++)
    {
        if (g_julibrot_3d_options[i] == std::string_view{cmd.value})
        {
            julibrot_mode = i;
            break;
        }
    }
    if (julibrot_mode < 0)
    {
        return cmd.bad_arg();
    }
    g_julibrot_3d_mode = static_cast<Julibrot3DMode>(julibrot_mode);
    return CmdArgFlags::FRACTAL_PARAM;
}

// ambient=?
static CmdArgFlags cmd_ambient(const Command &cmd)
{
    if (cmd.num_val < 0 || cmd.num_val > 100)
    {
        return cmd.bad_arg();
    }
    g_ambient = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// askvideo=?
static CmdArgFlags cmd_ask_video(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_ask_video = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

// aspectdrift=?
static CmdArgFlags cmd_aspect_drift(const Command &cmd)
{
    if (cmd.num_float_params != 1 || cmd.float_vals[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_aspect_drift = (float) cmd.float_vals[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

// attack=?
static CmdArgFlags cmd_attack(const Command &cmd)
{
    g_fm_attack = cmd.num_val & 0x0F;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_attenuate(const Command &cmd)
{
    if (cmd.char_val[0] == 'n')
    {
        g_hi_attenuation = 0;
    }
    else if (cmd.char_val[0] == 'l')
    {
        g_hi_attenuation = 1;
    }
    else if (cmd.char_val[0] == 'm')
    {
        g_hi_attenuation = 2;
    }
    else if (cmd.char_val[0] == 'h')
    {
        g_hi_attenuation = 3;
    }
    else
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_auto_key(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "record")
    {
        g_slides = SlidesMode::RECORD;
    }
    else if (value == "play")
    {
        g_slides = SlidesMode::PLAY;
    }
    else
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_auto_key_name(const Command &cmd)
{
    if (merge_path_names(g_auto_name, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return CmdArgFlags::NONE;
}

// background=?/?/?
static CmdArgFlags cmd_background(const Command &cmd)
{
    if (cmd.total_params != 3 || cmd.num_int_params != 3)
    {
        return cmd.bad_arg();
    }
    for (int i = 0; i < 3; i++)
    {
        if (cmd.int_vals[i] & ~0xff)
        {
            return cmd.bad_arg();
        }
    }
    g_background_color[0] = (Byte) cmd.int_vals[0];
    g_background_color[1] = (Byte) cmd.int_vals[1];
    g_background_color[2] = (Byte) cmd.int_vals[2];
    return CmdArgFlags::PARAM_3D;
}

// bailout=?
static CmdArgFlags cmd_bailout(const Command &cmd)
{
    if (cmd.float_vals[0] < 1 || cmd.float_vals[0] > 2100000000L)
    {
        return cmd.bad_arg();
    }
    g_bailout = (long) cmd.float_vals[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

// bailoutest=?
static CmdArgFlags cmd_bailout_test(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "mod")
    {
        g_bailout_test = Bailout::MOD;
    }
    else if (value == "real")
    {
        g_bailout_test = Bailout::REAL;
    }
    else if (value == "imag")
    {
        g_bailout_test = Bailout::IMAG;
    }
    else if (value == "or")
    {
        g_bailout_test = Bailout::OR;
    }
    else if (value == "and")
    {
        g_bailout_test = Bailout::AND;
    }
    else if (value == "manh")
    {
        g_bailout_test = Bailout::MANH;
    }
    else if (value == "manr")
    {
        g_bailout_test = Bailout::MANR;
    }
    else
    {
        return cmd.bad_arg();
    }
    set_bailout_formula(g_bailout_test);
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_bf_digits(const Command &cmd)
{
    if (cmd.num_val == NON_NUMERIC || (cmd.num_val < 0 || cmd.num_val > 2000))
    {
        return cmd.bad_arg();
    }
    g_bf_digits = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM;
}

// biomorph=?
static CmdArgFlags cmd_biomorph(const Command &cmd)
{
    g_user_biomorph_value = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM;
}

// brief=?
static CmdArgFlags cmd_brief(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_brief = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

// bright=?
static CmdArgFlags cmd_bright(const Command &cmd)
{
    if (cmd.total_params != 2 || cmd.num_int_params != 2)
    {
        return cmd.bad_arg();
    }
    g_red_bright = cmd.int_vals[0];
    g_blue_bright = cmd.int_vals[1];
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// center-mag=?/?/?[/?/?/?]
static CmdArgFlags cmd_center_mag(const Command &cmd)
{
    if ((cmd.total_params != cmd.num_float_params) || (cmd.total_params != 0 && cmd.total_params < 3) ||
        (cmd.total_params >= 3 && cmd.float_vals[2] == 0.0))
    {
        return cmd.bad_arg();
    }
    if (g_fractal_type == FractalType::CELLULAR)
    {
        return CmdArgFlags::FRACTAL_PARAM; // skip setting the corners
    }

    g_use_center_mag = true;
    if (cmd.total_params == 0)
    {
        return CmdArgFlags::NONE; // turns center-mag mode on
    }
    s_init_corners = true;
    // dec = get_max_curarg_len(floatvalstr, totparms);
    LDouble magnification;
    std::sscanf(cmd.float_val_strs[2], "%Lf", &magnification);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (magnification > LDBL_MAX || magnification < -LDBL_MAX)
    {
        return cmd.bad_arg(); // ie: magnification is +-1.#INF
    }

    const int dec = get_power10(magnification) + 4; // 4 digits of padding sounds good

    if ((dec <= DBL_DIG + 1 && g_debug_flag != DebugFlags::FORCE_ARBITRARY_PRECISION_MATH) ||
        g_debug_flag == DebugFlags::PREVENT_ARBITRARY_PRECISION_MATH)
    {
        // rough estimate that double is OK
        double x_ctr = cmd.float_vals[0];
        double y_ctr = cmd.float_vals[1];
        double x_mag_factor = 1;
        double rotation = 0;
        double skew = 0;
        if (cmd.num_float_params > 3)
        {
            x_mag_factor = cmd.float_vals[3];
        }
        if (x_mag_factor == 0)
        {
            x_mag_factor = 1;
        }
        if (cmd.num_float_params > 4)
        {
            rotation = cmd.float_vals[4];
        }
        if (cmd.num_float_params > 5)
        {
            skew = cmd.float_vals[5];
        }
        // calculate bounds
        cvt_corners(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
        return CmdArgFlags::FRACTAL_PARAM;
    }

    // use arbitrary precision
    s_init_corners = true;
    BFMathType old_bf_math = g_bf_math;
    if (g_bf_math == BFMathType::NONE || dec > g_decimals)
    {
        init_bf_dec(dec);
    }
    if (old_bf_math == BFMathType::NONE)
    {
        for (int k = 0; k < MAX_PARAMS; k++)
        {
            float_to_bf(g_bf_params[k], g_params[k]);
        }
    }
    g_use_center_mag = true;
    int saved = save_stack();
    BigFloat b_x_ctr = alloc_stack(g_bf_length + 2);
    BigFloat b_y_ctr = alloc_stack(g_bf_length + 2);
    get_bf(b_x_ctr, cmd.float_val_strs[0]);
    get_bf(b_y_ctr, cmd.float_val_strs[1]);
    double x_mag_factor = 1;
    double rotation = 0;
    double skew = 0;
    if (cmd.num_float_params > 3)
    {
        x_mag_factor = cmd.float_vals[3];
    }
    if (x_mag_factor == 0)
    {
        x_mag_factor = 1;
    }
    if (cmd.num_float_params > 4)
    {
        rotation = cmd.float_vals[4];
    }
    if (cmd.num_float_params > 5)
    {
        skew = cmd.float_vals[5];
    }
    // calculate bounds
    cvt_corners_bf(b_x_ctr, b_y_ctr, magnification, x_mag_factor, rotation, skew);
    bf_corners_to_float();
    restore_stack(saved);
    return CmdArgFlags::FRACTAL_PARAM;
}

// coarse=?
static CmdArgFlags cmd_coarse(const Command &cmd)
{
    if (cmd.num_val < 3 || cmd.num_val > 2000)
    {
        return cmd.bad_arg();
    }
    g_preview_factor = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags parse_colors(const char *value)
{
    if (*value == '@')
    {
        if (merge_path_names(g_map_name, &value[1], CmdFile::AT_CMD_LINE_SET_NAME) < 0)
        {
            init_msg("", &value[1], CmdFile::AT_CMD_LINE_SET_NAME);
        }
        if ((int)std::strlen(value) > ID_FILE_MAX_PATH || validate_luts(g_map_name.c_str()))
        {
            goto badcolor;
        }
        if (g_display_3d != Display3DMode::NONE)
        {
            g_map_set = true;
        }
        else
        {
            if (merge_path_names(g_color_file, &value[1], CmdFile::AT_CMD_LINE_SET_NAME) < 0)
            {
                init_msg("", &value[1], CmdFile::AT_CMD_LINE_SET_NAME);
            }
            g_color_state = ColorState::MAP_FILE;
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
                    if (k <= '9')
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
                    g_dac_box[i][j] = (Byte)k;
                    if (smooth)
                    {
                        int spread = smooth + 1;
                        int start = i - spread;
                        int c_num{};
                        if ((k - (int)g_dac_box[start][j]) == 0)
                        {
                            while (++c_num < spread)
                            {
                                g_dac_box[start+c_num][j] = (Byte)k;
                            }
                        }
                        else
                        {
                            while (++c_num < spread)
                            {
                                g_dac_box[start+c_num][j] =
                                    (Byte)((c_num *g_dac_box[i][j]
                                            + (i-(start+c_num))*g_dac_box[start][j]
                                            + spread/2)
                                           / (Byte) spread);
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
        g_color_state = ColorState::UNKNOWN;
    }
    g_colors_preloaded = true;
    std::memcpy(g_old_dac_box, g_dac_box, 256*3);
    return CmdArgFlags::NONE;
badcolor:
    return CmdArgFlags::BAD_ARG;
}

// colors=, set current colors
static CmdArgFlags cmd_colors(const Command &cmd)
{
    if (parse_colors(cmd.value) == CmdArgFlags::BAD_ARG)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_comment(const Command &cmd)
{
    parse_comments(cmd.value);
    return CmdArgFlags::NONE;
}

// converge=?
static CmdArgFlags cmd_converge(const Command &cmd)
{
    g_converge_x_adjust = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// corners=?/?/?/?
static CmdArgFlags cmd_corners(const Command &cmd)
{
    if (g_fractal_type == FractalType::CELLULAR)
    {
        return CmdArgFlags::FRACTAL_PARAM; // skip setting the corners
    }

    if (cmd.num_float_params != cmd.total_params || (cmd.total_params != 0 && cmd.total_params != 4 && cmd.total_params != 6))
    {
        return cmd.bad_arg();
    }

    g_use_center_mag = false;
    if (cmd.total_params == 0)
    {
        return CmdArgFlags::NONE; // turns corners mode on
    }

    s_init_corners = true;
    // good first approx, but dec could be too big
    int dec = get_max_cur_arg_len(cmd.float_val_strs, cmd.total_params) + 1;
    if ((dec > DBL_DIG + 1 || g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH) &&
        g_debug_flag != DebugFlags::PREVENT_ARBITRARY_PRECISION_MATH)
    {
        BFMathType old_bf_math = g_bf_math;
        if (g_bf_math == BFMathType::NONE || dec > g_decimals)
        {
            init_bf_dec(dec);
        }
        if (old_bf_math == BFMathType::NONE)
        {
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                float_to_bf(g_bf_params[k], g_params[k]);
            }
        }

        // xx3rd = xxmin = floatval[0];
        get_bf(g_bf_x_min, cmd.float_val_strs[0]);
        get_bf(g_bf_x_3rd, cmd.float_val_strs[0]);

        // xxmax = floatval[1];
        get_bf(g_bf_x_max, cmd.float_val_strs[1]);

        // yy3rd = yymin = floatval[2];
        get_bf(g_bf_y_min, cmd.float_val_strs[2]);
        get_bf(g_bf_y_3rd, cmd.float_val_strs[2]);

        // yymax = floatval[3];
        get_bf(g_bf_y_max, cmd.float_val_strs[3]);

        if (cmd.total_params == 6)
        {
            // xx3rd = floatval[4];
            get_bf(g_bf_x_3rd, cmd.float_val_strs[4]);

            // yy3rd = floatval[5];
            get_bf(g_bf_y_3rd, cmd.float_val_strs[5]);
        }

        // now that all the corners have been read in, get a more
        // accurate value for dec and do it all again

        dec = get_prec_bf_mag();
        if (dec < 0)
        {
            return cmd.bad_arg(); // ie: magnification is +-1.#INF
        }

        if (dec > g_decimals) // get corners again if we need more precision
        {
            init_bf_dec(dec);

            // now get parameters and corners all over again at new
            // decimal setting
            for (int k = 0; k < MAX_PARAMS; k++)
            {
                float_to_bf(g_bf_params[k], g_params[k]);
            }

            // xx3rd = xxmin = floatval[0];
            get_bf(g_bf_x_min, cmd.float_val_strs[0]);
            get_bf(g_bf_x_3rd, cmd.float_val_strs[0]);

            // xxmax = floatval[1];
            get_bf(g_bf_x_max, cmd.float_val_strs[1]);

            // yy3rd = yymin = floatval[2];
            get_bf(g_bf_y_min, cmd.float_val_strs[2]);
            get_bf(g_bf_y_3rd, cmd.float_val_strs[2]);

            // yymax = floatval[3];
            get_bf(g_bf_y_max, cmd.float_val_strs[3]);

            if (cmd.total_params == 6)
            {
                // xx3rd = floatval[4];
                get_bf(g_bf_x_3rd, cmd.float_val_strs[4]);

                // yy3rd = floatval[5];
                get_bf(g_bf_y_3rd, cmd.float_val_strs[5]);
            }
        }
    }
    g_x_min = cmd.float_vals[0];
    g_x_3rd = cmd.float_vals[0];
    g_x_max = cmd.float_vals[1];
    g_y_min = cmd.float_vals[2];
    g_y_3rd = cmd.float_vals[2];
    g_y_max = cmd.float_vals[3];

    if (cmd.total_params == 6)
    {
        g_x_3rd = cmd.float_vals[4];
        g_y_3rd = cmd.float_vals[5];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// crop=?/?/?/?
static CmdArgFlags cmd_crop(const Command &cmd)
{
    if (cmd.total_params != 4 || cmd.num_int_params != 4 || cmd.int_vals[0] < 0 || cmd.int_vals[0] > 100 ||
        cmd.int_vals[1] < 0 || cmd.int_vals[1] > 100 || cmd.int_vals[2] < 0 || cmd.int_vals[2] > 100 ||
        cmd.int_vals[3] < 0 || cmd.int_vals[3] > 100)
    {
        return cmd.bad_arg();
    }
    g_red_crop_left = cmd.int_vals[0];
    g_red_crop_right = cmd.int_vals[1];
    g_blue_crop_left = cmd.int_vals[2];
    g_blue_crop_right = cmd.int_vals[3];
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// curdir=?
static CmdArgFlags cmd_cur_dir(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_check_cur_dir = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_cycle_limit(const Command &cmd)
{
    if (cmd.num_val <= 1 || cmd.num_val > 256)
    {
        return cmd.bad_arg();
    }
    g_init_cycle_limit = cmd.num_val;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_cycle_range(const Command &cmd)
{
    int end{cmd.int_vals[1]};
    if (cmd.total_params < 2)
    {
        end = 255;
    }
    int begin{cmd.int_vals[0]};
    if (cmd.total_params < 1)
    {
        begin = 1;
    }
    if (cmd.total_params != cmd.num_int_params || begin < 0 || end > 255 || begin > end)
    {
        return cmd.bad_arg();
    }
    g_color_cycle_range_lo = begin;
    g_color_cycle_range_hi = end;
    return CmdArgFlags::NONE;
}

// debugflag=? or debug=?
static CmdArgFlags cmd_debug_flag(const Command &cmd)
{
    // internal use only
    g_debug_flag = static_cast<DebugFlags>(cmd.num_val);
    g_timer_flag = (g_debug_flag & DebugFlags::BENCHMARK_TIMER) != DebugFlags::NONE; // separate timer flag
    g_debug_flag &= ~DebugFlags::BENCHMARK_TIMER;
    return CmdArgFlags::NONE;
}

// decay=?
static CmdArgFlags cmd_decay(const Command &cmd)
{
    g_fm_decay = cmd.num_val & 0x0F;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_decomp(const Command &cmd)
{
    if (cmd.total_params != cmd.num_int_params || cmd.total_params < 1)
    {
        return cmd.bad_arg();
    }
    g_decomp[0] = cmd.int_vals[0];
    g_decomp[1] = 0;
    if (cmd.total_params > 1) // backward compatibility
    {
        g_decomp[1] = cmd.int_vals[1];
        g_bailout = g_decomp[1];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_dist_est(const Command &cmd)
{
    if (cmd.total_params != cmd.num_int_params || cmd.total_params < 1)
    {
        return cmd.bad_arg();
    }
    g_user_distance_estimator_value = (long) cmd.float_vals[0];
    g_distance_estimator_width_factor = 71;
    if (cmd.total_params > 1)
    {
        g_distance_estimator_width_factor = cmd.int_vals[1];
    }
    if (cmd.total_params > 3 && cmd.int_vals[2] > 0 && cmd.int_vals[3] > 0)
    {
        g_distance_estimator_x_dots = cmd.int_vals[2];
        g_distance_estimator_y_dots = cmd.int_vals[3];
    }
    else
    {
        g_distance_estimator_y_dots = 0;
        g_distance_estimator_x_dots = 0;
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_dither(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_dither_flag = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

// fastrestore=?
static CmdArgFlags cmd_fast_restore(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_fast_restore = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_file_name(const Command &cmd)
{
    if (cmd.char_val[0] == '.' && cmd.value[1] != SLASH_CH)
    {
        if (cmd.value_len > 4)
        {
            return cmd.bad_arg();
        }
        g_gif_filename_mask = std::string{"*"} + cmd.value;
        return CmdArgFlags::NONE;
    }
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (cmd.mode == CmdFile::AT_AFTER_STARTUP &&
        g_display_3d == Display3DMode::NONE) // can't do this in @ command
    {
        return cmd.bad_arg();
    }

    const int exist_dir = merge_path_names(g_read_filename, cmd.value, cmd.mode);
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
        g_browse_name = extract_file_name(g_read_filename.c_str());
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// fillcolor=?
static CmdArgFlags cmd_fill_color(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "normal")
    {
        g_fill_color = -1;
    }
    else if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }
    else
    {
        g_fill_color = cmd.num_val;
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// filltype=?
static CmdArgFlags cmd_fill_type(const Command &cmd)
{
    if (cmd.num_val < +FillType::SURFACE_GRID || cmd.num_val > +FillType::LIGHT_SOURCE_AFTER)
    {
        return cmd.bad_arg();
    }
    g_fill_type = static_cast<FillType>(cmd.num_val);
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_fin_attract(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_finite_attractor = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

// float=?
// deprecated, validate argument and swallow value
static CmdArgFlags cmd_float(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

// formulafile=?
static CmdArgFlags cmd_formula_file(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_path_names(g_formula_filename, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// formulaname=?
static CmdArgFlags cmd_formula_name(const Command &cmd)
{
    if (cmd.value_len > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_formula_name = cmd.value;
    return CmdArgFlags::FRACTAL_PARAM;
}

// fullcolor=?
static CmdArgFlags cmd_full_color(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_targa_out = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_function(const Command &cmd)
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
    return CmdArgFlags::FRACTAL_PARAM;
}

// deprecated, validate argument and swallow value
static CmdArgFlags cmd_gif87a(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

// haze=?
static CmdArgFlags cmd_haze(const Command &cmd)
{
    if (cmd.num_val < 0 || cmd.num_val > 100)
    {
        return cmd.bad_arg();
    }
    g_haze = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// hertz=?
static CmdArgFlags cmd_hertz(const Command &cmd)
{
    g_base_hertz = cmd.num_val;
    return CmdArgFlags::NONE;
}

// ifs=? or ifs3d=?
static CmdArgFlags cmd_ifs(const Command &cmd)
{
    // ifs3d for old time's sake
    if (cmd.value_len > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_ifs_name = cmd.value;
    reset_ifs_definition();
    return CmdArgFlags::FRACTAL_PARAM;
}

// ifsfile=??
static CmdArgFlags cmd_ifs_file(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (const int exist_dir = merge_path_names(g_ifs_filename, cmd.value, cmd.mode); exist_dir == 0)
    {
        reset_ifs_definition();
    }
    else if (exist_dir < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// initorbit=?/?
static CmdArgFlags cmd_init_orbit(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "pixel")
    {
        g_use_init_orbit = InitOrbitMode::PIXEL;
    }
    else
    {
        if (cmd.total_params != 2 || cmd.num_float_params != 2)
        {
            return cmd.bad_arg();
        }
        g_init_orbit.x = cmd.float_vals[0];
        g_init_orbit.y = cmd.float_vals[1];
        g_use_init_orbit = InitOrbitMode::VALUE;
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_inside(const Command &cmd)
{
    struct Inside
    {
        std::string_view arg;
        int inside;
    };
    const Inside inside_names[] = {
        {"zmag", ZMAG},             //
        {"bof60", BOF60},           //
        {"bof61", BOF61},           //
        {"epsiloncross", EPS_CROSS}, //
        {"startrail", STAR_TRAIL},   //
        {"period", PERIOD},         //
        {"fmod", FMODI},            //
        {"atan", ATANI},            //
        {"maxiter", -1}             //
    };
    for (const Inside &arg : inside_names)
    {
        if (arg.arg == cmd.value)
        {
            g_inside_color = arg.inside;
            return CmdArgFlags::FRACTAL_PARAM;
        }
    }
    if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }

    g_inside_color = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM;
}

// interocular=?
static CmdArgFlags cmd_interocular(const Command &cmd)
{
    g_eye_separation = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// invert=?/?/?
static CmdArgFlags cmd_invert(const Command &cmd)
{
    if (cmd.total_params != cmd.num_float_params || (cmd.total_params != 1 && cmd.total_params != 3))
    {
        return cmd.bad_arg();
    }
    g_inversion[0] = cmd.float_vals[0];
    g_invert = (g_inversion[0] != 0.0) ? cmd.total_params : 0;
    if (cmd.total_params == 3)
    {
        g_inversion[1] = cmd.float_vals[1];
        g_inversion[2] = cmd.float_vals[2];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_is_mand(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_is_mandelbrot = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

// julibrot3d=?/?/?/?
static CmdArgFlags cmd_julibrot3d(const Command &cmd)
{
    if (cmd.num_float_params != cmd.total_params)
    {
        return cmd.bad_arg();
    }
    if (cmd.total_params > 0)
    {
        g_julibrot_z_dots = (int) cmd.float_vals[0];
    }
    if (cmd.total_params > 1)
    {
        g_julibrot_origin_fp = (float) cmd.float_vals[1];
    }
    if (cmd.total_params > 2)
    {
        g_julibrot_depth_fp = (float) cmd.float_vals[2];
    }
    if (cmd.total_params > 3)
    {
        g_julibrot_height_fp = (float) cmd.float_vals[3];
    }
    if (cmd.total_params > 4)
    {
        g_julibrot_width_fp = (float) cmd.float_vals[4];
    }
    if (cmd.total_params > 5)
    {
        g_julibrot_dist_fp = (float) cmd.float_vals[5];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// julibroteyes=?
static CmdArgFlags cmd_julibrot_eyes(const Command &cmd)
{
    if (cmd.num_float_params != cmd.total_params || cmd.total_params != 1)
    {
        return cmd.bad_arg();
    }
    g_eyes_fp = (float) cmd.float_vals[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

// julibrotfromto=?/?/?/?
static CmdArgFlags cmd_julibrot_from_to(const Command &cmd)
{
    if (cmd.num_float_params != cmd.total_params || cmd.total_params != 4)
    {
        return cmd.bad_arg();
    }
    g_julibrot_x_max = cmd.float_vals[0];
    g_julibrot_x_min = cmd.float_vals[1];
    g_julibrot_y_max = cmd.float_vals[2];
    g_julibrot_y_min = cmd.float_vals[3];
    return CmdArgFlags::FRACTAL_PARAM;
}

// latitude=?/?
static CmdArgFlags cmd_latitude(const Command &cmd)
{
    if (cmd.total_params != 2 || cmd.num_int_params != 2)
    {
        return cmd.bad_arg();
    }
    g_sphere_theta_min = cmd.int_vals[0];
    g_sphere_theta_max = cmd.int_vals[1];
    return CmdArgFlags::PARAM_3D;
}

// lfile=?
static CmdArgFlags cmd_l_file(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_path_names(g_l_system_filename, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// lightname=?
static CmdArgFlags cmd_light_name(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (g_first_init || cmd.mode == CmdFile::AT_AFTER_STARTUP)
    {
        g_light_name = cmd.value;
    }
    return CmdArgFlags::NONE;
}

// lightsource=?/?/?
static CmdArgFlags cmd_light_source(const Command &cmd)
{
    if (cmd.total_params != 3 || cmd.num_int_params != 3)
    {
        return cmd.bad_arg();
    }
    g_light_x = cmd.int_vals[0];
    g_light_y = cmd.int_vals[1];
    g_light_z = cmd.int_vals[2];
    return CmdArgFlags::PARAM_3D;
}

// lname=?
static CmdArgFlags cmd_l_name(const Command &cmd)
{
    if (cmd.value_len > ITEM_NAME_LEN)
    {
        return cmd.bad_arg();
    }
    g_l_system_name = cmd.value;
    return CmdArgFlags::FRACTAL_PARAM;
}

// logmap=?
static CmdArgFlags cmd_log_map(const Command &cmd)
{
    g_log_map_auto_calculate = false; // turn this off if loading a PAR
    if (cmd.char_val[0] == 'y')
    {
        g_log_map_flag = 1; // palette is logarithmic
    }
    else if (cmd.char_val[0] == 'n')
    {
        g_log_map_flag = 0;
    }
    else if (cmd.char_val[0] == 'o')
    {
        g_log_map_flag = -1; // old log palette
    }
    else
    {
        g_log_map_flag = (long) cmd.float_vals[0];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// logmode=?
static CmdArgFlags cmd_log_mode(const Command &cmd)
{
    g_log_map_fly_calculate = 0; // turn off if error
    g_log_map_auto_calculate = false;
    if (cmd.char_val[0] == 'f')
    {
        g_log_map_fly_calculate = 1; // calculate on the fly
    }
    else if (cmd.char_val[0] == 't')
    {
        g_log_map_fly_calculate = 2; // force use of LogTable
    }
    else if (cmd.char_val[0] == 'a')
    {
        g_log_map_auto_calculate = true; // force auto calc of logmap
    }
    else
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// longitude=?/?
static CmdArgFlags cmd_longitude(const Command &cmd)
{
    if (cmd.total_params != 2 || cmd.num_int_params != 2)
    {
        return cmd.bad_arg();
    }
    g_sphere_phi_min = cmd.int_vals[0];
    g_sphere_phi_max = cmd.int_vals[1];
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_make_mig(const Command &cmd)
{
    if (cmd.total_params < 2)
    {
        return cmd.bad_arg();
    }
    const int x_mult = cmd.int_vals[0];
    const int y_mult = cmd.int_vals[1];
    make_mig(x_mult, y_mult);
    cmd_files_test::s_goodbye();
    return CmdArgFlags::GOODBYE;
}

// map=, set default colors
static CmdArgFlags cmd_map(const Command &cmd)
{
    if (cmd.value_len > ID_FILE_MAX_PATH - 1)
    {
        return cmd.bad_arg();
    }

    const int exist_dir = merge_path_names(g_map_name, cmd.value, cmd.mode);
    if (exist_dir > 0)
    {
        return CmdArgFlags::NONE; // got a directory
    }
    if (exist_dir < 0) // error
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
        return CmdArgFlags::NONE;
    }
    set_color_palette_name(g_map_name.c_str());
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_math_tolerance(const Command &cmd)
{
    if (cmd.char_val[0] == '/')
    {
        // leave math_tol[0] at the default value
    }
    else if (cmd.total_params >= 1)
    {
        g_math_tol[0] = cmd.float_vals[0];
    }

    if (cmd.total_params >= 2)
    {
        g_math_tol[1] = cmd.float_vals[1];
    }
    return CmdArgFlags::NONE;
}

// maxcolorres deprecated, validate value and gobble argument
// Change default color resolution
static CmdArgFlags cmd_max_color_res(const Command &cmd)
{
    if (cmd.num_val == 1 || cmd.num_val == 4 || cmd.num_val == 8 || cmd.num_val == 16 || cmd.num_val == 24)
    {
        return CmdArgFlags::NONE;
    }
    return cmd.bad_arg();
}

static CmdArgFlags cmd_max_iter(const Command &cmd)
{
    if (cmd.float_vals[0] < 2)
    {
        return cmd.bad_arg();
    }
    g_max_iterations = (long) cmd.float_vals[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_max_line_length(const Command &cmd)
{
    if (cmd.num_val < MIN_MAX_LINE_LENGTH || cmd.num_val > MAX_MAX_LINE_LENGTH)
    {
        return cmd.bad_arg();
    }
    g_max_line_length = cmd.num_val;
    return CmdArgFlags::NONE;
}

// miim=?[/?[/?[/?]]]
static CmdArgFlags cmd_miim(const Command &cmd)
{
    if (cmd.total_params > 6)
    {
        return cmd.bad_arg();
    }
    if (cmd.char_val[0] == 'b')
    {
        g_major_method = Major::BREADTH_FIRST;
    }
    else if (cmd.char_val[0] == 'd')
    {
        g_major_method = Major::DEPTH_FIRST;
    }
    else if (cmd.char_val[0] == 'w')
    {
        g_major_method = Major::RANDOM_WALK;
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

    if (cmd.char_val[1] == 'l')
    {
        g_inverse_julia_minor_method = Minor::LEFT_FIRST;
    }
    else if (cmd.char_val[1] == 'r')
    {
        g_inverse_julia_minor_method = Minor::RIGHT_FIRST;
    }
    else
    {
        return cmd.bad_arg();
    }

    // keep this next part in for backwards compatibility with old PARs ???

    if (cmd.total_params > 2)
    {
        for (int k = 2; k < 6; ++k)
        {
            g_params[k - 2] = (k < cmd.total_params) ? cmd.float_vals[k] : 0.0;
        }
    }

    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_min_stack(const Command &cmd)
{
    if (cmd.total_params != 1)
    {
        return cmd.bad_arg();
    }
    g_soi_min_stack = cmd.int_vals[0];
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_no_bof(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_bof_match_book_images = cmd.yes_no_val[0] == 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

// noninterlaced no longer used, validate value and gobble argument
static CmdArgFlags cmd_non_interlaced(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

// olddemmcolors=?
static CmdArgFlags cmd_old_demm_colors(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_old_demm_colors = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_orbit_delay(const Command &cmd)
{
    g_orbit_delay = cmd.num_val;
    return CmdArgFlags::NONE;
}

// orbit corners=?/?/?/?
static CmdArgFlags cmd_orbit_corners(const Command &cmd)
{
    g_set_orbit_corners = false;
    if (cmd.num_float_params != cmd.total_params || (cmd.total_params != 0 && cmd.total_params != 4 && cmd.total_params != 6))
    {
        return cmd.bad_arg();
    }
    g_orbit_corner_min_x = cmd.float_vals[0];
    g_orbit_corner_3rd_x = cmd.float_vals[0];
    g_orbit_corner_max_x = cmd.float_vals[1];
    g_orbit_corner_min_y = cmd.float_vals[2];
    g_orbit_corner_3rd_y = cmd.float_vals[2];
    g_orbit_corner_max_y = cmd.float_vals[3];

    if (cmd.total_params == 6)
    {
        g_orbit_corner_3rd_x = cmd.float_vals[4];
        g_orbit_corner_3rd_y = cmd.float_vals[5];
    }
    g_set_orbit_corners = true;
    g_keep_screen_coords = true;
    return CmdArgFlags::FRACTAL_PARAM;
}

// orbitdrawmode=?
// rectangle, line, or function (not yet tested or documented)
static CmdArgFlags cmd_orbit_draw_mode(const Command &cmd)
{
    if (cmd.char_val[0] != 'l' && cmd.char_val[0] != 'r' && cmd.char_val[0] != 'f')
    {
        return cmd.bad_arg();
    }
    g_draw_mode = cmd.char_val[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_orbit_interval(const Command &cmd)
{
    g_orbit_interval = cmd.num_val;
    g_orbit_interval = std::max(g_orbit_interval, 1L);
    g_orbit_interval = std::min(g_orbit_interval, 255L);
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_orbit_name(const Command &cmd)
{
    if (check_orbit_name(cmd.value))
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// orbitsave=?
static CmdArgFlags cmd_orbit_save(const Command &cmd)
{
    if (cmd.char_val[0] == 's')
    {
        g_orbit_save_flags |= OSF_MIDI;
    }
    else if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_orbit_save_flags |= (cmd.yes_no_val[0] ? OSF_RAW : 0);
    return CmdArgFlags::FRACTAL_PARAM;
}

// orbitsavename=?
static CmdArgFlags cmd_orbit_save_name(const Command &cmd)
{
    g_orbit_save_name = cmd.value;
    return CmdArgFlags::FRACTAL_PARAM;
}

// orgfrmdir=?
static CmdArgFlags cmd_org_frm_dir(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!is_a_directory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_organize_formulas_search = true;
    g_organize_formulas_dir = cmd.value;
    fix_dir_name(g_organize_formulas_dir);
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_outside(const Command &cmd)
{
    struct Outside
    {
        std::string_view arg;
        int outside;
    };
    static const Outside OUTSIDES[] = {
        {"iter", ITER}, //
        {"real", REAL}, //
        {"imag", IMAG}, //
        {"mult", MULT}, //
        {"summ", SUM},  //
        {"atan", ATAN}, //
        {"fmod", FMOD}, //
        {"tdis", TDIS}  //
    };
    for (const Outside &arg : OUTSIDES)
    {
        if (cmd.value == arg.arg)
        {
            g_outside_color = arg.outside;
            return CmdArgFlags::FRACTAL_PARAM;
        }
    }
    if (cmd.num_val == NON_NUMERIC || (cmd.num_val < TDIS || cmd.num_val > 255))
    {
        return cmd.bad_arg();
    }
    g_outside_color = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_overwrite(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_overwrite_file = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_params(const Command &cmd)
{
    if (cmd.total_params != cmd.num_float_params || cmd.total_params > MAX_PARAMS)
    {
        return cmd.bad_arg();
    }
    s_init_params = true;
    for (int k = 0; k < MAX_PARAMS; ++k)
    {
        g_params[k] = (k < cmd.total_params) ? cmd.float_vals[k] : 0.0;
    }
    if (g_bf_math != BFMathType::NONE)
    {
        for (int k = 0; k < MAX_PARAMS; k++)
        {
            float_to_bf(g_bf_params[k], g_params[k]);
        }
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// parmfile=?
static CmdArgFlags cmd_parm_file(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_PATH - 1))
    {
        return cmd.bad_arg();
    }
    if (merge_path_names(g_command_file, cmd.value, cmd.mode) < 0)
    {
        init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_passes(const Command &cmd)
{
    if (std::strchr("123gbtsdop", cmd.char_val[0]) == nullptr)
    {
        return cmd.bad_arg();
    }
    g_user_std_calc_mode = cmd.char_val[0];
    if (cmd.char_val[0] == 'g')
    {
        g_stop_pass = ((int) cmd.value[1] - (int) '0');
        if (g_stop_pass < 0 || g_stop_pass > 6)
        {
            g_stop_pass = 0;
        }
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// periodicity=?
static CmdArgFlags cmd_periodicity(const Command &cmd)
{
    g_user_periodicity_value = 1;
    if (cmd.char_val[0] == 'n' || cmd.num_val == 0)
    {
        g_user_periodicity_value = 0;
    }
    else if (cmd.char_val[0] == 'y')
    {
        g_user_periodicity_value = 1;
    }
    else if (cmd.char_val[0] == 's') // 's' for 'show'
    {
        g_user_periodicity_value = -1;
    }
    else if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }
    else
    {
        g_user_periodicity_value = cmd.num_val;
        g_user_periodicity_value = std::min(g_user_periodicity_value, 255);
        g_user_periodicity_value = std::max(g_user_periodicity_value, -255);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// perspective=?
static CmdArgFlags cmd_perspective(const Command &cmd)
{
    if (cmd.num_val == NON_NUMERIC)
    {
        return cmd.bad_arg();
    }
    g_viewer_z = cmd.num_val;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// pixelzoom no longer used, validate value and gobble argument
static CmdArgFlags cmd_pixel_zoom(const Command &cmd)
{
    if (cmd.num_val >= 5)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_polyphony(const Command &cmd)
{
    if (cmd.num_val > 9)
    {
        return cmd.bad_arg();
    }
    g_polyphony = std::abs(cmd.num_val - 1);
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_potential(const Command &cmd)
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
        if (std::string_view{value} != "16bit")
        {
            return cmd.bad_arg();
        }
        g_potential_16bit = true;
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// preview=?
static CmdArgFlags cmd_preview(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_preview = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_proximity(const Command &cmd)
{
    g_close_proximity = cmd.float_vals[0];
    return CmdArgFlags::FRACTAL_PARAM;
}

// radius=?
static CmdArgFlags cmd_radius(const Command &cmd)
{
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }
    g_sphere_radius = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// randomize=?
static CmdArgFlags cmd_randomize(const Command &cmd)
{
    if (cmd.num_val < 0 || cmd.num_val > 7)
    {
        return cmd.bad_arg();
    }
    g_randomize_3d = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// ranges=?/?/.../?
static CmdArgFlags cmd_ranges(const Command &cmd)
{
    if (cmd.total_params != cmd.num_int_params)
    {
        return cmd.bad_arg();
    }

    int i = 0;
    int prev = i;
    int entries = prev;
    g_log_map_flag = 0; // ranges overrides logmap
    int tmp_ranges[128];
    while (i < cmd.total_params)
    {
        int k = cmd.int_vals[i++];
        if (k < 0) // striping
        {
            k = -k;
            if (k >= 16384 || i >= cmd.total_params)
            {
                return cmd.bad_arg();
            }
            tmp_ranges[entries++] = -1; // {-1,width,limit} for striping
            tmp_ranges[entries++] = k;
            k = cmd.int_vals[i++];
        }
        if (k < prev)
        {
            return cmd.bad_arg();
        }
        prev = k;
        tmp_ranges[entries++] = k;
    }
    if (prev == 0)
    {
        return cmd.bad_arg();
    }
    try
    {
        g_iteration_ranges.resize(entries);
    }
    catch (const std::bad_alloc &)
    {
        stop_msg(StopMsgFlags::NO_STACK, "Insufficient memory for ranges=");
        return CmdArgFlags::BAD_ARG;
    }
    g_iteration_ranges_len = entries;
    for (int i2 = 0; i2 < g_iteration_ranges_len; ++i2)
    {
        g_iteration_ranges[i2] = tmp_ranges[i2];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// ray=?
static CmdArgFlags cmd_ray(const Command &cmd)
{
    if (cmd.num_val < 0 || cmd.num_val > 6)
    {
        return cmd.bad_arg();
    }
    g_raytrace_format = static_cast<RayTraceFormat>(cmd.num_val);
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_record_colors(const Command &cmd)
{
    if (*cmd.value != 'y' && *cmd.value != 'c' && *cmd.value != 'a')
    {
        return cmd.bad_arg();
    }
    g_record_colors = static_cast<RecordColorsMode>(*cmd.value);
    return CmdArgFlags::NONE;
}

// release=?
static CmdArgFlags cmd_release(const Command &cmd)
{
    return cmd.bad_arg();
}

static CmdArgFlags cmd_reset(const Command &cmd)
{
    // PAR release unknown unless specified
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }

    init_vars_fractal();
    if (cmd.num_int_params == 1)
    {
        // Id version: reset=100, reset=101; legacy version: reset=1960
        if (cmd.int_vals[0] >= 100)
        {
            g_release = cmd.int_vals[0];
            g_version.major = g_release / 100;
            g_version.minor = g_release % 100;
            g_version.patch = 0;
            g_version.tweak = 0;
            g_version.legacy = cmd.int_vals[0] != 100 && cmd.int_vals[0] != 101;
        }
        // Id version: reset=<major>
        else
        {
            g_release = cmd.int_vals[0] * 100;
            g_version = Version{};
            g_version.major = cmd.int_vals[0];
        }
    }
    // Id version: reset=<major>/<minor>[/<patch>[/<tweak>]]
    else if (cmd.num_int_params > 1)
    {
        g_release = cmd.int_vals[0] * 100 + cmd.int_vals[1];
        g_version = Version{};
        g_version.major = cmd.int_vals[0];
        g_version.minor = cmd.int_vals[1];
        if (cmd.num_int_params > 2)
        {
            g_version.patch = cmd.int_vals[2];
        }
        if (cmd.num_int_params > 3)
        {
            g_version.tweak = cmd.int_vals[3];
        }
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET;
}

// rotation=?/?/?
static CmdArgFlags cmd_rotation(const Command &cmd)
{
    if (cmd.total_params != 3 || cmd.num_int_params != 3)
    {
        return cmd.bad_arg();
    }
    g_x_rot = cmd.int_vals[0];
    g_y_rot = cmd.int_vals[1];
    g_z_rot = cmd.int_vals[2];
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// roughness=?
static CmdArgFlags cmd_roughness(const Command &cmd)
{
    // "rough" is really scale z, but we add it here for convenience
    g_rough = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// rseed=?
static CmdArgFlags cmd_r_seed(const Command &cmd)
{
    g_random_seed = cmd.num_val;
    g_random_seed_flag = true;
    return CmdArgFlags::FRACTAL_PARAM;
}

static CmdArgFlags cmd_save_dir(const Command &cmd)
{
    g_save_dir = cmd.value;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_save_name(const Command &cmd)
{
    if (cmd.value_len > ID_FILE_MAX_PATH - 1)
    {
        return cmd.bad_arg();
    }
    if (g_first_init || cmd.mode == CmdFile::AT_AFTER_STARTUP)
    {
        if (merge_path_names(g_save_filename, cmd.value, cmd.mode) < 0)
        {
            init_msg(cmd.variable.c_str(), cmd.value, cmd.mode);
        }
    }
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_save_time(const Command &cmd)
{
    g_init_save_time = cmd.num_val;
    return CmdArgFlags::NONE;
}

// scalemap=?/?/?/?/?/?/?/?/?/?/?
static CmdArgFlags cmd_scale_map(const Command &cmd)
{
    if (cmd.total_params != cmd.num_int_params)
    {
        return cmd.bad_arg();
    }
    for (int counter = 0; counter <= 11; counter++)
    {
        if ((cmd.total_params > counter) && (cmd.int_vals[counter] > 0) && (cmd.int_vals[counter] < 13))
        {
            g_scale_map[counter] = cmd.int_vals[counter];
        }
    }
    return CmdArgFlags::NONE;
}

// scalexyz=?/?[/?]
static CmdArgFlags cmd_scale_xyz(const Command &cmd)
{
    if (cmd.total_params < 2 || cmd.num_int_params != cmd.total_params)
    {
        return cmd.bad_arg();
    }
    g_x_scale = cmd.int_vals[0];
    g_y_scale = cmd.int_vals[1];
    if (cmd.total_params > 2)
    {
        g_rough = cmd.int_vals[2];
    }
    return CmdArgFlags::PARAM_3D;
}

// screencoords=?
static CmdArgFlags cmd_screen_coords(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_keep_screen_coords = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

// showbox=?
static CmdArgFlags cmd_show_box(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_show_box = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

// showdot=?[/?]
static CmdArgFlags cmd_show_dot(const Command &cmd)
{
    g_show_dot = 15;
    if (cmd.total_params > 0)
    {
        g_auto_show_dot = (char) 0;
        if (std::isalpha(cmd.char_val[0]))
        {
            if (std::strchr("abdm", cmd.char_val[0]) != nullptr)
            {
                g_auto_show_dot = cmd.char_val[0];
            }
            else
            {
                return cmd.bad_arg();
            }
        }
        else
        {
            g_show_dot = cmd.num_val;
            if (g_show_dot < 0)
            {
                g_show_dot = -1;
            }
        }
        if (cmd.total_params > 1 && cmd.num_int_params > 0)
        {
            g_size_dot = cmd.int_vals[1];
        }
        g_size_dot = std::max(g_size_dot, 0);
    }
    return CmdArgFlags::NONE;
}

// showorbit=yes|no
static CmdArgFlags cmd_show_orbit(const Command &cmd)
{
    g_start_show_orbit = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::NONE;
}

// smoothing=?
static CmdArgFlags cmd_smoothing(const Command &cmd)
{
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }
    g_light_avg = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// sound=?[/?/?/?/?]
static CmdArgFlags cmd_sound(const Command &cmd)
{
    if (cmd.total_params > 5)
    {
        return cmd.bad_arg();
    }
    g_sound_flag = SOUNDFLAG_OFF; // start with a clean slate, add bits as we go
    if (cmd.total_params == 1)
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
    if (cmd.char_val[0] == 'n' || cmd.char_val[0] == 'o')
    {
        g_sound_flag &= ~SOUNDFLAG_ORBIT_MASK;
    }
    else if ((std::strncmp(cmd.value, "ye", 2) == 0) || (cmd.char_val[0] == 'b'))
    {
        g_sound_flag |= SOUNDFLAG_BEEP;
    }
    else if (cmd.char_val[0] == 'x')
    {
        g_sound_flag |= SOUNDFLAG_X;
    }
    else if (cmd.char_val[0] == 'y' && std::strncmp(cmd.value, "ye", 2) != 0)
    {
        g_sound_flag |= SOUNDFLAG_Y;
    }
    else if (cmd.char_val[0] == 'z')
    {
        g_sound_flag |= SOUNDFLAG_Z;
    }
    else
    {
        return cmd.bad_arg();
    }
    if (cmd.total_params > 1)
    {
        g_sound_flag &= SOUNDFLAG_ORBIT_MASK; // reset options
        for (int i = 1; i < cmd.total_params; i++)
        {
            // this is for 2 or more options at the same time
            if (cmd.char_val[i] == 'f')
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
            else if (cmd.char_val[i] == 'p')
            {
                g_sound_flag |= SOUNDFLAG_SPEAKER;
            }
            else if (cmd.char_val[i] == 'm')
            {
                g_sound_flag |= SOUNDFLAG_MIDI;
            }
            else if (cmd.char_val[i] == 'q')
            {
                g_sound_flag |= SOUNDFLAG_QUANTIZED;
            }
            else
            {
                return cmd.bad_arg();
            }
        } // end for
    }     // end totparms > 1
    return CmdArgFlags::NONE;
}

// sphere=?
static CmdArgFlags cmd_sphere(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_sphere = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

// srelease=?
static CmdArgFlags cmd_s_release(const Command &cmd)
{
    g_fm_release = cmd.num_val & 0x0F;
    return CmdArgFlags::NONE;
}

// stereo=?
static CmdArgFlags cmd_stereo(const Command &cmd)
{
    if ((cmd.num_val < 0) || (cmd.num_val > 4))
    {
        return cmd.bad_arg();
    }
    g_glasses_type = static_cast<GlassesType>(cmd.num_val);
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// stereowidth=?, monitorwidth=?
static CmdArgFlags cmd_stereo_width(const Command &cmd)
{
    if (cmd.total_params != 1 || cmd.num_float_params != 1)
    {
        return cmd.bad_arg();
    }
    g_auto_stereo_width = cmd.float_vals[0];
    return CmdArgFlags::PARAM_3D;
}

// sustain=?
static CmdArgFlags cmd_sustain(const Command &cmd)
{
    g_fm_sustain = cmd.num_val & 0x0F;
    return CmdArgFlags::NONE;
}

// symmetry=?
static CmdArgFlags cmd_symmetry(const Command &cmd)
{
    const std::string_view value{cmd.value};
    if (value == "xaxis")
    {
        g_force_symmetry = SymmetryType::X_AXIS;
    }
    else if (value == "yaxis")
    {
        g_force_symmetry = SymmetryType::Y_AXIS;
    }
    else if (value == "xyaxis")
    {
        g_force_symmetry = SymmetryType::XY_AXIS;
    }
    else if (value == "origin")
    {
        g_force_symmetry = SymmetryType::ORIGIN;
    }
    else if (value == "pi")
    {
        g_force_symmetry = SymmetryType::PI_SYM;
    }
    else if (value == "none")
    {
        g_force_symmetry = SymmetryType::NONE;
    }
    else
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// targaoverlay=?
static CmdArgFlags cmd_targa_overlay(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_targa_overlay = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_temp_dir(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!is_a_directory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_temp_dir = cmd.value;
    fix_dir_name(g_temp_dir);
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_text_colors(const Command &cmd)
{
    const char *value = cmd.value;
    if (std::string_view(value) == "mono")
    {
        for (Byte &elem : g_text_color)
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
        g_text_color[25] = BLACK * 16 + LT_WHITE;
        g_text_color[24] = BLACK * 16 + LT_WHITE;
        g_text_color[22] = BLACK * 16 + LT_WHITE;
        g_text_color[17] = BLACK * 16 + LT_WHITE;
        g_text_color[16] = BLACK * 16 + LT_WHITE;
        g_text_color[11] = BLACK * 16 + LT_WHITE;
        g_text_color[5] = BLACK * 16 + LT_WHITE;
        g_text_color[2] = BLACK * 16 + LT_WHITE;
        g_text_color[0] = BLACK * 16 + LT_WHITE;
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
                unsigned int hex_val;
                std::sscanf(value, "%x", &hex_val);
                unsigned int i = (hex_val / 16) & 7;
                unsigned int j = hex_val & 15;
                if (i == j || (i == 0 && j == 8)) // force contrast
                {
                    j = 15;
                }
                g_text_color[k] = (Byte) (i * 16 + j);
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

    return CmdArgFlags::NONE;
}

// tplus no longer used, validate value and gobble argument
static CmdArgFlags cmd_tplus(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::NONE;
}

// transparent=?
static CmdArgFlags cmd_transparent(const Command &cmd)
{
    if (cmd.total_params != cmd.num_int_params || cmd.total_params < 1)
    {
        return cmd.bad_arg();
    }
    g_transparent_color_3d[0] = cmd.int_vals[0];
    g_transparent_color_3d[1] = cmd.int_vals[0];
    if (cmd.total_params > 1)
    {
        g_transparent_color_3d[1] = cmd.int_vals[1];
    }
    return CmdArgFlags::PARAM_3D;
}

// truecolor=?
static CmdArgFlags cmd_true_color(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_true_color = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// truemode=?
static CmdArgFlags cmd_true_mode(const Command &cmd)
{
    g_true_mode = TrueColorMode::DEFAULT_COLOR;
    if (cmd.char_val[0] == 'd')
    {
        g_true_mode = TrueColorMode::DEFAULT_COLOR;
    }
    if (cmd.char_val[0] == 'i' || cmd.int_vals[0] == 1)
    {
        g_true_mode = TrueColorMode::ITERATE;
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_type(const Command &cmd)
{
    std::string value{cmd.value, static_cast<std::string::size_type>(cmd.value_len)};
    if (!value.empty() && value.back() == '*')
    {
        value.pop_back();
    }

    // kludge because type ifs3d has an asterisk in front
    if (value == "ifs3d")
    {
        value = "ifs";
    }
    FractalType type{FractalType::NO_FRACTAL};
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (value == g_fractal_specific[i].name)
        {
            type = g_fractal_specific[i].type;
            break;
        }
    }
    if (type == FractalType::NO_FRACTAL)
    {
        return cmd.bad_arg();
    }
    const FractalType previous{g_fractal_type};
    set_fractal_type(type);
    if (!s_init_corners)
    {
        g_x_min = g_cur_fractal_specific->x_min;
        g_x_3rd = g_x_min;
        g_x_max = g_cur_fractal_specific->x_max;
        g_y_min = g_cur_fractal_specific->y_min;
        g_y_3rd = g_y_min;
        g_y_max = g_cur_fractal_specific->y_max;
    }
    if (!s_init_functions)
    {
        set_fractal_default_functions(previous);
    }
    if (!s_init_params)
    {
        load_params(g_fractal_type);
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// usegrayscale=?
static CmdArgFlags cmd_use_gray_scale(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_gray_flag = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::PARAM_3D;
}

static CmdArgFlags cmd_video(const Command &cmd)
{
    const int k = check_vid_mode_key_name(cmd.value);
    if (k == 0)
    {
        return cmd.bad_arg();
    }
    g_init_mode = -1;
    for (int i = 0; i < g_video_table_len; ++i)
    {
        if (g_video_table[i].key == k)
        {
            g_init_mode = i;
            break;
        }
    }
    if (g_init_mode == -1)
    {
        return cmd.bad_arg();
    }
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// viewwindows=?/?/?/?/?
static CmdArgFlags cmd_view_windows(const Command &cmd)
{
    if (cmd.total_params > 5 || cmd.num_float_params - cmd.num_int_params > 2 || cmd.num_int_params > 4)
    {
        return cmd.bad_arg();
    }

    g_view_window = true;
    g_view_reduction = 4.2F; // reset default values
    g_final_aspect_ratio = g_screen_aspect;
    g_view_crop = true;
    g_view_x_dots = 0;
    g_view_y_dots = 0;

    if (cmd.total_params > 0 && cmd.float_vals[0] > 0.001)
    {
        g_view_reduction = (float) cmd.float_vals[0];
    }
    if (cmd.total_params > 1 && cmd.float_vals[1] > 0.001)
    {
        g_final_aspect_ratio = (float) cmd.float_vals[1];
    }
    if (cmd.total_params > 2 && cmd.yes_no_val[2] == 0)
    {
        g_view_crop = cmd.yes_no_val[2] != 0;
    }
    if (cmd.total_params > 3 && cmd.int_vals[3] > 0)
    {
        g_view_x_dots = cmd.int_vals[3];
    }
    if (cmd.total_params == 5 && cmd.int_vals[4] > 0)
    {
        g_view_y_dots = cmd.int_vals[4];
    }
    return CmdArgFlags::FRACTAL_PARAM;
}

// virtual=?
static CmdArgFlags cmd_virtual(const Command &cmd)
{
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_virtual_screens = cmd.yes_no_val[0] != 0;
    return CmdArgFlags::FRACTAL_PARAM;
}

// volume=?
static CmdArgFlags cmd_volume(const Command &cmd)
{
    g_fm_volume = cmd.num_val & 0x3F; // 63
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_warn(const Command &cmd)
{
    // keep this for backward compatibility
    if (cmd.yes_no_val[0] < 0)
    {
        return cmd.bad_arg();
    }
    g_overwrite_file = cmd.yes_no_val[0] == 0;
    return CmdArgFlags::NONE;
}

// waterline=?
static CmdArgFlags cmd_water_line(const Command &cmd)
{
    if (cmd.num_val < 0)
    {
        return cmd.bad_arg();
    }
    g_water_line = cmd.num_val;
    return CmdArgFlags::PARAM_3D;
}

// wavetype=?
static CmdArgFlags cmd_wave_type(const Command &cmd)
{
    g_fm_wave_type = cmd.num_val & 0x0F;
    return CmdArgFlags::NONE;
}

static CmdArgFlags cmd_work_dir(const Command &cmd)
{
    if (cmd.value_len > (ID_FILE_MAX_DIR - 1))
    {
        return cmd.bad_arg();
    }
    if (!is_a_directory(cmd.value))
    {
        return cmd.bad_arg();
    }
    g_working_dir = cmd.value;
    fix_dir_name(g_working_dir);
    return CmdArgFlags::NONE;
}

// xyadjust=?
static CmdArgFlags cmd_xy_adjust(const Command &cmd)
{
    if (cmd.total_params != 2 || cmd.num_int_params != 2)
    {
        return cmd.bad_arg();
    }
    g_adjust_3d_x = cmd.int_vals[0];
    g_adjust_3d_y = cmd.int_vals[1];
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// xyshift=?/?
static CmdArgFlags cmd_xy_shift(const Command &cmd)
{
    if (cmd.total_params != 2 || cmd.num_int_params != 2)
    {
        return cmd.bad_arg();
    }
    g_shift_x = cmd.int_vals[0];
    g_shift_y = cmd.int_vals[1];
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D;
}

// tolerance=?
static CmdArgFlags cmd_tolerance(const Command &cmd)
{
    g_perturbation_tolerance = cmd.float_vals[0];
    return CmdArgFlags::NONE;
}

// perturbation=?
static CmdArgFlags cmd_perturbation(const Command &cmd)
{
    std::string p = cmd.value;
    if (p == "auto")
    {
        g_perturbation = PerturbationMode::AUTO;
    }
    else if (p == "yes")
    {
        g_perturbation = PerturbationMode::YES;
    }
    else
    {
        g_perturbation = PerturbationMode::NO;
    }
    return CmdArgFlags::NONE;
}

// Keep this sorted by parameter name for binary search to work correctly.
static std::array<CommandHandler, 159> s_commands{
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
    CommandHandler{"bailout", cmd_bailout},                //
    CommandHandler{"bailoutest", cmd_bailout_test},        //
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
    CommandHandler{"filename", cmd_file_name},               //
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
    CommandHandler{"julibrot3d", cmd_julibrot3d},          //
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
    CommandHandler{"perturbation", cmd_perturbation},       //
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
    CommandHandler{"savedir", cmd_save_dir},                //
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
    CommandHandler{"tolerance", cmd_tolerance},             // perturbation glitch tolerance
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
static std::optional<CmdArgFlags> handle_command(const std::array<CommandHandler, N> &handlers, Command &cmd)
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
CmdArgFlags cmd_arg(char *cur_arg, CmdFile mode) // process a single argument
{
    Command cmd{cur_arg, mode};
    if (cmd.status != CmdArgFlags::NONE)
    {
        return cmd.status;
    }

    if (mode != CmdFile::AT_AFTER_STARTUP || g_debug_flag == DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)
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

static void arg_error(const char *bad_arg)      // oops. couldn't decode this
{
    std::string spillover{bad_arg};
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
    cmd_files_test::s_stop_msg(StopMsgFlags::NONE, msg);
    if (g_init_batch != BatchMode::NONE)
    {
        g_init_batch = BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE;
        cmd_files_test::s_goodbye();
    }
}

void set_3d_defaults()
{
    g_rough     = 30;
    g_water_line = 0;
    g_viewer_z   = 0;
    g_shift_x    = 0;
    g_shift_y    = 0;
    g_adjust_3d_x    = 0;
    g_adjust_3d_y    = 0;
    g_light_avg  = 0;
    g_ambient   = 20;
    g_randomize_3d = 0;
    g_haze      = 0;
    g_background_color[0] = 51;
    g_background_color[1] = 153;
    g_background_color[2] = 200;
    if (g_sphere)
    {
        g_sphere_phi_min      =  180;
        g_sphere_phi_max      =  0;
        g_sphere_theta_min    =  -90;
        g_sphere_theta_max    =  90;
        g_sphere_radius    =  100;
        g_fill_type  = FillType::SURFACE_INTERPOLATED;
        g_light_x    = 1;
        g_light_y    = 1;
        g_light_z    = 1;
    }
    else
    {
        g_x_rot      = 60;
        g_y_rot      = 30;
        g_z_rot      = 0;
        g_x_scale    = 90;
        g_y_scale    = 90;
        g_fill_type  = FillType::POINTS;
        g_light_x    = 1;
        g_light_y    = -1;
        g_light_z    = 1;
    }
}

// copy a big number from a string, up to slash
static int get_bf(BigFloat bf, const char *cur_arg)
{
    if (const char *s = std::strchr(cur_arg, '/'); s)
    {
        std::string buff(cur_arg, s);
        str_to_bf(bf, buff.c_str());
    }
    else
    {
        str_to_bf(bf, cur_arg);
    }
    return 0;
}

// Get length of current args
static int get_cur_arg_len(const char *cur_arg)
{
    if (const char *s = std::strchr(cur_arg, '/'); s)
    {
        return static_cast<int>(s - cur_arg);
    }

    return static_cast<int>(std::strlen(cur_arg));
}

// Get max length of current args
static int get_max_cur_arg_len(const char *const float_val_str[], int num_args)
{
    int max_str = 0;
    for (int i = 0; i < num_args; i++)
    {
        int tmp = get_cur_arg_len(float_val_str[i]);
        max_str = std::max(tmp, max_str);
    }
    return max_str;
}

static std::string to_string(CmdFile value)
{
    static const char *const mode_str[4] = {"command line", "sstools.ini", "PAR file", "PAR file"};
    return mode_str[static_cast<int>(value)];
}

// mode = 0 command line @filename
//        1 sstools.ini
//        2 <@> command after startup
//        3 command line @filename/setname
// this is like stopmsg() but can be used in cmdfiles()
// call with NULL for badfilename to get pause for driver_get_key()
int init_msg(const char *cmd_str, const char *bad_filename, CmdFile mode)
{
    static int row = 1;

    if (g_init_batch == BatchMode::NORMAL)
    {
        // in batch mode
        if (bad_filename)
        {
            return -1;
        }
    }
    char cmd[80];
    std::strncpy(cmd, cmd_str, 30);
    cmd[29] = 0;

    if (*cmd)
    {
        std::strcat(cmd, "=");
    }
    std::string msg;
    if (bad_filename)
    {
        msg = std::string {"Can't find "} + cmd + bad_filename + ", please check " + to_string(mode);
    }
    if (g_first_init)
    {
        // & cmdfiles hasn't finished 1st try
        if (row == 1 && bad_filename)
        {
            driver_set_for_text();
            driver_put_string(0, 0, 15, "Id found the following problems when parsing commands: ");
        }
        if (bad_filename)
        {
            driver_put_string(row++, 0, 7, msg);
        }
        else if (row > 1)
        {
            driver_put_string(++row, 0, 15, "Press Escape to abort, any other key to continue");
            driver_move_cursor(row+1, 0);
            do_pause(2);  // defer getakeynohelp until after parsing
        }
    }
    else if (bad_filename)
    {
        stop_msg(msg);
    }
    return 0;
}

// Crude function to detect a floating point number. Intended for
// use with arbitrary precision.
static bool is_a_big_float(const char *str)
{
    // [+|-]numbers][.]numbers[+|-][e|g]numbers
    bool result = true;
    const char *s = str;
    int num_dot{};
    int num_e{};
    int num_sign{};
    while (*s != 0 && *s != '/' && *s != ' ')
    {
        if (*s == '-' || *s == '+')
        {
            num_sign++;
        }
        else if (*s == '.')
        {
            num_dot++;
        }
        else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G')
        {
            num_e++;
        }
        else if (!std::isdigit(*s))
        {
            result = false;
            break;
        }
        s++;
    }
    if (num_dot > 1 || num_sign > 2 || num_e > 1)
    {
        result = false;
    }
    return result;
}
