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
#include "main_menu_switch.h"
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

static void save_evolver_image()
{
    if (driver_diskp() && g_disk_targa)
    {
        return; // disk video and targa, nothing to save
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
}

static void prompt_evolver_options(int key, bool &kbd_more)
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
        i = get_evolve_Parms();
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
}

static void exit_evolver(bool &kbd_more)
{
    g_evolving = evolution_mode_flags::NONE;
    g_view_window = false;
    save_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

static void move_evolver_selection(int key)
{
    if (!g_box_count)
    {
        move_zoom_box(key);
        return;
    }

    // if no zoombox, scroll by arrows
    // borrow ctrl cursor keys for moving selection box in evolver mode
    GENEBASE gene[NUM_GENES];
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
        fiddleparms(gene, unspiralmap()); // change all parameters
        // to values appropriate to the image selected
        set_evolve_ranges();
        change_box(0, 0);
        drawparmbox(0);
    }
    copy_genes_to_bank(gene);
}

static void evolve_param_zoom_decrease()
{
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
}

static void evolve_param_zoom_increase()
{
    if (g_evolve_param_box_count)
    {
        g_evolve_param_zoom += 1.0;
        if (g_evolve_param_zoom > (double) g_evolve_image_grid_size / 2.0)
        {
            g_evolve_param_zoom = (double) g_evolve_image_grid_size / 2.0;
        }
        drawparmbox(0);
        set_evolve_ranges();
    }
}

static void evolver_zoom_in()
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
}

static void evolver_zoom_out()
{
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
}

static void halve_mutation_params(bool &kbd_more)
{
    g_evolve_max_random_mutation = g_evolve_max_random_mutation / 2;
    g_evolve_x_parameter_range = g_evolve_x_parameter_range / 2;
    g_evolve_new_x_parameter_offset = g_evolve_x_parameter_offset + g_evolve_x_parameter_range / 2;
    g_evolve_y_parameter_range = g_evolve_y_parameter_range / 2;
    g_evolve_new_y_parameter_offset = g_evolve_y_parameter_offset + g_evolve_y_parameter_range / 2;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

static void double_mutation_params(bool &kbd_more)
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
}

static void decrease_grid_size(bool &kbd_more)
{
    if (g_evolve_image_grid_size > 3)
    {
        g_evolve_image_grid_size =
            g_evolve_image_grid_size - 2; // evolve_image_grid_size must have odd value only
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
}

static void increase_grid_size(bool &kbd_more)
{
    if (g_evolve_image_grid_size < (g_screen_x_dots / (MIN_PIXELS << 1)))
    {
        g_evolve_image_grid_size = g_evolve_image_grid_size + 2;
        kbd_more = false;
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
}

static void toggle_gene_variation(bool &kbd_more)
{
    for (GENEBASE &gene : g_gene_bank)
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
}

static void request_mutation_level(int mutation_level, bool &kbd_more)
{
    set_mutation_level(mutation_level);
    restore_param_history();
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

static void turn_off_evolving(bool &kbd_more)
{
    g_evolving = evolution_mode_flags::NONE;
    g_view_window = false;
    kbd_more = false;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

main_state evolver_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked)
{
    int i;
    int k;

    switch (*kbdchar)
    {
    case 't':                    // new fractal type
        return request_fractal_type(*frommandel);

    case 'x':                    // invoke options screen
    case 'y':
    case 'p':                    // passes options
    case 'z':                    // type specific parms
    case 'g':
    case ID_KEY_CTL_E:
    case ID_KEY_SPACE:
        prompt_evolver_options(*kbdchar, *kbdmore);
        break;

    case 'b': // quick exit from evolve mode
        exit_evolver(*kbdmore);
        break;

    case 'f':                    // floating pt toggle
        toggle_float();
        return main_state::IMAGE_START;
        
    case '\\':                   // return to prev image
    case ID_KEY_CTL_BACKSLASH:
    case 'h':
    case ID_KEY_BACKSPACE:
        if (const main_state result = get_history(*kbdchar); result != main_state::NOTHING)
        {
            return result;
        }
        break;
        
    case 'c':                    // switch to color cycling
    case '+':                    // rotate palette
    case '-':                    // rotate palette
        color_cycle(*kbdchar);
        return main_state::CONTINUE;
        
    case 'e':                    // switch to color editing
        if (color_editing(*kbdmore))
        {
            return main_state::CONTINUE;
        }
        break;
        
    case 's':                    // save-to-disk
        save_evolver_image();
        return main_state::CONTINUE;

    case 'r':                    // restore-from
        restore_from_image(*frommandel, *kbdchar, *stacked);
        return main_state::RESTORE_START;
        
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
        move_evolver_selection(*kbdchar);
        break;
        
    case ID_KEY_CTL_HOME:               // Ctrl-home
        skew_zoom_left();
        break;
        
    case ID_KEY_CTL_END:                // Ctrl-end
        skew_zoom_right();
        break;
        
    case ID_KEY_CTL_PAGE_UP:
        evolve_param_zoom_decrease();
        break;
        
    case ID_KEY_CTL_PAGE_DOWN:
        evolve_param_zoom_increase();
        break;

    case ID_KEY_PAGE_UP:                // page up
        evolver_zoom_in();
        break;
        
    case ID_KEY_PAGE_DOWN:              // page down
        evolver_zoom_out();
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

    /* grabbed a couple of video mode keys, user can change to these using
        delete and the menu if necessary */

    case ID_KEY_F2: // halve mutation params and regen
        halve_mutation_params(*kbdmore);
        break;

    case ID_KEY_F3: //double mutation parameters and regenerate
        double_mutation_params(*kbdmore);
        break;

    case ID_KEY_F4: //decrement  gridsize and regen
        decrease_grid_size(*kbdmore);
        break;

    case ID_KEY_F5: // increment gridsize and regen
        increase_grid_size(*kbdmore);
        break;

    case ID_KEY_F6: /* toggle all variables selected for random variation to center weighted variation and vice versa */
        toggle_gene_variation(*kbdmore);
        break;

    case ID_KEY_ALT_1: // alt + number keys set mutation level
    case ID_KEY_ALT_2:
    case ID_KEY_ALT_3:
    case ID_KEY_ALT_4:
    case ID_KEY_ALT_5:
    case ID_KEY_ALT_6:
    case ID_KEY_ALT_7:
        request_mutation_level(*kbdchar - ID_KEY_ALT_1 + 1, *kbdmore);
        break;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        request_mutation_level(*kbdchar - '1' + 1, *kbdmore);
        break;

    case '0': // mutation level 0 == turn off evolving
        turn_off_evolving(*kbdmore);
        break;

    case ID_KEY_DELETE:         // select video mode from list
        request_video_mode(*kbdchar);
        // fallthrough

    default: // NOLINT(clang-diagnostic-implicit-fallthrough)
        // other (maybe valid Fn key
        if (requested_video_fn(*kbdmore, *kbdchar))
        {
            return main_state::CONTINUE;
        }
        break;
    }

    return main_state::NOTHING;
}
