// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/framain2.h"

#include "3d/line3d.h"
#include "engine/calc_frac_init.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "io/decoder.h"
#include "io/dir_file.h"
#include "io/gifview.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "misc/debug_flags.h"
#include "misc/drivers.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/evolver_menu_switch.h"
#include "ui/get_fract_type.h"
#include "ui/goodbye.h"
#include "ui/history.h"
#include "ui/id_keys.h"
#include "ui/main_menu.h"
#include "ui/main_menu_switch.h"
#include "ui/mouse.h"
#include "ui/read_ticker.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"
#include "ui/video_mode.h"
#include "ui/zoom.h"

#include <config/port.h>

#include <array> // std::size
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

static int call_line3d(Byte *pixels, int line_len);
static void cmp_line_cleanup();
static int cmp_line(Byte *pixels, int line_len);

static long s_save_base{}; // base clock ticks
static long s_save_ticks{};  // save after this many ticks
static std::FILE *s_cmp_fp{};
static int s_err_count{};

bool g_from_text{}; // = true if we're in graphics mode
int g_finish_row{}; // save when this row is finished
EvolutionInfo g_evolve_info{};
bool g_have_evolve_info{};
char g_old_std_calc_mode{};
void (*g_out_line_cleanup)(){};
bool g_virtual_screens{};

static int iround(double value)
{
    return static_cast<int>(std::lround(value));
}

