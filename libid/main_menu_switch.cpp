// SPDX-License-Identifier: GPL-3.0-only
//
#include "main_menu_switch.h"

#include "engine/calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "flip_image.h"
#include "fractals/ant.h"
#include "fractals/cellular.h"
#include "fractals/fractalp.h"
#include "fractals/frothy_basin.h"
#include "fractals/parser.h"
#include "fractype.h"
#include "framain2.h"
#include "get_commands.h"
#include "id.h"
#include "engine/id_data.h"
#include "id_keys.h"
#include "io/merge_path_names.h"
#include "engine/jiim.h"
#include "loadfile.h"
#include "load_params.h"
#include "make_batch_file.h"
#include "math/fixed_pt.h"
#include "menu_handler.h"
#include "spindac.h"
#include "starfield.h"
#include "stereo.h"
#include "ui/get_3d_params.h"
#include "ui/get_browse_params.h"
#include "ui/get_cmd_string.h"
#include "ui/get_fract_type.h"
#include "ui/get_rds_params.h"
#include "ui/get_sound_params.h"
#include "ui/get_toggles.h"
#include "ui/get_toggles2.h"
#include "ui/get_view_params.h"
#include "ui/passes_options.h"
#include "ui/select_video_mode.h"
#include "zoom.h"

#include <algorithm>
#include <iterator>
#include <string>

