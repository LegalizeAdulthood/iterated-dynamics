// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/id_main.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "io/load_config.h"
#include "io/loadfile.h"
#include "math/fixed_pt.h"
#include "misc/drivers.h"
#include "misc/id.h"
#include "misc/memory.h"
#include "ui/cmdfiles.h"
#include "ui/do_pause.h"
#include "ui/evolve.h"
#include "ui/framain2.h"
#include "ui/get_3d_params.h"
#include "ui/get_a_filename.h"
#include "ui/get_browse_params.h"
#include "ui/get_cmd_string.h"
#include "ui/get_commands.h"
#include "ui/get_fract_type.h"
#include "ui/get_sound_params.h"
#include "ui/get_toggles.h"
#include "ui/get_toggles2.h"
#include "ui/get_view_params.h"
#include "ui/goodbye.h"
#include "ui/history.h"
#include "ui/id_keys.h"
#include "ui/init_failure.h"
#include "ui/intro.h"
#include "ui/main_menu.h"
#include "ui/main_state.h"
#include "ui/mouse.h"
#include "ui/rotate.h"
#include "ui/select_video_mode.h"
#include "ui/stop_msg.h"
#include "ui/video_mode.h"

#include <config/port.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

VideoInfo g_video_entry{};                                    //
HelpLabels g_help_mode{};                                    //
int g_adapter{};                                              // Video Adapter chosen from list in ...h
std::string g_fractal_search_dir1;                            //
std::string g_fractal_search_dir2;                            //
int g_screen_x_dots{}, g_screen_y_dots{};                     // # of dots on the physical screen
int g_logical_screen_x_offset{}, g_logical_screen_y_offset{}; // physical top left of logical screen
int g_logical_screen_x_dots{}, g_logical_screen_y_dots{};     // # of dots on the logical screen
double g_logical_screen_x_size_dots{}, g_logical_screen_y_size_dots{}; // xdots-1, ydots-1
int g_colors{256};                                                     // maximum colors available
long g_max_iterations{};                                               // try this many iterations
int g_box_count{};                                                     // 0 if no zoom-box yet
int g_zoom_box_rotation{};                                             // zoombox rotation
double g_zoom_box_x{}, g_zoom_box_y{};                                 // topleft of zoombox
double g_zoom_box_width{}, g_zoom_box_height{};                        // zoombox size
double g_zoom_box_skew{};                                              // zoombox shape
FractalType g_fractal_type{FractalType::NO_FRACTAL};                  // if == 0, use Mandelbrot
char g_std_calc_mode{};                                                // '1', '2', 'g', 'b'
long g_l_delta_x{}, g_l_delta_y{};                                     // screen pixel increments
long g_l_delta_x2{}, g_l_delta_y2{};                                   // screen pixel increments
LDouble g_delta_x{}, g_delta_y{};                                         // screen pixel increments
LDouble g_delta_x2{}, g_delta_y2{};                                       // screen pixel increments
long g_l_delta_min{};                                                  // for calcfrac/calcmand
double g_delta_min{};                                                  // same as a double
double g_params[MAX_PARAMS]{};                                         // parameters
double g_potential_params[3]{};                                        // three potential parameters
ConfigStatus g_bad_config{};                                          // 'id.cfg' ok?
bool g_has_inverse{};                                                  //
int g_integer_fractal{};                                               // TRUE if fractal uses integer math
                         // user_xxx is what the user wants, vs what we may be forced to do
char g_user_std_calc_mode{};            //
int g_user_periodicity_value{};         //
long g_user_distance_estimator_value{}; //
bool g_user_float_flag{};               //
bool g_view_window{};                   // false for full screen, true for window
float g_view_reduction{};               // window auto-sizing
bool g_view_crop{};                     // true to crop default coords
float g_final_aspect_ratio{};           // for view shape and rotation
int g_view_x_dots{}, g_view_y_dots{};   // explicit view sizing
bool g_keep_aspect_ratio{};             // true to keep virtual aspect
bool g_z_scroll{};                      // screen/zoombox false fixed, true relaxed
                                        // variables defined by the command line/files processor
bool g_compare_gif{};                   // compare two gif files flag
int g_timed_save{};                     // when doing a timed save
int g_resave_flag{};                    // tells encoder not to incr filename
bool g_started_resaves{};               // but incr on first resave
int g_save_system{};                    // from and for save files
bool g_tab_mode{true};                  // tab display enabled
                                        // for historical reasons (before rotation):
                                        //    top    left  corner of screen is (xxmin,yymax)
                                        //    bottom left  corner of screen is (xx3rd,yy3rd)
                                        //    bottom right corner of screen is (xxmax,yymin)
