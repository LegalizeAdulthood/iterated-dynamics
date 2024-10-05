// SPDX-License-Identifier: GPL-3.0-only
//
#include "tab_display.h"

/*
        Resident odds and ends that don't fit anywhere else.
*/
#include "port.h"
#include "prototyp.h"

#include "biginit.h"
#include "calc_frac_init.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "diffusion_scan.h"
#include "drivers.h"
#include "engine_timer.h"
#include "file_gets.h"
#include "find_file.h"
#include "file_item.h"
#include "fractalp.h"
#include "fractype.h"
#include "get_calculation_time.h"
#include "get_key_no_help.h"
#include "help_title.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "make_path.h"
#include "param_not_used.h"
#include "parser.h"
#include "pixel_grid.h"
#include "put_string_center.h"
#include "rotate.h"
#include "slideshw.h"
#include "soi.h"
#include "split_path.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "trig_fns.h"
#include "trim_filename.h"
#include "type_has_param.h"
#include "update_save_name.h"
#include "version.h"
#include "video.h"
#include "video_mode.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>

static void area();

// Wrapping version of putstring for long numbers
// row     -- pointer to row variable, internally incremented if needed
// col1    -- starting column
// col2    -- last column
// color   -- attribute (same as for putstring)
// maxrow -- max number of rows to write
// returns false if success, true if hit maxrow before done
static bool putstringwrap(int *row, int col1, int col2, int color, char *str, int maxrow)
{
    char save1;
    char save2;
    int length;
    int decpt;
    int g_padding;
    int startrow;
    bool done = false;
    startrow = *row;
    length = (int) std::strlen(str);
    g_padding = 3; // space between col1 and decimal.
    // find decimal point
    for (decpt = 0; decpt < length; decpt++)
    {
        if (str[decpt] == '.')
        {
            break;
        }
    }
    if (decpt >= length)
    {
        decpt = 0;
    }
    if (decpt < g_padding)
    {
        g_padding -= decpt;
    }
    else
    {
        g_padding = 0;
    }
    col1 += g_padding;
    decpt += col1+1; // column just past where decimal is
    while (length > 0)
    {
        if (col2-col1 < length)
        {
            done = (*row - startrow + 1) >= maxrow;
            save1 = str[col2-col1+1];
            save2 = str[col2-col1+2];
            if (done)
            {
                str[col2-col1+1]   = '+';
            }
            else
            {
                str[col2-col1+1]   = '\\';
            }
            str[col2-col1+2] = 0;
            driver_put_string(*row, col1, color, str);
            if (done)
            {
                break;
            }
            str[col2-col1+1] = save1;
            str[col2-col1+2] = save2;
            str += col2-col1;
            (*row)++;
        }
        else
        {
            driver_put_string(*row, col1, color, str);
        }
        length -= col2-col1;
        col1 = decpt; // align with decimal
    }
    return done;
}

static void show_str_var(char const *name, char const *var, int *row, char *msg)
{
    if (var == nullptr)
    {
        return;
    }
    if (*var != 0)
    {
        std::sprintf(msg, "%s=%s", name, var);
        driver_put_string((*row)++, 2, C_GENERAL_HI, msg);
    }
}

static void write_row(int row, char const *format, ...)
{
    char text[78] = { 0 };
    std::va_list args;

    va_start(args, format);
    std::vsnprintf(text, std::size(text), format, args);
    va_end(args);

    driver_put_string(row, 2, C_GENERAL_HI, text);
}

