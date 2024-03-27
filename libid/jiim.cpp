/*
 * JIIM.C
 *
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
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "frothy_basin.h"
#include "get_a_number.h"
#include "get_color.h"
#include "helpdefs.h"
#include "id_data.h"
#include "lorenz.h"
#include "miscfrac.h"
#include "os.h"
#include "pixel_grid.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "zoom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#define MAXRECT         1024      // largest width of SaveRect/RestoreRect

static int show_numbers = 0;              // toggle for display of coords
static std::vector<char> screen_rect;
static int windows = 0;               // windows management system

static int xc, yc;                       // corners of the window
static int xd, yd;                       // dots in the window
double g_julia_c_x = BIG;
double g_julia_c_y = BIG;

// circle routines from Dr. Dobbs June 1990
static int xbase, ybase;
static unsigned int xAspect, yAspect;

void SetAspect(double aspect)
{
    xAspect = 0;
    yAspect = 0;
    aspect = std::fabs(aspect);
    if (aspect != 1.0)
    {
        if (aspect > 1.0)
        {
            yAspect = (unsigned int)(65536.0 / aspect);
        }
        else
        {
            xAspect = (unsigned int)(65536.0 * aspect);
        }
    }
}

void c_putcolor(int x, int y, int color)
{
    // avoid writing outside window
    if (x < xc || y < yc || x >= xc + xd || y >= yc + yd)
    {
        return ;
    }
    if (y >= g_screen_y_dots - show_numbers)   // avoid overwriting coords
    {
        return;
    }
    if (windows == 2)   // avoid overwriting fractal
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
    if (x < xc || y < yc || x >= xc + xd || y >= yc + yd)
    {
        return 1000;
    }
    if (y >= g_screen_y_dots - show_numbers)   // avoid overreading coords
    {
        return 1000;
    }
    if (windows == 2)   // avoid overreading fractal
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
    if (xAspect == 0)
    {
        if (yAspect == 0)
        {
            c_putcolor(x+xbase, y+ybase, color);
        }
        else
        {
            c_putcolor(x+xbase, (short)(ybase + (((long) y * (long) yAspect) >> 16)), color);
        }
    }
    else
    {
        c_putcolor((int)(xbase + (((long) x * (long) xAspect) >> 16)), y+ybase, color);
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
    int x, y, sum;

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


static long   ListFront, ListBack, ListSize;  // head, tail, size of MIIM Queue
static long   lsize, lmax;                    // how many in queue (now, ever)
static int    maxhits = 1;
static bool   OKtoMIIM = false;
static int    SecretExperimentalMode;
static float  luckyx = 0, luckyy = 0;

static void fillrect(int x, int y, int width, int depth, int color)
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
        putrow(x, y++, width, &row[0]);
    }
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int QueueEmpty()            // True if NO points remain in queue
{
    return ListFront == ListBack;
}

int QueueFullAlmost()       // True if room for ONE more point in queue
{
    return ((ListFront + 2) % ListSize) == ListBack;
}

void ClearQueue()
{
    lmax = 0;
    lsize = lmax;
    ListBack = lsize;
    ListFront = ListBack;
}


/*
 * Queue functions for MIIM julia:
 * move to JIIM.C when done
 */

bool Init_Queue(unsigned long request)
{
    if (driver_diskp())
    {
        stopmsg(STOPMSG_NONE, "Don't try this in disk video mode, kids...\n");
        ListSize = 0;
        return false;
    }

    for (ListSize = request; ListSize > 1024; ListSize /= 2)
    {
        switch (common_startdisk(ListSize * 8, 1, 256))
        {
        case 0:                        // success
            ListBack = 0;
            ListFront = ListBack;
            lmax = 0;
            lsize = lmax;
            return true;
        case -1:
            continue;                   // try smaller queue size
        case -2:
            ListSize = 0;               // cancelled by user
            return false;
        }
    }

    // failed to get memory for MIIM Queue
    ListSize = 0;
    return false;
}

void Free_Queue()
{
    enddisk();
    lmax = 0;
    lsize = lmax;
    ListSize = lsize;
    ListBack = ListSize;
    ListFront = ListBack;
}

