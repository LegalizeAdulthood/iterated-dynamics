// SPDX-License-Identifier: GPL-3.0-only
//
/*
        load an existing fractal image, control level
*/
#include "ui/file_get_window.h"

#include "config/path_limits.h"
#include "config/string_case_compare.h"
#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/lorenz.h"
#include "fractals/parser.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "io/make_path.h"
#include "io/split_path.h"
#include "math/big.h"
#include "math/biginit.h"
#include "math/round_float_double.h"
#include "misc/Driver.h"
#include "ui/field_prompt.h"
#include "ui/find_special_colors.h"
#include "ui/framain2.h"
#include "ui/get_browse_params.h"
#include "ui/id_keys.h"
#include "ui/temp_msg.h"
#include "ui/trig_fns.h"
#include "ui/zoom.h"

#include <fmt/format.h>

#include <filesystem>

namespace
{

enum
{
    MAX_WINDOWS_OPEN = 450
};

constexpr double MIN_DIF{0.001};

struct DblCoords
{
    double x;
    double y;
};

// for file_get_window on screen browser
struct FileWindow
{
    void draw(int color);
    bool is_visible(const FractalInfo *info, const ExtBlock5 *blk_5_info);

    Coord top_left;             // screen coordinates
    Coord bot_left;             //
    Coord top_right;            //
    Coord bot_right;            //
    double win_size;            // box size for draw_window()
    std::string filename;       // for filename
    std::filesystem::path path; // full path to file
    int box_count;              // bytes of saved screen info
};

} // namespace

// prototypes
static void transform(DblCoords *point);
static bool params_ok(const FractalInfo *info);
static bool type_ok(const FractalInfo *info, const ExtBlock3 *blk_3_info);
static bool function_ok(const FractalInfo *info, int num_fn);
static void check_history(const char *old_name, const char *new_name);
static void bf_setup_convert_to_screen();
static void bf_transform(BigFloat bt_x, BigFloat bt_y, DblCoords *point);

static std::FILE *s_fp{};
static std::vector<FileWindow> s_browse_windows;
static std::vector<int> s_browse_box_x;
static std::vector<int> s_browse_box_y;
static std::vector<int> s_browse_box_values;
// here because must be visible inside several routines
static Affine *s_cvt{};
static BigFloat s_bt_a{};
static BigFloat s_bt_b{};
static BigFloat s_bt_c{};
static BigFloat s_bt_d{};
static BigFloat s_bt_e{};
static BigFloat s_bt_f{};
static BigFloat s_n_a{};
static BigFloat s_n_b{};
static BigFloat s_n_c{};
static BigFloat s_n_d{};
static BigFloat s_n_e{};
static BigFloat s_n_f{};
static BFMathType s_old_bf_math{};

// browse code RB

static void save_box(int num_dots, int which)
{
    std::copy(&g_box_x[0], &g_box_x[num_dots], &s_browse_box_x[num_dots*which]);
    std::copy(&g_box_y[0], &g_box_y[num_dots], &s_browse_box_y[num_dots*which]);
    std::copy(&g_box_values[0], &g_box_values[num_dots], &s_browse_box_values[num_dots*which]);
}

static void restore_box(int num_dots, int which)
{
    std::copy(&s_browse_box_x[num_dots*which], &s_browse_box_x[num_dots*(which + 1)], &g_box_x[0]);
    std::copy(&s_browse_box_y[num_dots*which], &s_browse_box_y[num_dots*(which + 1)], &g_box_y[0]);
    std::copy(&s_browse_box_values[num_dots*which], &s_browse_box_values[num_dots*(which + 1)], &g_box_values[0]);
}

/* on exit done = 1 for quick exit,
           done = 2 for erase boxes and  exit
           done = 3 for rescan
           done = 4 for set boxes and exit to save image */
enum class FileWindowStatus
{
    CONTINUE = 0,
    EXIT = 1,
    ERASE_BOXES_EXIT = 2,
    RESCAN = 3,
    SAVE_BOXES_EXIT = 4,
};