MainState big_while_loop(MainContext &context)
{
    int     i = 0;                           // temporary loop counters
    MainState mms_value;

#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif
    context.from_mandel = false;            // if julia entered from mandel
    if (context.resume)
    {
        goto resumeloop;
    }

    while (true)                    // eternal loop
    {
#if defined(_WIN32)
        _ASSERTE(_CrtCheckMemory());
#endif

        if (g_calc_status != CalcStatus::RESUMABLE || g_show_file == 0)
        {
            std::memcpy((char *)&g_video_entry, (char *)&g_video_table[g_adapter],
                   sizeof(g_video_entry));
            g_logical_screen_x_dots   = g_video_entry.x_dots;       // # dots across the screen
            g_logical_screen_y_dots   = g_video_entry.y_dots;       // # dots down the screen
            g_colors  = g_video_entry.colors;      // # colors available
            g_screen_x_dots  = g_logical_screen_x_dots;
            g_screen_y_dots  = g_logical_screen_y_dots;
            g_logical_screen_y_offset = 0;
            g_logical_screen_x_offset = 0;
            g_color_cycle_range_hi = (g_color_cycle_range_hi < g_colors) ? g_color_cycle_range_hi : g_colors - 1;

            std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC

            if (g_overlay_3d && (g_init_batch == BatchMode::NONE))
            {
                driver_unstack_screen();            // restore old graphics image
                g_overlay_3d = false;
            }
            else
            {
                driver_set_video_mode(&g_video_entry); // switch video modes
                // switching video modes may have changed drivers or disk flag...
                if (!g_good_mode)
                {
                    if (driver_is_disk())
                    {
                        g_ask_video = true;
                    }
                    else
                    {
                        stop_msg("That video mode is not available with your adapter.");
                        g_ask_video = true;
                    }
                    g_init_mode = -1;
                    driver_set_for_text(); // switch to text mode
                    return MainState::RESTORE_START;
                }

                if (g_virtual_screens && (g_logical_screen_x_dots > g_screen_x_dots || g_logical_screen_y_dots > g_screen_y_dots))
                {
#define MSG_XY1 "Can't set virtual line that long, width cut down."
#define MSG_XY2 "Not enough video memory for that many lines, height cut down."
                    if (g_logical_screen_x_dots > g_screen_x_dots && g_logical_screen_y_dots > g_screen_y_dots)
                    {
                        stop_msg(MSG_XY1 "\n" MSG_XY2);
                    }
                    else if (g_logical_screen_y_dots > g_screen_y_dots)
                    {
                        stop_msg(MSG_XY2);
                    }
                    else
                    {
                        stop_msg(MSG_XY1);
                    }
#undef MSG_XY1
#undef MSG_XY2
                }
                g_logical_screen_x_dots = g_screen_x_dots;
                g_logical_screen_y_dots = g_screen_y_dots;
                g_video_entry.x_dots = g_logical_screen_x_dots;
                g_video_entry.y_dots = g_logical_screen_y_dots;
            }

            if (g_save_dac || g_colors_preloaded)
            {
                std::memcpy(g_dac_box, g_old_dac_box, 256*3); // restore the DAC
                spin_dac(0, 1);
                g_colors_preloaded = false;
            }
            else
            {
                // reset DAC to defaults, which setvideomode has done for us
                if (g_map_specified)
                {
                    // but there's a map=, so load that
                    for (int j = 0; j < 256; ++j)
                    {
                        g_dac_box[j][0] = g_map_clut[j][0];
                        g_dac_box[j][1] = g_map_clut[j][1];
                        g_dac_box[j][2] = g_map_clut[j][2];
                    }
                    spin_dac(0, 1);
                }
                else if ((driver_is_disk() && g_colors == 256) || !g_colors)
                {
                    // disk video, setvideomode via bios didn't get it right, so:
                    validate_luts("default"); // read the default palette file
                }
                g_color_state = ColorState::DEFAULT;
            }
            if (g_view_window)
            {
                // bypass for VESA virtual screen
                const double f_temp{g_final_aspect_ratio *
                    (((double) g_screen_y_dots) / ((double) g_screen_x_dots) / g_screen_aspect)};
                g_logical_screen_x_dots = g_view_x_dots;
                if (g_logical_screen_x_dots != 0)
                {
                    // xdots specified
                    g_logical_screen_y_dots = g_view_y_dots;
                    if (g_logical_screen_y_dots == 0) // calc ydots?
                    {
                        g_logical_screen_y_dots = iround(g_logical_screen_x_dots * f_temp);
                    }
                }
                else if (g_final_aspect_ratio <= g_screen_aspect)
                {
                    g_logical_screen_x_dots = iround((double) g_screen_x_dots / g_view_reduction);
                    g_logical_screen_y_dots = iround(g_logical_screen_x_dots * f_temp);
                }
                else
                {
                    g_logical_screen_y_dots = iround((double) g_screen_y_dots / g_view_reduction);
                    g_logical_screen_x_dots = iround(g_logical_screen_y_dots / f_temp);
                }
                if (g_logical_screen_x_dots > g_screen_x_dots || g_logical_screen_y_dots > g_screen_y_dots)
                {
                    stop_msg("View window too large; using full screen.");
                    g_view_window = false;
                    g_view_x_dots = g_screen_x_dots;
                    g_logical_screen_x_dots = g_view_x_dots;
                    g_view_y_dots = g_screen_y_dots;
                    g_logical_screen_y_dots = g_view_y_dots;
                }
                // changed test to 1, so a 2x2 window will work with the sound feature
                else if ((g_logical_screen_x_dots <= 1 || g_logical_screen_y_dots <= 1) &&
                    !bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
                {
                    // so ssg works
                    // but no check if in evolve mode to allow lots of small views
                    stop_msg("View window too small; using full screen.");
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP) &&
                    bit_set(g_cur_fractal_specific->flags, FractalFlags::INF_CALC))
                {
                    stop_msg("Fractal doesn't terminate! switching off evolution.");
                    g_evolving ^= EvolutionModeFlags::FIELD_MAP;
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
                {
                    const int grout = bit_set(g_evolving, EvolutionModeFlags::NO_GROUT) ? 0 : 1;
                    g_logical_screen_x_dots = (g_screen_x_dots / g_evolve_image_grid_size) - grout;
                    // trim to multiple of 4 for SSG
                    g_logical_screen_x_dots = g_logical_screen_x_dots - (g_logical_screen_x_dots % 4);
                    g_logical_screen_y_dots = (g_screen_y_dots / g_evolve_image_grid_size) - grout;
                    g_logical_screen_y_dots = g_logical_screen_y_dots - (g_logical_screen_y_dots % 4);
                }
                else
                {
                    g_logical_screen_x_offset = (g_screen_x_dots - g_logical_screen_x_dots) / 2;
                    g_logical_screen_y_offset = (g_screen_y_dots - g_logical_screen_y_dots) / 3;
                }
            }
            g_logical_screen_x_size_dots = g_logical_screen_x_dots - 1;            // convert just once now
            g_logical_screen_y_size_dots = g_logical_screen_y_dots - 1;
        }
        // assume we save next time (except jb)
        g_save_dac = (g_save_dac == 0) ? 2 : 1;
        if (g_init_batch == BatchMode::NONE)
        {
            g_look_at_mouse = -ID_KEY_PAGE_UP;        // mouse left button == pgup
        }

        if (g_show_file == 0)
        {
            // loading an image
            g_out_line_cleanup = nullptr;          // outln routine can set this
            if (g_display_3d != Display3DMode::NONE)                 // set up 3D decoding
            {
                g_out_line = call_line3d;
            }
            else if (g_compare_gif)            // debug 50
            {
                g_out_line = cmp_line;
            }
            else if (g_potential_16bit)
            {
                // .pot format input file
                if (pot_start_disk() < 0)
                {
                    // pot file failed?
                    g_show_file = 1;
                    g_potential_flag  = false;
                    g_potential_16bit = false;
                    g_init_mode = -1;
                    g_calc_status = CalcStatus::RESUMABLE;         // "resume" without 16-bit
                    driver_set_for_text();
                    get_fract_type();
                    return MainState::IMAGE_START;
                }
                g_out_line = pot_line;
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP &&
                g_evolving == EvolutionModeFlags::NONE) // regular gif/fra input file
            {
                g_out_line = sound_line;      // sound decoding
            }
            else
            {
                g_out_line = out_line;        // regular decoding
            }
            if (g_debug_flag == DebugFlags::SHOW_FLOAT_FLAG)
            {
                char msg[MSG_LEN];
                std::snprintf(msg, std::size(msg), "floatflag=%d", g_user_float_flag ? 1 : 0);
                stop_msg(StopMsgFlags::NO_BUZZER, msg);
            }
            i = funny_glasses_call(gif_view);
            if (g_out_line_cleanup)              // cleanup routine defined?
            {
                (*g_out_line_cleanup)();
            }
            if (i == 0)
            {
                driver_buzzer(Buzzer::COMPLETE);
            }
            else
            {
                g_calc_status = CalcStatus::NO_FRACTAL;
                if (driver_key_pressed())
                {
                    driver_buzzer(Buzzer::INTERRUPT);
                    while (driver_key_pressed())
                    {
                        driver_get_key();
                    }
                    text_temp_msg("*** load incomplete ***");
                }
            }
        }

        // for these cases disable zooming
        g_zoom_enabled = !driver_is_disk() && !bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_ZOOM);
        if (g_evolving == EvolutionModeFlags::NONE)
        {
            calc_frac_init();
        }
        driver_schedule_alarm(1);

        g_save_x_min = g_x_min; // save 3 corners for zoom.c ref points
        g_save_x_max = g_x_max;
        g_save_x_3rd = g_x_3rd;
        g_save_y_min = g_y_min;
        g_save_y_max = g_y_max;
        g_save_y_3rd = g_y_3rd;

        if (g_bf_math != BFMathType::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_min);
            copy_bf(g_bf_save_x_max, g_bf_x_max);
            copy_bf(g_bf_save_y_min, g_bf_y_min);
            copy_bf(g_bf_save_y_max, g_bf_y_max);
            copy_bf(g_bf_save_x_3rd, g_bf_x_3rd);
            copy_bf(g_bf_save_y_3rd, g_bf_y_3rd);
        }
        save_history_info();

        if (g_show_file == 0)
        {
            // image has been loaded
            g_show_file = 1;
            if (g_init_batch == BatchMode::NORMAL && g_calc_status == CalcStatus::RESUMABLE)
            {
                g_init_batch = BatchMode::FINISH_CALC_BEFORE_SAVE;
            }
            if (g_loaded_3d)      // 'r' of image created with '3'
            {
                g_display_3d = Display3DMode::YES;  // so set flag for 'b' command
            }
        }
        else
        {
            // draw an image
            if (g_init_save_time != 0 // autosave and resumable?
                && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_RESUME))
            {
                s_save_base = read_ticker(); // calc's start time
                s_save_ticks = std::abs(g_init_save_time);
                s_save_ticks *= 1092; // bios ticks/minute
                if ((s_save_ticks & 65535L) == 0)
                {
                    ++s_save_ticks; // make low word nonzero
                }
                g_finish_row = -1;
            }
            g_browsing = false;      // regenerate image, turn off browsing
            //rb
            g_filename_stack_index = -1;   // reset pointer
            g_browse_name.clear();
            if (g_view_window && bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP) &&
                g_calc_status != CalcStatus::COMPLETED)
            {
                // generate a set of images with varied parameters on each one
                int count;
                GeneBase gene[NUM_GENES];
                copy_genes_from_bank(gene);
                if (g_have_evolve_info && (g_calc_status == CalcStatus::RESUMABLE))
                {
                    g_evolve_x_parameter_range = g_evolve_info.x_parameter_range;
                    g_evolve_y_parameter_range = g_evolve_info.y_parameter_range;
                    g_evolve_new_x_parameter_offset = g_evolve_info.x_parameter_offset;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_new_y_parameter_offset = g_evolve_info.y_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_new_discrete_x_parameter_offset = (char)g_evolve_info.discrete_x_parameter_offset;
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_new_discrete_y_parameter_offset = (char)g_evolve_info.discrete_y_parameter_offset;
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset;
                    g_evolve_param_grid_x           = g_evolve_info.px;
                    g_evolve_param_grid_y           = g_evolve_info.py;
                    g_logical_screen_x_offset       = g_evolve_info.screen_x_offset;
                    g_logical_screen_y_offset       = g_evolve_info.screen_y_offset;
                    g_logical_screen_x_dots        = g_evolve_info.x_dots;
                    g_logical_screen_y_dots        = g_evolve_info.y_dots;
                    g_evolve_image_grid_size = g_evolve_info.image_grid_size;
                    g_evolve_this_generation_random_seed = g_evolve_info.this_generation_random_seed;
                    g_evolve_max_random_mutation = g_evolve_info.max_random_mutation;
                    g_evolving = static_cast<EvolutionModeFlags>(g_evolve_info.evolving);
                    g_view_window = g_evolving != EvolutionModeFlags::NONE;
                    count       = g_evolve_info.count;
                    g_have_evolve_info = false;
                }
                else
                {
                    // not resuming, start from the beginning
                    int mid = g_evolve_image_grid_size / 2;
                    if ((g_evolve_param_grid_x != mid) || (g_evolve_param_grid_y != mid))
                    {
                        g_evolve_this_generation_random_seed = (unsigned int)std::clock(); // time for new set
                    }
                    save_param_history();
                    count = 0;
                    g_evolve_max_random_mutation = g_evolve_max_random_mutation * g_evolve_mutation_reduction_factor;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset; // evolve_discrete_x_parameter_offset used for discrete params like inside, outside, trigfn etc
                }
                g_evolve_param_box_count = 0;
                g_evolve_dist_per_x = g_evolve_x_parameter_range /(g_evolve_image_grid_size -1);
                g_evolve_dist_per_y = g_evolve_y_parameter_range /(g_evolve_image_grid_size -1);
                const int grout = bit_set(g_evolving, EvolutionModeFlags::NO_GROUT) ? 0 : 1;
                int tmp_x_dots = g_logical_screen_x_dots + grout;
                int tmp_y_dots = g_logical_screen_y_dots + grout;
                int grid_sqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
                while (count < grid_sqr)
                {
                    spiral_map(count); // sets px & py
                    g_logical_screen_x_offset = tmp_x_dots * g_evolve_param_grid_x;
                    g_logical_screen_y_offset = tmp_y_dots * g_evolve_param_grid_y;
                    restore_param_history();
                    fiddle_params(gene, count);
                    calc_frac_init();
                    if (calc_fract() == -1)
                    {
                        goto done;
                    }
                    count ++;
                }
done:
#if defined(_WIN32)
                _ASSERTE(_CrtCheckMemory());
#endif

                if (count == grid_sqr)
                {
                    i = 0;
                    driver_buzzer(Buzzer::COMPLETE); // finished!!
                }
                else
                {
                    g_evolve_info.x_parameter_range = g_evolve_x_parameter_range;
                    g_evolve_info.y_parameter_range = g_evolve_y_parameter_range;
                    g_evolve_info.x_parameter_offset = g_evolve_x_parameter_offset;
                    g_evolve_info.y_parameter_offset = g_evolve_y_parameter_offset;
                    g_evolve_info.discrete_x_parameter_offset = (short) g_evolve_discrete_x_parameter_offset;
                    g_evolve_info.discrete_y_parameter_offset = (short) g_evolve_discrete_y_parameter_offset;
                    g_evolve_info.px              = (short)g_evolve_param_grid_x;
                    g_evolve_info.py              = (short)g_evolve_param_grid_y;
                    g_evolve_info.screen_x_offset          = (short)g_logical_screen_x_offset;
                    g_evolve_info.screen_y_offset          = (short)g_logical_screen_y_offset;
                    g_evolve_info.x_dots           = (short)g_logical_screen_x_dots;
                    g_evolve_info.y_dots           = (short)g_logical_screen_y_dots;
                    g_evolve_info.image_grid_size = (short) g_evolve_image_grid_size;
                    g_evolve_info.this_generation_random_seed = (short) g_evolve_this_generation_random_seed;
                    g_evolve_info.max_random_mutation = g_evolve_max_random_mutation;
                    g_evolve_info.evolving        = (short) +g_evolving;
                    g_evolve_info.count          = (short) count;
                    g_have_evolve_info = true;
                }
                g_logical_screen_y_offset = 0;
                g_logical_screen_x_offset = g_logical_screen_y_offset;
                g_logical_screen_x_dots = g_screen_x_dots;
                g_logical_screen_y_dots = g_screen_y_dots; // otherwise save only saves a sub image and boxes get clipped

                // set up for 1st selected image, this reuses px and py
                g_evolve_param_grid_y = g_evolve_image_grid_size /2;
                g_evolve_param_grid_x = g_evolve_param_grid_y;
                unspiral_map();    // first time called, w/above line sets up array
                restore_param_history();
                fiddle_params(gene, 0);
                copy_genes_to_bank(gene);
            }
            // end of evolution loop
            else
            {
                i = calc_fract();       // draw the fractal using "C"
                if (i == 0)
                {
                    driver_buzzer(Buzzer::COMPLETE); // finished!!
                }
            }

            s_save_ticks = 0;                 // turn off autosave timer
            if (driver_is_disk() && i == 0) // disk-video
            {
                dvid_status(0, "Image has been completed");
            }
        }
        g_box_count = 0;                     // no zoom box yet
        g_zoom_box_width = 0;

        if (g_fractal_type == FractalType::PLASMA)
        {
            g_cycle_limit = 256;              // plasma clouds need quick spins
            g_dac_count = 256;
            g_dac_learn = true;
        }

