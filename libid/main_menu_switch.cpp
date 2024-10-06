// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "main_menu_switch.h"

#include "ant.h"
#include "calcfrac.h"
#include "cellular.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "find_special_colors.h"
#include "fixed_pt.h"
#include "flip_image.h"
#include "fractalp.h"
#include "fractype.h"
#include "framain2.h"
#include "frothy_basin.h"
#include "get_3d_params.h"
#include "get_browse_params.h"
#include "get_cmd_string.h"
#include "get_commands.h"
#include "get_fract_type.h"
#include "get_rds_params.h"
#include "get_sound_params.h"
#include "get_toggles.h"
#include "get_toggles2.h"
#include "get_view_params.h"
#include "history.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "jb.h"
#include "jiim.h"
#include "load_params.h"
#include "loadfile.h"
#include "lorenz.h"
#include "make_batch_file.h"
#include "merge_path_names.h"
#include "parser.h"
#include "passes_options.h"
#include "rotate.h"
#include "select_video_mode.h"
#include "spindac.h"
#include "starfield.h"
#include "stereo.h"
#include "update_save_name.h"
#include "value_saver.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstring>
#include <ctime>
#include <string>

static bool look(bool *stacked)
{
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP_BROWSE;
    switch (fgetwindow())
    {
    case ID_KEY_ENTER:
    case ID_KEY_ENTER_2:
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
    case ID_KEY_ESC:
    case 'l':              // turn it off
    case 'L':
        g_browsing = false;
        g_help_mode = old_help_mode;
        break;

    case 's':
        g_browsing = false;
        g_help_mode = old_help_mode;
        save_image(g_save_filename);
        break;

    default:               // or no files found, leave the state of browsing alone
        break;
    }

    return false;
}

static void toggle_mandelbrot_julia(bool &kbd_more, bool &from_mandel)
{
    static double s_j_x_min;
    static double s_j_x_max;
    static double s_j_y_min;
    static double s_j_y_max; // "Julia mode" entry point
    static double s_j_x_3rd;
    static double s_j_y_3rd;

    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        if (g_is_mandelbrot)
        {
            g_fractal_specific[+g_fractal_type].tojulia = g_fractal_type;
            g_fractal_specific[+g_fractal_type].tomandel = fractal_type::NOFRACTAL;
            g_is_mandelbrot = false;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].tojulia = fractal_type::NOFRACTAL;
            g_fractal_specific[+g_fractal_type].tomandel = g_fractal_type;
            g_is_mandelbrot = true;
        }
    }
    if (g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 &&
        g_params[1] == 0.0)
    {
        // switch to corresponding Julia set
        g_has_inverse =
            (g_fractal_type == fractal_type::MANDEL || g_fractal_type == fractal_type::MANDELFP) &&
            g_bf_math == bf_math_type::NONE;
        clear_zoom_box();
        Jiim(jiim_types::JIIM);
        const int key = driver_get_key(); // flush keyboard buffer
        if (key != ID_KEY_SPACE)
        {
            driver_unget_key(key);
            return;
        }
        g_fractal_type = g_cur_fractal_specific->tojulia;
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        if (g_julia_c_x == JULIA_C_NOT_SET || g_julia_c_y == JULIA_C_NOT_SET)
        {
            g_params[0] = (g_x_max + g_x_min) / 2;
            g_params[1] = (g_y_max + g_y_min) / 2;
        }
        else
        {
            g_params[0] = g_julia_c_x;
            g_params[1] = g_julia_c_y;
            g_julia_c_y = JULIA_C_NOT_SET;
            g_julia_c_x = JULIA_C_NOT_SET;
        }
        s_j_x_min = g_save_x_min;
        s_j_x_max = g_save_x_max;
        s_j_y_max = g_save_y_max;
        s_j_y_min = g_save_y_min;
        s_j_x_3rd = g_save_x_3rd;
        s_j_y_3rd = g_save_y_3rd;
        from_mandel = true;
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
        kbd_more = false;
    }
    else if (g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
    {
        // switch to corresponding Mandel set
        g_fractal_type = g_cur_fractal_specific->tomandel;
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        if (from_mandel)
        {
            g_x_min = s_j_x_min;
            g_x_max = s_j_x_max;
            g_y_min = s_j_y_min;
            g_y_max = s_j_y_max;
            g_x_3rd = s_j_x_3rd;
            g_y_3rd = s_j_y_3rd;
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
        kbd_more = false;
    }
    else
    {
        driver_buzzer(buzzer_codes::PROBLEM); // can't switch
    }
}

static bool new_fractal_type(bool &from_mandel)
{
    g_julibrot = false;
    clear_zoom_box();
    driver_stack_screen();
    if (const int type = get_fract_type(); type >= 0)
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
        save_param_history();
        if (type == 0)
        {
            g_init_mode = g_adapter;
            from_mandel = false;
        }
        else if (g_init_mode < 0)   // it is supposed to be...
        {
            driver_set_for_text();     // reset to text mode
        }
        return true;
    }
    driver_unstack_screen();
    return false;
}