// TODO: move this to the UI folder
int file_get_window()
{
    Affine stack_cvt;
    std::time_t this_time;
    std::time_t last_time;
    int c;
    FileWindowStatus status{};
    int win_count;
    bool toggle{};
    int color_of_box;
    FileWindow window;
    char drive[id::ID_FILE_MAX_DRIVE];
    char dir[id::ID_FILE_MAX_DIR];
    char fname[id::ID_FILE_MAX_FNAME];
    char ext[id::ID_FILE_MAX_EXT];
    char tmp_mask[id::ID_FILE_MAX_PATH];
    bool vid_too_big{};
    int saved;

    s_old_bf_math = g_bf_math;
    g_bf_math = BFMathType::BIG_FLT;
    if (s_old_bf_math == BFMathType::NONE)
    {
        CalcStatus old_calc_status = g_calc_status; // kludge because next sets it = 0
        id::fractal_float_to_bf();
        g_calc_status = old_calc_status;
    }
    saved = save_stack();
    s_bt_a = alloc_stack(g_r_bf_length+2);
    s_bt_b = alloc_stack(g_r_bf_length+2);
    s_bt_c = alloc_stack(g_r_bf_length+2);
    s_bt_d = alloc_stack(g_r_bf_length+2);
    s_bt_e = alloc_stack(g_r_bf_length+2);
    s_bt_f = alloc_stack(g_r_bf_length + 2);

    const int num_dots = g_screen_x_dots + g_screen_y_dots;
    vid_too_big = num_dots > 4096;
    s_browse_box_x.resize(num_dots*MAX_WINDOWS_OPEN);
    s_browse_box_y.resize(num_dots*MAX_WINDOWS_OPEN);
    s_browse_box_values.resize(num_dots*MAX_WINDOWS_OPEN);

    // set up complex-plane-to-screen transformation
    if (s_old_bf_math != BFMathType::NONE)
    {
        bf_setup_convert_to_screen();
    }
    else
    {
        s_cvt = &stack_cvt; // use stack
        setup_convert_to_screen(s_cvt);
        // put in bf variables
        float_to_bf(s_bt_a, s_cvt->a);
        float_to_bf(s_bt_b, s_cvt->b);
        float_to_bf(s_bt_c, s_cvt->c);
        float_to_bf(s_bt_d, s_cvt->d);
        float_to_bf(s_bt_e, s_cvt->e);
        float_to_bf(s_bt_f, s_cvt->f);
    }
    find_special_colors();
    color_of_box = g_color_medium;
rescan:  // entry for changed browse parms
    std::time(&last_time);
    toggle = false;
    win_count = 0;
    g_browse_sub_images = true;
    split_drive_dir(g_read_filename, drive, dir);
    split_fname_ext(g_browse_mask, fname, ext);
    make_path(tmp_mask, drive, dir, fname, ext);
    std::filesystem::path path{id::io::find_wildcard_first(id::io::ReadFile::IMAGE, tmp_mask)};
    status = (vid_too_big || path.empty()) ? FileWindowStatus::EXIT : FileWindowStatus::CONTINUE;
    // draw all visible windows
    while (status == FileWindowStatus::CONTINUE)
    {
        if (driver_key_pressed())
        {
            driver_get_key();
            break;
        }
        FractalInfo read_info;
        ExtBlock2 blk_2_info;
        ExtBlock3 blk_3_info;
        ExtBlock4 blk_4_info;
        ExtBlock5 blk_5_info;
        ExtBlock6 blk_6_info;
        ExtBlock7 blk_7_info;
        if (!find_fractal_info(path.string(), &read_info,                                     //
                &blk_2_info, &blk_3_info, &blk_4_info, &blk_5_info, &blk_6_info, &blk_7_info) //
            && (type_ok(&read_info, &blk_3_info) || !g_browse_check_fractal_type)             //
            && (params_ok(&read_info) || !g_browse_check_fractal_params)                      //
            && g_browse_name != path.string()                                                 //
            && !blk_6_info.got_data                                                           //
            && window.is_visible(&read_info, &blk_5_info))                                    //
        {
            window.path = path;
            window.filename = path.filename().string();
            window.draw(color_of_box);
            window.box_count = g_box_count;
            s_browse_windows.push_back(window);
            save_box(num_dots, win_count);
            win_count++;
            assert(static_cast<size_t>(win_count) == s_browse_windows.size());
        }
        path = id::io::find_wildcard_next();
        status = path.empty() || win_count >= MAX_WINDOWS_OPEN ? FileWindowStatus::EXIT : FileWindowStatus::CONTINUE;
    }

    if (win_count >= MAX_WINDOWS_OPEN)
    {
        // hard code message at MAX_WINDOWS_OPEN = 450
        text_temp_msg("Sorry...no more space, 450 displayed.");
    }
    if (vid_too_big)
    {
        text_temp_msg("Xdots + Ydots > 4096.");
    }
    c = 0;
    if (win_count)
    {
        driver_buzzer(Buzzer::COMPLETE); //let user know we've finished
        int index = 0;
        status = FileWindowStatus::CONTINUE;
        window = s_browse_windows[index];
        restore_box(num_dots, index);
        show_temp_msg(window.filename);
        while (status == FileWindowStatus::CONTINUE)
        {
            char msg[40];
            while (!driver_key_pressed())
            {
                std::time(&this_time);
                if (static_cast<double>(this_time - last_time) > 0.2)
                {
                    last_time = this_time;
                    toggle = !toggle;
                }
                if (toggle)
                {
                    window.draw(g_color_bright);   // flash current window
                }
                else
                {
                    window.draw(g_color_dark);
                }
            }

            c = driver_get_key();
            switch (c)
            {
            case ID_KEY_RIGHT_ARROW:
            case ID_KEY_LEFT_ARROW:
            case ID_KEY_DOWN_ARROW:
            case ID_KEY_UP_ARROW:
                clear_temp_msg();
                window.draw(color_of_box);// dim last window
                if (c == ID_KEY_RIGHT_ARROW || c == ID_KEY_UP_ARROW)
                {
                    index++;                     // shift attention to next window
                    if (index >= win_count)
                    {
                        index = 0;
                    }
                }
                else
                {
                    index -- ;
                    if (index < 0)
                    {
                        index = win_count -1 ;
                    }
                }
                window = s_browse_windows[index];
                restore_box(num_dots, index);
                show_temp_msg(window.filename);
                break;
            case ID_KEY_CTL_INSERT:
                color_of_box += key_count(ID_KEY_CTL_INSERT);
                for (FileWindow &s_browse_window : s_browse_windows)
                {
                    s_browse_window.draw(color_of_box);
                }
                window = s_browse_windows[index];
                window.draw(color_of_box);
                break;

            case ID_KEY_CTL_DEL:
                color_of_box -= key_count(ID_KEY_CTL_DEL);
                for (int i = 0; i < win_count ; i++)
                {
                    s_browse_windows[i].draw(color_of_box);
                }
                window = s_browse_windows[index];
                window.draw(color_of_box);
                break;
            case ID_KEY_ENTER:
            case ID_KEY_ENTER_2:   // this file please
                g_browse_name = window.filename;
                status = FileWindowStatus::EXIT;
                break;

            case ID_KEY_ESC:
            case 'l':
            case 'L':
                g_auto_browse = false;
                status = FileWindowStatus::ERASE_BOXES_EXIT;
                break;

            case 'D': // delete file
                clear_temp_msg();
                show_temp_msg(fmt::format("Delete {:s}? (Y/N)", window.filename));
                driver_wait_key_pressed(false);
                clear_temp_msg();
                c = driver_get_key();
                if (c == 'Y' && g_confirm_file_deletes)
                {
                    text_temp_msg("ARE YOU SURE???? (Y/N)");
                    if (driver_get_key() != 'Y')
                    {
                        c = 'N';
                    }
                }
                if (c == 'Y')
                {
                    std::error_code ec;
                    if (std::filesystem::remove(window.path, ec))
                    {
                        // do a rescan
                        status = FileWindowStatus::RESCAN;
                        check_history(window.filename.c_str(), "");
                        break;
                    }
                    if (ec.value() == EACCES)
                    {
                        text_temp_msg("Sorry...it's a read only file, can't del");
                        show_temp_msg(window.filename);
                        break;
                    }
                }
                {
                    text_temp_msg("file not deleted (phew!)");
                }
                show_temp_msg(window.filename);
                break;

            case 'R':
            {
                clear_temp_msg();
                driver_stack_screen();
                char new_name[60];
                new_name[0] = 0;
                std::strcpy(msg, "Enter the new filename for ");
                std::strcpy(new_name, window.filename.c_str());
                int i = field_prompt(msg, nullptr, new_name, 60, nullptr);
                driver_unstack_screen();
                if (i != -1)
                {
                    std::error_code ec;
                    std::string new_filename{std::filesystem::path{new_name}.filename().string()};
                    std::filesystem::path new_path{window.path.parent_path() / new_filename};
                    std::filesystem::rename(window.path, new_path, ec);
                    if (ec)
                    {
                        if (ec.value() == EACCES)
                        {
                            text_temp_msg("Sorry....can't rename");
                        }
                    }
                    else
                    {
                        check_history(window.filename.c_str(), new_filename.c_str());
                        window.path = new_path;
                        window.filename = new_filename;
                    }
                }
                s_browse_windows[index] = window;
                show_temp_msg(window.filename);
                break;
            }

            case ID_KEY_CTL_B:
                clear_temp_msg();
                driver_stack_screen();
                status = static_cast<FileWindowStatus>(std::abs(get_browse_params()));
                driver_unstack_screen();
                show_temp_msg(window.filename);
                break;

            case 's': // save image with boxes
                g_auto_browse = false;
                window.draw(color_of_box); // current window white
                status = FileWindowStatus::SAVE_BOXES_EXIT;
                break;

            case '\\': //back out to last image
                status = FileWindowStatus::ERASE_BOXES_EXIT;
                break;

            default:
                break;
            } //switch
        } //while

        // now clean up memory (and the screen if necessary)
        clear_temp_msg();
        if (status >= FileWindowStatus::EXIT && status < FileWindowStatus::SAVE_BOXES_EXIT)
        {
            for (int i = win_count-1; i >= 0; i--)
            {
                window = s_browse_windows[i];
                g_box_count = window.box_count;
                restore_box(num_dots, i);
                if (g_box_count > 0)
                {
                    clear_box();
                }
            }
        }
        if (status == FileWindowStatus::RESCAN)
        {
            goto rescan; // hey everybody I just used the g word!
        }
    }//if
    else
    {
        driver_buzzer(Buzzer::INTERRUPT); //no suitable files in directory!
        text_temp_msg("Sorry... I can't find anything");
        g_browse_sub_images = false;
    }

    s_browse_windows.clear();
    s_browse_box_x.clear();
    s_browse_box_y.clear();
    s_browse_box_values.clear();
    restore_stack(saved);
    if (s_old_bf_math == BFMathType::NONE)
    {
        free_bf_vars();
    }
    g_bf_math = s_old_bf_math;

    return c;
}

