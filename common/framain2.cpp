#include "port.h"
#include "prototyp.h"

#include "ant.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "decoder.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractype.h"
#include "framain2.h"
#include "gifview.h"
#include "helpdefs.h"
#include "jb.h"
#include "jiim.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "miscovl.h"
#include "miscres.h"
#include "parser.h"
#include "plot3d.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"
#include "zoom.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <string>
#include <vector>

// routines in this module

static main_state evolver_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
static void move_zoombox(int);
static bool fromtext_flag = false;      // = true if we're in graphics mode
static int call_line3d(BYTE *pixels, int linelen);
static  void move_zoombox(int keynum);
static  void cmp_line_cleanup();
static void restore_history_info(int);
static void save_history_info();

int g_finish_row = 0;    // save when this row is finished
EVOLUTION_INFO g_evolve_info = { 0 };
bool g_have_evolve_info = false;
char g_old_std_calc_mode;
static  int        historyptr = -1;     // user pointer into history tbl
static  int        saveptr = 0;         // save ptr into history tbl
static bool historyflag = false;        // are we backing off in history?
void (*g_out_line_cleanup)();
bool g_virtual_screens = false;

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
            memcpy((char *)&g_video_entry, (char *)&g_video_table[g_adapter],
                   sizeof(g_video_entry));
            g_dot_mode = g_video_entry.dotmode;     // assembler dot read/write
            g_logical_screen_x_dots   = g_video_entry.xdots;       // # dots across the screen
            g_logical_screen_y_dots   = g_video_entry.ydots;       // # dots down the screen
            g_colors  = g_video_entry.colors;      // # colors available
            g_dot_mode %= 100;
            g_screen_x_dots  = g_logical_screen_x_dots;
            g_screen_y_dots  = g_logical_screen_y_dots;
            g_logical_screen_y_offset = 0;
            g_logical_screen_x_offset = 0;
            g_color_cycle_range_hi = (g_color_cycle_range_hi < g_colors) ? g_color_cycle_range_hi : g_colors - 1;

            memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC

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
                        stopmsg(STOPMSG_NONE,
                            "That video mode is not available with your adapter.");
                        g_ask_video = true;
                    }
                    g_init_mode = -1;
                    driver_set_for_text(); // switch to text mode
                    return main_state::RESTORE_START;
                }

                if (g_virtual_screens && (g_logical_screen_x_dots > g_screen_x_dots || g_logical_screen_y_dots > g_screen_y_dots))
                {
                    char buf[120];
                    static char msgxy1[] = {"Can't set virtual line that long, width cut down."};
                    static char msgxy2[] = {"Not enough video memory for that many lines, height cut down."};
                    if (g_logical_screen_x_dots > g_screen_x_dots && g_logical_screen_y_dots > g_screen_y_dots)
                    {
                        sprintf(buf, "%s\n%s", msgxy1, msgxy2);
                        stopmsg(STOPMSG_NONE, buf);
                    }
                    else if (g_logical_screen_y_dots > g_screen_y_dots)
                    {
                        stopmsg(STOPMSG_NONE, msgxy2);
                    }
                    else
                    {
                        stopmsg(STOPMSG_NONE, msgxy1);
                    }
                }
                g_logical_screen_x_dots = g_screen_x_dots;
                g_logical_screen_y_dots = g_screen_y_dots;
                g_video_entry.xdots = g_logical_screen_x_dots;
                g_video_entry.ydots = g_logical_screen_y_dots;
            }

            if (g_save_dac || g_colors_preloaded)
            {
                memcpy(g_dac_box, g_old_dac_box, 256*3); // restore the DAC
                spindac(0, 1);
                g_colors_preloaded = false;
            }
            else
            {
                // reset DAC to defaults, which setvideomode has done for us
                if (g_map_specified)
                {
                    // but there's a map=, so load that
                    for (int i = 0; i < 256; ++i)
                    {
                        g_dac_box[i][0] = g_map_clut[i][0];
                        g_dac_box[i][1] = g_map_clut[i][1];
                        g_dac_box[i][2] = g_map_clut[i][2];
                    }
                    spindac(0, 1);
                }
                else if ((driver_diskp() && g_colors == 256) || !g_colors)
                {
                    // disk video, setvideomode via bios didn't get it right, so:
#if !defined(XFRACT) && !defined(_WIN32)
                    ValidateLuts("default"); // read the default palette file
#endif
                }
                g_color_state = 0;
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
                    stopmsg(STOPMSG_NONE,
                        "View window too large; using full screen.");
                    g_view_window = false;
                    g_view_x_dots = g_screen_x_dots;
                    g_logical_screen_x_dots = g_view_x_dots;
                    g_view_y_dots = g_screen_y_dots;
                    g_logical_screen_y_dots = g_view_y_dots;
                }
                else if (((g_logical_screen_x_dots <= 1) // changed test to 1, so a 2x2 window will
                          || (g_logical_screen_y_dots <= 1)) // work with the sound feature
                         && !(g_evolving & FIELDMAP))
                {
                    // so ssg works
                    // but no check if in evolve mode to allow lots of small views
                    stopmsg(STOPMSG_NONE,
                        "View window too small; using full screen.");
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if ((g_evolving & FIELDMAP) && (g_cur_fractal_specific->flags & INFCALC))
                {
                    stopmsg(STOPMSG_NONE,
                        "Fractal doesn't terminate! switching off evolution.");
                    g_evolving ^= FIELDMAP;
                    g_view_window = false;
                    g_logical_screen_x_dots = g_screen_x_dots;
                    g_logical_screen_y_dots = g_screen_y_dots;
                }
                if (g_evolving & FIELDMAP)
                {
                    g_logical_screen_x_dots = (g_screen_x_dots / g_evolve_image_grid_size)-!((g_evolving & NOGROUT)/NOGROUT);
                    g_logical_screen_x_dots = g_logical_screen_x_dots - (g_logical_screen_x_dots % 4); // trim to multiple of 4 for SSG
                    g_logical_screen_y_dots = (g_screen_y_dots / g_evolve_image_grid_size)-!((g_evolving & NOGROUT)/NOGROUT);
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
            g_look_at_mouse = -FIK_PAGE_UP;        // mouse left button == pgup
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
                if (pot_startdisk() < 0)
                {
                    // pot file failed?
                    g_show_file = 1;
                    g_potential_flag  = false;
                    g_potential_16bit = false;
                    g_init_mode = -1;
                    g_calc_status = calc_status_value::RESUMABLE;         // "resume" without 16-bit
                    driver_set_for_text();
                    get_fracttype();
                    return main_state::IMAGE_START;
                }
                g_out_line = pot_line;
            }
            else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP && !g_evolving) // regular gif/fra input file
            {
                g_out_line = sound_line;      // sound decoding
            }
            else
            {
                g_out_line = out_line;        // regular decoding
            }
            if (g_debug_flag == debug_flags::show_float_flag)
            {
                char msg[MSGLEN];
                sprintf(msg, "floatflag=%d", g_user_float_flag ? 1 : 0);
                stopmsg(STOPMSG_NO_BUZZER, msg);
            }
            i = funny_glasses_call(gifview);
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
                    texttempmsg("*** load incomplete ***");
                }
            }
        }

        g_zoom_off = true;                 // zooming is enabled
        if (driver_diskp() || (g_cur_fractal_specific->flags&NOZOOM) != 0)
        {
            g_zoom_off = false;            // for these cases disable zooming
        }
        if (!g_evolving)
        {
            calcfracinit();
        }
        driver_schedule_alarm(1);

        g_save_x_min = g_x_min; // save 3 corners for zoom.c ref points
        g_save_x_max = g_x_max;
        g_save_x_3rd = g_x_3rd;
        g_save_y_min = g_y_min;
        g_save_y_max = g_y_max;
        g_save_y_3rd = g_y_3rd;

        if (bf_math != bf_math_type::NONE)
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
            if (g_init_save_time != 0          // autosave and resumable?
                    && (g_cur_fractal_specific->flags&NORESUME) == 0)
            {
                g_save_base = readticker(); // calc's start time
                g_save_ticks = abs(g_init_save_time);
                g_save_ticks *= 1092; // bios ticks/minute
                if ((g_save_ticks & 65535L) == 0)
                {
                    ++g_save_ticks; // make low word nonzero
                }
                g_finish_row = -1;
            }
            g_browsing = false;      // regenerate image, turn off browsing
            //rb
            g_filename_stack_index = -1;   // reset pointer
            g_browse_name.clear();
            if (g_view_window && (g_evolving & FIELDMAP) && (g_calc_status != calc_status_value::COMPLETED))
            {
                // generate a set of images with varied parameters on each one
                int grout, ecount, tmpxdots, tmpydots, gridsqr;
                GENEBASE gene[NUMGENES];
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
                    g_evolving     = g_evolve_info.evolving;
                    g_view_window = g_evolving != 0;
                    ecount       = g_evolve_info.ecount;
                    g_have_evolve_info = false;
                }
                else
                {
                    // not resuming, start from the beginning
                    int mid = g_evolve_image_grid_size / 2;
                    if ((g_evolve_param_grid_x != mid) || (g_evolve_param_grid_y != mid))
                    {
                        g_evolve_this_generation_random_seed = (unsigned int)clock_ticks(); // time for new set
                    }
                    param_history(0); // save old history
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
                grout  = !((g_evolving & NOGROUT)/NOGROUT);
                tmpxdots = g_logical_screen_x_dots+grout;
                tmpydots = g_logical_screen_y_dots+grout;
                gridsqr = g_evolve_image_grid_size * g_evolve_image_grid_size;
                while (ecount < gridsqr)
                {
                    spiralmap(ecount); // sets px & py
                    g_logical_screen_x_offset = tmpxdots * g_evolve_param_grid_x;
                    g_logical_screen_y_offset = tmpydots * g_evolve_param_grid_y;
                    param_history(1); // restore old history
                    fiddleparms(gene, ecount);
                    calcfracinit();
                    if (calcfract() == -1)
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
                    g_evolve_info.evolving        = (short)g_evolving;
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
                unspiralmap(); // first time called, w/above line sets up array
                param_history(1); // restore old history
                fiddleparms(gene, 0);
                copy_genes_to_bank(gene);
            }
            // end of evolution loop
            else
            {
                i = calcfract();       // draw the fractal using "C"
                if (i == 0)
                {
                    driver_buzzer(buzzer_codes::COMPLETE); // finished!!
                }
            }

            g_save_ticks = 0;                 // turn off autosave timer
            if (driver_diskp() && i == 0) // disk-video
            {
                dvid_status(0, "Image has been completed");
            }
        }