static void prompt_options(bool &kbd_more, const int kbdchar)
{
    const long old_maxit = g_max_iterations;
    clear_zoom_box();
    if (g_from_text)
    {
        g_from_text = false;
    }
    else
    {
        driver_stack_screen();
    }
    int i;
    if (kbdchar == 'x')
    {
        i = get_toggles();
    }
    else if (kbdchar == 'y')
    {
        i = get_toggles2();
    }
    else if (kbdchar == 'p')
    {
        i = passes_options();
    }
    else if (kbdchar == 'z')
    {
        i = get_fract_params(true);
    }
    else if (kbdchar == 'v')
    {
        i = get_view_params(); // get the parameters
    }
    else if (kbdchar == ID_KEY_CTL_B)
    {
        i = get_browse_params();
    }
    else if (kbdchar == ID_KEY_CTL_E)
    {
        i = get_evolve_Parms();
        if (i > 0)
        {
            g_start_show_orbit = false;
            g_sound_flag &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); // turn off only x,y,z
            g_log_map_auto_calculate = false; // turn it off
        }
    }
    else if (kbdchar == ID_KEY_CTL_F)
    {
        i = get_sound_params();
    }
    else
    {
        i = get_cmd_string();
    }
    driver_unstack_screen();
    if (g_evolving != evolution_mode_flags::NONE && g_truecolor)
    {
        g_truecolor = false;          // truecolor doesn't play well with the evolver
    }
    if (g_max_iterations > old_maxit
        && g_inside_color >= COLOR_BLACK
        && g_calc_status == calc_status_value::COMPLETED
        && g_cur_fractal_specific->calctype == standard_fractal
        && !g_log_map_flag
        && !g_truecolor     // recalc not yet implemented with truecolor
        && (g_user_std_calc_mode != 't' || g_fill_color <= -1) // tesseral with fill doesn't work
        && g_user_std_calc_mode != 'o'
        && i == 1 // nothing else changed
        && g_outside_color != ATAN)
    {
        g_quick_calc = true;
        g_old_std_calc_mode = g_user_std_calc_mode;
        g_user_std_calc_mode = '1';
        kbd_more = false;
        g_calc_status = calc_status_value::RESUMABLE;
    }
    else if (i > 0)
    {
        // time to redraw?
        g_quick_calc = false;
        save_param_history();
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
}

static bool begin_ant()
{
    clear_zoom_box();
    double oldparm[MAX_PARAMS];
    fractal_type oldtype = g_fractal_type;
    for (int j = 0; j < MAX_PARAMS; ++j)
    {
        oldparm[j] = g_params[j];
    }
    if (g_fractal_type != fractal_type::ANT)
    {
        g_fractal_type = fractal_type::ANT;
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        load_params(g_fractal_type);
    }
    if (!g_from_text)
    {
        driver_stack_screen();
    }
    g_from_text = false;
    int err = get_fract_params(true);
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
    for (int j = 0; j < MAX_PARAMS; ++j)
    {
        g_params[j] = oldparm[j];
    }
    return err >= 0;
}

static void toggle_float()
{
    if (!g_user_float_flag)
    {
        g_user_float_flag = true;
    }
    else if (g_std_calc_mode != 'o')     // don't go there
    {
        g_user_float_flag = false;
    }
    g_init_mode = g_adapter;
}

static void show_orbit_window()
{
    // must use standard fractal and have a float variant
    if ((g_fractal_specific[+g_fractal_type].calctype == standard_fractal
            || g_fractal_specific[+g_fractal_type].calctype == calcfroth)
        && (g_fractal_specific[+g_fractal_type].isinteger == 0 ||
             g_fractal_specific[+g_fractal_type].tofloat != fractal_type::NOFRACTAL)
        && (g_bf_math == bf_math_type::NONE) // for now no arbitrary precision support
        && !(g_is_true_color && g_true_mode != true_color_mode::default_color))
    {
        clear_zoom_box();
        Jiim(jiim_types::ORBIT);
    }
}