void FileWindow::draw(int color)
{
    g_box_color = color;
    g_box_count = 0;
    if (win_size >= g_smallest_box_size_shown)
    {
        // big enough on screen to show up as a box so draw it
        // corner pixels
        add_box(top_left);
        add_box(top_right);
        add_box(bot_left);
        add_box(bot_right);
        draw_lines(
            top_left, top_right, bot_left.x - top_left.x, bot_left.y - top_left.y);  // top & bottom lines
        draw_lines(
            top_left, bot_left, top_right.x - top_left.x, top_right.y - top_left.y); // left & right lines
        display_box();
    }
    else
    {
        Coord ibl;
        Coord itr;
        // draw crosshairs
        int cross_size = g_logical_screen_y_dots / 45;
        cross_size = std::max(cross_size, 2);
        itr.x = top_left.x - cross_size;
        itr.y = top_left.y;
        ibl.y = top_left.y - cross_size;
        ibl.x = top_left.x;
        draw_lines(top_left, itr, ibl.x - itr.x, 0); // top & bottom lines
        draw_lines(top_left, ibl, 0, itr.y - ibl.y); // left & right lines
        display_box();
    }
}

bool FileWindow::is_visible(const FractalInfo *info, const ExtBlock5 *blk_5_info)
{
    DblCoords tl;
    DblCoords tr;
    DblCoords bl;
    DblCoords br;
    double too_big = std::sqrt(sqr((double) g_screen_x_dots) + sqr((double) g_screen_y_dots)) * 1.5;
    // arbitrary value... stops browser zooming out too far
    int corner_count = 0;
    bool cant_see = false;

    int saved = save_stack();
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    /*
       if (oldbf_math && info->bf_math && (g_bn_length+4 < info->g_bf_length)) {
          g_bn_length = info->g_bf_length;
          calc_lengths();
       }
    */
    int two_len = g_bf_length + 2;
    BigFloat bt_x = alloc_stack(two_len);
    BigFloat bt_y = alloc_stack(two_len);
    BigFloat bt_x_min = alloc_stack(two_len);
    BigFloat bt_x_max = alloc_stack(two_len);
    BigFloat bt_y_min = alloc_stack(two_len);
    BigFloat bt_y_max = alloc_stack(two_len);
    BigFloat bt_x_3rd = alloc_stack(two_len);
    BigFloat bt_y_3rd = alloc_stack(two_len);

    if (info->bf_math)
    {
        const int di_bf_length = info->bf_length + g_bn_step;
        const int two_di_len = di_bf_length + 2;
        const int two_r_bf = g_r_bf_length + 2;

        s_n_a     = alloc_stack(two_r_bf);
        s_n_b     = alloc_stack(two_r_bf);
        s_n_c     = alloc_stack(two_r_bf);
        s_n_d     = alloc_stack(two_r_bf);
        s_n_e     = alloc_stack(two_r_bf);
        s_n_f     = alloc_stack(two_r_bf);

        convert_bf(s_n_a, s_bt_a, g_r_bf_length, orig_r_bf_length);
        convert_bf(s_n_b, s_bt_b, g_r_bf_length, orig_r_bf_length);
        convert_bf(s_n_c, s_bt_c, g_r_bf_length, orig_r_bf_length);
        convert_bf(s_n_d, s_bt_d, g_r_bf_length, orig_r_bf_length);
        convert_bf(s_n_e, s_bt_e, g_r_bf_length, orig_r_bf_length);
        convert_bf(s_n_f, s_bt_f, g_r_bf_length, orig_r_bf_length);

        BigFloat bt_t1 = alloc_stack(two_di_len);
        BigFloat bt_t2 = alloc_stack(two_di_len);
        BigFloat bt_t3 = alloc_stack(two_di_len);
        BigFloat bt_t4 = alloc_stack(two_di_len);
        BigFloat bt_t5 = alloc_stack(two_di_len);
        BigFloat bt_t6 = alloc_stack(two_di_len);

        std::memcpy(bt_t1, blk_5_info->apm_data.data(), two_di_len);
        std::memcpy(bt_t2, &blk_5_info->apm_data[two_di_len], two_di_len);
        std::memcpy(bt_t3, &blk_5_info->apm_data[2*two_di_len], two_di_len);
        std::memcpy(bt_t4, &blk_5_info->apm_data[3*two_di_len], two_di_len);
        std::memcpy(bt_t5, &blk_5_info->apm_data[4*two_di_len], two_di_len);
        std::memcpy(bt_t6, &blk_5_info->apm_data[5*two_di_len], two_di_len);

        convert_bf(bt_x_min, bt_t1, two_len, two_di_len);
        convert_bf(bt_x_max, bt_t2, two_len, two_di_len);
        convert_bf(bt_y_min, bt_t3, two_len, two_di_len);
        convert_bf(bt_y_max, bt_t4, two_len, two_di_len);
        convert_bf(bt_x_3rd, bt_t5, two_len, two_di_len);
        convert_bf(bt_y_3rd, bt_t6, two_len, two_di_len);
    }

    /* transform maps real plane co-ords onto the current screen view see above */
    if (s_old_bf_math != BFMathType::NONE || info->bf_math != 0)
    {
        if (!info->bf_math)
        {
            float_to_bf(bt_x, info->x_min);
            float_to_bf(bt_y, info->y_max);
        }
        else
        {
            copy_bf(bt_x, bt_x_min);
            copy_bf(bt_y, bt_y_max);
        }
        bf_transform(bt_x, bt_y, &tl);
    }
    else
    {
        tl.x = info->x_min;
        tl.y = info->y_max;
        transform(&tl);
    }
    top_left.x = (int) std::lround(tl.x);
    top_left.y = (int) std::lround(tl.y);
    if (s_old_bf_math != BFMathType::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            float_to_bf(bt_x, (info->x_max)-(info->x3rd-info->x_min));
            float_to_bf(bt_y, (info->y_max)+(info->y_min-info->y3rd));
        }
        else
        {
            neg_a_bf(sub_bf(bt_x, bt_x_3rd, bt_x_min));
            add_a_bf(bt_x, bt_x_max);
            sub_bf(bt_y, bt_y_min, bt_y_3rd);
            add_a_bf(bt_y, bt_y_max);
        }
        bf_transform(bt_x, bt_y, &tr);
    }
    else
    {
        tr.x = (info->x_max)-(info->x3rd-info->x_min);
        tr.y = (info->y_max)+(info->y_min-info->y3rd);
        transform(&tr);
    }
    top_right.x = (int) std::lround(tr.x);
    top_right.y = (int) std::lround(tr.y);
    if (s_old_bf_math != BFMathType::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            float_to_bf(bt_x, info->x3rd);
            float_to_bf(bt_y, info->y3rd);
        }
        else
        {
            copy_bf(bt_x, bt_x_3rd);
            copy_bf(bt_y, bt_y_3rd);
        }
        bf_transform(bt_x, bt_y, &bl);
    }
    else
    {
        bl.x = info->x3rd;
        bl.y = info->y3rd;
        transform(&bl);
    }
    bot_left.x = (int) std::lround(bl.x);
    bot_left.y = (int) std::lround(bl.y);
    if (s_old_bf_math != BFMathType::NONE || info->bf_math)
    {
        if (!info->bf_math)
        {
            float_to_bf(bt_x, info->x_max);
            float_to_bf(bt_y, info->y_min);
        }
        else
        {
            copy_bf(bt_x, bt_x_max);
            copy_bf(bt_y, bt_y_min);
        }
        bf_transform(bt_x, bt_y, &br);
    }
    else
    {
        br.x = info->x_max;
        br.y = info->y_min;
        transform(&br);
    }
    bot_right.x = (int) std::lround(br.x);
    bot_right.y = (int) std::lround(br.y);

    double tmp_sqrt = std::sqrt(sqr(tr.x - bl.x) + sqr(tr.y - bl.y));
    win_size = tmp_sqrt; // used for box vs crosshair in drawindow()
    // reject anything too small or too big on screen
    if ((tmp_sqrt < g_smallest_window_display_size) || (tmp_sqrt > too_big))
    {
        cant_see = true;
    }

    // restore original values
    g_bf_length      = orig_bf_length;
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;

    restore_stack(saved);
    if (cant_see)   // do it this way so bignum stack is released
    {
        return false;
    }

    // now see how many corners are on the screen, accept if one or more
    if (tl.x >= (0-g_logical_screen_x_offset) && tl.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && tl.y >= (0-g_logical_screen_y_offset) && tl.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        corner_count++;
    }
    if (bl.x >= (0-g_logical_screen_x_offset) && bl.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && bl.y >= (0-g_logical_screen_y_offset) && bl.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        corner_count++;
    }
    if (tr.x >= (0-g_logical_screen_x_offset) && tr.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && tr.y >= (0-g_logical_screen_y_offset) && tr.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        corner_count++;
    }
    if (br.x >= (0-g_logical_screen_x_offset) && br.x <= (g_screen_x_dots-g_logical_screen_x_offset)
        && br.y >= (0-g_logical_screen_y_offset) && br.y <= (g_screen_y_dots-g_logical_screen_y_offset))
    {
        corner_count++;
    }

    return corner_count >= 1;
}