#ifndef XFRACT
        g_box_count = 0;                     // no zoom box yet
        g_zoom_box_width = 0;
#else
        if (!g_x_zoom_waiting)
        {
            g_box_count = 0;                 // no zoom box yet
            g_zoom_box_width = 0;
        }
#endif

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
                    kbdchar = FIK_ENTER;
                }
            }
            else if (g_init_batch == batch_modes::NONE)      // not batch mode
            {
#ifndef XFRACT
                g_look_at_mouse = (g_zoom_box_width == 0 && !g_video_scroll) ? -FIK_PAGE_UP : 3;
#else
                g_look_at_mouse = (g_zoom_box_width == 0) ? -FIK_PAGE_UP : 3;
#endif
                if (g_calc_status == calc_status_value::RESUMABLE && g_zoom_box_width == 0 && !driver_key_pressed())
                {
                    kbdchar = FIK_ENTER ;  // no visible reason to stop, continue
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
                    if (kbdchar == FIK_ESC || kbdchar == 'm' || kbdchar == 'M')
                    {
                        if (kbdchar == FIK_ESC && g_escape_exit)
                        {
                            // don't ask, just get out
                            goodbye();
                        }
                        driver_stack_screen();
#ifndef XFRACT
                        kbdchar = main_menu(1);
#else
                        if (g_x_zoom_waiting)
                        {
                            kbdchar = FIK_ENTER;
                        }
                        else
                        {
                            kbdchar = main_menu(1);
                            if (g_x_zoom_waiting)
                            {
                                kbdchar = FIK_ENTER;
                            }
                        }
#endif
                        if (kbdchar == '\\' || kbdchar == FIK_CTL_BACKSLASH ||
                                kbdchar == 'h' || kbdchar == FIK_CTL_H ||
                                check_vidmode_key(0, kbdchar) >= 0)
                        {
                            driver_discard_screen();
                        }
                        else if (kbdchar == 'x' || kbdchar == 'y' ||
                                 kbdchar == 'z' || kbdchar == 'g' ||
                                 kbdchar == 'v' || kbdchar == FIK_CTL_B ||
                                 kbdchar == FIK_CTL_E || kbdchar == FIK_CTL_F)
                        {
                            fromtext_flag = true;
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
                    kbdchar = FIK_ENTER;
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

#ifndef XFRACT
            if ('A' <= kbdchar && kbdchar <= 'Z')
            {
                kbdchar = tolower(kbdchar);
            }
#endif
            if (g_evolving)
            {
                mms_value = evolver_menu_switch(&kbdchar, &frommandel, kbdmore, stacked);
            }
            else
            {
                mms_value = main_menu_switch(&kbdchar, &frommandel, kbdmore, stacked);
            }
            if (g_quick_calc && (mms_value == main_state::IMAGE_START ||
                               mms_value == main_state::RESTORE_START ||
                               mms_value == main_state::RESTART))
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
                drawbox(true);
            }
            if (driver_resize())
            {
                g_calc_status = calc_status_value::NO_FRACTAL;
            }
        }
    }
}

