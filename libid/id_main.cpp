#include "id_main.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "do_pause.h"
#include "drivers.h"
#include "evolve.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "framain2.h"
#include "get_3d_params.h"
#include "get_a_filename.h"
#include "get_browse_params.h"
#include "get_cmd_string.h"
#include "get_commands.h"
#include "get_fract_type.h"
#include "get_sound_params.h"
#include "get_toggles.h"
#include "get_toggles2.h"
#include "get_view_params.h"
#include "goodbye.h"
#include "helpdefs.h"
#include "history.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "intro.h"
#include "jb.h"
#include "loadfile.h"
#include "load_config.h"
#include "main_menu.h"
#include "main_state.h"
#include "memory.h"
#include "rotate.h"
#include "select_video_mode.h"
#include "stop_msg.h"
#include "video_mode.h"

#include <cctype>
#include <csignal>
#include <cstring>
#include <ctime>
#include <helpcom.h>
#include <init_failure.h>
#include <string>

VIDEOINFO g_video_entry;
help_labels g_help_mode;

int g_look_at_mouse = 0;  // see notes at mouseread routine

int     g_adapter;                      // Video Adapter chosen from list in ...h
std::string g_fractal_search_dir1;
std::string g_fractal_search_dir2;

/*
   the following variables are out here only so
   that the calcfract() and assembler routines can get at them easily
*/
int g_dot_mode;                                                    // video access method
int textsafe2;                                                     // textsafe override from g_video_table
int g_screen_x_dots, g_screen_y_dots;                              // # of dots on the physical screen
int g_logical_screen_x_offset, g_logical_screen_y_offset;          // physical top left of logical screen
int g_logical_screen_x_dots, g_logical_screen_y_dots;              // # of dots on the logical screen
double g_logical_screen_x_size_dots, g_logical_screen_y_size_dots; // xdots-1, ydots-1
int g_colors = 256;                                                // maximum colors available
long g_max_iterations;                                             // try this many iterations
int g_box_count;                                                   // 0 if no zoom-box yet
int g_zoom_box_rotation;                                           // zoombox rotation
double g_zoom_box_x, g_zoom_box_y;                                 // topleft of zoombox
double g_zoom_box_width, g_zoom_box_height;                        // zoombox size
double g_zoom_box_skew;                                            // zoombox shape
fractal_type g_fractal_type;                                       // if == 0, use Mandelbrot
char g_std_calc_mode;                                              // '1', '2', 'g', 'b'
long g_l_delta_x, g_l_delta_y;                                     // screen pixel increments
long g_l_delta_x2, g_l_delta_y2;                                   // screen pixel increments
LDBL g_delta_x, g_delta_y;                                         // screen pixel increments
LDBL g_delta_x2, g_delta_y2;                                       // screen pixel increments
long g_l_delta_min;                                                // for calcfrac/calcmand
double g_delta_min;                                                // same as a double
double g_params[MAX_PARAMS];                                       // parameters
double g_potential_params[3];                                      // three potential parameters

config_status     g_bad_config{};          // 'id.cfg' ok?
bool g_has_inverse = false;
int     g_integer_fractal;         // TRUE if fractal uses integer math

// usr_xxx is what the user wants, vs what we may be forced to do
char    g_user_std_calc_mode;
int     g_user_periodicity_value;
long    g_user_distance_estimator_value;
bool    g_user_float_flag;

bool    g_view_window = false;     // false for full screen, true for window
float   g_view_reduction;          // window auto-sizing
bool    g_view_crop = false;       // true to crop default coords
float   g_final_aspect_ratio;       // for view shape and rotation
int     g_view_x_dots, g_view_y_dots;    // explicit view sizing
bool    g_keep_aspect_ratio = false;  // true to keep virtual aspect
bool    g_z_scroll = false;        // screen/zoombox false fixed, true relaxed

// variables defined by the command line/files processor
bool    g_compare_gif = false;             // compare two gif files flag
int     g_timed_save = 0;                    // when doing a timed save
int     g_resave_flag = 0;                  // tells encoder not to incr filename
bool    g_started_resaves = false;        // but incr on first resave
int     g_save_system;                    // from and for save files
bool    g_tab_mode = true;                 // tab display enabled

// for historical reasons (before rotation):
//    top    left  corner of screen is (xxmin,yymax)
//    bottom left  corner of screen is (xx3rd,yy3rd)
//    bottom right corner of screen is (xxmax,yymin)
double  g_x_min, g_x_max, g_y_min, g_y_max, g_x_3rd, g_y_3rd; // selected screen corners
long    g_l_x_min, g_l_x_max, g_l_y_min, g_l_y_max, g_l_x_3rd, g_l_y_3rd;  // integer equivs
double  g_save_x_min, g_save_x_max, g_save_y_min, g_save_y_max, g_save_x_3rd, g_save_y_3rd; // displayed screen corners
double  g_plot_mx1, g_plot_mx2, g_plot_my1, g_plot_my2;     // real->screen multipliers

calc_status_value g_calc_status = calc_status_value::NO_FRACTAL;
// -1 no fractal
//  0 parms changed, recalc reqd
//  1 actively calculating
//  2 interrupted, resumable
//  3 interrupted, not resumable
//  4 completed
long g_calc_time;