// maps points onto view screen
static void transform(DblCoords *point)
{
    double tmp_pt_x = s_cvt->a * point->x + s_cvt->b * point->y + s_cvt->e;
    point->y = s_cvt->c * point->x + s_cvt->d * point->y + s_cvt->f;
    point->x = tmp_pt_x;
}

static bool params_ok(const FractalInfo *info)
{
    double tmp_param3;
    double tmp_param4;
    double tmp_param5;
    double tmp_param6;
    double tmp_param7;
    double tmp_param8;
    double tmp_param9;
    double tmp_param10;

    if (info->info_version > 6)
    {
        tmp_param3 = info->d_param3;
        tmp_param4 = info->d_param4;
    }
    else
    {
        tmp_param3 = info->param3;
        round_float_double(&tmp_param3);
        tmp_param4 = info->param4;
        round_float_double(&tmp_param4);
    }
    if (info->info_version > 8)
    {
        tmp_param5 = info->d_param5;
        tmp_param6 = info->d_param6;
        tmp_param7 = info->d_param7;
        tmp_param8 = info->d_param8;
        tmp_param9 = info->d_param9;
        tmp_param10 = info->d_param10;
    }
    else
    {
        tmp_param5 = 0.0;
        tmp_param6 = 0.0;
        tmp_param7 = 0.0;
        tmp_param8 = 0.0;
        tmp_param9 = 0.0;
        tmp_param10 = 0.0;
    }
    // parameters are in range?
    return std::abs(info->c_real - g_params[0]) < MIN_DIF //
        && std::abs(info->c_imag - g_params[1]) < MIN_DIF //
        && std::abs(tmp_param3 - g_params[2]) < MIN_DIF    //
        && std::abs(tmp_param4 - g_params[3]) < MIN_DIF    //
        && std::abs(tmp_param5 - g_params[4]) < MIN_DIF    //
        && std::abs(tmp_param6 - g_params[5]) < MIN_DIF    //
        && std::abs(tmp_param7 - g_params[6]) < MIN_DIF    //
        && std::abs(tmp_param8 - g_params[7]) < MIN_DIF    //
        && std::abs(tmp_param9 - g_params[8]) < MIN_DIF    //
        && std::abs(tmp_param10 - g_params[9]) < MIN_DIF   //
        && info->invert[0] - g_inversion[0] < MIN_DIF;
}