static bool look(bool *stacked)
{
    int old_help_mode;
    old_help_mode = g_help_mode;
    g_help_mode = HELPBROWSE;
    switch (fgetwindow())
    {
    case FIK_ENTER:
    case FIK_ENTER_2:
        g_show_file = 0;       // trigger load
        g_browsing = true;    // but don't ask for the file name as it's just been selected
        if (g_filename_stack_index == 15)
        {
            /* about to run off the end of the file
                * history stack so shift it all back one to
                * make room, lose the 1st one */
            for (int tmp = 1; tmp < 16; tmp++)
            {
                g_file_name_stack[tmp - 1] = g_file_name_stack[tmp];
            }
            g_filename_stack_index = 14;
        }
        g_filename_stack_index++;
        g_file_name_stack[g_filename_stack_index] = g_browse_name;
        merge_pathnames(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
        if (g_ask_video)
        {
            driver_stack_screen();   // save graphics image
            *stacked = true;
        }
        return true;       // hop off and do it!!

    case '\\':
        if (g_filename_stack_index >= 1)
        {
            // go back one file if somewhere to go (ie. browsing)
            g_filename_stack_index--;
            while (g_file_name_stack[g_filename_stack_index].empty()
                    && g_filename_stack_index >= 0)
            {
                g_filename_stack_index--;
            }
            if (g_filename_stack_index < 0) // oops, must have deleted first one
            {
                break;
            }
            g_browse_name = g_file_name_stack[g_filename_stack_index];
            merge_pathnames(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
            g_browsing = true;
            g_show_file = 0;
            if (g_ask_video)
            {
                driver_stack_screen();// save graphics image
                *stacked = true;
            }
            return true;
        }                   // otherwise fall through and turn off browsing
    case FIK_ESC:
    case 'l':              // turn it off
    case 'L':
        g_browsing = false;
        g_help_mode = old_help_mode;
        break;

    case 's':
        g_browsing = false;
        g_help_mode = old_help_mode;
        savetodisk(g_save_filename);
        break;

    default:               // or no files found, leave the state of browsing alone
        break;
    }

    return false;
}

main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked)
{
    int i, k;
    static double  jxxmin, jxxmax, jyymin, jyymax; // "Julia mode" entry point
    static double  jxx3rd, jyy3rd;
    long old_maxit;

    if (g_quick_calc && g_calc_status == calc_status_value::COMPLETED)
    {
        g_quick_calc = false;
        g_user_std_calc_mode = g_old_std_calc_mode;
    }
    if (g_quick_calc && g_calc_status != calc_status_value::COMPLETED)
    {
        g_user_std_calc_mode = g_old_std_calc_mode;
    }
    switch (*kbdchar)
    {
    case 't':                    // new fractal type
        g_julibrot = false;
        clear_zoombox();
        driver_stack_screen();
        i = get_fracttype();
        if (i >= 0)
        {
            driver_discard_screen();
            g_save_dac = 0;
            g_magnitude_calc = true;
            g_use_old_periodicity = false;
            g_bad_outside = false;
            g_ld_check = false;
            set_current_params();
            g_evolve_new_discrete_y_parameter_offset = 0;
            g_evolve_new_discrete_x_parameter_offset = 0;
            g_evolve_discrete_y_parameter_offset = 0;
            g_evolve_discrete_x_parameter_offset = 0;
            g_evolve_max_random_mutation = 1;           // reset param evolution stuff
            g_set_orbit_corners = false;
            param_history(0); // save history
            if (i == 0)
            {
                g_init_mode = g_adapter;
                *frommandel = false;
            }
            else if (g_init_mode < 0)   // it is supposed to be...
            {
                driver_set_for_text();     // reset to text mode
            }
            return main_state::IMAGE_START;
        }
        driver_unstack_screen();
        break;
    case FIK_CTL_X:                     // Ctl-X, Ctl-Y, CTL-Z do flipping
    case FIK_CTL_Y:
    case FIK_CTL_Z:
        flip_image(*kbdchar);
        break;
    case 'x':                    // invoke options screen
    case 'y':
    case 'p':                    // passes options
    case 'z':                    // type specific parms
    case 'g':
    case 'v':
    case FIK_CTL_B:
    case FIK_CTL_E:
    case FIK_CTL_F:
        old_maxit = g_max_iterations;
        clear_zoombox();
        if (fromtext_flag)
        {
            fromtext_flag = false;
        }
        else
        {
            driver_stack_screen();
        }
        if (*kbdchar == 'x')
        {
            i = get_toggles();
        }
        else if (*kbdchar == 'y')
        {
            i = get_toggles2();
        }
        else if (*kbdchar == 'p')
        {
            i = passes_options();
        }
        else if (*kbdchar == 'z')
        {
            i = get_fract_params(1);
        }
        else if (*kbdchar == 'v')
        {
            i = get_view_params(); // get the parameters
        }
        else if (*kbdchar == FIK_CTL_B)
        {
            i = get_browse_params();
        }
        else if (*kbdchar == FIK_CTL_E)
        {
            i = get_evolve_Parms();
            if (i > 0)
            {
                g_start_show_orbit = false;
                g_sound_flag &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); // turn off only x,y,z
                g_log_map_auto_calculate = false; // turn it off
            }
        }
        else if (*kbdchar == FIK_CTL_F)
        {
            i = get_sound_params();
        }
        else
        {
            i = get_cmd_string();
        }
        driver_unstack_screen();
        if (g_evolving && g_truecolor)
        {
            g_truecolor = false;          // truecolor doesn't play well with the evolver
        }
        if (g_max_iterations > old_maxit && g_inside_color >= COLOR_BLACK && g_calc_status == calc_status_value::COMPLETED &&
                g_cur_fractal_specific->calctype == standard_fractal && !g_log_map_flag &&
                !g_truecolor &&    // recalc not yet implemented with truecolor
                !(g_user_std_calc_mode == 't' && g_fill_color > -1) &&
                // tesseral with fill doesn't work
                !(g_user_std_calc_mode == 'o') &&
                i == 1 && // nothing else changed
                g_outside_color != ATAN)
        {
            g_quick_calc = true;
            g_old_std_calc_mode = g_user_std_calc_mode;
            g_user_std_calc_mode = '1';
            *kbdmore = false;
            g_calc_status = calc_status_value::RESUMABLE;
        }
        else if (i > 0)
        {
            // time to redraw?
            g_quick_calc = false;
            param_history(0);           // save history
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;
#ifndef XFRACT
    case '@':                    // execute commands
    case '2':                    // execute commands
#else
    case FIK_F2:                     // execute commands
#endif
        driver_stack_screen();
        i = get_commands();
        if (g_init_mode != -1)
        {
            // video= was specified
            g_adapter = g_init_mode;
            g_init_mode = -1;
            i |= CMDARG_FRACTAL_PARAM;
            g_save_dac = 0;
        }
        else if (g_colors_preloaded)
        {
            // colors= was specified
            spindac(0, 1);
            g_colors_preloaded = false;
        }
        else if (i & CMDARG_RESET)           // reset was specified
        {
            g_save_dac = 0;
        }
        if (i & CMDARG_3D_YES)
        {
            // 3d = was specified
            *kbdchar = '3';
            driver_unstack_screen();
            goto do_3d_transform;  // pretend '3' was keyed
        }
        if (i & CMDARG_FRACTAL_PARAM)
        {
            // fractal parameter changed
            driver_discard_screen();
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        else
        {
            driver_unstack_screen();
        }
        break;
    case 'f':                    // floating pt toggle
        if (!g_user_float_flag)
        {
            g_user_float_flag = true;
        }
        else if (g_std_calc_mode != 'o')     // don't go there
        {
            g_user_float_flag = false;
        }
        g_init_mode = g_adapter;
        return main_state::IMAGE_START;
    case 'i':                    // 3d fractal parms
        if (get_fract3d_params() >= 0)    // get the parameters
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            *kbdmore = false;    // time to redraw
        }
        break;
    case FIK_CTL_A:                     // ^a Ant
        clear_zoombox();
        {
            int err;
            double oldparm[MAXPARAMS];
            fractal_type oldtype = g_fractal_type;
            for (int i = 0; i < MAXPARAMS; ++i)
            {
                oldparm[i] = g_params[i];
            }
            if (g_fractal_type != fractal_type::ANT)
            {
                g_fractal_type = fractal_type::ANT;
                g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
                load_params(g_fractal_type);
            }
            if (!fromtext_flag)
            {
                driver_stack_screen();
            }
            fromtext_flag = false;
            err = get_fract_params(2);
            if (err >= 0)
            {
                driver_unstack_screen();
                if (ant() >= 0)
                {
                    g_calc_status = calc_status_value::PARAMS_CHANGED;
                }
            }
            else
            {
                driver_unstack_screen();
            }
            g_fractal_type = oldtype;
            for (int i = 0; i < MAXPARAMS; ++i)
            {
                g_params[i] = oldparm[i];
            }
            if (err >= 0)
            {
                return main_state::CONTINUE;
            }
        }
        break;
    case 'k':                    // ^s is irritating, give user a single key
    case FIK_CTL_S:                     // ^s RDS
        clear_zoombox();
        if (get_rds_params() >= 0)
        {
            if (do_AutoStereo())
            {
                g_calc_status = calc_status_value::PARAMS_CHANGED;
            }
            return main_state::CONTINUE;
        }
        break;
    case 'a':                    // starfield parms
        clear_zoombox();
        if (get_starfield_params() >= 0)
        {
            if (starfield() >= 0)
            {
                g_calc_status = calc_status_value::PARAMS_CHANGED;
            }
            return main_state::CONTINUE;
        }
        break;
    case FIK_CTL_O:                     // ctrl-o
    case 'o':
        // must use standard fractal and have a float variant
        if ((g_fractal_specific[static_cast<int>(g_fractal_type)].calctype == standard_fractal
                || g_fractal_specific[static_cast<int>(g_fractal_type)].calctype == calcfroth) &&
                (g_fractal_specific[static_cast<int>(g_fractal_type)].isinteger == 0 ||
                 g_fractal_specific[static_cast<int>(g_fractal_type)].tofloat != fractal_type::NOFRACTAL) &&
                (bf_math == bf_math_type::NONE) && // for now no arbitrary precision support
                !(g_is_true_color && g_true_mode != true_color_mode::default_color))
        {
            clear_zoombox();
            Jiim(jiim_types::ORBIT);
        }
        break;
    case FIK_SPACE:                  // spacebar, toggle mand/julia
        if (bf_math != bf_math_type::NONE || g_evolving)
        {
            break;
        }
        if (g_fractal_type == fractal_type::CELLULAR)
        {
            g_cellular_next_screen = !g_cellular_next_screen;
            g_calc_status = calc_status_value::RESUMABLE;
            *kbdmore = false;
        }
        else
        {
            if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
            {
                if (g_is_mandelbrot)
                {
                    g_fractal_specific[static_cast<int>(g_fractal_type)].tojulia = g_fractal_type;
                    g_fractal_specific[static_cast<int>(g_fractal_type)].tomandel = fractal_type::NOFRACTAL;
                    g_is_mandelbrot = false;
                }
                else
                {
                    g_fractal_specific[static_cast<int>(g_fractal_type)].tojulia = fractal_type::NOFRACTAL;
                    g_fractal_specific[static_cast<int>(g_fractal_type)].tomandel = g_fractal_type;
                    g_is_mandelbrot = true;
                }
            }
            if (g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL
                    && g_params[0] == 0.0 && g_params[1] == 0.0)
            {
                // switch to corresponding Julia set
                int key;
                g_has_inverse = (g_fractal_type == fractal_type::MANDEL || g_fractal_type == fractal_type::MANDELFP) && bf_math == bf_math_type::NONE;
                clear_zoombox();
                Jiim(jiim_types::JIIM);
                key = driver_get_key();    // flush keyboard buffer
                if (key != FIK_SPACE)
                {
                    driver_unget_key(key);
                    break;
                }
                g_fractal_type = g_cur_fractal_specific->tojulia;
                g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
                if (g_julia_c_x == BIG || g_julia_c_y == BIG)
                {
                    g_params[0] = (g_x_max + g_x_min) / 2;
                    g_params[1] = (g_y_max + g_y_min) / 2;
                }
                else
                {
                    g_params[0] = g_julia_c_x;
                    g_params[1] = g_julia_c_y;
                    g_julia_c_y = BIG;
                    g_julia_c_x = BIG;
                }
                jxxmin = g_save_x_min;
                jxxmax = g_save_x_max;
                jyymax = g_save_y_max;
                jyymin = g_save_y_min;
                jxx3rd = g_save_x_3rd;
                jyy3rd = g_save_y_3rd;
                *frommandel = true;
                g_x_min = g_cur_fractal_specific->xmin;
                g_x_max = g_cur_fractal_specific->xmax;
                g_y_min = g_cur_fractal_specific->ymin;
                g_y_max = g_cur_fractal_specific->ymax;
                g_x_3rd = g_x_min;
                g_y_3rd = g_y_min;
                if (g_user_distance_estimator_value == 0 && g_user_biomorph_value != -1 && g_bit_shift != 29)
                {
                    g_x_min *= 3.0;
                    g_x_max *= 3.0;
                    g_y_min *= 3.0;
                    g_y_max *= 3.0;
                    g_x_3rd *= 3.0;
                    g_y_3rd *= 3.0;
                }
                g_zoom_off = true;
                g_calc_status = calc_status_value::PARAMS_CHANGED;
                *kbdmore = false;
            }
            else if (g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
            {
                // switch to corresponding Mandel set
                g_fractal_type = g_cur_fractal_specific->tomandel;
                g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
                if (*frommandel)
                {
                    g_x_min = jxxmin;
                    g_x_max = jxxmax;
                    g_y_min = jyymin;
                    g_y_max = jyymax;
                    g_x_3rd = jxx3rd;
                    g_y_3rd = jyy3rd;
                }
                else
                {
                    g_x_3rd = g_cur_fractal_specific->xmin;
                    g_x_min = g_x_3rd;
                    g_x_max = g_cur_fractal_specific->xmax;
                    g_y_3rd = g_cur_fractal_specific->ymin;
                    g_y_min = g_y_3rd;
                    g_y_max = g_cur_fractal_specific->ymax;
                }
                g_save_c.x = g_params[0];
                g_save_c.y = g_params[1];
                g_params[0] = 0;
                g_params[1] = 0;
                g_zoom_off = true;
                g_calc_status = calc_status_value::PARAMS_CHANGED;
                *kbdmore = false;
            }
            else
            {
                driver_buzzer(buzzer_codes::PROBLEM);          // can't switch
            }
        }                         // end of else for if == cellular
        break;
    case 'j':                    // inverse julia toggle
        // if the inverse types proliferate, something more elegant will be needed
        if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::INVERSEJULIA)
        {
            static fractal_type oldtype = fractal_type::NOFRACTAL;
            if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP)
            {
                oldtype = g_fractal_type;
                g_fractal_type = fractal_type::INVERSEJULIA;
            }
            else if (g_fractal_type == fractal_type::INVERSEJULIA)
            {
                if (oldtype != fractal_type::NOFRACTAL)
                {
                    g_fractal_type = oldtype;
                }
                else
                {
                    g_fractal_type = fractal_type::JULIA;
                }
            }
            g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
            g_zoom_off = true;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            *kbdmore = false;
        }
        else
        {
            driver_buzzer(buzzer_codes::PROBLEM);
        }
        break;
    case '\\':                   // return to prev image
    case FIK_CTL_BACKSLASH:
    case 'h':
    case FIK_BACKSPACE:
        if (g_filename_stack_index >= 1)
        {
            // go back one file if somewhere to go (ie. browsing)
            g_filename_stack_index--;
            while (g_file_name_stack[g_filename_stack_index].empty()
                    && g_filename_stack_index >= 0)
            {
                g_filename_stack_index--;
            }
            if (g_filename_stack_index < 0)   // oops, must have deleted first one
            {
                break;
            }
            g_browse_name = g_file_name_stack[g_filename_stack_index];
            merge_pathnames(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
            g_browsing = true;
            g_browse_sub_images = true;
            g_show_file = 0;
            if (g_ask_video)
            {
                driver_stack_screen();      // save graphics image
                *stacked = true;
            }
            return main_state::RESTORE_START;
        }
        else if (g_max_image_history > 0 && bf_math == bf_math_type::NONE)
        {
            if (*kbdchar == '\\' || *kbdchar == 'h')
            {
                if (--historyptr < 0)
                {
                    historyptr = g_max_image_history - 1;
                }
            }
            if (*kbdchar == FIK_CTL_BACKSLASH || *kbdchar == FIK_BACKSPACE)
            {
                if (++historyptr >= g_max_image_history)
                {
                    historyptr = 0;
                }
            }
            restore_history_info(historyptr);
            g_zoom_off = true;
            g_init_mode = g_adapter;
            if (g_cur_fractal_specific->isinteger != 0 &&
                    g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = false;
            }
            if (g_cur_fractal_specific->isinteger == 0 &&
                    g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = true;
            }
            historyflag = true;         // avoid re-store parms due to rounding errs
            return main_state::IMAGE_START;
        }
        break;
    case 'd':                    // shell to MS-DOS
        driver_stack_screen();
        driver_shell();
        driver_unstack_screen();
        break;

    case 'c':                    // switch to color cycling
    case '+':                    // rotate palette
    case '-':                    // rotate palette
        clear_zoombox();
        memcpy(g_old_dac_box, g_dac_box, 256 * 3);
        rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
        if (memcmp(g_old_dac_box, g_dac_box, 256 * 3))
        {
            g_color_state = 1;
            save_history_info();
        }
        return main_state::CONTINUE;
    case 'e':                    // switch to color editing
        if (g_is_true_color && (g_init_batch == batch_modes::NONE))
        {
            // don't enter palette editor
            if (!load_palette())
            {
                *kbdmore = false;
                g_calc_status = calc_status_value::PARAMS_CHANGED;
                break;
            }
            else
            {
                return main_state::CONTINUE;
            }
        }
        clear_zoombox();
        if (g_dac_box[0][0] != 255 && g_colors >= 16 && !driver_diskp())
        {
            int old_help_mode;
            old_help_mode = g_help_mode;
            memcpy(g_old_dac_box, g_dac_box, 256 * 3);
            g_help_mode = HELPXHAIR;
            EditPalette();
            g_help_mode = old_help_mode;
            if (memcmp(g_old_dac_box, g_dac_box, 256 * 3))
            {
                g_color_state = 1;
                save_history_info();
            }
        }
        return main_state::CONTINUE;
    case 's':                    // save-to-disk
        if (driver_diskp() && g_disk_targa)
        {
            return main_state::CONTINUE;  // disk video and targa, nothing to save
        }
        savetodisk(g_save_filename);
        return main_state::CONTINUE;
    case '#':                    // 3D overlay
#ifdef XFRACT
    case FIK_F3:                     // 3D overlay
#endif
        clear_zoombox();
        g_overlay_3d = true;
    case '3':                    // restore-from (3d)
do_3d_transform:
        if (g_overlay_3d)
        {
            g_display_3d = display_3d_modes::B_COMMAND;         // for <b> command
        }
        else
        {
            g_display_3d = display_3d_modes::YES;
        }
    case 'r':                    // restore-from
        g_compare_gif = false;
        *frommandel = false;
        g_browsing = false;
        if (*kbdchar == 'r')
        {
            if (g_debug_flag == debug_flags::force_disk_restore_not_save)
            {
                g_compare_gif = true;
                g_overlay_3d = true;
                if (g_init_batch == batch_modes::SAVE)
                {
                    driver_stack_screen();   // save graphics image
                    g_read_filename = g_save_filename;
                    g_show_file = 0;
                    return main_state::RESTORE_START;
                }
            }
            else
            {
                g_compare_gif = false;
                g_overlay_3d = false;
            }
            g_display_3d = display_3d_modes::NONE;
        }
        driver_stack_screen();            // save graphics image
        if (g_overlay_3d)
        {
            *stacked = false;
        }
        else
        {
            *stacked = true;
        }
        if (g_resave_flag)
        {
            updatesavename(g_save_filename);      // do the pending increment
            g_resave_flag = 0;
            g_started_resaves = false;
        }
        g_show_file = -1;
        return main_state::RESTORE_START;
    case 'l':
    case 'L':                    // Look for other files within this view
        if ((g_zoom_box_width != 0) || driver_diskp())
        {
            g_browsing = false;
            driver_buzzer(buzzer_codes::PROBLEM);             // can't browse if zooming or disk video
        }
        else if (look(stacked))
        {
            return main_state::RESTORE_START;
        }
        break;
    case 'b':                    // make batch file
        make_batch_file();
        break;
    case FIK_CTL_P:                    // print current image
        driver_buzzer(buzzer_codes::INTERRUPT);
        return main_state::CONTINUE;
    case FIK_ENTER:                  // Enter
    case FIK_ENTER_2:                // Numeric-Keypad Enter
#ifdef XFRACT
        g_x_zoom_waiting = false;
#endif
        if (g_zoom_box_width != 0.0)
        {
            // do a zoom
            init_pan_or_recalc(false);
            *kbdmore = false;
        }
        if (g_calc_status != calc_status_value::COMPLETED)       // don't restart if image complete
        {
            *kbdmore = false;
        }
        break;
    case FIK_CTL_ENTER:              // control-Enter
    case FIK_CTL_ENTER_2:            // Control-Keypad Enter
        init_pan_or_recalc(true);
        *kbdmore = false;
        zoomout();                // calc corners for zooming out
        break;
    case FIK_INSERT:         // insert
        driver_set_for_text();           // force text mode
        return main_state::RESTART;
    case FIK_LEFT_ARROW:             // cursor left
    case FIK_RIGHT_ARROW:            // cursor right
    case FIK_UP_ARROW:               // cursor up
    case FIK_DOWN_ARROW:             // cursor down
        move_zoombox(*kbdchar);
        break;
    case FIK_CTL_LEFT_ARROW:           // Ctrl-cursor left
    case FIK_CTL_RIGHT_ARROW:          // Ctrl-cursor right
    case FIK_CTL_UP_ARROW:             // Ctrl-cursor up
    case FIK_CTL_DOWN_ARROW:           // Ctrl-cursor down
        move_zoombox(*kbdchar);
        break;
    case FIK_CTL_HOME:               // Ctrl-home
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            i = key_count(FIK_CTL_HOME);
            if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
            {
                g_zoom_box_skew = -0.48;
            }
        }
        break;
    case FIK_CTL_END:                // Ctrl-end
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            i = key_count(FIK_CTL_END);
            if ((g_zoom_box_skew += 0.02 * i) > 0.48)
            {
                g_zoom_box_skew = 0.48;
            }
        }
        break;
    case FIK_CTL_PAGE_UP:            // Ctrl-pgup
        if (g_box_count)
        {
            chgboxi(0, -2 * key_count(FIK_CTL_PAGE_UP));
        }
        break;
    case FIK_CTL_PAGE_DOWN:          // Ctrl-pgdn
        if (g_box_count)
        {
            chgboxi(0, 2 * key_count(FIK_CTL_PAGE_DOWN));
        }
        break;

    case FIK_PAGE_UP:                // page up
        if (g_zoom_off)
        {
            if (g_zoom_box_width == 0)
            {
                // start zoombox
                g_zoom_box_height = 1;
                g_zoom_box_width = g_zoom_box_height;
                g_zoom_box_rotation = 0;
                g_zoom_box_skew = g_zoom_box_rotation;
                g_zoom_box_x = 0;
                g_zoom_box_y = 0;
                find_special_colors();
                g_box_color = g_color_bright;
                g_evolve_param_grid_y = g_evolve_image_grid_size /2;
                g_evolve_param_grid_x = g_evolve_param_grid_y;
                moveboxf(0.0, 0.0); // force scrolling
            }
            else
            {
                resizebox(0 - key_count(FIK_PAGE_UP));
            }
        }
        break;
    case FIK_PAGE_DOWN:              // page down
        if (g_box_count)
        {
            if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)   // end zoombox
            {
                g_zoom_box_width = 0;
            }
            else
            {
                resizebox(key_count(FIK_PAGE_DOWN));
            }
        }
        break;
    case FIK_CTL_MINUS:              // Ctrl-kpad-
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            g_zoom_box_rotation += key_count(FIK_CTL_MINUS);
        }
        break;
    case FIK_CTL_PLUS:               // Ctrl-kpad+
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            g_zoom_box_rotation -= key_count(FIK_CTL_PLUS);
        }
        break;
    case FIK_CTL_INSERT:             // Ctrl-ins
        g_box_color += key_count(FIK_CTL_INSERT);
        break;
    case FIK_CTL_DEL:                // Ctrl-del
        g_box_color -= key_count(FIK_CTL_DEL);
        break;

    case FIK_ALT_1: // alt + number keys set mutation level and start evolution engine
    case FIK_ALT_2:
    case FIK_ALT_3:
    case FIK_ALT_4:
    case FIK_ALT_5:
    case FIK_ALT_6:
    case FIK_ALT_7:
        g_evolving = FIELDMAP;
        g_view_window = true;
        set_mutation_level(*kbdchar - FIK_ALT_1 + 1);
        param_history(0); // save parameter history
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case FIK_DELETE:         // select video mode from list
    {
        driver_stack_screen();
        *kbdchar = select_video_mode(g_adapter);
        if (check_vidmode_key(0, *kbdchar) >= 0)    // picked a new mode?
        {
            driver_discard_screen();
        }
        else
        {
            driver_unstack_screen();
        }
        // fall through
    }
    default:                     // other (maybe a valid Fn key)
        k = check_vidmode_key(0, *kbdchar);
        if (k >= 0)
        {
            g_adapter = k;
            if (g_video_table[g_adapter].colors != g_colors)
            {
                g_save_dac = 0;
            }
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            *kbdmore = false;
            return main_state::CONTINUE;
        }
        break;
    }                            // end of the big switch
    return main_state::NOTHING;
}