static void inverse_julia_toggle(bool &kbd_more)
{
    // if the inverse types proliferate, something more elegant will be needed
    if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP ||
        g_fractal_type == fractal_type::INVERSEJULIA)
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
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        g_zoom_off = true;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        kbd_more = false;
    }
    else
    {
        driver_buzzer(buzzer_codes::PROBLEM);
    }
}

static main_state get_history(bool &stacked, int kbd_char)
{
    if (g_filename_stack_index >= 1)
    {
        // go back one file if somewhere to go (ie. browsing)
        g_filename_stack_index--;
        while (g_file_name_stack[g_filename_stack_index].empty() && g_filename_stack_index >= 0)
        {
            g_filename_stack_index--;
        }
        if (g_filename_stack_index < 0) // oops, must have deleted first one
        {
            return main_state::NOTHING;
        }
        g_browse_name = g_file_name_stack[g_filename_stack_index];
        merge_pathnames(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
        g_browsing = true;
        g_browse_sub_images = true;
        g_show_file = 0;
        if (g_ask_video)
        {
            driver_stack_screen(); // save graphics image
            stacked = true;
        }
        return main_state::RESTORE_START;
    }

    if (g_max_image_history > 0 && g_bf_math == bf_math_type::NONE)
    {
        if (kbd_char == '\\' || kbd_char == 'h')
        {
            if (--g_history_ptr < 0)
            {
                g_history_ptr = g_max_image_history - 1;
            }
        }
        if (kbd_char == ID_KEY_CTL_BACKSLASH || kbd_char == ID_KEY_BACKSPACE)
        {
            if (++g_history_ptr >= g_max_image_history)
            {
                g_history_ptr = 0;
            }
        }
        restore_history_info(g_history_ptr);
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
        g_history_flag = true; // avoid re-store parms due to rounding errs
        return main_state::IMAGE_START;
    }

    return main_state::NOTHING;
}

static bool color_editing(bool &kbd_more)
{
    if (g_is_true_color && (g_init_batch == batch_modes::NONE))
    {
        // don't enter palette editor
        if (!load_palette())
        {
            kbd_more = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            return false;
        }

        return true;
    }

    clear_zoom_box();
    if (g_dac_box[0][0] != 255 && g_colors >= 16 && !driver_diskp())
    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_PALETTE_EDITOR};
        std::memcpy(g_old_dac_box, g_dac_box, 256 * 3);
        EditPalette();
        if (std::memcmp(g_old_dac_box, g_dac_box, 256 * 3) != 0)
        {
            g_color_state = color_state::UNKNOWN;
            save_history_info();
        }
    }
    return true;
}

static void restore_from_image(bool &from_mandel, int kbd_char, bool &stacked)
{
    g_compare_gif = false;
    from_mandel = false;
    g_browsing = false;
    if (kbd_char == 'r')
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
                return;
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
    stacked = !g_overlay_3d;
    if (g_resave_flag)
    {
        update_save_name(g_save_filename);      // do the pending increment
        g_resave_flag = 0;
        g_started_resaves = false;
    }
    g_show_file = -1;
}

static bool look_for_files(bool &stacked)
{
    if ((g_zoom_box_width != 0) || driver_diskp())
    {
        g_browsing = false;
        driver_buzzer(buzzer_codes::PROBLEM);             // can't browse if zooming or disk video
    }
    else if (look(&stacked))
    {
        return true;
    }
    return false;
}

static void request_zoom_in(bool &kbd_more)
{
    if (g_zoom_box_width != 0.0)
    {
        // do a zoom
        init_pan_or_recalc(false);
        kbd_more = false;
    }
    if (g_calc_status != calc_status_value::COMPLETED)       // don't restart if image complete
    {
        kbd_more = false;
    }
}

static void request_zoom_out(bool &kbd_more)
{
    init_pan_or_recalc(true);
    kbd_more = false;
    zoom_out();                // calc corners for zooming out
}

static void skew_zoom_left()
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
    {
        const int i = key_count(ID_KEY_CTL_HOME);
        if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
        {
            g_zoom_box_skew = -0.48;
        }
    }
}

static void skew_zoom_right()
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
    {
        const int i = key_count(ID_KEY_CTL_END);
        if ((g_zoom_box_skew += 0.02 * i) > 0.48)
        {
            g_zoom_box_skew = 0.48;
        }
    }
}

