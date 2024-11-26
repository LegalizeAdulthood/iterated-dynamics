// SPDX-License-Identifier: GPL-3.0-only
//
/*
 * Generates Inverse Julia in real time, lets move a cursor which determines
 * the J-set.
 *
 *  The J-set is generated in a fixed-size window, a third of the screen.
 */
#include "port.h"
#include "prototyp.h"

#include "jiim.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "find_special_colors.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "frothy_basin.h"
#include "get_a_number.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "lorenz.h"
#include "miscfrac.h"
#include "mouse.h"
#include "os.h"
#include "pixel_grid.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "value_saver.h"
#include "video.h"
#include "zoom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

enum
{
    MAXRECT = 1024      // largest width of SaveRect/RestoreRect
};

enum class JuliaWindowStyle
{
    LARGE = 0,       // full screen or large view window
    NO_OVERLAP = 1,  // Julia/orbit and fractal don't overlap
    FULL_SCREEN = 2, // Julia/orbit takes whole screen
    HIDDEN = 3,
};

static int s_show_numbers{};              // toggle for display of coords
static std::vector<char> s_screen_rect;   //
static JuliaWindowStyle s_window_style{}; // windows management system
static int s_corner_x{};                  // corners of the window
static int s_corner_y{};                  //
static int s_win_width{};                 // dots in the window
static int s_win_height{};                //
static int s_x_base{};                    // circle routines from Dr. Dobbs June 1990
static int s_y_base{};                    //
static unsigned int s_x_aspect{};         //
static unsigned int s_y_aspect{};         //
static long s_list_front{};               // head, tail, size of MIIM Queue
static long s_list_back{};                //
static long s_list_size{};                //
static long s_l_size{};                   // how many in queue (now, ever)
static long s_l_max{};                    //
static int s_max_hits{1};                 //
static bool s_ok_to_miim{};               //
static int s_secret_experimental_mode{};  //
static float s_lucky_x{};                 //
static float s_lucky_y{};                 //
static CrossHairCursor s_cursor;          //

double g_julia_c_x{JULIA_C_NOT_SET}; //
double g_julia_c_y{JULIA_C_NOT_SET}; //
DComplex g_save_c{-3000.0, -3000.0}; //

void SetAspect(double aspect)
{
    s_x_aspect = 0;
    s_y_aspect = 0;
    aspect = std::fabs(aspect);
    if (aspect != 1.0)
    {
        if (aspect > 1.0)
        {
            s_y_aspect = (unsigned int)(65536.0 / aspect);
        }
        else
        {
            s_x_aspect = (unsigned int)(65536.0 * aspect);
        }
    }
}

