// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/tab_display.h"

#include "3d/line3d.h"
#include "engine/calc_frac_init.h"
#include "engine/convert_center_mag.h"
#include "engine/diffusion_scan.h"
#include "engine/engine_timer.h"
#include "engine/id_data.h"
#include "engine/param_not_used.h"
#include "engine/perturbation.h"
#include "engine/pixel_grid.h"
#include "engine/soi.h"
#include "engine/type_has_param.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "fractals/parser.h"
#include "io/loadfile.h"
#include "io/trim_filename.h"
#include "math/biginit.h"
#include "misc/Driver.h"
#include "misc/version.h"
#include "ui/cmdfiles.h"
#include "ui/get_calculation_time.h"
#include "ui/get_key_no_help.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/put_string_center.h"
#include "ui/rotate.h"
#include "ui/slideshw.h"
#include "ui/stop_msg.h"
#include "ui/trig_fns.h"
#include "ui/video.h"
#include "ui/video_mode.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

static void area();

// Wrapping version of putstring for long numbers
// row     -- pointer to row variable, internally incremented if needed
// col1    -- starting column
// col2    -- last column
// color   -- attribute (same as for putstring)
// maxrow -- max number of rows to write
// returns false if success, true if hit maxrow before done
static bool put_string_wrap(int *row, int col1, int col2, int color, char *str, int max_row)
{
    int dec_pt;
    bool done = false;
    int start_row = *row;
    int length = (int) std::strlen(str);
    int padding = 3; // space between col1 and decimal.
    // find decimal point
    for (dec_pt = 0; dec_pt < length; dec_pt++)
    {
        if (str[dec_pt] == '.')
        {
            break;
        }
    }
    if (dec_pt >= length)
    {
        dec_pt = 0;
    }
    if (dec_pt < padding)
    {
        padding -= dec_pt;
    }
    else
    {
        padding = 0;
    }
    col1 += padding;
    dec_pt += col1+1; // column just past where decimal is
    while (length > 0)
    {
        if (col2-col1 < length)
        {
            done = (*row - start_row + 1) >= max_row;
            char save1 = str[col2 - col1 + 1];
            char save2 = str[col2 - col1 + 2];
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
        col1 = dec_pt; // align with decimal
    }
    return done;
}

static void show_str_var(const char *name, const char *var, int *row, char *msg)
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

static void write_row(int row, const char *format, ...)
{
    char text[78]{};
    std::va_list args;

    va_start(args, format);
    std::vsnprintf(text, std::size(text), format, args);
    va_end(args);

    driver_put_string(row, 2, C_GENERAL_HI, text);
}

static bool tab_display2(char *msg)
{
    int key = 0;

    help_title();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background

    int row = 1;
    put_string_center(row++, 0, 80, C_PROMPT_HI, "Top Secret Developer's Screen");

    write_row(++row, "Version %d patch %d", g_release, g_patch_level);
    write_row(++row, "%ld of %ld bignum memory used", g_bignum_max_stack_addr, g_max_stack);
    write_row(++row, "   %ld used for bignum globals", g_start_stack);
    write_row(++row, "   %ld stack used == %ld variables of length %d", //
        g_bignum_max_stack_addr - g_start_stack,                           //
        (g_bignum_max_stack_addr - g_start_stack) / (g_r_bf_length + 2), g_r_bf_length + 2);
    if (g_bf_math != BFMathType::NONE)
    {
        write_row(++row, "g_int_length %-d g_bf_length %-d ", g_int_length, g_bf_length);
    }
    row++;
    show_str_var("tempdir",     g_temp_dir.string().c_str(),      &row, msg);
    show_str_var("workdir",     g_working_dir.string().c_str(),      &row, msg);
    show_str_var("filename",    g_read_filename.c_str(),     &row, msg);
    show_str_var("formulafile", g_formula_filename.c_str(), &row, msg);
    show_str_var("savename",    g_save_filename.c_str(),     &row, msg);
    show_str_var("parmfile",    g_command_file.c_str(),  &row, msg);
    show_str_var("ifsfile",     g_ifs_filename.c_str(),  &row, msg);
    show_str_var("autokeyname", g_auto_name.c_str(), &row, msg);
    show_str_var("lightname",   g_light_name.c_str(),   &row, msg);
    show_str_var("map",         g_map_name.c_str(),     &row, msg);
    write_row(row++, "Sizeof fractalspecific array %d",
              g_num_fractal_types*(int)sizeof(FractalSpecific));
    write_row(row, "calc_status %d pixel [%d, %d]", g_calc_status, g_col, row);
    ++row;
    if (g_fractal_type == FractalType::FORMULA)
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
    write_row(row++, "%dx%d %s (%s)", g_logical_screen_x_dots, g_logical_screen_y_dots,
              g_driver->get_name().c_str(), g_driver->get_description().c_str());
    write_row(row++, "xxstart %d xxstop %d yystart %d yystop %d %s uses_ismand %d",
              g_start_pt.x, g_stop_pt.x, g_start_pt.y, g_stop_pt.y,
              g_cur_fractal_specific->orbit_calc ==  formula_orbit ? "slow parser" :
              g_cur_fractal_specific->orbit_calc ==  bad_formula ? "bad formula" :
              "", g_frm_uses_ismand ? 1 : 0);
    /*
        write_row(row++, "ixstart %d ixstop %d iystart %d iystop %d bitshift %d",
            ixstart, ixstop, iystart, iystop, bitshift);
    */
    write_row(row++, "minstackavail %d use_grid %s", g_soi_min_stack_available, g_use_grid ? "yes" : "no");
    put_string_center(24, 0, 80, C_GENERAL_LO, "Press Esc to continue, Backspace for first screen");
    *msg = 0;

    // display keycodes while waiting for ESC, BACKSPACE or TAB
    while ((key != ID_KEY_ESC) && (key != ID_KEY_BACKSPACE) && (key != ID_KEY_TAB))
    {
        driver_put_string(row, 2, C_GENERAL_HI, msg);
        key = get_a_key_no_help();
        std::sprintf(msg, "%d (0x%04x)      ", key, static_cast<unsigned int>(key));
    }
    return key != ID_KEY_ESC;
}

int tab_display()       // display the status of the current image
{
    int add_row = 0;
    BigFloat bf_x_ctr = nullptr;
    BigFloat bf_y_ctr = nullptr;
    char msg[350];
    const char *msg_ptr;
    int saved = 0;
    int has_form_param = 0;

    if (g_calc_status < CalcStatus::PARAMS_CHANGED)        // no active fractal image
    {
        return 0;                // (no TAB on the credits screen)
    }
    if (g_calc_status == CalcStatus::IN_PROGRESS)        // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    {
        g_calc_time += (std::clock() - g_timer_start) / (CLOCKS_PER_SEC/100);
    }
    driver_stack_screen();
    if (g_bf_math != BFMathType::NONE)
    {
        saved = save_stack();
        bf_x_ctr = alloc_stack(g_bf_length+2);
        bf_y_ctr = alloc_stack(g_bf_length+2);
    }
    if (g_fractal_type == FractalType::FORMULA)
    {
        for (int i = 0; i < MAX_PARAMS; i += 2)
        {
            if (!param_not_used(i))
            {
                has_form_param++;
            }
        }
    }

top:
    int k = 0; /* initialize here so parameter line displays correctly on return
                from control-tab */
    help_title();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background
    int start_row = 2;
    driver_put_string(start_row, 2, C_GENERAL_MED, "Fractal type:");
    if (g_display_3d > Display3DMode::NONE)
    {
        driver_put_string(start_row, 16, C_GENERAL_HI, "3D Transform");
    }
    else
    {
        driver_put_string(start_row, 16, C_GENERAL_HI, g_cur_fractal_specific->name);
        int i = 0;
        if (g_fractal_type == FractalType::FORMULA)
        {
            driver_put_string(start_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(start_row+1, 16, C_GENERAL_HI, g_formula_name);
            i = static_cast<int>(g_formula_name.length() + 1);
            driver_put_string(start_row+2, 3, C_GENERAL_MED, "Item file:");
            driver_put_string(start_row + 2 + add_row, 16, C_GENERAL_HI, trim_file_name(g_formula_filename, 29));
        }
        trig_details(msg);
        driver_put_string(start_row+1, 16+i, C_GENERAL_HI, msg);
        if (g_fractal_type == FractalType::L_SYSTEM)
        {
            driver_put_string(start_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(start_row+1, 16, C_GENERAL_HI, g_l_system_name);
            driver_put_string(start_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_l_system_filename.length() >= 28)
            {
                add_row = 1;
            }
            driver_put_string(start_row+2+add_row, 16, C_GENERAL_HI, g_l_system_filename);
        }
        if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
        {
            driver_put_string(start_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(start_row+1, 16, C_GENERAL_HI, g_ifs_name);
            driver_put_string(start_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_ifs_filename.length() >= 28)
            {
                add_row = 1;
            }
            driver_put_string(start_row+2+add_row, 16, C_GENERAL_HI, g_ifs_filename);
        }
    }

    switch (g_calc_status)
    {
    case CalcStatus::PARAMS_CHANGED:
        msg_ptr = "Parms chgd since generated";
        break;
    case CalcStatus::IN_PROGRESS:
        msg_ptr = "Still being generated";
        break;
    case CalcStatus::RESUMABLE:
        msg_ptr = "Interrupted, resumable";
        break;
    case CalcStatus::NON_RESUMABLE:
        msg_ptr = "Interrupted, non-resumable";
        break;
    case CalcStatus::COMPLETED:
        msg_ptr = "Image completed";
        break;
    default:
        msg_ptr = "";
    }
    driver_put_string(start_row, 45, C_GENERAL_HI, msg_ptr);
    if (g_init_batch != BatchMode::NONE && g_calc_status != CalcStatus::PARAMS_CHANGED)
    {
        driver_put_string(-1, -1, C_GENERAL_HI, " (Batch mode)");
    }

    if (g_help_mode == HelpLabels::HELP_CYCLING)
    {
        driver_put_string(start_row+1, 45, C_GENERAL_HI, "You are in color-cycling mode");
    }
    ++start_row;
    // if (g_bf_math == bf_math_type::NONE)
    ++start_row;

    if (g_bf_math == BFMathType::NONE)
    {
        driver_put_string(start_row, 45, C_GENERAL_HI, "Floating-point in use");
    }
    else
    {
        std::sprintf(msg, "(%-d decimals)", g_decimals /*getprecbf(Resolution::CURRENT)*/);
        driver_put_string(start_row, 45, C_GENERAL_HI, "Arbitrary precision ");
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }
    start_row += 1;

   if (g_use_perturbation)
    {
       int ref = get_number_references();
       std::sprintf(msg, (ref == 1) ? " (%d reference)" : " (%d references)", ref);
       driver_put_string(start_row, 45, C_GENERAL_HI, "Perturbation");
       driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }
    start_row += 1;

    if (g_calc_status == CalcStatus::IN_PROGRESS || g_calc_status == CalcStatus::RESUMABLE)
    {
        if (bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_RESUME))
        {
            driver_put_string(start_row++, 2, C_GENERAL_HI,
                              "Note: can't resume this type after interrupts other than <Tab> and <F1>");
        }
    }
    start_row += add_row;
    driver_put_string(start_row, 2, C_GENERAL_MED, "Savename: ");
    driver_put_string(start_row, -1, C_GENERAL_HI, g_save_filename);

    ++start_row;

    if (g_got_status >= StatusValues::ONE_OR_TWO_PASS &&
        (g_calc_status == CalcStatus::IN_PROGRESS || g_calc_status == CalcStatus::RESUMABLE))
    {
        switch (g_got_status)
        {
        case StatusValues::ONE_OR_TWO_PASS:
            std::sprintf(msg, "%d Pass Mode", g_total_passes);
            driver_put_string(start_row, 2, C_GENERAL_HI, msg);
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(start_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case StatusValues::SOLID_GUESS:
            driver_put_string(start_row, 2, C_GENERAL_HI, "Solid Guessing");
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(start_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case StatusValues::BOUNDARY_TRACE:
            driver_put_string(start_row, 2, C_GENERAL_HI, "Boundary Tracing");
            break;
        case StatusValues::THREE_D:
            std::sprintf(msg, "Processing row %d (of %d) of input image", g_current_row, g_file_y_dots);
            driver_put_string(start_row, 2, C_GENERAL_HI, msg);
            break;
        case StatusValues::TESSERAL:
            driver_put_string(start_row, 2, C_GENERAL_HI, "Tesseral");
            break;
        case StatusValues::DIFFUSION:
            driver_put_string(start_row, 2, C_GENERAL_HI, "Diffusion");
            break;
        case StatusValues::ORBITS:
            driver_put_string(start_row, 2, C_GENERAL_HI, "Orbits");
            break;
        case StatusValues::NONE:
            break;
        }
        ++start_row;
        if (g_got_status == StatusValues::DIFFUSION)
        {
            std::sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
                    (100.0 * g_diffusion_counter)/g_diffusion_limit,
                    g_diffusion_counter, g_diffusion_limit, g_diffusion_bits);
            driver_put_string(start_row, 2, C_GENERAL_MED, msg);
            ++start_row;
        }
        else if (g_got_status != StatusValues::THREE_D)
        {
            std::sprintf(msg, "Working on block (y, x) [%d, %d]...[%d, %d], ",
                    g_start_pt.y, g_start_pt.x, g_stop_pt.y, g_stop_pt.x);
            driver_put_string(start_row, 2, C_GENERAL_MED, msg);
            if (g_got_status == StatusValues::BOUNDARY_TRACE || g_got_status == StatusValues::TESSERAL)
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
            ++start_row;
        }
    }
    driver_put_string(start_row, 2, C_GENERAL_MED, "Calculation time:");
    strncpy(msg, get_calculation_time(g_calc_time).c_str(), std::size(msg));
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    if (g_got_status == StatusValues::DIFFUSION &&
        g_calc_status == CalcStatus::IN_PROGRESS) // estimate total time
    {
        driver_put_string(-1, -1, C_GENERAL_MED, " estimated total time: ");
        const std::string time{get_calculation_time(
            (long) (g_calc_time * (static_cast<double>(g_diffusion_limit) / g_diffusion_counter)))};
        strncpy(msg, time.c_str(),std::size(msg));
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (bit_set(g_cur_fractal_specific->flags, FractalFlags::INF_CALC) && g_color_iter != 0)
    {
        driver_put_string(start_row, -1, C_GENERAL_MED, " 1000's of points:");
        std::sprintf(msg, " %ld of %ld", g_color_iter-2, g_max_count);
        driver_put_string(start_row, -1, C_GENERAL_HI, msg);
    }

    ++start_row;
    if (g_bf_math == BFMathType::NONE)
    {
        ++start_row;
    }
    std::snprintf(
        msg, std::size(msg), "Driver: %s, %s", g_driver->get_name().c_str(), g_driver->get_description().c_str());
    driver_put_string(start_row++, 2, C_GENERAL_MED, msg);
    if (g_video_entry.x_dots && g_bf_math == BFMathType::NONE)
    {
        std::sprintf(msg, "Video: %dx%dx%d %s",
                g_video_entry.x_dots, g_video_entry.y_dots, g_video_entry.colors,
                g_video_entry.comment);
        driver_put_string(start_row++, 2, C_GENERAL_MED, msg);
    }
    if (bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_ZOOM))
    {
        LDouble magnification;
        double x_mag_factor;
        double rotation;
        double skew;
        adjust_corner(); // make bottom left exact if very near exact
        if (g_bf_math != BFMathType::NONE)
        {
            int dec = std::min(320, g_decimals);
            adjust_corner_bf(); // make bottom left exact if very near exact
            cvt_center_mag_bf(bf_x_ctr, bf_y_ctr, magnification, x_mag_factor, rotation, skew);
            // find alignment information
            msg[0] = 0;
            bool truncate = false;
            if (dec < g_decimals)
            {
                truncate = true;
            }
            int truncate_row = g_row;
            driver_put_string(++start_row, 2, C_GENERAL_MED, "Ctr");
            driver_put_string(start_row, 8, C_GENERAL_MED, "x");
            bf_to_str(msg, dec, bf_x_ctr);
            if (put_string_wrap(&start_row, 10, 78, C_GENERAL_HI, msg, 5))
            {
                truncate = true;
            }
            driver_put_string(++start_row, 8, C_GENERAL_MED, "y");
            bf_to_str(msg, dec, bf_y_ctr);
            if (put_string_wrap(&start_row, 10, 78, C_GENERAL_HI, msg, 5) || truncate)
            {
                driver_put_string(truncate_row, 2, C_GENERAL_MED, "(Center values shown truncated to 320 decimals)");
            }
            driver_put_string(++start_row, 2, C_GENERAL_MED, "Mag");
            std::sprintf(msg, "%10.8Le", magnification);
            driver_put_string(-1, 11, C_GENERAL_HI, msg);
            driver_put_string(++start_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", x_mag_factor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
        else // bf != 1
        {
            double x_ctr;
            double y_ctr;
            driver_put_string(start_row, 2, C_GENERAL_MED, "Corners:                X                     Y");
            driver_put_string(++start_row, 3, C_GENERAL_MED, "Top-l");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_min, g_y_max);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);
            driver_put_string(++start_row, 3, C_GENERAL_MED, "Bot-r");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_max, g_y_min);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);

            if (g_x_min != g_x_3rd || g_y_min != g_y_3rd)
            {
                driver_put_string(++start_row, 3, C_GENERAL_MED, "Bot-l");
                std::sprintf(msg, "%20.16f  %20.16f", g_x_3rd, g_y_3rd);
                driver_put_string(-1, 17, C_GENERAL_HI, msg);
            }
            cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
            driver_put_string(start_row += 2, 2, C_GENERAL_MED, "Ctr");
            std::sprintf(msg, "%20.16f %20.16f  ", x_ctr, y_ctr);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Mag");
            std::sprintf(msg, " %10.8Le", magnification);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(++start_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", x_mag_factor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
    }

    if (type_has_param(g_fractal_type, 0, msg) || has_form_param)
    {
        for (int i = 0; i < MAX_PARAMS; i++)
        {
            char p[50];
            if (type_has_param(g_fractal_type, i, p))
            {
                int col;
                if (k%4 == 0)
                {
                    start_row++;
                    col = 9;
                }
                else
                {
                    col = -1;
                }
                if (k == 0)   // only true with first displayed parameter
                {
                    driver_put_string(++start_row, 2, C_GENERAL_MED, "Params ");
                }
                std::sprintf(msg, "%3d: ", i+1);
                driver_put_string(start_row, col, C_GENERAL_MED, msg);
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
                    if ((std::abs(g_params[i]) < 0.00001 || std::abs(g_params[i]) > 100000.0) && g_params[i] != 0.0)
                    {
                        std::sprintf(msg, "%-12.9e", g_params[i]);
                    }
                    else
                    {
                        std::sprintf(msg, "%-12.9f", g_params[i]);
                    }
                }
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                k++;
            }
        }
    }
    driver_put_string(start_row += 2, 2, C_GENERAL_MED, "Current (Max) Iteration: ");
    std::sprintf(msg, "%ld (%ld)", g_color_iter, g_max_iterations);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    driver_put_string(-1, -1, C_GENERAL_MED, "     Effective bailout: ");
    std::sprintf(msg, "%f", g_magnitude_limit);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);

    if (g_fractal_type == FractalType::PLASMA || g_fractal_type == FractalType::ANT || g_fractal_type == FractalType::CELLULAR)
    {
        driver_put_string(++start_row, 2, C_GENERAL_MED, "Current 'rseed': ");
        std::sprintf(msg, "%d", g_random_seed);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (g_invert != 0)
    {
        driver_put_string(++start_row, 2, C_GENERAL_MED, "Inversion radius: ");
        std::sprintf(msg, "%12.9f", g_f_radius);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  xcenter: ");
        std::sprintf(msg, "%12.9f", g_f_x_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  ycenter: ");
        std::sprintf(msg, "%12.9f", g_f_y_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if ((start_row += 2) < 23)
    {
        ++start_row;
    }
    put_string_center(
        24, 0, 80, C_GENERAL_LO, "Press any key to continue, F6 for area, Ctrl+Tab for next page");
    driver_hide_text_cursor();
    int key = get_a_key_no_help();
    if (key == ID_KEY_F6)
    {
        driver_stack_screen();
        area();
        driver_unstack_screen();
        goto top;
    }
    if (key == ID_KEY_CTL_TAB || key == ID_KEY_SHF_TAB || key == ID_KEY_F7)
    {
        if (tab_display2(msg))
        {
            goto top;
        }
    }
    driver_unstack_screen();
    g_timer_start = std::clock(); // tab display was "time out"
    if (g_bf_math != BFMathType::NONE)
    {
        restore_stack(saved);
    }
    return 0;
}

static void area()
{
    const char *msg;
    char buf[160];
    long cnt = 0;
    if (g_inside_color < COLOR_BLACK)
    {
        stop_msg("Need solid inside to compute area");
        return;
    }
    for (int y = 0; y < g_logical_screen_y_dots; y++)
    {
        for (int x = 0; x < g_logical_screen_x_dots; x++)
        {
            if (get_color(x, y) == g_inside_color)
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
    stop_msg(StopMsgFlags::NO_BUZZER, buf);
}