resumeloop:                             // return here on failed overlays
#if defined(_WIN32)
        _ASSERTE(_CrtCheckMemory());
#endif
        context.more_keys = true;
        while (context.more_keys)
        {
            // loop through command keys
            if (g_timed_save != 0)
            {
                if (g_timed_save == 1)
                {
                    // woke up for timed save
                    driver_get_key();     // eat the dummy char
                    context.key = 's'; // do the save
                    g_resave_flag = 1;
                    g_timed_save = 2;
                }
                else
                {
                    // save done, resume
                    g_timed_save = 0;
                    g_resave_flag = 2;
                    context.key = ID_KEY_ENTER;
                }
            }
            else if (g_init_batch == BatchMode::NONE)      // not batch mode
            {
                g_look_at_mouse = g_zoom_box_width == 0 ? -ID_KEY_PAGE_UP : +MouseLook::POSITION;
                if (g_calc_status == CalcStatus::RESUMABLE && g_zoom_box_width == 0 && !driver_key_pressed())
                {
                    context.key = ID_KEY_ENTER;  // no visible reason to stop, continue
                }
                else      // wait for a real keystroke
                {
                    if (g_auto_browse && g_browse_sub_images)
                    {
                        context.key = 'l';
                    }
                    else
                    {
                        driver_wait_key_pressed(0);
                        context.key = driver_get_key();
                    }
                    if (context.key == ID_KEY_ESC || context.key == 'm' || context.key == 'M')
                    {
                        if (context.key == ID_KEY_ESC && g_escape_exit)
                        {
                            // don't ask, just get out
                            goodbye();
                        }
                        driver_stack_screen();
                        context.key = main_menu(true);
                        if (context.key == '\\' || context.key == ID_KEY_CTL_BACKSLASH
                            || context.key == 'h' || context.key == ID_KEY_CTL_H
                            || check_vid_mode_key(context.key) >= 0)
                        {
                            driver_discard_screen();
                        }
                        else if (context.key == 'x' || context.key == 'y'
                            || context.key == 'z' || context.key == 'g'
                            || context.key == 'v' || context.key == ID_KEY_CTL_B
                            || context.key == ID_KEY_CTL_E || context.key == ID_KEY_CTL_F)
                        {
                            g_from_text = true;
                        }
                        else
                        {
                            driver_unstack_screen();
                        }
                    }
                }
            }
            else          // batch mode, fake next keystroke
            {
                // clang-format off
                // init_batch == FINISH_CALC_BEFORE_SAVE        flag to finish calc before save
                // init_batch == NONE                           not in batch mode
                // init_batch == NORMAL                         normal batch mode
                // init_batch == SAVE                           was NORMAL, now do a save
                // init_batch == BAILOUT_ERROR_NO_SAVE          bailout with errorlevel == 2, error occurred, no save
                // init_batch == BAILOUT_INTERRUPTED_TRY_SAVE   bailout with errorlevel == 1, interrupted, try to save
                // init_batch == BAILOUT_INTERRUPTED_SAVE       was BAILOUT_INTERRUPTED_TRY_SAVE, now do a save
                // clang-format on

                if (g_init_batch == BatchMode::FINISH_CALC_BEFORE_SAVE)
                {
                    context.key = ID_KEY_ENTER;
                    g_init_batch = BatchMode::NORMAL;
                }
                else if (g_init_batch == BatchMode::NORMAL || g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE)         // save-to-disk
                {
                    context.key = (g_debug_flag == DebugFlags::FORCE_DISK_RESTORE_NOT_SAVE) ? 'r' : 's';
                    if (g_init_batch == BatchMode::NORMAL)
                    {
                        g_init_batch = BatchMode::SAVE;
                    }
                    if (g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE)
                    {
                        g_init_batch = BatchMode::BAILOUT_INTERRUPTED_SAVE;
                    }
                }
                else
                {
                    if (g_calc_status != CalcStatus::COMPLETED)
                    {
                        g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE; // bailout with error
                    }
                    goodbye();               // done, exit
                }
            }

            context.key = std::tolower(context.key);
            if (g_evolving != EvolutionModeFlags::NONE)
            {
                mms_value = evolver_menu_switch(context);
            }
            else
            {
                mms_value = main_menu_switch(context);
            }
            if (g_quick_calc
                && (mms_value == MainState::IMAGE_START
                    || mms_value == MainState::RESTORE_START
                    || mms_value == MainState::RESTART))
            {
                g_quick_calc = false;
                g_user_std_calc_mode = g_old_std_calc_mode;
            }
            if (g_quick_calc && g_calc_status != CalcStatus::COMPLETED)
            {
                g_user_std_calc_mode = '1';
            }
            switch (mms_value)
            {
            case MainState::IMAGE_START:
                return MainState::IMAGE_START;
            case MainState::RESTORE_START:
                return MainState::RESTORE_START;
            case MainState::RESTART:
                return MainState::RESTART;
            case MainState::CONTINUE:
                continue;
            default:
                break;
            }
            if (g_zoom_enabled && context.more_keys) // draw/clear a zoom box?
            {
                draw_box(true);
            }
            if (driver_resize())
            {
                g_calc_status = CalcStatus::NO_FRACTAL;
            }
        }
    }
}