static bool function_ok(const FractalInfo *info, int num_fn)
{
    for (int i = 0; i < num_fn; i++)
    {
        if (info->trig_index[i] != +g_trig_index[i])
        {
            return false;
        }
    }
    return true; // they all match
}

static bool type_ok(const FractalInfo *info, const ExtBlock3 *blk_3_info)
{
    int num_fn;
    if (g_fractal_type == FractalType::FORMULA && migrate_integer_types(info->fractal_type) == FractalType::FORMULA)
    {
        if (id::string_case_equal(blk_3_info->form_name, g_formula_name.c_str()))
        {
            num_fn = g_max_function;
            if (num_fn > 0)
            {
                return function_ok(info, num_fn);
            }
            return true; // match up formula names with no functions
        }
        return false; // two formulas but names don't match
    }
    if (info->fractal_type == +g_fractal_type || g_fractal_type == migrate_integer_types(info->fractal_type))
    {
        num_fn = (+g_cur_fractal_specific->flags >> 6) & 7;
        if (num_fn > 0)
        {
            return function_ok(info, num_fn);
        }
        return true; // match types with no functions
    }
    return false; // no match
}

// The history stack needs to be adjusted if the rename or delete functions of the
// browser are used.
static void check_history(const char *old_name, const char *new_name)
{
    for (std::string &name : g_filename_stack)
    {
        if (name == old_name)
        {
            name = new_name;
        }
    }
}