bool tab_display_2(char *msg)
{
    int row;
    int key = 0;

    helptitle();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background

    row = 1;
    putstringcenter(row++, 0, 80, C_PROMPT_HI, "Top Secret Developer's Screen");

    write_row(++row, "Version %d patch %d", g_release, g_patch_level);
    write_row(++row, "%ld of %ld bignum memory used", g_bignum_max_stack_addr, g_max_stack);
    write_row(++row, "   %ld used for bignum globals", g_start_stack);
    write_row(++row, "   %ld stack used == %ld variables of length %d", //
        g_bignum_max_stack_addr - g_start_stack,                           //
        (g_bignum_max_stack_addr - g_start_stack) / (g_r_bf_length + 2), g_r_bf_length + 2);
    if (g_bf_math != bf_math_type::NONE)
    {
        write_row(++row, "g_int_length %-d g_bf_length %-d ", g_int_length, g_bf_length);
    }
    row++;
    show_str_var("tempdir",     g_temp_dir.c_str(),      &row, msg);
    show_str_var("workdir",     g_working_dir.c_str(),      &row, msg);
    show_str_var("filename",    g_read_filename.c_str(),     &row, msg);
    show_str_var("formulafile", g_formula_filename.c_str(), &row, msg);
    show_str_var("savename",    g_save_filename.c_str(),     &row, msg);
    show_str_var("parmfile",    g_command_file.c_str(),  &row, msg);
    show_str_var("ifsfile",     g_ifs_filename.c_str(),  &row, msg);
    show_str_var("autokeyname", g_auto_name.c_str(), &row, msg);
    show_str_var("lightname",   g_light_name.c_str(),   &row, msg);
    show_str_var("map",         g_map_name.c_str(),     &row, msg);
    write_row(row++, "Sizeof fractalspecific array %d",
              g_num_fractal_types*(int)sizeof(fractalspecificstuff));
    write_row(row, "calc_status %d pixel [%d, %d]", g_calc_status, g_col, row);
    ++row;
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        write_row(row++, "Max_Ops (posp) %u Max_Args (vsp) %u",
                  g_operation_index, g_variable_index);
        write_row(row++, "   Store ptr %d Loadptr %d Max_Ops var %u Max_Args var %u LastInitOp %d",
                  g_store_index, g_load_index, g_max_function_ops, g_max_function_args, g_last_init_op);
    }
    else if (g_rhombus_stack[0])
    {
        write_row(row++, "SOI Recursion %d stack free %d %d %d %d %d %d %d %d %d %d",
                  g_max_rhombus_depth+1,
                  g_rhombus_stack[0], g_rhombus_stack[1], g_rhombus_stack[2],
                  g_rhombus_stack[3], g_rhombus_stack[4], g_rhombus_stack[5],
                  g_rhombus_stack[6], g_rhombus_stack[7], g_rhombus_stack[8],
                  g_rhombus_stack[9]);
    }

    /*
        write_row(row++, "xdots %d ydots %d sxdots %d sydots %d", xdots, ydots, sxdots, sydots);
    */
    write_row(row++, "%dx%d dm=%d %s (%s)", g_logical_screen_x_dots, g_logical_screen_y_dots, g_dot_mode,
              g_driver->get_name().c_str(), g_driver->get_description().c_str());
    write_row(row++, "xxstart %d xxstop %d yystart %d yystop %d %s uses_ismand %d",
              g_xx_start, g_xx_stop, g_yy_start, g_yy_stop,
              g_cur_fractal_specific->orbitcalc ==  formula ? "slow parser" :
              g_cur_fractal_specific->orbitcalc ==  bad_formula ? "bad formula" :
              "", g_frm_uses_ismand ? 1 : 0);
    /*
        write_row(row++, "ixstart %d ixstop %d iystart %d iystop %d bitshift %d",
            ixstart, ixstop, iystart, iystop, bitshift);
    */
    write_row(row++, "minstackavail %d llimit2 %ld use_grid %d",
              g_soi_min_stack_available, g_l_magnitude_limit2, g_use_grid ? 1 : 0);
    putstringcenter(24, 0, 80, C_GENERAL_LO, "Press Esc to continue, Backspace for first screen");
    *msg = 0;

    // display keycodes while waiting for ESC, BACKSPACE or TAB
    while ((key != ID_KEY_ESC) && (key != ID_KEY_BACKSPACE) && (key != ID_KEY_TAB))
    {
        driver_put_string(row, 2, C_GENERAL_HI, msg);
        key = getakeynohelp();
        std::sprintf(msg, "%d (0x%04x)      ", key, key);
    }
    return key != ID_KEY_ESC;
}

int tab_display()       // display the status of the current image
{
    int addrow = 0;
    double Xctr;
    double Yctr;
    LDBL Magnification;
    double Xmagfactor;
    double Rotation;
    double Skew;
    bf_t bfXctr = nullptr;
    bf_t bfYctr = nullptr;
    char msg[350];
    char const *msgptr;
    int key;
    int saved = 0;
    int k;
    int hasformparam = 0;

    if (g_calc_status < calc_status_value::PARAMS_CHANGED)        // no active fractal image
    {
        return 0;                // (no TAB on the credits screen)
    }
    if (g_calc_status == calc_status_value::IN_PROGRESS)        // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    {
        g_calc_time += (std::clock() - g_timer_start) / (CLOCKS_PER_SEC/100);
    }
    driver_stack_screen();
    if (g_bf_math != bf_math_type::NONE)
    {
        saved = save_stack();
        bfXctr = alloc_stack(g_bf_length+2);
        bfYctr = alloc_stack(g_bf_length+2);
    }
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        for (int i = 0; i < MAX_PARAMS; i += 2)
        {
            if (!paramnotused(i))
            {
                hasformparam++;
            }
        }
    }

