// SPDX-License-Identifier: GPL-3.0-only
//
#include "menu_handler.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "editpal.h"
#include "evolve.h"
#include "fractalp.h"
#include "get_fract_type.h"
#include "history.h"
#include "id_data.h"
#include "id_keys.h"
#include "jb.h"
#include "loadfile.h"
#include "lorenz.h"
#include "rotate.h"
#include "spindac.h"
#include "update_save_name.h"
#include "value_saver.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstring>

MainState request_fractal_type(MainContext &context)
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
            context.from_mandel = false;
        }
        else if (g_init_mode < 0)   // it is supposed to be...
        {
            driver_set_for_text();     // reset to text mode
        }
        return MainState::IMAGE_START;
    }
    driver_unstack_screen();
    return MainState::NOTHING;
}

MainState toggle_float(MainContext &)
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
    return MainState::IMAGE_START;
}

MainState get_history(int kbd_char)
{
    if (g_max_image_history <= 0 || g_bf_math != BFMathType::NONE)
    {
        return MainState::NOTHING;
    }

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
    g_zoom_enabled = true;
    g_init_mode = g_adapter;
    if (g_cur_fractal_specific->isinteger != 0 && g_cur_fractal_specific->tofloat != FractalType::NOFRACTAL)
    {
        g_user_float_flag = false;
    }
    if (g_cur_fractal_specific->isinteger == 0 && g_cur_fractal_specific->tofloat != FractalType::NOFRACTAL)
    {
        g_user_float_flag = true;
    }
    g_history_flag = true; // avoid re-store parms due to rounding errs
    return MainState::IMAGE_START;
}

MainState color_cycle(MainContext &context)
{
    clear_zoom_box();
    std::memcpy(g_old_dac_box, g_dac_box, 256 * 3);
    if (context.key == 'c')
    {
        rotate(0);
    }
    else
    {
        rotate(context.key == '+' ? 1 : -1);
    }
    if (std::memcmp(g_old_dac_box, g_dac_box, 256 * 3))
    {
        g_color_state = ColorState::UNKNOWN;
        save_history_info();
    }
    return MainState::CONTINUE;
}

MainState color_editing(MainContext &context)
{
    if (g_is_true_color && (g_init_batch == BatchMode::NONE))
    {
        // don't enter palette editor
        if (!load_palette())
        {
            context.more_keys = false;
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            return MainState::NOTHING;
        }

        return MainState::CONTINUE;
    }

    clear_zoom_box();
    if (g_dac_box[0][0] != 255 && g_colors >= 16 && !driver_diskp())
    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_PALETTE_EDITOR};
        std::memcpy(g_old_dac_box, g_dac_box, 256 * 3);
        edit_palette();
        if (std::memcmp(g_old_dac_box, g_dac_box, 256 * 3) != 0)
        {
            g_color_state = ColorState::UNKNOWN;
            save_history_info();
        }
    }
    return MainState::CONTINUE;
}

MainState restore_from_image(MainContext &context)
{
    g_compare_gif = false;
    context.from_mandel = false;
    g_browsing = false;
    if (context.key == 'r')
    {
        if (g_debug_flag == DebugFlags::force_disk_restore_not_save)
        {
            g_compare_gif = true;
            g_overlay_3d = true;
            if (g_init_batch == BatchMode::SAVE)
            {
                driver_stack_screen();   // save graphics image
                g_read_filename = g_save_filename;
                g_show_file = 0;
                return MainState::RESTORE_START;
            }
        }
        else
        {
            g_compare_gif = false;
            g_overlay_3d = false;
        }
        g_display_3d = Display3DMode::NONE;
    }
    driver_stack_screen();            // save graphics image
    context.stacked = !g_overlay_3d;
    if (g_resave_flag)
    {
        update_save_name(g_save_filename);      // do the pending increment
        g_resave_flag = 0;
        g_started_resaves = false;
    }
    g_show_file = -1;
    return MainState::RESTORE_START;
}

MainState requested_video_fn(MainContext &context)
{
    const int k = check_vid_mode_key(0, context.key);
    if (k < 0)
    {
        return MainState::NOTHING;
    }

    g_adapter = k;
    if (g_video_table[g_adapter].colors != g_colors)
    {
        g_save_dac = 0;
    }
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    context.more_keys = false;
    return MainState::CONTINUE;
}

MainState request_restart(MainContext &)
{
    driver_set_for_text(); // force text mode
    return MainState::RESTART;
}
