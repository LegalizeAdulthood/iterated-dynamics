// SPDX-License-Identifier: GPL-3.0-only
//
#include "evolver_menu_switch.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "encoder.h"
#include "evolve.h"
#include "find_special_colors.h"
#include "fractalp.h"
#include "fractype.h"
#include "framain2.h"
#include "get_cmd_string.h"
#include "get_fract_type.h"
#include "get_toggles.h"
#include "get_toggles2.h"
#include "history.h"
#include "id_data.h"
#include "id_keys.h"
#include "jb.h"
#include "loadfile.h"
#include "lorenz.h"
#include "passes_options.h"
#include "pixel_limits.h"
#include "rotate.h"
#include "select_video_mode.h"
#include "spindac.h"
#include "update_save_name.h"
#include "value_saver.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstring>

main_state evolver_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked)
{
    int i;
    int k;

    switch (*kbdchar)
    {
    case 't':                    // new fractal type
        g_julibrot = false;
        clear_zoom_box();
        driver_stack_screen();
        i = get_fract_type();
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
            save_param_history();
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
    case ID_KEY_CTL_E:
    case ID_KEY_SPACE:
        clear_zoom_box();
        if (g_from_text)
        {
            g_from_text = false;
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
            i = get_fract_params(true);
        }
        else if (*kbdchar == ID_KEY_CTL_E || *kbdchar == ID_KEY_SPACE)
        {
            i = get_evolve_Parms();
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
        if (i > 0)
        {
            // time to redraw?
            save_param_history();
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;
    case 'b': // quick exit from evolve mode
        g_evolving = evolution_mode_flags::NONE;
        g_view_window = false;
        save_param_history();
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
    case ID_KEY_CTL_BACKSLASH:
    case 'h':
    case ID_KEY_BACKSPACE:
        if (g_max_image_history > 0 && g_bf_math == bf_math_type::NONE)
        {
            if (*kbdchar == '\\' || *kbdchar == 'h')
            {
                if (--g_history_ptr < 0)
                {
                    g_history_ptr = g_max_image_history - 1;
                }
            }
            if (*kbdchar == ID_KEY_CTL_BACKSLASH || *kbdchar == 8)
            {
                if (++g_history_ptr >= g_max_image_history)
                {
                    g_history_ptr = 0;
                }
            }
            restore_history_info(g_history_ptr);
            g_zoom_off = true;
            g_init_mode = g_adapter;
            if (g_cur_fractal_specific->isinteger != 0
                && g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = false;
            }
            if (g_cur_fractal_specific->isinteger == 0
                && g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
            {
                g_user_float_flag = true;
            }
            g_history_flag = true;         // avoid re-store parms due to rounding errs
            return main_state::IMAGE_START;
        }
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
        clear_zoom_box();
        if (g_dac_box[0][0] != 255 && g_colors >= 16 && !driver_diskp())
        {
            ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_PALETTE_EDITOR};
            std::memcpy(g_old_dac_box, g_dac_box, 256 * 3);
            EditPalette();
            if (std::memcmp(g_old_dac_box, g_dac_box, 256 * 3))
            {
                g_color_state = color_state::UNKNOWN;
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

        GENEBASE gene[NUM_GENES];
        copy_genes_from_bank(gene);
        {
            ValueSaver saved_logical_screen_x_offset{g_logical_screen_x_offset, 0};
            ValueSaver saved_logical_screen_y_offset{g_logical_screen_y_offset, 0};
            ValueSaver saved_logical_screen_x_dots{g_logical_screen_x_dots, g_screen_x_dots};
            ValueSaver saved_logical_screen_y_dots{g_logical_screen_y_dots, g_screen_y_dots};
            {
                ValueSaver saved_evolve_param_grid_x{g_evolve_param_grid_x, g_evolve_image_grid_size / 2};
                ValueSaver saved_evolve_param_grid_y{g_evolve_param_grid_y, g_evolve_image_grid_size / 2};
                restore_param_history();
                fiddleparms(gene, 0);
                drawparmbox(1);
                save_image(g_save_filename);
            }
            restore_param_history();
            fiddleparms(gene, unspiralmap());
        }
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
            update_save_name(g_save_filename);      // do the pending increment
            g_resave_flag = 0;
            g_started_resaves = false;
        }
        g_show_file = -1;
        return main_state::RESTORE_START;
    case ID_KEY_ENTER:                  // Enter
    case ID_KEY_ENTER_2:                // Numeric-Keypad Enter
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
    case ID_KEY_CTL_ENTER:              // control-Enter
    case ID_KEY_CTL_ENTER_2:            // Control-Keypad Enter
        init_pan_or_recalc(true);
        *kbdmore = false;
        zoom_out();                // calc corners for zooming out
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
        // borrow ctrl cursor keys for moving selection box
        // in evolver mode
        if (g_box_count)
        {
            GENEBASE gene[NUM_GENES];
            copy_genes_from_bank(gene);
            if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
            {
                if (*kbdchar == ID_KEY_CTL_LEFT_ARROW)
                {
                    g_evolve_param_grid_x--;
                }
                if (*kbdchar == ID_KEY_CTL_RIGHT_ARROW)
                {
                    g_evolve_param_grid_x++;
                }
                if (*kbdchar == ID_KEY_CTL_UP_ARROW)
                {
                    g_evolve_param_grid_y--;
                }
                if (*kbdchar == ID_KEY_CTL_DOWN_ARROW)
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
                const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
                g_logical_screen_x_offset = g_evolve_param_grid_x * (int)(g_logical_screen_x_size_dots+1+grout);
                g_logical_screen_y_offset = g_evolve_param_grid_y * (int)(g_logical_screen_y_size_dots+1+grout);

                restore_param_history();
                fiddleparms(gene, unspiralmap()); // change all parameters
                // to values appropriate to the image selected
                set_evolve_ranges();
                change_box(0, 0);
                drawparmbox(0);
            }
            copy_genes_to_bank(gene);
        }
        else                         // if no zoombox, scroll by arrows
        {
            move_zoom_box(*kbdchar);
        }
        break;
    case ID_KEY_CTL_HOME:               // Ctrl-home
        if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
        {
            i = key_count(ID_KEY_CTL_HOME);
            if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
            {
                g_zoom_box_skew = -0.48;
            }
        }
        break;
    case ID_KEY_CTL_END:                // Ctrl-end
        if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
        {
            i = key_count(ID_KEY_CTL_END);
            if ((g_zoom_box_skew += 0.02 * i) > 0.48)
            {
                g_zoom_box_skew = 0.48;
            }
        }
        break;
    case ID_KEY_CTL_PAGE_UP:
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
    case ID_KEY_CTL_PAGE_DOWN:
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

    case ID_KEY_PAGE_UP:                // page up
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
                if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
                {
                    // set screen view params back (previously changed to allow full screen saves in viewwindow mode)
                    const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
                    g_logical_screen_x_offset = g_evolve_param_grid_x * (int)(g_logical_screen_x_size_dots+1+grout);
                    g_logical_screen_y_offset = g_evolve_param_grid_y * (int)(g_logical_screen_y_size_dots+1+grout);
                    SetupParamBox();
                    drawparmbox(0);
                }
                move_box(0.0, 0.0); // force scrolling
            }
            else
            {
                resize_box(0 - key_count(ID_KEY_PAGE_UP));
            }
        }
        break;
    case ID_KEY_PAGE_DOWN:              // page down
        if (g_box_count)
        {
            if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)
            {
                // end zoombox
                g_zoom_box_width = 0;
                if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
                {
                    drawparmbox(1); // clear boxes off screen
                    ReleaseParamBox();
                }
            }
            else
            {
                resize_box(key_count(ID_KEY_PAGE_DOWN));
            }
        }
        break;
    case ID_KEY_CTL_MINUS:              // Ctrl-kpad-
        if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
        {
            g_zoom_box_rotation += key_count(ID_KEY_CTL_MINUS);
        }
        break;
    case ID_KEY_CTL_PLUS:               // Ctrl-kpad+
        if (g_box_count && bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOROTATE))
        {
            g_zoom_box_rotation -= key_count(ID_KEY_CTL_PLUS);
        }
        break;
    case ID_KEY_CTL_INSERT:             // Ctrl-ins
        g_box_color += key_count(ID_KEY_CTL_INSERT);
        break;
    case ID_KEY_CTL_DEL:                // Ctrl-del
        g_box_color -= key_count(ID_KEY_CTL_DEL);
        break;

    /* grabbed a couple of video mode keys, user can change to these using
        delete and the menu if necessary */

    case ID_KEY_F2: // halve mutation params and regen
        g_evolve_max_random_mutation = g_evolve_max_random_mutation / 2;
        g_evolve_x_parameter_range = g_evolve_x_parameter_range / 2;
        g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
        g_evolve_y_parameter_range = g_evolve_y_parameter_range / 2;
        g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case ID_KEY_F3: //double mutation parameters and regenerate
    {
        double centerx;
        double centery;
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

    case ID_KEY_F4: //decrement  gridsize and regen
        if (g_evolve_image_grid_size > 3)
        {
            g_evolve_image_grid_size = g_evolve_image_grid_size - 2;  // evolve_image_grid_size must have odd value only
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;

    case ID_KEY_F5: // increment gridsize and regen
        if (g_evolve_image_grid_size < (g_screen_x_dots / (MIN_PIXELS << 1)))
        {
            g_evolve_image_grid_size = g_evolve_image_grid_size + 2;
            *kbdmore = false;
            g_calc_status = calc_status_value::PARAMS_CHANGED;
        }
        break;

    case ID_KEY_F6: /* toggle all variables selected for random variation to center weighted variation and vice versa */
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

    case ID_KEY_ALT_1: // alt + number keys set mutation level
    case ID_KEY_ALT_2:
    case ID_KEY_ALT_3:
    case ID_KEY_ALT_4:
    case ID_KEY_ALT_5:
    case ID_KEY_ALT_6:
    case ID_KEY_ALT_7:
        set_mutation_level(*kbdchar - ID_KEY_ALT_1 + 1);
        restore_param_history();
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
        restore_param_history();
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case '0': // mutation level 0 == turn off evolving
        g_evolving = evolution_mode_flags::NONE;
        g_view_window = false;
        *kbdmore = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
        break;

    case ID_KEY_DELETE:         // select video mode from list
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