static main_state evolver_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked)
{
    int i, k;

    switch (*kbdchar)
    {
    case 't':                    // new fractal type
        g_julibrot = false;
        clear_zoombox();
        driver_stack_screen();
        i = get_fracttype();
        if (i >= 0)
        {
            driver_discard_screen();
            g_save_dac = 0;
            g_magnitude_calc = true;
            g_use_old_periodicity = false;
            g_bad_outside = false;
            g_ld_check = false;
            set_current_params();
            g_evolve_new_discrete_y_parameter_offset = 0;
            g_evolve_new_discrete_x_parameter_offset = 0;
            g_evolve_discrete_y_parameter_offset = 0;
            g_evolve_discrete_x_parameter_offset = 0;
            g_evolve_max_random_mutation = 1;           // reset param evolution stuff
            g_set_orbit_corners = false;
            param_history(0); // save history
            if (i == 0)
            {
                g_init_mode = g_adapter;
                *frommandel = false;
            }
            else if (g_init_mode < 0)   // it is supposed to be...
            {
                driver_set_for_text();     // reset to text mode
            }
            return main_state::IMAGE_START;
        }
        driver_unstack_screen();
        break;
    case 'x':                    // invoke options screen
    case 'y':
    case 'p':                    // passes options
    case 'z':                    // type specific parms
    case 'g':
    case FIK_CTL_E:
    case FIK_SPACE:
        clear_zoombox();
        if (fromtext_flag)
        {
            fromtext_flag = false;
        }
        else
        {
            driver_stack_screen();
        }
        if (*kbdchar == 'x')
        {
            i = get_toggles();
        }
        else if (*kbdchar == 'y')
        {
            i = get_toggles2();
        }
        else if (*kbdchar == 'p')
        {
            i = passes_options();
        }
        else if (*kbdchar == 'z')
        {
            i = get_fract_params(1);
        }
        else if (*kbdchar == FIK_CTL_E || *kbdchar == FIK_SPACE)
        {
            i = get_evolve_Parms();
        }
        else
        {
            i = get_cmd_string();
        }
        driver_unstack_screen();
        if (g_evolving && g_truecolor)
        {
            g_truecolor = false;          // truecolor doesn't play well with the evolver
        }
        if (i > 0)
        {
            // time to redraw?
            param_history(0); // save history
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;
    case 'b': // quick exit from evolve mode
        g_evolving = 0;
        g_view_window = false;
        param_history(0); // save history
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case 'f':                    // floating pt toggle
        if (!g_user_float_flag)
        {
            g_user_float_flag = true;
        }
        else if (g_std_calc_mode != 'o')     // don't go there
        {
            g_user_float_flag = false;
        }
        g_init_mode = g_adapter;
        return main_state::IMAGE_START;
    case '\\':                   // return to prev image
    case FIK_CTL_BACKSLASH:
    case 'h':
    case FIK_BACKSPACE:
        if (g_max_image_history > 0 && bf_math == bf_math_type::NONE)
        {
            if (*kbdchar == '\\' || *kbdchar == 'h')
            {
                if (--historyptr < 0)
                {
                    historyptr = g_max_image_history - 1;
                }
            }
            if (*kbdchar == FIK_CTL_BACKSLASH || *kbdchar == 8)
            {
                if (++historyptr >= g_max_image_history)
                {
                    historyptr = 0;
                }
            }
            restore_history_info(historyptr);
            g_zoom_off = true;
            g_init_mode = g_adapter;
            if (g_cur_fractal_specific->isinteger != 0 &&
                    g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = false;
            }
            if (g_cur_fractal_specific->isinteger == 0 &&
                    g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = true;
            }
            historyflag = true;         // avoid re-store parms due to rounding errs
            return main_state::IMAGE_START;
        }
        break;
    case 'c':                    // switch to color cycling
    case '+':                    // rotate palette
    case '-':                    // rotate palette
        clear_zoombox();
        memcpy(g_old_dac_box, g_dac_box, 256 * 3);
        rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
        if (memcmp(g_old_dac_box, g_dac_box, 256 * 3))
        {
            g_color_state = 1;
            save_history_info();
        }
        return main_state::CONTINUE;
    case 'e':                    // switch to color editing
        if (g_is_true_color && (g_init_batch == batch_modes::NONE))
        {
            // don't enter palette editor
            if (!load_palette())
            {
                *kbdmore = false;
                g_calc_status = calc_status_value::PARAMS_CHANGED;
                break;
            }
            else
            {
                return main_state::CONTINUE;
            }
        }
        clear_zoombox();
        if (g_dac_box[0][0] != 255 && g_colors >= 16 && !driver_diskp())
        {
            int old_help_mode;
            old_help_mode = g_help_mode;
            memcpy(g_old_dac_box, g_dac_box, 256 * 3);
            g_help_mode = HELPXHAIR;
            EditPalette();
            g_help_mode = old_help_mode;
            if (memcmp(g_old_dac_box, g_dac_box, 256 * 3))
            {
                g_color_state = 1;
                save_history_info();
            }
        }
        return main_state::CONTINUE;
    case 's':                    // save-to-disk
    {
        if (driver_diskp() && g_disk_targa)
        {
            return main_state::CONTINUE;  // disk video and targa, nothing to save
        }

        int oldsxoffs, oldsyoffs, oldxdots, oldydots, oldpx, oldpy;
        GENEBASE gene[NUMGENES];
        copy_genes_from_bank(gene);
        oldsxoffs = g_logical_screen_x_offset;
        oldsyoffs = g_logical_screen_y_offset;
        oldxdots = g_logical_screen_x_dots;
        oldydots = g_logical_screen_y_dots;
        oldpx = g_evolve_param_grid_x;
        oldpy = g_evolve_param_grid_y;
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_offset = g_logical_screen_y_offset;
        g_logical_screen_x_dots = g_screen_x_dots;
        g_logical_screen_y_dots = g_screen_y_dots; // for full screen save and pointer move stuff
        g_evolve_param_grid_y = g_evolve_image_grid_size / 2;
        g_evolve_param_grid_x = g_evolve_param_grid_y;
        param_history(1); // restore old history
        fiddleparms(gene, 0);
        drawparmbox(1);
        savetodisk(g_save_filename);
        g_evolve_param_grid_x = oldpx;
        g_evolve_param_grid_y = oldpy;
        param_history(1); // restore old history
        fiddleparms(gene, unspiralmap());
        g_logical_screen_x_offset = oldsxoffs;
        g_logical_screen_y_offset = oldsyoffs;
        g_logical_screen_x_dots = oldxdots;
        g_logical_screen_y_dots = oldydots;
        copy_genes_to_bank(gene);
        return main_state::CONTINUE;
    }

    case 'r':                    // restore-from
        g_compare_gif = false;
        *frommandel = false;
        g_browsing = false;
        if (*kbdchar == 'r')
        {
            if (g_debug_flag == debug_flags::force_disk_restore_not_save)
            {
                g_compare_gif = true;
                g_overlay_3d = true;
                if (g_init_batch == batch_modes::SAVE)
                {
                    driver_stack_screen();   // save graphics image
                    g_read_filename = g_save_filename;
                    g_show_file = 0;
                    return main_state::RESTORE_START;
                }
            }
            else
            {
                g_compare_gif = false;
                g_overlay_3d = false;
            }
            g_display_3d = display_3d_modes::NONE;
        }
        driver_stack_screen();            // save graphics image
        if (g_overlay_3d)
        {
            *stacked = false;
        }
        else
        {
            *stacked = true;
        }
        if (g_resave_flag)
        {
            updatesavename(g_save_filename);      // do the pending increment
            g_resave_flag = 0;
            g_started_resaves = false;
        }
        g_show_file = -1;
        return main_state::RESTORE_START;
    case FIK_ENTER:                  // Enter
    case FIK_ENTER_2:                // Numeric-Keypad Enter
#ifdef XFRACT
        g_x_zoom_waiting = false;
#endif
        if (g_zoom_box_width != 0.0)
        {
            // do a zoom
            init_pan_or_recalc(false);
            *kbdmore = false;
        }
        if (g_calc_status != calc_status_value::COMPLETED)       // don't restart if image complete
        {
            *kbdmore = false;
        }
        break;
    case FIK_CTL_ENTER:              // control-Enter
    case FIK_CTL_ENTER_2:            // Control-Keypad Enter
        init_pan_or_recalc(true);
        *kbdmore = false;
        zoomout();                // calc corners for zooming out
        break;
    case FIK_INSERT:         // insert
        driver_set_for_text();           // force text mode
        return main_state::RESTART;
    case FIK_LEFT_ARROW:             // cursor left
    case FIK_RIGHT_ARROW:            // cursor right
    case FIK_UP_ARROW:               // cursor up
    case FIK_DOWN_ARROW:             // cursor down
        move_zoombox(*kbdchar);
        break;
    case FIK_CTL_LEFT_ARROW:           // Ctrl-cursor left
    case FIK_CTL_RIGHT_ARROW:          // Ctrl-cursor right
    case FIK_CTL_UP_ARROW:             // Ctrl-cursor up
    case FIK_CTL_DOWN_ARROW:           // Ctrl-cursor down
        // borrow ctrl cursor keys for moving selection box
        // in evolver mode
        if (g_box_count)
        {
            GENEBASE gene[NUMGENES];
            copy_genes_from_bank(gene);
            if (g_evolving & FIELDMAP)
            {
                if (*kbdchar == FIK_CTL_LEFT_ARROW)
                {
                    g_evolve_param_grid_x--;
                }
                if (*kbdchar == FIK_CTL_RIGHT_ARROW)
                {
                    g_evolve_param_grid_x++;
                }
                if (*kbdchar == FIK_CTL_UP_ARROW)
                {
                    g_evolve_param_grid_y--;
                }
                if (*kbdchar == FIK_CTL_DOWN_ARROW)
                {
                    g_evolve_param_grid_y++;
                }
                if (g_evolve_param_grid_x <0)
                {
                    g_evolve_param_grid_x = g_evolve_image_grid_size -1;
                }
                if (g_evolve_param_grid_x > (g_evolve_image_grid_size -1))
                {
                    g_evolve_param_grid_x = 0;
                }
                if (g_evolve_param_grid_y < 0)
                {
                    g_evolve_param_grid_y = g_evolve_image_grid_size -1;
                }
                if (g_evolve_param_grid_y > (g_evolve_image_grid_size -1))
                {
                    g_evolve_param_grid_y = 0;
                }
                int grout = !((g_evolving & NOGROUT)/NOGROUT) ;
                g_logical_screen_x_offset = g_evolve_param_grid_x * (int)(g_logical_screen_x_size_dots+1+grout);
                g_logical_screen_y_offset = g_evolve_param_grid_y * (int)(g_logical_screen_y_size_dots+1+grout);

                param_history(1); // restore old history
                fiddleparms(gene, unspiralmap()); // change all parameters
                // to values appropriate to the image selected
                set_evolve_ranges();
                chgboxi(0, 0);
                drawparmbox(0);
            }
            copy_genes_to_bank(gene);
        }
        else                         // if no zoombox, scroll by arrows
        {
            move_zoombox(*kbdchar);
        }
        break;
    case FIK_CTL_HOME:               // Ctrl-home
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            i = key_count(FIK_CTL_HOME);
            if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
            {
                g_zoom_box_skew = -0.48;
            }
        }
        break;
    case FIK_CTL_END:                // Ctrl-end
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            i = key_count(FIK_CTL_END);
            if ((g_zoom_box_skew += 0.02 * i) > 0.48)
            {
                g_zoom_box_skew = 0.48;
            }
        }
        break;
    case FIK_CTL_PAGE_UP:
        if (g_evolve_param_box_count)
        {
            g_evolve_param_zoom -= 1.0;
            if (g_evolve_param_zoom < 1.0)
            {
                g_evolve_param_zoom = 1.0;
            }
            drawparmbox(0);
            set_evolve_ranges();
        }
        break;
    case FIK_CTL_PAGE_DOWN:
        if (g_evolve_param_box_count)
        {
            g_evolve_param_zoom += 1.0;
            if (g_evolve_param_zoom > (double) g_evolve_image_grid_size /2.0)
            {
                g_evolve_param_zoom = (double) g_evolve_image_grid_size /2.0;
            }
            drawparmbox(0);
            set_evolve_ranges();
        }
        break;

    case FIK_PAGE_UP:                // page up
        if (g_zoom_off)
        {
            if (g_zoom_box_width == 0)
            {
                // start zoombox
                g_zoom_box_height = 1;
                g_zoom_box_width = g_zoom_box_height;
                g_zoom_box_rotation = 0;
                g_zoom_box_skew = g_zoom_box_rotation;
                g_zoom_box_x = 0;
                g_zoom_box_y = 0;
                find_special_colors();
                g_box_color = g_color_bright;
                if (g_evolving & FIELDMAP)
                {
                    // set screen view params back (previously changed to allow full screen saves in viewwindow mode)
                    int grout = !((g_evolving & NOGROUT) / NOGROUT);
                    g_logical_screen_x_offset = g_evolve_param_grid_x * (int)(g_logical_screen_x_size_dots+1+grout);
                    g_logical_screen_y_offset = g_evolve_param_grid_y * (int)(g_logical_screen_y_size_dots+1+grout);
                    SetupParamBox();
                    drawparmbox(0);
                }
                moveboxf(0.0, 0.0); // force scrolling
            }
            else
            {
                resizebox(0 - key_count(FIK_PAGE_UP));
            }
        }
        break;
    case FIK_PAGE_DOWN:              // page down
        if (g_box_count)
        {
            if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)
            {
                // end zoombox
                g_zoom_box_width = 0;
                if (g_evolving & FIELDMAP)
                {
                    drawparmbox(1); // clear boxes off screen
                    ReleaseParamBox();
                }
            }
            else
            {
                resizebox(key_count(FIK_PAGE_DOWN));
            }
        }
        break;
    case FIK_CTL_MINUS:              // Ctrl-kpad-
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            g_zoom_box_rotation += key_count(FIK_CTL_MINUS);
        }
        break;
    case FIK_CTL_PLUS:               // Ctrl-kpad+
        if (g_box_count && (g_cur_fractal_specific->flags & NOROTATE) == 0)
        {
            g_zoom_box_rotation -= key_count(FIK_CTL_PLUS);
        }
        break;
    case FIK_CTL_INSERT:             // Ctrl-ins
        g_box_color += key_count(FIK_CTL_INSERT);
        break;
    case FIK_CTL_DEL:                // Ctrl-del
        g_box_color -= key_count(FIK_CTL_DEL);
        break;

    /* grabbed a couple of video mode keys, user can change to these using
        delete and the menu if necessary */

    case FIK_F2: // halve mutation params and regen
        g_evolve_max_random_mutation = g_evolve_max_random_mutation / 2;
        g_evolve_x_parameter_range = g_evolve_x_parameter_range / 2;
        g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range / 2;
        g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case FIK_F3: //double mutation parameters and regenerate
    {
        double centerx, centery;
        g_evolve_max_random_mutation = g_evolve_max_random_mutation * 2;
        centerx = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
        g_evolve_x_parameter_range = g_evolve_x_parameter_range * 2;
        g_evolve_new_x_parameter_offset = centerx - g_evolve_x_parameter_range / 2;
        centery = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range * 2;
        g_evolve_new_y_parameter_offset = centery - g_evolve_y_parameter_range / 2;
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;
    }

    case FIK_F4: //decrement  gridsize and regen
        if (g_evolve_image_grid_size > 3)
        {
            g_evolve_image_grid_size = g_evolve_image_grid_size - 2;  // evolve_image_grid_size must have odd value only
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;

    case FIK_F5: // increment gridsize and regen
        if (g_evolve_image_grid_size < (g_screen_x_dots / (MINPIXELS << 1)))
        {
            g_evolve_image_grid_size = g_evolve_image_grid_size + 2;
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;

    case FIK_F6: /* toggle all variables selected for random variation to center weighted variation and vice versa */
        for (auto &elem : g_gene_bank)
        {
            if (elem.mutate == variations::RANDOM)
            {
                elem.mutate = variations::WEIGHTED_RANDOM;
                continue;
            }
            if (elem.mutate == variations::WEIGHTED_RANDOM)
            {
                elem.mutate = variations::RANDOM;
            }
        }
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case FIK_ALT_1: // alt + number keys set mutation level
    case FIK_ALT_2:
    case FIK_ALT_3:
    case FIK_ALT_4:
    case FIK_ALT_5:
    case FIK_ALT_6:
    case FIK_ALT_7:
        set_mutation_level(*kbdchar-1119);
        param_history(1); // restore old history
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        set_mutation_level(*kbdchar-(int)'0');
        param_history(1); // restore old history
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case '0': // mutation level 0 == turn off evolving
        g_evolving = 0;
        g_view_window = false;
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case FIK_DELETE:         // select video mode from list
        driver_stack_screen();
        *kbdchar = select_video_mode(g_adapter);
        if (check_vidmode_key(0, *kbdchar) >= 0)    // picked a new mode?
        {
            driver_discard_screen();
        }
        else
        {
            driver_unstack_screen();
        }
        // fall through

    default:             // other (maybe valid Fn key
        k = check_vidmode_key(0, *kbdchar);
        if (k >= 0)
        {
            g_adapter = k;
            if (g_video_table[g_adapter].colors != g_colors)
            {
                g_save_dac = 0;
            }
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            *kbdmore = false;
            return main_state::CONTINUE;
        }
        break;
    }                            // end of the big evolver switch
    return main_state::NOTHING;
}