static bool look(MainContext &context)
{
    HelpLabels const old_help_mode = g_help_mode;
    g_help_mode = HelpLabels::HELP_BROWSE;
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
        merge_path_names(g_read_filename, g_browse_name.c_str(), CmdFile::AT_AFTER_STARTUP);
        if (g_ask_video)
        {
            driver_stack_screen();   // save graphics image
            context.stacked = true;
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
            merge_path_names(g_read_filename, g_browse_name.c_str(), CmdFile::AT_AFTER_STARTUP);
            g_browsing = true;
            g_show_file = 0;
            if (g_ask_video)
            {
                driver_stack_screen();// save graphics image
                context.stacked = true;
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

static void toggle_mandelbrot_julia(MainContext &context)
{
    static double s_j_x_min;
    static double s_j_x_max;
    static double s_j_y_min;
    static double s_j_y_max; // "Julia mode" entry point
    static double s_j_x_3rd;
    static double s_j_y_3rd;

    if (g_fractal_type == FractalType::FORMULA || g_fractal_type == FractalType::FORMULA_FP)
    {
        if (g_is_mandelbrot)
        {
            g_fractal_specific[+g_fractal_type].to_julia = g_fractal_type;
            g_fractal_specific[+g_fractal_type].to_mandel = FractalType::NO_FRACTAL;
            g_is_mandelbrot = false;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].to_julia = FractalType::NO_FRACTAL;
            g_fractal_specific[+g_fractal_type].to_mandel = g_fractal_type;
            g_is_mandelbrot = true;
        }
    }
    if (g_cur_fractal_specific->to_julia != FractalType::NO_FRACTAL && g_params[0] == 0.0 &&
        g_params[1] == 0.0)
    {
        // switch to corresponding Julia set
        g_has_inverse =
            (g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::MANDEL_FP) &&
            g_bf_math == BFMathType::NONE;
        clear_zoom_box();
        jiim(JIIMType::JIIM);
        const int key = driver_get_key(); // flush keyboard buffer
        if (key != ID_KEY_SPACE)
        {
            driver_unget_key(key);
            return;
        }
        g_fractal_type = g_cur_fractal_specific->to_julia;
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
        context.from_mandel = true;
        g_x_min = g_cur_fractal_specific->x_min;
        g_x_max = g_cur_fractal_specific->x_max;
        g_y_min = g_cur_fractal_specific->y_min;
        g_y_max = g_cur_fractal_specific->y_max;
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
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        context.more_keys = false;
    }
    else if (g_cur_fractal_specific->to_mandel != FractalType::NO_FRACTAL)
    {
        // switch to corresponding Mandel set
        g_fractal_type = g_cur_fractal_specific->to_mandel;
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        if (context.from_mandel)
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
            g_x_3rd = g_cur_fractal_specific->x_min;
            g_x_min = g_x_3rd;
            g_x_max = g_cur_fractal_specific->x_max;
            g_y_3rd = g_cur_fractal_specific->y_min;
            g_y_min = g_y_3rd;
            g_y_max = g_cur_fractal_specific->y_max;
        }
        g_save_c.x = g_params[0];
        g_save_c.y = g_params[1];
        g_params[0] = 0;
        g_params[1] = 0;
        g_zoom_enabled = true;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        context.more_keys = false;
    }
    else
    {
        driver_buzzer(Buzzer::PROBLEM); // can't switch
    }
}

static MainState prompt_options(MainContext &context)
{
    const long old_max_iterations = g_max_iterations;
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
    if (context.key == 'x')
    {
        i = get_toggles();
    }
    else if (context.key == 'y')
    {
        i = get_toggles2();
    }
    else if (context.key == 'p')
    {
        i = passes_options();
    }
    else if (context.key == 'z')
    {
        i = get_fract_params(true);
    }
    else if (context.key == 'v')
    {
        i = get_view_params(); // get the parameters
    }
    else if (context.key == ID_KEY_CTL_B)
    {
        i = get_browse_params();
    }
    else if (context.key == ID_KEY_CTL_E)
    {
        i = get_evolve_params();
        if (i > 0)
        {
            g_start_show_orbit = false;
            g_sound_flag &= ~(SOUNDFLAG_X | SOUNDFLAG_Y | SOUNDFLAG_Z); // turn off only x,y,z
            g_log_map_auto_calculate = false; // turn it off
        }
    }
    else if (context.key == ID_KEY_CTL_F)
    {
        i = get_sound_params();
    }
    else
    {
        i = get_cmd_string();
    }
    driver_unstack_screen();
    if (g_evolving != EvolutionModeFlags::NONE && g_true_color)
    {
        g_true_color = false;          // truecolor doesn't play well with the evolver
    }
    if (g_max_iterations > old_max_iterations
        && g_inside_color >= COLOR_BLACK
        && g_calc_status == CalcStatus::COMPLETED
        && g_cur_fractal_specific->calc_type == standard_fractal
        && !g_log_map_flag
        && !g_true_color     // recalc not yet implemented with truecolor
        && (g_user_std_calc_mode != 't' || g_fill_color <= -1) // tesseral with fill doesn't work
        && g_user_std_calc_mode != 'o'
        && i == 1 // nothing else changed
        && g_outside_color != ATAN)
    {
        g_quick_calc = true;
        g_old_std_calc_mode = g_user_std_calc_mode;
        g_user_std_calc_mode = '1';
        context.more_keys = false;
        g_calc_status = CalcStatus::RESUMABLE;
    }
    else if (i > 0)
    {
        // time to redraw?
        g_quick_calc = false;
        save_param_history();
        context.more_keys = false;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
    }
    return MainState::NOTHING;
}

static MainState begin_ant(MainContext &)
{
    clear_zoom_box();
    double old_param[MAX_PARAMS];
    FractalType old_type = g_fractal_type;
    for (int j = 0; j < MAX_PARAMS; ++j)
    {
        old_param[j] = g_params[j];
    }
    if (g_fractal_type != FractalType::ANT)
    {
        g_fractal_type = FractalType::ANT;
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
            g_calc_status = CalcStatus::PARAMS_CHANGED;
        }
    }
    else
    {
        driver_unstack_screen();
    }
    g_fractal_type = old_type;
    for (int j = 0; j < MAX_PARAMS; ++j)
    {
        g_params[j] = old_param[j];
    }
    return err >= 0 ? MainState::CONTINUE : MainState::NOTHING;
}

