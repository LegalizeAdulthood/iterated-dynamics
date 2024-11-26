// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "framain2.h"

#include "calc_frac_init.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "decoder.h"
#include "dir_file.h"
#include "diskvid.h"
#include "drivers.h"
#include "evolve.h"
#include "evolver_menu_switch.h"
#include "fractalp.h"
#include "fractype.h"
#include "get_fract_type.h"
#include "gifview.h"
#include "goodbye.h"
#include "history.h"
#include "id_data.h"
#include "id_keys.h"
#include "line3d.h"
#include "loadfile.h"
#include "loadmap.h"
#include "lorenz.h"
#include "main_menu.h"
#include "main_menu_switch.h"
#include "mouse.h"
#include "os.h"
#include "read_ticker.h"
#include "rotate.h"
#include "spindac.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "video.h"
#include "video_mode.h"
#include "zoom.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

static int call_line3d(BYTE *pixels, int linelen);
static void cmp_line_cleanup();
static int cmp_line(BYTE *pixels, int linelen);

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

main_state big_while_loop(bool *const kbdmore, bool *const stacked, bool const resumeflag)
{
    double  ftemp;                       // fp temp
    int     i = 0;                           // temporary loop counters
    int kbdchar;
    main_state mms_value;

#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif
    bool frommandel = false;            // if julia entered from mandel
    if (resumeflag)
    {
        goto resumeloop;
    }

    while (true)                    // eternal loop
    {
#if defined(_WIN32)
        _ASSERTE(_CrtCheckMemory());
#endif

        if (g_calc_status != calc_status_value::RESUMABLE || g_show_file == 0)
        {
            std::memcpy((char *)&g_video_entry, (char *)&g_video_table[g_adapter],
                   sizeof(g_video_entry));
            g_dot_mode = 19;     // assembler dot read/write
            g_logical_screen_x_dots   = g_video_entry.xdots;       // # dots across the screen
            g_logical_screen_y_dots   = g_video_entry.ydots;       // # dots down the screen
            g_colors  = g_video_entry.colors;      // # colors available
            g_dot_mode %= 100;
            g_screen_x_dots  = g_logical_screen_x_dots;
            g_screen_y_dots  = g_logical_screen_y_dots;
            g_logical_screen_y_offset = 0;
            g_logical_screen_x_offset = 0;
            g_color_cycle_range_hi = (g_color_cycle_range_hi < g_colors) ? g_color_cycle_range_hi : g_colors - 1;

            std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC

            if (g_overlay_3d && (g_init_batch == batch_modes::NONE))
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
                    if (driver_diskp())
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
                    return main_state::RESTORE_START;
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
                g_video_entry.xdots = g_logical_screen_x_dots;
                g_video_entry.ydots = g_logical_screen_y_dots;
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
                else if ((driver_diskp() && g_colors == 256) || !g_colors)
                {
                    // disk video, setvideomode via bios didn't get it right, so:
                    validate_luts("default"); // read the default palette file
                }
                g_color_state = color_state::DEFAULT;
            }
            if (g_view_window)
            {
                // bypass for VESA virtual screen
                ftemp = g_final_aspect_ratio*(((double) g_screen_y_dots)/((double) g_screen_x_dots)/g_screen_aspect);
                g_logical_screen_x_dots = g_view_x_dots;
                if (g_logical_screen_x_dots != 0)
                {
                    // xdots specified
                    g_logical_screen_y_dots = g_view_y_dots;
                    if (g_logical_screen_y_dots == 0) // calc ydots?
                    {
                        g_logical_screen_y_dots = (int)((double)g_logical_screen_x_dots * ftemp + 0.5);
                    }
                }
                else if (g_final_aspect_ratio <= g_screen_aspect)
                {
                    g_logical_screen_x_dots = (int)((double)g_screen_x_dots / g_view_reduction + 0.5);
                    g_logical_screen_y_dots = (int)((double)g_logical_screen_x_dots * ftemp + 0.5);
                }
                else
                {
                    g_logical_screen_y_dots = (int)((double)g_screen_y_dots / g_view_reduction + 0.5);
                    g_logical_screen_x_dots = (int)((double)g_logical_screen_y_dots / ftemp + 0.5);
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
                    !bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
                {
                    // so ssg works
                    // but no check if in evolve mode to allow lots of small views
                    stop_msg("View window too small; using full screen.");
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP) &&
                    bit_set(g_cur_fractal_specific->flags, fractal_flags::INFCALC))
                {
                    stop_msg("Fractal doesn't terminate! switching off evolution.");
                    g_evolving ^= evolution_mode_flags::FIELDMAP;
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
                {
                    const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
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
        if (g_init_batch == batch_modes::NONE)
        {
            g_look_at_mouse = -ID_KEY_PAGE_UP;        // mouse left button == pgup
        }

        if (g_show_file == 0)
        {
            // loading an image
            g_out_line_cleanup = nullptr;          // outln routine can set this
            if (g_display_3d != display_3d_modes::NONE)                 // set up 3D decoding
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
                    g_calc_status = calc_status_value::RESUMABLE;         // "resume" without 16-bit
                    driver_set_for_text();
                    get_fract_type();
                    return main_state::IMAGE_START;
                }
                g_out_line = pot_line;
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP &&
                g_evolving == evolution_mode_flags::NONE) // regular gif/fra input file
            {
                g_out_line = sound_line;      // sound decoding
            }
            else
            {
                g_out_line = out_line;        // regular decoding
            }
            if (g_debug_flag == debug_flags::show_float_flag)
            {
                char msg[MSG_LEN];
                std::snprintf(msg, std::size(msg), "floatflag=%d", g_user_float_flag ? 1 : 0);
                stop_msg(stopmsg_flags::NO_BUZZER, msg);
            }
            i = funny_glasses_call(gif_view);
            if (g_out_line_cleanup)              // cleanup routine defined?
            {
                (*g_out_line_cleanup)();
            }
            if (i == 0)
            {
                driver_buzzer(buzzer_codes::COMPLETE);
            }
            else
            {
                g_calc_status = calc_status_value::NO_FRACTAL;
                if (driver_key_pressed())
                {
                    driver_buzzer(buzzer_codes::INTERRUPT);
                    while (driver_key_pressed())
                    {
                        driver_get_key();
                    }
                    text_temp_msg("*** load incomplete ***");
                }
            }
        }

        g_zoom_off = true;                 // zooming is enabled
        if (driver_diskp() || bit_set(g_cur_fractal_specific->flags, fractal_flags::NOZOOM))
        {
            g_zoom_off = false;            // for these cases disable zooming
        }
        if (g_evolving == evolution_mode_flags::NONE)
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

        if (g_bf_math != bf_math_type::NONE)
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
            if (g_init_batch == batch_modes::NORMAL && g_calc_status == calc_status_value::RESUMABLE)
            {
                g_init_batch = batch_modes::FINISH_CALC_BEFORE_SAVE;
            }
            if (g_loaded_3d)      // 'r' of image created with '3'
            {
                g_display_3d = display_3d_modes::YES;  // so set flag for 'b' command
            }
        }
        else
        {
            // draw an image
            if (g_init_save_time != 0 // autosave and resumable?
                && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NORESUME))
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
            if (g_view_window && bit_set(g_evolving, evolution_mode_flags::FIELDMAP) &&
                g_calc_status != calc_status_value::COMPLETED)
            {
                // generate a set of images with varied parameters on each one
                int ecount;
                int tmpxdots;
                int tmpydots;
                int gridsqr;
                GeneBase gene[NUM_GENES];
                copy_genes_from_bank(gene);
                if (g_have_evolve_info && (g_calc_status == calc_status_value::RESUMABLE))
                {
                    g_evolve_x_parameter_range = g_evolve_info.x_parameter_range;
                    g_evolve_y_parameter_range = g_evolve_info.y_parameter_range;
                    g_evolve_new_x_parameter_offset = g_evolve_info.x_parameter_offset;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_new_y_parameter_offset = g_evolve_info.y_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_new_discrete_x_parameter_offset = (char)g_evolve_info.discrete_x_parameter_offset;
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_new_discrete_y_parameter_offset = (char)g_evolve_info.discrete_y_paramter_offset;
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset;
                    g_evolve_param_grid_x           = g_evolve_info.px;
                    g_evolve_param_grid_y           = g_evolve_info.py;
                    g_logical_screen_x_offset       = g_evolve_info.sxoffs;
                    g_logical_screen_y_offset       = g_evolve_info.syoffs;
                    g_logical_screen_x_dots        = g_evolve_info.xdots;
                    g_logical_screen_y_dots        = g_evolve_info.ydots;
                    g_evolve_image_grid_size = g_evolve_info.image_grid_size;
                    g_evolve_this_generation_random_seed = g_evolve_info.this_generation_random_seed;
                    g_evolve_max_random_mutation = g_evolve_info.max_random_mutation;
                    g_evolving = static_cast<evolution_mode_flags>(g_evolve_info.evolving);
                    g_view_window = g_evolving != evolution_mode_flags::NONE;
                    ecount       = g_evolve_info.ecount;
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
                    ecount = 0;
                    g_evolve_max_random_mutation = g_evolve_max_random_mutation * g_evolve_mutation_reduction_factor;
                    g_evolve_x_parameter_offset = g_evolve_new_x_parameter_offset;
                    g_evolve_y_parameter_offset = g_evolve_new_y_parameter_offset;
                    g_evolve_discrete_x_parameter_offset = g_evolve_new_discrete_x_parameter_offset;
                    g_evolve_discrete_y_parameter_offset = g_evolve_new_discrete_y_parameter_offset; // evolve_discrete_x_parameter_offset used for discrete parms like inside, outside, trigfn etc
                }
                g_evolve_param_box_count = 0;
                g_evolve_dist_per_x = g_evolve_x_parameter_range /(g_evolve_image_grid_size -1);
                g_evolve_dist_per_y = g_evolve_y_parameter_range /(g_evolve_image_grid_size -1);
                const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
                tmpxdots = g_logical_screen_x_dots+grout;
                tmpydots = g_logical_screen_y_dots+grout;
                gridsqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
                while (ecount < gridsqr)
                {
                    spiral_map(ecount); // sets px & py
                    g_logical_screen_x_offset = tmpxdots * g_evolve_param_grid_x;
                    g_logical_screen_y_offset = tmpydots * g_evolve_param_grid_y;
                    restore_param_history();
                    fiddle_params(gene, ecount);
                    calc_frac_init();
                    if (calc_fract() == -1)
                    {
                        goto done;
                    }
                    ecount ++;
                }
done:
#if defined(_WIN32)
                _ASSERTE(_CrtCheckMemory());
#endif

                if (ecount == gridsqr)
                {
                    i = 0;
                    driver_buzzer(buzzer_codes::COMPLETE); // finished!!
                }
                else
                {
                    g_evolve_info.x_parameter_range = g_evolve_x_parameter_range;
                    g_evolve_info.y_parameter_range = g_evolve_y_parameter_range;
                    g_evolve_info.x_parameter_offset = g_evolve_x_parameter_offset;
                    g_evolve_info.y_parameter_offset = g_evolve_y_parameter_offset;
                    g_evolve_info.discrete_x_parameter_offset = (short) g_evolve_discrete_x_parameter_offset;
                    g_evolve_info.discrete_y_paramter_offset = (short) g_evolve_discrete_y_parameter_offset;
                    g_evolve_info.px              = (short)g_evolve_param_grid_x;
                    g_evolve_info.py              = (short)g_evolve_param_grid_y;
                    g_evolve_info.sxoffs          = (short)g_logical_screen_x_offset;
                    g_evolve_info.syoffs          = (short)g_logical_screen_y_offset;
                    g_evolve_info.xdots           = (short)g_logical_screen_x_dots;
                    g_evolve_info.ydots           = (short)g_logical_screen_y_dots;
                    g_evolve_info.image_grid_size = (short) g_evolve_image_grid_size;
                    g_evolve_info.this_generation_random_seed = (short) g_evolve_this_generation_random_seed;
                    g_evolve_info.max_random_mutation = g_evolve_max_random_mutation;
                    g_evolve_info.evolving        = (short) +g_evolving;
                    g_evolve_info.ecount          = (short) ecount;
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
                    driver_buzzer(buzzer_codes::COMPLETE); // finished!!
                }
            }

            s_save_ticks = 0;                 // turn off autosave timer
            if (driver_diskp() && i == 0) // disk-video
            {
                dvid_status(0, "Image has been completed");
            }
        }
        g_box_count = 0;                     // no zoom box yet
        g_zoom_box_width = 0;

        if (g_fractal_type == fractal_type::PLASMA)
        {
            g_cycle_limit = 256;              // plasma clouds need quick spins
            g_dac_count = 256;
            g_dac_learn = true;
        }

resumeloop:                             // return here on failed overlays
#if defined(_WIN32)
        _ASSERTE(_CrtCheckMemory());
#endif
        *kbdmore = true;
        while (*kbdmore)
        {
            // loop through command keys
            if (g_timed_save != 0)
            {
                if (g_timed_save == 1)
                {
                    // woke up for timed save
                    driver_get_key();     // eat the dummy char
                    kbdchar = 's'; // do the save
                    g_resave_flag = 1;
                    g_timed_save = 2;
                }
                else
                {
                    // save done, resume
                    g_timed_save = 0;
                    g_resave_flag = 2;
                    kbdchar = ID_KEY_ENTER;
                }
            }
            else if (g_init_batch == batch_modes::NONE)      // not batch mode
            {
                g_look_at_mouse = g_zoom_box_width == 0 ? -ID_KEY_PAGE_UP : +MouseLook::POSITION;
                if (g_calc_status == calc_status_value::RESUMABLE && g_zoom_box_width == 0 && !driver_key_pressed())
                {
                    kbdchar = ID_KEY_ENTER;  // no visible reason to stop, continue
                }
                else      // wait for a real keystroke
                {
                    if (g_auto_browse && g_browse_sub_images)
                    {
                        kbdchar = 'l';
                    }
                    else
                    {
                        driver_wait_key_pressed(0);
                        kbdchar = driver_get_key();
                    }
                    if (kbdchar == ID_KEY_ESC || kbdchar == 'm' || kbdchar == 'M')
                    {
                        if (kbdchar == ID_KEY_ESC && g_escape_exit)
                        {
                            // don't ask, just get out
                            goodbye();
                        }
                        driver_stack_screen();
                        kbdchar = main_menu(true);
                        if (kbdchar == '\\' || kbdchar == ID_KEY_CTL_BACKSLASH
                            || kbdchar == 'h' || kbdchar == ID_KEY_CTL_H
                            || check_vid_mode_key(0, kbdchar) >= 0)
                        {
                            driver_discard_screen();
                        }
                        else if (kbdchar == 'x' || kbdchar == 'y'
                            || kbdchar == 'z' || kbdchar == 'g'
                            || kbdchar == 'v' || kbdchar == ID_KEY_CTL_B
                            || kbdchar == ID_KEY_CTL_E || kbdchar == ID_KEY_CTL_F)
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
                // init_batch == FINISH_CALC_BEFORE_SAVE        flag to finish calc before save
                // init_batch == NONE                           not in batch mode
                // init_batch == NORMAL                         normal batch mode
                // init_batch == SAVE                           was NORMAL, now do a save
                // init_batch == BAILOUT_ERROR_NO_SAVE          bailout with errorlevel == 2, error occurred, no save
                // init_batch == BAILOUT_INTERRUPTED_TRY_SAVE   bailout with errorlevel == 1, interrupted, try to save
                // init_batch == BAILOUT_INTERRUPTED_SAVE       was BAILOUT_INTERRUPTED_TRY_SAVE, now do a save

                if (g_init_batch == batch_modes::FINISH_CALC_BEFORE_SAVE)
                {
                    kbdchar = ID_KEY_ENTER;
                    g_init_batch = batch_modes::NORMAL;
                }
                else if (g_init_batch == batch_modes::NORMAL || g_init_batch == batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE)         // save-to-disk
                {
                    kbdchar = (g_debug_flag == debug_flags::force_disk_restore_not_save) ? 'r' : 's';
                    if (g_init_batch == batch_modes::NORMAL)
                    {
                        g_init_batch = batch_modes::SAVE;
                    }
                    if (g_init_batch == batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE)
                    {
                        g_init_batch = batch_modes::BAILOUT_INTERRUPTED_SAVE;
                    }
                }
                else
                {
                    if (g_calc_status != calc_status_value::COMPLETED)
                    {
                        g_init_batch = batch_modes::BAILOUT_ERROR_NO_SAVE; // bailout with error
                    }
                    goodbye();               // done, exit
                }
            }

            if ('A' <= kbdchar && kbdchar <= 'Z')
            {
                kbdchar = std::tolower(kbdchar);
            }
            if (g_evolving != evolution_mode_flags::NONE)
            {
                mms_value = evolver_menu_switch(&kbdchar, &frommandel, kbdmore, stacked);
            }
            else
            {
                mms_value = main_menu_switch(kbdchar, frommandel, *kbdmore, *stacked);
            }
            if (g_quick_calc
                && (mms_value == main_state::IMAGE_START
                    || mms_value == main_state::RESTORE_START
                    || mms_value == main_state::RESTART))
            {
                g_quick_calc = false;
                g_user_std_calc_mode = g_old_std_calc_mode;
            }
            if (g_quick_calc && g_calc_status != calc_status_value::COMPLETED)
            {
                g_user_std_calc_mode = '1';
            }
            switch (mms_value)
            {
            case main_state::IMAGE_START:
                return main_state::IMAGE_START;
            case main_state::RESTORE_START:
                return main_state::RESTORE_START;
            case main_state::RESTART:
                return main_state::RESTART;
            case main_state::CONTINUE:
                continue;
            default:
                break;
            }
            if (g_zoom_off && *kbdmore) // draw/clear a zoom box?
            {
                draw_box(true);
            }
            if (driver_resize())
            {
                g_calc_status = calc_status_value::NO_FRACTAL;
            }
        }
    }
}

static int call_line3d(BYTE *pixels, int linelen)
{
    // this routine exists because line3d might be in an overlay
    return line3d(pixels, linelen);
}

// displays differences between current image file and new image
static int cmp_line(BYTE *pixels, int linelen)
{
    int row;
    int oldcolor;
    row = g_row_count++;
    if (row == 0)
    {
        s_err_count = 0;
        s_cmp_fp = dir_fopen(g_working_dir.c_str(), "cmperr", (g_init_batch != batch_modes::NONE) ? "a" : "w");
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
    for (int col = 0; col < linelen; col++)
    {
        oldcolor = get_color(col, row);
        if (oldcolor == (int)pixels[col])
        {
            g_put_color(col, row, 0);
        }
        else
        {
            if (oldcolor == 0)
            {
                g_put_color(col, row, 1);
            }
            ++s_err_count;
            if (g_init_batch == batch_modes::NONE)
            {
                std::fprintf(s_cmp_fp, "#%5d col %3d row %3d old %3d new %3d\n",
                        s_err_count, col, row, oldcolor, pixels[col]);
            }
        }
    }
    return 0;
}

static void cmp_line_cleanup()
{
    char *timestring;
    time_t ltime;
    if (g_init_batch != batch_modes::NONE)
    {
        time(&ltime);
        timestring = ctime(&ltime);
        timestring[24] = 0; //clobber newline in time string
        std::fprintf(s_cmp_fp, "%s compare to %s has %5d errs\n",
                timestring, g_read_filename.c_str(), s_err_count);
    }
    std::fclose(s_cmp_fp);
}

// read keystrokes while = specified key, return 1+count;
// used to catch up when moving zoombox is slower than keyboard
int key_count(int keynum)
{
    int ctr;
    ctr = 1;
    while (driver_key_pressed() == keynum)
    {
        driver_get_key();
        ++ctr;
    }
    return ctr;
}