static void decrease_zoom_aspect()
{
    if (g_box_count)
    {
        change_box(0, -2 * key_count(ID_KEY_CTL_PAGE_UP));
    }
}

static void increase_zoom_aspect()
{
    if (g_box_count)
    {
        change_box(0, 2 * key_count(ID_KEY_CTL_PAGE_DOWN));
    }
}

static void zoom_box_in()
{
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
            g_evolve_param_grid_y = g_evolve_image_grid_size / 2;
            g_evolve_param_grid_x = g_evolve_param_grid_y;
            move_box(0.0, 0.0); // force scrolling
        }
        else
        {
            resize_box(0 - key_count(ID_KEY_PAGE_UP));
        }
    }
}

static void zoom_box_out()
{
    if (g_box_count)
    {
        if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)   // end zoombox
        {
            g_zoom_box_width = 0;
        }
        else
        {
            resize_box(key_count(ID_KEY_PAGE_DOWN));
        }
    }
}

static void zoom_box_increase_rotation()
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
    {
        g_zoom_box_rotation += key_count(ID_KEY_CTL_MINUS);
    }
}

static void zoom_box_decrease_rotation()
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
    {
        g_zoom_box_rotation -= key_count(ID_KEY_CTL_PLUS);
    }
}

static void zoom_box_increase_color()
{
    g_box_color += key_count(ID_KEY_CTL_INSERT);
}

static void zoom_box_decrease_color()
{
    g_box_color -= key_count(ID_KEY_CTL_DEL);
}