static void bf_setup_convert_to_screen()
{
    // setup_convert_to_screen() in LORENZ.C, converted to g_bf_math
    // Call only from within fgetwindow()

    int saved = save_stack();
    BigFloat bt_inter1 = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_inter2 = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_det = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_xd = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_yd = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_tmp1 = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_tmp2 = alloc_stack(g_r_bf_length + 2);

    // xx3rd-xxmin
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_min);
    // yymin-yymax
    sub_bf(bt_inter2, g_bf_y_min, g_bf_y_max);
    // (xx3rd-xxmin)*(yymin-yymax)
    mult_bf(bt_tmp1, bt_inter1, bt_inter2);

    // yymax-yy3rd
    sub_bf(bt_inter1, g_bf_y_max, g_bf_y_3rd);
    // xxmax-xxmin
    sub_bf(bt_inter2, g_bf_x_max, g_bf_x_min);
    // (yymax-yy3rd)*(xxmax-xxmin)
    mult_bf(bt_tmp2, bt_inter1, bt_inter2);

    // det = (xx3rd-xxmin)*(yymin-yymax) + (yymax-yy3rd)*(xxmax-xxmin)
    add_bf(bt_det, bt_tmp1, bt_tmp2);

    // xd = x_size_d/det
    float_to_bf(bt_tmp1, g_logical_screen_x_size_dots);
    div_bf(bt_xd, bt_tmp1, bt_det);

    // a =  xd*(yymax-yy3rd)
    sub_bf(bt_inter1, g_bf_y_max, g_bf_y_3rd);
    mult_bf(s_bt_a, bt_xd, bt_inter1);

    // b =  xd*(xx3rd-xxmin)
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_min);
    mult_bf(s_bt_b, bt_xd, bt_inter1);

    // e = -(a*xxmin + b*yymax)
    mult_bf(bt_tmp1, s_bt_a, g_bf_x_min);
    mult_bf(bt_tmp2, s_bt_b, g_bf_y_max);
    neg_a_bf(add_bf(s_bt_e, bt_tmp1, bt_tmp2));

    // xx3rd-xxmax
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_max);
    // yymin-yymax
    sub_bf(bt_inter2, g_bf_y_min, g_bf_y_max);
    // (xx3rd-xxmax)*(yymin-yymax)
    mult_bf(bt_tmp1, bt_inter1, bt_inter2);

    // yymin-yy3rd
    sub_bf(bt_inter1, g_bf_y_min, g_bf_y_3rd);
    // xxmax-xxmin
    sub_bf(bt_inter2, g_bf_x_max, g_bf_x_min);
    // (yymin-yy3rd)*(xxmax-xxmin)
    mult_bf(bt_tmp2, bt_inter1, bt_inter2);

    // det = (xx3rd-xxmax)*(yymin-yymax) + (yymin-yy3rd)*(xxmax-xxmin)
    add_bf(bt_det, bt_tmp1, bt_tmp2);

    // yd = y_size_d/det
    float_to_bf(bt_tmp2, g_logical_screen_y_size_dots);
    div_bf(bt_yd, bt_tmp2, bt_det);

    // c =  yd*(yymin-yy3rd)
    sub_bf(bt_inter1, g_bf_y_min, g_bf_y_3rd);
    mult_bf(s_bt_c, bt_yd, bt_inter1);

    // d =  yd*(xx3rd-xxmax)
    sub_bf(bt_inter1, g_bf_x_3rd, g_bf_x_max);
    mult_bf(s_bt_d, bt_yd, bt_inter1);

    // f = -(c*xxmin + d*yymax)
    mult_bf(bt_tmp1, s_bt_c, g_bf_x_min);
    mult_bf(bt_tmp2, s_bt_d, g_bf_y_max);
    neg_a_bf(add_bf(s_bt_f, bt_tmp1, bt_tmp2));

    restore_stack(saved);
}

// maps points onto view screen
static void bf_transform(BigFloat bt_x, BigFloat bt_y, DblCoords *point)
{
    int saved = save_stack();
    BigFloat bt_tmp1 = alloc_stack(g_r_bf_length + 2);
    BigFloat bt_tmp2 = alloc_stack(g_r_bf_length + 2);

    //  point->x = cvt->a * point->x + cvt->b * point->y + cvt->e;
    mult_bf(bt_tmp1, s_n_a, bt_x);
    mult_bf(bt_tmp2, s_n_b, bt_y);
    add_a_bf(bt_tmp1, bt_tmp2);
    add_a_bf(bt_tmp1, s_n_e);
    point->x = (double)bf_to_float(bt_tmp1);

    //  point->y = cvt->c * point->x + cvt->d * point->y + cvt->f;
    mult_bf(bt_tmp1, s_n_c, bt_x);
    mult_bf(bt_tmp2, s_n_d, bt_y);
    add_a_bf(bt_tmp1, bt_tmp2);
    add_a_bf(bt_tmp1, s_n_f);
    point->y = (double)bf_to_float(bt_tmp1);

    restore_stack(saved);
}