static int call_line3d(BYTE *pixels, int linelen)
{
    // this routine exists because line3d might be in an overlay
    return line3d(pixels, linelen);
}

// do all pending movement at once for smooth mouse diagonal moves
static void move_zoombox(int keynum)
{
    int vertical, horizontal, getmore;
    horizontal = 0;
    vertical = horizontal;
    getmore = 1;
    while (getmore)
    {
        switch (keynum)
        {
        case FIK_LEFT_ARROW:               // cursor left
            --horizontal;
            break;
        case FIK_RIGHT_ARROW:              // cursor right
            ++horizontal;
            break;
        case FIK_UP_ARROW:                 // cursor up
            --vertical;
            break;
        case FIK_DOWN_ARROW:               // cursor down
            ++vertical;
            break;
        case FIK_CTL_LEFT_ARROW:             // Ctrl-cursor left
            horizontal -= 8;
            break;
        case FIK_CTL_RIGHT_ARROW:             // Ctrl-cursor right
            horizontal += 8;
            break;
        case FIK_CTL_UP_ARROW:               // Ctrl-cursor up
            vertical -= 8;
            break;
        case FIK_CTL_DOWN_ARROW:             // Ctrl-cursor down
            vertical += 8;
            break;                      // += 8 needed by VESA scrolling
        default:
            getmore = 0;
        }
        if (getmore)
        {
            if (getmore == 2)                // eat last key used
            {
                driver_get_key();
            }
            getmore = 2;
            keynum = driver_key_pressed();         // next pending key
        }
    }
    if (g_box_count)
    {
        moveboxf((double)horizontal/g_logical_screen_x_size_dots, (double)vertical/g_logical_screen_y_size_dots);
    }
#ifndef XFRACT
    else                                 // if no zoombox, scroll by arrows
    {
        scroll_relative(horizontal, vertical);
    }
#endif
}