void c_putcolor(int x, int y, int color)
{
    // avoid writing outside window
    if (x < s_corner_x || y < s_corner_y || x >= s_corner_x + s_win_width || y >= s_corner_y + s_win_height)
    {
        return ;
    }
    if (y >= g_screen_y_dots - s_show_numbers)   // avoid overwriting coords
    {
        return;
    }
    if (s_window_style == JuliaWindowStyle::FULL_SCREEN)   // avoid overwriting fractal
    {
        if (0 <= x && x < g_logical_screen_x_dots && 0 <= y && y < g_logical_screen_y_dots)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

int  c_getcolor(int x, int y)
{
    // avoid reading outside window
    if (x < s_corner_x || y < s_corner_y || x >= s_corner_x + s_win_width || y >= s_corner_y + s_win_height)
    {
        return 1000;
    }
    if (y >= g_screen_y_dots - s_show_numbers)   // avoid overreading coords
    {
        return 1000;
    }
    if (s_window_style == JuliaWindowStyle::FULL_SCREEN)   // avoid overreading fractal
    {
        if (0 <= x && x < g_logical_screen_x_dots && 0 <= y && y < g_logical_screen_y_dots)
        {
            return 1000;
        }
    }
    return getcolor(x, y);
}

void circleplot(int x, int y, int color)
{
    if (s_x_aspect == 0)
    {
        if (s_y_aspect == 0)
        {
            c_putcolor(x+s_x_base, y+s_y_base, color);
        }
        else
        {
            c_putcolor(x+s_x_base, (short)(s_y_base + (((long) y * (long) s_y_aspect) >> 16)), color);
        }
    }
    else
    {
        c_putcolor((int)(s_x_base + (((long) x * (long) s_x_aspect) >> 16)), y+s_y_base, color);
    }
}

void plot8(int x, int y, int color)
{
    circleplot(x, y, color);
    circleplot(-x, y, color);
    circleplot(x, -y, color);
    circleplot(-x, -y, color);
    circleplot(y, x, color);
    circleplot(-y, x, color);
    circleplot(y, -x, color);
    circleplot(-y, -x, color);
}

void circle(int radius, int color)
{
    int x;
    int y;
    int sum;

    x = 0;
    y = radius << 1;
    sum = 0;

    while (x <= y)
    {
        if (!(x & 1))       // plot if x is even
        {
            plot8(x >> 1, (y+1) >> 1, color);
        }
        sum += (x << 1) + 1;
        x++;
        if (sum > 0)
        {
            sum -= (y << 1) - 1;
            y--;
        }
    }
}

/*
 * MIIM section:
 *
 * Global variables and service functions used for computing
 * MIIM Julias will be grouped here (and shared by code in LORENZ.C)
 *
 */
static void fill_rect(int x, int y, int width, int depth, int color)
{
    // fast version of fillrect
    if (!g_has_inverse)
    {
        return;
    }
    std::vector<char> row(width, char(color % g_colors));
    while (depth-- > 0)
    {
        if (driver_key_pressed())   // we could do this less often when in fast modes
        {
            return;
        }
        put_row(x, y++, width, &row[0]);
    }
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int QueueEmpty()            // True if NO points remain in queue
{
    return s_list_front == s_list_back;
}

int QueueFullAlmost()       // True if room for ONE more point in queue
{
    return ((s_list_front + 2) % s_list_size) == s_list_back;
}

void ClearQueue()
{
    s_l_max = 0;
    s_l_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

/*
 * Queue functions for MIIM julia:
 * move to JIIM.C when done
 */

bool Init_Queue(unsigned long request)
{
    if (driver_diskp())
    {
        stopmsg("Don't try this in disk video mode, kids...\n");
        s_list_size = 0;
        return false;
    }

    for (s_list_size = request; s_list_size > 1024; s_list_size /= 2)
    {
        switch (common_start_disk(s_list_size * 8, 1, 256))
        {
        case 0:                        // success
            s_list_back = 0;
            s_list_front = 0;
            s_l_max = 0;
            s_l_size = 0;
            return true;
        case -1:
            continue;                   // try smaller queue size
        case -2:
            s_list_size = 0;               // cancelled by user
            return false;
        }
    }

    // failed to get memory for MIIM Queue
    s_list_size = 0;
    return false;
}

void Free_Queue()
{
    end_disk();
    s_l_max = 0;
    s_l_size = 0;
    s_list_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

int PushLong(long x, long y)
{
    if (((s_list_front + 1) % s_list_size) != s_list_back)
    {
        if (to_mem_disk(8*s_list_front, sizeof(x), &x)
            && to_mem_disk(8*s_list_front +sizeof(x), sizeof(y), &y))
        {
            s_list_front = (s_list_front + 1) % s_list_size;
            if (++s_l_size > s_l_max)
            {
                s_l_max   = s_l_size;
                s_lucky_x = (float)x;
                s_lucky_y = (float)y;
            }
            return 1;
        }
    }
    return 0;                    // fail
}

int PushFloat(float x, float y)
{
    if (((s_list_front + 1) % s_list_size) != s_list_back)
    {
        if (to_mem_disk(8*s_list_front, sizeof(x), &x)
            && to_mem_disk(8*s_list_front +sizeof(x), sizeof(y), &y))
        {
            s_list_front = (s_list_front + 1) % s_list_size;
            if (++s_l_size > s_l_max)
            {
                s_l_max   = s_l_size;
                s_lucky_x = x;
                s_lucky_y = y;
            }
            return 1;
        }
    }
    return 0;                    // fail
}

DComplex PopFloat()
{
    DComplex pop;
    float popx;
    float popy;

    if (!QueueEmpty())
    {
        s_list_front--;
        if (s_list_front < 0)
        {
            s_list_front = s_list_size - 1;
        }
        if (from_mem_disk(8*s_list_front, sizeof(popx), &popx)
            && from_mem_disk(8*s_list_front +sizeof(popx), sizeof(popy), &popy))
        {
            pop.x = popx;
            pop.y = popy;
            --s_l_size;
        }
        return pop;
    }
    pop.x = 0;
    pop.y = 0;
    return pop;
}

LComplex PopLong()
{
    LComplex pop;

    if (!QueueEmpty())
    {
        s_list_front--;
        if (s_list_front < 0)
        {
            s_list_front = s_list_size - 1;
        }
        if (from_mem_disk(8*s_list_front, sizeof(pop.x), &pop.x)
            && from_mem_disk(8*s_list_front +sizeof(pop.x), sizeof(pop.y), &pop.y))
        {
            --s_l_size;
        }
        return pop;
    }
    pop.x = 0;
    pop.y = 0;
    return pop;
}

int EnQueueFloat(float x, float y)
{
    return PushFloat(x, y);
}

int EnQueueLong(long x, long y)
{
    return PushLong(x, y);
}

DComplex DeQueueFloat()
{
    DComplex out;
    float outx;
    float outy;

    if (s_list_back != s_list_front)
    {
        if (from_mem_disk(8*s_list_back, sizeof(outx), &outx)
            && from_mem_disk(8*s_list_back +sizeof(outx), sizeof(outy), &outy))
        {
            s_list_back = (s_list_back + 1) % s_list_size;
            out.x = outx;
            out.y = outy;
            s_l_size--;
        }
        return out;
    }
    out.x = 0;
    out.y = 0;
    return out;
}

LComplex DeQueueLong()
{
    LComplex out;
    out.x = 0;
    out.y = 0;

    if (s_list_back != s_list_front)
    {
        if (from_mem_disk(8*s_list_back, sizeof(out.x), &out.x)
            && from_mem_disk(8*s_list_back +sizeof(out.x), sizeof(out.y), &out.y))
        {
            s_list_back = (s_list_back + 1) % s_list_size;
            s_l_size--;
        }
        return out;
    }
    out.x = 0;
    out.y = 0;
    return out;
}


/*
 * End MIIM section;
 */

static void SaveRect(int x, int y, int width, int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    s_screen_rect.clear();
    std::vector<char> const background(width, char(g_color_dark));
    s_screen_rect.resize(width*depth);
    s_cursor.hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        get_row(x, y+yoff, width, &s_screen_rect[width*yoff]);
        put_row(x, y+yoff, width, &background[0]);
    }
    s_cursor.show();
}

static void RestoreRect(int x, int y, int width, int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    s_cursor.hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        put_row(x, y+yoff, width, &s_screen_rect[width*yoff]);
    }
    s_cursor.show();
}

void Jiim(jiim_types which)
{
    affine cvt;
    bool exact = false;
    int count = 0;            // coloring julia
    static int mode = 0;      // point, circle, ...
    double cr;
    double ci;
    double r;
    int xfactor;
    int yfactor; // aspect ratio

    int xoff;
    int yoff; // center of the window
    int x;
    int y;
    int kbdchar = -1;

    long iter;
    int color;
    float zoom;
    int old_x;
    int old_y;
    double aspect;
    static int randir = 0;
    static int rancnt = 0;
    bool actively_computing = true;
    bool first_time = true;

    const ValueSaver saved_debug_flag{g_debug_flag};
    // must use standard fractal or be calcfroth
    if (g_fractal_specific[+g_fractal_type].calctype != standard_fractal
        && g_fractal_specific[+g_fractal_type].calctype != calcfroth)
    {
        return;
    }
    const ValueSaver saved_help_mode{
        g_help_mode, which == jiim_types::JIIM ? help_labels::HELP_JIIM : help_labels::HELP_ORBITS};
    if (which == jiim_types::ORBIT)
    {
        g_has_inverse = true;
    }
    const int oldsxoffs{g_logical_screen_x_offset};
    const int oldsyoffs{g_logical_screen_y_offset};
    const ValueSaver saved_calc_type{g_calc_type};
    s_show_numbers = 0;
    g_using_jiim = true;
    g_line_buff.resize(std::max(g_screen_x_dots, g_screen_y_dots));
    aspect = ((double)g_logical_screen_x_dots*3)/((double)g_logical_screen_y_dots*4);  // assumes 4:3
    actively_computing = true;
    SetAspect(aspect);
    ValueSaver saved_look_at_mouse{g_look_at_mouse, +MouseLook::POSITION};

    if (which == jiim_types::ORBIT)
    {
        per_image();
    }

    s_cursor = CrossHairCursor();

    /*
     * MIIM code:
     * Grab memory for Queue/Stack before SaveRect gets it.
     */
    s_ok_to_miim  = false;
    if (which == jiim_types::JIIM && g_debug_flag != debug_flags::prevent_miim)
    {
        s_ok_to_miim = Init_Queue(8*1024UL); // Queue Set-up Successful?
    }

    s_max_hits = 1;
    if (which == jiim_types::ORBIT)
    {
        g_plot = c_putcolor;                // for line with clipping
    }

    /*
     * end MIIM code.
     */

    g_vesa_x_res = g_screen_x_dots;
    g_vesa_y_res = g_screen_y_dots;

    if (g_logical_screen_x_offset != 0 || g_logical_screen_y_offset != 0) // we're in view windows
    {
        ValueSaver saved_has_inverse(g_has_inverse, true);
        SaveRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        g_logical_screen_x_offset = g_video_start_x;
        g_logical_screen_y_offset = g_video_start_y;
        RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
    }

    if (g_logical_screen_x_dots == g_vesa_x_res
        || g_logical_screen_y_dots == g_vesa_y_res
        || g_vesa_x_res-g_logical_screen_x_dots < g_vesa_x_res/3
        || g_vesa_y_res-g_logical_screen_y_dots < g_vesa_y_res/3
        || g_logical_screen_x_dots >= MAXRECT)
    {
        /* this mode puts orbit/julia in an overlapping window 1/3 the size of
           the physical screen */
        s_window_style = JuliaWindowStyle::LARGE;
        s_win_width = g_vesa_x_res / 3;
        s_win_height = g_vesa_y_res / 3;
        s_corner_x = g_video_start_x + s_win_width * 2;
        s_corner_y = g_video_start_y + s_win_height * 2;
        xoff = g_video_start_x + s_win_width * 5 / 2;
        yoff = g_video_start_y + s_win_height * 5 / 2;
    }
    else if (g_logical_screen_x_dots > g_vesa_x_res/3 && g_logical_screen_y_dots > g_vesa_y_res/3)
    {
        s_window_style = JuliaWindowStyle::NO_OVERLAP;
        s_win_width = g_vesa_x_res - g_logical_screen_x_dots;
        s_win_height = g_vesa_y_res - g_logical_screen_y_dots;
        s_corner_x = g_video_start_x + g_logical_screen_x_dots;
        s_corner_y = g_video_start_y + g_logical_screen_y_dots;
        xoff = s_corner_x + s_win_width/2;
        yoff = s_corner_y + s_win_height/2;
    }
    else
    {
        s_window_style = JuliaWindowStyle::FULL_SCREEN;
        s_win_width = g_vesa_x_res;
        s_win_height = g_vesa_y_res;
        s_corner_x = g_video_start_x;
        s_corner_y = g_video_start_y;
        xoff = g_video_start_x + s_win_width/2;
        yoff = g_video_start_y + s_win_height/2;
    }

    xfactor = (int)(s_win_width/5.33);
    yfactor = (int)(-s_win_height/4);

    if (s_window_style == JuliaWindowStyle::LARGE)
    {
        SaveRect(s_corner_x, s_corner_y, s_win_width, s_win_height);
    }
    else if (s_window_style == JuliaWindowStyle::FULL_SCREEN)    // leave the fractal
    {
        fill_rect(g_logical_screen_x_dots, s_corner_y, s_win_width-g_logical_screen_x_dots, s_win_height, g_color_dark);
        fill_rect(s_corner_x   , g_logical_screen_y_dots, g_logical_screen_x_dots, s_win_height-g_logical_screen_y_dots, g_color_dark);
    }
    else    // blank whole window
    {
        fill_rect(s_corner_x, s_corner_y, s_win_width, s_win_height, g_color_dark);
    }

    setup_convert_to_screen(&cvt);

    // reuse last location if inside window
    g_col = (int)(cvt.a*g_save_c.x + cvt.b*g_save_c.y + cvt.e + .5);
    g_row = (int)(cvt.c*g_save_c.x + cvt.d*g_save_c.y + cvt.f + .5);
    if (g_col < 0 || g_col >= g_logical_screen_x_dots
        || g_row < 0 || g_row >= g_logical_screen_y_dots)
    {
        cr = (g_x_max + g_x_min) / 2.0;
        ci = (g_y_max + g_y_min) / 2.0;
    }
    else
    {
        cr = g_save_c.x;
        ci = g_save_c.y;
    }

    old_y = -1;
    old_x = -1;

    g_col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
    g_row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);

    // possible extraseg arrays have been trashed, so set up again
    if (g_integer_fractal)
    {
        fill_lx_array();
    }
    else
    {
        fill_dx_array();
    }

    s_cursor.set_pos(g_col, g_row);
    s_cursor.show();
    color = g_color_bright;

    iter = 1;
    bool still = true;
    zoom = 1.0f;

    g_cursor_mouse_tracking = true;

    while (still)
    {
        if (actively_computing)
        {
            s_cursor.check_blink();
        }
        else
        {
            s_cursor.wait_key();
        }
        if (driver_key_pressed() || first_time) // prevent burning up UNIX CPU
        {
            first_time = false;
            while (driver_key_pressed())
            {
                s_cursor.wait_key();
                kbdchar = driver_get_key();

                int dcol = 0;
                int drow = 0;
                g_julia_c_x = JULIA_C_NOT_SET;
                g_julia_c_y = JULIA_C_NOT_SET;
                switch (kbdchar)
                {
                case ID_KEY_CTL_KEYPAD_5:      // ctrl - keypad 5
                case ID_KEY_KEYPAD_5:          // keypad 5
                    break;                  // do nothing
                case ID_KEY_CTL_PAGE_UP:
                    dcol = 4;
                    drow = -4;
                    break;
                case ID_KEY_CTL_PAGE_DOWN:
                    dcol = 4;
                    drow = 4;
                    break;
                case ID_KEY_CTL_HOME:
                    dcol = -4;
                    drow = -4;
                    break;
                case ID_KEY_CTL_END:
                    dcol = -4;
                    drow = 4;
                    break;
                case ID_KEY_PAGE_UP:
                    dcol = 1;
                    drow = -1;
                    break;
                case ID_KEY_PAGE_DOWN:
                    dcol = 1;
                    drow = 1;
                    break;
                case ID_KEY_HOME:
                    dcol = -1;
                    drow = -1;
                    break;
                case ID_KEY_END:
                    dcol = -1;
                    drow = 1;
                    break;
                case ID_KEY_UP_ARROW:
                    drow = -1;
                    break;
                case ID_KEY_DOWN_ARROW:
                    drow = 1;
                    break;
                case ID_KEY_LEFT_ARROW:
                    dcol = -1;
                    break;
                case ID_KEY_RIGHT_ARROW:
                    dcol = 1;
                    break;
                case ID_KEY_CTL_UP_ARROW:
                    drow = -4;
                    break;
                case ID_KEY_CTL_DOWN_ARROW:
                    drow = 4;
                    break;
                case ID_KEY_CTL_LEFT_ARROW:
                    dcol = -4;
                    break;
                case ID_KEY_CTL_RIGHT_ARROW:
                    dcol = 4;
                    break;
                case 'z':
                case 'Z':
                    zoom = 1.0F;
                    break;
                case '<':
                case ',':
                    zoom /= 1.15F;
                    break;
                case '>':
                case '.':
                    zoom *= 1.15F;
                    break;
                case ID_KEY_SPACE:
                    g_julia_c_x = cr;
                    g_julia_c_y = ci;
                    goto finish;
                case 'c':   // circle toggle
                case 'C':   // circle toggle
                    mode = mode ^ 1;
                    break;
                case 'l':
                case 'L':
                    mode = mode ^ 2;
                    break;
                case 'n':
                case 'N':
                    s_show_numbers = 8 - s_show_numbers;
                    if (s_window_style == JuliaWindowStyle::LARGE && s_show_numbers == 0)
                    {
                        s_cursor.hide();
                        cleartempmsg();
                        s_cursor.show();
                    }
                    break;
                case 'p':
                case 'P':
                    get_a_number(&cr, &ci);
                    exact = true;
                    g_col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
                    g_row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);
                    drow = 0;
                    dcol = 0;
                    break;
                case 'h':   // hide fractal toggle
                case 'H':   // hide fractal toggle
                    if (s_window_style == JuliaWindowStyle::FULL_SCREEN)
                    {
                        s_window_style = JuliaWindowStyle::HIDDEN;
                    }
                    else if (s_window_style == JuliaWindowStyle::HIDDEN && s_win_width == g_vesa_x_res)
                    {
                        RestoreRect(g_video_start_x, g_video_start_y, g_logical_screen_x_dots, g_logical_screen_y_dots);
                        s_window_style = JuliaWindowStyle::FULL_SCREEN;
                    }
                    break;
#ifdef XFRACT
                case ID_KEY_ENTER:
                    break;
#endif
                case '0':
                case '1':
                case '2':
                    // don't use '3', it's already meaningful
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (which == jiim_types::JIIM)
                    {
                        s_secret_experimental_mode = kbdchar - '0';
                        break;
                    }
                default:
                    still = false;
                }  // switch
                if (kbdchar == 's' || kbdchar == 'S')
                {
                    goto finish;
                }
                if (dcol > 0 || drow > 0)
                {
                    exact = false;
                }
                g_col += dcol;
                g_row += drow;
#ifdef XFRACT
                if (kbdchar == ID_KEY_ENTER)
                {
                    // We want to use the position of the cursor
                    exact = false;
                    g_col = s_cursor.get_x();
                    g_row = s_cursor.get_y();
                }
#endif

                // keep cursor in logical screen
                if (g_col >= g_logical_screen_x_dots)
                {
                    g_col = g_logical_screen_x_dots -1;
                    exact = false;
                }
                if (g_row >= g_logical_screen_y_dots)
                {
                    g_row = g_logical_screen_y_dots -1;
                    exact = false;
                }
                if (g_col < 0)
                {
                    g_col = 0;
                    exact = false;
                }
                if (g_row < 0)
                {
                    g_row = 0;
                    exact = false;
                }

                s_cursor.set_pos(g_col, g_row);
            }  // end while (driver_key_pressed)

            if (!exact)
            {
                if (g_integer_fractal)
                {
                    cr = g_l_x_pixel();
                    ci = g_l_y_pixel();
                    cr /= (1L << g_bit_shift);
                    ci /= (1L << g_bit_shift);
                }
                else
                {
                    cr = g_dx_pixel();
                    ci = g_dy_pixel();
                }
            }
            actively_computing = true;
            if (s_show_numbers) // write coordinates on screen
            {
                char str[41];
                std::snprintf(str, std::size(str), "%16.14f %16.14f %3d", cr, ci, getcolor(g_col, g_row));
                if (s_window_style == JuliaWindowStyle::LARGE)
                {
                    /* show temp msg will clear self if new msg is a
                       different length - pad to length 40*/
                    while ((int)std::strlen(str) < 40)
                    {
                        std::strcat(str, " ");
                    }
                    str[40] = 0;
                    s_cursor.hide();
                    actively_computing = true;
                    showtempmsg(str);
                    s_cursor.show();
                }
                else
                {
                    driver_display_string(5, g_vesa_y_res-s_show_numbers, WHITE, BLACK, str);
                }
            }
            iter = 1;
            g_l_old_z.y = 0;
            g_l_old_z.x = 0;
            g_old_z.y = 0;
            g_old_z.x = 0;
            g_init.x = cr;
            g_save_c.x = cr;
            g_init.y = ci;
            g_save_c.y = ci;
            g_l_init.x = (long)(cr*g_fudge_factor);
            g_l_init.y = (long)(ci*g_fudge_factor);

            old_y = -1;
            old_x = -1;
            /*
             * MIIM code:
             * compute fixed points and use them as starting points of JIIM
             */
            if (which == jiim_types::JIIM && s_ok_to_miim)
            {
                DComplex f1;
                DComplex f2;
                DComplex Sqrt; // Fixed points of Julia

                Sqrt = ComplexSqrtFloat(1 - 4 * cr, -4 * ci);
                f1.x = (1 + Sqrt.x) / 2;
                f2.x = (1 - Sqrt.x) / 2;
                f1.y =  Sqrt.y / 2;
                f2.y = -Sqrt.y / 2;

                ClearQueue();
                s_max_hits = 1;
                EnQueueFloat((float)f1.x, (float)f1.y);
                EnQueueFloat((float)f2.x, (float)f2.y);
            }
            /*
             * End MIIM code.
             */
            if (which == jiim_types::ORBIT)
            {
                per_pixel();
            }
            // move window if bumped
            if (s_window_style == JuliaWindowStyle::LARGE && g_col > s_corner_x &&
                g_col < s_corner_x + s_win_width && g_row > s_corner_y && g_row < s_corner_y + s_win_height)
            {
                RestoreRect(s_corner_x, s_corner_y, s_win_width, s_win_height);
                if (s_corner_x == g_video_start_x + s_win_width*2)
                {
                    s_corner_x = g_video_start_x + 2;
                }
                else
                {
                    s_corner_x = g_video_start_x + s_win_width*2;
                }
                xoff = s_corner_x + s_win_width /  2;
                SaveRect(s_corner_x, s_corner_y, s_win_width, s_win_height);
            }
            if (s_window_style == JuliaWindowStyle::FULL_SCREEN)
            {
                fill_rect(g_logical_screen_x_dots, s_corner_y, s_win_width-g_logical_screen_x_dots, s_win_height-s_show_numbers, g_color_dark);
                fill_rect(s_corner_x   , g_logical_screen_y_dots, g_logical_screen_x_dots, s_win_height-g_logical_screen_y_dots-s_show_numbers, g_color_dark);
            }
            else
            {
                fill_rect(s_corner_x, s_corner_y, s_win_width, s_win_height, g_color_dark);
            }
        } // end if (driver_key_pressed)

        if (which == jiim_types::JIIM)
        {
            if (!g_has_inverse)
            {
                continue;
            }
            /*
             * MIIM code:
             * If we have MIIM queue allocated, then use MIIM method.
             */
            if (s_ok_to_miim)
            {
                if (QueueEmpty())
                {
                    if (s_max_hits < g_colors - 1
                        && s_max_hits < 5
                        && (s_lucky_x != 0.0 || s_lucky_y != 0.0))
                    {
                        s_l_max = 0;
                        s_l_size = 0;
                        g_new_z.x = s_lucky_x;
                        g_old_z.x = s_lucky_x;
                        g_new_z.y = s_lucky_y;
                        g_old_z.y = s_lucky_y;
                        s_lucky_y = 0.0f;
                        s_lucky_x = 0.0f;
                        for (int i = 0; i < 199; i++)
                        {
                            g_old_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
                            g_new_z = ComplexSqrtFloat(g_new_z.x - cr, g_new_z.y - ci);
                            EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
                            EnQueueFloat((float)-g_old_z.x, (float)-g_old_z.y);
                        }
                        s_max_hits++;
                    }
                    else
                    {
                        continue;             // loop while (still)
                    }
                }

                g_old_z = DeQueueFloat();

                x = (int)(g_old_z.x * xfactor * zoom + xoff);
                y = (int)(g_old_z.y * yfactor * zoom + yoff);
                color = c_getcolor(x, y);
                if (color < s_max_hits)
                {
                    c_putcolor(x, y, color + 1);
                    g_new_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
                    EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
                    EnQueueFloat((float)-g_new_z.x, (float)-g_new_z.y);
                }
            }
            else
            {
                /*
                 * end Msnyder code, commence if not MIIM code.
                 */
                g_old_z.x -= cr;
                g_old_z.y -= ci;
                r = g_old_z.x*g_old_z.x + g_old_z.y*g_old_z.y;
                if (r > 10.0)
                {
                    g_old_z.y = 0.0;
                    g_old_z.x = 0.0; // avoids math error
                    iter = 1;
                    r = 0;
                }
                iter++;
                color = ((count++) >> 5)%g_colors; // chg color every 32 pts
                if (color == 0)
                {
                    color = 1;
                }

                //       r = sqrt(old.x*old.x + old.y*old.y); calculated above
                r = std::sqrt(r);
                g_new_z.x = std::sqrt(std::fabs((r + g_old_z.x)/2));
                if (g_old_z.y < 0)
                {
                    g_new_z.x = -g_new_z.x;
                }

                g_new_z.y = std::sqrt(std::fabs((r - g_old_z.x)/2));

                switch (s_secret_experimental_mode)
                {
                case 0:                     // unmodified random walk
                default:
                    if (std::rand() % 2)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    break;
                case 1:                     // always go one direction
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    break;
                case 2:                     // go one dir, draw the other
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(-g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(-g_new_z.y * yfactor * zoom + yoff);
                    break;
                case 4:                     // go negative if max color
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    if (c_getcolor(x, y) == g_colors - 1)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                        x = (int)(g_new_z.x * xfactor * zoom + xoff);
                        y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    }
                    break;
                case 5:                     // go positive if max color
                    g_new_z.x = -g_new_z.x;
                    g_new_z.y = -g_new_z.y;
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    if (c_getcolor(x, y) == g_colors - 1)
                    {
                        x = (int)(g_new_z.x * xfactor * zoom + xoff);
                        y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    }
                    break;
                case 7:
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(-g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(-g_new_z.y * yfactor * zoom + yoff);
                    if (iter > 10)
                    {
                        if (mode == 0)                          // pixels
                        {
                            c_putcolor(x, y, color);
                        }
                        else if (mode & 1)              // circles
                        {
                            s_x_base = x;
                            s_y_base = y;
                            circle((int)(zoom*(s_win_width >> 1)/iter), color);
                        }
                        if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
                        {
                            driver_draw_line(x, y, old_x, old_y, color);
                        }
                        old_x = x;
                        old_y = y;
                    }
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    break;
                case 8:                     // go in long zig zags
                    if (rancnt >= 300)
                    {
                        rancnt = -300;
                    }
                    if (rancnt < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    break;
                case 9:                     // "random run"
                    switch (randir)
                    {
                    case 0:             // go random direction for a while
                        if (std::rand() % 2)
                        {
                            g_new_z.x = -g_new_z.x;
                            g_new_z.y = -g_new_z.y;
                        }
                        if (++rancnt > 1024)
                        {
                            rancnt = 0;
                            if (std::rand() % 2)
                            {
                                randir =  1;
                            }
                            else
                            {
                                randir = -1;
                            }
                        }
                        break;
                    case 1:             // now go negative dir for a while
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                        // fall through
                    case -1:            // now go positive dir for a while
                        if (++rancnt > 512)
                        {
                            rancnt = 0;
                            randir = 0;
                        }
                        break;
                    }
                    x = (int)(g_new_z.x * xfactor * zoom + xoff);
                    y = (int)(g_new_z.y * yfactor * zoom + yoff);
                    break;
                } // end switch SecretMode (sorry about the indentation)
            } // end if not MIIM
        }
        else // orbits
        {
            if (iter < g_max_iterations)
            {
                color = (int)iter%g_colors;
                if (g_integer_fractal)
                {
                    g_old_z.x = g_l_old_z.x;
                    g_old_z.x /= g_fudge_factor;
                    g_old_z.y = g_l_old_z.y;
                    g_old_z.y /= g_fudge_factor;
                }
                x = (int)((g_old_z.x - g_init.x) * xfactor * 3 * zoom + xoff);
                y = (int)((g_old_z.y - g_init.y) * yfactor * 3 * zoom + yoff);
                if (orbit_calc())
                {
                    iter = g_max_iterations;
                }
                else
                {
                    iter++;
                }
            }
            else
            {
                y = -1;
                x = -1;
                actively_computing = false;
            }
        }
        if (which == jiim_types::ORBIT || iter > 10)
        {
            if (mode == 0)                    // pixels
            {
                c_putcolor(x, y, color);
            }
            else if (mode & 1)              // circles
            {
                s_x_base = x;
                s_y_base = y;
                circle((int)(zoom*(s_win_width >> 1)/iter), color);
            }
            if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
            {
                driver_draw_line(x, y, old_x, old_y, color);
            }
            old_x = x;
            old_y = y;
        }
        g_old_z = g_new_z;
        g_l_old_z = g_l_new_z;
    } // end while (still)
finish:
    Free_Queue();

    if (kbdchar != 's' && kbdchar != 'S')
    {
        s_cursor.hide();
        if (s_window_style == JuliaWindowStyle::LARGE)
        {
            RestoreRect(s_corner_x, s_corner_y, s_win_width, s_win_height);
        }
        else if (s_window_style >= JuliaWindowStyle::FULL_SCREEN)
        {
            if (s_window_style == JuliaWindowStyle::FULL_SCREEN)
            {
                fill_rect(g_logical_screen_x_dots, s_corner_y, s_win_width-g_logical_screen_x_dots, s_win_height, g_color_dark);
                fill_rect(s_corner_x   , g_logical_screen_y_dots, g_logical_screen_x_dots, s_win_height-g_logical_screen_y_dots, g_color_dark);
            }
            else
            {
                fill_rect(s_corner_x, s_corner_y, s_win_width, s_win_height, g_color_dark);
            }
            if (s_window_style == JuliaWindowStyle::HIDDEN && s_win_width == g_vesa_x_res) // unhide
            {
                RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
                s_window_style = JuliaWindowStyle::FULL_SCREEN;
            }
            s_cursor.hide();
            ValueSaver saved_has_inverse{g_has_inverse, true};
            SaveRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
            g_logical_screen_x_offset = oldsxoffs;
            g_logical_screen_y_offset = oldsyoffs;
            RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        }
    }
    g_cursor_mouse_tracking = false;
    g_line_buff.clear();
    s_screen_rect.clear();
    g_using_jiim = false;
    if (kbdchar == 's' || kbdchar == 'S')
    {
        g_view_window = false;
        g_view_x_dots = 0;
        g_view_y_dots = 0;
        g_view_reduction = 4.2F;
        g_view_crop = true;
        g_final_aspect_ratio = g_screen_aspect;
        g_logical_screen_x_dots = g_screen_x_dots;
        g_logical_screen_y_dots = g_screen_y_dots;
        g_logical_screen_x_size_dots = g_logical_screen_x_dots - 1;
        g_logical_screen_y_size_dots = g_logical_screen_y_dots - 1;
        g_logical_screen_x_offset = 0;
        g_logical_screen_y_offset = 0;
        freetempmsg();
    }
    else
    {
        cleartempmsg();
    }
    s_show_numbers = 0;
    driver_unget_key(kbdchar);

    if (g_cur_fractal_specific->calctype == calcfroth)
    {
        froth_cleanup();
    }
}
