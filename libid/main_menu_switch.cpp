// SPDX-License-Identifier: GPL-3.0-only
//
#include "main_menu_switch.h"

#include "ant.h"
#include "calcfrac.h"
#include "cellular.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
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
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "jiim.h"
#include "load_params.h"
#include "loadfile.h"
#include "make_batch_file.h"
#include "menu_handler.h"
#include "merge_path_names.h"
#include "parser.h"
#include "passes_options.h"
#include "select_video_mode.h"
#include "spindac.h"
#include "starfield.h"
#include "stereo.h"
#include "zoom.h"

#include <algorithm>
#include <iterator>
#include <string>

static bool look(bool *stacked)
{
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP_BROWSE;
    switch (file_get_window())
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
        merge_path_names(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
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
            merge_path_names(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
            g_browsing = true;
            g_show_file = 0;
            if (g_ask_video)
            {
                driver_stack_screen();// save graphics image
                *stacked = true;
            }
            return true;
        }
        // otherwise fall through and turn off browsing

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
        jiim(jiim_types::JIIM);
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
        g_zoom_enabled = true;
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
        g_zoom_enabled = true;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        kbd_more = false;
    }
    else
    {
        driver_buzzer(buzzer_codes::PROBLEM); // can't switch
    }
}

static main_state prompt_options(int &key, bool &, bool &kbd_more, bool &)
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
    if (key == 'x')
    {
        i = get_toggles();
    }
    else if (key == 'y')
    {
        i = get_toggles2();
    }
    else if (key == 'p')
    {
        i = passes_options();
    }
    else if (key == 'z')
    {
        i = get_fract_params(true);
    }
    else if (key == 'v')
    {
        i = get_view_params(); // get the parameters
    }
    else if (key == ID_KEY_CTL_B)
    {
        i = get_browse_params();
    }
    else if (key == ID_KEY_CTL_E)
    {
        i = get_evolve_params();
        if (i > 0)
        {
            g_start_show_orbit = false;
            g_sound_flag &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); // turn off only x,y,z
            g_log_map_auto_calculate = false; // turn it off
        }
    }
    else if (key == ID_KEY_CTL_F)
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
    return main_state::NOTHING;
}

static main_state begin_ant(int &, bool &, bool &, bool &)
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
    return err >= 0 ? main_state::CONTINUE : main_state::NOTHING;
}

static main_state request_3d_fractal_params(int &, bool &, bool &kbd_more, bool &)
{
    if (get_fract3d_params() >= 0) // get the parameters
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        kbd_more = false; // time to redraw
    }
    return main_state::NOTHING;
}

static main_state show_orbit_window(int &, bool &, bool &, bool &)
{
    // must use standard fractal and have a float variant
    if ((g_fractal_specific[+g_fractal_type].calctype == standard_fractal
            || g_fractal_specific[+g_fractal_type].calctype == calc_froth)
        && (g_fractal_specific[+g_fractal_type].isinteger == 0 ||
             g_fractal_specific[+g_fractal_type].tofloat != fractal_type::NOFRACTAL)
        && (g_bf_math == bf_math_type::NONE) // for now no arbitrary precision support
        && !(g_is_true_color && g_true_mode != true_color_mode::default_color))
    {
        clear_zoom_box();
        jiim(jiim_types::ORBIT);
    }
    return main_state::NOTHING;
}

static main_state space_command(int &, bool &from_mandel, bool &kbd_more, bool &)
{
    if (g_bf_math != bf_math_type::NONE || g_evolving != evolution_mode_flags::NONE)
    {
        return main_state::NOTHING;
    }

    if (g_fractal_type == fractal_type::CELLULAR)
    {
        g_cellular_next_screen = !g_cellular_next_screen;
        g_calc_status = calc_status_value::RESUMABLE;
        kbd_more = false;
    }
    else
    {
        toggle_mandelbrot_julia(kbd_more, from_mandel);
    }

    return main_state::NOTHING;
}

static main_state inverse_julia_toggle(int &, bool &, bool &kbd_more, bool &)
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
        g_zoom_enabled = true;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        kbd_more = false;
    }
    else
    {
        driver_buzzer(buzzer_codes::PROBLEM);
    }
    return main_state::NOTHING;
}