static MainState request_3d_fractal_params(MainContext &context)
{
    if (get_fract3d_params() >= 0) // get the parameters
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        context.more_keys = false; // time to redraw
    }
    return MainState::NOTHING;
}

static MainState show_orbit_window(MainContext &)
{
    // must use standard fractal and have a float variant
    if ((g_fractal_specific[+g_fractal_type].calc_type == standard_fractal
            || g_fractal_specific[+g_fractal_type].calc_type == calc_froth)
        && (g_fractal_specific[+g_fractal_type].is_integer == 0 ||
             g_fractal_specific[+g_fractal_type].to_float != FractalType::NO_FRACTAL)
        && (g_bf_math == BFMathType::NONE) // for now no arbitrary precision support
        && (!g_is_true_color || g_true_mode == TrueColorMode::DEFAULT_COLOR))
    {
        clear_zoom_box();
        jiim(JIIMType::ORBIT);
    }
    return MainState::NOTHING;
}

static MainState space_command(MainContext &context)
{
    if (g_bf_math != BFMathType::NONE || g_evolving != EvolutionModeFlags::NONE)
    {
        return MainState::NOTHING;
    }

    if (g_fractal_type == FractalType::CELLULAR)
    {
        g_cellular_next_screen = !g_cellular_next_screen;
        g_calc_status = CalcStatus::RESUMABLE;
        context.more_keys = false;
    }
    else
    {
        toggle_mandelbrot_julia(context);
    }

    return MainState::NOTHING;
}

static MainState inverse_julia_toggle(MainContext &context)
{
    // if the inverse types proliferate, something more elegant will be needed
    if (g_fractal_type == FractalType::JULIA || g_fractal_type == FractalType::JULIA_FP ||
        g_fractal_type == FractalType::INVERSE_JULIA)
    {
        static FractalType old_type = FractalType::NO_FRACTAL;
        if (g_fractal_type == FractalType::JULIA || g_fractal_type == FractalType::JULIA_FP)
        {
            old_type = g_fractal_type;
            g_fractal_type = FractalType::INVERSE_JULIA;
        }
        else if (g_fractal_type == FractalType::INVERSE_JULIA)
        {
            if (old_type != FractalType::NO_FRACTAL)
            {
                g_fractal_type = old_type;
            }
            else
            {
                g_fractal_type = FractalType::JULIA;
            }
        }
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
        g_zoom_enabled = true;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        context.more_keys = false;
    }
    else
    {
        driver_buzzer(Buzzer::PROBLEM);
    }
    return MainState::NOTHING;
}

static MainState unstack_file(bool &stacked)
{
    if (g_filename_stack_index < 1)
    {
        return MainState::NOTHING;
    }

    // go back one file if somewhere to go (ie. browsing)
    g_filename_stack_index--;
    while (g_file_name_stack[g_filename_stack_index].empty() && g_filename_stack_index >= 0)
    {
        g_filename_stack_index--;
    }
    if (g_filename_stack_index < 0) // oops, must have deleted first one
    {
        return MainState::NOTHING;
    }
    g_browse_name = g_file_name_stack[g_filename_stack_index];
    merge_path_names(g_read_filename, g_browse_name.c_str(), CmdFile::AT_AFTER_STARTUP);
    g_browsing = true;
    g_browse_sub_images = true;
    g_show_file = 0;
    if (g_ask_video)
    {
        driver_stack_screen(); // save graphics image
        stacked = true;
    }
    return MainState::RESTORE_START;
}

static MainState main_history(MainContext &context)
{
    if (const MainState result = unstack_file(context.stacked); result != MainState::NOTHING)
    {
        return result;
    }
    if (const MainState result = get_history(context.key); result != MainState::NOTHING)
    {
        return result;
    }
    return MainState::NOTHING;
}

static MainState request_shell(MainContext &)
{
    driver_stack_screen();
    driver_shell();
    driver_unstack_screen();
    return MainState::NOTHING;
}

