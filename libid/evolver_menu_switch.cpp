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

#include <algorithm>
#include <cassert>
#include <stdexcept>

static main_state save_evolver_image(int &, bool &, bool &, bool &)
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

static main_state prompt_evolver_options(int &kbd_char, bool &, bool &kbd_more, bool &)
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
    if (kbd_char == 'x')
    {
        i = get_toggles();
    }
    else if (kbd_char == 'y')
    {
        i = get_toggles2();
    }
    else if (kbd_char == 'p')
    {
        i = passes_options();
    }
    else if (kbd_char == 'z')
    {
        i = get_fract_params(true);
    }
    else if (kbd_char == ID_KEY_CTL_E || kbd_char == ID_KEY_SPACE)
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

static main_state exit_evolver(int &, bool &, bool &kbd_more, bool &)
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

static main_state evolve_param_zoom_decrease(int &, bool &, bool &, bool &)
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

static main_state evolve_param_zoom_increase(int &, bool &, bool &, bool &)
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

static main_state evolver_zoom_in(int &, bool &, bool &, bool &)
{
    if (g_zoom_enabled)
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

static main_state evolver_zoom_out(int &, bool &, bool &, bool &)
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

static main_state halve_mutation_params(int &, bool &, bool &kbd_more, bool &)
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

static main_state double_mutation_params(int &, bool &, bool &kbd_more, bool &)
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

static main_state decrease_grid_size(int &, bool &, bool &kbd_more, bool &)
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

static main_state increase_grid_size(int &, bool &, bool &kbd_more, bool &)
{
    if (g_evolve_image_grid_size < (g_screen_x_dots / (MIN_PIXELS << 1)))
    {
        g_evolve_image_grid_size = g_evolve_image_grid_size + 2;
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    return main_state::NOTHING;
}

static main_state toggle_gene_variation(int &, bool &, bool &kbd_more, bool &)
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

static main_state request_mutation_level(int &kbd_char, bool &, bool &kbd_more, bool &)
{
    int mutation_level;
    if (kbd_char >= ID_KEY_ALT_1 && kbd_char <= ID_KEY_ALT_7)
    {
        mutation_level = kbd_char - ID_KEY_ALT_1 + 1;
    }
    else if (kbd_char >= '1' && kbd_char <= '7')
    {
        mutation_level = kbd_char - '1' + 1;
    }
    else
    {
        throw std::runtime_error(
            "Bad mutation level character (" + std::to_string(kbd_char) + ") to request_mutation_level");
    }
    set_mutation_level(mutation_level);
    restore_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state turn_off_evolving(int &, bool &, bool &kbd_more, bool &)
{
    g_evolving = evolution_mode_flags::NONE;
    g_view_window = false;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}

static main_state evolver_get_history(int &kbd_char, bool &, bool &, bool &)
{
    return get_history(kbd_char);
}

// grabbed a couple of video mode keys, user can change to these using delete and the menu if necessary
static const MenuHandler s_handlers[]{
    {ID_KEY_CTL_E, prompt_evolver_options},             //
    {ID_KEY_CTL_H, evolver_get_history},                // return to prev image
    {ID_KEY_CTL_ENTER, request_zoom_out},               //
    {ID_KEY_ENTER, request_zoom_in},                    //
    {ID_KEY_CTL_BACKSLASH, evolver_get_history},        // return to prev image
    {ID_KEY_SPACE, prompt_evolver_options},             //
    {'+', color_cycle},                                 // rotate palette
    {'-', color_cycle},                                 // rotate palette
    {'0', turn_off_evolving},                           // mutation level 0 == turn off evolving
    {'1', request_mutation_level},                      //
    {'2', request_mutation_level},                      //
    {'3', request_mutation_level},                      //
    {'4', request_mutation_level},                      //
    {'5', request_mutation_level},                      //
    {'6', request_mutation_level},                      //
    {'7', request_mutation_level},                      //
    {'\\', evolver_get_history},                        // return to prev image
    {'b', exit_evolver},                                // quick exit from evolve mode
    {'c', color_cycle},                                 // switch to color cycling
    {'e', color_editing},                               // switch to color editing
    {'f', toggle_float},                                // floating pt toggle
    {'g', prompt_evolver_options},                      //
    {'h', evolver_get_history},                         // return to prev image
    {'p', prompt_evolver_options},                      // passes options
    {'r', restore_from_image},                          // restore-from
    {'s', save_evolver_image},                          // save-to-disk
    {'t', request_fractal_type},                        // new fractal type
    {'x', prompt_evolver_options},                      // invoke options screen
    {'y', prompt_evolver_options},                      // invoke options screen
    {'z', prompt_evolver_options},                      // type specific params
    {ID_KEY_CTL_ENTER_2, request_zoom_out},             //
    {ID_KEY_ENTER_2, request_zoom_in},                  //
    {ID_KEY_F2, halve_mutation_params},                 // halve mutation params and regen
    {ID_KEY_F3, double_mutation_params},                // double mutation parameters and regenerate
    {ID_KEY_F4, decrease_grid_size},                    // decrement grid size and regen
    {ID_KEY_F5, increase_grid_size},                    // increment grid size and regen
    {ID_KEY_F6, toggle_gene_variation},                 // toggle all variables selected for random variation
    {ID_KEY_UP_ARROW, move_zoom_box},                   //
    {ID_KEY_PAGE_UP, evolver_zoom_in},                  //
    {ID_KEY_LEFT_ARROW, move_zoom_box},                 //
    {ID_KEY_RIGHT_ARROW, move_zoom_box},                //
    {ID_KEY_DOWN_ARROW, move_zoom_box},                 //
    {ID_KEY_PAGE_DOWN, evolver_zoom_out},               //
    {ID_KEY_INSERT, request_restart},                   //
    {ID_KEY_CTL_LEFT_ARROW, move_evolver_selection},    //
    {ID_KEY_CTL_RIGHT_ARROW, move_evolver_selection},   //
    {ID_KEY_CTL_END, skew_zoom_right},                  //
    {ID_KEY_CTL_PAGE_DOWN, evolve_param_zoom_increase}, //
    {ID_KEY_CTL_HOME, skew_zoom_left},                  //
    {ID_KEY_ALT_1, request_mutation_level},             //
    {ID_KEY_ALT_2, request_mutation_level},             //
    {ID_KEY_ALT_3, request_mutation_level},             //
    {ID_KEY_ALT_4, request_mutation_level},             //
    {ID_KEY_ALT_5, request_mutation_level},             //
    {ID_KEY_ALT_6, request_mutation_level},             //
    {ID_KEY_ALT_7, request_mutation_level},             //
    {ID_KEY_CTL_PAGE_UP, evolve_param_zoom_decrease},   //
    {ID_KEY_CTL_UP_ARROW, move_evolver_selection},      //
    {ID_KEY_CTL_MINUS, zoom_box_increase_rotation},     //
    {ID_KEY_CTL_PLUS, zoom_box_decrease_rotation},      //
    {ID_KEY_CTL_DOWN_ARROW, move_evolver_selection},    //
    {ID_KEY_CTL_INSERT, zoom_box_increase_color},       //
    {ID_KEY_CTL_DEL, zoom_box_decrease_color},          //
};

main_state evolver_menu_switch(int &kbd_char, bool &from_mandel, bool &kbd_more, bool &stacked)
{
    assert(std::is_sorted(std::begin(s_handlers), std::end(s_handlers)));
    if (const auto it = std::lower_bound(std::cbegin(s_handlers), std::cend(s_handlers), kbd_char);
        it != std::cend(s_handlers) && it->key == kbd_char)
    {
        return it->handler(kbd_char, from_mandel, kbd_more, stacked);
    }

    if (kbd_char == ID_KEY_DELETE)
    {
        // select video mode from list
        request_video_mode(kbd_char);
    }

    // other (maybe a valid Fn key)
    return requested_video_fn(kbd_char, from_mandel, kbd_more, stacked);
}