// displays differences between current image file and new image
static FILE *cmp_fp;
static int errcount;
int cmp_line(BYTE *pixels, int linelen)
{
    int row;
    int oldcolor;
    row = g_row_count++;
    if (row == 0)
    {
        errcount = 0;
        cmp_fp = dir_fopen(g_working_dir.c_str(), "cmperr", (g_init_batch != batch_modes::NONE) ? "a" : "w");
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
        oldcolor = getcolor(col, row);
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
            ++errcount;
            if (g_init_batch == batch_modes::NONE)
            {
                fprintf(cmp_fp, "#%5d col %3d row %3d old %3d new %3d\n",
                        errcount, col, row, oldcolor, pixels[col]);
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
        fprintf(cmp_fp, "%s compare to %s has %5d errs\n",
                timestring, g_read_filename.c_str(), errcount);
    }
    fclose(cmp_fp);
}

void clear_zoombox()
{
    g_zoom_box_width = 0;
    drawbox(false);
    reset_zoom_corners();
}

void reset_zoom_corners()
{
    g_x_min = g_save_x_min;
    g_x_max = g_save_x_max;
    g_x_3rd = g_save_x_3rd;
    g_y_max = g_save_y_max;
    g_y_min = g_save_y_min;
    g_y_3rd = g_save_y_3rd;
    if (bf_math != bf_math_type::NONE)
    {
        copy_bf(g_bf_x_min, g_bf_save_x_min);
        copy_bf(g_bf_x_max, g_bf_save_x_max);
        copy_bf(g_bf_y_min, g_bf_save_y_min);
        copy_bf(g_bf_y_max, g_bf_save_y_max);
        copy_bf(g_bf_x_3rd, g_bf_save_x_3rd);
        copy_bf(g_bf_y_3rd, g_bf_save_y_3rd);
    }
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

static std::vector<HISTORY> history;
int g_max_image_history = 10;

void history_init()
{
    history.resize(g_max_image_history);
}

static void save_history_info()
{
    if (g_max_image_history <= 0 || bf_math != bf_math_type::NONE)
    {
        return;
    }
    HISTORY last = history[saveptr];

    HISTORY current;
    memset((void *)&current, 0, sizeof(HISTORY));
    current.fractal_type         = (short)g_fractal_type                  ;
    current.xmin                 = g_x_min                     ;
    current.xmax                 = g_x_max                     ;
    current.ymin                 = g_y_min                     ;
    current.ymax                 = g_y_max                     ;
    current.creal                = g_params[0]                  ;
    current.cimag                = g_params[1]                  ;
    current.dparm3               = g_params[2]                  ;
    current.dparm4               = g_params[3]                  ;
    current.dparm5               = g_params[4]                  ;
    current.dparm6               = g_params[5]                  ;
    current.dparm7               = g_params[6]                  ;
    current.dparm8               = g_params[7]                  ;
    current.dparm9               = g_params[8]                  ;
    current.dparm10              = g_params[9]                  ;
    current.fillcolor            = (short)g_fill_color                 ;
    current.potential[0]         = g_potential_params[0]               ;
    current.potential[1]         = g_potential_params[1]               ;
    current.potential[2]         = g_potential_params[2]               ;
    current.rflag                = (short) (g_random_seed_flag ? 1 : 0);
    current.rseed                = (short)g_random_seed                     ;
    current.inside               = (short)g_inside_color                    ;
    current.logmap               = g_log_map_flag                   ;
    current.invert[0]            = g_inversion[0]              ;
    current.invert[1]            = g_inversion[1]              ;
    current.invert[2]            = g_inversion[2]              ;
    current.decomp               = (short)g_decomp[0];                ;
    current.biomorph             = (short)g_biomorph                  ;
    current.symmetry             = (short)g_force_symmetry             ;
    current.init3d[0]            = (short)g_init_3d[0]                 ;
    current.init3d[1]            = (short)g_init_3d[1]                 ;
    current.init3d[2]            = (short)g_init_3d[2]                 ;
    current.init3d[3]            = (short)g_init_3d[3]                 ;
    current.init3d[4]            = (short)g_init_3d[4]                 ;
    current.init3d[5]            = (short)g_init_3d[5]                 ;
    current.init3d[6]            = (short)g_init_3d[6]                 ;
    current.init3d[7]            = (short)g_init_3d[7]                 ;
    current.init3d[8]            = (short)g_init_3d[8]                 ;
    current.init3d[9]            = (short)g_init_3d[9]                 ;
    current.init3d[10]           = (short)g_init_3d[10]               ;
    current.init3d[11]           = (short)g_init_3d[12]               ;
    current.init3d[12]           = (short)g_init_3d[13]               ;
    current.init3d[13]           = (short)g_init_3d[14]               ;
    current.init3d[14]           = (short)g_init_3d[15]               ;
    current.init3d[15]           = (short)g_init_3d[16]               ;
    current.previewfactor        = (short)g_preview_factor             ;
    current.xtrans               = (short)g_adjust_3d_x                    ;
    current.ytrans               = (short)g_adjust_3d_y                    ;
    current.red_crop_left        = (short)g_red_crop_left             ;
    current.red_crop_right       = (short)g_red_crop_right            ;
    current.blue_crop_left       = (short)g_blue_crop_left            ;
    current.blue_crop_right      = (short)g_blue_crop_right           ;
    current.red_bright           = (short)g_red_bright                ;
    current.blue_bright          = (short)g_blue_bright               ;
    current.xadjust              = (short)g_converge_x_adjust                   ;
    current.yadjust              = (short)g_converge_y_adjust                   ;
    current.eyeseparation        = (short)g_eye_separation             ;
    current.glassestype          = (short)g_glasses_type               ;
    current.outside              = (short)g_outside_color                   ;
    current.x3rd                 = g_x_3rd                     ;
    current.y3rd                 = g_y_3rd                     ;
    current.stdcalcmode          = g_user_std_calc_mode               ;
    current.three_pass           = g_three_pass ? 1 : 0;
    current.stoppass             = (short)g_stop_pass;
    current.distest              = g_distance_estimator                   ;
    current.trigndx[0]           = static_cast<BYTE>(g_trig_index[0]);
    current.trigndx[1]           = static_cast<BYTE>(g_trig_index[1]);
    current.trigndx[2]           = static_cast<BYTE>(g_trig_index[2]);
    current.trigndx[3]           = static_cast<BYTE>(g_trig_index[3]);
    current.finattract           = (short) (g_finite_attractor ? 1 : 0);
    current.initorbit[0]         = g_init_orbit.x               ;
    current.initorbit[1]         = g_init_orbit.y               ;
    current.useinitorbit         = static_cast<char>(g_use_init_orbit);
    current.periodicity          = (short)g_periodicity_check          ;
    current.pot16bit             = (short) (g_disk_16_bit ? 1 : 0);
    current.release              = (short)g_release                   ;
    current.save_release         = (short)g_release              ;
    current.display_3d           = g_display_3d;
    current.ambient              = (short)g_ambient                   ;
    current.randomize            = (short)g_randomize_3d                 ;
    current.haze                 = (short)g_haze                      ;
    current.transparent[0]       = (short)g_transparent_color_3d[0]            ;
    current.transparent[1]       = (short)g_transparent_color_3d[1]            ;
    current.rotate_lo            = (short)g_color_cycle_range_lo                 ;
    current.rotate_hi            = (short)g_color_cycle_range_hi                 ;
    current.distestwidth         = (short)g_distance_estimator_width_factor              ;
    current.mxmaxfp              = g_julibrot_x_max                   ;
    current.mxminfp              = g_julibrot_x_min                   ;
    current.mymaxfp              = g_julibrot_y_max                   ;
    current.myminfp              = g_julibrot_y_min                   ;
    current.zdots                = (short)g_julibrot_z_dots                         ;
    current.originfp             = g_julibrot_origin_fp                  ;
    current.depthfp              = g_julibrot_depth_fp                      ;
    current.heightfp             = g_julibrot_height_fp                  ;
    current.widthfp              = g_julibrot_width_fp                      ;
    current.distfp               = g_julibrot_dist_fp                       ;
    current.eyesfp               = g_eyes_fp                       ;
    current.orbittype            = (short)g_new_orbit_type              ;
    current.juli3Dmode           = (short)g_julibrot_3d_mode                ;
    current.maxfn                = g_max_function                     ;
    current.major_method         = (short)g_major_method              ;
    current.minor_method         = (short)g_inverse_julia_minor_method              ;
    current.bailout              = g_bail_out                   ;
    current.bailoutest           = (short)g_bail_out_test                ;
    current.iterations           = g_max_iterations                     ;
    current.old_demm_colors      = (short) (g_old_demm_colors ? 1 : 0);
    current.logcalc              = (short)g_log_map_fly_calculate;
    current.ismand               = (short) (g_is_mandelbrot ? 1 : 0);
    current.closeprox            = g_close_proximity;
    current.nobof                = (short) (g_bof_match_book_images ? 0 : 1);
    current.orbit_delay          = (short)g_orbit_delay;
    current.orbit_interval       = g_orbit_interval;
    current.oxmin                = g_orbit_corner_min_x;
    current.oxmax                = g_orbit_corner_max_x;
    current.oymin                = g_orbit_corner_min_y;
    current.oymax                = g_orbit_corner_max_y;
    current.ox3rd                = g_orbit_corner_3_x;
    current.oy3rd                = g_orbit_corner_3_y;
    current.keep_scrn_coords     = (short) (g_keep_screen_coords ? 1 : 0);
    current.drawmode             = g_draw_mode;
    memcpy(current.dac, g_dac_box, 256*3);
    switch (g_fractal_type)
    {
    case fractal_type::FORMULA:
    case fractal_type::FFORMULA:
        strncpy(current.filename, g_formula_filename.c_str(), FILE_MAX_PATH);
        strncpy(current.itemname, g_formula_name.c_str(), ITEMNAMELEN+1);
        break;
    case fractal_type::IFS:
    case fractal_type::IFS3D:
        strncpy(current.filename, g_ifs_filename.c_str(), FILE_MAX_PATH);
        strncpy(current.itemname, g_ifs_name.c_str(), ITEMNAMELEN+1);
        break;
    case fractal_type::LSYSTEM:
        strncpy(current.filename, g_l_system_filename.c_str(), FILE_MAX_PATH);
        strncpy(current.itemname, g_l_system_name.c_str(), ITEMNAMELEN+1);
        break;
    default:
        *(current.filename) = 0;
        *(current.itemname) = 0;
        break;
    }
    if (historyptr == -1)        // initialize the history file
    {
        for (int i = 0; i < g_max_image_history; i++)
        {
            history[i] = current;
        }
        historyflag = false;
        historyptr = 0;
        saveptr = 0;   // initialize history ptr
    }
    else if (historyflag)
    {
        historyflag = false;            // coming from user history command, don't save
    }
    else if (memcmp(&current, &last, sizeof(HISTORY)))
    {
        if (++saveptr >= g_max_image_history)    // back to beginning of circular buffer
        {
            saveptr = 0;
        }
        if (++historyptr >= g_max_image_history)    // move user pointer in parallel
        {
            historyptr = 0;
        }
        history[saveptr] = current;
    }
}

static void restore_history_info(int i)
{
    if (g_max_image_history <= 0 || bf_math != bf_math_type::NONE)
    {
        return;
    }
    HISTORY last = history[i];
    g_invert = 0;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    g_resuming = false;
    g_fractal_type              = static_cast<fractal_type>(last.fractal_type);
    g_x_min                 = last.xmin           ;
    g_x_max                 = last.xmax           ;
    g_y_min                 = last.ymin           ;
    g_y_max                 = last.ymax           ;
    g_params[0]              = last.creal          ;
    g_params[1]              = last.cimag          ;
    g_params[2]              = last.dparm3         ;
    g_params[3]              = last.dparm4         ;
    g_params[4]              = last.dparm5         ;
    g_params[5]              = last.dparm6         ;
    g_params[6]              = last.dparm7         ;
    g_params[7]              = last.dparm8         ;
    g_params[8]              = last.dparm9         ;
    g_params[9]              = last.dparm10        ;
    g_fill_color             = last.fillcolor      ;
    g_potential_params[0]           = last.potential[0]   ;
    g_potential_params[1]           = last.potential[1]   ;
    g_potential_params[2]           = last.potential[2]   ;
    g_random_seed_flag                 = last.rflag != 0;
    g_random_seed                 = last.rseed          ;
    g_inside_color                = last.inside         ;
    g_log_map_flag               = last.logmap         ;
    g_inversion[0]          = last.invert[0]      ;
    g_inversion[1]          = last.invert[1]      ;
    g_inversion[2]          = last.invert[2]      ;
    g_decomp[0]             = last.decomp         ;
    g_user_biomorph_value          = last.biomorph       ;
    g_biomorph              = last.biomorph       ;
    g_force_symmetry         = static_cast<symmetry_type>(last.symmetry);
    g_init_3d[0]             = last.init3d[0]      ;
    g_init_3d[1]             = last.init3d[1]      ;
    g_init_3d[2]             = last.init3d[2]      ;
    g_init_3d[3]             = last.init3d[3]      ;
    g_init_3d[4]             = last.init3d[4]      ;
    g_init_3d[5]             = last.init3d[5]      ;
    g_init_3d[6]             = last.init3d[6]      ;
    g_init_3d[7]             = last.init3d[7]      ;
    g_init_3d[8]             = last.init3d[8]      ;
    g_init_3d[9]             = last.init3d[9]      ;
    g_init_3d[10]            = last.init3d[10]     ;
    g_init_3d[12]            = last.init3d[11]     ;
    g_init_3d[13]            = last.init3d[12]     ;
    g_init_3d[14]            = last.init3d[13]     ;
    g_init_3d[15]            = last.init3d[14]     ;
    g_init_3d[16]            = last.init3d[15]     ;
    g_preview_factor         = last.previewfactor  ;
    g_adjust_3d_x                = last.xtrans         ;
    g_adjust_3d_y                = last.ytrans         ;
    g_red_crop_left         = last.red_crop_left  ;
    g_red_crop_right        = last.red_crop_right ;
    g_blue_crop_left        = last.blue_crop_left ;
    g_blue_crop_right       = last.blue_crop_right;
    g_red_bright            = last.red_bright     ;
    g_blue_bright           = last.blue_bright    ;
    g_converge_x_adjust               = last.xadjust        ;
    g_converge_y_adjust               = last.yadjust        ;
    g_eye_separation      = last.eyeseparation  ;
    g_glasses_type        = last.glassestype    ;
    g_outside_color               = last.outside        ;
    g_x_3rd                 = last.x3rd           ;
    g_y_3rd                 = last.y3rd           ;
    g_user_std_calc_mode       = last.stdcalcmode    ;
    g_std_calc_mode           = last.stdcalcmode    ;
    g_three_pass            = last.three_pass != 0;
    g_stop_pass              = last.stoppass       ;
    g_distance_estimator               = last.distest        ;
    g_user_distance_estimator_value           = last.distest        ;
    g_trig_index[0]            = static_cast<trig_fn>(last.trigndx[0]);
    g_trig_index[1]            = static_cast<trig_fn>(last.trigndx[1]);
    g_trig_index[2]            = static_cast<trig_fn>(last.trigndx[2]);
    g_trig_index[3]            = static_cast<trig_fn>(last.trigndx[3]);
    g_finite_attractor            = last.finattract != 0;
    g_init_orbit.x           = last.initorbit[0]   ;
    g_init_orbit.y           = last.initorbit[1]   ;
    g_use_init_orbit          = static_cast<init_orbit_mode>(last.useinitorbit);
    g_periodicity_check      = last.periodicity    ;
    g_user_periodicity_value  = last.periodicity    ;
    g_disk_16_bit             = last.pot16bit != 0;
    g_release             = last.release        ;
    g_display_3d = last.display_3d;
    g_ambient               = last.ambient        ;
    g_randomize_3d             = last.randomize      ;
    g_haze                  = last.haze           ;
    g_transparent_color_3d[0]        = last.transparent[0] ;
    g_transparent_color_3d[1]        = last.transparent[1] ;
    g_color_cycle_range_lo             = last.rotate_lo      ;
    g_color_cycle_range_hi             = last.rotate_hi      ;
    g_distance_estimator_width_factor          = last.distestwidth   ;
    g_julibrot_x_max               = last.mxmaxfp        ;
    g_julibrot_x_min               = last.mxminfp        ;
    g_julibrot_y_max               = last.mymaxfp        ;
    g_julibrot_y_min               = last.myminfp        ;
    g_julibrot_z_dots                 = last.zdots          ;
    g_julibrot_origin_fp              = last.originfp       ;
    g_julibrot_depth_fp               = last.depthfp        ;
    g_julibrot_height_fp              = last.heightfp       ;
    g_julibrot_width_fp               = last.widthfp        ;
    g_julibrot_dist_fp                = last.distfp         ;
    g_eyes_fp                = last.eyesfp         ;
    g_new_orbit_type          = static_cast<fractal_type>(last.orbittype);
    g_julibrot_3d_mode            = last.juli3Dmode     ;
    g_max_function                 = last.maxfn          ;
    g_major_method          = static_cast<Major>(last.major_method);
    g_inverse_julia_minor_method          = static_cast<Minor>(last.minor_method);
    g_bail_out               = last.bailout        ;
    g_bail_out_test            = static_cast<bailouts>(last.bailoutest);
    g_max_iterations                 = last.iterations     ;
    g_old_demm_colors       = last.old_demm_colors != 0;
    g_cur_fractal_specific    = &g_fractal_specific[static_cast<int>(g_fractal_type)];
    g_potential_flag               = (g_potential_params[0] != 0.0);
    if (g_inversion[0] != 0.0)
    {
        g_invert = 3;
    }
    g_log_map_fly_calculate = last.logcalc;
    g_is_mandelbrot = last.ismand != 0;
    g_close_proximity = last.closeprox;
    g_bof_match_book_images = last.nobof == 0;
    g_orbit_delay = last.orbit_delay;
    g_orbit_interval = last.orbit_interval;
    g_orbit_corner_min_x = last.oxmin;
    g_orbit_corner_max_x = last.oxmax;
    g_orbit_corner_min_y = last.oymin;
    g_orbit_corner_max_y = last.oymax;
    g_orbit_corner_3_x = last.ox3rd;
    g_orbit_corner_3_y = last.oy3rd;
    g_keep_screen_coords = last.keep_scrn_coords != 0;
    if (g_keep_screen_coords)
    {
        g_set_orbit_corners = true;
    }
    g_draw_mode = last.drawmode;
    g_user_float_flag = g_cur_fractal_specific->isinteger == 0;
    memcpy(g_dac_box, last.dac, 256*3);
    memcpy(g_old_dac_box, last.dac, 256*3);
    if (g_map_specified)
    {
        for (int i = 0; i < 256; ++i)
        {
            g_map_clut[i][0] = last.dac[i][0];
            g_map_clut[i][1] = last.dac[i][1];
            g_map_clut[i][2] = last.dac[i][2];
        }
    }
    spindac(0, 1);
    if (g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
    {
        g_save_dac = 0;
    }
    else
    {
        g_save_dac = 1;
    }
    switch (g_fractal_type)
    {
    case fractal_type::FORMULA:
    case fractal_type::FFORMULA:
        g_formula_filename = last.filename;
        g_formula_name = last.itemname;
        if (g_formula_name.length() > ITEMNAMELEN)
        {
            g_formula_name.resize(ITEMNAMELEN);
        }
        break;
    case fractal_type::IFS:
    case fractal_type::IFS3D:
        g_ifs_filename = last.filename;
        g_ifs_name = last.itemname;
        if (g_ifs_name.length() > ITEMNAMELEN)
        {
            g_ifs_name.resize(ITEMNAMELEN);
        }
        break;
    case fractal_type::LSYSTEM:
        g_l_system_filename = last.filename;
        g_l_system_name = last.itemname;
        if (g_l_system_name.length() > ITEMNAMELEN)
        {
            g_l_system_name.resize(ITEMNAMELEN);
        }
        break;
    default:
        break;
    }
}