static MainState request_save_image(MainContext &)
{
    if (driver_is_disk() && g_disk_targa)
    {
        return MainState::CONTINUE; // disk video and targa, nothing to save
    }
    save_image(g_save_filename);
    return MainState::CONTINUE;
}

static MainState look_for_files(MainContext &context)
{
    if ((g_zoom_box_width != 0) || driver_is_disk())
    {
        g_browsing = false;
        driver_buzzer(Buzzer::PROBLEM);             // can't browse if zooming or disk video
    }
    else if (look(context))
    {
        return MainState::RESTORE_START;
    }
    return MainState::NOTHING;
}

static MainState request_make_batch_file(MainContext &)
{
    make_batch_file();
    return MainState::NOTHING;
}

static MainState start_evolution(MainContext &context)
{
    g_evolving = EvolutionModeFlags::FIELD_MAP;
    g_view_window = true;
    set_mutation_level(context.key - ID_KEY_ALT_1 + 1);
    save_param_history();
    context.more_keys = false;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    return MainState::NOTHING;
}

static MainState restore_from_3d(MainContext &context)
{
    if (g_overlay_3d)
    {
        g_display_3d = Display3DMode::B_COMMAND; // for <b> command
    }
    else
    {
        g_display_3d = Display3DMode::YES;
    }
    return restore_from_image(context);
}

static MainState request_3d_overlay(MainContext &context)
{
    clear_zoom_box();
    g_overlay_3d = true;
    return restore_from_3d(context);
}

static MainState execute_commands(MainContext &context)
{
    driver_stack_screen();
    int i = +get_commands();
    if (g_init_mode != -1)
    {
        // video= was specified
        g_adapter = g_init_mode;
        g_init_mode = -1;
        i |= +CmdArgFlags::FRACTAL_PARAM;
        g_save_dac = 0;
    }
    else if (g_colors_preloaded)
    {
        // colors= was specified
        spin_dac(0, 1);
        g_colors_preloaded = false;
    }
    else if (i & +CmdArgFlags::RESET) // reset was specified
    {
        g_save_dac = 0;
    }
    if (i & +CmdArgFlags::YES_3D)
    {
        // 3d = was specified
        context.key = '3';
        driver_unstack_screen();
        // pretend '3' was keyed
        return restore_from_3d(context);
    }
    if (i & +CmdArgFlags::FRACTAL_PARAM)
    {
        // fractal parameter changed
        driver_discard_screen();
        context.more_keys = false;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
    }
    else
    {
        driver_unstack_screen();
    }
    return MainState::NOTHING;
}

static MainState random_dot_stereogram(MainContext &)
{
    clear_zoom_box();
    if (get_rds_params() >= 0)
    {
        if (auto_stereo_convert())
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;
        }
        return MainState::CONTINUE;
    }
    return MainState::NOTHING;
}

static MainState request_star_field_params(MainContext &)
{
    clear_zoom_box();
    if (get_star_field_params() >= 0)
    {
        if (star_field() >= 0)
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;
        }
        return MainState::CONTINUE;
    }
    return MainState::NOTHING;
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

MainState main_menu_switch(MainContext &context)
{
    if (g_quick_calc && g_calc_status == CalcStatus::COMPLETED)
    {
        g_quick_calc = false;
        g_user_std_calc_mode = g_old_std_calc_mode;
    }
    if (g_quick_calc && g_calc_status != CalcStatus::COMPLETED)
    {
        g_user_std_calc_mode = g_old_std_calc_mode;
    }

    assert(std::is_sorted(std::begin(s_handlers), std::end(s_handlers)));
    if (const auto it = std::lower_bound(std::cbegin(s_handlers), std::cend(s_handlers), context.key);
        it != std::cend(s_handlers) && it->key == context.key)
    {
        return it->handler(context);
    }

    if (context.key == ID_KEY_DELETE)
    {
        // select video mode from list
        request_video_mode(context.key);
    }

    // other (maybe a valid Fn key)
    return requested_video_fn(context);
}