static main_state unstack_file(bool &stacked)
{
    if (g_filename_stack_index < 1)
    {
        return main_state::NOTHING;
    }

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
    merge_path_names(g_read_filename, g_browse_name.c_str(), cmd_file::AT_AFTER_STARTUP);
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

static main_state main_history(int &key, bool &, bool &, bool &stacked)
{
    if (const main_state result = unstack_file(stacked); result != main_state::NOTHING)
    {
        return result;
    }
    if (const main_state result = get_history(key); result != main_state::NOTHING)
    {
        return result;
    }
    return main_state::NOTHING;
}

static main_state request_shell(int &, bool &, bool &, bool &)
{
    driver_stack_screen();
    driver_shell();
    driver_unstack_screen();
    return main_state::NOTHING;
}

static main_state request_save_image(int &, bool &, bool &, bool &)
{
    if (driver_diskp() && g_disk_targa)
    {
        return main_state::CONTINUE; // disk video and targa, nothing to save
    }
    save_image(g_save_filename);
    return main_state::CONTINUE;
}

static main_state look_for_files(int &, bool &, bool &, bool &stacked)
{
    if ((g_zoom_box_width != 0) || driver_diskp())
    {
        g_browsing = false;
        driver_buzzer(buzzer_codes::PROBLEM);             // can't browse if zooming or disk video
    }
    else if (look(&stacked))
    {
        return main_state::RESTORE_START;
    }
    return main_state::NOTHING;
}

static main_state request_make_batch_file(int &, bool &, bool &, bool &)
{
    make_batch_file();
    return main_state::NOTHING;
}

static main_state start_evolution(int &kbd_char, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    g_evolving = evolution_mode_flags::FIELDMAP;
    g_view_window = true;
    set_mutation_level(kbd_char - ID_KEY_ALT_1 + 1);
    save_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state restore_from_3d(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    if (g_overlay_3d)
    {
        g_display_3d = display_3d_modes::B_COMMAND; // for <b> command
    }
    else
    {
        g_display_3d = display_3d_modes::YES;
    }
    return restore_from_image(key, from_mandel, kbd_more, stacked);
}

static main_state request_3d_overlay(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    clear_zoom_box();
    g_overlay_3d = true;
    return restore_from_3d(key, from_mandel, kbd_more, stacked);
}

static main_state execute_commands(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    driver_stack_screen();
    int i = +get_commands();
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
        spin_dac(0, 1);
        g_colors_preloaded = false;
    }
    else if (i & +cmdarg_flags::RESET) // reset was specified
    {
        g_save_dac = 0;
    }
    if (i & +cmdarg_flags::YES_3D)
    {
        // 3d = was specified
        key = '3';
        driver_unstack_screen();
        // pretend '3' was keyed
        return restore_from_3d(key, from_mandel, kbd_more, stacked);
    }
    if (i & +cmdarg_flags::FRACTAL_PARAM)
    {
        // fractal parameter changed
        driver_discard_screen();
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    else
    {
        driver_unstack_screen();
    }
    return main_state::NOTHING;
}

static main_state random_dot_stereogram(int &, bool &, bool &, bool &)
{
    clear_zoom_box();
    if (get_rds_params() >= 0)
    {
        if (auto_stereo_convert())
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        return main_state::CONTINUE;
    }
    return main_state::NOTHING;
}

static main_state request_star_field_params(int &, bool &, bool &, bool &)
{
    clear_zoom_box();
    if (get_star_field_params() >= 0)
    {
        if (star_field() >= 0)
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        return main_state::CONTINUE;
    }
    return main_state::NOTHING;
}

static MenuHandler s_handlers[]{
    {ID_KEY_CTL_A, begin_ant},                      //
    {ID_KEY_CTL_B, prompt_options},                 //
    {ID_KEY_CTL_E, prompt_options},                 //
    {ID_KEY_CTL_F, prompt_options},                 //
    {ID_KEY_CTL_H, main_history},                   //
    {ID_KEY_CTL_ENTER, request_zoom_out},           //
    {ID_KEY_ENTER, request_zoom_in},                //
    {ID_KEY_CTL_O, show_orbit_window},              //
    {ID_KEY_CTL_S, random_dot_stereogram},          //
    {ID_KEY_CTL_X, flip_image},                     //
    {ID_KEY_CTL_Y, flip_image},                     //
    {ID_KEY_CTL_Z, flip_image},                     //
    {ID_KEY_CTL_BACKSLASH, main_history},           //
    {ID_KEY_SPACE, space_command},                  //
    {'#', request_3d_overlay},                      //
    {'+', color_cycle},                             //
    {'-', color_cycle},                             //
    {'2', execute_commands},                        //
    {'3', restore_from_3d},                         //
    {'@', execute_commands},                        //
    {'\\', main_history},                           //
    {'a', request_star_field_params},               //
    {'b', request_make_batch_file},                 //
    {'c', color_cycle},                             //
    {'d', request_shell},                           //
    {'e', color_editing},                           //
    {'f', toggle_float},                            //
    {'g', prompt_options},                          //
    {'h', main_history},                            //
    {'i', request_3d_fractal_params},               //
    {'j', inverse_julia_toggle},                    //
    {'k', random_dot_stereogram},                   //
    {'l', look_for_files},                          //
    {'o', show_orbit_window},                       //
    {'p', prompt_options},                          //
    {'r', restore_from_image},                      //
    {'s', request_save_image},                      //
    {'t', request_fractal_type},                    //
    {'v', prompt_options},                          //
    {'x', prompt_options},                          //
    {'y', prompt_options},                          //
    {'z', prompt_options},                          //
    {ID_KEY_CTL_ENTER_2, request_zoom_out},         //
    {ID_KEY_ENTER_2, request_zoom_in},              //
    {ID_KEY_UP_ARROW, move_zoom_box},               //
    {ID_KEY_PAGE_UP, zoom_box_in},                  //
    {ID_KEY_LEFT_ARROW, move_zoom_box},             //
    {ID_KEY_RIGHT_ARROW, move_zoom_box},            //
    {ID_KEY_DOWN_ARROW, move_zoom_box},             //
    {ID_KEY_PAGE_DOWN, zoom_box_out},               //
    {ID_KEY_INSERT, request_restart},               //
    {ID_KEY_CTL_LEFT_ARROW, move_zoom_box},         //
    {ID_KEY_CTL_RIGHT_ARROW, move_zoom_box},        //
    {ID_KEY_CTL_END, skew_zoom_right},              //
    {ID_KEY_CTL_PAGE_DOWN, increase_zoom_aspect},   //
    {ID_KEY_CTL_HOME, skew_zoom_left},              //
    {ID_KEY_ALT_1, start_evolution},                //
    {ID_KEY_ALT_2, start_evolution},                //
    {ID_KEY_ALT_3, start_evolution},                //
    {ID_KEY_ALT_4, start_evolution},                //
    {ID_KEY_ALT_5, start_evolution},                //
    {ID_KEY_ALT_6, start_evolution},                //
    {ID_KEY_ALT_7, start_evolution},                //
    {ID_KEY_CTL_PAGE_UP, decrease_zoom_aspect},     //
    {ID_KEY_CTL_UP_ARROW, move_zoom_box},           //
    {ID_KEY_CTL_MINUS, zoom_box_increase_rotation}, //
    {ID_KEY_CTL_PLUS, zoom_box_decrease_rotation},  //
    {ID_KEY_CTL_DOWN_ARROW, move_zoom_box},         //
    {ID_KEY_CTL_INSERT, zoom_box_increase_color},   //
    {ID_KEY_CTL_DEL, zoom_box_decrease_color},      //
};

main_state main_menu_switch(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    if (g_quick_calc && g_calc_status == calc_status_value::COMPLETED)
    {
        g_quick_calc = false;
        g_user_std_calc_mode = g_old_std_calc_mode;
    }
    if (g_quick_calc && g_calc_status != calc_status_value::COMPLETED)
    {
        g_user_std_calc_mode = g_old_std_calc_mode;
    }

    assert(std::is_sorted(std::begin(s_handlers), std::end(s_handlers)));
    if (const auto it = std::lower_bound(std::cbegin(s_handlers), std::cend(s_handlers), key);
        it != std::cend(s_handlers) && it->key == key)
    {
        return it->handler(key, from_mandel, kbd_more, stacked);
    }

    if (key == ID_KEY_DELETE)
    {
        // select video mode from list
        request_video_mode(key);
    }

    // other (maybe a valid Fn key)
    return requested_video_fn(key, from_mandel, kbd_more, stacked);
}