bool g_zoom_off = false;                   // false when zoom is disabled
int        g_save_dac;                     // save-the-Video DAC flag
bool g_browsing = false;                  // browse mode flag
std::string g_file_name_stack[16];        // array of file names used while browsing
int g_filename_stack_index ;
double g_smallest_window_display_size;
int g_smallest_box_size_shown;
bool g_browse_sub_images = true;
bool g_auto_browse = false;
bool g_confirm_file_deletes = false;
bool g_browse_check_fractal_params = false;
bool g_browse_check_fractal_type = false;
std::string g_browse_mask;
int g_scale_map[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // array for mapping notes to a (user defined) scale


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
static void initasmvars()
{
    g_overflow = false;
}

static void bad_id_cfg_msg()
{
    stopmsg(STOPMSG_NONE,
            "File id.cfg is missing or invalid.\n"
            "See Hardware Support and Video Modes in the full documentation for help.\n"
            "I will continue with only the built-in video modes available.");
    g_bad_config = config_status::BAD_WITH_MESSAGE;
}

static void main_restart(int const argc, char const *const argv[], bool &stacked)
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

    g_evolving = 0;
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
    srand(g_evolve_this_generation_random_seed);
    initgene(); /*initialise pointers to lots of variables for the evolution engine*/
    g_start_show_orbit = false;
    g_show_dot = -1; // turn off show_dot if entered with <g> command
    g_calc_status = calc_status_value::NO_FRACTAL;                    // no active fractal image

    cmdfiles(argc, argv);         // process the command-line
    dopause(0);                  // pause for error msg if not batch
    init_msg("", nullptr, cmd_file::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    history_init();

    driver_create_window();
    std::memcpy(g_old_dac_box, g_dac_box, 256*3);      // save in case colors= present

    driver_set_for_text();                      // switch to text mode
    g_save_dac = 0;                         // don't save the VGA DAC

    if (g_bad_config == config_status::BAD_NO_MESSAGE)
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
    stacked = false;
}

static bool main_restore_start(bool &stacked, bool &resumeflag)
{
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif

    if (g_colors_preloaded)
    {
        std::memcpy(g_dac_box, g_old_dac_box, 256*3);   // restore in case colors= present
    }

    g_look_at_mouse = 0;                     // ignore mouse

    while (g_show_file <= 0)              // image is to be loaded
    {
        char const *hdg;
        g_tab_mode = false;
        if (!g_browsing)      /*RB*/
        {
            if (g_overlay_3d)
            {
                hdg = "Select File for 3D Overlay";
                g_help_mode = help_labels::HELP_3D_OVERLAY;
            }
            else if (g_display_3d != display_3d_modes::NONE)
            {
                hdg = "Select File for 3D Transform";
                g_help_mode = help_labels::HELP_3D;
            }
            else
            {
                hdg = "Select File to Restore";
                g_help_mode = help_labels::HELP_SAVEREST;
            }
            if (g_show_file < 0 && getafilename(hdg, g_gif_filename_mask.c_str(), g_read_filename))
            {
                g_show_file = 1;               // cancelled
                g_init_mode = -1;
                break;
            }

            g_filename_stack_index = 0; // 'r' reads first filename for browsing
            g_file_name_stack[g_filename_stack_index] = g_browse_name;
        }

        g_evolving = 0;
        g_view_window = false;
        g_show_file = 0;
        g_help_mode = help_labels::NONE;
        g_tab_mode = true;
        if (stacked)
        {
            driver_discard_screen();
            driver_set_for_text();
            stacked = false;
        }
        if (read_overlay() == 0)       // read hdr, get video mode
        {
            break;                      // got it, exit
        }
        if (g_browsing) // break out of infinite loop, but lose your mind
        {
            g_show_file = 1;
        }
        else
        {
            g_show_file = -1;                 // retry
        }
    }

    g_help_mode = help_labels::HELP_MENU;                 // now use this help mode
    g_tab_mode = true;
    g_look_at_mouse = 0;                     // ignore mouse

    if (((g_overlay_3d && (g_init_batch == batch_modes::NONE)) || stacked) && g_init_mode < 0)        // overlay command failed
    {
        driver_unstack_screen();                  // restore the graphics screen
        stacked = false;
        g_overlay_3d = false;              // forget overlays
        g_display_3d = display_3d_modes::NONE;
        if (g_calc_status == calc_status_value::NON_RESUMABLE)
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        resumeflag = true;
        return true;
    }

    g_save_dac = 0;                         // don't save the VGA DAC

    return false;
}

static main_state main_image_start(bool &stacked, bool &resumeflag)
{
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif

    if (stacked)
    {
        driver_discard_screen();
        stacked = false;
    }
    g_got_status = -1;                     // for tab_display

    if (g_show_file)
    {
        if (g_calc_status > calc_status_value::PARAMS_CHANGED)                // goto imagestart implies re-calc
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
    }

    if (g_init_batch == batch_modes::NONE)
    {
        g_look_at_mouse = -ID_KEY_PAGE_UP;           // just mouse left button, == pgup
    }

    g_cycle_limit = g_init_cycle_limit;         // default cycle limit
    g_adapter = g_init_mode;                  // set the video adapter up
    g_init_mode = -1;                       // (once)

    while (g_adapter < 0)                // cycle through instructions
    {
        if (g_init_batch != batch_modes::NONE)                          // batch, nothing to do
        {
            g_init_batch = batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE;                 // exit with error condition set
            goodbye();
        }
        int kbdchar = main_menu(false);
        if (kbdchar == ID_KEY_INSERT)
        {
            return main_state::RESTART;      // restart pgm on Insert Key
        }
        if (kbdchar == ID_KEY_DELETE)                      // select video mode list
        {
            kbdchar = select_video_mode(-1);
        }
        g_adapter = check_vidmode_key(0, kbdchar);
        if (g_adapter >= 0)
        {
            break;                                 // got a video mode now
        }
        if ('A' <= kbdchar && kbdchar <= 'Z')
        {
            kbdchar = std::tolower(kbdchar);
        }
        if (kbdchar == 'd')
        {
            // shell to DOS
            driver_set_clear();
            driver_shell();
            return main_state::IMAGE_START;
        }

        if (kbdchar == '@' || kbdchar == '2')
        {
            // execute commands
            if ((get_commands() & CMDARG_3D_YES) == 0)
            {
                return main_state::IMAGE_START;
            }
            kbdchar = '3';                         // 3d=y so fall thru '3' code
        }
        if (kbdchar == 'r' || kbdchar == '3' || kbdchar == '#')
        {
            g_display_3d = display_3d_modes::NONE;
            if (kbdchar == '3' || kbdchar == '#' || kbdchar == ID_KEY_F3)
            {
                g_display_3d = display_3d_modes::YES;
            }
            if (g_colors_preloaded)
            {
                std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save in case colors= present
            }
            driver_set_for_text(); // switch to text mode
            g_show_file = -1;
            return main_state::RESTORE_START;
        }
        if (kbdchar == 't')
        {
            // set fractal type
            g_julibrot = false;
            get_fracttype();
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'x')
        {
            // generic toggle switch
            get_toggles();
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'y')
        {
            // generic toggle switch
            get_toggles2();
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'z')
        {
            // type specific parms
            get_fract_params(true);
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'v')
        {
            // view parameters
            get_view_params();
            return main_state::IMAGE_START;
        }
        if (kbdchar == ID_KEY_CTL_B)
        {
            /* ctrl B = browse parms*/
            get_browse_params();
            return main_state::IMAGE_START;
        }
        if (kbdchar == ID_KEY_CTL_F)
        {
            /* ctrl f = sound parms*/
            get_sound_params();
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'f')
        {
            // floating pt toggle
            g_user_float_flag = !g_user_float_flag;
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'i')
        {
            // set 3d fractal parms
            get_fract3d_params(); // get the parameters
            return main_state::IMAGE_START;
        }
        if (kbdchar == 'g')
        {
            get_cmd_string(); // get command string
            return main_state::IMAGE_START;
        }
    }

    g_zoom_off = true;                     // zooming is enabled
    g_help_mode = help_labels::HELP_MAIN;                // now use this help mode
    resumeflag = false;                 // allows taking goto inside big_while_loop()

    return main_state::CONTINUE;
}

int id_main(int argc, char *argv[])
{
    {
        const char *fract_dir = getenv("FRACTDIR");
        g_fractal_search_dir1 = fract_dir == nullptr ? "." : fract_dir;
    }
#ifdef SRCDIR
    g_fractal_search_dir2 = SRCDIR;
#else
    g_fractal_search_dir2 = ".";
#endif

    bool resumeflag = false;
    bool kbdmore = false;               // continuation variable
    bool stacked = false;               // flag to indicate screen stacked

    // this traps non-math library floating point errors
    std::signal(SIGFPE, my_floating_point_err);

    initasmvars();                       // initialize ASM stuff
    InitMemory();

    // let drivers add their video modes
    if (! init_drivers(&argc, argv))
    {
        init_failure("Sorry, I couldn't find any working video drivers for your system\n");
        exit(-1);
    }
    // load id.cfg, match against driver supplied modes
    load_config();
    init_help();

restart:   // insert key re-starts here
    main_restart(argc, argv, stacked);

restorestart:
    if (main_restore_start(stacked, resumeflag))
    {
        goto resumeloop;                // ooh, this is ugly
    }

imagestart:                             // calc/display a new image
    switch (main_image_start(stacked, resumeflag))
    {
    case main_state::RESTORE_START:
        goto restorestart;

    case main_state::IMAGE_START:
        goto imagestart;

    case main_state::RESTART:
        goto restart;

    default:
        break;
    }

resumeloop:
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif
    save_param_history();
    // this switch processes gotos that are now inside function
    switch (big_while_loop(&kbdmore, &stacked, resumeflag))
    {
    case main_state::RESTART:
        goto restart;

    case main_state::IMAGE_START:
        goto imagestart;

    case main_state::RESTORE_START:
        goto restorestart;

    default:
        break;
    }

    return 0;
}
