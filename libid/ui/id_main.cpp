// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/id_main.h"

#include "engine/Browse.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/show_dot.h"
#include "engine/Viewport.h"
#include "fractals/fractype.h"
#include "fractals/jb.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "io/library.h"
#include "io/load_config.h"
#include "io/loadfile.h"
#include "io/special_dirs.h"
#include "math/fixed_pt.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/memory.h"
#include "ui/do_pause.h"
#include "ui/evolve.h"
#include "ui/framain2.h"
#include "ui/get_3d_params.h"
#include "ui/get_browse_params.h"
#include "ui/get_cmd_string.h"
#include "ui/get_commands.h"
#include "ui/get_fract_type.h"
#include "ui/get_sound_params.h"
#include "ui/get_toggles.h"
#include "ui/get_toggles2.h"
#include "ui/get_view_params.h"
#include "ui/goodbye.h"
#include "ui/help.h"
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
#include "ui/tab_display.h"
#include "ui/video_mode.h"
#include "ui/zoom.h"

#include <config/home_dir.h>
#include <config/port.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

using namespace id::config;
using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

// Do nothing if math error
static void my_floating_point_err(const int sig)
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

static void main_restart(const int argc, const char *const argv[], MainContext &context)
{
    driver_check_memory();
    g_browse.auto_browse = false;
    g_browse.check_fractal_type = false;
    g_browse.check_fractal_params = true;
    g_browse.confirm_delete = true;
    g_browse.sub_images = true;
    g_browse.smallest_window = 6;
    g_browse.smallest_box = 3;
    g_browse.mask = "*.gif";
    g_browse.name.clear();
    g_browse.stack.clear(); // init loaded files stack

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
    g_evolve_this_generation_random_seed = static_cast<unsigned int>(std::clock());
    std::srand(g_evolve_this_generation_random_seed);
    init_gene(); /*initialise pointers to lots of variables for the evolution engine*/
    g_start_show_orbit = false;
    g_show_dot = -1; // turn off show_dot if entered with <g> command
    g_calc_status = CalcStatus::NO_FRACTAL;                    // no active fractal image

    driver_create_window();
    std::memcpy(g_old_dac_box, g_dac_box, 256 * 3); // save in case colors= present
    driver_set_for_text();                          // switch to text mode
    g_save_dac = SaveDAC::NO;                       // don't save the VGA DAC

    cmd_files(argc, argv);         // process the command-line
    do_pause(0);                  // pause for error msg if not batch
    init_msg("", nullptr, CmdFile::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    history_init();

    if (g_bad_config == ConfigStatus::BAD_NO_MESSAGE)
    {
        bad_id_cfg_msg();
    }

    g_max_keyboard_check_interval = 80;                  // check the keyboard this often

    if (g_show_file != ShowFile::LOAD_IMAGE && g_init_mode < 0)
    {
        intro();                          // display the credits screen
        if (driver_key_pressed() == ID_KEY_ESC)
        {
            driver_get_key();
            goodbye();
        }
    }

    g_browse.browsing = false;

    if (!g_new_bifurcation_functions_loaded)
    {
        set_if_old_bif();
    }
    context.stacked = false;
}

static bool main_restore_start(MainContext &context)
{
    driver_check_memory();
    if (g_colors_preloaded)
    {
        std::memcpy(g_dac_box, g_old_dac_box, 256 * 3); // restore in case colors= present
    }
    g_look_at_mouse = MouseLook::IGNORE_MOUSE;
    while (g_show_file <= ShowFile::LOAD_IMAGE) // image is to be loaded
    {
        g_tab_enabled = false;
        if (!g_browse.browsing) /*RB*/
        {
            const char *hdg;
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
            if (g_show_file == ShowFile::REQUEST_IMAGE &&
                driver_get_filename(hdg, "Image File", g_image_filename_mask.c_str(), g_read_filename))
            {
                g_show_file = ShowFile::IMAGE_LOADED; // cancelled
                g_init_mode = -1;
                break;
            }

            g_browse.stack.clear(); // 'r' reads first filename for browsing
            g_browse.stack.push_back(g_browse.name);
        }

        g_evolving = EvolutionModeFlags::NONE;
        g_viewport.enabled = false;
        g_show_file = ShowFile::LOAD_IMAGE;
        g_help_mode = HelpLabels::NONE;
        g_tab_enabled = true;
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
        if (g_browse.browsing) // break out of infinite loop, but lose your mind
        {
            g_show_file = ShowFile::IMAGE_LOADED;
        }
        else
        {
            g_show_file = ShowFile::REQUEST_IMAGE; // retry
        }
    }
    g_help_mode = HelpLabels::HELP_MENU; // now use this help mode
    g_tab_enabled = true;
    g_look_at_mouse = MouseLook::IGNORE_MOUSE;
    if (((g_overlay_3d && g_init_batch == BatchMode::NONE) || context.stacked) &&
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
    g_save_dac = SaveDAC::NO;
    return false;
}

static MainState main_image_start(MainContext &context)
{
    driver_check_memory();
    if (context.stacked)
    {
        driver_discard_screen();
        context.stacked = false;
    }
    g_passes = Passes::NONE;                     // for tab_display

    if (g_show_file != ShowFile::LOAD_IMAGE)
    {
        // goto image_start implies re-calc
        g_calc_status = std::min(g_calc_status, CalcStatus::PARAMS_CHANGED);
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
            g_show_file = ShowFile::REQUEST_IMAGE;
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
    context.resume = false;              // allows taking goto inside big_while_loop()

    return MainState::CONTINUE;
}

static void set_search_dirs()
{
    const char *fract_dir = getenv("FRACTDIR");
    if (fract_dir == nullptr)
    {
        fract_dir = ".";
    }
    g_fractal_search_dir1 = fract_dir;
    if (std::filesystem::exists(HOME_DIR))
    {
        g_fractal_search_dir2 = HOME_DIR;
    }
    else
    {
        g_fractal_search_dir2 = g_special_dirs->program_dir();
    }
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
        std::exit(-1);
    }
    // load id.cfg, match against driver supplied modes
    load_config();
    init_help();

    MainState state{MainState::RESTART};
    MainContext context;
    bool done{};
    while (!done)
    {
        driver_check_memory();
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

} // namespace id::ui