top:
    k = 0; /* initialize here so parameter line displays correctly on return
                from control-tab */
    helptitle();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background
    int s_row = 2;
    driver_put_string(s_row, 2, C_GENERAL_MED, "Fractal type:");
    if (g_display_3d > display_3d_modes::NONE)
    {
        driver_put_string(s_row, 16, C_GENERAL_HI, "3D Transform");
    }
    else
    {
        driver_put_string(s_row, 16, C_GENERAL_HI,
                          g_cur_fractal_specific->name[0] == '*' ?
                          &g_cur_fractal_specific->name[1] : g_cur_fractal_specific->name);
        int i = 0;
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_formula_name);
            i = static_cast<int>(g_formula_name.length() + 1);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            driver_put_string(s_row + 2 + addrow, 16, C_GENERAL_HI, trim_filename(g_formula_filename, 29));
        }
        trigdetails(msg);
        driver_put_string(s_row+1, 16+i, C_GENERAL_HI, msg);
        if (g_fractal_type == fractal_type::LSYSTEM)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_l_system_name);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_l_system_filename.length() >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, g_l_system_filename);
        }
        if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_ifs_name);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_ifs_filename.length() >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, g_ifs_filename);
        }
    }

    switch (g_calc_status)
    {
    case calc_status_value::PARAMS_CHANGED:
        msgptr = "Parms chgd since generated";
        break;
    case calc_status_value::IN_PROGRESS:
        msgptr = "Still being generated";
        break;
    case calc_status_value::RESUMABLE:
        msgptr = "Interrupted, resumable";
        break;
    case calc_status_value::NON_RESUMABLE:
        msgptr = "Interrupted, non-resumable";
        break;
    case calc_status_value::COMPLETED:
        msgptr = "Image completed";
        break;
    default:
        msgptr = "";
    }
    driver_put_string(s_row, 45, C_GENERAL_HI, msgptr);
    if (g_init_batch != batch_modes::NONE && g_calc_status != calc_status_value::PARAMS_CHANGED)
    {
        driver_put_string(-1, -1, C_GENERAL_HI, " (Batch mode)");
    }

    if (g_help_mode == help_labels::HELP_CYCLING)
    {
        driver_put_string(s_row+1, 45, C_GENERAL_HI, "You are in color-cycling mode");
    }
    ++s_row;
    // if (g_bf_math == bf_math_type::NONE)
    ++s_row;

    int j = 0;
    if (g_display_3d > display_3d_modes::NONE)
    {
        if (g_user_float_flag)
        {
            j = 1;
        }
    }
    else if (g_float_flag)
    {
        j = g_user_float_flag ? 1 : 2;
    }

    if (g_bf_math == bf_math_type::NONE)
    {
        if (j)
        {
            driver_put_string(s_row, 45, C_GENERAL_HI, "Floating-point");
            driver_put_string(-1, -1, C_GENERAL_HI,
                              (j == 1) ? " flag is activated" : " in use (required)");
        }
        else
        {
            driver_put_string(s_row, 45, C_GENERAL_HI, "Integer math is in use");
        }
    }
    else
    {
        std::sprintf(msg, "(%-d decimals)", g_decimals /*getprecbf(CURRENTREZ)*/);
        driver_put_string(s_row, 45, C_GENERAL_HI, "Arbitrary precision ");
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }
    s_row += 1;

    if (g_calc_status == calc_status_value::IN_PROGRESS || g_calc_status == calc_status_value::RESUMABLE)
    {
        if (bit_set(g_cur_fractal_specific->flags, fractal_flags::NORESUME))
        {
            driver_put_string(s_row++, 2, C_GENERAL_HI,
                              "Note: can't resume this type after interrupts other than <tab> and <F1>");
        }
    }
    s_row += addrow;
    driver_put_string(s_row, 2, C_GENERAL_MED, "Savename: ");
    driver_put_string(s_row, -1, C_GENERAL_HI, g_save_filename);

    ++s_row;

    if (g_got_status >= status_values::ONE_OR_TWO_PASS &&
        (g_calc_status == calc_status_value::IN_PROGRESS || g_calc_status == calc_status_value::RESUMABLE))
    {
        switch (g_got_status)
        {
        case status_values::ONE_OR_TWO_PASS:
            std::sprintf(msg, "%d Pass Mode", g_total_passes);
            driver_put_string(s_row, 2, C_GENERAL_HI, msg);
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case status_values::SOLID_GUESS:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Solid Guessing");
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case status_values::BOUNDARY_TRACE:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Boundary Tracing");
            break;
        case status_values::THREE_D:
            std::sprintf(msg, "Processing row %d (of %d) of input image", g_current_row, g_file_y_dots);
            driver_put_string(s_row, 2, C_GENERAL_HI, msg);
            break;
        case status_values::TESSERAL:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Tesseral");
            break;
        case status_values::DIFFUSION:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Diffusion");
            break;
        case status_values::ORBITS:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Orbits");
            break;
        case status_values::NONE:
            break;
        }
        ++s_row;
        if (g_got_status == status_values::DIFFUSION)
        {
            std::sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
                    (100.0 * g_diffusion_counter)/g_diffusion_limit,
                    g_diffusion_counter, g_diffusion_limit, g_diffusion_bits);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            ++s_row;
        }
        else if (g_got_status != status_values::THREE_D)
        {
            std::sprintf(msg, "Working on block (y, x) [%d, %d]...[%d, %d], ",
                    g_yy_start, g_xx_start, g_yy_stop, g_xx_stop);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            if (g_got_status == status_values::BOUNDARY_TRACE || g_got_status == status_values::TESSERAL)
            {
                driver_put_string(-1, -1, C_GENERAL_MED, "at ");
                std::sprintf(msg, "[%d, %d]", g_current_row, g_current_column);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            else
            {
                if (g_total_passes > 1)
                {
                    driver_put_string(-1, -1, C_GENERAL_MED, "pass ");
                    std::sprintf(msg, "%d", g_current_pass);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, " of ");
                    std::sprintf(msg, "%d", g_total_passes);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, ", ");
                }
                driver_put_string(-1, -1, C_GENERAL_MED, "at row ");
                std::sprintf(msg, "%d", g_current_row);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                driver_put_string(-1, -1, C_GENERAL_MED, " col ");
                std::sprintf(msg, "%d", g_col);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            ++s_row;
        }
    }
    driver_put_string(s_row, 2, C_GENERAL_MED, "Calculation time:");
    strncpy(msg, get_calculation_time(g_calc_time).c_str(), std::size(msg));
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    if (g_got_status == status_values::DIFFUSION &&
        g_calc_status == calc_status_value::IN_PROGRESS) // estimate total time
    {
        driver_put_string(-1, -1, C_GENERAL_MED, " estimated total time: ");
        const std::string time{get_calculation_time(
            (long) (g_calc_time * (static_cast<double>(g_diffusion_limit) / g_diffusion_counter)))};
        strncpy(msg, time.c_str(),std::size(msg));
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (bit_set(g_cur_fractal_specific->flags, fractal_flags::INFCALC) && g_color_iter != 0)
    {
        driver_put_string(s_row, -1, C_GENERAL_MED, " 1000's of points:");
        std::sprintf(msg, " %ld of %ld", g_color_iter-2, g_max_count);
        driver_put_string(s_row, -1, C_GENERAL_HI, msg);
    }

    ++s_row;
    if (g_bf_math == bf_math_type::NONE)
    {
        ++s_row;
    }
    std::snprintf(
        msg, std::size(msg), "Driver: %s, %s", g_driver->get_name().c_str(), g_driver->get_description().c_str());
    driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    if (g_video_entry.xdots && g_bf_math == bf_math_type::NONE)
    {
        std::sprintf(msg, "Video: %dx%dx%d %s",
                g_video_entry.xdots, g_video_entry.ydots, g_video_entry.colors,
                g_video_entry.comment);
        driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    }
    if (bit_clear(g_cur_fractal_specific->flags, fractal_flags::NOZOOM))
    {
        adjust_corner(); // make bottom left exact if very near exact
        if (g_bf_math != bf_math_type::NONE)
        {
            int truncaterow;
            int dec = std::min(320, g_decimals);
            adjust_cornerbf(); // make bottom left exact if very near exact
            cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            // find alignment information
            msg[0] = 0;
            bool truncate = false;
            if (dec < g_decimals)
            {
                truncate = true;
            }
            truncaterow = g_row;
            driver_put_string(++s_row, 2, C_GENERAL_MED, "Ctr");
            driver_put_string(s_row, 8, C_GENERAL_MED, "x");
            bftostr(msg, dec, bfXctr);
            if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5))
            {
                truncate = true;
            }
            driver_put_string(++s_row, 8, C_GENERAL_MED, "y");
            bftostr(msg, dec, bfYctr);
            if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5) || truncate)
            {
                driver_put_string(truncaterow, 2, C_GENERAL_MED, "(Center values shown truncated to 320 decimals)");
            }
            driver_put_string(++s_row, 2, C_GENERAL_MED, "Mag");
            std::sprintf(msg, "%10.8Le", Magnification);
            driver_put_string(-1, 11, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
        else // bf != 1
        {
            driver_put_string(s_row, 2, C_GENERAL_MED, "Corners:                X                     Y");
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Top-l");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_min, g_y_max);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-r");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_max, g_y_min);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);

            if (g_x_min != g_x_3rd || g_y_min != g_y_3rd)
            {
                driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-l");
                std::sprintf(msg, "%20.16f  %20.16f", g_x_3rd, g_y_3rd);
                driver_put_string(-1, 17, C_GENERAL_HI, msg);
            }
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Ctr");
            std::sprintf(msg, "%20.16f %20.16f  ", Xctr, Yctr);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Mag");
            std::sprintf(msg, " %10.8Le", Magnification);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
    }

    if (typehasparm(g_fractal_type, 0, msg) || hasformparam)
    {
        for (int i = 0; i < MAX_PARAMS; i++)
        {
            char p[50];
            if (typehasparm(g_fractal_type, i, p))
            {
                int col;
                if (k%4 == 0)
                {
                    s_row++;
                    col = 9;
                }
                else
                {
                    col = -1;
                }
                if (k == 0)   // only true with first displayed parameter
                {
                    driver_put_string(++s_row, 2, C_GENERAL_MED, "Params ");
                }
                std::sprintf(msg, "%3d: ", i+1);
                driver_put_string(s_row, col, C_GENERAL_MED, msg);
                if (*p == '+')
                {
                    std::sprintf(msg, "%-12d", (int)g_params[i]);
                }
                else if (*p == '#')
                {
                    std::sprintf(msg, "%-12u", (U32)g_params[i]);
                }
                else
                {
                    std::sprintf(msg, "%-12.9f", g_params[i]);
                }
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                k++;
            }
        }
    }
    driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Current (Max) Iteration: ");
    std::sprintf(msg, "%ld (%ld)", g_color_iter, g_max_iterations);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    driver_put_string(-1, -1, C_GENERAL_MED, "     Effective bailout: ");
    std::sprintf(msg, "%f", g_magnitude_limit);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);

    if (g_fractal_type == fractal_type::PLASMA || g_fractal_type == fractal_type::ANT || g_fractal_type == fractal_type::CELLULAR)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Current 'rseed': ");
        std::sprintf(msg, "%d", g_random_seed);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (g_invert != 0)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Inversion radius: ");
        std::sprintf(msg, "%12.9f", g_f_radius);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  xcenter: ");
        std::sprintf(msg, "%12.9f", g_f_x_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  ycenter: ");
        std::sprintf(msg, "%12.9f", g_f_y_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if ((s_row += 2) < 23)
    {
        ++s_row;
    }
    putstringcenter(
        24, 0, 80, C_GENERAL_LO, "Press any key to continue, F6 for area, Ctrl+Tab for next page");
    driver_hide_text_cursor();
#ifdef XFRACT
    while (driver_key_pressed())
    {
        driver_get_key();
    }
#endif
    key = getakeynohelp();
    if (key == ID_KEY_F6)
    {
        driver_stack_screen();
        area();
        driver_unstack_screen();
        goto top;
    }
    else if (key == ID_KEY_CTL_TAB || key == ID_KEY_SHF_TAB || key == ID_KEY_F7)
    {
        if (tab_display_2(msg))
        {
            goto top;
        }
    }
    driver_unstack_screen();
    g_timer_start = std::clock(); // tab display was "time out"
    if (g_bf_math != bf_math_type::NONE)
    {
        restore_stack(saved);
    }
    return 0;
}

static void area()
{
    char const *msg;
    char buf[160];
    long cnt = 0;
    if (g_inside_color < COLOR_BLACK)
    {
        stopmsg("Need solid inside to compute area");
        return;
    }
    for (int y = 0; y < g_logical_screen_y_dots; y++)
    {
        for (int x = 0; x < g_logical_screen_x_dots; x++)
        {
            if (getcolor(x, y) == g_inside_color)
            {
                cnt++;
            }
        }
    }
    if (g_inside_color > COLOR_BLACK && g_outside_color < COLOR_BLACK && g_max_iterations > g_inside_color)
    {
        msg = "Warning: inside may not be unique\n";
    }
    else
    {
        msg = "";
    }
    std::sprintf(buf, "%s%ld inside pixels of %ld%s%f",
            msg, cnt, (long)g_logical_screen_x_dots*(long)g_logical_screen_y_dots, ".  Total area ",
            cnt/((float)g_logical_screen_x_dots*(float)g_logical_screen_y_dots)*(g_x_max-g_x_min)*(g_y_max-g_y_min));
    stopmsg(stopmsg_flags::NO_BUZZER, buf);
}
