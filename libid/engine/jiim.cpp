// SPDX-License-Identifier: GPL-3.0-only
//
// Generates Inverse Julia in real time, lets move a cursor which determines
// the J-set.
//
//  The J-set is generated in a fixed-size window, a third of the screen.
//
#include "engine/jiim.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/frothy_basin.h"
#include "fractals/lorenz.h"
#include "math/fixed_pt.h"
#include "math/mpmath.h"
#include "misc/debug_flags.h"
#include "misc/drivers.h"
#include "misc/ValueSaver.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/editpal.h"
#include "ui/find_special_colors.h"
#include "ui/get_a_number.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

enum
{
    MAX_RECT = 1024      // largest width of SaveRect/RestoreRect
};

enum class JuliaWindowStyle
{
    LARGE = 0,       // full screen or large view window
    NO_OVERLAP = 1,  // Julia/orbit and fractal don't overlap
    FULL_SCREEN = 2, // Julia/orbit takes whole screen
    HIDDEN = 3,
};

enum class SecretMode
{
    UNMODIFIED_RANDOM_WALK = 0,
    ALWAYS_GO_ONE_DIRECTION = 1,
    GO_ONE_DIR_DRAW_OTHER_DIR = 2,
    GO_NEGATIVE_IF_MAX_COLOR = 4,
    GO_POSITIVE_IF_MAX_COLOR = 5,
    SIX = 6,
    SEVEN = 7,
    GO_IN_LONG_ZIG_ZAGS = 8,
    RANDOM_RUN = 9,
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
static SecretMode s_secret_mode{};        //
static float s_lucky_x{};                 //
static float s_lucky_y{};                 //
static CrossHairCursor s_cursor;          //

double g_julia_c_x{JULIA_C_NOT_SET}; //
double g_julia_c_y{JULIA_C_NOT_SET}; //
DComplex g_save_c{-3000.0, -3000.0}; //

static void set_aspect(double aspect)
{
    s_x_aspect = 0;
    s_y_aspect = 0;
    aspect = std::abs(aspect);
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

static void c_put_color(int x, int y, int color)
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

static int c_get_color(int x, int y)
{
    // avoid reading outside window
    if (x < s_corner_x || y < s_corner_y || x >= s_corner_x + s_win_width || y >= s_corner_y + s_win_height)
    {
        return 1000;
    }
    if (y >= g_screen_y_dots - s_show_numbers)   // avoid over reading coords
    {
        return 1000;
    }
    if (s_window_style == JuliaWindowStyle::FULL_SCREEN)   // avoid over reading fractal
    {
        if (0 <= x && x < g_logical_screen_x_dots && 0 <= y && y < g_logical_screen_y_dots)
        {
            return 1000;
        }
    }
    return get_color(x, y);
}

static void circle_plot(int x, int y, int color)
{
    if (s_x_aspect == 0)
    {
        if (s_y_aspect == 0)
        {
            c_put_color(x+s_x_base, y+s_y_base, color);
        }
        else
        {
            c_put_color(x+s_x_base, (short)(s_y_base + (((long) y * (long) s_y_aspect) >> 16)), color);
        }
    }
    else
    {
        c_put_color((int)(s_x_base + (((long) x * (long) s_x_aspect) >> 16)), y+s_y_base, color);
    }
}

static void plot8(int x, int y, int color)
{
    circle_plot(x, y, color);
    circle_plot(-x, y, color);
    circle_plot(x, -y, color);
    circle_plot(-x, -y, color);
    circle_plot(y, x, color);
    circle_plot(-y, x, color);
    circle_plot(y, -x, color);
    circle_plot(-y, -x, color);
}

static void circle(int radius, int color)
{
    int x = 0;
    int y = radius << 1;
    int sum = 0;

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

// MIIM section:
//
// Global variables and service functions used for computing
// MIIM Julias will be grouped here (and shared by code in LORENZ.C)
//
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
        put_row(x, y++, width, row.data());
    }
}

// Queue/Stack Section:
//
// Defines a buffer that can be used as a FIFO queue or LIFO stack.
//
int queue_empty()            // True if NO points remain in queue
{
    return s_list_front == s_list_back;
}

int queue_full_almost()       // True if room for ONE more point in queue
{
    return ((s_list_front + 2) % s_list_size) == s_list_back;
}

void clear_queue()
{
    s_l_max = 0;
    s_l_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

// Queue functions for MIIM julia:
bool init_queue(unsigned long request)
{
    if (driver_is_disk())
    {
        stop_msg("Don't try this in disk video mode, kids...\n");
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

void free_queue()
{
    end_disk();
    s_l_max = 0;
    s_l_size = 0;
    s_list_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

int push_long(long x, long y)
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

int push_float(float x, float y)
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

DComplex pop_float()
{
    DComplex pop;
    float pop_x;
    float pop_y;

    if (!queue_empty())
    {
        s_list_front--;
        if (s_list_front < 0)
        {
            s_list_front = s_list_size - 1;
        }
        if (from_mem_disk(8*s_list_front, sizeof(pop_x), &pop_x)
            && from_mem_disk(8*s_list_front +sizeof(pop_x), sizeof(pop_y), &pop_y))
        {
            pop.x = pop_x;
            pop.y = pop_y;
            --s_l_size;
        }
        return pop;
    }
    pop.x = 0;
    pop.y = 0;
    return pop;
}

LComplex pop_long()
{
    LComplex pop;

    if (!queue_empty())
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

int enqueue_float(float x, float y)
{
    return push_float(x, y);
}

int enqueue_long(long x, long y)
{
    return push_long(x, y);
}

DComplex dequeue_float()
{
    DComplex out;
    float out_x;
    float out_y;

    if (s_list_back != s_list_front)
    {
        if (from_mem_disk(8*s_list_back, sizeof(out_x), &out_x)
            && from_mem_disk(8*s_list_back +sizeof(out_x), sizeof(out_y), &out_y))
        {
            s_list_back = (s_list_back + 1) % s_list_size;
            out.x = out_x;
            out.y = out_y;
            s_l_size--;
        }
        return out;
    }
    out.x = 0;
    out.y = 0;
    return out;
}

LComplex dequeue_long()
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

static void save_rect(int x, int y, int width, int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    s_screen_rect.clear();
    std::vector<char> const background(width, char(g_color_dark));
    s_screen_rect.resize(width*depth);
    s_cursor.hide();
    for (int y_off = 0; y_off < depth; y_off++)
    {
        get_row(x, y+y_off, width, &s_screen_rect[width*y_off]);
        put_row(x, y+y_off, width, background.data());
    }
    s_cursor.show();
}

static void restore_rect(int x, int y, int width, int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    s_cursor.hide();
    for (int y_off = 0; y_off < depth; y_off++)
    {
        put_row(x, y+y_off, width, &s_screen_rect[width*y_off]);
    }
    s_cursor.show();
}

namespace
{

enum class OrbitFlags
{
    POINT = 0,
    CIRCLE = 1,
    LINE = 2
};

int operator+(OrbitFlags value)
{
    return static_cast<int>(value);
}
OrbitFlags operator&(OrbitFlags lhs, OrbitFlags rhs)
{
    return static_cast<OrbitFlags>(+lhs & +rhs);
}
bool bit_set(OrbitFlags value, OrbitFlags bit)
{
    return (value & bit) == bit;
}
OrbitFlags operator^(OrbitFlags lhs, OrbitFlags rhs)
{
    return static_cast<OrbitFlags>(+lhs ^ +rhs);
}
OrbitFlags &operator^=(OrbitFlags &lhs, OrbitFlags rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

class InverseJulia
{
public:
    InverseJulia(JIIMType which);

    void process();
    
private:
    void start();
    void finish();

    JIIMType m_which;
    Affine cvt{};
    bool exact{};
    int count{};           // coloring julia
    double c_real{};
    double c_imag{};
    double r{};
    int x_factor{};
    int y_factor{}; // aspect ratio
    int x_off{};
    int y_off{}; // center of the window
    int x{};
    int y{};
    int key{-1};
    long iter{};
    int color{};
    float zoom{};
    int old_x{};
    int old_y{};
    double aspect{};
    bool actively_computing{true};
    bool first_time{true};
    int old_screen_x_offset{g_logical_screen_x_offset};
    int old_screen_y_offset{g_logical_screen_y_offset};

    static OrbitFlags mode; // point, circle, ...
    static int ran_dir;
    static int ran_cnt;
};

OrbitFlags InverseJulia::mode{};
int InverseJulia::ran_dir{};
int InverseJulia::ran_cnt{};

InverseJulia::InverseJulia(JIIMType which) :
    m_which(which)
{
}

void InverseJulia::start()
{
    if (m_which == JIIMType::ORBIT)
    {
        g_has_inverse = true;
    }

    s_show_numbers = 0;
    g_using_jiim = true;
    g_line_buff.resize(std::max(g_screen_x_dots, g_screen_y_dots));
    aspect = ((double) g_logical_screen_x_dots * 3) / ((double) g_logical_screen_y_dots * 4); // assumes 4:3
    actively_computing = true;
    set_aspect(aspect);

    if (m_which == JIIMType::ORBIT)
    {
        per_image();
    }

    s_cursor = CrossHairCursor();

    // MIIM code: Grab memory for Queue/Stack before SaveRect gets it.
    s_ok_to_miim = false;
    if (m_which == JIIMType::JIIM && g_debug_flag != DebugFlags::PREVENT_MIIM)
    {
        s_ok_to_miim = init_queue(8 * 1024UL); // Queue Set-up Successful?
    }

    s_max_hits = 1;
    if (m_which == JIIMType::ORBIT)
    {
        g_plot = c_put_color; // for line with clipping
    }

    g_vesa_x_res = g_screen_x_dots;
    g_vesa_y_res = g_screen_y_dots;

    if (g_logical_screen_x_offset != 0 || g_logical_screen_y_offset != 0) // we're in view windows
    {
        ValueSaver saved_has_inverse(g_has_inverse, true);
        save_rect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        g_logical_screen_x_offset = g_video_start_x;
        g_logical_screen_y_offset = g_video_start_y;
        restore_rect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
    }

    if (g_logical_screen_x_dots == g_vesa_x_res || g_logical_screen_y_dots == g_vesa_y_res ||
        g_vesa_x_res - g_logical_screen_x_dots < g_vesa_x_res / 3 ||
        g_vesa_y_res - g_logical_screen_y_dots < g_vesa_y_res / 3 || g_logical_screen_x_dots >= MAX_RECT)
    {
        // this mode puts orbit/julia in an overlapping window 1/3 the size of the physical screen
        s_window_style = JuliaWindowStyle::LARGE;
        s_win_width = g_vesa_x_res / 3;
        s_win_height = g_vesa_y_res / 3;
        s_corner_x = g_video_start_x + s_win_width * 2;
        s_corner_y = g_video_start_y + s_win_height * 2;
        x_off = g_video_start_x + s_win_width * 5 / 2;
        y_off = g_video_start_y + s_win_height * 5 / 2;
    }
    else if (g_logical_screen_x_dots > g_vesa_x_res / 3 && g_logical_screen_y_dots > g_vesa_y_res / 3)
    {
        s_window_style = JuliaWindowStyle::NO_OVERLAP;
        s_win_width = g_vesa_x_res - g_logical_screen_x_dots;
        s_win_height = g_vesa_y_res - g_logical_screen_y_dots;
        s_corner_x = g_video_start_x + g_logical_screen_x_dots;
        s_corner_y = g_video_start_y + g_logical_screen_y_dots;
        x_off = s_corner_x + s_win_width / 2;
        y_off = s_corner_y + s_win_height / 2;
    }
    else
    {
        s_window_style = JuliaWindowStyle::FULL_SCREEN;
        s_win_width = g_vesa_x_res;
        s_win_height = g_vesa_y_res;
        s_corner_x = g_video_start_x;
        s_corner_y = g_video_start_y;
        x_off = g_video_start_x + s_win_width / 2;
        y_off = g_video_start_y + s_win_height / 2;
    }

    x_factor = (int) (s_win_width / 5.33);
    y_factor = (int) (-s_win_height / 4);

    if (s_window_style == JuliaWindowStyle::LARGE)
    {
        save_rect(s_corner_x, s_corner_y, s_win_width, s_win_height);
    }
    else if (s_window_style == JuliaWindowStyle::FULL_SCREEN) // leave the fractal
    {
        fill_rect(g_logical_screen_x_dots, s_corner_y, s_win_width - g_logical_screen_x_dots, s_win_height,
            g_color_dark);
        fill_rect(s_corner_x, g_logical_screen_y_dots, g_logical_screen_x_dots,
            s_win_height - g_logical_screen_y_dots, g_color_dark);
    }
    else // blank whole window
    {
        fill_rect(s_corner_x, s_corner_y, s_win_width, s_win_height, g_color_dark);
    }

    setup_convert_to_screen(&cvt);

    // reuse last location if inside window
    g_col = (int) std::lround(cvt.a * g_save_c.x + cvt.b * g_save_c.y + cvt.e);
    g_row = (int) std::lround(cvt.c * g_save_c.x + cvt.d * g_save_c.y + cvt.f);
    if (g_col < 0 || g_col >= g_logical_screen_x_dots || g_row < 0 || g_row >= g_logical_screen_y_dots)
    {
        c_real = (g_x_max + g_x_min) / 2.0;
        c_imag = (g_y_max + g_y_min) / 2.0;
    }
    else
    {
        c_real = g_save_c.x;
        c_imag = g_save_c.y;
    }

    old_y = -1;
    old_x = -1;

    g_col = (int) std::lround(cvt.a * c_real + cvt.b * c_imag + cvt.e);
    g_row = (int) std::lround(cvt.c * c_real + cvt.d * c_imag + cvt.f);

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
    zoom = 1.0f;

    g_cursor_mouse_tracking = true;
}

void InverseJulia::process()
{
    ValueSaver saved_debug_flag{g_debug_flag};
    ValueSaver saved_help_mode{
        g_help_mode, m_which == JIIMType::JIIM ? HelpLabels::HELP_JIIM : HelpLabels::HELP_ORBITS};
    ValueSaver saved_calc_type{g_calc_type};
    ValueSaver saved_look_at_mouse{g_look_at_mouse, +MouseLook::POSITION};

    start();

    bool still{true};
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
        if (driver_key_pressed() || first_time)
        {
            first_time = false;
            while (driver_key_pressed())
            {
                s_cursor.wait_key();
                key = driver_get_key();

                int d_col = 0;
                int d_row = 0;
                g_julia_c_x = JULIA_C_NOT_SET;
                g_julia_c_y = JULIA_C_NOT_SET;
                switch (key)
                {
                case ID_KEY_CTL_KEYPAD_5: // ctrl - keypad 5
                case ID_KEY_KEYPAD_5:     // keypad 5
                    break;                // do nothing

                case ID_KEY_CTL_PAGE_UP:
                    d_col = 4;
                    d_row = -4;
                    break;

                case ID_KEY_CTL_PAGE_DOWN:
                    d_col = 4;
                    d_row = 4;
                    break;
                    
                case ID_KEY_CTL_HOME:
                    d_col = -4;
                    d_row = -4;
                    break;
                    
                case ID_KEY_CTL_END:
                    d_col = -4;
                    d_row = 4;
                    break;
                    
                case ID_KEY_PAGE_UP:
                    d_col = 1;
                    d_row = -1;
                    break;
                    
                case ID_KEY_PAGE_DOWN:
                    d_col = 1;
                    d_row = 1;
                    break;
                    
                case ID_KEY_HOME:
                    d_col = -1;
                    d_row = -1;
                    break;
                    
                case ID_KEY_END:
                    d_col = -1;
                    d_row = 1;
                    break;
                    
                case ID_KEY_UP_ARROW:
                    d_row = -1;
                    break;
                    
                case ID_KEY_DOWN_ARROW:
                    d_row = 1;
                    break;
                    
                case ID_KEY_LEFT_ARROW:
                    d_col = -1;
                    break;
                    
                case ID_KEY_RIGHT_ARROW:
                    d_col = 1;
                    break;
                    
                case ID_KEY_CTL_UP_ARROW:
                    d_row = -4;
                    break;
                    
                case ID_KEY_CTL_DOWN_ARROW:
                    d_row = 4;
                    break;
                    
                case ID_KEY_CTL_LEFT_ARROW:
                    d_col = -4;
                    break;
                    
                case ID_KEY_CTL_RIGHT_ARROW:
                    d_col = 4;
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
                    g_julia_c_x = c_real;
                    g_julia_c_y = c_imag;
                    goto finish;
                    
                case 'c':
                case 'C':
                    mode ^= OrbitFlags::CIRCLE;
                    break;
                    
                case 'l':
                case 'L':
                    mode ^= OrbitFlags::LINE;
                    break;
                    
                case 'n':
                case 'N':
                    s_show_numbers = 8 - s_show_numbers;
                    if (s_window_style == JuliaWindowStyle::LARGE && s_show_numbers == 0)
                    {
                        s_cursor.hide();
                        clear_temp_msg();
                        s_cursor.show();
                    }
                    break;
                    
                case 'p':
                case 'P':
                    get_a_number(&c_real, &c_imag);
                    exact = true;
                    g_col = (int) std::lround(cvt.a * c_real + cvt.b * c_imag + cvt.e);
                    g_row = (int) std::lround(cvt.c * c_real + cvt.d * c_imag + cvt.f);
                    d_row = 0;
                    d_col = 0;
                    break;
                    
                case 'h':   // hide fractal toggle
                case 'H':   // hide fractal toggle
                    if (s_window_style == JuliaWindowStyle::FULL_SCREEN)
                    {
                        s_window_style = JuliaWindowStyle::HIDDEN;
                    }
                    else if (s_window_style == JuliaWindowStyle::HIDDEN && s_win_width == g_vesa_x_res)
                    {
                        restore_rect(g_video_start_x, g_video_start_y, g_logical_screen_x_dots, g_logical_screen_y_dots);
                        s_window_style = JuliaWindowStyle::FULL_SCREEN;
                    }
                    break;
                    
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
                    if (m_which == JIIMType::JIIM)
                    {
                        s_secret_mode = static_cast<SecretMode>(key - '0');
                    }
                    else
                    {
                        still = false;
                    }
                    break;
                    
                default:
                    still = false;
                    break;
                }  // switch

                if (key == 's' || key == 'S')
                {
                    goto finish;
                }
                
                if (d_col > 0 || d_row > 0)
                {
                    exact = false;
                }
                g_col += d_col;
                g_row += d_row;

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
                    c_real = g_l_x_pixel();
                    c_imag = g_l_y_pixel();
                    c_real /= (1L << g_bit_shift);
                    c_imag /= (1L << g_bit_shift);
                }
                else
                {
                    c_real = g_dx_pixel();
                    c_imag = g_dy_pixel();
                }
            }
            
            actively_computing = true;
            if (s_show_numbers) // write coordinates on screen
            {
                char str[41];
                std::snprintf(str, std::size(str), "%16.14f %16.14f %3d", c_real, c_imag, get_color(g_col, g_row));
                if (s_window_style == JuliaWindowStyle::LARGE)
                {
                    // show temp msg will clear self if new msg is a different length - pad to length 40
                    while ((int)std::strlen(str) < 40)
                    {
                        std::strcat(str, " ");
                    }
                    str[40] = 0;
                    s_cursor.hide();
                    actively_computing = true;
                    show_temp_msg(str);
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
            g_init.x = c_real;
            g_save_c.x = c_real;
            g_init.y = c_imag;
            g_save_c.y = c_imag;
            g_l_init.x = (long)(c_real*g_fudge_factor);
            g_l_init.y = (long)(c_imag*g_fudge_factor);

            old_y = -1;
            old_x = -1;
            // MIIM code: compute fixed points and use them as starting points of JIIM
            if (m_which == JIIMType::JIIM && s_ok_to_miim)
            {
                DComplex f1;
                DComplex f2;
                DComplex Sqrt; // Fixed points of Julia

                Sqrt = complex_sqrt_float(1 - 4 * c_real, -4 * c_imag);
                f1.x = (1 + Sqrt.x) / 2;
                f2.x = (1 - Sqrt.x) / 2;
                f1.y =  Sqrt.y / 2;
                f2.y = -Sqrt.y / 2;

                clear_queue();
                s_max_hits = 1;
                enqueue_float((float)f1.x, (float)f1.y);
                enqueue_float((float)f2.x, (float)f2.y);
            }
            // End MIIM code.
            if (m_which == JIIMType::ORBIT)
            {
                per_pixel();
            }
            // move window if bumped
            if (s_window_style == JuliaWindowStyle::LARGE && g_col > s_corner_x &&
                g_col < s_corner_x + s_win_width && g_row > s_corner_y && g_row < s_corner_y + s_win_height)
            {
                restore_rect(s_corner_x, s_corner_y, s_win_width, s_win_height);
                if (s_corner_x == g_video_start_x + s_win_width*2)
                {
                    s_corner_x = g_video_start_x + 2;
                }
                else
                {
                    s_corner_x = g_video_start_x + s_win_width*2;
                }
                x_off = s_corner_x + s_win_width /  2;
                save_rect(s_corner_x, s_corner_y, s_win_width, s_win_height);
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
        }

        if (m_which == JIIMType::JIIM)
        {
            if (!g_has_inverse)
            {
                continue;
            }
            // If we have MIIM queue allocated, then use MIIM method.
            if (s_ok_to_miim)
            {
                if (queue_empty())
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
                            g_old_z = complex_sqrt_float(g_old_z.x - c_real, g_old_z.y - c_imag);
                            g_new_z = complex_sqrt_float(g_new_z.x - c_real, g_new_z.y - c_imag);
                            enqueue_float((float)g_new_z.x, (float)g_new_z.y);
                            enqueue_float((float)-g_old_z.x, (float)-g_old_z.y);
                        }
                        s_max_hits++;
                    }
                    else
                    {
                        continue;             // loop while (still)
                    }
                }

                g_old_z = dequeue_float();

                x = (int)(g_old_z.x * x_factor * zoom + x_off);
                y = (int)(g_old_z.y * y_factor * zoom + y_off);
                color = c_get_color(x, y);
                if (color < s_max_hits)
                {
                    c_put_color(x, y, color + 1);
                    g_new_z = complex_sqrt_float(g_old_z.x - c_real, g_old_z.y - c_imag);
                    enqueue_float((float)g_new_z.x, (float)g_new_z.y);
                    enqueue_float((float)-g_new_z.x, (float)-g_new_z.y);
                }
            }
            else
            {
                // not MIIM code.
                g_old_z.x -= c_real;
                g_old_z.y -= c_imag;
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
                g_new_z.x = std::sqrt(std::abs((r + g_old_z.x)/2));
                if (g_old_z.y < 0)
                {
                    g_new_z.x = -g_new_z.x;
                }

                g_new_z.y = std::sqrt(std::abs((r - g_old_z.x)/2));

                switch (s_secret_mode)
                {
                case SecretMode::UNMODIFIED_RANDOM_WALK:
                default:
                    if (std::rand() % 2)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    break;

                case SecretMode::ALWAYS_GO_ONE_DIRECTION:
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    break;
                    
                case SecretMode::GO_ONE_DIR_DRAW_OTHER_DIR:
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(-g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(-g_new_z.y * y_factor * zoom + y_off);
                    break;
                    
                case SecretMode::GO_NEGATIVE_IF_MAX_COLOR:
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    if (c_get_color(x, y) == g_colors - 1)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                        x = (int)(g_new_z.x * x_factor * zoom + x_off);
                        y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    }
                    break;
                    
                case SecretMode::GO_POSITIVE_IF_MAX_COLOR:
                    g_new_z.x = -g_new_z.x;
                    g_new_z.y = -g_new_z.y;
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    if (c_get_color(x, y) == g_colors - 1)
                    {
                        x = (int)(g_new_z.x * x_factor * zoom + x_off);
                        y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    }
                    break;
                    
                case SecretMode::SEVEN:
                    if (g_save_c.y < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(-g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(-g_new_z.y * y_factor * zoom + y_off);
                    if (iter > 10)
                    {
                        if (mode == OrbitFlags::POINT)
                        {
                            c_put_color(x, y, color);
                        }
                        if (bit_set(mode, OrbitFlags::CIRCLE))
                        {
                            s_x_base = x;
                            s_y_base = y;
                            circle((int)(zoom*(s_win_width >> 1)/iter), color);
                        }
                        if (bit_set(mode, OrbitFlags::LINE) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
                        {
                            driver_draw_line(x, y, old_x, old_y, color);
                        }
                        old_x = x;
                        old_y = y;
                    }
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    break;
                    
                case SecretMode::GO_IN_LONG_ZIG_ZAGS:
                    if (ran_cnt >= 300)
                    {
                        ran_cnt = -300;
                    }
                    if (ran_cnt < 0)
                    {
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                    }
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    break;
                    
                case SecretMode::RANDOM_RUN:
                    switch (ran_dir)
                    {
                    case 0:             // go random direction for a while
                        if (std::rand() % 2)
                        {
                            g_new_z.x = -g_new_z.x;
                            g_new_z.y = -g_new_z.y;
                        }
                        if (++ran_cnt > 1024)
                        {
                            ran_cnt = 0;
                            if (std::rand() % 2)
                            {
                                ran_dir =  1;
                            }
                            else
                            {
                                ran_dir = -1;
                            }
                        }
                        break;
                    case 1:             // now go negative dir for a while
                        g_new_z.x = -g_new_z.x;
                        g_new_z.y = -g_new_z.y;
                        // fall through
                    case -1:            // now go positive dir for a while
                        if (++ran_cnt > 512)
                        {
                            ran_cnt = 0;
                            ran_dir = 0;
                        }
                        break;
                    }
                    x = (int)(g_new_z.x * x_factor * zoom + x_off);
                    y = (int)(g_new_z.y * y_factor * zoom + y_off);
                    break;
                }
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
                x = (int)((g_old_z.x - g_init.x) * x_factor * 3 * zoom + x_off);
                y = (int)((g_old_z.y - g_init.y) * y_factor * 3 * zoom + y_off);
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
        if (m_which == JIIMType::ORBIT || iter > 10)
        {
            if (mode == OrbitFlags::POINT)
            {
                c_put_color(x, y, color);
            }
            if (bit_set(mode, OrbitFlags::CIRCLE))
            {
                s_x_base = x;
                s_y_base = y;
                circle((int)(zoom*(s_win_width >> 1)/iter), color);
            }
            if (bit_set(mode, OrbitFlags::LINE) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
            {
                driver_draw_line(x, y, old_x, old_y, color);
            }
            old_x = x;
            old_y = y;
        }
        g_old_z = g_new_z;
        g_l_old_z = g_l_new_z;
    }

finish:
    finish();
}

void InverseJulia::finish()
{
    free_queue();

    if (key != 's' && key != 'S')
    {
        s_cursor.hide();
        if (s_window_style == JuliaWindowStyle::LARGE)
        {
            restore_rect(s_corner_x, s_corner_y, s_win_width, s_win_height);
        }
        else if (s_window_style >= JuliaWindowStyle::FULL_SCREEN)
        {
            if (s_window_style == JuliaWindowStyle::FULL_SCREEN)
            {
                fill_rect(g_logical_screen_x_dots, s_corner_y, s_win_width - g_logical_screen_x_dots,
                    s_win_height, g_color_dark);
                fill_rect(s_corner_x, g_logical_screen_y_dots, g_logical_screen_x_dots,
                    s_win_height - g_logical_screen_y_dots, g_color_dark);
            }
            else
            {
                fill_rect(s_corner_x, s_corner_y, s_win_width, s_win_height, g_color_dark);
            }
            if (s_window_style == JuliaWindowStyle::HIDDEN && s_win_width == g_vesa_x_res) // unhide
            {
                restore_rect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
                s_window_style = JuliaWindowStyle::FULL_SCREEN;
            }
            s_cursor.hide();
            ValueSaver saved_has_inverse{g_has_inverse, true};
            save_rect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
            g_logical_screen_x_offset = old_screen_x_offset;
            g_logical_screen_y_offset = old_screen_y_offset;
            restore_rect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        }
    }
    g_cursor_mouse_tracking = false;
    g_line_buff.clear();
    s_screen_rect.clear();
    g_using_jiim = false;
    if (key == 's' || key == 'S')
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
        free_temp_msg();
    }
    else
    {
        clear_temp_msg();
    }
    s_show_numbers = 0;
    driver_unget_key(key);

    if (g_cur_fractal_specific->calc_type == calc_froth)
    {
        froth_cleanup();
    }
}

} // namespace

void jiim(JIIMType which)
{
    // must use standard fractal or be calcfroth
    if (g_fractal_specific[+g_fractal_type].calc_type != standard_fractal &&
        g_fractal_specific[+g_fractal_type].calc_type != calc_froth)
    {
        return;
    }

    InverseJulia tool(which);
    tool.process();
}