int PushLong(long x, long y)
{
    if (((ListFront + 1) % ListSize) != ListBack)
    {
        if (ToMemDisk(8*ListFront, sizeof(x), &x)
            && ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
        {
            ListFront = (ListFront + 1) % ListSize;
            if (++lsize > lmax)
            {
                lmax   = lsize;
                luckyx = (float)x;
                luckyy = (float)y;
            }
            return 1;
        }
    }
    return 0;                    // fail
}

int PushFloat(float x, float y)
{
    if (((ListFront + 1) % ListSize) != ListBack)
    {
        if (ToMemDisk(8*ListFront, sizeof(x), &x)
            && ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
        {
            ListFront = (ListFront + 1) % ListSize;
            if (++lsize > lmax)
            {
                lmax   = lsize;
                luckyx = x;
                luckyy = y;
            }
            return 1;
        }
    }
    return 0;                    // fail
}

DComplex PopFloat()
{
    DComplex pop;
    float  popx, popy;

    if (!QueueEmpty())
    {
        ListFront--;
        if (ListFront < 0)
        {
            ListFront = ListSize - 1;
        }
        if (FromMemDisk(8*ListFront, sizeof(popx), &popx)
            && FromMemDisk(8*ListFront +sizeof(popx), sizeof(popy), &popy))
        {
            pop.x = popx;
            pop.y = popy;
            --lsize;
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
        ListFront--;
        if (ListFront < 0)
        {
            ListFront = ListSize - 1;
        }
        if (FromMemDisk(8*ListFront, sizeof(pop.x), &pop.x)
            && FromMemDisk(8*ListFront +sizeof(pop.x), sizeof(pop.y), &pop.y))
        {
            --lsize;
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
    float outx, outy;

    if (ListBack != ListFront)
    {
        if (FromMemDisk(8*ListBack, sizeof(outx), &outx)
            && FromMemDisk(8*ListBack +sizeof(outx), sizeof(outy), &outy))
        {
            ListBack = (ListBack + 1) % ListSize;
            out.x = outx;
            out.y = outy;
            lsize--;
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

    if (ListBack != ListFront)
    {
        if (FromMemDisk(8*ListBack, sizeof(out.x), &out.x)
            && FromMemDisk(8*ListBack +sizeof(out.x), sizeof(out.y), &out.y))
        {
            ListBack = (ListBack + 1) % ListSize;
            lsize--;
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

    screen_rect.clear();
    std::vector<char> const background(width, char(g_color_dark));
    screen_rect.resize(width*depth);
    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        getrow(x, y+yoff, width, &screen_rect[width*yoff]);
        putrow(x, y+yoff, width, &background[0]);
    }
    Cursor_Show();
}


static void RestoreRect(int x, int y, int width, int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        putrow(x, y+yoff, width, &screen_rect[width*yoff]);
    }
    Cursor_Show();
}

/*
 * interface to FRACTINT
 */

DComplex g_save_c = {-3000.0, -3000.0};

void Jiim(jiim_types which)
{
    affine cvt;
    bool exact = false;
    int count = 0;            // coloring julia
    static int mode = 0;      // point, circle, ...
    int       old_look_at_mouse = g_look_at_mouse;
    double cr, ci, r;
    int xfactor, yfactor;             // aspect ratio

    int xoff, yoff;                   // center of the window
    int x, y;
    int kbdchar = -1;

    long iter;
    int color;
    float zoom;
    int oldsxoffs, oldsyoffs;
    int (*oldcalctype)();
    int old_x, old_y;
    double aspect;
    static int randir = 0;
    static int rancnt = 0;
    bool actively_computing = true;
    bool first_time = true;

    debug_flags old_debugflag = g_debug_flag;
    // must use standard fractal or be calcfroth
    if (g_fractal_specific[static_cast<int>(g_fractal_type)].calctype != standard_fractal
        && g_fractal_specific[static_cast<int>(g_fractal_type)].calctype != calcfroth)
    {
        return;
    }
    help_labels const old_help_mode = g_help_mode;
    if (which == jiim_types::JIIM)
    {
        g_help_mode = help_labels::HELP_JIIM;
    }
    else
    {
        g_help_mode = help_labels::HELP_ORBITS;
        g_has_inverse = true;
    }
    oldsxoffs = g_logical_screen_x_offset;
    oldsyoffs = g_logical_screen_y_offset;
    oldcalctype = g_calc_type;
    show_numbers = 0;
    g_using_jiim = true;
    g_line_buff.resize(std::max(g_screen_x_dots, g_screen_y_dots));
    aspect = ((double)g_logical_screen_x_dots*3)/((double)g_logical_screen_y_dots*4);  // assumes 4:3
    actively_computing = true;
    SetAspect(aspect);
    g_look_at_mouse = 3;

    if (which == jiim_types::ORBIT)
    {
        per_image();
    }

    Cursor_Construct();

    /*
     * MIIM code:
     * Grab memory for Queue/Stack before SaveRect gets it.
     */
    OKtoMIIM  = false;
    if (which == jiim_types::JIIM && g_debug_flag != debug_flags::prevent_miim)
    {
        OKtoMIIM = Init_Queue(8*1024UL); // Queue Set-up Successful?
    }

    maxhits = 1;
    if (which == jiim_types::ORBIT)
    {
        g_plot = c_putcolor;                // for line with clipping
    }

    /*
     * end MIIM code.
     */

    if (!g_video_scroll)
    {
        g_vesa_x_res = g_screen_x_dots;
        g_vesa_y_res = g_screen_y_dots;
    }

    if (g_logical_screen_x_offset != 0 || g_logical_screen_y_offset != 0) // we're in view windows
    {
        bool const savehasinverse = g_has_inverse;
        g_has_inverse = true;
        SaveRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        g_logical_screen_x_offset = g_video_start_x;
        g_logical_screen_y_offset = g_video_start_y;
        RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
        g_has_inverse = savehasinverse;
    }

    if (g_logical_screen_x_dots == g_vesa_x_res
        || g_logical_screen_y_dots == g_vesa_y_res
        || g_vesa_x_res-g_logical_screen_x_dots < g_vesa_x_res/3
        || g_vesa_y_res-g_logical_screen_y_dots < g_vesa_y_res/3
        || g_logical_screen_x_dots >= MAXRECT)
    {
        /* this mode puts orbit/julia in an overlapping window 1/3 the size of
           the physical screen */
        windows = 0; // full screen or large view window
        xd = g_vesa_x_res / 3;
        yd = g_vesa_y_res / 3;
        xc = g_video_start_x + xd * 2;
        yc = g_video_start_y + yd * 2;
        xoff = g_video_start_x + xd * 5 / 2;
        yoff = g_video_start_y + yd * 5 / 2;
    }
    else if (g_logical_screen_x_dots > g_vesa_x_res/3 && g_logical_screen_y_dots > g_vesa_y_res/3)
    {
        // Julia/orbit and fractal don't overlap
        windows = 1;
        xd = g_vesa_x_res - g_logical_screen_x_dots;
        yd = g_vesa_y_res - g_logical_screen_y_dots;
        xc = g_video_start_x + g_logical_screen_x_dots;
        yc = g_video_start_y + g_logical_screen_y_dots;
        xoff = xc + xd/2;
        yoff = yc + yd/2;

    }
    else
    {
        // Julia/orbit takes whole screen
        windows = 2;
        xd = g_vesa_x_res;
        yd = g_vesa_y_res;
        xc = g_video_start_x;
        yc = g_video_start_y;
        xoff = g_video_start_x + xd/2;
        yoff = g_video_start_y + yd/2;
    }

    xfactor = (int)(xd/5.33);
    yfactor = (int)(-yd/4);

    if (windows == 0)
    {
        SaveRect(xc, yc, xd, yd);
    }
    else if (windows == 2)    // leave the fractal
    {
        fillrect(g_logical_screen_x_dots, yc, xd-g_logical_screen_x_dots, yd, g_color_dark);
        fillrect(xc   , g_logical_screen_y_dots, g_logical_screen_x_dots, yd-g_logical_screen_y_dots, g_color_dark);
    }
    else    // blank whole window
    {
        fillrect(xc, yc, xd, yd, g_color_dark);
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
    old_x = old_y;

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

    Cursor_SetPos(g_col, g_row);
    Cursor_Show();
    color = g_color_bright;

    iter = 1;
    bool still = true;
    zoom = 1;

#ifdef XFRACT
    Cursor_StartMouseTracking();
#endif

    while (still)
    {
        if (actively_computing)
        {
            Cursor_CheckBlink();
        }
        else
        {
            Cursor_WaitKey();
        }
        if (driver_key_pressed() || first_time) // prevent burning up UNIX CPU
        {
            first_time = false;
            while (driver_key_pressed())
            {
                Cursor_WaitKey();
                kbdchar = driver_get_key();

                int dcol = 0;
                int drow = 0;
                g_julia_c_x = BIG;
                g_julia_c_y = BIG;
                switch (kbdchar)
                {
                case FIK_CTL_KEYPAD_5:      // ctrl - keypad 5
                case FIK_KEYPAD_5:          // keypad 5
                    break;                  // do nothing
                case FIK_CTL_PAGE_UP:
                    dcol = 4;
                    drow = -4;
                    break;
                case FIK_CTL_PAGE_DOWN:
                    dcol = 4;
                    drow = 4;
                    break;
                case FIK_CTL_HOME:
                    dcol = -4;
                    drow = -4;
                    break;
                case FIK_CTL_END:
                    dcol = -4;
                    drow = 4;
                    break;
                case FIK_PAGE_UP:
                    dcol = 1;
                    drow = -1;
                    break;
                case FIK_PAGE_DOWN:
                    dcol = 1;
                    drow = 1;
                    break;
                case FIK_HOME:
                    dcol = -1;
                    drow = -1;
                    break;
                case FIK_END:
                    dcol = -1;
                    drow = 1;
                    break;
                case FIK_UP_ARROW:
                    drow = -1;
                    break;
                case FIK_DOWN_ARROW:
                    drow = 1;
                    break;
                case FIK_LEFT_ARROW:
                    dcol = -1;
                    break;
                case FIK_RIGHT_ARROW:
                    dcol = 1;
                    break;
                case FIK_CTL_UP_ARROW:
                    drow = -4;
                    break;
                case FIK_CTL_DOWN_ARROW:
                    drow = 4;
                    break;
                case FIK_CTL_LEFT_ARROW:
                    dcol = -4;
                    break;
                case FIK_CTL_RIGHT_ARROW:
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
                case FIK_SPACE:
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
                    show_numbers = 8 - show_numbers;
                    if (windows == 0 && show_numbers == 0)
                    {
                        Cursor_Hide();
                        cleartempmsg();
                        Cursor_Show();
                    }
                    break;
                case 'p':
                case 'P':
                    get_a_number(&cr, &ci);
                    exact = true;
                    g_col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
                    g_row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);
                    drow = 0;
                    dcol = drow;
                    break;
                case 'h':   // hide fractal toggle
                case 'H':   // hide fractal toggle
                    if (windows == 2)
                    {
                        windows = 3;
                    }
                    else if (windows == 3 && xd == g_vesa_x_res)
                    {
                        RestoreRect(g_video_start_x, g_video_start_y, g_logical_screen_x_dots, g_logical_screen_y_dots);
                        windows = 2;
                    }
                    break;
#ifdef XFRACT
                case FIK_ENTER:
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
                        SecretExperimentalMode = kbdchar - '0';
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
                if (kbdchar == FIK_ENTER)
                {
                    // We want to use the position of the cursor
                    exact = false;
                    g_col = Cursor_GetX();
                    g_row = Cursor_GetY();
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

                Cursor_SetPos(g_col, g_row);
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
            if (show_numbers) // write coordinates on screen
            {
                char str[41];
                std::snprintf(str, std::size(str), "%16.14f %16.14f %3d", cr, ci, getcolor(g_col, g_row));
                if (windows == 0)
                {
                    /* show temp msg will clear self if new msg is a
                       different length - pad to length 40*/
                    while ((int)std::strlen(str) < 40)
                    {
                        std::strcat(str, " ");
                    }
                    str[40] = 0;
                    Cursor_Hide();
                    actively_computing = true;
                    showtempmsg(str);
                    Cursor_Show();
                }
                else
                {
                    driver_display_string(5, g_vesa_y_res-show_numbers, WHITE, BLACK, str);
                }
            }
            iter = 1;
            g_l_old_z.y = 0;
            g_l_old_z.x = g_l_old_z.y;
            g_old_z.y = g_l_old_z.x;
            g_old_z.x = g_old_z.y;
            g_init.x = cr;
            g_save_c.x = g_init.x;
            g_init.y = ci;
            g_save_c.y = g_init.y;
            g_l_init.x = (long)(g_init.x*g_fudge_factor);
            g_l_init.y = (long)(g_init.y*g_fudge_factor);

            old_y = -1;
            old_x = old_y;
            /*
             * MIIM code:
             * compute fixed points and use them as starting points of JIIM
             */
            if (which == jiim_types::JIIM && OKtoMIIM)
            {
                DComplex f1, f2, Sqrt;        // Fixed points of Julia

                Sqrt = ComplexSqrtFloat(1 - 4 * cr, -4 * ci);
                f1.x = (1 + Sqrt.x) / 2;
                f2.x = (1 - Sqrt.x) / 2;
                f1.y =  Sqrt.y / 2;
                f2.y = -Sqrt.y / 2;

                ClearQueue();
                maxhits = 1;
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
            if (windows == 0 && g_col > xc && g_col < xc+xd && g_row > yc && g_row < yc+yd)
            {
                RestoreRect(xc, yc, xd, yd);
                if (xc == g_video_start_x + xd*2)
                {
                    xc = g_video_start_x + 2;
                }
                else
                {
                    xc = g_video_start_x + xd*2;
                }
                xoff = xc + xd /  2;
                SaveRect(xc, yc, xd, yd);
            }
            if (windows == 2)
            {
                fillrect(g_logical_screen_x_dots, yc, xd-g_logical_screen_x_dots, yd-show_numbers, g_color_dark);
                fillrect(xc   , g_logical_screen_y_dots, g_logical_screen_x_dots, yd-g_logical_screen_y_dots-show_numbers, g_color_dark);
            }
            else
            {
                fillrect(xc, yc, xd, yd, g_color_dark);
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
            if (OKtoMIIM)
            {
                if (QueueEmpty())
                {
                    if (maxhits < g_colors - 1
                        && maxhits < 5
                        && (luckyx != 0.0 || luckyy != 0.0))
                    {
                        lmax = 0;
                        lsize = lmax;
                        g_new_z.x = luckyx;
                        g_old_z.x = g_new_z.x;
                        g_new_z.y = luckyy;
                        g_old_z.y = g_new_z.y;
                        luckyy = 0.0F;
                        luckyx = luckyy;
                        for (int i = 0; i < 199; i++)
                        {
                            g_old_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
                            g_new_z = ComplexSqrtFloat(g_new_z.x - cr, g_new_z.y - ci);
                            EnQueueFloat((float)g_new_z.x, (float)g_new_z.y);
                            EnQueueFloat((float)-g_old_z.x, (float)-g_old_z.y);
                        }
                        maxhits++;
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
                if (color < maxhits)
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
                    g_old_z.x = g_old_z.y; // avoids math error
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


                switch (SecretExperimentalMode)
                {
                case 0:                     // unmodified random walk
                default:
                    if (rand() % 2)
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
                            xbase = x;
                            ybase = y;
                            circle((int)(zoom*(xd >> 1)/iter), color);
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
                        if (rand() % 2)
                        {
                            g_new_z.x = -g_new_z.x;
                            g_new_z.y = -g_new_z.y;
                        }
                        if (++rancnt > 1024)
                        {
                            rancnt = 0;
                            if (rand() % 2)
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
                            randir = rancnt;
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
                x = y;
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
                xbase = x;
                ybase = y;
                circle((int)(zoom*(xd >> 1)/iter), color);
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
        Cursor_Hide();
        if (windows == 0)
        {
            RestoreRect(xc, yc, xd, yd);
        }
        else if (windows >= 2)
        {
            if (windows == 2)
            {
                fillrect(g_logical_screen_x_dots, yc, xd-g_logical_screen_x_dots, yd, g_color_dark);
                fillrect(xc   , g_logical_screen_y_dots, g_logical_screen_x_dots, yd-g_logical_screen_y_dots, g_color_dark);
            }
            else
            {
                fillrect(xc, yc, xd, yd, g_color_dark);
            }
            if (windows == 3 && xd == g_vesa_x_res) // unhide
            {
                RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
                windows = 2;
            }
            Cursor_Hide();
            bool const savehasinverse = g_has_inverse;
            g_has_inverse = true;
            SaveRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
            g_logical_screen_x_offset = oldsxoffs;
            g_logical_screen_y_offset = oldsyoffs;
            RestoreRect(0, 0, g_logical_screen_x_dots, g_logical_screen_y_dots);
            g_has_inverse = savehasinverse;
        }
    }
#ifdef XFRACT
    Cursor_EndMouseTracking();
#endif
    g_line_buff.clear();
    screen_rect.clear();
    g_look_at_mouse = old_look_at_mouse;
    g_using_jiim = false;
    g_calc_type = oldcalctype;
    g_debug_flag = old_debugflag;
    g_help_mode = old_help_mode;
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
    show_numbers = 0;
    driver_unget_key(kbdchar);

    if (g_cur_fractal_specific->calctype == calcfroth)
    {
        froth_cleanup();
    }
}