double g_x_min{}, g_x_max{};            //
double g_y_min{}, g_y_max{};            //
double g_x_3rd{}, g_y_3rd{};            // selected screen corners
long g_l_x_min{}, g_l_x_max{};          //
long g_l_y_min{}, g_l_y_max{};          //
long g_l_x_3rd{}, g_l_y_3rd{};          // integer equivs
double g_save_x_min{}, g_save_x_max{};  //
double g_save_y_min{}, g_save_y_max{};  //
double g_save_x_3rd{}, g_save_y_3rd{};  // displayed screen corners
double g_plot_mx1{}, g_plot_mx2{};      //
double g_plot_my1{}, g_plot_my2{};      // real->screen multipliers
CalcStatus g_calc_status{CalcStatus::NO_FRACTAL}; //
long g_calc_time{};                                             //
bool g_zoom_enabled{};                                          // true when zoom is enabled
int g_save_dac{};                                               // save-the-Video DAC flag
bool g_browsing{};                                              // browse mode flag
std::string g_file_name_stack[16]{};                            // array of file names used while browsing
int g_filename_stack_index{};                                   //
double g_smallest_window_display_size{};                        //
int g_smallest_box_size_shown{};                                //
bool g_browse_sub_images{true};                                 //
bool g_auto_browse{};                                           //
bool g_confirm_file_deletes{};                                  //
bool g_browse_check_fractal_params{};                           //
bool g_browse_check_fractal_type{};                             //
std::string g_browse_mask;                                      //
int g_scale_map[12]{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // array for mapping notes to a (user defined) scale

// Do nothing if math error
static void my_floating_point_err(int sig)
{
    if (sig != 0)
    {
        g_overflow = true;
    }
}

/*
; ****************** Function initasmvars() *****************************
*/
static void init_asm_vars()
{
    g_overflow = false;
}

static void bad_id_cfg_msg()
{
    stop_msg("File id.cfg is missing or invalid.\n"
            "See Hardware Support and Video Modes in the full documentation for help.\n"
            "I will continue with only the built-in video modes available.");
    g_bad_config = ConfigStatus::BAD_WITH_MESSAGE;
}

static void main_restart(int const argc, char const *const argv[], MainContext &context)
{
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif
    g_auto_browse = false;
    g_browse_check_fractal_type = false;
    g_browse_check_fractal_params = true;
    g_confirm_file_deletes = true;
    g_browse_sub_images = true;
    g_smallest_window_display_size = 6;
    g_smallest_box_size_shown = 3;
    g_browse_mask = "*.gif";
    g_browse_name = "            ";
    g_filename_stack_index = -1; // init loaded files stack

    g_evolving = EvolutionModeFlags::NONE;
    g_evolve_x_parameter_range = 4;
    g_evolve_new_x_parameter_offset = -2.0;
    g_evolve_x_parameter_offset = -2.0;
    g_evolve_y_parameter_range = 3;
    g_evolve_new_y_parameter_offset = -1.5;
    g_evolve_y_parameter_offset = -1.5;
    g_evolve_discrete_y_parameter_offset = 0;
    g_evolve_discrete_x_parameter_offset = 0;
    g_evolve_image_grid_size = 9;
    g_evolve_max_random_mutation = 1;
    g_evolve_mutation_reduction_factor = 1.0;
    g_evolve_this_generation_random_seed = (unsigned int)std::clock();
    std::srand(g_evolve_this_generation_random_seed);
    init_gene(); /*initialise pointers to lots of variables for the evolution engine*/
    g_start_show_orbit = false;
    g_show_dot = -1; // turn off show_dot if entered with <g> command
    g_calc_status = CalcStatus::NO_FRACTAL;                    // no active fractal image

    driver_create_window();
    std::memcpy(g_old_dac_box, g_dac_box, 256 * 3); // save in case colors= present
    driver_set_for_text();                          // switch to text mode
    g_save_dac = 0;                                 // don't save the VGA DAC

    cmd_files(argc, argv);         // process the command-line
    do_pause(0);                  // pause for error msg if not batch
    init_msg("", nullptr, CmdFile::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    history_init();

    if (g_bad_config == ConfigStatus::BAD_NO_MESSAGE)
    {
        bad_id_cfg_msg();
    }

    g_max_keyboard_check_interval = 80;                  // check the keyboard this often

    if (g_show_file && g_init_mode < 0)
    {
        intro();                          // display the credits screen
        if (driver_key_pressed() == ID_KEY_ESC)
        {
            driver_get_key();
            goodbye();
        }
    }

    g_browsing = false;

    if (!g_new_bifurcation_functions_loaded)
    {
        set_if_old_bif();
    }
    context.stacked = false;
}

static bool main_restore_start(MainContext &context)
{
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif
    if (g_colors_preloaded)
    {
        memcpy(g_dac_box, g_old_dac_box, 256 * 3); // restore in case colors= present
    }
    g_look_at_mouse = +MouseLook::IGNORE_MOUSE;
    while (g_show_file <= 0) // image is to be loaded
    {
        g_tab_mode = false;
        if (!g_browsing) /*RB*/
        {
            char const *hdg;
            if (g_overlay_3d)
            {
                hdg = "Select File for 3D Overlay";
                g_help_mode = HelpLabels::HELP_3D_OVERLAY;
            }
            else if (g_display_3d != Display3DMode::NONE)
            {
                hdg = "Select File for 3D Transform";
                g_help_mode = HelpLabels::HELP_3D;
            }
            else
            {
                hdg = "Select File to Restore";
                g_help_mode = HelpLabels::HELP_SAVE_RESTORE;
            }
            if (g_show_file < 0 && get_a_file_name(hdg, g_gif_filename_mask.c_str(), g_read_filename))
            {
                g_show_file = 1; // cancelled
                g_init_mode = -1;
                break;
            }

            g_filename_stack_index = 0; // 'r' reads first filename for browsing
            g_file_name_stack[g_filename_stack_index] = g_browse_name;
        }

        g_evolving = EvolutionModeFlags::NONE;
        g_view_window = false;
        g_show_file = 0;
        g_help_mode = HelpLabels::NONE;
        g_tab_mode = true;
        if (context.stacked)
        {
            driver_discard_screen();
            driver_set_for_text();
            context.stacked = false;
        }
        if (read_overlay() == 0) // read hdr, get video mode
        {
            break; // got it, exit
        }
        if (g_browsing) // break out of infinite loop, but lose your mind
        {
            g_show_file = 1;
        }
        else
        {
            g_show_file = -1; // retry
        }
    }
    g_help_mode = HelpLabels::HELP_MENU; // now use this help mode
    g_tab_mode = true;
    g_look_at_mouse = +MouseLook::IGNORE_MOUSE;
    if (((g_overlay_3d && (g_init_batch == BatchMode::NONE)) || context.stacked) &&
        g_init_mode < 0) // overlay command failed
    {
        driver_unstack_screen(); // restore the graphics screen
        context.stacked = false;
        g_overlay_3d = false; // forget overlays
        g_display_3d = Display3DMode::NONE;
        if (g_calc_status == CalcStatus::NON_RESUMABLE)
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;
        }
        context.resume = true;
        return true;
    }
    g_save_dac = 0; // don't save the VGA DAC
    return false;
}

static MainState main_image_start(bool &stacked, bool &resume_flag)
{
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif

    if (stacked)
    {
        driver_discard_screen();
        stacked = false;
    }
    g_got_status = StatusValues::NONE;                     // for tab_display

    if (g_show_file)
    {
        // goto image_start implies re-calc
        g_calc_status = std::min(g_calc_status, CalcStatus::PARAMS_CHANGED);
    }

    if (g_init_batch == BatchMode::NONE)
    {
        g_look_at_mouse = -ID_KEY_PAGE_UP;           // just mouse left button, == pgup
    }

    g_cycle_limit = g_init_cycle_limit;         // default cycle limit
    g_adapter = g_init_mode;                  // set the video adapter up
    g_init_mode = -1;                       // (once)

    while (g_adapter < 0)                // cycle through instructions
    {
        if (g_init_batch != BatchMode::NONE)                          // batch, nothing to do
        {
            g_init_batch = BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE;                 // exit with error condition set
            goodbye();
        }
        int key = main_menu(false);
        if (key == ID_KEY_INSERT)
        {
            return MainState::RESTART;      // restart pgm on Insert Key
        }
        if (key == ID_KEY_DELETE)                      // select video mode list
        {
            key = select_video_mode(-1);
        }
        g_adapter = check_vid_mode_key(key);
        if (g_adapter >= 0)
        {
            break;                                 // got a video mode now
        }
        key = std::tolower(key);
        if (key == 'd')
        {
            // shell to DOS
            driver_set_clear();
            driver_shell();
            return MainState::IMAGE_START;
        }

        if (key == '@' || key == '2')
        {
            // execute commands
            if (!bit_set(get_commands(), CmdArgFlags::YES_3D))
            {
                return MainState::IMAGE_START;
            }
            key = '3';                         // 3d=y so fall thru '3' code
        }
        if (key == 'r' || key == '3' || key == '#')
        {
            g_display_3d = Display3DMode::NONE;
            if (key == '3' || key == '#' || key == ID_KEY_F3)
            {
                g_display_3d = Display3DMode::YES;
            }
            if (g_colors_preloaded)
            {
                std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save in case colors= present
            }
            driver_set_for_text(); // switch to text mode
            g_show_file = -1;
            return MainState::RESTORE_START;
        }
        if (key == 't')
        {
            // set fractal type
            g_julibrot = false;
            get_fract_type();
            return MainState::IMAGE_START;
        }
        if (key == 'x')
        {
            // generic toggle switch
            get_toggles();
            return MainState::IMAGE_START;
        }
        if (key == 'y')
        {
            // generic toggle switch
            get_toggles2();
            return MainState::IMAGE_START;
        }
        if (key == 'z')
        {
            // type specific params
            get_fract_params(true);
            return MainState::IMAGE_START;
        }
        if (key == 'v')
        {
            // view parameters
            get_view_params();
            return MainState::IMAGE_START;
        }
        if (key == ID_KEY_CTL_B)
        {
            /* ctrl B = browse params*/
            get_browse_params();
            return MainState::IMAGE_START;
        }
        if (key == ID_KEY_CTL_F)
        {
            /* ctrl f = sound params*/
            get_sound_params();
            return MainState::IMAGE_START;
        }
        if (key == 'f')
        {
            // floating pt toggle
            g_user_float_flag = !g_user_float_flag;
            return MainState::IMAGE_START;
        }
        if (key == 'i')
        {
            // set 3d fractal params
            get_fract3d_params(); // get the parameters
            return MainState::IMAGE_START;
        }
        if (key == 'g')
        {
            get_cmd_string(); // get command string
            return MainState::IMAGE_START;
        }
    }

    g_zoom_enabled = true;
    g_help_mode = HelpLabels::HELP_MAIN; // now use this help mode
    resume_flag = false;                   // allows taking goto inside big_while_loop()

    return MainState::CONTINUE;
}

static MainState main_image_start(MainContext &context)
{
    return main_image_start(context.stacked, context.resume);
}

static void set_search_dirs()
{
    const char *fract_dir = getenv("FRACTDIR");
    if (fract_dir == nullptr)
    {
        fract_dir = ".";
    }
    g_fractal_search_dir1 = fract_dir;
#ifdef SRCDIR
    g_fractal_search_dir2 = SRCDIR;
#else
    g_fractal_search_dir2 = ".";
#endif
}

int id_main(int argc, char *argv[])
{
    set_search_dirs();

    // this traps non-math library floating point errors
    std::signal(SIGFPE, my_floating_point_err);

    init_asm_vars();                       // initialize ASM stuff
    init_memory();

    // let drivers add their video modes
    if (! init_drivers(&argc, argv))
    {
        init_failure("Sorry, I couldn't find any working video drivers for your system\n");
        exit(-1);
    }
    // load id.cfg, match against driver supplied modes
    load_config();
    init_help();

    MainState state{MainState::RESTART};
    MainContext context;
    bool done{};
    while (!done)
    {
#if defined(_WIN32)
        assert(_CrtCheckMemory());
#endif
        switch (state)
        {
        case MainState::RESTART:
            // insert key re-starts here
            main_restart(argc, argv, context);
            state = MainState::RESTORE_START;
            break;

        case MainState::RESTORE_START:
            state = main_restore_start(context) ? MainState::RESUME_LOOP : MainState::IMAGE_START;
            break;

        case MainState::IMAGE_START:
            state = main_image_start(context);
            if (state != MainState::RESTORE_START && state != MainState::IMAGE_START &&
                state != MainState::RESTART)
            {
                state = MainState::RESUME_LOOP;
            }
            break;

        case MainState::RESUME_LOOP:
            save_param_history();
            state = big_while_loop(context);
            break;

        case MainState::CONTINUE:
        case MainState::NOTHING:
            done = true;
            break;
        }
    }

    return 0;
}