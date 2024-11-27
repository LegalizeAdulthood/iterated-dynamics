// SPDX-License-Identifier: GPL-3.0-only
//
#include "evolver_menu_switch.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "encoder.h"
#include "evolve.h"
#include "find_special_colors.h"
#include "framain2.h"
#include "get_cmd_string.h"
#include "get_fract_type.h"
#include "get_toggles.h"
#include "get_toggles2.h"
#include "id_data.h"
#include "id_keys.h"
#include "menu_handler.h"
#include "passes_options.h"
#include "pixel_limits.h"
#include "select_video_mode.h"
#include "value_saver.h"
#include "zoom.h"

static main_state save_evolver_image()
{
    if (driver_diskp() && g_disk_targa)
    {
        // disk video and targa, nothing to save
        return main_state::NOTHING;
    }

    GeneBase gene[NUM_GENES];
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
            fiddle_params(gene, 0);
            draw_param_box(1);
            save_image(g_save_filename);
        }
        restore_param_history();
        fiddle_params(gene, unspiral_map());
    }
    copy_genes_to_bank(gene);
    return main_state::CONTINUE;
}

static main_state prompt_evolver_options(int key, bool &kbd_more)
{
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
    else if (key == ID_KEY_CTL_E || key == ID_KEY_SPACE)
    {
        i = get_evolve_params();
    }
    else
    {
        i = get_cmd_string();
    }
    driver_unstack_screen();
    if (g_evolving != evolution_mode_flags::NONE && g_truecolor)
    {
        g_truecolor = false; // truecolor doesn't play well with the evolver
    }
    if (i > 0)
    {
        // time to redraw?
        save_param_history();
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    return main_state::NOTHING;
}

static main_state exit_evolver(bool &kbd_more)
{
    g_evolving = evolution_mode_flags::NONE;
    g_view_window = false;
    save_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state move_evolver_selection(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    if (!g_box_count)
    {
        return move_zoom_box(key, from_mandel, kbd_more, stacked);
    }

    // if no zoombox, scroll by arrows
    // borrow ctrl cursor keys for moving selection box in evolver mode
    GeneBase gene[NUM_GENES];
    copy_genes_from_bank(gene);
    if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
    {
        if (key == ID_KEY_CTL_LEFT_ARROW)
        {
            g_evolve_param_grid_x--;
        }
        if (key == ID_KEY_CTL_RIGHT_ARROW)
        {
            g_evolve_param_grid_x++;
        }
        if (key == ID_KEY_CTL_UP_ARROW)
        {
            g_evolve_param_grid_y--;
        }
        if (key == ID_KEY_CTL_DOWN_ARROW)
        {
            g_evolve_param_grid_y++;
        }
        if (g_evolve_param_grid_x < 0)
        {
            g_evolve_param_grid_x = g_evolve_image_grid_size - 1;
        }
        if (g_evolve_param_grid_x > (g_evolve_image_grid_size - 1))
        {
            g_evolve_param_grid_x = 0;
        }
        if (g_evolve_param_grid_y < 0)
        {
            g_evolve_param_grid_y = g_evolve_image_grid_size - 1;
        }
        if (g_evolve_param_grid_y > (g_evolve_image_grid_size - 1))
        {
            g_evolve_param_grid_y = 0;
        }
        const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
        g_logical_screen_x_offset = g_evolve_param_grid_x * (int) (g_logical_screen_x_size_dots + 1 + grout);
        g_logical_screen_y_offset = g_evolve_param_grid_y * (int) (g_logical_screen_y_size_dots + 1 + grout);

        restore_param_history();
        fiddle_params(gene, unspiral_map()); // change all parameters
        // to values appropriate to the image selected
        set_evolve_ranges();
        change_box(0, 0);
        draw_param_box(0);
    }
    copy_genes_to_bank(gene);
    return main_state::NOTHING;
}

static main_state evolve_param_zoom_decrease()
{
    if (g_evolve_param_box_count)
    {
        g_evolve_param_zoom -= 1.0;
        if (g_evolve_param_zoom < 1.0)
        {
            g_evolve_param_zoom = 1.0;
        }
        draw_param_box(0);
        set_evolve_ranges();
    }
    return main_state::NOTHING;
}

static main_state evolve_param_zoom_increase()
{
    if (g_evolve_param_box_count)
    {
        g_evolve_param_zoom += 1.0;
        if (g_evolve_param_zoom > (double) g_evolve_image_grid_size / 2.0)
        {
            g_evolve_param_zoom = (double) g_evolve_image_grid_size / 2.0;
        }
        draw_param_box(0);
        set_evolve_ranges();
    }
    return main_state::NOTHING;
}

static main_state evolver_zoom_in()
{
    if (g_zoom_off)
    {
        if (g_zoom_box_width == 0)
        {
            // start zoombox
            g_zoom_box_height = 1;
            g_zoom_box_width = 1;
            g_zoom_box_rotation = 0;
            g_zoom_box_skew = 0.0;
            g_zoom_box_x = 0.0;
            g_zoom_box_y = 0.0;
            find_special_colors();
            g_box_color = g_color_bright;
            if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
            {
                // set screen view params back (previously changed to allow full screen saves in viewwindow
                // mode)
                const int grout = bit_set(g_evolving, evolution_mode_flags::NOGROUT) ? 0 : 1;
                g_logical_screen_x_offset =
                    g_evolve_param_grid_x * (int) (g_logical_screen_x_size_dots + 1 + grout);
                g_logical_screen_y_offset =
                    g_evolve_param_grid_y * (int) (g_logical_screen_y_size_dots + 1 + grout);
                setup_param_box();
                draw_param_box(0);
            }
            move_box(0.0, 0.0); // force scrolling
        }
        else
        {
            resize_box(0 - key_count(ID_KEY_PAGE_UP));
        }
    }
    return main_state::NOTHING;
}

static main_state evolver_zoom_out()
{
    if (g_box_count)
    {
        if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)
        {
            // end zoombox
            g_zoom_box_width = 0;
            if (bit_set(g_evolving, evolution_mode_flags::FIELDMAP))
            {
                draw_param_box(1); // clear boxes off screen
                release_param_box();
            }
        }
        else
        {
            resize_box(key_count(ID_KEY_PAGE_DOWN));
        }
    }
    return main_state::NOTHING;
}

static main_state halve_mutation_params(bool &kbd_more)
{
    g_evolve_max_random_mutation = g_evolve_max_random_mutation / 2;
    g_evolve_x_parameter_range = g_evolve_x_parameter_range / 2;
    g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
    g_evolve_y_parameter_range = g_evolve_y_parameter_range / 2;
    g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state double_mutation_params(bool &kbd_more)
{
    g_evolve_max_random_mutation = g_evolve_max_random_mutation * 2;
    const double centerx = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
    g_evolve_x_parameter_range = g_evolve_x_parameter_range * 2;
    g_evolve_new_x_parameter_offset = centerx - g_evolve_x_parameter_range / 2;
    const double centery = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
    g_evolve_y_parameter_range = g_evolve_y_parameter_range * 2;
    g_evolve_new_y_parameter_offset = centery - g_evolve_y_parameter_range / 2;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state decrease_grid_size(bool &kbd_more)
{
    if (g_evolve_image_grid_size > 3)
    {
        g_evolve_image_grid_size =
            g_evolve_image_grid_size - 2; // evolve_image_grid_size must have odd value only
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    return main_state::NOTHING;
}

static main_state increase_grid_size(bool &kbd_more)
{
    if (g_evolve_image_grid_size < (g_screen_x_dots / (MIN_PIXELS << 1)))
    {
        g_evolve_image_grid_size = g_evolve_image_grid_size + 2;
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    return main_state::NOTHING;
}

static main_state toggle_gene_variation(bool &kbd_more)
{
    for (GeneBase &gene : g_gene_bank)
    {
        if (gene.mutate == variations::RANDOM)
        {
            gene.mutate = variations::WEIGHTED_RANDOM;
            continue;
        }
        if (gene.mutate == variations::WEIGHTED_RANDOM)
        {
            gene.mutate = variations::RANDOM;
        }
    }
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state request_mutation_level(int mutation_level, bool &kbd_more)
{
    set_mutation_level(mutation_level);
    restore_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static void turn_off_evolving(bool &kbd_more)
{
    g_evolving = evolution_mode_flags::NONE;
    g_view_window = false;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

main_state evolver_menu_switch(int &kbd_char, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    switch (kbd_char)
    {
    case 't':                    // new fractal type
        return request_fractal_type(kbd_char, from_mandel, kbd_more, stacked);

    case 'x':                    // invoke options screen
    case 'y':
    case 'p':                    // passes options
    case 'z':                    // type specific parms
    case 'g':
    case ID_KEY_CTL_E:
    case ID_KEY_SPACE:
        return prompt_evolver_options(kbd_char, kbd_more);

    case 'b': // quick exit from evolve mode
        return exit_evolver(kbd_more);

    case 'f':                    // floating pt toggle
        return toggle_float(kbd_char, from_mandel, kbd_more, stacked);
        
    case '\\':                   // return to prev image
    case ID_KEY_CTL_BACKSLASH:
    case 'h':
    case ID_KEY_BACKSPACE:
        return get_history(kbd_char);
        
    case 'c':                    // switch to color cycling
    case '+':                    // rotate palette
    case '-':                    // rotate palette
        return color_cycle(kbd_char, from_mandel, kbd_more, stacked);
        
    case 'e':                    // switch to color editing
        return color_editing(kbd_char, from_mandel, kbd_more, stacked);
        
    case 's':                    // save-to-disk
        return save_evolver_image();

    case 'r':                    // restore-from
        return restore_from_image(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_ENTER:                  // Enter
    case ID_KEY_ENTER_2:                // Numeric-Keypad Enter
        return request_zoom_in(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_ENTER:              // control-Enter
    case ID_KEY_CTL_ENTER_2:            // Control-Keypad Enter
        return request_zoom_out(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_INSERT:
        return request_restart(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_LEFT_ARROW:             // cursor left
    case ID_KEY_RIGHT_ARROW:            // cursor right
    case ID_KEY_UP_ARROW:               // cursor up
    case ID_KEY_DOWN_ARROW:             // cursor down
        return move_zoom_box(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_LEFT_ARROW:           // Ctrl-cursor left
    case ID_KEY_CTL_RIGHT_ARROW:          // Ctrl-cursor right
    case ID_KEY_CTL_UP_ARROW:             // Ctrl-cursor up
    case ID_KEY_CTL_DOWN_ARROW:           // Ctrl-cursor down
        return move_evolver_selection(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_HOME:               // Ctrl-home
        return skew_zoom_left(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_END:                // Ctrl-end
        return skew_zoom_right(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_PAGE_UP:
        return evolve_param_zoom_decrease();
        
    case ID_KEY_CTL_PAGE_DOWN:
        return evolve_param_zoom_increase();

    case ID_KEY_PAGE_UP:                // page up
        return evolver_zoom_in();
        
    case ID_KEY_PAGE_DOWN:              // page down
        return evolver_zoom_out();
        
    case ID_KEY_CTL_MINUS:              // Ctrl-kpad-
        return zoom_box_increase_rotation(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_PLUS:               // Ctrl-kpad+
        return zoom_box_decrease_rotation(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_INSERT:             // Ctrl-ins
        return zoom_box_increase_color(kbd_char, from_mandel, kbd_more, stacked);
        
    case ID_KEY_CTL_DEL:                // Ctrl-del
        return zoom_box_decrease_color(kbd_char, from_mandel, kbd_more, stacked);

    /* grabbed a couple of video mode keys, user can change to these using
        delete and the menu if necessary */

    case ID_KEY_F2: // halve mutation params and regen
        return halve_mutation_params(kbd_more);

    case ID_KEY_F3: //double mutation parameters and regenerate
        return double_mutation_params(kbd_more);

    case ID_KEY_F4: //decrement  gridsize and regen
        return decrease_grid_size(kbd_more);

    case ID_KEY_F5: // increment gridsize and regen
        return increase_grid_size(kbd_more);

    case ID_KEY_F6: /* toggle all variables selected for random variation to center weighted variation and vice versa */
        return toggle_gene_variation(kbd_more);

    case ID_KEY_ALT_1: // alt + number keys set mutation level
    case ID_KEY_ALT_2:
    case ID_KEY_ALT_3:
    case ID_KEY_ALT_4:
    case ID_KEY_ALT_5:
    case ID_KEY_ALT_6:
    case ID_KEY_ALT_7:
        return request_mutation_level(kbd_char - ID_KEY_ALT_1 + 1, kbd_more);

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        return request_mutation_level(kbd_char - '1' + 1, kbd_more);

    case '0': // mutation level 0 == turn off evolving
        turn_off_evolving(kbd_more);
        break;

    case ID_KEY_DELETE:         // select video mode from list
        request_video_mode(kbd_char);
        // fallthrough

    default: // NOLINT(clang-diagnostic-implicit-fallthrough)
        // other (maybe valid Fn key
        return requested_video_fn(kbd_char, from_mandel, kbd_more, stacked);
    }

    return main_state::NOTHING;
}