static int call_line3d(Byte *pixels, int line_len)
{
    // this routine exists because line3d might be in an overlay
    return line3d(pixels, line_len);
}

// displays differences between current image file and new image
static int cmp_line(Byte *pixels, int line_len)
{
    int row = g_row_count++;
    if (row == 0)
    {
        s_err_count = 0;
        s_cmp_fp = dir_fopen(g_working_dir.c_str(), "cmperr", (g_init_batch != BatchMode::NONE) ? "a" : "w");
        g_out_line_cleanup = cmp_line_cleanup;
    }
    if (g_potential_16bit)
    {
        // 16 bit info, ignore odd numbered rows
        if ((row & 1) != 0)
        {
            return 0;
        }
        row >>= 1;
    }
    for (int col = 0; col < line_len; col++)
    {
        int old_color = get_color(col, row);
        if (old_color == (int)pixels[col])
        {
            g_put_color(col, row, 0);
        }
        else
        {
            if (old_color == 0)
            {
                g_put_color(col, row, 1);
            }
            ++s_err_count;
            if (g_init_batch == BatchMode::NONE)
            {
                std::fprintf(s_cmp_fp, "#%5d col %3d row %3d old %3d new %3d\n",
                        s_err_count, col, row, old_color, pixels[col]);
            }
        }
    }
    return 0;
}

static void cmp_line_cleanup()
{
    if (g_init_batch != BatchMode::NONE)
    {
        time_t now;
        std::time(&now);
        char *times_text = std::ctime(&now);
        times_text[24] = 0; //clobber newline in time string
        std::fprintf(s_cmp_fp, "%s compare to %s has %5d errs\n",
                times_text, g_read_filename.c_str(), s_err_count);
    }
    std::fclose(s_cmp_fp);
}

// read keystrokes while = specified key, return 1+count;
// used to catch up when moving zoombox is slower than keyboard
int key_count(int key)
{
    int ctr = 1;
    while (driver_key_pressed() == key)
    {
        driver_get_key();
        ++ctr;
    }
    return ctr;
}