main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked)
{
    int i;
    int k;

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
        if (new_fractal_type(*frommandel))
        {
            return main_state::IMAGE_START;
        }
        break;
    case ID_KEY_CTL_X:                     // Ctl-X, Ctl-Y, CTL-Z do flipping
    case ID_KEY_CTL_Y:
    case ID_KEY_CTL_Z:
        flip_image(*kbdchar);
        break;
    case 'x':          // invoke options screen
    case 'y':          //
    case 'p':          // passes options
    case 'z':          // type specific parms
    case 'g':          // enter a command
    case 'v':          // view window
    case ID_KEY_CTL_B: // browsing
    case ID_KEY_CTL_E: // evolving
    case ID_KEY_CTL_F: // sound options
        prompt_options(*kbdmore, *kbdchar);
        break;
    case '@':                    // execute commands
    case '2':                    // execute commands
        driver_stack_screen();
        i = +get_commands();
        if (g_init_mode != -1)
        {
            // video= was specified
            g_adapter = g_init_mode;
            g_init_mode = -1;
            i |= +cmdarg_flags::FRACTAL_PARAM;
            g_save_dac = 0;
        }
        else if (g_colors_preloaded)
        {
            // colors= was specified
            spindac(0, 1);
            g_colors_preloaded = false;
        }
        else if (i & +cmdarg_flags::RESET)           // reset was specified
        {
            g_save_dac = 0;
        }
        if (i & +cmdarg_flags::YES_3D)
        {
            // 3d = was specified
            *kbdchar = '3';
            driver_unstack_screen();
            goto do_3d_transform;  // pretend '3' was keyed
        }
        if (i & +cmdarg_flags::FRACTAL_PARAM)
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
        toggle_float();
        return main_state::IMAGE_START;
    case 'i':                    // 3d fractal parms
        if (get_fract3d_params() >= 0)    // get the parameters
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
            *kbdmore = false;    // time to redraw
        }
        break;
    case ID_KEY_CTL_A:                     // ^a Ant
        if (begin_ant())
        {
            return main_state::CONTINUE;
        }
        break;
    case 'k':                    // ^s is irritating, give user a single key
    case ID_KEY_CTL_S:                     // ^s RDS
        clear_zoom_box();
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
        clear_zoom_box();
        if (get_starfield_params() >= 0)
        {
            if (starfield() >= 0)
            {
                g_calc_status = calc_status_value::PARAMS_CHANGED;
            }
            return main_state::CONTINUE;
        }
        break;
    case ID_KEY_CTL_O:                     // ctrl-o
    case 'o':
        show_orbit_window();
        break;
    case ID_KEY_SPACE:                  // spacebar, toggle mand/julia
        if (g_bf_math != bf_math_type::NONE || g_evolving != evolution_mode_flags::NONE)
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
            toggle_mandelbrot_julia(*kbdmore, *frommandel);
        }
        break;
    case 'j':                    // inverse julia toggle
        inverse_julia_toggle(*kbdmore);
        break;
    case '\\':                   // return to prev image
    case ID_KEY_CTL_BACKSLASH:
    case 'h':
    case ID_KEY_BACKSPACE:
        if (const main_state result = get_history(*stacked, *kbdchar); result != main_state::NOTHING)
        {
            return result;
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
        clear_zoom_box();
        std::memcpy(g_old_dac_box, g_dac_box, 256 * 3);
        rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
        if (std::memcmp(g_old_dac_box, g_dac_box, 256 * 3))
        {
            g_color_state = color_state::UNKNOWN;
            save_history_info();
        }
        return main_state::CONTINUE;
    case 'e':                    // switch to color editing
        if (color_editing(*kbdmore))
        {
            return main_state::CONTINUE;
        }
        break;
    case 's':                    // save-to-disk
        if (driver_diskp() && g_disk_targa)
        {
            return main_state::CONTINUE;  // disk video and targa, nothing to save
        }
        save_image(g_save_filename);
        return main_state::CONTINUE;
    case '#':                    // 3D overlay
        clear_zoom_box();
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
        restore_from_image(*frommandel, *kbdchar, *stacked);
        return main_state::RESTORE_START;
    case 'l':
    case 'L':                    // Look for other files within this view
        if (look_for_files(*stacked))
        {
            return main_state::RESTORE_START;
        }
        break;
    case 'b':                    // make batch file
        make_batch_file();
        break;
    case ID_KEY_ENTER:                  // Enter
    case ID_KEY_ENTER_2:                // Numeric-Keypad Enter
        request_zoom_in(*kbdmore);
        break;
    case ID_KEY_CTL_ENTER:              // control-Enter
    case ID_KEY_CTL_ENTER_2:            // Control-Keypad Enter
        request_zoom_out(*kbdmore);
        break;
    case ID_KEY_INSERT:         // insert
        driver_set_for_text();           // force text mode
        return main_state::RESTART;
    case ID_KEY_LEFT_ARROW:             // cursor left
    case ID_KEY_RIGHT_ARROW:            // cursor right
    case ID_KEY_UP_ARROW:               // cursor up
    case ID_KEY_DOWN_ARROW:             // cursor down
        move_zoom_box(*kbdchar);
        break;
    case ID_KEY_CTL_LEFT_ARROW:           // Ctrl-cursor left
    case ID_KEY_CTL_RIGHT_ARROW:          // Ctrl-cursor right
    case ID_KEY_CTL_UP_ARROW:             // Ctrl-cursor up
    case ID_KEY_CTL_DOWN_ARROW:           // Ctrl-cursor down
        move_zoom_box(*kbdchar);
        break;
    case ID_KEY_CTL_HOME:               // Ctrl-home
        skew_zoom_left();
        break;
    case ID_KEY_CTL_END:                // Ctrl-end
        skew_zoom_right();
        break;
    case ID_KEY_CTL_PAGE_UP:            // Ctrl-pgup
        decrease_zoom_aspect();
        break;
    case ID_KEY_CTL_PAGE_DOWN:          // Ctrl-pgdn
        increase_zoom_aspect();
        break;

    case ID_KEY_PAGE_UP:                // page up
        zoom_box_in();
        break;
    case ID_KEY_PAGE_DOWN:              // page down
        zoom_box_out();
        break;
    case ID_KEY_CTL_MINUS:              // Ctrl-kpad-
        zoom_box_increase_rotation();
        break;
    case ID_KEY_CTL_PLUS:               // Ctrl-kpad+
        zoom_box_decrease_rotation();
        break;
    case ID_KEY_CTL_INSERT:             // Ctrl-ins
        zoom_box_increase_color();
        break;
    case ID_KEY_CTL_DEL:                // Ctrl-del
        zoom_box_decrease_color();
        break;

    case ID_KEY_ALT_1: // alt + number keys set mutation level and start evolution engine
    case ID_KEY_ALT_2:
    case ID_KEY_ALT_3:
    case ID_KEY_ALT_4:
    case ID_KEY_ALT_5:
    case ID_KEY_ALT_6:
    case ID_KEY_ALT_7:
        g_evolving = evolution_mode_flags::FIELDMAP;
        g_view_window = true;
        set_mutation_level(*kbdchar - ID_KEY_ALT_1 + 1);
        save_param_history();
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case ID_KEY_DELETE:         // select video mode from list
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
